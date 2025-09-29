#!/bin/bash

echo "Building Shift-Or string matching benchmark..."

# Build the benchmark
g++ -std=c++20 -Iinclude -Isrell_include -O3 -pedantic -Wall -Wextra -Werror -Wconversion \
    -march=native -mtune=native -mavx2 -msse4.2 -mfma -mbmi2 -mlzcnt -mpopcnt \
    -funroll-loops -ffast-math -flto \
    tests/shift_or_proper_benchmark.cpp -o tests/shift_or_proper_benchmark

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo "Running Shift-Or string matching benchmark..."
echo ""

# Run the benchmark
./tests/shift_or_proper_benchmark

echo ""
echo "Shift-Or string matching benchmark completed!"
