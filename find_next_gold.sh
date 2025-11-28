#!/bin/bash
echo "=== FINDING NEXT OPTIMIZATION TARGET ==="
echo ""

echo "1. Analyzing current top patterns for remaining opportunities..."
echo ""

# Look at the slowest patterns that could still improve
echo "Slowest patterns (biggest opportunity):"
echo "  suffix_ing:    2.56x  (sequence pattern)"
echo "  [a-z]*_32:     7.13x  (range check)"
echo "  [aeiou]*_32:   7.13x  (small set)"
echo ""

echo "2. Checking if we can optimize small sets (aeiou) better..."
echo "   Current: Uses direct comparison (3-5 chars)"
echo "   Could we use PSHUFB lookup? Let's test..."
echo ""

# Analyze what suffix_ing is doing
echo "3. Analyzing suffix_ing pattern..."
g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 \
    tests/individual_benchmarks/suffix_ing_bench.cpp \
    -o /tmp/suffix_test 2>/dev/null

echo "   Running detailed timing..."
for i in {1..5}; do
    /tmp/suffix_test | awk -F'@@' '{printf "   Run %d: %s ns\n", NR, $3}'
done

echo ""
echo "4. Looking for patterns with most variance (unstable performance)..."
echo "   These might benefit from further optimization..."

