#ifndef CTRE__SIMD_MULTIRANGE__HPP
#define CTRE__SIMD_MULTIRANGE__HPP

#include "atoms_characters.hpp"
#include "simd_detection.hpp"
#include <immintrin.h>
#include <tuple>
#include <utility>

namespace ctre::simd {

// Range info extraction
template <typename T> struct range_info;
template <auto A, auto B> struct range_info<char_range<A, B>> { static constexpr char min = A, max = B; };
template <auto C> struct range_info<character<C>> { static constexpr char min = C, max = C; };

template <typename T> struct is_char_range_type : std::false_type {};
template <auto A, auto B> struct is_char_range_type<char_range<A, B>> : std::true_type {};
template <auto C> struct is_char_range_type<character<C>> : std::true_type {};
template <typename T> inline constexpr bool is_char_range_type_v = is_char_range_type<T>::value;

// Multi-range detector
template <typename T> struct is_multi_range : std::false_type {
    static constexpr size_t num_ranges = 0;
    static constexpr bool all_are_ranges = false;
    static constexpr bool is_valid = false;
};

template <typename... Ranges>
struct is_multi_range<set<Ranges...>> : std::true_type {
    static constexpr size_t num_ranges = sizeof...(Ranges);
    static constexpr bool all_are_ranges = (is_char_range_type_v<Ranges> && ...);
    static constexpr bool is_valid = (num_ranges >= 2) && all_are_ranges;

    template <size_t N> [[nodiscard]] static consteval char get_min() noexcept {
        return range_info<std::tuple_element_t<N, std::tuple<Ranges...>>>::min;
    }
    template <size_t N> [[nodiscard]] static consteval char get_max() noexcept {
        return range_info<std::tuple_element_t<N, std::tuple<Ranges...>>>::max;
    }
};

template <typename T> inline constexpr bool is_multi_range_v = is_multi_range<T>::value;
template <typename T> inline constexpr bool is_valid_multi_range_v = is_multi_range<T>::is_valid;

template <typename T> struct is_two_range : std::false_type {};
template <typename... R> struct is_two_range<set<R...>>
    : std::bool_constant<is_multi_range<set<R...>>::num_ranges == 2 && is_valid_multi_range_v<set<R...>>> {};

template <typename T> struct is_three_range : std::false_type {};
template <typename... R> struct is_three_range<set<R...>>
    : std::bool_constant<is_multi_range<set<R...>>::num_ranges == 3 && is_valid_multi_range_v<set<R...>>> {};

// Range check helpers
[[nodiscard]] inline __m256i check_range_avx2(__m256i data, unsigned char min_c, unsigned char max_c) noexcept {
    __m256i min_vec = _mm256_set1_epi8(static_cast<char>(min_c));
    __m256i adjusted = _mm256_sub_epi8(data, min_vec);
    __m256i width = _mm256_set1_epi8(static_cast<char>(max_c - min_c));
    return _mm256_cmpeq_epi8(_mm256_min_epu8(adjusted, width), adjusted);
}

[[nodiscard]] inline __m128i check_range_sse(__m128i data, unsigned char min_c, unsigned char max_c) noexcept {
    __m128i min_vec = _mm_set1_epi8(static_cast<char>(min_c));
    __m128i adjusted = _mm_sub_epi8(data, min_vec);
    __m128i width = _mm_set1_epi8(static_cast<char>(max_c - min_c));
    return _mm_cmpeq_epi8(_mm_min_epu8(adjusted, width), adjusted);
}

// SSE implementation
template <typename PatternType, size_t... Is, typename Iterator, typename EndIterator>
[[nodiscard]] inline Iterator match_n_range_sse_impl(Iterator current, EndIterator last, size_t& count,
                                                      std::index_sequence<Is...>) noexcept {
    while (last - current >= 16) {
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&*current));
        __m128i result = _mm_setzero_si128();
        ((result = _mm_or_si128(result, check_range_sse(data,
            static_cast<unsigned char>(is_multi_range<PatternType>::template get_min<Is>()),
            static_cast<unsigned char>(is_multi_range<PatternType>::template get_max<Is>())))), ...);

        int mask = _mm_movemask_epi8(result);
        if (mask == 0xFFFF) { current += 16; count += 16; }
        else { int m = __builtin_ctz(~mask & 0xFFFF); current += m; count += m; break; }
    }
    return current;
}

template <typename PatternType, typename Iterator, typename EndIterator>
[[nodiscard]] inline Iterator match_n_range_sse(Iterator current, EndIterator last, size_t& count) noexcept {
    return match_n_range_sse_impl<PatternType>(current, last, count,
        std::make_index_sequence<is_multi_range<PatternType>::num_ranges>{});
}

// AVX2 implementation
template <typename PatternType, size_t... Is, typename Iterator, typename EndIterator>
[[nodiscard]] inline Iterator match_n_range_avx2_impl(Iterator current, EndIterator last, size_t& count,
                                                       std::index_sequence<Is...>) noexcept {
    while (last - current >= 32) {
        __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*current));
        __m256i result = _mm256_setzero_si256();
        ((result = _mm256_or_si256(result, check_range_avx2(data,
            static_cast<unsigned char>(is_multi_range<PatternType>::template get_min<Is>()),
            static_cast<unsigned char>(is_multi_range<PatternType>::template get_max<Is>())))), ...);

        int mask = _mm256_movemask_epi8(result);
        if (static_cast<unsigned>(mask) == 0xFFFFFFFFU) { current += 32; count += 32; }
        else { int m = __builtin_ctz(~static_cast<unsigned>(mask)); current += m; count += m; break; }
    }
    return current;
}

template <typename PatternType, typename Iterator, typename EndIterator>
[[nodiscard]] inline Iterator match_n_range_avx2(Iterator current, EndIterator last, size_t& count) noexcept {
    return match_n_range_avx2_impl<PatternType>(current, last, count,
        std::make_index_sequence<is_multi_range<PatternType>::num_ranges>{});
}

// Scalar range check using fold expression
template <typename PatternType, size_t... Is>
[[nodiscard]] constexpr bool matches_any_range(char ch, std::index_sequence<Is...>) noexcept {
    return ((ch >= is_multi_range<PatternType>::template get_min<Is>() &&
             ch <= is_multi_range<PatternType>::template get_max<Is>()) || ...);
}

// Main dispatcher
template <typename PatternType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
[[nodiscard]] inline Iterator match_multirange_repeat(Iterator current, EndIterator last,
                                                       [[maybe_unused]] const flags& f) noexcept {
    Iterator start = current;
    size_t count = 0;

    if constexpr (is_valid_multi_range_v<PatternType> && can_use_simd()) {
        auto remaining = last - current;
#ifdef __AVX2__
        if (remaining >= 16 && remaining < 32)
            current = match_n_range_sse<PatternType>(current, last, count);
        else if (remaining >= 32)
            current = match_n_range_avx2<PatternType>(current, last, count);
#elif defined(__SSE4_2__) || defined(__SSE2__)
        if (remaining >= 16)
            current = match_n_range_sse<PatternType>(current, last, count);
#endif
    }

    // Scalar tail using fold expression
    while (current != last && (MaxCount == 0 || count < MaxCount)) {
        if constexpr (is_valid_multi_range_v<PatternType>) {
            if (matches_any_range<PatternType>(*current,
                    std::make_index_sequence<is_multi_range<PatternType>::num_ranges>{})) {
                ++current; ++count;
            } else break;
        } else break;
    }
    return (count >= MinCount) ? current : start;
}

} // namespace ctre::simd

#endif // CTRE__SIMD_MULTIRANGE__HPP
