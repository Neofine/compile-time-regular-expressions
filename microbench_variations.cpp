#include <chrono>
#include <iostream>
#include <cstring>
#include <immintrin.h>

// Test different implementation variations
template<typename Func>
double benchmark(Func f, int iterations = 10000000) {
    // Warmup
    for (int i = 0; i < 1000; ++i) f();
    
    double best = 1e9;
    for (int run = 0; run < 5; ++run) {
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iterations; ++i) f();
        auto end = std::chrono::high_resolution_clock::now();
        double time = std::chrono::duration<double, std::nano>(end - start).count() / iterations;
        if (time < best) best = time;
    }
    return best;
}

// Test data
alignas(32) char data[256];

// Variation 1: Current implementation (movemask + ctz)
bool test_current() {
    __m256i vec = _mm256_load_si256((__m256i*)data);
    __m256i target = _mm256_set1_epi8('a');
    __m256i result = _mm256_cmpeq_epi8(vec, target);
    int mask = _mm256_movemask_epi8(result);
    if (mask == 0xFFFFFFFF) return true;
    int pos = __builtin_ctz(~mask);
    return pos > 0;
}

// Variation 2: Using testc directly
bool test_testc() {
    __m256i vec = _mm256_load_si256((__m256i*)data);
    __m256i target = _mm256_set1_epi8('a');
    __m256i result = _mm256_cmpeq_epi8(vec, target);
    __m256i all_ones = _mm256_set1_epi8(0xFF);
    if (_mm256_testc_si256(result, all_ones)) return true;
    int mask = _mm256_movemask_epi8(result);
    int pos = __builtin_ctz(~mask);
    return pos > 0;
}

// Variation 3: Hoisted all_ones
__m256i global_all_ones = _mm256_set1_epi8(0xFF);
bool test_hoisted() {
    __m256i vec = _mm256_load_si256((__m256i*)data);
    __m256i target = _mm256_set1_epi8('a');
    __m256i result = _mm256_cmpeq_epi8(vec, target);
    if (_mm256_testc_si256(result, global_all_ones)) return true;
    int mask = _mm256_movemask_epi8(result);
    int pos = __builtin_ctz(~mask);
    return pos > 0;
}

// Variation 4: Avoid creating mask if testc succeeds
bool test_lazy_mask() {
    __m256i vec = _mm256_load_si256((__m256i*)data);
    __m256i target = _mm256_set1_epi8('a');
    __m256i result = _mm256_cmpeq_epi8(vec, target);
    __m256i all_ones = _mm256_set1_epi8(0xFF);
    if (__builtin_expect(_mm256_testc_si256(result, all_ones), 1)) {
        return true;  // Hot path: no mask creation
    }
    // Cold path: create mask only on mismatch
    int mask = _mm256_movemask_epi8(result);
    int pos = __builtin_ctz(~mask);
    return pos > 0;
}

int main() {
    // Fill test data
    memset(data, 'a', 256);
    
    std::cout << "=== MICRO-BENCHMARKING SIMD VARIATIONS ===" << std::endl;
    std::cout << std::endl;
    std::cout << "Test: All bytes match (hot path)" << std::endl;
    std::cout << "------------------------------------" << std::endl;
    std::cout << "Current (movemask+cmp):  " << benchmark(test_current) << " ns" << std::endl;
    std::cout << "With testc:              " << benchmark(test_testc) << " ns" << std::endl;
    std::cout << "Hoisted all_ones:        " << benchmark(test_hoisted) << " ns" << std::endl;
    std::cout << "Lazy mask creation:      " << benchmark(test_lazy_mask) << " ns" << std::endl;
    
    std::cout << std::endl;
    // Test with mismatch at end
    data[31] = 'b';
    std::cout << "Test: Mismatch at byte 31 (cold path)" << std::endl;
    std::cout << "------------------------------------" << std::endl;
    std::cout << "Current (movemask+cmp):  " << benchmark(test_current) << " ns" << std::endl;
    std::cout << "With testc:              " << benchmark(test_testc) << " ns" << std::endl;
    std::cout << "Hoisted all_ones:        " << benchmark(test_hoisted) << " ns" << std::endl;
    std::cout << "Lazy mask creation:      " << benchmark(test_lazy_mask) << " ns" << std::endl;
    
    return 0;
}
