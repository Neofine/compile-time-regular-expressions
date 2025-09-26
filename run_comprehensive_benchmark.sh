#!/bin/bash

# Comprehensive SIMD vs Non-SIMD Benchmark Script
# Produces a minimalistic table comparing performance across different test cases

set -e

echo "Building comprehensive benchmark..."

# Build SIMD version
g++ -std=c++20 -O3 -march=native -Iinclude tests/comprehensive_benchmark.cpp -o tests/comprehensive_benchmark_simd -lstdc++

# Build non-SIMD version
g++ -std=c++20 -O3 -march=native -Iinclude -DCTRE_DISABLE_SIMD tests/comprehensive_benchmark.cpp -o tests/comprehensive_benchmark_nosimd -lstdc++

echo "Running benchmarks..."

# Run SIMD version and capture results
echo "Running SIMD benchmark..."
./tests/comprehensive_benchmark_simd > simd_results.csv

# Run non-SIMD version and capture results
echo "Running non-SIMD benchmark..."
./tests/comprehensive_benchmark_nosimd > nosimd_results.csv

# Process results and create comparison table
echo ""
echo "=========================================="
echo "SIMD vs Non-SIMD Performance Comparison"
echo "=========================================="
printf "%-20s %-12s %-12s %-8s\n" "Test Type" "SIMD (ns)" "Non-SIMD (ns)" "Speedup"
echo "------------------------------------------"

# Create associative arrays to store results
declare -A simd_times
declare -A nosimd_times

# Read SIMD results
while IFS=',' read -r test_name simd_time; do
    if [[ -n "$test_name" && -n "$simd_time" && "$simd_time" != "inf" && "$simd_time" != "0" ]]; then
        simd_times["$test_name"]="$simd_time"
    fi
done < simd_results.csv

# Read non-SIMD results
while IFS=',' read -r test_name nosimd_time; do
    if [[ -n "$test_name" && -n "$nosimd_time" && "$nosimd_time" != "inf" && "$nosimd_time" != "0" ]]; then
        nosimd_times["$test_name"]="$nosimd_time"
    fi
done < nosimd_results.csv

# Print comparison table
for test_name in "${!simd_times[@]}"; do
    if [[ -n "${nosimd_times[$test_name]}" ]]; then
        simd_time="${simd_times[$test_name]}"
        nosimd_time="${nosimd_times[$test_name]}"
        speedup=$(echo "scale=2; $nosimd_time / $simd_time" | bc -l)
        printf "%-20s %-12.2f %-12.2f %-8.2fx\n" "$test_name" "$simd_time" "$nosimd_time" "$speedup"
    fi
done

echo "------------------------------------------"

# Calculate overall statistics
echo ""
echo "Overall Statistics:"
total_simd=0
total_nosimd=0
count=0

for test_name in "${!simd_times[@]}"; do
    if [[ -n "${nosimd_times[$test_name]}" ]]; then
        total_simd=$(echo "$total_simd + ${simd_times[$test_name]}" | bc -l)
        total_nosimd=$(echo "$total_nosimd + ${nosimd_times[$test_name]}" | bc -l)
        count=$((count + 1))
    fi
done

if [[ $count -gt 0 ]]; then
    overall_speedup=$(echo "scale=2; $total_nosimd / $total_simd" | bc -l)
    echo "Total SIMD time: ${total_simd} ns"
    echo "Total Non-SIMD time: ${total_nosimd} ns"
    echo "Overall speedup: ${overall_speedup}x"
    echo "Number of tests: $count"
else
    echo "Unable to calculate overall statistics (no matching tests found)"
fi

# Cleanup
rm -f simd_results.csv nosimd_results.csv
rm -f tests/comprehensive_benchmark_simd tests/comprehensive_benchmark_nosimd

echo ""
echo "Benchmark completed successfully!"