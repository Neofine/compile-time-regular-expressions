#!/bin/bash
echo "=== PROFILING SIMD HOTSPOTS WITH PERF ==="
echo ""

# Check if perf is available
if ! command -v perf &> /dev/null; then
    echo "perf not available, using alternative approach..."
    echo "Will use cycle counting and assembly inspection"
    exit 1
fi

# Compile benchmark with debug symbols
echo "Step 1: Compiling with debug symbols..."
g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 -g tests/master_benchmark.cpp -o /tmp/bench_profile 2>&1 | head -5

echo ""
echo "Step 2: Running perf record..."
sudo perf record -F 99 -g /tmp/bench_profile 2>&1 | head -10

echo ""
echo "Step 3: Analyzing hotspots..."
sudo perf report --stdio -n --sort symbol | head -50
