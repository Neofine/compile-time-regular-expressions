#ifndef CTRE_BITNFA_FAST_PATH_HPP
#define CTRE_BITNFA_FAST_PATH_HPP

#include <string_view>
#include "bitnfa_types.hpp"

// Fast path optimizations: Generate specialized code for simple patterns at compile-time
// instead of running generic state machine

// Forward declarations from CTRE
namespace ctre {
    template <auto... Chars> struct string;
    template <auto C> struct character;
}

namespace ctre::bitnfa {

// =============================================================================
// Pattern Analysis (Compile-Time)
// =============================================================================

// Check if pattern is a simple literal string
template <typename Pattern>
struct is_simple_string : std::false_type {};

template <auto... Chars>
struct is_simple_string<ctre::string<Chars...>> : std::true_type {};

// Check if pattern is a single character
template <typename Pattern>
struct is_simple_char : std::false_type {};

template <auto C>
struct is_simple_char<ctre::character<C>> : std::true_type {};

// Check if pattern is simple enough for fast path
template <typename Pattern>
struct can_use_fast_path {
    static constexpr bool value = is_simple_string<Pattern>::value ||
                                   is_simple_char<Pattern>::value;
};

// =============================================================================
// Fast Path: Simple String Match
// =============================================================================

// Match a simple string pattern (e.g., "abc") with direct comparison
template <auto... Chars>
inline bool fast_match_string(std::string_view input, ctre::string<Chars...>*) {
    constexpr size_t len = sizeof...(Chars);
    if (input.size() != len) return false;

    size_t i = 0;
    return ((input[i++] == static_cast<char>(Chars)) && ...);
}

// =============================================================================
// Fast Path: Single Character Match
// =============================================================================

// Match a single character pattern
template <auto C>
inline bool fast_match_char(std::string_view input, ctre::character<C>*) {
    return input.size() == 1 && input[0] == static_cast<char>(C);
}

// =============================================================================
// Fast Path: Search Operations
// =============================================================================

// Search for simple string in input
template <auto... Chars>
inline match_result fast_search_string(std::string_view input, ctre::string<Chars...>*) {
    constexpr size_t pattern_len = sizeof...(Chars);
    constexpr char first_char = static_cast<char>((... , Chars)); // Get first char

    if (input.size() < pattern_len) {
        return {0, 0, false};
    }

    // Boyer-Moore-like: scan for first character
    for (size_t pos = 0; pos <= input.size() - pattern_len; ++pos) {
        if (input[pos] == first_char) {
            // Check remaining characters
            size_t i = 0;
            bool matches = ((input[pos + (i++)] == static_cast<char>(Chars)) && ...);
            if (matches) {
                return {pos, pattern_len, true};
            }
        }
    }

    return {0, 0, false};
}

// Search for single character
template <auto C>
inline match_result fast_search_char(std::string_view input, ctre::character<C>*) {
    constexpr char target = static_cast<char>(C);

    for (size_t i = 0; i < input.size(); ++i) {
        if (input[i] == target) {
            return {i, 1, true};
        }
    }

    return {0, 0, false};
}

// =============================================================================
// Fast Path Dispatcher
// =============================================================================

// Try fast path for match, return match_result
template <typename Pattern>
inline match_result try_fast_match(std::string_view input) {
    if constexpr (is_simple_string<Pattern>::value) {
        bool matched = fast_match_string(input, static_cast<Pattern*>(nullptr));
        return {0, matched ? input.size() : size_t(0), matched};
    } else if constexpr (is_simple_char<Pattern>::value) {
        bool matched = fast_match_char(input, static_cast<Pattern*>(nullptr));
        return {0, matched ? size_t(1) : size_t(0), matched};
    }
    return {0, 0, false}; // No fast path available
}

// Try fast path for search
template <typename Pattern>
inline match_result try_fast_search(std::string_view input) {
    if constexpr (is_simple_string<Pattern>::value) {
        return fast_search_string(input, static_cast<Pattern*>(nullptr));
    } else if constexpr (is_simple_char<Pattern>::value) {
        return fast_search_char(input, static_cast<Pattern*>(nullptr));
    }
    return {0, 0, false}; // No fast path available
}

} // namespace ctre::bitnfa

#endif // CTRE_BITNFA_FAST_PATH_HPP
