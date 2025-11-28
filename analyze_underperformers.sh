#!/bin/bash
echo "=== ANALYZING 32-BYTE PATTERNS (Should be faster after our optimization) ==="
echo ""

cat results/individual/all_results.txt | \
awk -F'@@' '{
    speedup = $6;
    gsub(/x/, "", speedup);
    name = $1;
    pattern = $2;
    simd = $3;
    nosimd = $5;
    
    # Focus on 32-byte single-char patterns that should be fast
    if (name ~ /_32$/ && pattern ~ /^[a-z0-9][\*\+]$/) {
        printf "%-20s %6.2fx  (SIMD: %6.2f ns, NoSIMD: %6.2f ns)  Pattern: %s\n", 
               name, speedup, simd, nosimd, pattern;
    }
}' | sort -t' ' -k2 -n

echo ""
echo "=== COMPARISON ==="
echo "Expected (after 32-byte fast path): ~15-20x"
echo "Actual: Many are still 4-6x"
echo ""
echo "Hypothesis: Our 32-byte fast path isn't being hit consistently"
echo "Let me check the code..."

