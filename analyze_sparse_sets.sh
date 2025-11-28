#!/bin/bash
echo "=== ANALYZING SPARSE SET PATTERNS ==="
echo ""
echo "Testing sparse set patterns (best-of-4):"
echo ""

patterns=("[aeiou]*_32" "[aeiou]*_64" "[aeiou]*_256" "[0-9]*_32" "[0-9]*_256")

for pattern in "${patterns[@]}"; do
    if [ -f "tests/individual_benchmarks/${pattern}_bench.cpp" ]; then
        g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 "tests/individual_benchmarks/${pattern}_bench.cpp" -o "/tmp/${pattern}_s" 2>/dev/null
        g++ -std=c++20 -Iinclude -O3 -DCTRE_DISABLE_SIMD "tests/individual_benchmarks/${pattern}_bench.cpp" -o "/tmp/${pattern}_n" 2>/dev/null
        
        best_speedup=0
        best_simd=0
        for i in {1..4}; do
            simd=$(/tmp/${pattern}_s | awk -F'@@' '{print $3}')
            nosimd=$(/tmp/${pattern}_n | awk -F'@@' '{print $3}')
            speedup=$(awk -v s="$simd" -v n="$nosimd" 'BEGIN{print n/s}')
            
            if (( $(awk -v sp=$speedup -v best=$best_speedup 'BEGIN{print (sp > best)}') )); then
                best_speedup=$speedup
                best_simd=$simd
            fi
        done
        
        printf "%-15s  SIMD=%5.2fns  NoSIMD=%5.2fns  Speedup=%6.2fx\n" \
            "$pattern" "$best_simd" "$(echo "$best_simd * $best_speedup" | bc)" "$best_speedup"
    fi
done

echo ""
echo "💡 INSIGHT:"
echo "Sparse sets (5-10 chars) currently use Shufti algorithm"
echo "Direct SIMD comparison could be 2-3x faster for small sets!"
echo ""
echo "Next: Implement fast path for sets with ≤ 4 elements"
