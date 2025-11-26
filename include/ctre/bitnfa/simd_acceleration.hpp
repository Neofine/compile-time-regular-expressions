#ifndef CTRE_BITNFA_SIMD_ACCELERATION_HPP
#define CTRE_BITNFA_SIMD_ACCELERATION_HPP

#include "../simd_detection.hpp"
#include "../simd_character_classes.hpp"
#include "../atoms.hpp"
#include <cstddef>
#include <cstring>
#include <string_view>

using namespace ctre::simd;

// Hyperscan-style SIMD acceleration for BitNFA
// Two-layer architecture:
// 1. Acceleration Layer (SIMD) - Skip to interesting positions
// 2. NFA Layer (State machine) - Only run when necessary

namespace ctre::bitnfa {

// =============================================================================
// Layer 1: SIMD Acceleration - Skip Forward Quickly
// =============================================================================

// Helper: Find next position where a character class STOPS matching
// Scan forward from 'begin' while characters match, return first non-match
template <typename CharClassType>
__attribute__((always_inline)) inline const char* simd_find_char_class_end(const char* begin, const char* end) {
    // Use SIMD to scan matching range for simple character ranges
    if constexpr (requires { simd_pattern_trait<CharClassType>::min_char;
                             simd_pattern_trait<CharClassType>::max_char; }) {
        constexpr char min_char = simd_pattern_trait<CharClassType>::min_char;
        constexpr char max_char = simd_pattern_trait<CharClassType>::max_char;

        const char* current = begin;

        if (can_use_simd() && get_simd_capability() >= SIMD_CAPABILITY_AVX2) {
            // Use AVX2 to find first NON-matching character
            const char* scan_end = end - 32;

            __m256i min_vec = _mm256_set1_epi8(min_char);
            __m256i max_vec = _mm256_set1_epi8(max_char);

            while (current <= scan_end) {
                __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(current));
                __m256i ge_min = _mm256_cmpgt_epi8(data, _mm256_sub_epi8(min_vec, _mm256_set1_epi8(1)));
                __m256i le_max = _mm256_cmpgt_epi8(_mm256_add_epi8(max_vec, _mm256_set1_epi8(1)), data);
                __m256i result = _mm256_and_si256(ge_min, le_max);

                int mask = _mm256_movemask_epi8(result);
                if (static_cast<unsigned int>(mask) != 0xFFFFFFFFU) {
                    // Found a non-matching character!
                    int first_nonmatch = __builtin_ctz(~mask);
                    return current + first_nonmatch;
                }
                current += 32;
            }
        }

        // Scalar tail
        while (current < end && *current >= min_char && *current <= max_char) {
            ++current;
        }
        return current;
    } else {
        // Fallback for non-range character classes
        const char* current = begin;
        while (current < end && CharClassType::match_char(*current, flags{})) {
            ++current;
        }
        return current;
    }
}

