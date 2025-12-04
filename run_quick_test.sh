#!/bin/bash
# Quick test to verify SIMD code compiles and works
set -e
cd "$(dirname "$0")"

echo "=== Quick SIMD Test ==="

# Test 1: Compile check
echo "[1/2] Compiling SIMD benchmark..."
cd plots
g++ -std=c++20 -O3 -march=native \
    -I../include -I./lib/include \
    benchmarks/thesis_benchmark.cpp \
    -L./lib/lib -lre2 -lpcre2-8 -lhs \
    -Wl,-rpath,./lib/lib \
    -o build/bench_simd 2>&1

echo "[2/2] Running quick benchmark (Simple category only)..."
export LD_LIBRARY_PATH="./lib/lib:$LD_LIBRARY_PATH"
./build/bench_simd Simple 2>&1 | head -50

echo ""
echo "✓ SIMD code compiles and runs!"


