#!/bin/bash
echo "=== DIGGING DEEPER INTO ASSEMBLY ==="
echo ""

# Recompile and check if AVX512 check is gone
g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 \
    analyze_machine_code.cpp -o /tmp/analyze_new 2>/dev/null

echo "Step 1: Checking if AVX512 overhead is eliminated..."
objdump -d -M intel /tmp/analyze_new | grep -c "avx512" && echo "Still has AVX512 refs" || echo "✅ AVX512 removed!"

echo ""
echo "Step 2: Analyzing NEW assembly for benchmark_a16..."
objdump -d -M intel /tmp/analyze_new | \
    awk '/benchmark_a16>:/,/^$/ {print NR": "$0}' | head -100

echo ""
echo "Step 3: Looking for optimization opportunities..."
echo "  - Stack frame setup"
echo "  - Redundant register moves"
echo "  - Unnecessary branches"

# Count instructions in the hot path
objdump -d /tmp/analyze_new | \
    awk '/benchmark_a16>:/,/ret/' | \
    wc -l | xargs -I {} echo "  Total instructions in benchmark_a16: {}"

# Look for stack alignment
objdump -d -M intel /tmp/analyze_new | \
    awk '/benchmark_a16>:/,/ret/' | \
    grep "and.*rsp" && echo "  Found expensive stack alignment!" || echo "  No stack alignment found"

echo ""
echo "Step 4: Checking range check assembly..."
objdump -d -M intel /tmp/analyze_new | \
    awk '/benchmark_range32>:/,/vpcmpgtb/ {print NR": "$0}' | tail -20
