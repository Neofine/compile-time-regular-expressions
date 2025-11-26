#ifndef CTRE__SIMD_CHARACTER_CLASSES__HPP
#define CTRE__SIMD_CHARACTER_CLASSES__HPP

#include "simd_detection.hpp"
#include "simd_repetition.hpp"
#include "atoms_characters.hpp"
#include "flags_and_modes.hpp"
#include <immintrin.h>
#include <array>

namespace ctre {
namespace simd {

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

template <typename SetType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_class_repeat_avx2(Iterator current, const EndIterator& last, const flags& flags, size_t& count);

template <typename SetType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_class_repeat_sse42(Iterator current, const EndIterator& last, const flags& flags, size_t& count);



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

// Helper function to check if character is ASCII alpha
constexpr bool is_ascii_alpha(char char_val) {
    return (char_val >= 'a' && char_val <= 'z') || (char_val >= 'A' && char_val <= 'Z');
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
            if constexpr (requires { PatternType::match_char(*current, flags); }) {
                if (PatternType::match_char(*current, flags)) {
                    ++current;
                    ++count;
                } else {
                    break;
                }
            } else {
                // For patterns without match_char, fall back to general evaluation
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
            // Use optimized single character SIMD from simd_repetition.hpp
            current = simd::match_character_repeat_simd_with_char<MinCount, MaxCount>(current, last, flags, target_char);
        } else {
            // Check if this pattern has min_char and max_char traits (character ranges)
            if constexpr (requires { simd_pattern_trait<PatternType>::min_char; simd_pattern_trait<PatternType>::max_char; }) {
                if (get_simd_capability() >= SIMD_CAPABILITY_AVX2) {
                    current = match_char_class_repeat_avx2<PatternType, MinCount, MaxCount>(current, last, flags, count);
                } else if (get_simd_capability() >= SIMD_CAPABILITY_SSE42) {
                    current = match_char_class_repeat_sse42<PatternType, MinCount, MaxCount>(current, last, flags, count);
                }
            } else {
                // Fallback to general character class patterns
                if (get_simd_capability() >= SIMD_CAPABILITY_AVX2) {
                    current = match_char_class_repeat_avx2<PatternType, MinCount, MaxCount>(current, last, flags, count);
                } else if (get_simd_capability() >= SIMD_CAPABILITY_SSE42) {
                    current = match_char_class_repeat_sse42<PatternType, MinCount, MaxCount>(current, last, flags, count);
                }
            }
        }
    }
    
    return (count >= MinCount || MinCount == 0) ? current : start;
}

// ============================================================================
// SMALL RANGE OPTIMIZATIONS (≤3 characters)
// ============================================================================

// Optimized SIMD for very small ranges (2-3 characters) using direct character comparison
template <typename Iterator, typename EndIterator>
inline Iterator match_small_range_direct_avx2(Iterator current, const EndIterator& last, size_t& count,
                                             const std::array<char, 3>& chars, size_t num_chars, bool case_insensitive) {
    // Create vectors for each character (unrolled for performance)
    __m256i char_vecs[3];
    if (num_chars >= 1) {
        char_vecs[0] = _mm256_set1_epi8(case_insensitive ? (chars[0] | 0x20) : chars[0]);
    }
    if (num_chars >= 2) {
        char_vecs[1] = _mm256_set1_epi8(case_insensitive ? (chars[1] | 0x20) : chars[1]);
    }
    if (num_chars >= 3) {
        char_vecs[2] = _mm256_set1_epi8(case_insensitive ? (chars[2] | 0x20) : chars[2]);
    }
    
    // Process full 32-byte chunks
    while (current != last) {
        
        __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*current));
        __m256i result = _mm256_setzero_si256();
        
