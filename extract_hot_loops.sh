#!/bin/bash
echo "=== EXTRACTING HOT LOOPS FROM ASSEMBLY ==="
echo ""

# For a*_16 - look for the 16-byte path
echo "===== a*_16 Hot Loop (16-byte path) ====="
echo ""
# Find the main benchmark loop
awk '/benchmark.*\(/,/ret/ {print NR": "$0}' /tmp/a*_16_bench.asm | grep -A 30 "vmovdqu\|vmovdqa\|vpcmpeqb" | head -40

echo ""
echo "===== [a-z]*_32 Hot Loop (32-byte range check) ====="
echo ""
awk '/benchmark.*\(/,/ret/ {print NR": "$0}' /tmp/[a-z]*_32_bench.asm | grep -A 30 "vpcmpgtb\|vpxor\|vpand" | head -40

