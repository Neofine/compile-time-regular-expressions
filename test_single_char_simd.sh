#!/bin/bash

cd /root/compile-time-regular-expressions

echo "=== Testing Single-Char SIMD Optimization ===" > /tmp/simd_test_results.txt
echo "" >> /tmp/simd_test_results.txt

# Test 1: Compilation
echo "Test 1: Compiling..." >> /tmp/simd_test_results.txt
g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 tests/master_benchmark.cpp -o /tmp/bench_simd 2>&1
if [ $? -eq 0 ]; then
    echo "✓ Compilation PASSED" >> /tmp/simd_test_results.txt
else
    echo "✗ Compilation FAILED" >> /tmp/simd_test_results.txt
    exit 1
fi

# Test 2: Simple functional test
echo "" >> /tmp/simd_test_results.txt
echo "Test 2: Running quick functional test..." >> /tmp/simd_test_results.txt
cat > /tmp/test_func.cpp << 'EOFCPP'
#include <iostream>
#include <ctre.hpp>

int main() {
    std::string s32(32, 'a');
    std::string s64(64, 'a');

    auto m1 = ctre::match<"a*">(s32);
    auto m2 = ctre::match<"a*">(s64);

    if (m1.size() == 32 && m2.size() == 64) {
        std::cout << "FUNCTIONAL_TEST_PASS\n";
        return 0;
    } else {
        std::cout << "FUNCTIONAL_TEST_FAIL\n";
        return 1;
    }
}
EOFCPP

g++ -std=c++20 -Iinclude -O3 -march=native -mavx2 /tmp/test_func.cpp -o /tmp/test_func 2>&1
if [ $? -eq 0 ]; then
    timeout 2 /tmp/test_func >> /tmp/simd_test_results.txt 2>&1
    if [ $? -eq 0 ]; then
        echo "✓ Functional test PASSED" >> /tmp/simd_test_results.txt
    else
        echo "✗ Functional test FAILED or TIMEOUT" >> /tmp/simd_test_results.txt
        exit 1
    fi
else
    echo "✗ Functional test compilation FAILED" >> /tmp/simd_test_results.txt
    exit 1
fi

# Test 3: Run benchmark (abbreviated)
echo "" >> /tmp/simd_test_results.txt
echo "Test 3: Running abbreviated benchmark..." >> /tmp/simd_test_results.txt
timeout 30 /tmp/bench_simd >> /tmp/simd_test_results.txt 2>&1
if [ $? -eq 0 ]; then
    echo "✓ Benchmark completed" >> /tmp/simd_test_results.txt
else
    echo "✗ Benchmark FAILED or TIMEOUT" >> /tmp/simd_test_results.txt
fi

echo "" >> /tmp/simd_test_results.txt
echo "=== Test Complete ===" >> /tmp/simd_test_results.txt

exit 0
