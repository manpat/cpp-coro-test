#!/bin/bash

flags="-std=c++2a -g -Wall -Wextra -fcoroutines-ts -stdlib=libc++ -lpthread"

clang++ $flags *.cpp -obuild && ./build