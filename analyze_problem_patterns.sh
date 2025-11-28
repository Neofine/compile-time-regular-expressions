#!/bin/bash
echo "=== ANALYZING THE BOTTOM 6 PATTERNS ==="
echo ""

patterns=("a*_16" "negated_class" "alternation_4" "group_alt" "literal_Twain" "whitespace_ing")

for pattern in "${patterns[@]}"; do
    echo "==== $pattern ===="
    result=$(grep "^$pattern@@" results/individual/all_results.txt | head -1)
    
    if [ -n "$result" ]; then
        echo "$result" | awk -F'@@' '{
            printf "Pattern: %s\n", $2;
            printf "Description: %s\n", $4;
            printf "Speedup: %s\n", $6;
        }'
    else
        echo "No result found"
    fi
    echo ""
done

echo "=== CATEGORIZATION ==="
echo ""
echo "1. ALTERNATIONS (no SIMD benefit expected):"
echo "   - alternation_4: Tom|Sawyer|Huckleberry|Finn"
echo "   - group_alt: ([A-Za-z]awyer|[A-Za-z]inn)\\s"
echo "   → These are fundamental - alternations don't benefit from SIMD"
echo ""

echo "2. LITERALS (no SIMD benefit expected):"
echo "   - literal_Twain: Just matching 'Twain'"
echo "   → No repetition, SIMD overhead > benefit"
echo ""

echo "3. COMPLEX PATTERNS (whitespace, negation):"
echo "   - whitespace_ing: \\s[a-zA-Z]{0,12}ing\\s"
echo "   - negated_class: [a-q][^u-z]{13}x"
echo "   → Need fallback for whitespace/complex logic"
echo ""

echo "4. SHORT INPUTS (blocked by threshold):"
echo "   - a*_16: 16 bytes < 24-byte threshold"
echo "   → SIMD skipped due to threshold"
echo ""

echo "=== OPTIMIZATION STRATEGIES ==="
echo ""
echo "Option 1: Lower threshold for simple patterns (a*_16)"
echo "  - Use pattern-specific threshold"
echo "  - Simple single-char: 16 bytes"
echo "  - Complex patterns: 24 bytes"
echo ""

echo "Option 2: Accept 1.0x for alternations/literals"
echo "  - These patterns fundamentally don't benefit"
echo "  - 1.0x (neutral) is actually good!"
echo ""

echo "Option 3: Optimize negated_class & whitespace_ing"
echo "  - These have small negative impact"
echo "  - Need better heuristics to skip SIMD"
echo ""

