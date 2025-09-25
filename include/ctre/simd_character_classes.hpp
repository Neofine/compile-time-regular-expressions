#ifndef CTRE__SIMD_CHARACTER_CLASSES__HPP
#define CTRE__SIMD_CHARACTER_CLASSES__HPP

#include "simd_detection.hpp"
#include "atoms_characters.hpp"
#include "flags_and_modes.hpp"
#include <immintrin.h>
#include <array>

namespace ctre {
namespace simd {

// ============================================================================
// EXTENSIBLE SIMD PATTERN MATCHING FRAMEWORK
// ============================================================================

// Base trait for SIMD-optimizable patterns
template <typename PatternType>
struct simd_pattern_trait {
    static constexpr bool is_simd_optimizable = false;
    static constexpr size_t min_simd_length = 0;
};

// Trait to detect if a pattern is a set<char_range<A, B>>
template <typename T>
struct is_char_range_set_trait : std::false_type {};

template <auto A, auto B>
struct is_char_range_set_trait<set<char_range<A, B>>> : std::true_type {};

template <typename T>
constexpr bool is_char_range_set() {
    return is_char_range_set_trait<T>::value;
}

// ============================================================================
// CHARACTER CLASS SIMD TRAITS
// ============================================================================

// Single character range [a-z], [0-9], etc.
template <auto A, auto B>
struct simd_pattern_trait<char_range<A, B>> {
    static constexpr bool is_simd_optimizable = true;
    static constexpr size_t min_simd_length = 16; // Use SSE4.2 for 16+ chars
    
    static constexpr auto min_char = A;
    static constexpr auto max_char = B;
    static constexpr bool is_ascii_range = (A >= 0 && B <= 127);
    static constexpr bool is_contiguous = true;
};

// Character range set [a-z], [0-9], etc.
template <auto A, auto B>
struct simd_pattern_trait<set<char_range<A, B>>> {
    static constexpr bool is_simd_optimizable = true;
    static constexpr size_t min_simd_length = 16; // Use SSE4.2 for 16+ chars
    
    static constexpr auto min_char = A;
    static constexpr auto max_char = B;
    static constexpr bool is_ascii_range = (A >= 0 && B <= 127);
    static constexpr bool is_contiguous = true;
};

// Character enumeration [abc], [xyz], etc.
template <auto... Cs>
struct simd_pattern_trait<enumeration<Cs...>> {
    static constexpr bool is_simd_optimizable = (sizeof...(Cs) <= 16); // Limit for SIMD efficiency
    static constexpr size_t min_simd_length = 32; // Use AVX2 for 32+ chars
    
