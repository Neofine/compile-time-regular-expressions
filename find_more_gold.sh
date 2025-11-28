#!/bin/bash
echo "=== FINDING MORE GOLD IN ASSEMBLY ==="
echo ""

# Check for vzeroupper instructions (expensive AVX-SSE transition)
echo "1. Checking for vzeroupper overhead..."
vzero_count=$(objdump -d /tmp/analyze_new | grep -c vzeroupper)
echo "   Found $vzero_count vzeroupper instructions"
echo "   Cost: ~20 cycles each on some CPUs!"
echo "   Note: Needed for AVX-SSE transitions, hard to eliminate"

echo ""
echo "2. Analyzing range check instruction sequence..."
objdump -d -M intel /tmp/analyze_new | \
    awk '/benchmark_range32>:/,/vpor/' | \
    grep -A 5 "vpcmpgtb" | head -15

echo ""
echo "3. Counting branches in hot paths..."
branch_count=$(objdump -d /tmp/analyze_new | \
    awk '/benchmark_a16>:/,/ret/' | \
    grep -c "^.*j[a-z]")
echo "   benchmark_a16 has $branch_count branches"

echo ""
echo "4. Looking for redundant register moves..."
objdump -d -M intel /tmp/analyze_new | \
    awk '/benchmark_a16>:/,/vpcmpeqb/' | \
    grep "mov.*," | head -20

echo ""
echo "5. Checking scalar tail loop efficiency..."
objdump -d -M intel /tmp/analyze_new | \
    awk '/benchmark_a16>:/,/ret/' | \
    grep -A 4 "cmp.*BYTE.*0x61" | head -10

echo ""
echo "💡 OPTIMIZATION IDEAS:"
echo "   1. Range check: Still uses 5 instructions"
echo "   2. vzeroupper: Costs ~20 cycles but needed for correctness"
echo "   3. Scalar tail: Could use SWAR (SIMD Within A Register)"
echo "   4. Branch count: Many branches, but most are predictable"
