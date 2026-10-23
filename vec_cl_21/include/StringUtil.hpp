#pragma once
#include <iostream>
#include <string>

class StringUtil {
private:
    std::string m_name;

public:
    explicit StringUtil(std::string str){}
    ~StringUtil() {}

    // UTF-8の1文字分のバイト数を判定する関数
    size_t get_utf8_char_len(unsigned char c) {
        if (c < 0x80) return 1;       // 1バイト文字 (ASCII)
        else if ((c & 0xE0) == 0xC0) return 2; // 2バイト文字
        else if ((c & 0xF0) == 0xE0) return 3; // 3バイト文字 (日本語の多く)
        else if ((c & 0xF8) == 0xF0) return 4; // 4バイト文字
        return 1;
    }

    // 先頭から指定文字数分の部分文字列を取得する関数
    std::string get_top_chars(const std::string& str, size_t num_chars) {
        size_t byte_index = 0;
        size_t current_chars = 0;

        while (byte_index < str.length() && current_chars < num_chars) {
            size_t char_len = get_utf8_char_len(str[byte_index]);
            byte_index += char_len;
            current_chars++;
        }

        return str.substr(0, byte_index);
    }
    
};
