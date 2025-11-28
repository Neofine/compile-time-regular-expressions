// Test Rose-style suffix literal optimization
#include <ctre.hpp>
#include <string>
#include <chrono>
#include <iostream>
#include <immintrin.h>

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

// SIMD literal search for "ing"
const char* simd_search_ing(const char* begin, const char* end) {
    if (end - begin < 3) return end;
    
    __m128i i_vec = _mm_set1_epi8('i');
    __m128i n_vec = _mm_set1_epi8('n');
    __m128i g_vec = _mm_set1_epi8('g');
    
    const char* pos = begin;
    const char* search_end = end - 2; // Need 3 bytes
    
    while (pos + 16 <= search_end) {
        __m128i data1 = _mm_loadu_si128((__m128i*)pos);
        __m128i data2 = _mm_loadu_si128((__m128i*)(pos + 1));
        __m128i data3 = _mm_loadu_si128((__m128i*)(pos + 2));
        
        __m128i cmp_i = _mm_cmpeq_epi8(data1, i_vec);
        __m128i cmp_n = _mm_cmpeq_epi8(data2, n_vec);
        __m128i cmp_g = _mm_cmpeq_epi8(data3, g_vec);
        
        __m128i match_in = _mm_and_si128(cmp_i, cmp_n);
        __m128i match_ing = _mm_and_si128(match_in, cmp_g);
        
        int mask = _mm_movemask_epi8(match_ing);
        
        if (mask) {
            int offset = __builtin_ctz(mask);
            return pos + offset;
        }
        
        pos += 16;
    }
    
    // Scalar tail
    while (pos <= search_end) {
        if (pos[0] == 'i' && pos[1] == 'n' && pos[2] == 'g') {
            return pos;
        }
        pos++;
    }
    
    return end;
}

// Rose-style: Search for "ing" suffix, verify [a-zA-Z]+ prefix
bool rose_suffix_search(const std::string& input) {
    const char* begin = input.data();
    const char* end = begin + input.size();
    const char* pos = begin;
    
    while (true) {
        // Find "ing"
        pos = simd_search_ing(pos, end);
        if (pos == end) return false;
        
        // Verify [a-zA-Z]+ before "ing"
        const char* prefix_end = pos - 1;
        const char* prefix_start = prefix_end;
        
        while (prefix_start >= begin) {
            char c = *prefix_start;
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
                prefix_start--;
            } else {
                break;
            }
        }
        prefix_start++; // Move back to first alpha char
        
        // Check if we have at least one alpha char
        if (prefix_start <= prefix_end) {
            return true; // Found match!
        }
        
        // No match, continue searching
        pos += 3;
    }
}

// Current CTRE approach
bool ctre_approach(const std::string& input) {
    return static_cast<bool>(ctre::match<"[a-zA-Z]+ing">(input));
}

int main() {
    std::string test1 = "fishingfishingfishing"; // Has "fishing"
    std::string test2 = "no match here";
    std::string test3 = "running and jumping";
    
    std::cout << "=== ROSE SUFFIX SEARCH TEST ===" << std::endl;
    std::cout << std::endl;
    
    std::cout << "Test 1: '" << test1 << "'" << std::endl;
    std::cout << "  CTRE:  " << bench([&]{ return ctre_approach(test1); }) << " ns" << std::endl;
    std::cout << "  Rose:  " << bench([&]{ return rose_suffix_search(test1); }) << " ns" << std::endl;
    
    std::cout << std::endl;
    std::cout << "Correctness:" << std::endl;
    std::cout << "  CTRE: " << ctre_approach(test1) << std::endl;
    std::cout << "  Rose: " << rose_suffix_search(test1) << std::endl;
    
    std::cout << std::endl;
    std::cout << "Test 2: '" << test2 << "'" << std::endl;
    std::cout << "  CTRE: " << ctre_approach(test2) << std::endl;
    std::cout << "  Rose: " << rose_suffix_search(test2) << std::endl;
    
    std::cout << std::endl;
    std::cout << "Test 3: '" << test3 << "'" << std::endl;
    std::cout << "  CTRE: " << ctre_approach(test3) << std::endl;
    std::cout << "  Rose: " << rose_suffix_search(test3) << std::endl;
    
    return 0;
}
