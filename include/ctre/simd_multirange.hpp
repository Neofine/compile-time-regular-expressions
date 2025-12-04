#ifndef CTRE__SIMD_MULTIRANGE__HPP
#define CTRE__SIMD_MULTIRANGE__HPP

#include "atoms_characters.hpp"
#include "simd_detection.hpp"
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

// FIX: Treat single character as a range [C, C]
template <auto C>
struct range_info<character<C>> {
    static constexpr char min = C;
    static constexpr char max = C;
};

// Helper to check if type is char_range (or single character, treated as range)
template <typename T>
struct is_char_range_type : std::false_type {};

template <auto A, auto B>
struct is_char_range_type<char_range<A, B>> : std::true_type {};

// FIX: Single characters are also valid "ranges" for SIMD purposes
template <auto C>
struct is_char_range_type<character<C>> : std::true_type {};

// Generic multi-range detector - works for 2, 3, 4, ... N ranges
template <typename T>
struct is_multi_range : std::false_type {
    static constexpr size_t num_ranges = 0;
    static constexpr bool all_are_ranges = false;
    // TEMPORARILY DISABLED - has bugs, use single-range SIMD instead
    static constexpr bool is_valid = false; // Was: would check num_ranges >= 2
};

template <typename... Ranges>
struct is_multi_range<set<Ranges...>> : std::true_type {
    static constexpr size_t num_ranges = sizeof...(Ranges);

    // Check if ALL elements are char_range (not individual characters or other types)
    static constexpr bool all_are_ranges = (... && is_char_range_type<Ranges>::value);

    // Re-enabled for .match operations (bugs were in .search)
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
struct is_two_range<set<Ranges...>>
    : std::bool_constant<is_multi_range<set<Ranges...>>::num_ranges == 2 && is_multi_range<set<Ranges...>>::is_valid> {
};

template <typename T>
struct is_three_range : std::false_type {};

template <typename... Ranges>
struct is_three_range<set<Ranges...>>
    : std::bool_constant<is_multi_range<set<Ranges...>>::num_ranges == 3 && is_multi_range<set<Ranges...>>::is_valid> {
};

// ============================================================================
// Generic AVX2 Multi-Range Implementation (handles 2-N ranges)
// ============================================================================

// Optimized single-range check using unsigned comparisons (faster) - AVX2 version
inline __m256i check_range_avx2(__m256i data, unsigned char min_char, unsigned char max_char) {
    // Use unsigned saturating subtract for efficient range check
    // For a value to be in [min, max]:
    //   (unsigned)(value - min) <= (unsigned)(max - min)

    __m256i min_vec = _mm256_set1_epi8(static_cast<char>(min_char));
    __m256i adjusted = _mm256_sub_epi8(data, min_vec);

    // Range width
    unsigned char range_width = max_char - min_char;
    __m256i width_vec = _mm256_set1_epi8(static_cast<char>(range_width));

    // Compare using unsigned min (returns min of two unsigned bytes)
    __m256i clamped = _mm256_min_epu8(adjusted, width_vec);

    // If adjusted <= width, then clamped == adjusted
    return _mm256_cmpeq_epi8(clamped, adjusted);
}

// Optimized single-range check - SSE version (16 bytes)
inline __m128i check_range_sse(__m128i data, unsigned char min_char, unsigned char max_char) {
    __m128i min_vec = _mm_set1_epi8(static_cast<char>(min_char));
    __m128i adjusted = _mm_sub_epi8(data, min_vec);

    unsigned char range_width = max_char - min_char;
    __m128i width_vec = _mm_set1_epi8(static_cast<char>(range_width));

    __m128i clamped = _mm_min_epu8(adjusted, width_vec);
    return _mm_cmpeq_epi8(clamped, adjusted);
}

// ============================================================================
// SSE Multi-Range Implementation (16 bytes)
// ============================================================================

template <typename PatternType, size_t... Indices, typename Iterator, typename EndIterator>
inline Iterator match_n_range_sse_impl(Iterator current, const EndIterator last, size_t& count,
                                       std::index_sequence<Indices...>) {
    constexpr size_t chunk_size = 16;

    while (true) {
        auto remaining = last - current;
        if (remaining < chunk_size)
            break;

        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&*current));

        // Check all ranges and OR results
        __m128i result = _mm_setzero_si128();

        ((result = _mm_or_si128(
              result, check_range_sse(
                          data, static_cast<unsigned char>(is_multi_range<PatternType>::template get_min<Indices>()),
                          static_cast<unsigned char>(is_multi_range<PatternType>::template get_max<Indices>())))),
         ...);

        int mask = _mm_movemask_epi8(result);
        if (mask == 0xFFFF) {
            current += chunk_size;
            count += chunk_size;
        } else {
            // Find first non-matching byte
            int first_mismatch = __builtin_ctz(~mask & 0xFFFF);
            current += first_mismatch;
            count += first_mismatch;
            break;
        }
    }

    return current;
}

