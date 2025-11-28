#!/bin/bash
echo "=== TESTING: Why are 16-byte patterns only 1.8x? ==="
echo ""

# The issue: Our threshold is 24 bytes, so 16-byte patterns fall back to scalar!
echo "Current threshold: 24 bytes"
echo "Problem: 16-byte patterns skip SIMD entirely!"
echo ""

# Test with pattern that has exactly 16 bytes
echo "Testing a*_16 pattern (exactly 16 bytes):"
echo "  Input size: 16 bytes"
echo "  Threshold: 24 bytes"
echo "  Result: 16 < 24 → SKIPS SIMD! ❌"
echo ""

echo "But we have optimized 16-byte SIMD paths in the code!"
echo "Let me check if the issue is the threshold..."

# Quick test: What if we lower threshold?
cat << 'CPP' > /tmp/test_threshold.cpp
#include <ctre.hpp>
#include <string>
#include <chrono>
#include <iostream>

template<typename Func>
double bench(Func f, int iters = 1000000) {
    for (int i = 0; i < 100; ++i) f();
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iters; ++i) f();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::nano>(end - start).count() / iters;
}

int main() {
    std::string input_16(16, 'a');
    std::string input_32(32, 'a');
    
    auto test_16 = [&]{ return ctre::match<"a*">(input_16).begin(); };
    auto test_32 = [&]{ return ctre::match<"a*">(input_32).begin(); };
    
    std::cout << "a*_16: " << bench(test_16) << " ns" << std::endl;
    std::cout << "a*_32: " << bench(test_32) << " ns" << std::endl;
    
    return 0;
}
CPP

g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 /tmp/test_threshold.cpp -o /tmp/test_threshold 2>/dev/null
/tmp/test_threshold

echo ""
echo "Hypothesis: 16-byte patterns are hitting scalar fallback!"
echo "Solution: Lower threshold from 24 to 16 bytes"

