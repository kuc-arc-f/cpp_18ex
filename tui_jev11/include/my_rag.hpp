#pragma once
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>
#include <map>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include "models.hpp"
#include "my_config.hpp"
#include "openrouter_client.hpp"
#include "HttpClient.hpp"
#include "JevClient.hpp"
#include "pg_search.hpp"

using json = nlohmann::json;

std::string PG_CONN_STR = "host=localhost port=5432 dbname=mydb user=root password=admin";

//const std::string DATA_PATH = "./data";
std::string TABLE_NAME = "document";
std::string API_URL_BASE = "http://localhost:8888";

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

class MyRag {
private:
    std::string m_name;

    public:
    explicit MyRag(std::string str){}

    ~MyRag() {}

    std::string llm_search(std::string query, std::string input){
        std::string ret = "";
        try{
            // API_KEYを環境変数から取得（または直接設定）
            const char* api_key = std::getenv("OPENROUTER_API_KEY");
            if (api_key != nullptr) {
                //std::cout << "api_key:" << api_key << std::endl;
            }else{
                std::cerr << "Error: OPENROUTER_API_KEY environment variable not set" << std::endl;
                std::cerr << "Please set it with: export OPENROUTER_API_KEY=your_api_key_here" << std::endl;
                return ret;
            }      
            const char* model_name = std::getenv("OPENROUTER_MODEL");
            if (!model_name) {
                std::cerr << "Error: OPENROUTER_MODEL environment variable not set" << std::endl;
                return ret;
            }          
            std::string resp_str= input;
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
            //std::cout << out_str  << std::endl; 
            OpenRouterClient client(api_key);
            auto response = client.sendChatCompletion(
                model_name,
                out_str,
                1.0,
                2000
            );

            if (response.has_value()) {
                //std::cout << "Response: " << response.value() << std::endl;
                auto outStr = response.value();
                ret = outStr;
                return ret;
            } else {
                std::cerr << "Failed to get response from API" << std::endl;
                return ret;
            }

            return ret;
        } catch (const std::exception& e) {
            std::cout << "Error , main" << std::endl;
        } 
        return ret; 
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

    std::string rag_search(std::string query){
        std::string ret = "";
        try{
            HttpClient htClient(30 /*timeout*/, true /*verify_ssl*/);         
            // API_KEYを環境変数から取得（または直接設定）
            const char* api_key = std::getenv("OPENROUTER_API_KEY");
            if (api_key != nullptr) {
                //std::cout << "api_key:" << api_key << std::endl;
            }else{
                std::cerr << "Error: OPENROUTER_API_KEY environment variable not set" << std::endl;
                std::cerr << "Please set it with: export OPENROUTER_API_KEY=your_api_key_here" << std::endl;
                return ret;
            }      
            const char* model_name = std::getenv("OPENROUTER_MODEL");
            if (!model_name) {
                std::cerr << "Error: OPENROUTER_MODEL environment variable not set" << std::endl;
                return ret;
            }      
            struct QueryReq req_data;
            req_data.input = query;  
            json j_req = req_data;
            std::string j4 = j_req.dump();
            std::cout << "json_str : " << j4 << "\n";
            auto res2 = htClient.post_json("http://localhost:8080/embedding", j4);
            if (!res2.error.empty()) {
                std::cerr << "[ERROR] " << res2.error << "\n";
                return ret;
            }        
            std::cout << "Status : " << res2.status_code << "\n";
            if (res2.is_ok() == false) {
                std::cerr << "[ERROR] Status NG \n";
                return ret;
            }
            std::string str = res2.body;
            json j = json::parse(str);
            auto embedding = j[0]["embedding"];
            auto vec = embedding[0];
            int vlength = sizeof(vec) / sizeof(vec[0]);
            std::cout << "vlen=" << vec.size() << std::endl;            
            std::stringstream ss;
            ss << "[";
            for (size_t i = 0; i < vec.size(); ++i) {
                if (i > 0) ss << ",";
                ss << vec[i];
            }
            ss << "]";
            std::string emb_str = ss.str();  
            std::string res1 = emb_str.substr(0, 40);      
            std::cout << "res1=" << res1 << std::endl; 
            std::vector<float> vec2;
            for(const auto& value : vec) {
                vec2.push_back(value.get<float>());
            }             
            auto embeddings = vec2;   
            PgSearch pLib("");
            std::string out_str = "日本語で、回答して欲しい。 \n要約して欲しい。\n\n";
            std::string resp_str = "";
            auto respList = pLib.rag_search_items(embeddings, PG_CONN_STR);
            std::cout << "respList.size=" << respList.size()  << std::endl;
            std::string content = ""; 
            
            if (respList.size() == 0){
                std::cout << "========\n" << "\n";
                std::cout << "result none \n" << "\n";
                return ret;
            }
            if (respList.size() == 1){
                SearchDbItem row = respList[0];
                std::string target = row.content;
                content = target;
            }
            if(respList.size() >=2){
                std::vector<std::string> strJev;
                for(int i=0; i < respList.size(); i++){
                SearchDbItem row = respList[i];
                std::string target = row.content;
                strJev.push_back(target);
                }
                std::cout << "strJev.size=" << strJev.size()  << std::endl;
                content = get_jev_item(api_key, strJev, query);
            }            
            std::cout << "\n===============" << "\n"; 
            std::cout << "result:" << "\n"; 
            std::cout << content << "\n";
            ret = content; 
            return ret;
        } catch (const std::exception& e) {
            std::cout << "Error , main" << std::endl;
        }  
        return ret;        
    }
};
