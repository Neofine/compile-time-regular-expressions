#!/bin/bash
echo "=== TESTING BRANCH HINTS IMPACT (10 patterns, 5 runs each) ==="
echo ""

patterns=("a*_32" "a*_64" "a*_128" "a*_256" "a+_32" "a+_64" "[a-z]*_32" "[a-z]*_64" "[a-z]*_256" "[aeiou]*_32")

for pattern in "${patterns[@]}"; do
    g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 "tests/individual_benchmarks/${pattern}_bench.cpp" -o "/tmp/${pattern}_s" 2>/dev/null
    g++ -std=c++20 -Iinclude -O3 -DCTRE_DISABLE_SIMD "tests/individual_benchmarks/${pattern}_bench.cpp" -o "/tmp/${pattern}_n" 2>/dev/null
    
    # Run 5 times, collect all speedups
    speedups=""
    for i in {1..5}; do
        simd=$(/tmp/${pattern}_s | awk -F'@@' '{print $3}')
        nosimd=$(/tmp/${pattern}_n | awk -F'@@' '{print $3}')
        speedup=$(awk -v s="$simd" -v n="$nosimd" 'BEGIN{printf "%.2f", n/s}')
        speedups="$speedups $speedup"
    done
    
    # Calculate average
    avg=$(echo $speedups | awk '{sum=0; for(i=1;i<=NF;i++) sum+=$i; print sum/NF}')
    best=$(echo $speedups | tr ' ' '\n' | sort -rn | head -1)
    
    printf "%-15s  Best=%6.2fx  Avg=%6.2fx  Runs: [%s]\n" "$pattern" "$best" "$avg" "$speedups"
done
