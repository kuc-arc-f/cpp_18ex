#include <chrono>
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <map>
#include <stdexcept>
#include <vector>
#include <curl/curl.h>
#include <nlohmann/json.hpp> // JSONライブラリ
#include <random>

#include "include/models.hpp"
#include "include/HttpClient.hpp"
#include "pg_search.hpp"
#include "include/dotenv.h"
#include "include/openrouter_client.hpp"

using namespace std;

using json = nlohmann::json;

std::string PG_CONN_STR = "host=localhost port=5432 dbname=mydb user=root password=admin";

struct QueryReq {
    std::string input;
};   
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(QueryReq, input)
struct ChatQuery {
    std::string role;
    std::string content;
};
// これ一行で、QueryReq <=> json の変換が魔法のように可能になります
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ChatQuery, role, content)

std::string extractContent(const std::string& jsonStr)
{
    try {
        auto j = nlohmann::json::parse(jsonStr);
        return j["choices"][0]["message"]["content"].get<std::string>();
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] JSON parse: " << e.what() << "\n";
        return "";
    }
}

// ─────────────────────────────────────────────
// main : 動作確認サンプル
// ─────────────────────────────────────────────
int main(int argc, char* argv[])
{
    dotenv::init();
    const char* api_key = std::getenv("OPENROUTER_API_KEY");
    if (api_key != nullptr) {
        std::cout << "OPENROUTER.api_key:" << api_key << std::endl;
    }else{
        std::cerr << "Error: OPENROUTER_API_KEY environment variable not set" << std::endl;
        std::cerr << "Please set it with: export OPENROUTER_API_KEY=your_api_key_here" << std::endl;
        return -1;
    }    
    const char* model_name = std::getenv("OPENROUTER_MODEL");
    if (!model_name) {
        std::cerr << "Error: OPENROUTER_MODEL environment variable not set" << std::endl;
        return -1;
    }    

    if(argc < 2) {
        std::cerr << "[ERROR] argment none" << "\n";
        return 0;
    }
    // 開始時刻
    auto start = std::chrono::high_resolution_clock::now();                

    HttpClient htClient(30 /*timeout*/, true /*verify_ssl*/); 
    int arg_count = argc;
    std::string query = argv[1];
    std::cout << "query=" << query << "\n";

    std::string api_url = "";
    try{
        struct QueryReq req_data;
        req_data.input = query; 
        json j_req = req_data;
        //std::cout << j_req.dump() << std::endl;
        std::string j2 = j_req.dump();
        std::cout << "json_str : " << j2 << "\n";
        auto res2 = htClient.post_json("http://localhost:8080/embedding", j2);
        if (!res2.error.empty()) {
            std::cerr << "[ERROR] " << res2.error << "\n";
            return 0;
        }
        std::cout << "Status : " << res2.status_code << "\n"; 
        if (res2.is_ok() == false) {
            std::cerr << "[ERROR] res2.is_ok = NG" << "\n";
            return -1;
        }
        std::string str = res2.body;
        json j = json::parse(str);
        auto embedding = j[0]["embedding"];
        auto vec = embedding[0];
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
        std::vector<float> vec2;
        for(const auto& value : vec) {
            vec2.push_back(value.get<float>());
        }             
        auto embeddings = vec2;
        PgSearch pLib("");
        std::string resp_str = "";
        auto respList = pLib.rag_search_items(embeddings, PG_CONN_STR);
        std::cout << "respList.size=" << respList.size()  << std::endl;
        std::string content = ""; 
        if (respList.size() == 0){
            std::cout << "========\n" << "\n";
            std::cout << "result none \n" << "\n";
            return -1;
        }
        SearchDbItem row = respList[0];
        std::string target = row.content;
        //std::cout << "target=" << target << "\n";
        resp_str = target;
        std::string out_str = "日本語で、回答して欲しい。 \n要約して欲しい。\n\n";
        if(resp_str.empty()){
            out_str.append("user query: ");
            out_str.append(query);
            out_str.append(" \n");
        }else{
            out_str.append("context:");
            out_str.append(resp_str);
            out_str.append("\n user query: ");
            out_str.append(query);
            out_str.append(" \n");
        }
        std::cout << out_str  << std::endl;
        OpenRouterClient client(api_key);
        // チャットリクエストの送信
        auto response = client.sendChatCompletion(
            model_name,
            out_str,
            1.0,
            2000
        );

        if (response.has_value()) {
            auto end = std::chrono::high_resolution_clock::now();
            // 差分（ミリ秒）
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            std::cout << "time_end: " << duration.count() << " ms" << std::endl;
            std::cout << "result==================" << std::endl;
            std::cout << "AI: " << response.value() << std::endl;

            return -1;
        } else {
            std::cerr << "Failed to get response from API" << std::endl;
            return -1;
        }
        return 1;
    } catch (const std::exception& e) {
        std::cout << "Error , main" << std::endl;
        return 0;
    }        
    return 0;
}


