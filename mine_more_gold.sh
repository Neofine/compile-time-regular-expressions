#!/bin/bash
echo "=== MINING FOR MORE GOLD! Target: 16x ==="
echo ""
echo "Current: 14.98x"
echo "Target:  16.00x (+7%)"
echo ""

# Strategy: Look at patterns between 7-12x (medium performers that could improve)
echo "Medium performers with optimization potential:"
echo "  a*_32:      7.17x  (could match a*_16 at 10.86x?)"
echo "  a+_16:      8.14x  (could reach 10x?)"
echo "  [a-z]*_32:  7.17x  (range check overhead?)"
echo "  a*_64:     11.98x  (close to 12x)"
echo ""

# Let's analyze the 32-byte single-char patterns
echo "DEEP DIVE: Why is a*_32 only 7.17x when a*_16 is 10.86x?"
echo ""

cat << 'CPP' > /tmp/compare_sizes.cpp
#include <ctre.hpp>
#include <string_view>

extern "C" __attribute__((noinline))
const char* bench_16(const char* b, const char* e) {
    return ctre::match<"a+">(std::string_view(b, e-b)).begin();
}

extern "C" __attribute__((noinline))
const char* bench_32(const char* b, const char* e) {
    return ctre::match<"a+">(std::string_view(b, e-b)).begin();
}

int main() {
    char d16[16], d32[32];
    for(int i=0;i<16;i++) d16[i]='a';
    for(int i=0;i<32;i++) d32[i]='a';
    
    auto r1 = bench_16(d16, d16+16);
    auto r2 = bench_32(d32, d32+32);
    return (r1 != nullptr) + (r2 != nullptr);
}
CPP

g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 /tmp/compare_sizes.cpp -o /tmp/cmp 2>/dev/null

echo "Comparing assembly for 16 vs 32 byte patterns..."
objdump -d -M intel /tmp/cmp > /tmp/cmp.asm

echo ""
echo "16-byte function branches:"
awk '/bench_16>:/,/^$/' /tmp/cmp.asm | grep -c "j[a-z]" | xargs -I {} echo "  {}"

echo "32-byte function branches:"
awk '/bench_32>:/,/^$/' /tmp/cmp.asm | grep -c "j[a-z]" | xargs -I {} echo "  {}"

echo ""
echo "Looking for differences in hot paths..."

# Extract the core loop from both
echo ""
echo "16-byte SIMD sequence (first few lines):"
awk '/bench_16>:/,/vpcmpeqb/' /tmp/cmp.asm | tail -15 | head -10

echo ""
echo "32-byte SIMD sequence (first few lines):"
awk '/bench_32>:/,/vpcmpeqb/' /tmp/cmp.asm | tail -15 | head -10

