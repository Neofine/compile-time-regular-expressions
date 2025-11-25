#!/bin/bash

set -e

echo "Building character-class expansion benchmark..."

# Build benchmark
g++ -std=c++20 -Iinclude -O3 -march=native -mtune=native -mavx2 -msse4.2 -mfma -mbmi2 -mlzcnt -mpopcnt \
    -funroll-loops -ffast-math -flto \
    tests/char_expansion_benchmark.cpp -o tests/benchmark-exec/char_expansion_benchmark -lstdc++

echo "Running benchmark..."

# Run benchmark
./tests/benchmark-exec/char_expansion_benchmark

echo ""
