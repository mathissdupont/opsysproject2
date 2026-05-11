#!/usr/bin/env bash

set -euo pipefail

echo "Compiling projects..."
gcc -Wall -Wextra -pedantic -std=c11 it_support.c -o it_support -lpthread
gcc -Wall -Wextra -pedantic -std=c11 ai_researchers.c -o ai_researchers -lpthread

echo -e "\n--- Running Problem 1: Sleeping IT Support ---"
echo "12" | ./it_support

echo -e "\n--- Running Problem 2: AI Researchers and GPUs ---"
echo "0" | ./ai_researchers

echo -e "\nAll tests completed!"