// Helper: Find next position where a character class matches
// Returns end() if no match found
template <typename CharClassType>
__attribute__((always_inline)) inline const char* simd_find_char_class(const char* begin, const char* end) {
    if (!can_use_simd()) {
        // Scalar fallback
        for (const char* p = begin; p != end; ++p) {
            if (CharClassType::match_char(*p, flags{})) {
                return p;
            }
        }
        return end;
    }

    // SIMD acceleration using CTRE's existing infrastructure
    if constexpr (requires { simd::simd_pattern_trait<CharClassType>::min_char;
                             simd::simd_pattern_trait<CharClassType>::max_char; }) {
        // Character range like [a-z]
        constexpr char min_char = simd::simd_pattern_trait<CharClassType>::min_char;
        constexpr char max_char = simd::simd_pattern_trait<CharClassType>::max_char;

        if (get_simd_capability() >= SIMD_CAPABILITY_AVX2) {
            // Use AVX2 for fast scanning
            const char* current = begin;
            const char* scan_end = end - 32;

            __m256i min_vec = _mm256_set1_epi8(min_char);
            __m256i max_vec = _mm256_set1_epi8(max_char);

            while (current <= scan_end) {
                __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(current));
                __m256i ge_min = _mm256_cmpgt_epi8(data, _mm256_sub_epi8(min_vec, _mm256_set1_epi8(1)));
                __m256i le_max = _mm256_cmpgt_epi8(_mm256_add_epi8(max_vec, _mm256_set1_epi8(1)), data);
                __m256i result = _mm256_and_si256(ge_min, le_max);

                int mask = _mm256_movemask_epi8(result);
                if (mask != 0) {
                    // Found a match!
                    int first_match = __builtin_ctz(mask);
                    return current + first_match;
                }
                current += 32;
            }

            // Scalar tail
            for (; current != end; ++current) {
                if (*current >= min_char && *current <= max_char) {
                    return current;
                }
            }
            return end;
        } else if (get_simd_capability() >= SIMD_CAPABILITY_SSE42) {
            // Use SSE4.2 for scanning
            const char* current = begin;
            const char* scan_end = end - 16;

            __m128i min_vec = _mm_set1_epi8(min_char);
            __m128i max_vec = _mm_set1_epi8(max_char);

            while (current <= scan_end) {
                __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(current));
                __m128i ge_min = _mm_cmpgt_epi8(data, _mm_sub_epi8(min_vec, _mm_set1_epi8(1)));
                __m128i le_max = _mm_cmpgt_epi8(_mm_add_epi8(max_vec, _mm_set1_epi8(1)), data);
                __m128i result = _mm_and_si128(ge_min, le_max);

                int mask = _mm_movemask_epi8(result);
                if (mask != 0) {
                    int first_match = __builtin_ctz(mask);
                    return current + first_match;
                }
                current += 16;
            }

            // Scalar tail
            for (; current != end; ++current) {
                if (*current >= min_char && *current <= max_char) {
                    return current;
                }
            }
            return end;
        }
    }

    // Fallback: scalar search
    for (const char* p = begin; p != end; ++p) {
        if (CharClassType::match_char(*p, flags{})) {
            return p;
        }
    }
    return end;
}

// Helper: Find next single character
inline const char* simd_find_char(const char* begin, const char* end, char target) {
    if (!can_use_simd() || end - begin < 16) {
        // Scalar for short strings
        return static_cast<const char*>(memchr(begin, target, end - begin));
    }

    if (get_simd_capability() >= SIMD_CAPABILITY_AVX2) {
        const char* current = begin;
        const char* scan_end = end - 32;

        __m256i target_vec = _mm256_set1_epi8(target);

        while (current <= scan_end) {
            __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(current));
            __m256i result = _mm256_cmpeq_epi8(data, target_vec);

            int mask = _mm256_movemask_epi8(result);
            if (mask != 0) {
                int first_match = __builtin_ctz(mask);
                return current + first_match;
            }
            current += 32;
        }

        // Scalar tail
        for (; current != end; ++current) {
            if (*current == target) return current;
        }
        return end;
    }

    // Use memchr for SSE4.2 (it's optimized)
    const char* result = static_cast<const char*>(memchr(begin, target, end - begin));
    return result ? result : end;
}

// =============================================================================
// Pattern Analysis for Acceleration
// =============================================================================

// Detect if a pattern can use SIMD acceleration
template <typename AST>
struct can_accelerate : std::false_type {};

// Repeat patterns with character classes: [a-z]+, [0-9]+, etc.
// AST is repeat<1, 0, set<char_range<'a', 'z'>>>
template <size_t A, size_t B, typename Content>
struct can_accelerate<ctre::repeat<A, B, Content>> {
    // Check if Content has SIMD traits (set<char_range<>> has them)
    static constexpr bool value = requires {
        { simd_pattern_trait<Content>::min_char } -> std::convertible_to<char>;
        { simd_pattern_trait<Content>::max_char } -> std::convertible_to<char>;
    };
};

// Helper to extract content from repeat
template <typename T>
struct extract_repeat_content;

template <size_t A, size_t B, typename Content>
struct extract_repeat_content<ctre::repeat<A, B, Content>> {
    using type = Content;
};

} // namespace ctre::bitnfa

#endif // CTRE_BITNFA_SIMD_ACCELERATION_HPP