    static constexpr auto chars = std::array{Cs...};
    static constexpr size_t char_count = sizeof...(Cs);
};

// Character set [a-z0-9], [A-Za-z], etc.
template <typename... Content>
struct simd_pattern_trait<set<Content...>> {
    static constexpr bool is_simd_optimizable = (sizeof...(Content) <= 4); // Limit complexity
    static constexpr size_t min_simd_length = 32; // Use AVX2 for 32+ chars
};

// Negative set [^a-z], [^0-9], etc.
template <typename... Content>
struct simd_pattern_trait<negative_set<Content...>> {
    static constexpr bool is_simd_optimizable = (sizeof...(Content) <= 4); // Limit complexity
    static constexpr size_t min_simd_length = 32; // Use AVX2 for 32+ chars
};

// ============================================================================
// SIMD CHARACTER CLASS MATCHING IMPLEMENTATIONS
// ============================================================================

// ============================================================================
// DIRECT CHARACTER CLASS REPETITION FUNCTIONS
// ============================================================================


// Forward declarations
template <typename SetType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_class_repeat_avx2(Iterator current, EndIterator last, const flags& flags, size_t& count);

template <typename SetType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_class_repeat_sse42(Iterator current, EndIterator last, const flags& flags, size_t& count);

template <typename SetType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_class_repeat_scalar(Iterator current, EndIterator last, const flags& flags, size_t& count);


// Generic SIMD repetition matcher - dispatches to appropriate implementation
template <typename PatternType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_pattern_repeat_simd(Iterator current, const EndIterator last, const flags& flags) {
    // Check if this is a set<char_range<A, B>> pattern
    if constexpr (is_char_range_set<PatternType>()) {
        // Use SIMD for bulk matching if available and enabled
        Iterator start = current;
        size_t count = 0;
        
        if (can_use_simd()) {
            if (get_simd_capability() >= SIMD_CAPABILITY_AVX2) {
                // AVX2 path - use generic character class matching
                current = match_char_class_repeat_avx2<PatternType, MinCount, MaxCount>(current, last, flags, count);
            } else if (get_simd_capability() >= SIMD_CAPABILITY_SSE42) {
                // SSE4.2 path
                current = match_char_class_repeat_sse42<PatternType, MinCount, MaxCount>(current, last, flags, count);
            }
        }
        
        // Handle remaining characters with scalar fallback
        current = match_char_class_repeat_scalar<PatternType, MinCount, MaxCount>(current, last, flags, count);
        
        // Check if we met the minimum requirement
        if (count >= MinCount) {
            return current;
        }
        return start; // No match
    } else {
        // Fall back to scalar implementation for other pattern types
        Iterator start = current;
        size_t count = 0;
        
        while (current != last && (MaxCount == 0 || count < MaxCount)) {
            if (PatternType::match_char(*current, flags)) {
                ++current;
                ++count;
            } else {
                break;
            }
        }
        
        if (count >= MinCount) {
            return current;
        }
        return start;
    }
}

// ============================================================================
// GENERIC CHARACTER CLASS SIMD IMPLEMENTATIONS
// ============================================================================


// Generic AVX2 character class repetition matching
template <typename SetType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_class_repeat_avx2(Iterator current, EndIterator last, const flags& flags, size_t& count) {
    // Use compile-time range detection for optimal SIMD
    if constexpr (requires { typename SetType::min_char; typename SetType::max_char; }) {
        // If the SetType has compile-time min/max, use them directly
        constexpr char min_char = SetType::min_char;
        constexpr char max_char = SetType::max_char;
        
        __m256i min_vec = _mm256_set1_epi8(min_char);
        __m256i max_vec = _mm256_set1_epi8(max_char);
        
        while (current + 32 <= last && (MaxCount == 0 || count + 32 <= MaxCount)) {
            // Prefetch next iteration's data
            if (current + 64 <= last) {
                __builtin_prefetch(&*(current + 32), 0, 3);
            }
            
            __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*current));
            
            // Check if data >= min_char (data > min_char-1)
            __m256i min_minus_one = _mm256_sub_epi8(min_vec, _mm256_set1_epi8(1));
            __m256i ge_min = _mm256_cmpgt_epi8(data, min_minus_one);
            // Check if data <= max_char (data < max_char+1)
            __m256i max_plus_one = _mm256_add_epi8(max_vec, _mm256_set1_epi8(1));
            __m256i le_max = _mm256_cmpgt_epi8(max_plus_one, data);
            
            __m256i result = _mm256_and_si256(ge_min, le_max);
            
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
    } else {
        // Fallback to scalar for unknown ranges
        return match_char_class_repeat_scalar<SetType, MinCount, MaxCount>(current, last, flags, count);
    }
    
    return current;
}


// Generic SSE4.2 character class repetition matching
template <typename SetType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_class_repeat_sse42(Iterator current, EndIterator last, const flags& flags, size_t& count) {
    // Use compile-time range detection for optimal SIMD
    if constexpr (requires { typename SetType::min_char; typename SetType::max_char; }) {
        // If the SetType has compile-time min/max, use them directly
        constexpr char min_char = SetType::min_char;
        constexpr char max_char = SetType::max_char;
        
        __m128i min_vec = _mm_set1_epi8(min_char);
        __m128i max_vec = _mm_set1_epi8(max_char);
        
        while (current + 16 <= last && (MaxCount == 0 || count + 16 <= MaxCount)) {
            // Prefetch next iteration's data
            if (current + 32 <= last) {
                __builtin_prefetch(&*(current + 16), 0, 3);
            }
            
            __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&*current));
            
            // Check if data >= min_char (data > min_char-1)
            __m128i min_minus_one = _mm_sub_epi8(min_vec, _mm_set1_epi8(1));
            __m128i ge_min = _mm_cmpgt_epi8(data, min_minus_one);
            // Check if data <= max_char (data < max_char+1)
            __m128i max_plus_one = _mm_add_epi8(max_vec, _mm_set1_epi8(1));
            __m128i le_max = _mm_cmpgt_epi8(max_plus_one, data);
            
            __m128i result = _mm_and_si128(ge_min, le_max);
            
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
    } else {
        // Fallback to scalar for unknown ranges
        return match_char_class_repeat_scalar<SetType, MinCount, MaxCount>(current, last, flags, count);
    }
    
    return current;
}


