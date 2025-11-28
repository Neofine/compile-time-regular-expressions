#!/bin/bash
echo "Comparing SIMD vs NoSIMD for problematic patterns..."
echo "===================================================="
echo ""

for pattern in "negated_class" "complex_alt" "a*_16" "a+_16"; do
    echo "Testing: $pattern"
    
    # SIMD version
    bench_file="tests/individual_benchmarks/${pattern}_bench.cpp"
    if [ -f "$bench_file" ]; then
        g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 \
            "$bench_file" -o "/tmp/${pattern}_simd" 2>/dev/null
        simd_time=$(/tmp/${pattern}_simd 2>&1 | awk -F'@@' '{print $3}')
        
        # NoSIMD version
        g++ -DCTRE_DISABLE_SIMD -std=c++20 -Iinclude -O3 \
            "$bench_file" -o "/tmp/${pattern}_nosimd" 2>/dev/null
        nosimd_time=$(/tmp/${pattern}_nosimd 2>&1 | awk -F'@@' '{print $3}')
        
        echo "  SIMD:   ${simd_time}ns"
        echo "  NoSIMD: ${nosimd_time}ns"
        
        if [ -n "$simd_time" ] && [ -n "$nosimd_time" ]; then
            speedup=$(python3 -c "print(f'{float($nosimd_time)/float($simd_time):.2f}x')")
            echo "  Speedup: $speedup"
        fi
        echo ""
    fi
done

