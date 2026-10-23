# vec_cl_21

 Version: 0.9.1

 date    : 2026/10/23 
 
 update :

***

C++ CLI , Jev + Vector Search

* TypeSafe Jev
* llama-server : embeddinggemma-2-GGUF
* OpenRouter 
* LLVM CLang
* Linux

***
### related DB Server

https://github.com/kuc-arc-f/rs_vec_db_rest

***
### related LLM

* huggingface

https://huggingface.co/unsloth/embeddinggemma-2-GGUF

* unsloth

https://unsloth.ai/docs/jp/moderu/embeddinggemma-2

***

* llama-server
* ctx-size: 2048
```
/usr/local/llama-b11541/llama-server -m /var/lm_data/unsloth/embeddinggemma-2-BF16.gguf --embedding \
 --ctx-size 2048 \
 --batch-size 2048 \
 --ubatch-size 2048  \
 --port 8080
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
