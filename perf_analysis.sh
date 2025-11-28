#!/bin/bash
echo "=== PERFORMANCE COUNTER ANALYSIS ==="
echo ""

for pattern in "a*_16" "a*_256" "[a-z]*_32"; do
    echo "===== Pattern: $pattern ====="
    
    # Compile
    g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 \
        "tests/individual_benchmarks/${pattern}_bench.cpp" \
        -o "/tmp/${pattern}_perf" 2>/dev/null
    
    # Run with perf stat
    echo "Running perf stat..."
    perf stat -e cycles,instructions,branches,branch-misses,cache-references,cache-misses,L1-dcache-loads,L1-dcache-load-misses \
        /tmp/${pattern}_perf 2>&1 | grep -E "cycles|instructions|branches|cache|Performance"
    
    echo ""
done
