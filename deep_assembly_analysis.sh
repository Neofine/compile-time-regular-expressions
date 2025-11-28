#!/bin/bash
echo "=== DEEP ASSEMBLY ANALYSIS ==="
echo ""

# Create a minimal test case that's easy to analyze
cat << 'CPP' > /tmp/minimal_test.cpp
#include <ctre.hpp>
#include <string_view>

// Minimal test - just the hot path
__attribute__((noinline)) bool test_a16(std::string_view input) {
    return ctre::match<"a+">(input);
}

__attribute__((noinline)) bool test_a256(std::string_view input) {
    return ctre::match<"a+">(input);
}

__attribute__((noinline)) bool test_range32(std::string_view input) {
    return ctre::match<"[a-z]+">(input);
}

int main() {
    std::string_view s16("aaaaaaaaaaaaaaaa");  // 16 bytes
    std::string_view s256(std::string(256, 'a'));  // 256 bytes
    std::string_view r32(std::string(32, 'a'));
    
    volatile bool r1 = test_a16(s16);
    volatile bool r2 = test_a256(s256);
    volatile bool r3 = test_range32(r32);
    
    return r1 + r2 + r3;
}
CPP

echo "Step 1: Compiling with full optimization..."
g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 -fverbose-asm \
    /tmp/minimal_test.cpp -S -o /tmp/minimal_test.s 2>/dev/null

echo "Step 2: Extracting hot functions..."
echo ""
echo "===== test_a16 function (16-byte pattern) ====="
awk '/test_a16.*:/,/\.cfi_endproc/ {print NR": "$0}' /tmp/minimal_test.s | head -100

echo ""
echo "===== Looking for SIMD instruction sequence ====="
grep -n "vmovdqu\|vpcmpeqb\|vpmovmskb\|vtestc" /tmp/minimal_test.s | head -20
