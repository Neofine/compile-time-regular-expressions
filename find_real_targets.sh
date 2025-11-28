#!/bin/bash
echo "=== PATTERNS WITH ROOM FOR IMPROVEMENT (2x-8x) ==="
echo ""
echo "These patterns use SIMD but could be faster:"
echo ""

cat results/individual/all_results.txt | \
awk -F'@@' '{
    speedup = $6;
    gsub(/x/, "", speedup);
    name = $1;
    pattern = $2;
    
    # Skip literals and alternations (no SIMD benefit expected)
    if (pattern ~ /^[A-Za-z]+$/ || pattern ~ /\|/) next;
    
    # Focus on patterns with some benefit but room for improvement
    if (speedup >= 2 && speedup <= 8) {
        printf "%-25s %5.2fx  %-35s\n", name, speedup, pattern;
    }
}' | sort -t' ' -k2 -n | head -20

echo ""
echo "=== ANALYSIS ==="
echo "If we can improve these 20 patterns from avg ~5x to ~10x:"
echo "  Gain: (10-5) × 20 = +100x spread over 80 patterns"
echo "  New average: 9.85x + (100/80) = 11.10x ✅"
echo ""
echo "Strategy: Analyze why these patterns are only 2-8x"

