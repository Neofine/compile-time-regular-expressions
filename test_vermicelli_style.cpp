#include <immintrin.h>
#include <chrono>
#include <iostream>
#include <cstring>

// Current approach (what we have)
template<typename Func>
double bench(Func f, int iters = 5000000) {
    for (int i = 0; i < 1000; ++i) f();
    
    double best = 1e9;
    for (int run = 0; run < 5; ++run) {
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iters; ++i) f();
        auto end = std::chrono::high_resolution_clock::now();
        double time = std::chrono::duration<double, std::nano>(end - start).count() / iters;
        if (time < best) best = time;
    }
    return best;
}

alignas(32) char data[32];

// Our current approach (from simd_character_classes.hpp)
int current_approach() {
    __m256i target = _mm256_set1_epi8('a');
    __m256i all_ones = _mm256_set1_epi8(0xFF);
    __m256i vec = _mm256_load_si256((__m256i*)data);
    __m256i result = _mm256_cmpeq_epi8(vec, target);
    
    if (_mm256_testc_si256(result, all_ones)) {
        return 32; // All match
    } else {
        int mask = _mm256_movemask_epi8(result);
        return __builtin_ctz(~mask);
    }
}

// Vermicelli-inspired: Use movemask directly (fewer instructions)
int vermicelli_style() {
    __m256i target = _mm256_set1_epi8('a');
    __m256i vec = _mm256_load_si256((__m256i*)data);
    __m256i result = _mm256_cmpeq_epi8(vec, target);
    
    int mask = _mm256_movemask_epi8(result);
    
    // All-match check without testc (uses existing mask)
    if (mask == 0xFFFFFFFF) {
        return 32;
    } else {
        return __builtin_ctz(~mask);
    }
}

// Vermicelli-inspired: Inverted comparison (find mismatch directly)
int vermicelli_inverted() {
    __m256i target = _mm256_set1_epi8('a');
    __m256i vec = _mm256_load_si256((__m256i*)data);
    
    // Use XOR to find differences (0 = match, non-zero = mismatch)
    __m256i diff = _mm256_xor_si256(vec, target);
    __m256i zeros = _mm256_setzero_si256();
    __m256i is_match = _mm256_cmpeq_epi8(diff, zeros);
    
    int mask = _mm256_movemask_epi8(is_match);
    
    if (mask == 0xFFFFFFFF) {
        return 32;
    } else {
        return __builtin_ctz(~mask);
    }
}

// SSE version (might be faster for 32 bytes with 2x16)
int sse_double() {
    __m128i target = _mm_set1_epi8('a');
    __m128i vec1 = _mm_load_si128((__m128i*)data);
    __m128i vec2 = _mm_load_si128((__m128i*)(data + 16));
    
    __m128i result1 = _mm_cmpeq_epi8(vec1, target);
    __m128i result2 = _mm_cmpeq_epi8(vec2, target);
    
    int mask1 = _mm_movemask_epi8(result1);
    int mask2 = _mm_movemask_epi8(result2);
    
    if (mask1 == 0xFFFF && mask2 == 0xFFFF) {
        return 32;
    } else if (mask1 != 0xFFFF) {
        return __builtin_ctz(~mask1);
    } else {
        return 16 + __builtin_ctz(~mask2);
    }
}

int main() {
    memset(data, 'a', 32);
    
    std::cout << "=== VERMICELLI-STYLE OPTIMIZATION TEST ===" << std::endl;
    std::cout << std::endl;
    std::cout << "Current (testc + movemask): " << bench(current_approach) << " ns" << std::endl;
    std::cout << "Vermicelli (movemask only):  " << bench(vermicelli_style) << " ns" << std::endl;
    std::cout << "Inverted (XOR + cmpeq):     " << bench(vermicelli_inverted) << " ns" << std::endl;
    std::cout << "SSE double (2x16):          " << bench(sse_double) << " ns" << std::endl;
    
    std::cout << std::endl;
    std::cout << "Correctness check:" << std::endl;
    std::cout << "  Current:    " << current_approach() << std::endl;
    std::cout << "  Vermicelli: " << vermicelli_style() << std::endl;
    std::cout << "  Inverted:   " << vermicelli_inverted() << std::endl;
    std::cout << "  SSE double: " << sse_double() << std::endl;
    
    return 0;
}
