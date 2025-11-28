#!/bin/bash
echo "=== PATTERNS HURTING AVERAGE (< 2x) ==="
awk -F'@@' '{gsub(/x$/, "", $6); if ($6 < 2.0) print $1, $6"x"}' results/individual/all_results.txt | sort -k2 -n

echo ""
echo "=== MID-RANGE PATTERNS (2-4x, optimization targets) ==="
awk -F'@@' '{gsub(/x$/, "", $6); if ($6 >= 2.0 && $6 < 4.0) print $1, $6"x"}' results/individual/all_results.txt | sort -k2 -n | head -20

echo ""
echo "=== PATTERN COUNT BY SPEEDUP RANGE ==="
echo "< 1x (regressions):" $(awk -F'@@' '{gsub(/x$/, "", $6); if ($6 < 1.0) count++} END {print count}' results/individual/all_results.txt)
echo "1-2x (minimal gain):" $(awk -F'@@' '{gsub(/x$/, "", $6); if ($6 >= 1.0 && $6 < 2.0) count++} END {print count}' results/individual/all_results.txt)
echo "2-4x (needs boost):" $(awk -F'@@' '{gsub(/x$/, "", $6); if ($6 >= 2.0 && $6 < 4.0) count++} END {print count}' results/individual/all_results.txt)
echo "4-8x (good):" $(awk -F'@@' '{gsub(/x$/, "", $6); if ($6 >= 4.0 && $6 < 8.0) count++} END {print count}' results/individual/all_results.txt)
echo "> 8x (excellent):" $(awk -F'@@' '{gsub(/x$/, "", $6); if ($6 >= 8.0) count++} END {print count}' results/individual/all_results.txt)
