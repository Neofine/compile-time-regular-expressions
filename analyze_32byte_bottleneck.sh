#!/bin/bash
echo "=== ANALYZING 32-BYTE PATTERN BOTTLENECK ==="
echo ""

# Compile a 32-byte pattern and look at assembly
cat << 'CPP' > /tmp/analyze_32.cpp
#include <ctre.hpp>
#include <string>
#include <chrono>

int main() {
    std::string input(32, 'a');
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 1000000; ++i) {
        volatile auto r = ctre::match<"a*">(input);
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    double time_ns = std::chrono::duration<double, std::nano>(end - start).count() / 1000000;
    std::cout << "Time: " << time_ns << " ns" << std::endl;
    
    return 0;
}
CPP

echo "Compiling 32-byte a* pattern..."
g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 /tmp/analyze_32.cpp -o /tmp/analyze_32 2>&1 | head -10

if [ -f /tmp/analyze_32 ]; then
    echo "Running benchmark..."
    /tmp/analyze_32
    
    echo ""
    echo "Analyzing assembly (looking for overhead)..."
    
    # Count instructions in hot path
    echo ""
    echo "1. Total branches in hot path:"
    objdump -d /tmp/analyze_32 | awk '/match<"a\*">/,/ret/' | grep -c "j[a-z]"
    
    echo ""
    echo "2. SIMD instructions used:"
    objdump -d /tmp/analyze_32 | awk '/match<"a\*">/,/ret/' | grep -E "vpcmp|vmov|vpbroadcast|vptest|vzeroupper" | wc -l
    
    echo ""
    echo "3. Memory operations:"
    objdump -d /tmp/analyze_32 | awk '/match<"a\*">/,/ret/' | grep -E "mov.*QWORD|mov.*DWORD" | wc -l
    
    echo ""
    echo "4. Function calls:"
    objdump -d /tmp/analyze_32 | awk '/match<"a\*">/,/ret/' | grep "call" | wc -l
    
    echo ""
    echo "Key question: Is the 32-byte fast path being hit?"
    echo "Let's check for the 32-byte check pattern..."
    objdump -d /tmp/analyze_32 | grep -A 5 "cmp.*0x20" | head -10
else
    echo "Compilation failed!"
fi

