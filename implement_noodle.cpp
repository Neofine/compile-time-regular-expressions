// Noodle: Fast literal string search (Hyperscan technique)
#include <immintrin.h>
#include <cstring>
#include <chrono>
#include <iostream>

template<typename Func>
double bench(Func f, int iters = 1000000) {
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

// Current: Standard memcmp approach
bool current_memcmp(const char* haystack, size_t len) {
    const char* needle = "Twain";
    size_t needle_len = 5;
    
    for (size_t i = 0; i <= len - needle_len; ++i) {
        if (memcmp(haystack + i, needle, needle_len) == 0) {
            return true;
        }
    }
    return false;
}

// Noodle: SIMD first-char search + verify
bool noodle_simd(const char* haystack, size_t len) {
    const char* needle = "Twain";
    size_t needle_len = 5;
    
    if (len < needle_len) return false;
    
    __m128i first_char = _mm_set1_epi8('T');
    const char* end = haystack + len - needle_len + 1;
    const char* pos = haystack;
    
    // SIMD search for first character
    while (pos + 16 <= end) {
        __m128i data = _mm_loadu_si128((__m128i*)pos);
        __m128i cmp = _mm_cmpeq_epi8(data, first_char);
        int mask = _mm_movemask_epi8(cmp);
        
        if (mask) {
            // Found potential match, check each position
            for (int i = 0; i < 16 && pos + i <= end; ++i) {
                if (mask & (1 << i)) {
                    // Verify full string
                    if (memcmp(pos + i + 1, needle + 1, needle_len - 1) == 0) {
                        return true;
                    }
                }
            }
        }
        pos += 16;
    }
    
    // Scalar tail
    while (pos <= end) {
        if (*pos == 'T' && memcmp(pos + 1, needle + 1, needle_len - 1) == 0) {
            return true;
        }
        pos++;
    }
    
    return false;
}

// Noodle optimized: Load needle once, compare with SIMD
bool noodle_optimized(const char* haystack, size_t len) {
    const char* needle = "Twain";
    size_t needle_len = 5;
    
    if (len < needle_len) return false;
    
    __m128i first_char = _mm_set1_epi8('T');
    // Load first 4 chars of needle for fast comparison
    uint32_t needle_prefix = *(uint32_t*)needle;
    
    const char* end = haystack + len - needle_len + 1;
    const char* pos = haystack;
    
    while (pos + 16 <= end) {
        __m128i data = _mm_loadu_si128((__m128i*)pos);
        __m128i cmp = _mm_cmpeq_epi8(data, first_char);
        int mask = _mm_movemask_epi8(cmp);
        
        if (mask) {
            for (int i = 0; i < 16 && pos + i <= end; ++i) {
                if (mask & (1 << i)) {
                    // Fast 4-byte comparison
                    if (*(uint32_t*)(pos + i) == needle_prefix &&
                        pos[i + 4] == 'n') {
                        return true;
                    }
                }
            }
        }
        pos += 16;
    }
    
    // Scalar tail
    while (pos <= end) {
        if (*(uint32_t*)pos == needle_prefix && pos[4] == 'n') {
            return true;
        }
        pos++;
    }
    
    return false;
}

int main() {
    const char* haystack = "Mark Twain was a great author";
    size_t len = strlen(haystack);
    
    std::cout << "=== NOODLE (LITERAL STRING SEARCH) TEST ===" << std::endl;
    std::cout << std::endl;
    std::cout << "Current (memcmp):      " << bench([&]{ return current_memcmp(haystack, len); }) << " ns" << std::endl;
    std::cout << "Noodle (SIMD):         " << bench([&]{ return noodle_simd(haystack, len); }) << " ns" << std::endl;
    std::cout << "Noodle (optimized):    " << bench([&]{ return noodle_optimized(haystack, len); }) << " ns" << std::endl;
    
    std::cout << std::endl;
    std::cout << "Correctness:" << std::endl;
    std::cout << "  Current:   " << current_memcmp(haystack, len) << std::endl;
    std::cout << "  SIMD:      " << noodle_simd(haystack, len) << std::endl;
    std::cout << "  Optimized: " << noodle_optimized(haystack, len) << std::endl;
    
    return 0;
}
