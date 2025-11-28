#!/bin/bash
echo "=== FINDING THE PATH TO 11x ==="
echo ""
echo "Current: ~10.3x"
echo "Goal: 11.0x"
echo "Need: +0.7x (+7%)"
echo ""

echo "Strategy 1: Improve patterns between 2x-8x"
echo "==========================================="
cat results/individual/all_results.txt | \
awk -F'@@' '{
    speedup = $6;
    gsub(/x/, "", speedup);
    name = $1;
    pattern = $2;
    
    # Skip alternations and literals
    if (pattern ~ /\|/ || pattern ~ /^[A-Za-z]+$/) next;
    
    if (speedup >= 2 && speedup <= 8) {
        printf "%-25s %5.2fx  %-30s\n", name, speedup, pattern;
        sum += speedup;
        count++;
    }
}
END {
    if (count > 0) {
        avg = sum / count;
        printf "\nPatterns 2-8x: %d patterns, avg %.2fx\n", count, avg;
        printf "If we improve them to 12x:\n";
        printf "  Gain: (12 - %.2f) × %d = %.1fx\n", avg, count, (12 - avg) * count;
        printf "  Per pattern: %.2fx / 80 = %.2fx gain\n", (12 - avg) * count, (12 - avg) * count / 80;
        printf "  New avg: 10.3x + %.2fx = %.2fx\n", (12 - avg) * count / 80, 10.3 + (12 - avg) * count / 80;
    }
}'

echo ""
echo ""
echo "Strategy 2: Pattern-specific thresholds"
echo "========================================"
echo "Problem: 28-byte threshold blocks simple 16-byte patterns"
echo "Solution: Use 16 bytes for simple single-char patterns"
echo ""
echo "Expected gain:"
echo "  - a*_16, a+_16: 1.77x → ~8x (like a*_32 after optimization)"
echo "  - Impact: (8 - 1.77) × ~4 patterns = +25x / 80 = +0.31x"
echo "  - New avg: 10.3x + 0.31x = 10.61x"
echo ""

echo ""
echo "Strategy 3: Optimize scalar tail processing"
echo "==========================================="
echo "Current: Byte-by-byte for <28 bytes"
echo "Could: Use SWAR (SIMD Within A Register) for 8-16 bytes"
echo "Expected gain: +2-3% → +0.2-0.3x"
echo ""

echo ""
echo "Strategy 4: More fast paths"
echo "==========================="
echo "Add fast paths for:"
echo "  - 48 bytes (between 32 and 64)"
echo "  - 80 bytes (between 64 and 128)"
echo "Expected gain: +1-2% → +0.1-0.2x"
echo ""

echo ""
echo "=== RECOMMENDED APPROACH ==="
echo "1. Pattern-specific thresholds (most impact: +0.31x)"
echo "2. Optimize scalar tail with SWAR (+0.2-0.3x)"
echo "3. Add 48-byte fast path (+0.1-0.2x)"
echo ""
echo "Total potential: +0.6-0.8x = 10.9x-11.1x ✅"

