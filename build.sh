#!/bin/bash
# "pandaheroast" contract name macro: MY_CT_AST
trap 'exit' INT 
CONTRACT='cryptoart'
eosio-cpp \
  -abigen \
  -D MY_CT_AST=pandaheroast \
  -I include \
  -R src \
  -contract $CONTRACT \
  -o $CONTRACT.wasm \
  src/$CONTRACT.cpp 
if [ $? -eq 0 ]; then
  echo -e "\033[32m√ build crypto.art success.\n \033[0m"
else
  echo -e "\033[31m× build crypto.art failed.\n \033[0m"
fi
