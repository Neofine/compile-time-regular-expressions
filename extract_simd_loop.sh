#!/bin/bash
echo "=== EXTRACTING SIMD HOT LOOP ==="
echo ""

# Look for the actual SIMD matching function
echo "Step 1: Finding match_single_char_repeat function..."
objdump -d -C /tmp/minimal_test.s 2>/dev/null | grep -A 200 "match_single_char_repeat" | head -250 > /tmp/simd_func.txt

# Compile to actual binary to get better disassembly
g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 \
    /tmp/minimal_test.cpp -o /tmp/minimal_test_bin 2>/dev/null

echo "Step 2: Disassembling the SIMD function..."
objdump -d -C -M intel /tmp/minimal_test_bin | \
    awk '/match_single_char_repeat_avx2/,/^$/ {print}' | head -150

echo ""
echo "Step 3: Counting instruction types in hot loop..."
objdump -d /tmp/minimal_test_bin | \
    awk '/match_single_char_repeat_avx2/,/ret/' | \
    grep -E "vmov|vcmp|vpand|vpor|test|cmp|j" | \
    awk '{print $3}' | sort | uniq -c | sort -rn