// Generic scalar character class repetition matching
template <typename SetType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_class_repeat_scalar(Iterator current, EndIterator last, const flags& flags, size_t& count) {
    while (current != last && (MaxCount == 0 || count < MaxCount)) {
        if (SetType::match_char(*current, flags)) {
            ++current;
            ++count;
        } else {
            break;
        }
    }
    return current;
}

// ============================================================================
// PATTERN-SPECIFIC SIMD IMPLEMENTATIONS
// ============================================================================


// AVX2 implementation for char_range
template <auto A, auto B, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_range_repeat_avx2(Iterator current, EndIterator last, const flags& flags, size_t& count) {
    constexpr char min_char = static_cast<char>(A);
    constexpr char max_char = static_cast<char>(B);
    
    __m256i min_vec = _mm256_set1_epi8(min_char);
    __m256i max_vec = _mm256_set1_epi8(max_char);
    
    while (current + 32 <= last && (MaxCount == 0 || count + 32 <= MaxCount)) {
        // Prefetch next iteration's data
        if (current + 64 <= last) {
            __builtin_prefetch(&*(current + 32), 0, 3);
        }
        
        __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*current));
        
        // Check if data >= min_char (data > min_char-1)
        __m256i min_minus_one = _mm256_sub_epi8(min_vec, _mm256_set1_epi8(1));
        __m256i ge_min = _mm256_cmpgt_epi8(data, min_minus_one);
        // Check if data <= max_char (data < max_char+1)
        __m256i max_plus_one = _mm256_add_epi8(max_vec, _mm256_set1_epi8(1));
        __m256i le_max = _mm256_cmpgt_epi8(max_plus_one, data);
        
        __m256i result = _mm256_and_si256(ge_min, le_max);
        
        int mask = _mm256_movemask_epi8(result);
        if (static_cast<unsigned int>(mask) == 0xFFFFFFFFU) {
            // All 32 characters match
            current += 32;
            count += 32;
        } else {
            // Find first mismatch
            int first_mismatch = __builtin_ctz(~mask);
            current += first_mismatch;
            count += first_mismatch;
            break;
        }
    }
    
    return current;
}

// SSE4.2 implementation for char_range
template <auto A, auto B, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_range_repeat_sse42(Iterator current, EndIterator last, const flags& flags, size_t& count) {
    constexpr char min_char = static_cast<char>(A);
    constexpr char max_char = static_cast<char>(B);
    
    __m128i min_vec = _mm_set1_epi8(min_char);
    __m128i max_vec = _mm_set1_epi8(max_char);
    
    while (current + 16 <= last && (MaxCount == 0 || count + 16 <= MaxCount)) {
        // Prefetch next iteration's data
        if (current + 32 <= last) {
            __builtin_prefetch(&*(current + 16), 0, 3);
        }
        
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&*current));
        
        // Check if data >= min_char (data > min_char-1)
        __m128i min_minus_one = _mm_sub_epi8(min_vec, _mm_set1_epi8(1));
        __m128i ge_min = _mm_cmpgt_epi8(data, min_minus_one);
        // Check if data <= max_char (data < max_char+1)
        __m128i max_plus_one = _mm_add_epi8(max_vec, _mm_set1_epi8(1));
        __m128i le_max = _mm_cmpgt_epi8(max_plus_one, data);
        
        __m128i result = _mm_and_si128(ge_min, le_max);
        
        int mask = _mm_movemask_epi8(result);
        if (static_cast<unsigned int>(mask) == 0xFFFFU) {
            // All 16 characters match
            current += 16;
            count += 16;
        } else {
            // Find first mismatch
            int first_mismatch = __builtin_ctz(~mask);
            current += first_mismatch;
            count += first_mismatch;
            break;
        }
    }
    
    return current;
}

// Scalar implementation for char_range
template <auto A, auto B, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_range_repeat_scalar(Iterator current, EndIterator last, const flags& flags, size_t& count) {
    constexpr char min_char = static_cast<char>(A);
    constexpr char max_char = static_cast<char>(B);
    
    while (current != last && (MaxCount == 0 || count < MaxCount)) {
        char character = *current;
        if (character >= min_char && character <= max_char) {
            ++current;
            ++count;
        } else {
            break;
        }
    }
    
    return current;
}

// ============================================================================
// SPECIALIZED CHARACTER CLASS REPETITION FUNCTIONS
// ============================================================================


} // namespace simd
} // namespace ctre

#endif // CTRE__SIMD_CHARACTER_CLASSES__HPP
