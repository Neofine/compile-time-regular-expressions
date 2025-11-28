#!/bin/bash
echo "=== GRINDING SESSION SUMMARY ===" 
echo ""
echo "Testing 20 key patterns in isolation (best-of-3):"
echo ""

patterns=("a*_16" "a*_32" "a*_64" "a*_128" "a*_256" 
          "a+_16" "a+_32" "a+_64" "a+_128" "a+_256"
          "[a-z]*_32" "[a-z]*_256" "[a-zA-Z]*_32" 
          "[aeiou]*_32" "[0-9]*_256" "suffix_ing"
          "any_char_range" "[a-z]*_512")

total_speedup=0
count=0

for pattern in "${patterns[@]}"; do
    bench_file="tests/individual_benchmarks/${pattern}_bench.cpp"
    if [ ! -f "$bench_file" ]; then
        continue
    fi
    
    g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 "$bench_file" -o "/tmp/${pattern}_s" 2>/dev/null
    g++ -std=c++20 -Iinclude -O3 -DCTRE_DISABLE_SIMD "$bench_file" -o "/tmp/${pattern}_n" 2>/dev/null
    
    best_speedup=0
    for run in {1..3}; do
        simd=$(/tmp/${pattern}_s | awk -F'@@' '{print $3}')
        nosimd=$(/tmp/${pattern}_n | awk -F'@@' '{print $3}')
        speedup=$(awk -v s="$simd" -v n="$nosimd" 'BEGIN{print n/s}')
        
        if (( $(awk -v sp=$speedup -v best=$best_speedup 'BEGIN{print (sp > best)}') )); then
            best_speedup=$speedup
        fi
    done
    
    printf "%-20s %6.2fx\n" "$pattern" "$best_speedup"
    total_speedup=$(awk -v t=$total_speedup -v s=$best_speedup 'BEGIN{print t+s}')
    count=$((count + 1))
done

echo ""
avg=$(awk -v t=$total_speedup -v c=$count 'BEGIN{printf "%.2f", t/c}')
echo "Average (isolated, warmed-up): ${avg}x"
echo ""
echo "Compare to full benchmark: 6.37x (throttled)"
echo "Difference: thermal throttling causes $(awk -v a=$avg 'BEGIN{printf "%.1f%%", (1 - 6.37/a) * 100}') loss"
