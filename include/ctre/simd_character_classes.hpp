#ifndef CTRE__SIMD_CHARACTER_CLASSES__HPP
#define CTRE__SIMD_CHARACTER_CLASSES__HPP

#include "simd_detection.hpp"
#include "atoms_characters.hpp"
#include "flags_and_modes.hpp"
#include <immintrin.h>

namespace ctre {
namespace simd {

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

template <typename SetType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_class_repeat_avx2(Iterator current, const EndIterator& last, const flags& flags, size_t& count);

template <typename SetType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_class_repeat_sse42(Iterator current, const EndIterator& last, const flags& flags, size_t& count);


template <char MinChar, char MaxChar, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_small_range_avx2(Iterator current, EndIterator last, const flags& flags, size_t& count);

template <char MinChar, char MaxChar, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_small_range_sse42(Iterator current, EndIterator last, const flags& flags, size_t& count);

template <typename SetType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_class_repeat_scalar(Iterator current, const EndIterator& last, const flags& flags, size_t& count);

template <char TargetChar, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_single_char_repeat_avx2(Iterator current, const EndIterator& last, const flags& flags, size_t& count);

template <char TargetChar, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_single_char_repeat_sse42(Iterator current, const EndIterator& last, const flags& flags, size_t& count);

template <char TargetChar, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_single_char_repeat_scalar(Iterator current, const EndIterator& last, const flags& flags, size_t& count);

// ============================================================================
// SIMD PATTERN TRAITS
// ============================================================================

template <typename PatternType>
struct simd_pattern_trait {
    static constexpr bool is_simd_optimizable = false;
    static constexpr size_t min_simd_length = 0;
};

template <auto A, auto B>
struct simd_pattern_trait<char_range<A, B>> {
    static constexpr bool is_simd_optimizable = true;
    static constexpr size_t min_simd_length = 8;
    static constexpr auto min_char = A;
    static constexpr auto max_char = B;
    static constexpr bool is_ascii_range = (A >= 0 && B <= 127);
    static constexpr bool is_contiguous = true;
};

template <auto A, auto B>
struct simd_pattern_trait<set<char_range<A, B>>> {
    static constexpr bool is_simd_optimizable = true;
    static constexpr size_t min_simd_length = 8;
    static constexpr auto min_char = A;
    static constexpr auto max_char = B;
    static constexpr bool is_ascii_range = (A >= 0 && B <= 127);
    static constexpr bool is_contiguous = true;
};

template <auto C>
struct simd_pattern_trait<set<character<C>>> {
    static constexpr bool is_simd_optimizable = true;
    static constexpr size_t min_simd_length = 8;
    static constexpr auto min_char = C;
    static constexpr auto max_char = C;
    static constexpr bool is_ascii_range = (C >= 0 && C <= 127);
    static constexpr bool is_contiguous = true;
    static constexpr char single_char = C;
};

template <auto C>
struct simd_pattern_trait<character<C>> {
    static constexpr bool is_simd_optimizable = true;
    static constexpr size_t min_simd_length = 8;
    static constexpr auto min_char = C;
    static constexpr auto max_char = C;
    static constexpr bool is_ascii_range = (C >= 0 && C <= 127);
    static constexpr bool is_contiguous = true;
    static constexpr char single_char = C;
};

// ============================================================================
// PATTERN DETECTION TRAITS
// ============================================================================

template <typename T>
struct is_char_range_set_trait : std::false_type {
    using type = std::false_type;
};

template <auto A, auto B>
struct is_char_range_set_trait<set<char_range<A, B>>> : std::true_type {
    using type = std::true_type;
};

template <auto C>
struct is_char_range_set_trait<set<character<C>>> : std::false_type {
    using type = std::false_type;
};

template <auto C>
struct is_char_range_set_trait<character<C>> : std::true_type {
    using type = std::true_type;
};

template <typename T>
constexpr bool is_char_range_set() {
    return is_char_range_set_trait<T>::value;
}

// Add type alias for evaluation.hpp compatibility
template <typename T>
using is_char_range_set_trait_t = is_char_range_set_trait<T>;

// Make trait accessible in simd namespace for evaluation.hpp
namespace simd {
    template <typename T>
    using is_char_range_set_trait = ctre::simd::is_char_range_set_trait<T>;
}

// Helper function to check if character is ASCII alpha
constexpr bool is_ascii_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

// ============================================================================
// UNIFIED SIMD MATCHING FUNCTIONS
// ============================================================================

template <typename PatternType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_pattern_repeat_simd(Iterator current, const EndIterator last, const flags& flags) {
    if constexpr (!is_char_range_set<PatternType>()) {
        // Fallback to scalar for non-SIMD patterns
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
        return (count >= MinCount) ? current : start;
    }

    Iterator start = current;
    size_t count = 0;
    
    if (can_use_simd()) {
        // Check if this is a single character pattern
        if constexpr (requires { simd_pattern_trait<PatternType>::single_char; }) {
            constexpr char target_char = simd_pattern_trait<PatternType>::single_char;
            if (get_simd_capability() >= SIMD_CAPABILITY_AVX2) {
                current = match_single_char_repeat_avx2<target_char, MinCount, MaxCount>(current, last, flags, count);
            } else if (get_simd_capability() >= SIMD_CAPABILITY_SSE42) {
                current = match_single_char_repeat_sse42<target_char, MinCount, MaxCount>(current, last, flags, count);
            }
        } else {
            // Character class patterns
            if (get_simd_capability() >= SIMD_CAPABILITY_AVX2) {
                current = match_char_class_repeat_avx2<PatternType, MinCount, MaxCount>(current, last, flags, count);
            } else if (get_simd_capability() >= SIMD_CAPABILITY_SSE42) {
                current = match_char_class_repeat_sse42<PatternType, MinCount, MaxCount>(current, last, flags, count);
            }
        }
    }
    
    return (count >= MinCount || MinCount == 0) ? current : start;
}

// ============================================================================
// SMALL RANGE OPTIMIZATIONS (≤10 characters)
// ============================================================================

// ============================================================================
// GENERIC CHARACTER CLASS SIMD IMPLEMENTATIONS
// ============================================================================

template <typename SetType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_class_repeat_avx2(Iterator current, const EndIterator& last, const flags& flags, size_t& count) {
    
    if constexpr (!simd_pattern_trait<SetType>::is_simd_optimizable) {
        return match_char_class_repeat_scalar<SetType, MinCount, MaxCount>(current, last, flags, count);
    }

    constexpr char min_char = simd_pattern_trait<SetType>::min_char;
    constexpr char max_char = simd_pattern_trait<SetType>::max_char;
    const bool case_insensitive = is_ascii_alpha(min_char) && is_ascii_alpha(max_char) && ctre::is_case_insensitive(flags);
    // Small range check is now handled at dispatch level
    
    __m256i min_vec = _mm256_set1_epi8(min_char);
    __m256i max_vec = _mm256_set1_epi8(max_char);
    
    // Process full 32-byte chunks
    while (current + 32 <= last && (MaxCount == 0 || count + 32 <= MaxCount)) {
        if (current + 64 <= last) {
            __builtin_prefetch(&*(current + 32), 0, 3);
        }
        
        __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*current));
        __m256i result;
        