// Generic wrapper for SSE N-range
template <typename PatternType, typename Iterator, typename EndIterator>
inline Iterator match_n_range_sse(Iterator current, const EndIterator last, size_t& count) {
    constexpr size_t N = is_multi_range<PatternType>::num_ranges;
    return match_n_range_sse_impl<PatternType>(current, last, count, std::make_index_sequence<N>{});
}

// ============================================================================
// AVX2 Multi-Range Implementation (32 bytes)
// ============================================================================

template <typename PatternType, size_t... Indices, typename Iterator, typename EndIterator>
inline Iterator match_n_range_avx2_impl(Iterator current, const EndIterator last, size_t& count,
                                        std::index_sequence<Indices...>) {
    constexpr size_t chunk_size = 32;

    while (true) {
        auto remaining = last - current;
        if (remaining < chunk_size)
            break;

        __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*current));

        // Check all ranges and OR results using optimized helper
        __m256i result = _mm256_setzero_si256();

        // Use fold expression correctly - expand Indices pack
        ((result = _mm256_or_si256(
              result, check_range_avx2(
                          data, static_cast<unsigned char>(is_multi_range<PatternType>::template get_min<Indices>()),
                          static_cast<unsigned char>(is_multi_range<PatternType>::template get_max<Indices>())))),
         ...);

        int mask = _mm256_movemask_epi8(result);
        if (static_cast<unsigned int>(mask) == 0xFFFFFFFFU) {
            current += chunk_size;
            count += chunk_size;
        } else {
            // Find first non-matching byte
            int first_mismatch = __builtin_ctz(~static_cast<unsigned int>(mask));
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
    // Compile-time SIMD dispatch for zero runtime overhead
    if constexpr (is_multi_range<PatternType>::is_valid && can_use_simd()) {
        auto remaining = last - current;

#ifdef __AVX2__
        // Try SSE first for 16-31 byte inputs (faster than loading AVX2 tables)
        if (remaining >= 16 && remaining < 32) {
            current = match_n_range_sse<PatternType>(current, last, count);
        }
        // Use AVX2 for 32+ byte inputs
        else if (remaining >= 32) {
            current = match_n_range_avx2<PatternType>(current, last, count);
        }
#elif defined(__SSE4_2__) || defined(__SSE2__)
        if (remaining >= 16) {
            current = match_n_range_sse<PatternType>(current, last, count);
        }
#endif
    }

    // Continue with scalar matching for:
    // 1. Tail bytes (< 32 bytes remaining after SIMD)
    // 2. Continuing the match beyond SIMD chunks
    // 3. Fallback if SIMD not available
    while (current != last && (MaxCount == 0 || count < MaxCount)) {
        char ch = *current;
        bool matches = false;

        // Check if character matches any range using compile-time expansion
        if constexpr (is_multi_range<PatternType>::is_valid) {
            // Manually expand for each range (safer than lambda + fold)
            constexpr size_t N = is_multi_range<PatternType>::num_ranges;
            if constexpr (N >= 1) {
                matches = (ch >= is_multi_range<PatternType>::template get_min<0>() &&
                           ch <= is_multi_range<PatternType>::template get_max<0>());
            }
            if constexpr (N >= 2) {
                matches = matches || (ch >= is_multi_range<PatternType>::template get_min<1>() &&
                                      ch <= is_multi_range<PatternType>::template get_max<1>());
            }
            if constexpr (N >= 3) {
                matches = matches || (ch >= is_multi_range<PatternType>::template get_min<2>() &&
                                      ch <= is_multi_range<PatternType>::template get_max<2>());
            }
            if constexpr (N >= 4) {
                matches = matches || (ch >= is_multi_range<PatternType>::template get_min<3>() &&
                                      ch <= is_multi_range<PatternType>::template get_max<3>());
            }
            // FIX: Added support for 5-8 ranges (common in real-world patterns)
            if constexpr (N >= 5) {
                matches = matches || (ch >= is_multi_range<PatternType>::template get_min<4>() &&
                                      ch <= is_multi_range<PatternType>::template get_max<4>());
            }
            if constexpr (N >= 6) {
                matches = matches || (ch >= is_multi_range<PatternType>::template get_min<5>() &&
                                      ch <= is_multi_range<PatternType>::template get_max<5>());
            }
            if constexpr (N >= 7) {
                matches = matches || (ch >= is_multi_range<PatternType>::template get_min<6>() &&
                                      ch <= is_multi_range<PatternType>::template get_max<6>());
            }
            if constexpr (N >= 8) {
                matches = matches || (ch >= is_multi_range<PatternType>::template get_min<7>() &&
                                      ch <= is_multi_range<PatternType>::template get_max<7>());
            }
        }

        if (matches) {
            ++current;
            ++count;
        } else {
            break;
        }
    }

    return (count >= MinCount) ? current : start;
}

} // namespace simd
} // namespace ctre

#endif // CTRE__SIMD_MULTIRANGE__HPP
