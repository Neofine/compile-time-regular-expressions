#!/bin/bash

echo "=== ISOLATED PERFORMANCE TEST (REAL NUMBERS) ==="
echo ""

# Test all patterns that showed < 4x in full benchmark
patterns=(
    "a*_16" "a+_16" "9*_32" "9+_32" "z*_32" 
    "[0-2]*_32" "[0-2]+_32" "[02468]*_32" "[02468]+_32"
    "a*_32" "b*_32" "A*_32" "a+_32"
    "a*_64" "suffix_ing" "long_range_1"
)

for pattern in "${patterns[@]}"; do
    bench_file="tests/individual_benchmarks/${pattern}_bench.cpp"
    if [ ! -f "$bench_file" ]; then
        continue
    fi
    
    # Compile both versions
    g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 "$bench_file" -o "/tmp/${pattern}_s" 2>/dev/null
    g++ -std=c++20 -Iinclude -O3 -DCTRE_DISABLE_SIMD "$bench_file" -o "/tmp/${pattern}_n" 2>/dev/null
    
    # Run 3 times, take best (avoid cold start)
    best_speedup=0
    for run in {1..3}; do
        simd=$(/tmp/${pattern}_s | awk -F'@@' '{print $3}')
        nosimd=$(/tmp/${pattern}_n | awk -F'@@' '{print $3}')
        speedup=$(awk -v s="$simd" -v n="$nosimd" 'BEGIN{print n/s}')
        
        if (( $(awk -v sp=$speedup -v best=$best_speedup 'BEGIN{print (sp > best)}') )); then
            best_speedup=$speedup
            best_simd=$simd
            best_nosimd=$nosimd
        fi
    done
    
    # Color code: Red < 4x, Yellow 4-6x, Green > 6x
    if (( $(awk -v sp=$best_speedup 'BEGIN{print (sp < 4.0)}') )); then
        color="🔴"
    elif (( $(awk -v sp=$best_speedup 'BEGIN{print (sp < 6.0)}') )); then
        color="🟡"
    else
        color="🟢"
    fi
    
    printf "%s %-15s  SIMD=%6.2fns  NoSIMD=%6.2fns  Speedup=%5.2fx\n" \
        "$color" "$pattern" "$best_simd" "$best_nosimd" "$best_speedup"
done

echo ""
echo "🔴 = Need optimization (< 4x)"
echo "🟡 = Good but can improve (4-6x)"
echo "🟢 = Excellent (> 6x)"
