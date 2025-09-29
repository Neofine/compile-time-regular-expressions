#!/bin/bash

echo "Building proper CTRE regex benchmark..."

# Build the benchmark
g++ -std=c++20 -Iinclude -Isrell_include -O3 -pedantic -Wall -Wextra -Werror -Wconversion \
    -march=native -mtune=native -mavx2 -msse4.2 -mfma -mbmi2 -mlzcnt -mpopcnt \
    -funroll-loops -ffast-math -flto \
    tests/ctre_proper_benchmark.cpp -o tests/ctre_proper_benchmark

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo "Running proper CTRE regex benchmark..."
echo ""

# Run the benchmark
./tests/ctre_proper_benchmark

echo ""
echo "Proper CTRE regex benchmark completed!"