        // Simple range comparison - accept that it's less efficient than single character matching
        if (case_insensitive) {
            __m256i data_lower = _mm256_or_si256(data, _mm256_set1_epi8(0x20));
            __m256i min_lower = _mm256_or_si256(min_vec, _mm256_set1_epi8(0x20));
            __m256i max_lower = _mm256_or_si256(max_vec, _mm256_set1_epi8(0x20));
            
            // Range check: (data >= min) && (data <= max)
            __m256i ge_min = _mm256_cmpgt_epi8(data_lower, _mm256_sub_epi8(min_lower, _mm256_set1_epi8(1)));
            __m256i le_max = _mm256_cmpgt_epi8(_mm256_add_epi8(max_lower, _mm256_set1_epi8(1)), data_lower);
            
            result = _mm256_and_si256(ge_min, le_max);
        } else {
            // Range check: (data >= min) && (data <= max)
            __m256i ge_min = _mm256_cmpgt_epi8(data, _mm256_sub_epi8(min_vec, _mm256_set1_epi8(1)));
            __m256i le_max = _mm256_cmpgt_epi8(_mm256_add_epi8(max_vec, _mm256_set1_epi8(1)), data);
            
            result = _mm256_and_si256(ge_min, le_max);
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
    
    // Process remaining characters with scalar (much faster for small amounts)
    char min_char_lower = case_insensitive ? (min_char | 0x20) : min_char;
    char max_char_lower = case_insensitive ? (max_char | 0x20) : max_char;
    
    for (; current < last && (MaxCount == 0 || count < MaxCount); ++current, ++count) {
        char c = *current;
        if (case_insensitive) {
            c |= 0x20;  // Convert to lowercase
        }
        if (c < min_char_lower || c > max_char_lower) {
            break;
        }
    }
    
    return current;
}

template <typename SetType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_class_repeat_sse42(Iterator current, const EndIterator& last, const flags& flags, size_t& count) {
    if constexpr (!simd_pattern_trait<SetType>::is_simd_optimizable) {
        return match_char_class_repeat_scalar<SetType, MinCount, MaxCount>(current, last, flags, count);
    }

    constexpr char min_char = simd_pattern_trait<SetType>::min_char;
    constexpr char max_char = simd_pattern_trait<SetType>::max_char;
    const bool case_insensitive = is_ascii_alpha(min_char) && is_ascii_alpha(max_char) && ctre::is_case_insensitive(flags);
    // Small range check is now handled at dispatch level
    
    __m128i min_vec = _mm_set1_epi8(min_char);
    __m128i max_vec = _mm_set1_epi8(max_char);
    
    // Process full 16-byte chunks
    while (current + 16 <= last && (MaxCount == 0 || count + 16 <= MaxCount)) {
        if (current + 32 <= last) {
            __builtin_prefetch(&*(current + 16), 0, 3);
        }
        
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&*current));
        __m128i result;
        
