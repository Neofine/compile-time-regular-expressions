#!/bin/bash

set -e

echo "Building CTRE regex benchmarks..."

# Build SIMD version
g++ -std=c++20 -Iinclude -Isrell_include -O3 -march=native -mtune=native -mavx2 -msse4.2 -mfma -mbmi2 -mlzcnt -mpopcnt \
    -funroll-loops -ffast-math -flto \
    tests/ctre_proper_benchmark.cpp -o tests/ctre_proper_benchmark_simd -lstdc++

# Build non-SIMD version
g++ -std=c++20 -Iinclude -Isrell_include -O3 -march=native -mtune=native -mavx2 -msse4.2 -mfma -mbmi2 -mlzcnt -mpopcnt \
    -funroll-loops -ffast-math -flto -DCTRE_DISABLE_SIMD \
    tests/ctre_proper_benchmark.cpp -o tests/ctre_proper_benchmark_nosimd -lstdc++

echo "Running benchmarks..."

# Run SIMD version and capture results
./tests/ctre_proper_benchmark_simd > simd_results.csv

# Run non-SIMD version and capture results
./tests/ctre_proper_benchmark_nosimd > nosimd_results.csv

# Process results and create comparison table
echo ""
echo "=========================================================================="
echo "CTRE Regex Performance Comparison"
echo "=========================================================================="
printf "%-20s %-15s %-15s %-10s\n" "Pattern" "Before (ns)" "After (ns)" "Speedup"
echo "--------------------------------------------------------------------------"

# Create associative arrays to store results
declare -A simd_times
declare -A nosimd_times

# Read SIMD results (skip first 3 lines and last 2 lines which are headers/footers)
tail -n +4 simd_results.csv | head -n -2 | while IFS= read -r line; do
    # Parse pattern and time from "pattern    time" format
    pattern=$(echo "$line" | awk '{print $1}')
    time=$(echo "$line" | awk '{print $2}')
    if [[ -n "$pattern" && -n "$time" ]]; then
        simd_times["$pattern"]="$time"
    fi
done

# Read non-SIMD results
tail -n +4 nosimd_results.csv | head -n -2 | while IFS= read -r line; do
    pattern=$(echo "$line" | awk '{print $1}')
    time=$(echo "$line" | awk '{print $2}')
    if [[ -n "$pattern" && -n "$time" ]]; then
        nosimd_times["$pattern"]="$time"
    fi
done

# Store results in temporary files to access them (subshell issue with while loop)
tail -n +4 simd_results.csv | head -n -2 | awk '{print $1","$2}' > /tmp/simd_parsed.csv
tail -n +4 nosimd_results.csv | head -n -2 | awk '{print $1","$2}' > /tmp/nosimd_parsed.csv

declare -A simd_times_final
declare -A nosimd_times_final

while IFS=',' read -r pattern time; do
    simd_times_final["$pattern"]="$time"
done < /tmp/simd_parsed.csv

while IFS=',' read -r pattern time; do
    nosimd_times_final["$pattern"]="$time"
done < /tmp/nosimd_parsed.csv

# Print comparison table
for pattern in "${!simd_times_final[@]}"; do
    if [[ -n "${nosimd_times_final[$pattern]}" ]]; then
        simd_time="${simd_times_final[$pattern]}"
        nosimd_time="${nosimd_times_final[$pattern]}"
        speedup=$(echo "scale=2; $nosimd_time / $simd_time" | bc -l)
        printf "%-20s %-15.2f %-15.2f %-10.2fx\n" "$pattern" "$nosimd_time" "$simd_time" "$speedup"
    fi
done

echo "--------------------------------------------------------------------------"

# Calculate overall statistics
echo ""
echo "Overall Statistics:"
total_simd=0
total_nosimd=0
count=0

for pattern in "${!simd_times_final[@]}"; do
    if [[ -n "${nosimd_times_final[$pattern]}" ]]; then
        total_simd=$(echo "$total_simd + ${simd_times_final[$pattern]}" | bc -l)
        total_nosimd=$(echo "$total_nosimd + ${nosimd_times_final[$pattern]}" | bc -l)
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
rm -f simd_results.csv nosimd_results.csv /tmp/simd_parsed.csv /tmp/nosimd_parsed.csv
rm -f tests/ctre_proper_benchmark_simd tests/ctre_proper_benchmark_nosimd

echo ""
echo "CTRE regex benchmark completed!"
