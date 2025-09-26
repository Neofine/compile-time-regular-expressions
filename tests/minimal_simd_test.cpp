#include <iostream>
#include <string>
#include <chrono>
#include <immintrin.h>

using namespace std::chrono;

// Minimal flags structure for testing
struct flags {
    bool case_insensitive = false;
};

// Test single character SIMD function directly (copied from our implementation)
template<char TargetChar, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_single_char_repeat_avx2(Iterator current, EndIterator last, const flags& flags, size_t& count) {
    const bool case_insensitive = ((TargetChar >= 'a' && TargetChar <= 'z') || (TargetChar >= 'A' && TargetChar <= 'Z')) && flags.case_insensitive;
    const __m256i target_vec = _mm256_set1_epi8(TargetChar);
    const __m256i target_lower_vec = case_insensitive ? _mm256_set1_epi8(TargetChar | 0x20) : target_vec;
    
    // Process full 32-byte chunks
    while (current + 32 <= last && (MaxCount == 0 || count + 32 <= MaxCount)) {
        if (current + 64 <= last) {
            __builtin_prefetch(&*(current + 32), 0, 3);
        }
        
        __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*current));
        __m256i result;
        
        if (case_insensitive) {
            __m256i data_lower = _mm256_or_si256(data, _mm256_set1_epi8(0x20));
            result = _mm256_cmpeq_epi8(data_lower, target_lower_vec);
        } else {
            result = _mm256_cmpeq_epi8(data, target_vec);
        }
        
        int mask = _mm256_movemask_epi8(result);
        if (static_cast<unsigned int>(mask) == 0xFFFFFFFFU) {
            current += 32;
            count += 32;
        } else {
            int first_mismatch = __builtin_ctz(~mask);
            current += first_mismatch;
            count += first_mismatch;
            break;
        }
    }
    
    // Process remaining characters with SIMD
    if (current < last && (MaxCount == 0 || count < MaxCount)) {
        size_t remaining = std::distance(current, last);
        if (remaining > 0) {
            __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*current));
            __m256i result;
            
            if (case_insensitive) {
                __m256i data_lower = _mm256_or_si256(data, _mm256_set1_epi8(0x20));
                result = _mm256_cmpeq_epi8(data_lower, target_lower_vec);
            } else {
                result = _mm256_cmpeq_epi8(data, target_vec);
            }
            
            int mask = _mm256_movemask_epi8(result);
            mask &= (1 << remaining) - 1;
            
            if (static_cast<unsigned int>(mask) == static_cast<unsigned int>((1 << remaining) - 1)) {
                current = last;
                count += remaining;
            } else {
                int first_mismatch = __builtin_ctz(~mask);
                current += first_mismatch;
                count += first_mismatch;
            }
        }
    }
    
    return current;
}

// Test scalar version for comparison
template<char TargetChar, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_single_char_repeat_scalar(Iterator current, EndIterator last, const flags& flags, size_t& count) {
    const bool case_insensitive = ((TargetChar >= 'a' && TargetChar <= 'z') || (TargetChar >= 'A' && TargetChar <= 'Z')) && flags.case_insensitive;
    
    while (current != last && (MaxCount == 0 || count < MaxCount)) {
        char c = *current;
        bool matches = false;
        
        if (case_insensitive) {
            char c_lower = c | 0x20;
            char target_lower = TargetChar | 0x20;
            matches = (c_lower == target_lower);
        } else {
            matches = (c == TargetChar);
        }
        
        if (matches) {
            ++current;
            ++count;
        } else {
            break;
        }
    }
    
    return current;
}

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
        current = match_single_char_repeat_avx2<TargetChar, 0, 0>(current, last, flags{}, count);
        
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
        current = match_single_char_repeat_scalar<TargetChar, 0, 0>(current, last, flags{}, count);
        
        // Prevent optimization
        volatile size_t result = count;
        (void)result;
    }
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<nanoseconds>(end - start);
    
    std::cout << "Scalar: " << duration.count() / iterations << " ns per iteration" << std::endl;
}

int main() {
    std::cout << "=== Minimal Single Character SIMD Test ===" << std::endl;
    
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
