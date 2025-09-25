#include "ctre.hpp"
#include <iostream>
#include <chrono>
#include <vector>
#include <string>

// Benchmark function that prevents compiler optimizations
template<typename Func>
double benchmark(Func func, const std::string& name, int iterations = 1000000) {
    (void)name; // Suppress unused parameter warning
    auto start = std::chrono::high_resolution_clock::now();
    volatile bool result = false; // Use volatile to prevent optimization
    for (int i = 0; i < iterations; ++i) {
        result = func();
    }
    (void)result; // Suppress unused variable warning
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::nano> duration = end - start;
    return duration.count() / static_cast<double>(iterations);
}

int main() {
    std::cout << "Comprehensive CTRE SIMD Performance Test" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    // Test various string lengths including edge cases
    std::vector<std::pair<size_t, std::string>> test_cases = {
        {15, "abcdefghijklmno"},      // Below SSE4.2 threshold
        {16, "abcdefghijklmnop"},     // At SSE4.2 threshold
        {17, "abcdefghijklmnopq"},    // Above SSE4.2 threshold
        {31, "abcdefghijklmnopqrstuvwxyz12345"}, // Just below AVX2 threshold
        {32, "abcdefghijklmnopqrstuvwxyz123456"}, // At AVX2 threshold
        {33, "abcdefghijklmnopqrstuvwxyz1234567"}, // Just above AVX2 threshold
        {48, "abcdefghijklmnopqrstuvwxyz1234567890ABCDEF"}, // 1.5x AVX2 threshold
        {64, "abcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ"}, // 2x AVX2 threshold
        {65, "abcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZa"}, // Just above 64
        {96, "abcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ"}, // 3x AVX2 threshold
        {128, "abcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ"} // 4x AVX2 threshold
    };
    
    for (const auto& [length, pattern] : test_cases) {
        std::cout << "Testing " << length << "-character strings:" << std::endl;
        
        // Test matching case
        double match_time = benchmark([&]() {
            volatile bool result = false;
            if (length == 15) {
                result = ctre::match<"abcdefghijklmno">(pattern);
            } else if (length == 16) {
                result = ctre::match<"abcdefghijklmnop">(pattern);
            } else if (length == 17) {
                result = ctre::match<"abcdefghijklmnopq">(pattern);
            } else if (length == 31) {
                result = ctre::match<"abcdefghijklmnopqrstuvwxyz12345">(pattern);
            } else if (length == 32) {
                result = ctre::match<"abcdefghijklmnopqrstuvwxyz123456">(pattern);
            } else if (length == 33) {
                result = ctre::match<"abcdefghijklmnopqrstuvwxyz1234567">(pattern);
            } else if (length == 48) {
                result = ctre::match<"abcdefghijklmnopqrstuvwxyz1234567890ABCDEF">(pattern);
            } else if (length == 64) {
                result = ctre::match<"abcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ">(pattern);
            } else if (length == 65) {
                result = ctre::match<"abcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZa">(pattern);
            } else if (length == 96) {
                result = ctre::match<"abcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ">(pattern);
            } else if (length == 128) {
                result = ctre::match<"abcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ">(pattern);
            }
            return result;
        }, "Match", 1000000);
        
        // Test non-matching case (change last character)
        std::string mismatch_pattern = pattern;
        mismatch_pattern.back() = 'X';
        
        double mismatch_time = benchmark([&]() {
            volatile bool result = false;
            if (length == 15) {
                result = ctre::match<"abcdefghijklmno">(mismatch_pattern);
            } else if (length == 16) {
                result = ctre::match<"abcdefghijklmnop">(mismatch_pattern);
            } else if (length == 17) {
                result = ctre::match<"abcdefghijklmnopq">(mismatch_pattern);
            } else if (length == 31) {
                result = ctre::match<"abcdefghijklmnopqrstuvwxyz12345">(mismatch_pattern);
            } else if (length == 32) {
                result = ctre::match<"abcdefghijklmnopqrstuvwxyz123456">(mismatch_pattern);
            } else if (length == 33) {
                result = ctre::match<"abcdefghijklmnopqrstuvwxyz1234567">(mismatch_pattern);
            } else if (length == 48) {
                result = ctre::match<"abcdefghijklmnopqrstuvwxyz1234567890ABCDEF">(mismatch_pattern);
            } else if (length == 64) {
                result = ctre::match<"abcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ">(mismatch_pattern);
            } else if (length == 65) {
                result = ctre::match<"abcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZa">(mismatch_pattern);
            } else if (length == 96) {
                result = ctre::match<"abcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ">(mismatch_pattern);
            } else if (length == 128) {
                result = ctre::match<"abcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ">(mismatch_pattern);
            }
            return result;
        }, "Mismatch", 1000000);
        
        std::cout << "  Match time:    " << match_time << " ns" << std::endl;
        std::cout << "  Mismatch time: " << mismatch_time << " ns" << std::endl;
        std::cout << std::endl;
    }
    
    return 0;
}
