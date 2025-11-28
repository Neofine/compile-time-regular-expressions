#!/bin/bash
echo "=== OPTIMIZATION TARGETS (from last benchmark) ==="
echo ""
echo "Category 1: REGRESSIONS (< 1x) - Fix to neutral"
awk -F'@@' '{gsub(/x$/,"",$6); if ($6 < 1.0) print $6 "x", $1, $2}' results/individual/all_results.txt | sort -n | head -5

echo ""
echo "Category 2: MINIMAL GAINS (1-2x) - Boost to 3x+"
awk -F'@@' '{gsub(/x$/,"",$6); if ($6 >= 1.0 && $6 < 2.0) print $6 "x", $1, $2}' results/individual/all_results.txt | sort -n | head -10

echo ""
echo "Category 3: MID-RANGE (2-4x) - Boost to 6x+"
awk -F'@@' '{gsub(/x$/,"",$6); if ($6 >= 2.0 && $6 < 4.0) print $6 "x", $1, $2}' results/individual/all_results.txt | sort -n | head -15

echo ""
echo "=== OPTIMIZATION STRATEGY ==="
echo "Target Category 3 first (biggest impact on average)"
echo "Then Category 2, finally Category 1"
