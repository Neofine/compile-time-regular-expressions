#!/bin/bash
echo "Checking small pattern performance..."
echo "====================================="
echo ""

# Test the problematic patterns
for pattern in "negated_class" "complex_alt" "a*_16" "a+_16"; do
    bench_file="tests/individual_benchmarks/${pattern}_bench.cpp"
    if [ -f "$bench_file" ]; then
        g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 \
            "$bench_file" -o "/tmp/${pattern}_test" 2>/dev/null
        if [ $? -eq 0 ]; then
            result=$(/tmp/${pattern}_test 2>&1)
            echo "$result"
        fi
    fi
done

