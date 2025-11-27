#ifndef CTRE__SIMD_MULTIRANGE__HPP
#define CTRE__SIMD_MULTIRANGE__HPP

#include "simd_detection.hpp"
#include "atoms_characters.hpp"
#include <immintrin.h>

namespace ctre {
namespace simd {

// ============================================================================
// Multi-Range Pattern Traits (for patterns with gaps like [a-zA-Z])
// ============================================================================

// Detect 2-range patterns like [a-zA-Z]
template <typename T>
struct is_two_range : std::false_type {};

template <auto A1, auto B1, auto A2, auto B2>
struct is_two_range<set<char_range<A1, B1>, char_range<A2, B2>>> : std::true_type {
    static constexpr char min1 = A1;
    static constexpr char max1 = B1;
    static constexpr char min2 = A2;
    static constexpr char max2 = B2;
};

// Detect 3-range patterns like [0-9a-fA-F]
template <typename T>
struct is_three_range : std::false_type {};

template <auto A1, auto B1, auto A2, auto B2, auto A3, auto B3>
struct is_three_range<set<char_range<A1, B1>, char_range<A2, B2>, char_range<A3, B3>>> : std::true_type {
    static constexpr char min1 = A1;
    static constexpr char max1 = B1;
    static constexpr char min2 = A2;
    static constexpr char max2 = B2;
    static constexpr char min3 = A3;
    static constexpr char max3 = B3;
};

// ============================================================================
// AVX2 Multi-Range Implementations
// ============================================================================

// Two-range matching (e.g., [a-zA-Z])
template <char Min1, char Max1, char Min2, char Max2, typename Iterator, typename EndIterator>
inline Iterator match_two_range_avx2(Iterator current, const EndIterator last, size_t& count) {
    if (std::distance(current, last) < 32) return current;

    __m256i min1_vec = _mm256_set1_epi8(Min1);
    __m256i max1_vec = _mm256_set1_epi8(Max1);
    __m256i min2_vec = _mm256_set1_epi8(Min2);
    __m256i max2_vec = _mm256_set1_epi8(Max2);

    while (std::distance(current, last) >= 32) {
        __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*current));

        // Check range 1: data >= Min1 && data <= Max1
        __m256i ge1 = _mm256_cmpgt_epi8(data, _mm256_sub_epi8(min1_vec, _mm256_set1_epi8(1)));
        __m256i le1 = _mm256_cmpgt_epi8(_mm256_add_epi8(max1_vec, _mm256_set1_epi8(1)), data);
        __m256i range1 = _mm256_and_si256(ge1, le1);

        // Check range 2: data >= Min2 && data <= Max2
        __m256i ge2 = _mm256_cmpgt_epi8(data, _mm256_sub_epi8(min2_vec, _mm256_set1_epi8(1)));
        __m256i le2 = _mm256_cmpgt_epi8(_mm256_add_epi8(max2_vec, _mm256_set1_epi8(1)), data);
        __m256i range2 = _mm256_and_si256(ge2, le2);

        // Combined: range1 OR range2
        __m256i result = _mm256_or_si256(range1, range2);

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

// Three-range matching (e.g., [0-9a-fA-F])
template <char Min1, char Max1, char Min2, char Max2, char Min3, char Max3, typename Iterator, typename EndIterator>
inline Iterator match_three_range_avx2(Iterator current, const EndIterator last, size_t& count) {
    if (std::distance(current, last) < 32) return current;

    __m256i min1_vec = _mm256_set1_epi8(Min1);
    __m256i max1_vec = _mm256_set1_epi8(Max1);
    __m256i min2_vec = _mm256_set1_epi8(Min2);
    __m256i max2_vec = _mm256_set1_epi8(Max2);
    __m256i min3_vec = _mm256_set1_epi8(Min3);
    __m256i max3_vec = _mm256_set1_epi8(Max3);

    while (std::distance(current, last) >= 32) {
        __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*current));

        // Check all three ranges
        __m256i ge1 = _mm256_cmpgt_epi8(data, _mm256_sub_epi8(min1_vec, _mm256_set1_epi8(1)));
        __m256i le1 = _mm256_cmpgt_epi8(_mm256_add_epi8(max1_vec, _mm256_set1_epi8(1)), data);
        __m256i range1 = _mm256_and_si256(ge1, le1);

        __m256i ge2 = _mm256_cmpgt_epi8(data, _mm256_sub_epi8(min2_vec, _mm256_set1_epi8(1)));
        __m256i le2 = _mm256_cmpgt_epi8(_mm256_add_epi8(max2_vec, _mm256_set1_epi8(1)), data);
        __m256i range2 = _mm256_and_si256(ge2, le2);

        __m256i ge3 = _mm256_cmpgt_epi8(data, _mm256_sub_epi8(min3_vec, _mm256_set1_epi8(1)));
        __m256i le3 = _mm256_cmpgt_epi8(_mm256_add_epi8(max3_vec, _mm256_set1_epi8(1)), data);
        __m256i range3 = _mm256_and_si256(ge3, le3);

        // Combined: range1 OR range2 OR range3
        __m256i result = _mm256_or_si256(_mm256_or_si256(range1, range2), range3);

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

// ============================================================================
// Dispatcher for multi-range patterns
// ============================================================================

template <typename PatternType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_multirange_repeat(Iterator current, const EndIterator last, const flags& f) {
    Iterator start = current;
    size_t count = 0;

    // Dispatch to 2-range SIMD
    if constexpr (is_two_range<PatternType>::value) {
        if (can_use_simd() && get_simd_capability() >= SIMD_CAPABILITY_AVX2) {
            using Trait = is_two_range<PatternType>;
            current = match_two_range_avx2<Trait::min1, Trait::max1, Trait::min2, Trait::max2>(current, last, count);
        }
    }
    // Dispatch to 3-range SIMD
    else if constexpr (is_three_range<PatternType>::value) {
        if (can_use_simd() && get_simd_capability() >= SIMD_CAPABILITY_AVX2) {
            using Trait = is_three_range<PatternType>;
            current = match_three_range_avx2<Trait::min1, Trait::max1, Trait::min2, Trait::max2, Trait::min3, Trait::max3>(current, last, count);
        }
    }

    return (count >= MinCount || MinCount == 0) ? current : start;
}

} // namespace simd
} // namespace ctre

#endif // CTRE__SIMD_MULTIRANGE__HPP
