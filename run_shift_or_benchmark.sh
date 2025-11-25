#!/bin/bash

set -e

echo "Building Shift-Or benchmarks..."

# Build SIMD version
g++ -std=c++20 -Iinclude -Isrell_include -O3 -march=native -mtune=native -mavx2 -msse4.2 -mfma -mbmi2 -mlzcnt -mpopcnt \
    -funroll-loops -ffast-math -flto \
    tests/shift_or_benchmark.cpp -o tests/shift_or_benchmark_simd -lstdc++

# Build non-SIMD version
g++ -std=c++20 -Iinclude -Isrell_include -O3 -march=native -mtune=native -mavx2 -msse4.2 -mfma -mbmi2 -mlzcnt -mpopcnt \
    -funroll-loops -ffast-math -flto -DCTRE_DISABLE_SIMD \
    tests/shift_or_benchmark.cpp -o tests/shift_or_benchmark_nosimd -lstdc++

echo "Running benchmarks..."

# Run SIMD version and capture results
./tests/shift_or_benchmark_simd > simd_results.csv

# Run non-SIMD version and capture results
./tests/shift_or_benchmark_nosimd > nosimd_results.csv

# Process results and create comparison table
echo ""
echo "=========================================================================="
echo "Shift-Or String Matching Performance Comparison"
echo "=========================================================================="
printf "%-20s %-15s %-15s %-10s\n" "Pattern" "Before (ns)" "After (ns)" "Speedup"
echo "--------------------------------------------------------------------------"

# Create associative arrays to store results
declare -A simd_times
declare -A nosimd_times

# Read SIMD results
while IFS=',' read -r pattern_name simd_time; do
    if [[ -n "$pattern_name" && -n "$simd_time" && "$simd_time" != "inf" && "$simd_time" != "0" ]]; then
        simd_times["$pattern_name"]="$simd_time"
    fi
done < simd_results.csv

# Read non-SIMD results
while IFS=',' read -r pattern_name nosimd_time; do
    if [[ -n "$pattern_name" && -n "$nosimd_time" && "$nosimd_time" != "inf" && "$nosimd_time" != "0" ]]; then
        nosimd_times["$pattern_name"]="$nosimd_time"
    fi
done < nosimd_results.csv

# Print comparison table
for pattern_name in "${!simd_times[@]}"; do
    if [[ -n "${nosimd_times[$pattern_name]}" ]]; then
        simd_time="${simd_times[$pattern_name]}"
        nosimd_time="${nosimd_times[$pattern_name]}"
        speedup=$(echo "scale=2; $nosimd_time / $simd_time" | bc -l)
        printf "%-20s %-15.2f %-15.2f %-10.2fx\n" "$pattern_name" "$nosimd_time" "$simd_time" "$speedup"
    fi
done

echo "--------------------------------------------------------------------------"

# Calculate overall statistics
echo ""
echo "Overall Statistics:"
total_simd=0
total_nosimd=0
count=0

for pattern_name in "${!simd_times[@]}"; do
    if [[ -n "${nosimd_times[$pattern_name]}" ]]; then
        total_simd=$(echo "$total_simd + ${simd_times[$pattern_name]}" | bc -l)
        total_nosimd=$(echo "$total_nosimd + ${nosimd_times[$pattern_name]}" | bc -l)
        count=$((count + 1))
    fi
done

if [[ $count -gt 0 ]]; then
    overall_speedup=$(echo "scale=2; $total_nosimd / $total_simd" | bc -l)
    echo "Total Non-SIMD time: ${total_nosimd} ns"
    echo "Total SIMD time: ${total_simd} ns"
    echo "Overall speedup: ${overall_speedup}x"
    echo "Number of patterns: $count"
fi

# Cleanup
rm -f simd_results.csv nosimd_results.csv
rm -f tests/shift_or_benchmark_simd tests/shift_or_benchmark_nosimd

echo ""
echo "Shift-Or benchmark completed!"
