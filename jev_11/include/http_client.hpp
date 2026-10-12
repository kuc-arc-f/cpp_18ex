#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// コールバック関数：レスポンスデータを蓄積
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    size_t totalSize = size * nmemb;
    userp->append((char*)contents, totalSize);
    return totalSize;
}


// ------------------------------------------------------------
//
// ------------------------------------------------------------
class HttpClient {
public:
    // コンストラクタ
    // api_key: API キー (なければ空文字列)
    HttpClient(const std::string& host = "localhost",
                 int port = 6333,
                 const std::string& api_key = "")
        : base_url_("http://" + host + ":" + std::to_string(port)),
          api_key_(api_key)
    {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }

    ~HttpClient() {
        curl_global_cleanup();
    }

    // --------------------------------------------------------
    // ポイント（ベクトル）登録
    // points: { id -> { vector, payload } } のリスト
    // --------------------------------------------------------
    struct Point {
        uint64_t id;
        std::vector<float> vector;
        json payload;  // 任意のメタデータ
    };

    std::string post(const std::string& url, const std::string& body) {
        std::string ret = "";
        auto [status, resp] = request("POST", url, body);

        std::vector<SearchResult> results;
        if (status == 200) {
            ret = resp;
        } else {
            //std::cerr << "[ERROR] 検索失敗 (HTTP " << status << "): " << resp << "\n";
            std::wcerr << L"[ERROR] 検索失敗 (HTTP " << status << "): \n";
        }
        return ret;
    }
    // --------------------------------------------------------
    // ベクトル検索
    // query_vector : 検索クエリベクトル
    // top_k        : 上位何件を返すか
    // score_threshold: スコアの閾値 (optional)
    // with_payload : payload を含めるか
    // --------------------------------------------------------
    struct SearchResult {
        uint64_t id;
        float score;
        json payload;
    };

private:
    std::string base_url_;
    std::string api_key_;

    // HTTP リクエスト共通処理 (GET/POST/PUT/DELETE)
    std::pair<long, std::string> request(const std::string& method,
                                         const std::string& url,
                                         const std::string& body)
    {
        CURL* curl = curl_easy_init();
        if (!curl) throw std::runtime_error("curl_easy_init() 失敗");

        std::string response_body;
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        if (!api_key_.empty()) {
            headers = curl_slist_append(headers,
                ("api-key: " + api_key_).c_str());
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
//        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

        if (method == "POST") {
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)body.size());
        } else if (method == "PUT") {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)body.size());
        } else if (method == "DELETE") {
            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
        }
        // GET はデフォルト

        CURLcode res = curl_easy_perform(curl);
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            throw std::runtime_error(std::string("curl エラー: ") + curl_easy_strerror(res));
        }
        return {http_code, response_body};
    }
};
