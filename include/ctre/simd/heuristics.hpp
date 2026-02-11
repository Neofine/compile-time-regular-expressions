#ifndef CTRE__SIMD_HEURISTICS__HPP
#define CTRE__SIMD_HEURISTICS__HPP

#include <cstddef>

namespace ctre::simd {

template <typename Pattern>
struct simd_suitability {
    static constexpr bool is_suitable = true;
    static constexpr size_t min_beneficial_size = 32;
};

template <typename... Content>
[[nodiscard]] consteval size_t count_segments(sequence<Content...>*) noexcept { return sizeof...(Content); }

template <typename Pattern>
[[nodiscard]] consteval size_t pattern_segment_count() noexcept {
    if constexpr (requires { count_segments(static_cast<Pattern*>(nullptr)); })
        return count_segments(static_cast<Pattern*>(nullptr));
    return 1;
}

template <typename Pattern>
[[nodiscard]] consteval size_t get_min_beneficial_size() noexcept {
    constexpr size_t segments = pattern_segment_count<Pattern>();
    if constexpr (segments >= 5) return 128;
    else if constexpr (segments >= 3) return 64;
    else return 32;
}

} // namespace ctre::simd

#endif // CTRE__SIMD_HEURISTICS__HPP
