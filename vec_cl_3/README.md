# vec_cl_3

 Version: 0.9.1

 date    : 2026/09/25
 
 update :

***

C++ CLI , Jev + Vector Search

* TypeSafe Jev
* llama-server : Qwen3-Embedding-0.6B
* OpenRouter 
* LLVM CLang
* make
* Linux

***
### related DB Server

https://github.com/kuc-arc-f/cpp_vec_server_db1

***

* llama-server
```
/usr/local/llama-b8951/llama-server -m /var/lm_data/qwen/Qwen3-Embedding-0.6B-Q8_0.gguf --embedding  -c 4096 --port 8080
```
***
* LIB

```
sudo apt update
sudo apt install uuid-dev
sudo apt install nlohmann-json3-dev
sudo apt install libcurl4-openssl-dev
```
***
* embed build
```
clang++ -std=c++17 -I./include -o embed embed.cpp -lcurl
```

* search
```
clang++ -std=c++17 -I./include -o search search.cpp -lcurl
```
***
* .env
```
OPENROUTER_API_KEY=
OPENROUTER_MODEL=deepseek/deepseek-v4-flash
```
***
* vector data add
```
./embed ./data
```
***
* search start
```
./search hello
```
***
