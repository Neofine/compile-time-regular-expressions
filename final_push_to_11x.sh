#!/bin/bash
echo "=== FINAL PUSH TO 11x (Need +3%) ==="
echo ""
echo "Current: 10.70x"
echo "Goal: 11.00x"
echo "Need: +0.30x (+3%)"
echo ""

echo "Strategy: Find patterns still below 5x that could be optimized"
echo ""

cat results/individual/all_results.txt | \
awk -F'@@' '{
    speedup = $6;
    gsub(/x/, "", speedup);
    name = $1;
    pattern = $2;
    
    # Skip literals and alternations
    if (pattern ~ /^[A-Za-z]+$/ || pattern ~ /\|/) next;
    
    # Focus on patterns < 5x
    if (speedup > 0 && speedup < 5) {
        printf "%-25s %5.2fx  %-35s\n", name, speedup, pattern;
        sum += speedup;
        count++;
    }
}
END {
    printf "\n";
    printf "Patterns < 5x: %d\n", count;
    printf "Average of these: %.2fx\n", sum/count;
    printf "\n";
    printf "If we improve these from %.2fx to 7x:\n", sum/count;
    printf "  Gain: (7 - %.2f) × %d = +%.1fx spread over 80\n", sum/count, count, (7 - sum/count) * count;
    printf "  New average: 10.70x + %.2fx = %.2fx\n", ((7 - sum/count) * count) / 80, 10.70 + ((7 - sum/count) * count) / 80;
}'

echo ""
echo "Alternatively, look for assembly-level micro-optimizations..."

