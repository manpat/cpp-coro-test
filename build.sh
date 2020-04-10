#!/bin/bash

flags="-std=c++17 -g -Wall -Wextra -fcoroutines-ts -stdlib=libc++ -lpthread"

clang++ $flags *.cpp -obuild && ./build