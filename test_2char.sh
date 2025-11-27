#!/bin/bash
cd /root/compile-time-regular-expressions

echo "=== Testing 2-Char Range SIMD ===" > /tmp/test_2char.txt

# Compile
g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 tests/master_benchmark.cpp -o /tmp/bench_2char 2>&1
if [ $? -eq 0 ]; then
    echo "✓ Compilation PASSED" >> /tmp/test_2char.txt

    # Run benchmark
    timeout 30 /tmp/bench_2char >> /tmp/test_2char.txt 2>&1
    if [ $? -eq 0 ]; then
        echo "✓ Benchmark completed" >> /tmp/test_2char.txt
    else
        echo "✗ Benchmark FAILED" >> /tmp/test_2char.txt
    fi
else
    echo "✗ Compilation FAILED" >> /tmp/test_2char.txt
fi
