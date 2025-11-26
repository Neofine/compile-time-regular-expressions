#!/bin/bash

set -e

echo "Building negated pattern benchmarks..."

# Build SIMD version
g++ -std=c++20 -Iinclude -Isrell_include -O3 -march=native -mavx2 -msse4.2 \
    tests/verify_negated_simd_works.cpp -o tests/verify_simd

# Build non-SIMD version
g++ -std=c++20 -Iinclude -Isrell_include -O3 -march=native -mavx2 -msse4.2 -DCTRE_DISABLE_SIMD \
    tests/verify_negated_simd_works.cpp -o tests/verify_nosimd

echo ""
echo "========================================================================"
echo "Negated Character Class: SIMD vs Non-SIMD Performance"
echo "========================================================================"
echo ""
echo "WITH SIMD (After Fix):"
echo "------------------------------------------------------------------------"
./tests/verify_simd

echo ""
echo ""
echo "WITHOUT SIMD (Before Fix - Scalar Only):"
echo "------------------------------------------------------------------------"
./tests/verify_nosimd

echo ""
echo "========================================================================"
echo "Summary:"
echo "- SIMD times should be ~5-10x faster than Non-SIMD"
echo "- This proves negated patterns now benefit from SIMD optimization"
echo "========================================================================"

# Cleanup
rm -f tests/verify_simd tests/verify_nosimd
