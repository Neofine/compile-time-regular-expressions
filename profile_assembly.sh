#!/bin/bash
echo "=== ASSEMBLY-LEVEL PROFILING ==="
echo ""

# Compile and get assembly for a hot pattern
echo "Step 1: Generating assembly for a*_256 pattern..."
g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 -S tests/individual_benchmarks/a*_256_bench.cpp -o /tmp/bench.s 2>/dev/null

echo "Step 2: Analyzing match_single_char_repeat_avx2 assembly..."
# Find the SIMD matching function
grep -A 30 "match_single_char_repeat" /tmp/bench.s | head -50

echo ""
echo "Step 3: Counting instructions in hot loops..."
# Count AVX2 instructions
avx2_count=$(grep -c "vmov\|vcmp\|vpor\|vpand\|vtest" /tmp/bench.s)
echo "  AVX2 instructions: $avx2_count"

# Count branches
branch_count=$(grep -c "j[a-z]" /tmp/bench.s)
echo "  Branch instructions: $branch_count"

# Count memory ops
mem_count=$(grep -c "mov.*(%\|mov.*,(%\|lea.*(" /tmp/bench.s)
echo "  Memory operations: $mem_count"

echo ""
echo "Step 4: Identifying optimization opportunities..."
echo "Looking for:"
echo "  - Redundant register moves"
echo "  - Unnecessary branches"  
echo "  - Cache line splits"
echo "  - Long dependency chains"
