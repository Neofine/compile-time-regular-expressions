#include <chrono>
#include <iostream>
#include <immintrin.h>
#include <cstring>

template<typename Func>
double benchmark(Func f, int iterations = 5000000) {
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

// Current approach: 5 instructions
bool range_check_current() {
    __m256i vec = _mm256_load_si256((__m256i*)data);
    __m256i min_vec = _mm256_set1_epi8('a');
    __m256i max_vec = _mm256_set1_epi8('z');
    
    __m256i lt_min = _mm256_cmpgt_epi8(min_vec, vec);      // 1
    __m256i gt_max = _mm256_cmpgt_epi8(vec, max_vec);      // 2
    __m256i or_result = _mm256_or_si256(lt_min, gt_max);   // 3
    __m256i result = _mm256_xor_si256(or_result, _mm256_set1_epi8(0xFF)); // 4
    int mask = _mm256_movemask_epi8(result);               // 5
    
    return mask == 0xFFFFFFFF;
}

// Optimized: Use min/max + compare (4 instructions)
bool range_check_minmax() {
    __m256i vec = _mm256_load_si256((__m256i*)data);
    __m256i min_vec = _mm256_set1_epi8('a');
    __m256i max_vec = _mm256_set1_epi8('z');
    
    __m256i clamped_min = _mm256_max_epu8(vec, min_vec);   // 1: clamp to >= 'a'
    __m256i clamped = _mm256_min_epu8(clamped_min, max_vec); // 2: clamp to <= 'z'
    __m256i result = _mm256_cmpeq_epi8(clamped, vec);      // 3: check if unchanged
    int mask = _mm256_movemask_epi8(result);               // 4
    
    return mask == 0xFFFFFFFF;
}

// Alternative: Use subtraction + compare (3 instructions for simple ranges)
bool range_check_subtract() {
    __m256i vec = _mm256_load_si256((__m256i*)data);
    __m256i min_vec = _mm256_set1_epi8('a');
    
    __m256i offset = _mm256_sub_epi8(vec, min_vec);        // 1: offset from 'a'
    __m256i max_offset = _mm256_set1_epi8('z' - 'a');     // Range is 0-25
    __m256i result = _mm256_cmpgt_epi8(max_offset, offset); // 2: check < 26 (unsigned trick)
    // Note: This doesn't handle unsigned properly, needs more work
    
    int mask = _mm256_movemask_epi8(result);               // 3
    return mask == 0xFFFFFFFF;
}

int main() {
    memset(data, 'a', 256);
    
    std::cout << "=== RANGE CHECK OPTIMIZATION TEST ===" << std::endl;
    std::cout << std::endl;
    std::cout << "Current (5 inst):  " << benchmark(range_check_current) << " ns" << std::endl;
    std::cout << "Min/Max (4 inst):  " << benchmark(range_check_minmax) << " ns" << std::endl;
    std::cout << "Subtract (3 inst): " << benchmark(range_check_subtract) << " ns" << std::endl;
    
    // Verify correctness
    std::cout << std::endl;
    std::cout << "Correctness check:" << std::endl;
    std::cout << "  Current:  " << range_check_current() << std::endl;
    std::cout << "  Min/Max:  " << range_check_minmax() << std::endl;
    
    return 0;
}
