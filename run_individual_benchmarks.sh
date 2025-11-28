#!/bin/bash

cd /root/compile-time-regular-expressions

echo "Compiling and running 81 individual benchmarks..."
echo "Each binary will be small (< 32KB) to avoid I-cache issues"
echo ""

# Create results directory
mkdir -p results/individual

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
            simd_result=$(/tmp/${name}_simd)
            nosimd_result=$(/tmp/${name}_nosimd)
            
            # Parse results
            simd_time=$(echo "$simd_result" | cut -d'|' -f3)
            nosimd_time=$(echo "$nosimd_result" | cut -d'|' -f3)
            
            # Calculate speedup
            speedup=$(echo "scale=2; $nosimd_time / $simd_time" | bc)
            
            echo "$simd_result|$nosimd_time|${speedup}x" >> results/individual/all_results.txt
            success=$((success + 1))
            
            printf "\r[%d/%d] %s: %.2fx" $success $total "$name" $speedup
        fi
    fi
done

echo ""
echo ""
echo "Results saved to results/individual/all_results.txt"
echo ""

# Display summary
echo "Summary of Individual Benchmarks:"
echo "=================================="
awk -F'|' '{
    name=$1
    pattern=$2  
    simd=$3
    nosimd=$5
    speedup=$6
    
    gsub(/x$/, "", speedup)
    
    printf "%-40s %8.2f ns  %8.2f ns  %8s\n", name, simd, nosimd, speedup
}' results/individual/all_results.txt | sort -t' ' -k6 -n | tail -20

echo ""
echo "Bottom 10 (slowest speedups):"
awk -F'|' '{
    gsub(/x$/, "", $6)
    print $0
}' results/individual/all_results.txt | sort -t'|' -k6 -n | head -10 | \
awk -F'|' '{printf "%-30s %6s\n", $1, $6"x"}'

echo ""
echo "Top 10 (best speedups):"
awk -F'|' '{
    gsub(/x$/, "", $6)
    print $0
}' results/individual/all_results.txt | sort -t'|' -k6 -n | tail -10 | \
awk -F'|' '{printf "%-30s %6s\n", $1, $6"x"}'

# Calculate overall speedup
echo ""
awk -F'|' '{
    simd_sum += $3
    nosimd_sum += $5
    count++
}
END {
    overall = nosimd_sum / simd_sum
    printf "Overall Speedup: %.2fx (%d patterns)\n", overall, count
}' results/individual/all_results.txt

