#!/bin/bash
echo "=== FINDING OPTIMIZATION TARGETS (Patterns <5x) ==="
echo ""

cat results/individual/all_results.txt | \
awk -F'@@' '{
    speedup = $6;
    gsub(/x/, "", speedup);
    if (speedup > 0 && speedup < 5) {
        printf "%-25s %5.2fx  Pattern: %-30s Input: %s\n", $1, speedup, $2, $4;
    }
}' | sort -t' ' -k2 -n | head -25

echo ""
echo "=== CATEGORIZING UNDERPERFORMERS ==="
echo ""

echo "1. Literal patterns (no SIMD benefit expected):"
cat results/individual/all_results.txt | awk -F'@@' '$6 < "2x" && $2 ~ /^[A-Za-z]+$/ {print "  " $1 ": " $6}'

echo ""
echo "2. Alternations (complex, hard to optimize):"
cat results/individual/all_results.txt | awk -F'@@' '$6 < "2x" && $2 ~ /\|/ {print "  " $1 ": " $6}'

echo ""
echo "3. Short repetitions (32 bytes, SIMD-optimizable!):"
cat results/individual/all_results.txt | awk -F'@@' '$6 < "5x" && $1 ~ /_(16|32)$/ && $2 ~ /[\*\+]/ {print "  " $1 ": " $6}'

echo ""
echo "4. Other patterns:"
cat results/individual/all_results.txt | awk -F'@@' '{
    speedup = $6;
    gsub(/x/, "", speedup);
    name = $1;
    pattern = $2;
    if (speedup < 5 && pattern !~ /^[A-Za-z]+$/ && pattern !~ /\|/ && name !~ /_(16|32)$/) {
        print "  " name ": " $6;
    }
}'

