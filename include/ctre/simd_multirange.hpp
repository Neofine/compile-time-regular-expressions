#ifndef CTRE__SIMD_MULTIRANGE__HPP
#define CTRE__SIMD_MULTIRANGE__HPP

#include "simd_detection.hpp"
#include "atoms_characters.hpp"
#include <immintrin.h>
#include <tuple>
#include <utility>

namespace ctre {
namespace simd {

// ============================================================================
// Generic Multi-Range Pattern Traits (handles any number of ranges)
// ============================================================================

// Helper to extract range info
template <typename T>
struct range_info;

template <auto A, auto B>
struct range_info<char_range<A, B>> {
    static constexpr char min = A;
    static constexpr char max = B;
};

// Helper to check if type is char_range
template <typename T>
struct is_char_range_type : std::false_type {};

template <auto A, auto B>
struct is_char_range_type<char_range<A, B>> : std::true_type {};

// Generic multi-range detector - works for 2, 3, 4, ... N ranges
template <typename T>
struct is_multi_range : std::false_type {
    static constexpr size_t num_ranges = 0;
    static constexpr bool all_are_ranges = false;
    static constexpr bool is_valid = false;
};

template <typename... Ranges>
struct is_multi_range<set<Ranges...>> : std::true_type {
    static constexpr size_t num_ranges = sizeof...(Ranges);

    // Check if ALL elements are char_range (not individual characters or other types)
    static constexpr bool all_are_ranges = (... && is_char_range_type<Ranges>::value);

    // Only consider it a multi-range if we have 2+ char_range elements
    // This matches patterns like [a-zA-Z] (2 ranges) or [0-9a-fA-F] (3 ranges)
    // Does NOT match [aeiou] (5 individual characters - should use Shufti)
    static constexpr bool is_valid = (num_ranges >= 2) && all_are_ranges;

    // Helper to get min/max for Nth range
    template <size_t N>
    static constexpr char get_min() {
        return range_info<std::tuple_element_t<N, std::tuple<Ranges...>>>::min;
    }

    template <size_t N>
    static constexpr char get_max() {
        return range_info<std::tuple_element_t<N, std::tuple<Ranges...>>>::max;
    }
};

// Legacy compatibility
template <typename T>
struct is_two_range : std::false_type {};

template <typename... Ranges>
struct is_two_range<set<Ranges...>> : std::bool_constant<is_multi_range<set<Ranges...>>::num_ranges == 2 && is_multi_range<set<Ranges...>>::is_valid> {};

template <typename T>
struct is_three_range : std::false_type {};

template <typename... Ranges>
struct is_three_range<set<Ranges...>> : std::bool_constant<is_multi_range<set<Ranges...>>::num_ranges == 3 && is_multi_range<set<Ranges...>>::is_valid> {};

// ============================================================================
// Generic AVX2 Multi-Range Implementation (handles 2-N ranges)
// ============================================================================

// Helper to check a single range with SIMD
inline __m256i check_range_avx2(__m256i data, char min_char, char max_char) {
    __m256i min_vec = _mm256_set1_epi8(min_char);
    __m256i max_vec = _mm256_set1_epi8(max_char);

    // FIX: Avoid signed char overflow - rewrite to avoid wraparound
    // data >= min is !(min > data)
    __m256i lt_min = _mm256_cmpgt_epi8(min_vec, data);
    __m256i ge = _mm256_xor_si256(lt_min, _mm256_set1_epi8(static_cast<char>(0xFF)));

    // data <= max is !(data > max)
    __m256i gt_max = _mm256_cmpgt_epi8(data, max_vec);
    __m256i le = _mm256_xor_si256(gt_max, _mm256_set1_epi8(static_cast<char>(0xFF)));

    return _mm256_and_si256(ge, le);
}

// Generic N-range matching using recursion
template <typename PatternType, size_t... Indices, typename Iterator, typename EndIterator>
inline Iterator match_n_range_avx2_impl(Iterator current, const EndIterator last, size_t& count, std::index_sequence<Indices...>) {
    constexpr size_t chunk_size = 32;

    // Only use this implementation for iterator types that support distance checking
    // (e.g., std::string iterators, not C-string sentinel iterators)
    if constexpr (!requires { std::distance(current, last); }) {
        return current;  // Fall back to scalar for unsupported iterator types
    }

    while (std::distance(current, last) >= chunk_size) {
        __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*current));

        // Check all ranges and OR them together using fold expression
        __m256i result = _mm256_setzero_si256();
        (void)((result = _mm256_or_si256(result, check_range_avx2(
            data,
            is_multi_range<PatternType>::template get_min<Indices>(),
            is_multi_range<PatternType>::template get_max<Indices>()
        )), 0) , ...);

        int mask = _mm256_movemask_epi8(result);
        if (static_cast<unsigned int>(mask) == 0xFFFFFFFFU) {
            current += chunk_size;
            count += chunk_size;
        } else {
            int first_mismatch = __builtin_ctz(~mask);
            current += first_mismatch;
            count += first_mismatch;
            break;
        }
    }

    return current;
}

// Generic wrapper for any number of ranges
template <typename PatternType, typename Iterator, typename EndIterator>
inline Iterator match_n_range_avx2(Iterator current, const EndIterator last, size_t& count) {
    constexpr size_t N = is_multi_range<PatternType>::num_ranges;
    return match_n_range_avx2_impl<PatternType>(current, last, count, std::make_index_sequence<N>{});
}

// ============================================================================
// Generic Dispatcher for N-range patterns (2, 3, 4, ... unlimited!)
// ============================================================================

template <typename PatternType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_multirange_repeat(Iterator current, const EndIterator last, const flags& f) {
    Iterator start = current;
    size_t count = 0;

    // Use generic N-range SIMD if this is a valid multi-range pattern
    if constexpr (is_multi_range<PatternType>::is_valid) {
        if (can_use_simd() && get_simd_capability() >= SIMD_CAPABILITY_AVX2) {
            current = match_n_range_avx2<PatternType>(current, last, count);
        }
    }

    return (count >= MinCount || MinCount == 0) ? current : start;
}

} // namespace simd
} // namespace ctre

#endif // CTRE__SIMD_MULTIRANGE__HPP
