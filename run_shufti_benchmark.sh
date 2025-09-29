#!/bin/bash

# SHUFTI (SIMD Character-Class Matching) Benchmark Script
# Tests the real SHUFTI algorithm for character classes like [A-Za-z0-9_], \s, etc.

set -e

echo "Building SHUFTI benchmark..."

# Build SIMD version
g++ -std=c++20 -O3 -march=native -mtune=native -mavx2 -msse4.2 -mfma -mbmi2 -mlzcnt -mpopcnt -funroll-loops -ffast-math -flto -Iinclude tests/shufti_benchmark.cpp -o tests/shufti_benchmark_simd -lstdc++

# Build non-SIMD version
g++ -std=c++20 -O3 -march=native -mtune=native -mavx2 -msse4.2 -mfma -mbmi2 -mlzcnt -mpopcnt -funroll-loops -ffast-math -flto -Iinclude -DCTRE_DISABLE_SIMD tests/shufti_benchmark.cpp -o tests/shufti_benchmark_nosimd -lstdc++

echo "Running benchmarks..."

# Run SIMD version and capture results
./tests/shufti_benchmark_simd > simd_results.csv

# Run non-SIMD version and capture results
./tests/shufti_benchmark_nosimd > nosimd_results.csv

# Process results and create comparison table
echo ""
echo "SHUFTI (SIMD Character-Class Matching) Performance Comparison"
echo "============================================================="
printf "%-15s %-10s %-10s %-8s\n" "Pattern" "SIMD" "Non-SIMD" "Speedup"
echo "------------------------------------------------------------"

# Create associative arrays to store results
declare -A simd_times
declare -A nosimd_times

# Read SIMD results (skip header)
while IFS=',' read -r pattern_name simd_time; do
    if [[ -n "$pattern_name" && -n "$simd_time" && "$simd_time" != "inf" && "$simd_time" != "0" && "$pattern_name" != "Pattern" && "$pattern_name" != "Time(ns)" ]]; then
        simd_times["$pattern_name"]="$simd_time"
    fi
done < <(tail -n +2 simd_results.csv)

# Read non-SIMD results (skip header)
while IFS=',' read -r pattern_name nosimd_time; do
    if [[ -n "$pattern_name" && -n "$nosimd_time" && "$nosimd_time" != "inf" && "$nosimd_time" != "0" && "$pattern_name" != "Pattern" && "$pattern_name" != "Time(ns)" ]]; then
        nosimd_times["$pattern_name"]="$nosimd_time"
    fi
done < <(tail -n +2 nosimd_results.csv)

# Print comparison table in the exact format requested
for pattern_name in "${!simd_times[@]}"; do
    if [[ -n "${nosimd_times[$pattern_name]}" ]]; then
        simd_time="${simd_times[$pattern_name]}"
        nosimd_time="${nosimd_times[$pattern_name]}"
        speedup=$(echo "scale=2; $nosimd_time / $simd_time" | bc -l 2>/dev/null || echo "0")
        printf "%-15s %-10.2f %-10.2f %-8.2f x\n" "$pattern_name" "$simd_time" "$nosimd_time" "$speedup"
    fi
done

echo "------------------------------------------------------------"

# Calculate overall statistics
total_simd=0
total_nosimd=0
count=0

for pattern_name in "${!simd_times[@]}"; do
    if [[ -n "${nosimd_times[$pattern_name]}" ]]; then
        total_simd=$(echo "$total_simd + ${simd_times[$pattern_name]}" | bc -l 2>/dev/null || echo "0")
        total_nosimd=$(echo "$total_nosimd + ${nosimd_times[$pattern_name]}" | bc -l 2>/dev/null || echo "0")
        count=$((count + 1))
    fi
done

if [[ $count -gt 0 ]]; then
    overall_speedup=$(echo "scale=2; $total_nosimd / $total_simd" | bc -l 2>/dev/null || echo "N/A")
    echo ""
    echo "Overall Statistics:"
    echo "Total SIMD time: ${total_simd} ns"
    echo "Total Non-SIMD time: ${total_nosimd} ns"
    echo "Overall speedup: ${overall_speedup}x"
    echo "Number of patterns: $count"

    # Show character class specific analysis
    echo ""
    echo "Character Class Analysis:"
    echo "========================"

    # Find SHUFTI vs regex comparisons
    for pattern_name in "${!simd_times[@]}"; do
        if [[ -n "${nosimd_times[$pattern_name]}" ]]; then
            if [[ "$pattern_name" == *"shufti"* ]]; then
                simd_time="${simd_times[$pattern_name]}"
                nosimd_time="${nosimd_times[$pattern_name]}"
                speedup=$(echo "scale=2; $nosimd_time / $simd_time" | bc -l 2>/dev/null || echo "0")
                printf "%-20s %-8.2f x (SHUFTI vs Regex)\n" "$pattern_name" "$speedup"
            fi
        fi
    done
fi

# Cleanup
rm -f simd_results.csv nosimd_results.csv
rm -f tests/shufti_benchmark_simd tests/shufti_benchmark_nosimd

echo ""
echo "SHUFTI benchmark completed successfully!"
