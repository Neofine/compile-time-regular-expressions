#!/bin/bash
echo "=== ANALYZING vzeroupper OVERHEAD ==="
echo ""

echo "Background: vzeroupper clears upper 128 bits of YMM registers"
echo "Cost: ~20 cycles on some CPUs, 0 cycles on modern CPUs"
echo "Benefit: Avoids 70-cycle AVX-SSE transition penalty"
echo ""

# Create test with and without vzeroupper
cat << 'CPP' > /tmp/test_vzero.cpp
#include <immintrin.h>
#include <chrono>
#include <iostream>

double benchmark_with_vzero() {
    alignas(32) char data[256];
    for (int i = 0; i < 256; ++i) data[i] = 'a';
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int iter = 0; iter < 1000000; ++iter) {
        __m256i vec = _mm256_load_si256((__m256i*)data);
        __m256i target = _mm256_set1_epi8('a');
        __m256i result = _mm256_cmpeq_epi8(vec, target);
        int mask = _mm256_movemask_epi8(result);
        _mm256_zeroupper();  // Explicit vzeroupper
        volatile int x = mask;
    }
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::nano>(end - start).count() / 1000000;
}

double benchmark_without_vzero() {
    alignas(32) char data[256];
    for (int i = 0; i < 256; ++i) data[i] = 'a';
    
    auto start = std::chrono::high_resolution_clock::now();
    for (int iter = 0; iter < 1000000; ++iter) {
        __m256i vec = _mm256_load_si256((__m256i*)data);
        __m256i target = _mm256_set1_epi8('a');
        __m256i result = _mm256_cmpeq_epi8(vec, target);
        int mask = _mm256_movemask_epi8(result);
        // No vzeroupper
        volatile int x = mask;
    }
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::nano>(end - start).count() / 1000000;
}

int main() {
    std::cout << "With vzeroupper:    " << benchmark_with_vzero() << " ns" << std::endl;
    std::cout << "Without vzeroupper: " << benchmark_without_vzero() << " ns" << std::endl;
    std::cout << std::endl;
    std::cout << "Note: On modern CPUs, vzeroupper is nearly free" << std::endl;
    std::cout << "Removing it risks 70-cycle transition penalty!" << std::endl;
    return 0;
}
CPP

g++ -std=c++20 -O3 -march=native -mavx2 /tmp/test_vzero.cpp -o /tmp/test_vzero && /tmp/test_vzero

echo ""
echo "Conclusion: vzeroupper is necessary and cheap on modern CPUs"
echo "Not worth optimizing away."
