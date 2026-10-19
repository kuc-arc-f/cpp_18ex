# redis_cl_11

 Version: 0.9.1

 date    : 2026/10/11 
 
 update :

***

C++ CLI , Jev + Vector Search Redis database

* TypeSafe Jev
* llama-server : embeddinggemma-2-GGUF
* OpenRouter 
* LLVM CLang
* make
* Linux

***
### related API

https://github.com/kuc-arc-f/rs_vec_redis_rest

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
PREFIX_KEY=doc1:
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
### blog

https://zenn.dev/link/comments/257a86ed675f11

