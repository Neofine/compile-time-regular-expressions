#!/bin/bash
echo "=== ANALYZING INSTRUCTION SCHEDULING & REGISTER PRESSURE ==="
echo ""

# Look at the actual hot loop in detail
cat << 'CPP' > /tmp/hot_loop.cpp
#include <immintrin.h>
#include <chrono>
#include <iostream>
#include <cstring>

// Simulate the actual hot loop
__attribute__((noinline))
int hot_loop_current(char* data, int len) {
    char* ptr = data;
    char* end = data + len;
    int count = 0;
    
    __m256i all_ones = _mm256_set1_epi8(0xFF);
    __m256i target = _mm256_set1_epi8('a');
    
    // 64-byte unrolled loop (current implementation)
    while (ptr + 64 <= end) {
        __m256i data1 = _mm256_loadu_si256((__m256i*)ptr);
        __m256i data2 = _mm256_loadu_si256((__m256i*)(ptr + 32));
        __m256i result1 = _mm256_cmpeq_epi8(data1, target);
        __m256i result2 = _mm256_cmpeq_epi8(data2, target);
        
        bool match1 = _mm256_testc_si256(result1, all_ones);
        bool match2 = _mm256_testc_si256(result2, all_ones);
        
        if (match1 && match2) {
            ptr += 64;
            count += 64;
        } else if (match1) {
            int mask2 = _mm256_movemask_epi8(result2);
            int pos = __builtin_ctz(~mask2);
            count += 32 + pos;
            break;
        } else {
            int mask1 = _mm256_movemask_epi8(result1);
            int pos = __builtin_ctz(~mask1);
            count += pos;
            break;
        }
    }
    
    return count;
}

// Optimized: Interleave loads and compares (better ILP)
__attribute__((noinline))
int hot_loop_interleaved(char* data, int len) {
    char* ptr = data;
    char* end = data + len;
    int count = 0;
    
    __m256i all_ones = _mm256_set1_epi8(0xFF);
    __m256i target = _mm256_set1_epi8('a');
    
    while (ptr + 64 <= end) {
        // Interleave loads to hide latency
        __m256i data1 = _mm256_loadu_si256((__m256i*)ptr);
        __m256i data2 = _mm256_loadu_si256((__m256i*)(ptr + 32));
        
        // Both compares can execute in parallel
        __m256i result1 = _mm256_cmpeq_epi8(data1, target);
        __m256i result2 = _mm256_cmpeq_epi8(data2, target);
        
        // AND them together first (saves a testc if both need checking)
        __m256i combined = _mm256_and_si256(result1, result2);
        
        // Fast path: Both match
        if (_mm256_testc_si256(combined, all_ones)) {
            ptr += 64;
            count += 64;
            continue;
        }
        
        // Slow path: Check which one failed
        if (!_mm256_testc_si256(result1, all_ones)) {
            int mask1 = _mm256_movemask_epi8(result1);
            count += __builtin_ctz(~mask1);
            break;
        } else {
            int mask2 = _mm256_movemask_epi8(result2);
            count += 32 + __builtin_ctz(~mask2);
            break;
        }
    }
    
    return count;
}

double bench(auto f, char* data, int len) {
    for (int i = 0; i < 1000; ++i) f(data, len);
    
    double best = 1e9;
    for (int run = 0; run < 5; ++run) {
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < 1000000; ++i) {
            volatile int r = f(data, len);
        }
        auto end = std::chrono::high_resolution_clock::now();
        double time = std::chrono::duration<double, std::nano>(end - start).count() / 1000000;
        if (time < best) best = time;
    }
    return best;
}

int main() {
    alignas(32) char data[256];
    memset(data, 'a', 256);
    
    std::cout << "Current implementation:  " << bench(hot_loop_current, data, 256) << " ns" << std::endl;
    std::cout << "Interleaved (better ILP): " << bench(hot_loop_interleaved, data, 256) << " ns" << std::endl;
    
    return 0;
}
CPP

g++ -std=c++20 -O3 -march=native -mavx2 /tmp/hot_loop.cpp -o /tmp/hot_loop && /tmp/hot_loop

