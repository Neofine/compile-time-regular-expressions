#include <iostream>
#include <string>
#include <chrono>
#include <immintrin.h>
#include "ctre/simd_character_classes.hpp"

using namespace std::chrono;

// Test single character SIMD function directly
template<char TargetChar>
void test_single_char_simd(const std::string& test_string, int iterations = 1000000) {
    std::cout << "Testing single character SIMD for '" << TargetChar << "' on string of length " << test_string.length() << std::endl;
    
    auto start = high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        auto current = test_string.begin();
        auto last = test_string.end();
        size_t count = 0;
        
        // Call SIMD function directly
        current = ctre::simd::match_single_char_repeat_avx2<TargetChar, 0, 0>(current, last, ctre::flags{}, count);
        
        // Prevent optimization
        volatile size_t result = count;
        (void)result;
    }
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<nanoseconds>(end - start);
    
    std::cout << "SIMD AVX2: " << duration.count() / iterations << " ns per iteration" << std::endl;
}

// Test scalar version for comparison
template<char TargetChar>
void test_single_char_scalar(const std::string& test_string, int iterations = 1000000) {
    std::cout << "Testing single character scalar for '" << TargetChar << "' on string of length " << test_string.length() << std::endl;
    
    auto start = high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        auto current = test_string.begin();
        auto last = test_string.end();
        size_t count = 0;
        
        // Call scalar function directly
        current = ctre::simd::match_single_char_repeat_scalar<TargetChar, 0, 0>(current, last, ctre::flags{}, count);
        
        // Prevent optimization
        volatile size_t result = count;
        (void)result;
    }
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<nanoseconds>(end - start);
    
    std::cout << "Scalar: " << duration.count() / iterations << " ns per iteration" << std::endl;
}

int main() {
    std::cout << "=== Single Character SIMD Test ===" << std::endl;
    
    // Test with different string lengths
    std::string test_16 = std::string(16, 'a');
    std::string test_32 = std::string(32, 'a');
    std::string test_64 = std::string(64, 'a');
    std::string test_128 = std::string(128, 'a');
    
    std::cout << "\n--- Testing 'a' character ---" << std::endl;
    
    std::cout << "\n16 characters:" << std::endl;
    test_single_char_simd<'a'>(test_16);
    test_single_char_scalar<'a'>(test_16);
    
    std::cout << "\n32 characters:" << std::endl;
    test_single_char_simd<'a'>(test_32);
    test_single_char_scalar<'a'>(test_32);
    
    std::cout << "\n64 characters:" << std::endl;
    test_single_char_simd<'a'>(test_64);
    test_single_char_scalar<'a'>(test_64);
    
    std::cout << "\n128 characters:" << std::endl;
    test_single_char_simd<'a'>(test_128);
    test_single_char_scalar<'a'>(test_128);
    
    return 0;
}
