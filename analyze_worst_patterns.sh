#!/bin/bash
echo "=== ANALYZING WORST PERFORMING PATTERNS ==="
echo ""
echo "Target patterns:"
echo "  1. suffix_ing: 2.58x (worst overall)"
echo "  2. a*_16: 5.46x (short input)"
echo "  3. [a-z]*_32: 5.15x (range pattern)"
echo ""

# Compile with debug symbols and save assembly
for pattern in "suffix_ing" "a*_16" "[a-z]*_32"; do
    echo "===== Analyzing: $pattern ====="
    echo ""
    
    # Compile with debug symbols
    g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 -g \
        "tests/individual_benchmarks/${pattern}_bench.cpp" \
        -o "/tmp/${pattern}_bench" 2>/dev/null
    
    # Generate assembly
    echo "Step 1: Generating assembly..."
    objdump -d -C -S "/tmp/${pattern}_bench" > "/tmp/${pattern}_bench.asm"
    
    # Find SIMD matching functions
    echo "Step 2: Finding SIMD functions..."
    grep -n "match_single_char_repeat_avx2\|match_char_class_repeat_avx2" "/tmp/${pattern}_bench.asm" | head -5
    
    echo ""
done

echo "Assembly files saved to /tmp/*_bench.asm"
echo "Use: less /tmp/pattern_bench.asm to inspect"
