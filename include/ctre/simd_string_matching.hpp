#ifndef CTRE__SIMD_STRING_MATCHING__HPP
#define CTRE__SIMD_STRING_MATCHING__HPP

#include <immintrin.h>
#include <cstdio>
#include "simd_detection.hpp"
#include "atoms_characters.hpp"
#include "flags_and_modes.hpp"
#include <immintrin.h>
#include <iterator>

namespace ctre {
namespace simd {

// Forward declarations
template <typename Iterator, typename EndIterator>
inline bool match_string_avx2_impl(Iterator& current, const EndIterator last, const flags& f, const char* string_chars, size_t string_length);

template <typename Iterator, typename EndIterator>
inline bool match_string_sse42_impl(Iterator& current, const EndIterator last, const flags& f, const char* string_chars, size_t string_length);

template <typename Iterator, typename EndIterator>
inline bool match_string_scalar_impl(Iterator& current, const EndIterator last, const flags& f, const char* string_chars, size_t string_length);

// Runtime-only SIMD-optimized string matching (never constexpr)
template <auto... String, typename Iterator, typename EndIterator>
inline bool match_string_simd(Iterator& current, const EndIterator last, const flags& f) {
    constexpr size_t string_length = sizeof...(String);
    constexpr char string_chars[] = {static_cast<char>(String)...};
    
    // DEBUG: Print first few characters to see if template expansion is working
    // if (string_length >= 4) {
    //     printf("Template expansion: %c%c%c%c... (length=%zu)\n", 
    //            string_chars[0], string_chars[1], string_chars[2], string_chars[3], string_length);
    // }
    
    // Bounds checking is handled by the caller
    
    // Use SIMD if available and string is long enough
    if constexpr (CTRE_SIMD_ENABLED) {
        if (get_simd_capability() >= SIMD_CAPABILITY_AVX2 && string_length >= 32) {
            // AVX2 implementation for 32+ character strings
            return match_string_avx2_impl(current, last, f, string_chars, string_length);
        } else if (get_simd_capability() >= SIMD_CAPABILITY_SSE42 && string_length >= 16) {
            // SSE4.2 implementation for 16+ character strings
            return match_string_sse42_impl(current, last, f, string_chars, string_length);
        }
    }
    
    // Fallback to scalar implementation for short strings or no SIMD
    return match_string_scalar_impl(current, last, f, string_chars, string_length);
}

// AVX2 implementation for long strings
template <typename Iterator, typename EndIterator>
inline bool match_string_avx2_impl(Iterator& current, const EndIterator last, const flags& f, const char* string_chars, size_t string_length) {
    (void)last; (void)f; // Suppress unused parameter warnings
    
    Iterator pos = current;
    
    // Process chunks using hybrid SSE4.2 + AVX2 approach
    size_t processed = 0;
    
    // First, process 64-byte chunks using AVX2 (2x 32-byte chunks)
    while (processed + 64 <= string_length) {
        // Prefetch next iteration's data (128 bytes ahead for good coverage)
        if (processed + 128 <= string_length) {
            __builtin_prefetch(&*(pos + 64), 0, 3);  // Prefetch input data
            __builtin_prefetch(&string_chars[processed + 64], 0, 3);  // Prefetch pattern data
        }
        
        // Load both 32-byte chunks in parallel
        __m256i data1 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*pos));
        __m256i data2 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*(pos + 32)));
        __m256i pattern1 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&string_chars[processed]));
        __m256i pattern2 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&string_chars[processed + 32]));
        
        // Compare both chunks in parallel
        __m256i result1 = _mm256_cmpeq_epi8(data1, pattern1);
        __m256i result2 = _mm256_cmpeq_epi8(data2, pattern2);
        
        // Extract masks and check if both chunks matched
        int mask1 = _mm256_movemask_epi8(result1);
        int mask2 = _mm256_movemask_epi8(result2);
        
        if (static_cast<unsigned int>(mask1) != 0xFFFFFFFFU || 
            static_cast<unsigned int>(mask2) != 0xFFFFFFFFU) {
            return false; // Mismatch found
        }
        
        // Advance iterator by 64 positions
        pos += 64;
        processed += 64;
    }
    
    // Then, process 32-byte chunk using AVX2 (if needed)
    if (processed + 32 <= string_length) {
        // Prefetch next data if available
        if (processed + 48 <= string_length) {
            __builtin_prefetch(&*(pos + 32), 0, 3);  // Prefetch input data
            __builtin_prefetch(&string_chars[processed + 32], 0, 3);  // Prefetch pattern data
        }
        
        __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*pos));
        __m256i pattern = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&string_chars[processed]));
        __m256i result = _mm256_cmpeq_epi8(data, pattern);
        int mask = _mm256_movemask_epi8(result);
        
        if (static_cast<unsigned int>(mask) != 0xFFFFFFFFU) {
            return false; // Mismatch found
        }
        
        pos += 32;
        processed += 32;
    }
    
    // Finally, process 16-byte chunk using SSE4.2 (if needed)
    if (processed + 16 <= string_length) {
        // Prefetch next data if available
        if (processed + 32 <= string_length) {
            __builtin_prefetch(&*(pos + 16), 0, 3);  // Prefetch input data
            __builtin_prefetch(&string_chars[processed + 16], 0, 3);  // Prefetch pattern data
        }
        
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&*pos));
        __m128i pattern = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&string_chars[processed]));
        __m128i result = _mm_cmpeq_epi8(data, pattern);
        int mask = _mm_movemask_epi8(result);
        
        if (static_cast<unsigned int>(mask) != 0xFFFFU) {
            return false; // Mismatch found
        }
        
        pos += 16;
        processed += 16;
    }
    
    // Handle remaining characters with scalar code (caller should ensure bounds are valid)
    for (size_t i = processed; i < string_length; ++i) {
        if (*pos != string_chars[i]) {
            return false;
        }
        ++pos;
    }
    
    current = pos;
    return true;
}

