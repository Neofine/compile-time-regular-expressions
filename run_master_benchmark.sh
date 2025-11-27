#!/bin/bash
set -e

# Build both versions
g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 -msse4.2 \
    tests/master_benchmark.cpp -o /tmp/ctre_simd 2>/dev/null

g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 -msse4.2 -DCTRE_DISABLE_SIMD \
    tests/master_benchmark.cpp -o /tmp/ctre_nosimd 2>/dev/null

# Run and collect results
/tmp/ctre_nosimd > /tmp/nosimd.csv
/tmp/ctre_simd > /tmp/simd.csv

# Print header
printf "%-20s %-15s %-15s %-10s\n" "Pattern" "Before (ns)" "After (ns)" "Speedup"
echo "--------------------------------------------------------------------------"

# Process and store results
python3 << 'PYTHON'
import sys
import math

# Read data
nosimd = {}
simd = {}

with open('/tmp/nosimd.csv') as f:
    for line in f:
        parts = line.strip().split(',')
        if len(parts) == 2:
            try:
                val = float(parts[1])
                if not math.isinf(val) and not math.isnan(val):
                    nosimd[parts[0]] = val
            except:
                pass

with open('/tmp/simd.csv') as f:
    for line in f:
        parts = line.strip().split(',')
        if len(parts) == 2:
            try:
                val = float(parts[1])
                if not math.isinf(val) and not math.isnan(val):
                    simd[parts[0]] = val
            except:
                pass

# Calculate and print
results = []
total_nosimd = 0
total_simd = 0

for pattern in sorted(nosimd.keys()):
    if pattern in simd:
        ns = nosimd[pattern]
        s = simd[pattern]
        if s > 0 and ns > 0:
            speedup = ns / s
            results.append((pattern, ns, s, speedup))
            total_nosimd += ns
            total_simd += s

# Print results
for pattern, ns, s, speedup in results:
    print(f"{pattern:20s} {ns:15.2f} {s:15.2f} {speedup:10.2f} x")

print("-" * 74)
print()
print("Overall Statistics:")
print(f"Total Non-SIMD time: {total_nosimd:.2f} ns")
print(f"Total SIMD time: {total_simd:.2f} ns")
if total_simd > 0:
    print(f"Overall speedup: {total_nosimd/total_simd:.2f}x")
print(f"Number of patterns: {len(results)}")
PYTHON

# Cleanup
rm -f /tmp/ctre_simd /tmp/ctre_nosimd /tmp/simd.csv /tmp/nosimd.csv
