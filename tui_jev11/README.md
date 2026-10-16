# tui_jev11

 Version: 0.9.1

 date    : 2026/10/16
 
 update :

***

C++ TUI , Jev + Vector Search PGVector

* TypeSafe Jev
* PGVector Database
* llama-server : embeddinggemma-2-GGUF
* OpenRouter 
* LLVM CLang
* make
* Linux

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
* Table:  table.sql

***
* LIB

```
sudo apt update
sudo apt install uuid-dev
sudo apt install libpq-dev libpqxx-dev
sudo apt install nlohmann-json3-dev
sudo apt install libcurl4-openssl-dev
```

***
* embed build
```
clang++ -std=c++17 -I./include -o embed embed.cpp -lcurl -lpqxx -lpq
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
* build, lib 
* node start

```
make all
pnpm i
npm run start
```

***
### blog

https://zenn.dev/link/comments/4cfb71a5103258
