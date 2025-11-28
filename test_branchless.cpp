#include <chrono>
#include <iostream>
#include <cstring>
#include <immintrin.h>

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

// Current: Branch on every iteration
int loop_with_branches() {
    char* ptr = data;
    int count = 0;
    while (count < 256) {
        if ((data + 256 - ptr) < 32) break;  // Branch 1
        
        __m256i vec = _mm256_load_si256((__m256i*)ptr);
        __m256i target = _mm256_set1_epi8('a');
        __m256i result = _mm256_cmpeq_epi8(vec, target);
        int mask = _mm256_movemask_epi8(result);
        
        if ((unsigned int)mask != 0xFFFFFFFFU) break;  // Branch 2
        
        ptr += 32;
        count += 32;
    }
    return count;
}

// Branchless: Use conditional move (cmov)
int loop_branchless() {
    char* ptr = data;
    int count = 0;
    size_t remaining = 256;
    
    while (remaining >= 32) {
        __m256i vec = _mm256_load_si256((__m256i*)ptr);
        __m256i target = _mm256_set1_epi8('a');
        __m256i result = _mm256_cmpeq_epi8(vec, target);
        int mask = _mm256_movemask_epi8(result);
        
        // Branchless: advance = (mask == 0xFFFFFFFF) ? 32 : 0
        int advance = ((unsigned int)mask == 0xFFFFFFFFU) * 32;
        ptr += advance;
        count += advance;
        remaining -= 32;
        
        // Early exit only when no match (rare)
        if (advance == 0) break;
    }
    return count;
}

int main() {
    memset(data, 'a', 256);
    
    std::cout << "=== BRANCHLESS VS BRANCHING ===" << std::endl;
    std::cout << std::endl;
    std::cout << "With branches:    " << benchmark(loop_with_branches) << " ns" << std::endl;
    std::cout << "Branchless:       " << benchmark(loop_branchless) << " ns" << std::endl;
    
    return 0;
}
