#!/bin/bash
set -e

# Build both versions
g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 -msse4.2 \
    tests/ctre_proper_benchmark.cpp -o /tmp/ctre_simd 2>/dev/null

g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 -msse4.2 -DCTRE_DISABLE_SIMD \
    tests/ctre_proper_benchmark.cpp -o /tmp/ctre_nosimd 2>/dev/null

# Run and collect results
/tmp/ctre_nosimd > /tmp/nosimd.csv
/tmp/ctre_simd > /tmp/simd.csv

# Print header
printf "%-20s %-15s %-15s %-10s\n" "Pattern" "Before (ns)" "After (ns)" "Speedup"
echo "--------------------------------------------------------------------------"

# Process results
total_nosimd=0
total_simd=0
count=0

while IFS=',' read -r pattern nosimd_time; do
    # Look up matching simd time
    simd_time=$(grep "^$pattern," /tmp/simd.csv | cut -d',' -f2)
    
    if [[ -n "$simd_time" && "$nosimd_time" != *"inf"* && "$simd_time" != *"inf"* ]]; then
        speedup=$(awk -v n="$nosimd_time" -v s="$simd_time" 'BEGIN {printf "%.2f", n/s}')
        printf "%-20s %-15.2f %-15.2f %-10s x\n" "$pattern" "$nosimd_time" "$simd_time" "$speedup"
        
        total_nosimd=$(awk -v t="$total_nosimd" -v n="$nosimd_time" 'BEGIN {print t+n}')
        total_simd=$(awk -v t="$total_simd" -v s="$simd_time" 'BEGIN {print t+s}')
        count=$((count + 1))
    fi
done < /tmp/nosimd.csv | sort

echo "--------------------------------------------------------------------------"
echo ""
echo "Overall Statistics:"
printf "Total Non-SIMD time: %.2f ns\n" "$total_nosimd"
printf "Total SIMD time: %.2f ns\n" "$total_simd"
overall=$(awk -v n="$total_nosimd" -v s="$total_simd" 'BEGIN {printf "%.2f", n/s}')
echo "Overall speedup: ${overall}x"
echo "Number of patterns: $count"

# Cleanup
rm -f /tmp/ctre_simd /tmp/ctre_nosimd /tmp/simd.csv /tmp/nosimd.csv