        if (case_insensitive) {
            __m128i data_lower = _mm_or_si128(data, _mm_set1_epi8(0x20));
            __m128i min_lower = _mm_or_si128(min_vec, _mm_set1_epi8(0x20));
            __m128i max_lower = _mm_or_si128(max_vec, _mm_set1_epi8(0x20));
            
            __m128i min_minus_one = _mm_sub_epi8(min_lower, _mm_set1_epi8(1));
            __m128i ge_min = _mm_cmpgt_epi8(data_lower, min_minus_one);
            __m128i max_plus_one = _mm_add_epi8(max_lower, _mm_set1_epi8(1));
            __m128i le_max = _mm_cmpgt_epi8(max_plus_one, data_lower);
            
            result = _mm_and_si128(ge_min, le_max);
        } else {
            __m128i min_minus_one = _mm_sub_epi8(min_vec, _mm_set1_epi8(1));
            __m128i ge_min = _mm_cmpgt_epi8(data, min_minus_one);
            __m128i max_plus_one = _mm_add_epi8(max_vec, _mm_set1_epi8(1));
            __m128i le_max = _mm_cmpgt_epi8(max_plus_one, data);
            
            result = _mm_and_si128(ge_min, le_max);
        }
        
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
    
    // Process remaining characters with scalar (much faster for small amounts)
    char min_char_lower = case_insensitive ? (min_char | 0x20) : min_char;
    char max_char_lower = case_insensitive ? (max_char | 0x20) : max_char;
    
    for (; current < last && (MaxCount == 0 || count < MaxCount); ++current, ++count) {
        char c = *current;
        if (case_insensitive) {
            c |= 0x20;  // Convert to lowercase
        }
        if (c < min_char_lower || c > max_char_lower) {
            break;
        }
    }
    
