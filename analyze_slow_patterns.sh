#!/bin/bash
echo "=== ANALYZING SLOW PATTERNS WITH ASSEMBLY ==="
echo ""

# Patterns that SHOULD be faster but aren't:
# - suffix_ing: 1.50x  (Pattern: [a-zA-Z]+ing)
# - long_range_1: 2.65x (Pattern: Tom.{10,25}river)

for pattern_name in "suffix_ing"; do
    echo "===================================================================="
    echo "PATTERN: $pattern_name"
    echo "===================================================================="
    
    # Compile the benchmark
    g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 \
        "tests/individual_benchmarks/${pattern_name}_bench.cpp" \
        -o "/tmp/${pattern_name}" 2>/dev/null
    
    # Run it to get timing
    result=$(/tmp/${pattern_name} | awk -F'@@' '{print "SIMD: " $3 " ns, NoSIMD: " $5 " ns, Speedup: " $6}')
    echo "$result"
    echo ""
    
    # Disassemble and analyze
    echo "Branch count:"
    objdump -d /tmp/${pattern_name} | awk '/benchmark_simd/,/^$/' | grep -c "j[a-z]" | xargs -I {} echo "  {}"
    
    echo ""
    echo "Function size (lines):"
    objdump -d /tmp/${pattern_name} | awk '/benchmark_simd/,/^$/' | wc -l | xargs -I {} echo "  {}"
    
    echo ""
    echo "SIMD instructions used:"
    objdump -d /tmp/${pattern_name} | awk '/benchmark_simd/,/^$/' | grep -E "vpcmp|vmov|vpbroadcast|vptest" | wc -l | xargs -I {} echo "  {}"
    
    echo ""
    echo "First 30 lines of hot path:"
    objdump -d -M intel /tmp/${pattern_name} | awk '/benchmark_simd>:/,/vpcmp/ {print NR": "$0}' | head -35 | tail -30
    
    echo ""
done

