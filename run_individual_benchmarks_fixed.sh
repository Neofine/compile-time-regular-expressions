#!/bin/bash

cd /root/compile-time-regular-expressions

echo "Compiling and running 81 individual benchmarks..."
echo "Using TAB delimiter to avoid | conflicts"
echo ""

# Create results directory
mkdir -p results/individual
rm -f results/individual/all_results.tsv

# Compile and run each benchmark
total=0
success=0

for bench_file in tests/individual_benchmarks/*_bench.cpp; do
    total=$((total + 1))
    name=$(basename "$bench_file" _bench.cpp)
    
    # Compile SIMD version
    if g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 -flto \
        "$bench_file" -o "/tmp/${name}_simd" 2>/dev/null; then
        
        # Compile NoSIMD version
        if g++ -std=c++20 -Iinclude -O3 -DCTRE_DISABLE_SIMD \
            "$bench_file" -o "/tmp/${name}_nosimd" 2>/dev/null; then
            
            # Run both and capture results
            simd_output=$(/tmp/${name}_simd)
            nosimd_output=$(/tmp/${name}_nosimd)
            
            # Parse results (using | from benchmark output)
            simd_time=$(echo "$simd_output" | cut -d'|' -f3)
            nosimd_time=$(echo "$nosimd_output" | cut -d'|' -f3)
            pattern=$(echo "$simd_output" | cut -d'|' -f2)
            desc=$(echo "$simd_output" | cut -d'|' -f4)
            
            # Calculate speedup
            speedup=$(echo "scale=2; $nosimd_time / $simd_time" | bc)
            
            # Use TAB as delimiter for output
            printf "%s\t%s\t%s\t%s\t%s\t%s\n" "$name" "$pattern" "$simd_time" "$desc" "$nosimd_time" "$speedup" >> results/individual/all_results.tsv
            success=$((success + 1))
            
            printf "\r[%d/%d] %s: %.2fx" $success $total "$name" $speedup
        fi
    fi
done

echo ""
echo ""
echo "Results saved to results/individual/all_results.tsv"
