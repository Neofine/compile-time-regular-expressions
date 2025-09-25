#ifndef CTRE__SIMD_REPETITION__HPP
#define CTRE__SIMD_REPETITION__HPP

#include "simd_detection.hpp"
#include "atoms_characters.hpp"
#include "flags_and_modes.hpp"
#include <immintrin.h>

namespace ctre {
namespace simd {

// Forward declaration
template <size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_character_repeat_simd_with_char(Iterator current, const EndIterator last, const flags& f, char target_char);

// SIMD-optimized character repetition matching
template <typename CharacterType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_character_repeat_simd(Iterator current, const EndIterator last, const flags& f) {
    Iterator start = current;
    (void)MinCount; (void)MaxCount; // Suppress unused parameter warnings
    
    // Try to extract character value at runtime for SIMD optimization
    if (current != last) {
        char first_char = *current;
        
        // Use SIMD if we can determine the target character
        if (CharacterType::match_char(first_char, f)) {
            // We found the target character, now use SIMD for bulk matching
            return match_character_repeat_simd_with_char<MinCount, MaxCount>(current, last, f, first_char);
        }
    }
    
    // Fallback to scalar approach for non-matching characters
    // For patterns like a* (MinCount = 0), we can still match zero characters
    if (MinCount == 0) {
        return current; // Match zero characters
    }
    
    return start; // No match
}

// AVX2 case-sensitive matching
template <typename Iterator, typename EndIterator>
inline Iterator match_avx2_case_sensitive(Iterator current, const EndIterator last, char target_char, size_t& count, size_t max_count) {
    __m256i target_vec = _mm256_set1_epi8(target_char);
    
    while (current + 32 <= last && (max_count == 0 || count + 32 <= max_count)) {
        // Prefetch next iteration's data
        if (current + 64 <= last) {
            __builtin_prefetch(&*(current + 32), 0, 3);
        }
        
        __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*current));
        __m256i result = _mm256_cmpeq_epi8(data, target_vec);
        
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
    
    return current;
}

// AVX2 case-insensitive matching
template <typename Iterator, typename EndIterator>
inline Iterator match_avx2_case_insensitive(Iterator current, const EndIterator last, char target_char, size_t& count, size_t max_count) {
    __m256i target_lower = _mm256_set1_epi8(target_char | 0x20);
    
    while (current + 32 <= last && (max_count == 0 || count + 32 <= max_count)) {
        // Prefetch next iteration's data
        if (current + 64 <= last) {
            __builtin_prefetch(&*(current + 32), 0, 3);
        }
        
        __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*current));
        __m256i data_lower = _mm256_or_si256(data, _mm256_set1_epi8(0x20));
        __m256i result = _mm256_cmpeq_epi8(data_lower, target_lower);
        
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
    
    return current;
}

// SSE4.2 case-sensitive matching
template <typename Iterator, typename EndIterator>
inline Iterator match_sse42_case_sensitive(Iterator current, const EndIterator last, char target_char, size_t& count, size_t max_count) {
    __m128i target_vec = _mm_set1_epi8(target_char);
    
    while (current + 16 <= last && (max_count == 0 || count + 16 <= max_count)) {
        // Prefetch next iteration's data
        if (current + 32 <= last) {
            __builtin_prefetch(&*(current + 16), 0, 3);
        }
        
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&*current));
        __m128i result = _mm_cmpeq_epi8(data, target_vec);
        
        int mask = _mm_movemask_epi8(result);
        if (static_cast<unsigned int>(mask) == 0xFFFFU) {
            current += 16;
            count += 16;
        } else {
            int first_mismatch = __builtin_ctz(~mask);
            current += first_mismatch;
            count += first_mismatch;
            break;
        }
    }
    
    return current;
}

// SSE4.2 case-insensitive matching
template <typename Iterator, typename EndIterator>
inline Iterator match_sse42_case_insensitive(Iterator current, const EndIterator last, char target_char, size_t& count, size_t max_count) {
    __m128i target_lower = _mm_set1_epi8(target_char | 0x20);
    
    while (current + 16 <= last && (max_count == 0 || count + 16 <= max_count)) {
        // Prefetch next iteration's data
        if (current + 32 <= last) {
            __builtin_prefetch(&*(current + 16), 0, 3);
        }
        
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&*current));
        __m128i data_lower = _mm_or_si128(data, _mm_set1_epi8(0x20));
        __m128i result = _mm_cmpeq_epi8(data_lower, target_lower);
        
        int mask = _mm_movemask_epi8(result);
        if (static_cast<unsigned int>(mask) == 0xFFFFU) {
            current += 16;
            count += 16;
        } else {
            int first_mismatch = __builtin_ctz(~mask);
            current += first_mismatch;
            count += first_mismatch;
            break;
        }
    }
    
    return current;
}

// Scalar case-sensitive matching
template <typename Iterator, typename EndIterator>
inline Iterator match_scalar_case_sensitive(Iterator current, const EndIterator last, char target_char, size_t& count, size_t max_count) {
    while (current != last && (max_count == 0 || count < max_count)) {
        if (*current == target_char) {
            ++current;
            ++count;
        } else {
            break;
        }
    }
    return current;
}

// Scalar case-insensitive matching
template <typename Iterator, typename EndIterator>
inline Iterator match_scalar_case_insensitive(Iterator current, const EndIterator last, char target_char, size_t& count, size_t max_count) {
    while (current != last && (max_count == 0 || count < max_count)) {
        if ((*current | 0x20) == (target_char | 0x20)) {
            ++current;
            ++count;
        } else {
            break;
        }
    }
    return current;
}

// SIMD implementation with known character
template <size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_character_repeat_simd_with_char(Iterator current, const EndIterator last, const flags& f, char target_char) {
    Iterator start = current;
    size_t count = 0;
    const bool case_insensitive = is_ascii_alpha(target_char) && is_case_insensitive(f);
    
    // Use SIMD for bulk matching if available and enabled
    if constexpr (CTRE_SIMD_ENABLED) {
        // Use AVX2 for bulk matching if available
        if (get_simd_capability() >= SIMD_CAPABILITY_AVX2) {
            if (case_insensitive) {
                current = match_avx2_case_insensitive(current, last, target_char, count, MaxCount);
            } else {
                current = match_avx2_case_sensitive(current, last, target_char, count, MaxCount);
            }
        }
        // Use SSE4.2 for medium sequences
        else if (get_simd_capability() >= SIMD_CAPABILITY_SSE42) {
            if (case_insensitive) {
                current = match_sse42_case_insensitive(current, last, target_char, count, MaxCount);
            } else {
                current = match_sse42_case_sensitive(current, last, target_char, count, MaxCount);
            }
        }
    }
    
    // Handle remaining characters with scalar code
    if (case_insensitive) {
        current = match_scalar_case_insensitive(current, last, target_char, count, MaxCount);
    } else {
        current = match_scalar_case_sensitive(current, last, target_char, count, MaxCount);
    }
    
    // Check if we met the minimum requirement
    if (count >= MinCount) {
        return current;
    } else {
        return start; // No match
    }
}

} // namespace simd
} // namespace ctre

#endif

