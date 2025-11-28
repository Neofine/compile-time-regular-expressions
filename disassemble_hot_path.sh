#!/bin/bash
echo "=== DISASSEMBLING HOT PATH FOR a*_16 ==="
echo ""

# Compile simple test
cat << 'CPP' > /tmp/test_a16.cpp
#include <ctre.hpp>
#include <string>

int main() {
    std::string input = "aaaaaaaaaaaaaaaa";  // Exactly 16 bytes
    auto result = ctre::match<"a+">(input);
    return result ? 1 : 0;
}
CPP

g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 /tmp/test_a16.cpp -o /tmp/test_a16 2>/dev/null

# Disassemble main
echo "Main function assembly:"
objdump -d -C /tmp/test_a16 | awk '/^[0-9a-f]+ <main>:/,/^[0-9a-f]+ </' | head -80

echo ""
echo "Looking for function calls (potential inlining issues):"
objdump -d -C /tmp/test_a16 | grep "call.*match\|call.*repeat" | head -10
