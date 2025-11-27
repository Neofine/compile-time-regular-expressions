#!/bin/bash
set -e

echo "=========================================="
echo "PGO vs Baseline Comparison"
echo "=========================================="
echo ""

# Build baseline (normal)
echo "Building baseline (no PGO)..."
g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 -msse4.2 \
    tests/master_benchmark.cpp -o /tmp/ctre_baseline 2>&1 | head -10

echo "Building baseline with SIMD disabled..."
g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 -msse4.2 \
    -DCTRE_DISABLE_SIMD \
    tests/master_benchmark.cpp -o /tmp/ctre_baseline_nosimd 2>&1 | head -10

echo ""
echo "Running baseline benchmark..."
BASELINE_SIMD=$(/tmp/ctre_baseline 2>&1 | grep "^Total time:" | awk '{print $3}')
BASELINE_NOSIMD=$(/tmp/ctre_baseline_nosimd 2>&1 | grep "^Total time:" | awk '{print $3}')

echo "Running PGO benchmark..."
PGO_SIMD=$(/tmp/ctre_pgo 2>&1 | grep "^Total time:" | awk '{print $3}')

echo ""
echo "=========================================="
echo "Results"
echo "=========================================="
printf "%-25s %10s\n" "Configuration" "Time (ns)"
printf "%-25s %10s\n" "-------------------------" "----------"
printf "%-25s %10s\n" "Baseline (no SIMD)" "$BASELINE_NOSIMD"
printf "%-25s %10s\n" "Baseline (SIMD)" "$BASELINE_SIMD"
printf "%-25s %10s\n" "PGO (SIMD)" "$PGO_SIMD"

echo ""
BASELINE_SPEEDUP=$(echo "scale=2; $BASELINE_NOSIMD / $BASELINE_SIMD" | bc -l)
PGO_SPEEDUP=$(echo "scale=2; $BASELINE_NOSIMD / $PGO_SIMD" | bc -l)
IMPROVEMENT=$(echo "scale=2; ($BASELINE_SIMD - $PGO_SIMD) / $BASELINE_SIMD * 100" | bc -l)

echo "Baseline SIMD speedup:  ${BASELINE_SPEEDUP}x"
echo "PGO SIMD speedup:       ${PGO_SPEEDUP}x"
echo "PGO improvement:        ${IMPROVEMENT}%"

# Binary sizes
SIZE_BASELINE=$(stat -c%s /tmp/ctre_baseline)
SIZE_PGO=$(stat -c%s /tmp/ctre_pgo)
SIZE_REDUCTION=$(echo "scale=1; (1 - $SIZE_PGO/$SIZE_BASELINE) * 100" | bc -l)

echo ""
echo "Binary size baseline:   $SIZE_BASELINE bytes"
echo "Binary size PGO:        $SIZE_PGO bytes"
echo "Size reduction:         ${SIZE_REDUCTION}%"

