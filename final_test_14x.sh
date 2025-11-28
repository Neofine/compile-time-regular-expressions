#!/bin/bash
echo "=== COMPREHENSIVE TEST FOR 14X (5 runs per pattern) ==="
echo ""

patterns=("a*_16" "a*_32" "a*_64" "a*_128" "a*_256" "a+_16" "a+_32" "a+_64" "a+_128" "a+_256" "[a-z]*_32" "[a-z]*_64" "[a-z]*_256" "[a-z]*_512" "[a-zA-Z]*_32" "[aeiou]*_32" "[aeiou]*_64" "[0-9]*_256" "suffix_ing" "any_char_range")

total_speedup=0
count=0

for pattern in "${patterns[@]}"; do
    if [ -f "tests/individual_benchmarks/${pattern}_bench.cpp" ]; then
        g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 "tests/individual_benchmarks/${pattern}_bench.cpp" -o "/tmp/${pattern}_s" 2>/dev/null
        g++ -std=c++20 -Iinclude -O3 -DCTRE_DISABLE_SIMD "tests/individual_benchmarks/${pattern}_bench.cpp" -o "/tmp/${pattern}_n" 2>/dev/null
        
        # Run 5 times, take average
        sum=0
        for i in {1..5}; do
            simd=$(/tmp/${pattern}_s | awk -F'@@' '{print $3}')
            nosimd=$(/tmp/${pattern}_n | awk -F'@@' '{print $3}')
            speedup=$(awk -v s="$simd" -v n="$nosimd" 'BEGIN{printf "%.2f", n/s}')
            sum=$(awk -v sum=$sum -v sp=$speedup 'BEGIN{printf "%.2f", sum+sp}')
        done
        
        avg=$(awk -v sum=$sum 'BEGIN{printf "%.2f", sum/5}')
        printf "%-15s  %6.2fx\n" "$pattern" "$avg"
        
        total_speedup=$(awk -v tot=$total_speedup -v avg=$avg 'BEGIN{printf "%.2f", tot+avg}')
        count=$((count + 1))
    fi
done

overall_avg=$(awk -v tot=$total_speedup -v cnt=$count 'BEGIN{printf "%.2fx", tot/cnt}')
echo ""
echo "========================================="
echo "OVERALL AVERAGE: $overall_avg"
echo "========================================="

if (( $(awk -v avg=${overall_avg%x} 'BEGIN{print (avg >= 14)}') )); then
    echo "🎉🎉🎉 14X ACHIEVED! 🎉🎉🎉"
else
    gap=$(awk -v avg=${overall_avg%x} 'BEGIN{printf "%.1f%%", (14/avg - 1) * 100}')
    echo "Gap to 14x: $gap"
fi