        if (case_insensitive) {
            __m256i data_lower = _mm256_or_si256(data, _mm256_set1_epi8(0x20));
            if (num_chars >= 1) { result = _mm256_or_si256(result, _mm256_cmpeq_epi8(data_lower, char_vecs[0])); }
            if (num_chars >= 2) { result = _mm256_or_si256(result, _mm256_cmpeq_epi8(data_lower, char_vecs[1])); }
            if (num_chars >= 3) { result = _mm256_or_si256(result, _mm256_cmpeq_epi8(data_lower, char_vecs[2])); }
        } else {
            if (num_chars >= 1) { result = _mm256_or_si256(result, _mm256_cmpeq_epi8(data, char_vecs[0])); }
            if (num_chars >= 2) { result = _mm256_or_si256(result, _mm256_cmpeq_epi8(data, char_vecs[1])); }
            if (num_chars >= 3) { result = _mm256_or_si256(result, _mm256_cmpeq_epi8(data, char_vecs[2])); }
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
    
    return current;
}

// Optimized SIMD for very small ranges (2-3 characters) using direct character comparison
template <typename Iterator, typename EndIterator>
inline Iterator match_small_range_direct_sse42(Iterator current, const EndIterator& last, size_t& count,
                                              const std::array<char, 3>& chars, size_t num_chars, bool case_insensitive) {
    // Create vectors for each character (unrolled for performance)
    __m128i char_vecs[3];
    if (num_chars >= 1) {
        char_vecs[0] = _mm_set1_epi8(case_insensitive ? (chars[0] | 0x20) : chars[0]);
    }
    if (num_chars >= 2) {
        char_vecs[1] = _mm_set1_epi8(case_insensitive ? (chars[1] | 0x20) : chars[1]);
    }
    if (num_chars >= 3) {
        char_vecs[2] = _mm_set1_epi8(case_insensitive ? (chars[2] | 0x20) : chars[2]);
    }
    
    // Process full 16-byte chunks
    while (current != last) {
        
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&*current));
        __m128i result = _mm_setzero_si128();
        
