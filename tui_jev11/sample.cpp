#include <iostream>
#include <string>
#include <cstring>
#include "include/dotenv.h"
#include "include/my_rag.hpp"

extern "C" {
    // メモリ解放用関数
    void free_string(char* ptr)
    {
        delete[] ptr;
    }

    char* rag_search(const char* input)
    {
        dotenv::init();
        std::string input_str(input);
        //std::cout << "todo_add.Received in C++: " << input_str << std::endl;
        MyRag rLib("");
        std::string result = rLib.rag_search(input_str);
        char* output = new char[result.length() + 1];
        strcpy(output, result.c_str());
        return output;    
    }

    int add(int a, int b)
    {
        return a + b;
    }

    // 文字列を受信（Rust → C++）
    void receive_string(const char* msg)
    {
        std::cout << "C++ received: " << msg << std::endl;
    }
    
    // 文字列を送信（C++ → Rust）
    // 注意：呼び出し側でメモリ解放が必要
    char* send_string()
    {
        std::string message = "Hello from C++!";
        char* result = new char[message.length() + 1];
        strcpy(result, message.c_str());
        return result;
    }
    
    // 送受信両方（Rustから文字列を受け取り、加工して返す）
    char* process_string(const char* input)
    {
        std::string input_str(input);
        std::string result = "C++ processed: [" + input_str + "]";
        
        char* output = new char[result.length() + 1];
        strcpy(output, result.c_str());
        return output;
    }
}