// SSE4.2 implementation for medium strings
template <typename Iterator, typename EndIterator>
inline bool match_string_sse42_impl(Iterator& current, const EndIterator last, const flags& f, const char* string_chars, size_t string_length) {
    (void)last; (void)f; // Suppress unused parameter warnings
    
    Iterator pos = current;
    
    // Process 16-byte chunks using SSE4.2
    size_t processed = 0;
    while (processed + 16 <= string_length) {
        // Prefetch next iteration's data (32 bytes ahead for good coverage)
        if (processed + 32 <= string_length) {
            __builtin_prefetch(&*(pos + 16), 0, 3);  // Prefetch input data
            __builtin_prefetch(&string_chars[processed + 16], 0, 3);  // Prefetch pattern data
        }
        
        // Load 16 bytes from input string
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&*pos));
        // Load 16 bytes from pattern string
        __m128i pattern = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&string_chars[processed]));
        
        // Compare bytes
        __m128i result = _mm_cmpeq_epi8(data, pattern);
        int mask = _mm_movemask_epi8(result);
        
        // Check if all 16 bytes matched
        if (static_cast<unsigned int>(mask) != 0xFFFFU) {
            return false; // Mismatch found
        }
        
        pos += 16;
        processed += 16;
    }
    
    // Handle remaining characters with scalar code
    for (size_t i = processed; i < string_length; ++i) {
        if (*pos != string_chars[i]) {
            return false;
        }
        ++pos;
    }
    
    current = pos;
    return true;
}

// Scalar fallback implementation for short strings or when SIMD is not available
template <typename Iterator, typename EndIterator>
inline bool match_string_scalar_impl(Iterator& current, const EndIterator last, const flags& f, const char* string_chars, size_t string_length) {
    (void)last; (void)f; // Suppress unused parameter warnings
    
    // DEBUG: Print debug info
    // printf("Scalar impl: string_length=%zu\n", string_length);
    // if (string_length >= 4) {
    //     printf("Pattern: %c%c%c%c...\n", string_chars[0], string_chars[1], string_chars[2], string_chars[3]);
    //     printf("Input:   %c%c%c%c...\n", current[0], current[1], current[2], current[3]);
    // }
    
    // Match characters one by one, advancing iterator like original CTRE
    for (size_t i = 0; i < string_length; ++i) {
        if (current == last) {
            return false; // End of input reached
        }
        if (*current != string_chars[i]) {
            return false; // Character mismatch
        }
        ++current; // Advance iterator like original CTRE
    }
    
    return true;
}

} // namespace simd
} // namespace ctre

#endif
