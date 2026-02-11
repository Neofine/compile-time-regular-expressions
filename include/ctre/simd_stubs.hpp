#ifndef CTRE__SIMD_STUBS__HPP
#define CTRE__SIMD_STUBS__HPP

#include <cstddef>
#include <string_view>

namespace ctre::simd {

// Stub implementations when SIMD is disabled
template <typename T> struct simd_pattern_trait {};

template <typename T> struct is_multi_range {
    static constexpr bool is_valid = false;
};

template <typename T> struct shufti_pattern_trait {
    static constexpr bool should_use_shufti = false;
};

template <typename... Tail>
[[nodiscard]] constexpr bool has_literal_next() noexcept {
    return false;
}

template <typename Iterator>
[[nodiscard]] Iterator match_string_simd(Iterator begin, Iterator, const flags&) noexcept {
    return begin;
}

template <typename T, typename Iterator>
[[nodiscard]] Iterator match_sequence_fused(T*, Iterator begin, Iterator) noexcept {
    return begin;
}

template <typename Pattern, typename A, typename B, typename Iterator>
[[nodiscard]] Iterator match_multirange_repeat(Iterator begin, Iterator, const flags&) noexcept {
    return begin;
}

template <typename Pattern, typename A, typename B, typename Iterator>
[[nodiscard]] Iterator match_pattern_repeat_shufti(Iterator begin, Iterator, const flags&) noexcept {
    return begin;
}

template <typename Pattern, typename A, typename B, typename Iterator>
[[nodiscard]] Iterator match_pattern_repeat_simd(Iterator begin, Iterator, const flags&) noexcept {
    return begin;
}

} // namespace ctre::simd

#endif