    return current;
}

template <typename SetType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_class_repeat_scalar(Iterator current, const EndIterator& last, const flags& flags, size_t& count) {
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
// SMALL RANGE OPTIMIZATIONS (≤10 characters)
// ============================================================================


template <char MinChar, char MaxChar, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_small_range_avx2(Iterator current, EndIterator last, const flags& flags, size_t& count) {
    // EFFICIENT: Direct range check to eliminate CTRE function call overhead
    count = 0;
    const bool case_insensitive = is_ascii_alpha(MinChar) && is_ascii_alpha(MaxChar) && ctre::is_case_insensitive(flags);
    
    while (current != last && (MaxCount == 0 || count < MaxCount)) {
        char c = *current;
        bool matches = false;
        
        if (case_insensitive) {
            c |= 0x20;  // Convert to lowercase
            char min_lower = MinChar | 0x20;
            char max_lower = MaxChar | 0x20;
            matches = (c >= min_lower && c <= max_lower);
        } else {
            matches = (c >= MinChar && c <= MaxChar);
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


template <char MinChar, char MaxChar, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_small_range_sse42(Iterator current, EndIterator last, const flags& flags, size_t& count) {
    // const bool case_insensitive = is_ascii_alpha(MinChar) && is_ascii_alpha(MaxChar) && ctre::is_case_insensitive(flags);
    
    // Process full 16-byte chunks
    // while (current + 16 <= last && (MaxCount == 0 || count + 16 <= MaxCount)) {
    //     if (current + 32 <= last) {
    //         __builtin_prefetch(&*(current + 16), 0, 3);
    //     }
        
    //     __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&*current));
    //     __m128i result = _mm_setzero_si128();
        
    //     // Simple character-by-character comparison for small ranges
    //     for (char test_char = MinChar; test_char <= MaxChar; ++test_char) {
    //         __m128i char_vec = _mm_set1_epi8(test_char);
    //         __m128i char_lower = case_insensitive ? _mm_or_si128(char_vec, _mm_set1_epi8(0x20)) : char_vec;
    //         __m128i data_lower = case_insensitive ? _mm_or_si128(data, _mm_set1_epi8(0x20)) : data;
            
    //         __m128i match = _mm_cmpeq_epi8(data_lower, char_lower);
    //         result = _mm_or_si128(result, match);
    //     }
        
    //     int mask = _mm_movemask_epi8(result);
    //     if (static_cast<unsigned int>(mask) == 0xFFFFU) {
    //         current += 16;
    //         count += 16;
    //     } else {
    //         int first_mismatch = __builtin_ctz(~mask);
    //         current += first_mismatch;
    //         count += first_mismatch;
    //         break;
    //     }
    // }
    
    // INEFFICIENT: Manual range checking - much slower than CTRE's optimized matching
    // char min_char = case_insensitive ? (MinChar | 0x20) : MinChar;
    // char max_char = case_insensitive ? (MaxChar | 0x20) : MaxChar;
    
    // for (; current < last && (MaxCount == 0 || count < MaxCount); ++current, ++count) {
    //     char c = *current;
    //     if (case_insensitive) {
    //         c |= 0x20;  // Convert to lowercase
    //     }
    //     if (c < min_char || c > max_char) {
    //         break;
    //     }
    // }
    
    // return current;
    
    // EFFICIENT: Direct range check to eliminate CTRE function call overhead
    count = 0;
    const bool case_insensitive = is_ascii_alpha(MinChar) && is_ascii_alpha(MaxChar) && ctre::is_case_insensitive(flags);
    
    while (current != last && (MaxCount == 0 || count < MaxCount)) {
        char c = *current;
        bool matches = false;
        
        if (case_insensitive) {
            c |= 0x20;  // Convert to lowercase
            char min_lower = MinChar | 0x20;
            char max_lower = MaxChar | 0x20;
            matches = (c >= min_lower && c <= max_lower);
        } else {
            matches = (c >= MinChar && c <= MaxChar);
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

// ============================================================================
// SINGLE CHARACTER SIMD IMPLEMENTATIONS
// ============================================================================

template <char TargetChar, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_single_char_repeat_avx2(Iterator current, const EndIterator& last, const flags& flags, size_t& count) {
    const bool case_insensitive = is_ascii_alpha(TargetChar) && ctre::is_case_insensitive(flags);
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
    
    // Process remaining characters with scalar (much faster for small amounts)
    char target_char = case_insensitive ? (TargetChar | 0x20) : TargetChar;
    
    for (; current < last && (MaxCount == 0 || count < MaxCount); ++current, ++count) {
        char c = *current;
        if (case_insensitive) {
            c |= 0x20;  // Convert to lowercase
        }
        if (c != target_char) {
            break;
        }
    }
    
    return current;
}

template <char TargetChar, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_single_char_repeat_sse42(Iterator current, const EndIterator& last, const flags& flags, size_t& count) {
    const bool case_insensitive = is_ascii_alpha(TargetChar) && ctre::is_case_insensitive(flags);
    const __m128i target_vec = _mm_set1_epi8(TargetChar);
    const __m128i target_lower_vec = case_insensitive ? _mm_set1_epi8(TargetChar | 0x20) : target_vec;
    
    // Process full 16-byte chunks
    while (current + 16 <= last && (MaxCount == 0 || count + 16 <= MaxCount)) {
        if (current + 32 <= last) {
            __builtin_prefetch(&*(current + 16), 0, 3);
        }
        
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&*current));
        __m128i result;
        
        if (case_insensitive) {
            __m128i data_lower = _mm_or_si128(data, _mm_set1_epi8(0x20));
            result = _mm_cmpeq_epi8(data_lower, target_lower_vec);
        } else {
            result = _mm_cmpeq_epi8(data, target_vec);
        }
        
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
    
    // Process remaining characters with scalar (much faster for small amounts)
    char target_char = case_insensitive ? (TargetChar | 0x20) : TargetChar;
    
    for (; current < last && (MaxCount == 0 || count < MaxCount); ++current, ++count) {
        char c = *current;
        if (case_insensitive) {
            c |= 0x20;  // Convert to lowercase
        }
        if (c != target_char) {
            break;
        }
    }
    
    return current;
}

template <char TargetChar, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_single_char_repeat_scalar(Iterator current, const EndIterator& last, const flags& flags, size_t& count) {
    const bool case_insensitive = is_ascii_alpha(TargetChar) && ctre::is_case_insensitive(flags);
    
    char target_char = case_insensitive ? (TargetChar | 0x20) : TargetChar;
    
    for (; current != last && (MaxCount == 0 || count < MaxCount); ++current, ++count) {
        char c = *current;
        if (case_insensitive) {
            c |= 0x20;  // Convert to lowercase
        }
        if (c != target_char) {
            break;
        }
    }
    
    return current;
}


} // namespace simd
} // namespace ctre

#endif // CTRE__SIMD_CHARACTER_CLASSES__HPP