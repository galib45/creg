#! /bin/bash
set -xe
mkdir -p build
gcc -Wall -Wextra -o build/main main.c