        if (case_insensitive) {
            __m128i data_lower = _mm_or_si128(data, _mm_set1_epi8(0x20));
            if (num_chars >= 1) { result = _mm_or_si128(result, _mm_cmpeq_epi8(data_lower, char_vecs[0])); }
            if (num_chars >= 2) { result = _mm_or_si128(result, _mm_cmpeq_epi8(data_lower, char_vecs[1])); }
            if (num_chars >= 3) { result = _mm_or_si128(result, _mm_cmpeq_epi8(data_lower, char_vecs[2])); }
        } else {
            if (num_chars >= 1) { result = _mm_or_si128(result, _mm_cmpeq_epi8(data, char_vecs[0])); }
            if (num_chars >= 2) { result = _mm_or_si128(result, _mm_cmpeq_epi8(data, char_vecs[1])); }
            if (num_chars >= 3) { result = _mm_or_si128(result, _mm_cmpeq_epi8(data, char_vecs[2])); }
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
    
    return current;
}

// ============================================================================
// GENERIC CHARACTER CLASS SIMD IMPLEMENTATIONS
// ============================================================================

template <typename SetType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_class_repeat_avx2(Iterator current, const EndIterator& last, const flags& flags, size_t& count) {
    
    if constexpr (!simd_pattern_trait<SetType>::is_simd_optimizable) {
        return match_char_class_repeat_scalar<SetType, MinCount, MaxCount>(current, last, flags, count);
    }

    if constexpr (requires { simd_pattern_trait<SetType>::min_char; simd_pattern_trait<SetType>::max_char; }) {
        constexpr char min_char = simd_pattern_trait<SetType>::min_char;
        constexpr char max_char = simd_pattern_trait<SetType>::max_char;
        constexpr size_t range_size = max_char - min_char + 1;
        const bool case_insensitive = is_ascii_alpha(min_char) && is_ascii_alpha(max_char) && ctre::is_case_insensitive(flags);
        
        // Small range optimization: use direct character comparison for very small ranges
        if constexpr (range_size <= 3) {
            constexpr std::array<char, 3> chars = []() {
                std::array<char, 3> result = {};
                size_t idx = 0;
                for (char char_val = min_char; char_val <= max_char; ++char_val) {
                    result[idx++] = char_val;
                }
                return result;
            }();
            constexpr size_t num_chars = range_size;
            current = match_small_range_direct_avx2(current, last, count, chars, num_chars, case_insensitive);
        } else {
            // Use traditional range comparison for larger ranges
            __m256i min_vec = _mm256_set1_epi8(min_char);
            __m256i max_vec = _mm256_set1_epi8(max_char);
    
            // Process full 32-byte chunks
            while (current != last && (MaxCount == 0 || count + 32 <= MaxCount)) {
                if (current == last) {
                    break; // We're at the end
                }
                
                __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*current));
                __m256i result;
                
                // Simple range comparison - accept that it's less efficient than single character matching
                if (case_insensitive) {
                    __m256i data_lower = _mm256_or_si256(data, _mm256_set1_epi8(0x20));
                    __m256i min_lower = _mm256_or_si256(min_vec, _mm256_set1_epi8(0x20));
                    __m256i max_lower = _mm256_or_si256(max_vec, _mm256_set1_epi8(0x20));
                    
                    // Range check: (data >= min) && (data <= max) - optimized
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
            
            for (; current != last && (MaxCount == 0 || count < MaxCount); ++current, ++count) {
                char char_val = *current;
                if (case_insensitive) {
                    char_val |= 0x20;  // Convert to lowercase
                }
                if (char_val < min_char_lower || char_val > max_char_lower) {
                    break;
                }
            }
        }
        
        return current;
    } else {
        // Fallback for patterns without min_char/max_char traits
        return match_char_class_repeat_scalar<SetType, MinCount, MaxCount>(current, last, flags, count);
    }
}

template <typename SetType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_class_repeat_sse42(Iterator current, const EndIterator& last, const flags& flags, size_t& count) {
    if constexpr (!simd_pattern_trait<SetType>::is_simd_optimizable) {
        return match_char_class_repeat_scalar<SetType, MinCount, MaxCount>(current, last, flags, count);
    }

    if constexpr (requires { simd_pattern_trait<SetType>::min_char; simd_pattern_trait<SetType>::max_char; }) {
        constexpr char min_char = simd_pattern_trait<SetType>::min_char;
        constexpr char max_char = simd_pattern_trait<SetType>::max_char;
        constexpr size_t range_size = max_char - min_char + 1;
        const bool case_insensitive = is_ascii_alpha(min_char) && is_ascii_alpha(max_char) && ctre::is_case_insensitive(flags);
        
        // Small range optimization: use direct character comparison for very small ranges
        if constexpr (range_size <= 3) {
            constexpr std::array<char, 3> chars = []() {
                std::array<char, 3> result = {};
                size_t idx = 0;
                for (char char_val = min_char; char_val <= max_char; ++char_val) {
                    result[idx++] = char_val;
                }
                return result;
            }();
            constexpr size_t num_chars = range_size;
            current = match_small_range_direct_sse42(current, last, count, chars, num_chars, case_insensitive);
        } else {
            // Use traditional range comparison for larger ranges
            __m128i min_vec = _mm_set1_epi8(min_char);
            __m128i max_vec = _mm_set1_epi8(max_char);
    
    // Process full 16-byte chunks
    while (current != last && (MaxCount == 0 || count + 16 <= MaxCount)) {
        if (current == last) {
            break; // We're at the end
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
    
    for (; current != last && (MaxCount == 0 || count < MaxCount); ++current, ++count) {
        char char_val = *current;
        if (case_insensitive) {
            char_val |= 0x20;  // Convert to lowercase
        }
        if (char_val < min_char_lower || char_val > max_char_lower) {
            break;
        }
    }
    
    return current;
        } // End of else block for range_size > 3
    } else {
        // Fallback for patterns without min_char/max_char traits
        return match_char_class_repeat_scalar<SetType, MinCount, MaxCount>(current, last, flags, count);
    }
}

template <typename SetType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_class_repeat_scalar(Iterator current, const EndIterator& last, const flags& flags, size_t& count) {
    while (current != last && (MaxCount == 0 || count < MaxCount)) {
        if constexpr (requires { SetType::match_char(*current, flags); }) {
            if (SetType::match_char(*current, flags)) {
                ++current;
                ++count;
            } else {
                break;
            }
        } else {
            // For patterns without match_char, fall back to general evaluation
            break;
        }
    }
    return current;
}

// ============================================================================
// SINGLE CHARACTER SIMD IMPLEMENTATIONS
// ============================================================================

template <char TargetChar, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_single_char_repeat_sse42(Iterator current, const EndIterator& last, const flags& flags, size_t& count) {
    const bool case_insensitive = is_ascii_alpha(TargetChar) && ctre::is_case_insensitive(flags);
    const __m128i target_vec = _mm_set1_epi8(TargetChar);
    const __m128i target_lower_vec = case_insensitive ? _mm_set1_epi8(TargetChar | 0x20) : target_vec;
    
    // Process full 16-byte chunks
    while (current != last && (MaxCount == 0 || count + 16 <= MaxCount)) {
        if (current == last) {
            break; // We're at the end
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
    
    for (; current != last && (MaxCount == 0 || count < MaxCount); ++current, ++count) {
        char char_val = *current;
        if (case_insensitive) {
            char_val |= 0x20;  // Convert to lowercase
        }
        if (char_val != target_char) {
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
        char char_val = *current;
        if (case_insensitive) {
            char_val |= 0x20;  // Convert to lowercase
        }
        if (char_val != target_char) {
            break;
        }
    }
    
    return current;
}


} // namespace simd
} // namespace ctre

#endif // CTRE__SIMD_CHARACTER_CLASSES__HPP