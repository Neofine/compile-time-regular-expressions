#ifndef CTRE_BITNFA_PATTERN_TRAITS_HPP
#define CTRE_BITNFA_PATTERN_TRAITS_HPP

#include "../pattern_traits.hpp"
#include <cstddef>

namespace ctre::bitnfa {

// Import common traits from consolidated header
using namespace ctre::traits;

// Pattern Complexity Analysis
template <typename T> struct state_count { static constexpr size_t value = 1; };

template <auto... Chars>
struct state_count<ctre::string<Chars...>> {
    static constexpr size_t value = sizeof...(Chars) + 1;
};

template <typename... Content>
struct state_count<ctre::sequence<Content...>> {
    static constexpr size_t value = (state_count<Content>::value + ...);
};

template <typename... Content>
struct state_count<ctre::select<Content...>> {
    static constexpr size_t value = (state_count<Content>::value + ... + 0);
};

template <typename T> inline constexpr size_t state_count_v = state_count<T>::value;

// Fast Path Eligibility
template <typename Pattern>
struct should_use_fast_path {
    static constexpr bool value =
        is_string_v<Pattern> ||
        is_character_v<Pattern> ||
        (is_sequence_v<Pattern> && state_count_v<Pattern> < 10);
};

template <typename T> inline constexpr bool should_use_fast_path_v = should_use_fast_path<T>::value;

} // namespace ctre::bitnfa

#endif // CTRE_BITNFA_PATTERN_TRAITS_HPP
