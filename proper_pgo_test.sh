#!/bin/bash
echo "=== PROPER PGO TEST ===" 
echo ""
pattern="a*_256"

# Baseline (no PGO)
echo "Compiling baseline (no PGO)..."
g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 "tests/individual_benchmarks/${pattern}_bench.cpp" -o /tmp/simd_nopgo 2>/dev/null
g++ -std=c++20 -Iinclude -O3 -DCTRE_DISABLE_SIMD "tests/individual_benchmarks/${pattern}_bench.cpp" -o /tmp/nosimd_nopgo 2>/dev/null

# With PGO
echo "Compiling with PGO (SIMD)..."
g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 -fprofile-generate "tests/individual_benchmarks/${pattern}_bench.cpp" -o /tmp/simd_pgo_gen 2>/dev/null
/tmp/simd_pgo_gen > /dev/null 2>&1
g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 -fprofile-use -fprofile-correction "tests/individual_benchmarks/${pattern}_bench.cpp" -o /tmp/simd_pgo 2>/dev/null
rm -f *.gcda 2>/dev/null

echo "Compiling with PGO (NoSIMD)..."
g++ -std=c++20 -Iinclude -O3 -DCTRE_DISABLE_SIMD -fprofile-generate "tests/individual_benchmarks/${pattern}_bench.cpp" -o /tmp/nosimd_pgo_gen 2>/dev/null
/tmp/nosimd_pgo_gen > /dev/null 2>&1
g++ -std=c++20 -Iinclude -O3 -DCTRE_DISABLE_SIMD -fprofile-use -fprofile-correction "tests/individual_benchmarks/${pattern}_bench.cpp" -o /tmp/nosimd_pgo 2>/dev/null
rm -f *.gcda 2>/dev/null

echo ""
echo "RESULTS:"
echo "--------"

simd_nopgo=$(/tmp/simd_nopgo | awk -F'@@' '{print $3}')
nosimd_nopgo=$(/tmp/nosimd_nopgo | awk -F'@@' '{print $3}')
speedup_nopgo=$(awk -v s=$simd_nopgo -v n=$nosimd_nopgo 'BEGIN{printf "%.2f", n/s}')

simd_pgo=$(/tmp/simd_pgo | awk -F'@@' '{print $3}')
nosimd_pgo=$(/tmp/nosimd_pgo | awk -F'@@' '{print $3}')
speedup_pgo=$(awk -v s=$simd_pgo -v n=$nosimd_pgo 'BEGIN{printf "%.2f", n/s}')

echo "Without PGO:  SIMD=${simd_nopgo}ns  NoSIMD=${nosimd_nopgo}ns  Speedup=${speedup_nopgo}x"
echo "With PGO:     SIMD=${simd_pgo}ns  NoSIMD=${nosimd_pgo}ns  Speedup=${speedup_pgo}x"
