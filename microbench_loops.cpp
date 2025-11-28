#include <chrono>
#include <iostream>
#include <cstring>
#include <immintrin.h>

template<typename Func>
double benchmark(Func f, int iterations = 1000000) {
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

alignas(32) char data[256];

// Test 32-byte loop variations
int loop_current() {
    char* ptr = data;
    int count = 0;
    while (count < 256) {
        __m256i vec = _mm256_load_si256((__m256i*)ptr);
        __m256i target = _mm256_set1_epi8('a');
        __m256i result = _mm256_cmpeq_epi8(vec, target);
        int mask = _mm256_movemask_epi8(result);
        if ((unsigned int)mask == 0xFFFFFFFFU) {
            ptr += 32;
            count += 32;
        } else {
            break;
        }
    }
    return count;
}

int loop_with_testc() {
    char* ptr = data;
    int count = 0;
    __m256i all_ones = _mm256_set1_epi8(0xFF);
    while (count < 256) {
        __m256i vec = _mm256_load_si256((__m256i*)ptr);
        __m256i target = _mm256_set1_epi8('a');
        __m256i result = _mm256_cmpeq_epi8(vec, target);
        if (_mm256_testc_si256(result, all_ones)) {
            ptr += 32;
            count += 32;
        } else {
            break;
        }
    }
    return count;
}

// 64-byte unrolled with movemask
int loop_64byte_movemask() {
    char* ptr = data;
    int count = 0;
    while (count + 64 <= 256) {
        __m256i vec1 = _mm256_load_si256((__m256i*)ptr);
        __m256i vec2 = _mm256_load_si256((__m256i*)(ptr + 32));
        __m256i target = _mm256_set1_epi8('a');
        __m256i result1 = _mm256_cmpeq_epi8(vec1, target);
        __m256i result2 = _mm256_cmpeq_epi8(vec2, target);
        int mask1 = _mm256_movemask_epi8(result1);
        int mask2 = _mm256_movemask_epi8(result2);
        if (mask1 == 0xFFFFFFFF && mask2 == 0xFFFFFFFF) {
            ptr += 64;
            count += 64;
        } else {
            break;
        }
    }
    return count;
}

// 64-byte unrolled with testc
int loop_64byte_testc() {
    char* ptr = data;
    int count = 0;
    __m256i all_ones = _mm256_set1_epi8(0xFF);
    while (count + 64 <= 256) {
        __m256i vec1 = _mm256_load_si256((__m256i*)ptr);
        __m256i vec2 = _mm256_load_si256((__m256i*)(ptr + 32));
        __m256i target = _mm256_set1_epi8('a');
        __m256i result1 = _mm256_cmpeq_epi8(vec1, target);
        __m256i result2 = _mm256_cmpeq_epi8(vec2, target);
        bool match1 = _mm256_testc_si256(result1, all_ones);
        bool match2 = _mm256_testc_si256(result2, all_ones);
        if (match1 && match2) {
            ptr += 64;
            count += 64;
        } else {
            break;
        }
    }
    return count;
}

int main() {
    memset(data, 'a', 256);
    
    std::cout << "=== LOOP VARIATION BENCHMARKS ===" << std::endl;
    std::cout << std::endl;
    std::cout << "32-byte loops:" << std::endl;
    std::cout << "  movemask+cmp:    " << benchmark(loop_current) << " ns" << std::endl;
    std::cout << "  testc:           " << benchmark(loop_with_testc) << " ns" << std::endl;
    std::cout << std::endl;
    std::cout << "64-byte unrolled loops:" << std::endl;
    std::cout << "  movemask+cmp:    " << benchmark(loop_64byte_movemask) << " ns" << std::endl;
    std::cout << "  testc:           " << benchmark(loop_64byte_testc) << " ns" << std::endl;
    
    return 0;
}
