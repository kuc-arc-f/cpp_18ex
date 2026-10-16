#pragma once
#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

using json = nlohmann::json;

class JevClient {
private:
    std::string m_name;
public:
    explicit JevClient(std::string str){}
    ~JevClient() {}

        // libcurl のレスポンス書き込みコールバック
    static size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata) {
        const size_t total = size * nmemb;
        static_cast<std::string*>(userdata)->append(ptr, total);
        return total;
    }


    // HTTP POST (JSON) を実行してレスポンス本文を返す
    std::string http_post_json(const std::string& url,
                              const std::string& bearer_token,
                              const json& body) {
        CURL* curl = curl_easy_init();
        if (!curl) throw std::runtime_error("curl_easy_init failed");

        const std::string payload = body.dump();

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        const std::string auth = "Authorization: Bearer " + bearer_token;
        headers = curl_slist_append(headers, auth.c_str());

        std::string response;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(payload.size()));
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "cpp-openrouter-client/1.0");

        const CURLcode rc = curl_easy_perform(curl);

        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (rc != CURLE_OK) {
            throw std::runtime_error(std::string("curl error: ") + curl_easy_strerror(rc));
        }
        if (http_code < 200 || http_code >= 300) {
            throw std::runtime_error("HTTP " + std::to_string(http_code) +
                                    " body: " + response);
        }
        return response;
    }    

};
