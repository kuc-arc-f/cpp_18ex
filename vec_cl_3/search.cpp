#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <stdexcept>
#include <vector>
#include <curl/curl.h>
#include <nlohmann/json.hpp> // JSONライブラリ

#include "models.hpp"
#include "include/EmbeddingClient.hpp"
#include "include/openrouter_client.hpp"
#include "include/HttpClient.hpp"
#include "include/JevClient.hpp"
#include "include/dotenv.h"

using namespace std;

using json = nlohmann::json;

std::string TABLE_NAME = "document";
std::string API_URL_BASE = "http://localhost:8888";

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
struct ChatRequest {
    std::string model;
    std::vector<ChatQuery> messages;
    double temperature;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ChatRequest, model, messages, temperature)

struct SearchResponse {
    std::string ret;
    std::string text;
};   
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SearchResponse, ret, text)

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

std::string get_max_score(std::string json_str ){
    std::string ret = "";
    try{    
        json j = json::parse(json_str);

        // probabilities を取得
        const auto& probabilities = j.at("answers").at("document").at("probabilities");
        const auto& legend = j.at("answers").at("document").at("legend");

        std::cout << "--- 最大確率のラベル ---" << std::endl;
        std::string best_key;
        double best_prob = -1.0;
        for (auto it = probabilities.begin(); it != probabilities.end(); ++it) {
            double p = it.value().get<double>();
            if (p > best_prob) {
                best_prob = p;
                best_key = it.key();
            }
        }
        std::string outStr = legend.at(best_key).get<std::string>();
        //std::cout << "Best: " << outStr
        //          << " (" << best_prob << ")" << std::endl;
        return outStr;
    } catch (const json::exception& e) {
        std::cerr << "JSON error: " << e.what() << std::endl;
    }
    return ret;
}

std::string get_jev_item(
    std::string api_key, std::vector<std::string> vec, std::string query)
{
    std::string ret = "";
    curl_global_init(CURL_GLOBAL_DEFAULT);
    try {
        json body = {
            {"model", "typesafe/jev-1.13"},
            {"state", {
                {"document", "似ている文章を探して欲しい。"}
            }},
            {"questions", {
                {"document", {
                    {"type", "score"},
                    {"instructions", query},
                    {
                    "criteria", vec
                    }
                }}
            }}
        };
        //std::cout << "body=" << body << std::endl;  
        JevClient jLib("");          
        const std::string resp = jLib.http_post_json(
            "https://openrouter.ai/api/alpha/decisions",
            api_key,
            body);

        // レスポンスをパースして表示
        json parsed = json::parse(resp);
        std::cout << parsed.dump(2) << std::endl;

        std::string outStr = get_max_score(resp);
        std::cout << "outStr=" <<  outStr << std::endl;
        return outStr;
    } catch (const std::exception& e) {
        std::cerr << "get_jev_item Error: " << e.what() << std::endl;
        curl_global_cleanup();
        return ret;
    }
    curl_global_cleanup();    
    return ret;
}
// ─────────────────────────────────────────────
// main : 動作確認サンプル
// ─────────────────────────────────────────────
int main(int argc, char* argv[])
{
    dotenv::init();
    const char* api_key = std::getenv("OPENROUTER_API_KEY");
    if (api_key != nullptr) {
        //std::cout << "api_key:" << api_key << std::endl;
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
    //std::cout << "argv[0]=" << argv[0] << "\n";
    if(argc < 2) {
        std::cerr << "[ERROR] argment none" << "\n";
        return -1;
    }
    int arg_count = argc;
    std::string query = argv[1];

    try{
        HttpClient htClient(30 /*timeout*/, true /*verify_ssl*/);         
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
            std::cerr << "[ERROR] Status NG \n";
            return -1;
        }
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
        VectorSearchReq req1;
        req1.table = TABLE_NAME;
        req1.limit = 3;
        req1.vector = emb_str;
        json j1 = req1;
        std::string json_str = j1.dump();
        //std::cout << json_str << std::endl;
        std::string url = API_URL_BASE + "/api/select";

        auto resp = htClient.post_json(url, json_str);
        //print_response("POST-JSON:", resp);        
        //std::cout << "body=" << resp.body << std::endl;
        json j4 = json::parse(resp.body);
        std::string data = j4.at("data").get<std::string>();
        std::cout << "data=" << data << "\n";        
        json j3 = json::parse(data);
        std::cout << "j3.size=" << j3.size() << "\n";
        std::string content = "";        
        if (j3.size() == 0){
            std::cout << "========\n" << "\n";
            std::cout << "result none \n" << "\n";
            return -1;
        }
        if (j3.size() == 1){
            content = j3[0].at("content").get<std::string>();
        }
        // Jev score
        std::vector<std::string> vecJev;
        if(j3.size() >=2){
            for(int i=0; i < j3.size(); i++) {
                std::string row = j3[i].at("content").get<std::string>();
                vecJev.push_back(row);
            }
            content = get_jev_item(api_key, vecJev, query);
        }
        std::cout << "\n===============" << "\n"; 
        std::cout << "result:" << "\n"; 
        std::cout << content << "\n";         
        //std::cout << "\ncontent=" << content << "\n"; 
    } catch (const std::exception& e) {
        std::cout << "Error , main" << std::endl;
        return 0;
    }        
    return 0;
}

