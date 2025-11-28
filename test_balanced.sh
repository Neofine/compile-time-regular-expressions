#!/bin/bash
echo "╔══════════════════════════════════════════════════════════╗"
echo "║        BALANCED SIMPLIFICATION TESTING                   ║"
echo "╚══════════════════════════════════════════════════════════╝"
echo ""

# Critical patterns to watch
CRITICAL=("negated_class" "complex_alt" "a*_16" "a+_16")

echo "Testing critical patterns:"
echo "=========================="
for pattern in "${CRITICAL[@]}"; do
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

echo ""
echo "Testing sample of other sizes:"
echo "==============================="
for pattern in "a+_32" "a*_64" "a*_256" "[a-z]*_512"; do
    bench_file="tests/individual_benchmarks/${pattern}_bench.cpp"
    if [ -f "$bench_file" ]; then
        g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 \
            "$bench_file" -o "/tmp/${pattern}_test" 2>/dev/null
        result=$(/tmp/${pattern}_test 2>&1 | awk -F'@@' '{print $1 ": " $3 "ns"}')
        echo "  $result"
    fi
done

