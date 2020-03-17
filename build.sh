#!/bin/bash

trap 'exit' INT 
eosio-cpp -abigen -I ./include -R ./src -contract crypto.art -o  crypto.art.wasm ./src/crypto.art.cpp ./src/art.asset.cpp
if [ $? -eq 0 ]; then
  echo -e "\033[32m√ build crypto.art success.\n \033[0m"
else
  echo -e "\033[31m× build crypto.art failed.\n \033[0m"
fi
