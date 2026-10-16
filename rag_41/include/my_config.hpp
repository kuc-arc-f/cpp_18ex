#pragma once
#include <iostream>

class MyConfig {
private:

public:
    int EMBED_SIZE = 3072;

    explicit MyConfig(std::string str){}
    ~MyConfig() {}
};
