#!/bin/bash
echo "=== DEEP DIVE: 32-BYTE PATTERNS ==="
echo ""
echo "Target patterns (all around 7x):"
echo "  a*_32:        7.17x"
echo "  [a-z]*_32:    7.17x"
echo "  [a-zA-Z]*_32: 7.18x"
echo "  [aeiou]*_32:  7.18x"
echo ""

# Compile and disassemble a 32-byte pattern
cat << 'CPP' > /tmp/test_32byte.cpp
#include <ctre.hpp>
#include <string_view>

extern "C" __attribute__((noinline))
const char* bench_a32(const char* begin, const char* end) {
    return ctre::match<"a+">(std::string_view(begin, end - begin)).begin();
}

int main() {
    char data[32];
    for (int i = 0; i < 32; ++i) data[i] = 'a';
    return bench_a32(data, data + 32) != nullptr;
}
CPP

g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 /tmp/test_32byte.cpp -o /tmp/test_32 2>/dev/null

echo "Step 1: Disassembling 32-byte function..."
objdump -d -M intel /tmp/test_32 | awk '/bench_a32>:/,/^$/ {print NR": "$0}' | head -60

echo ""
echo "Step 2: Checking for hot path instructions..."
objdump -d /tmp/test_32 | awk '/bench_a32>:/,/ret/' | \
    grep -c "cmp\|test\|j[a-z]" | \
    xargs -I {} echo "   Branches/comparisons: {}"

echo ""
echo "Step 3: Looking for optimization opportunities..."
echo "   - Extra bounds checks?"
echo "   - Redundant moves?"
echo "   - Unnecessary vzeroupper?"

objdump -d /tmp/test_32 | awk '/bench_a32>:/,/ret/' | \
    grep -c "vzeroupper" | \
    xargs -I {} echo "   vzeroupper calls: {}"
