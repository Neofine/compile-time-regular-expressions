#ifndef CTRE_BITNFA_PATTERN_TRAITS_HPP
#define CTRE_BITNFA_PATTERN_TRAITS_HPP

#include <cstddef>

// Compile-time pattern analysis for optimization selection

namespace ctre {
    // Forward declarations to avoid pulling in full CTRE headers
    template <auto... Chars> struct string;
    template <auto C> struct character;
    template <typename... Content> struct sequence;
    template <typename... Content> struct select;
}

namespace ctre::bitnfa {

// =============================================================================
// Pattern Type Detection
// =============================================================================

template <typename T> struct is_string : std::false_type {};
template <auto... Chars> struct is_string<ctre::string<Chars...>> : std::true_type {};

template <typename T> struct is_character : std::false_type {};
template <auto C> struct is_character<ctre::character<C>> : std::true_type {};

template <typename T> struct is_sequence : std::false_type {};
template <typename... Content> struct is_sequence<ctre::sequence<Content...>> : std::true_type {};

// =============================================================================
// Pattern Complexity Analysis
// =============================================================================

// Count states in pattern (approximation)
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

// =============================================================================
// Fast Path Eligibility
// =============================================================================

template <typename Pattern>
struct should_use_fast_path {
    static constexpr bool value =
        is_string<Pattern>::value ||                    // Simple string
        is_character<Pattern>::value ||                 // Single character
        (is_sequence<Pattern>::value &&                 // Simple sequence
         state_count<Pattern>::value < 10);             // Not too long
};

} // namespace ctre::bitnfa

#endif // CTRE_BITNFA_PATTERN_TRAITS_HPP
