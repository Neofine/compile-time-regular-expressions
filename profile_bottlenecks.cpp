#include <chrono>
#include <iostream>
#include <cstring>
#include <immintrin.h>

template<typename Func>
double benchmark(const char* name, Func f, int iterations = 5000000) {
    for (int i = 0; i < 1000; ++i) f();
    
    double best = 1e9;
    for (int run = 0; run < 5; ++run) {
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iterations; ++i) f();
        auto end = std::chrono::high_resolution_clock::now();
        double time = std::chrono::duration<double, std::nano>(end - start).count() / iterations;
        if (time < best) best = time;
    }
    std::cout << name << ": " << best << " ns" << std::endl;
    return best;
}

alignas(32) char data[256];

// Profile: Has_at_least_bytes check overhead
bool test_has_at_least_bytes() {
    char* ptr = data;
    char* end = data + 256;
    // Simulate iterator distance check
    return (end - ptr) >= 32;
}

// Profile: Pointer arithmetic vs iterator
bool test_pointer_arith() {
    char* ptr = data;
    ptr += 32;
    return ptr < data + 256;
}

// Profile: Branch misprediction on early exit
int test_early_exit_branch() {
    static int counter = 0;
    counter++;
    if (counter % 100 == 0) return 1;  // 1% early exit
    return 0;
}

// Profile: Vector creation overhead
__m256i test_set1_epi8() {
    return _mm256_set1_epi8('a');
}

// Profile: Load overhead (aligned vs unaligned)
__m256i test_aligned_load() {
    return _mm256_load_si256((__m256i*)data);
}

__m256i test_unaligned_load() {
    return _mm256_loadu_si256((__m256i*)(data + 1));
}

// Profile: Comparison operations
__m256i test_cmpeq() {
    __m256i vec = _mm256_load_si256((__m256i*)data);
    __m256i target = _mm256_set1_epi8('a');
    return _mm256_cmpeq_epi8(vec, target);
}

// Profile: Range check (used in [a-z])
bool test_range_check() {
    __m256i vec = _mm256_load_si256((__m256i*)data);
    __m256i min_vec = _mm256_set1_epi8('a');
    __m256i max_vec = _mm256_set1_epi8('z');
    
    __m256i lt_min = _mm256_cmpgt_epi8(min_vec, vec);
    __m256i gt_max = _mm256_cmpgt_epi8(vec, max_vec);
    __m256i ge_min = _mm256_xor_si256(lt_min, _mm256_set1_epi8(0xFF));
    __m256i le_max = _mm256_xor_si256(gt_max, _mm256_set1_epi8(0xFF));
    __m256i result = _mm256_and_si256(ge_min, le_max);
    
    return _mm256_movemask_epi8(result) == 0xFFFFFFFF;
}

int main() {
    memset(data, 'a', 256);
    
    std::cout << "=== PROFILING BOTTLENECKS ===" << std::endl;
    std::cout << std::endl;
    
    std::cout << "Overhead measurements:" << std::endl;
    benchmark("  has_at_least_bytes", test_has_at_least_bytes);
    benchmark("  pointer arithmetic", test_pointer_arith);
    benchmark("  early exit branch ", test_early_exit_branch);
    
    std::cout << std::endl;
    std::cout << "SIMD operation costs:" << std::endl;
    benchmark("  _mm256_set1_epi8   ", test_set1_epi8);
    benchmark("  aligned load       ", test_aligned_load);
    benchmark("  unaligned load     ", test_unaligned_load);
    benchmark("  cmpeq              ", test_cmpeq);
    benchmark("  range check (full) ", test_range_check);
    
    std::cout << std::endl;
    std::cout << "INSIGHTS:" << std::endl;
    std::cout << "  - has_at_least_bytes overhead per call" << std::endl;
    std::cout << "  - Range checks require 5 ops vs 1 for cmpeq" << std::endl;
    std::cout << "  - Aligned loads may be faster (if possible)" << std::endl;
    
    return 0;
}
