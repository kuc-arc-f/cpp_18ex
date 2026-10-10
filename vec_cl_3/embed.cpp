
#include <fstream>
#include <filesystem>
#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <stdexcept>
#include <vector>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include "include/models.hpp"

#include "include/EmbeddingClient.hpp"
#include "include/HttpClient.hpp"
#include "include/dotenv.h"
#include "include/StringUtil.hpp"

using namespace std;

using json = nlohmann::json;

std::string TABLE_NAME = "document";
std::string API_URL_BASE = "http://localhost:8888";

struct QueryReq {
    std::string input;
};   
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(QueryReq, input)

/**
*
* @param
*
* @return
*/
int ebmed(std::string query){        
    try{
        HttpClient client(30 /*timeout*/, true /*verify_ssl*/);      
        int ret = 0;
        struct QueryReq req_data;
        req_data.input = query;        

        json j_req = req_data;
        //std::cout << j_req.dump() << std::endl;
        std::string j2 = j_req.dump();
        std::cout << "json_str : " << j2 << "\n";
        auto res2 = client.post_json("http://localhost:8080/embedding", j2);
        if (!res2.error.empty()) {
            std::cerr << "[ERROR] " << res2.error << "\n";
            return 0;
        }
        std::cout << "Status : " << res2.status_code << "\n";
        if (res2.is_ok() == false) {
            std::cout << "error ,  embedding res2.status_code = NG" << "\n";
            return 0;
        }        
        if (res2.is_ok()) {
            std::string str = res2.body;
            json j = json::parse(str);
            auto embedding = j[0]["embedding"];
            auto vec = embedding[0];
            int vlength = sizeof(vec) / sizeof(vec[0]);
            std::cout << "vlen=" << vec.size() << std::endl;            
            stringstream ss;
            ss << "[";
            for (size_t i = 0; i < vec.size(); ++i) {
                if (i > 0) ss << ",";
                ss << vec[i];
            }
            ss << "]";
            string emb_str = ss.str();  
            std::string res1 = emb_str.substr(0, 40);      
            std::cout << "res1=" << res1 << std::endl;   
            VectorCreateReq vdat;
            vdat.table = TABLE_NAME;
            vdat.content = query;
            vdat.vector = emb_str;
            json j1 = vdat;
            std::string json_str = j1.dump();
            //std::cout << json_str << std::endl;

            std::string url = API_URL_BASE + "/api/insert";

            auto resp = client.post_json(url, json_str);
            print_response("POST-JSON:", resp); 
            if (resp.is_ok() == false) {
                std::cout << "error ,  /api/insert resp.status_code = NG" << "\n";
                return 0;
            }                                 
        }
        return 1;
    } catch (const std::exception& e) {
        std::cout << "Error , main" << std::endl;
        std::cerr << "error: " << e.what() << std::endl;
        return 1;
    }        
    return 0;
}

// 1ファイル分のデータを保持する構造体
struct TextFile {
    std::string filename;
    std::vector<std::string> lines;
};

// .txt ファイルを読み込んで行を返す
TextFile loadTextFile(const std::filesystem::path& filepath) {
    TextFile tf;
    tf.filename = filepath.filename().string();

    std::ifstream ifs(filepath);
    if (!ifs.is_open()) {
        std::cerr << "[警告] ファイルを開けません: " << filepath << "\n";
        return tf;
    }

    std::string line;
    while (std::getline(ifs, line)) {
        tf.lines.push_back(line);
    }
    return tf;
}
void replace_all(std::string& str, const std::string& from, const std::string& to) {
    std::string::size_type pos = 0;
    while ((pos = str.find(from, pos)) != std::string::npos) {
        str.replace(pos, from.length(), to);
        pos += to.length(); // 置換後の文字列の長さ分進める
    }
}

// 読み込んだデータを表示する
void addTextFiles(const std::vector<TextFile>& files) {
    for (const auto& tf : files) {
        std::cout << "========================================\n";
        std::cout << "ファイル名: " << tf.filename << "\n";
        std::cout << "行数      : " << tf.lines.size() << "\n";
        std::cout << "----------------------------------------\n";
        std:string target = "";
        for (size_t i = 0; i < tf.lines.size(); ++i) {
            //std::cout << "[" << i + 1 << "] " << tf.lines[i] << "\n";
            std::string tmp = tf.lines[i] + "\n";
            replace_all(tmp, "'", "\"");
            target.append(tmp);
        }
        StringUtil sLib("");
        target =sLib.get_top_chars(target, 1000);
        //std::cout <<  target << "\n";
        std::cout <<  "target.size()=" << target.size() << "\n";
        int resp = ebmed(target);
        std::cout << "resp=" << resp << "\n";
        if(resp <= 0){
            std::cout << "error, addTextFiles.ebmed=0" << "\n";
            return;
        }        
    }
    std::cout << "========================================\n";
}
/**
*
* @param
*
* @return
*/
int main(int argc, char* argv[])
{
    dotenv::init();
    const char* api_key = std::getenv("OPENROUTER_API_KEY");
    if (api_key != nullptr) {
        std::cout << "api_key:" << api_key << std::endl;
    }else{
        std::cerr << "Error: OPENROUTER_API_KEY environment variable not set" << std::endl;
        std::cerr << "Please set it with: export OPENROUTER_API_KEY=your_api_key_here" << std::endl;
        return -1;
    }
    // 引数でフォルダを指定、省略時はカレントディレクトリ
    std::string dirPath = (argc >= 2) ? argv[1] : ".";
    if(argc < 2) {
        std::cerr << "[ERROR] argment none" << "\n";
        return 0;
    }    

    if (!std::filesystem::exists(dirPath) || !std::filesystem::is_directory(dirPath)) {
        std::cerr << "[エラー] 有効なディレクトリではありません: " << dirPath << "\n";
        return 1;
    }
    std::cout << "対象フォルダ: " << std::filesystem::absolute(dirPath) << "\n\n";    

    std::vector<TextFile> allFiles;
    // フォルダ内の .txt ファイルをすべて列挙
    for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
        if (entry.is_regular_file() && 
        (entry.path().extension() == ".txt" || entry.path().extension() == ".md") ) {
            TextFile tf = loadTextFile(entry.path());
            allFiles.push_back(std::move(tf));
        }
    }    
    if (allFiles.empty()) {
        std::cout << ".txt ファイルが見つかりませんでした。\n";
        return 0;
    }
    std::cout << "読み込んだファイル数: " << allFiles.size() << "\n\n";
    addTextFiles(allFiles);

    return 0;
}

