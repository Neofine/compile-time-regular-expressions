#!/bin/bash

cd /root/compile-time-regular-expressions

echo "Compiling and running individual benchmarks..."
echo "Using @@ delimiter to handle patterns with special chars"
echo ""

# Create results directory
mkdir -p results/individual
rm -f results/individual/all_results.txt

# Compile and run each benchmark
total=0
success=0

for bench_file in tests/individual_benchmarks/*_bench.cpp; do
    total=$((total + 1))
    name=$(basename "$bench_file" _bench.cpp)
    
    # Compile SIMD version
    if g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 \
        "$bench_file" -o "/tmp/${name}_simd" 2>/dev/null; then
        
        # Compile NoSIMD version
        if g++ -std=c++20 -Iinclude -O3 -DCTRE_DISABLE_SIMD \
            "$bench_file" -o "/tmp/${name}_nosimd" 2>/dev/null; then
            
            # Run both and capture results
            simd_result=$(/tmp/${name}_simd)
            nosimd_result=$(/tmp/${name}_nosimd)
            
            # Parse results using @@ delimiter
            simd_time=$(echo "$simd_result" | awk -F'@@' '{print $3}')
            nosimd_time=$(echo "$nosimd_result" | awk -F'@@' '{print $3}')
            
            # Calculate speedup using awk (more robust than bc)
            speedup=$(awk -v n="$nosimd_time" -v s="$simd_time" 'BEGIN {printf "%.2f", n/s}')
            
            # Save with @@ delimiter
            echo "${simd_result}@@${nosimd_time}@@${speedup}x" >> results/individual/all_results.txt
            success=$((success + 1))
            
            printf "\r[%d/%d] %s: %sx" $success $total "$name" "$speedup"
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
awk -F'@@' '{
    name=$1
    pattern=$2  
    simd=$3
    desc=$4
    nosimd=$5
    speedup=$6
    
    gsub(/x$/, "", speedup)
    
    printf "%-40s %8.2f ns  %8.2f ns  %8sx\n", name, simd, nosimd, speedup
}' results/individual/all_results.txt | sort -t' ' -k6 -n | tail -20

echo ""
echo "Bottom 10 (slowest speedups):"
awk -F'@@' '{
    gsub(/x$/, "", $6)
    printf "%s|%.2f\n", $1, $6
}' results/individual/all_results.txt | sort -t'|' -k2 -n | head -10 | \
awk -F'|' '{printf "%-40s %6.2fx\n", $1, $2}'

echo ""
echo "Top 10 (best speedups):"
awk -F'@@' '{
    gsub(/x$/, "", $6)
    printf "%s|%.2f\n", $1, $6
}' results/individual/all_results.txt | sort -t'|' -k2 -n | tail -10 | \
awk -F'|' '{printf "%-40s %6.2fx\n", $1, $2}'

# Calculate overall average speedup
echo ""
awk -F'@@' '{
    gsub(/x$/, "", $6)
    sum += $6
    count++
}
END {
    avg = sum / count
    printf "Overall Average Speedup: %.2fx (%d patterns)\n", avg, count
}' results/individual/all_results.txt
