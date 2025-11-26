#ifndef CTRE_BITNFA_SPECIALIZED_MATCHERS_HPP
#define CTRE_BITNFA_SPECIALIZED_MATCHERS_HPP

#include "pattern_traits.hpp"
#include <string_view>

// Specialized compile-time-generated matchers for simple patterns
// These generate CTRE-like direct comparison code

namespace ctre::bitnfa {

// match_result is defined in bitnfa_match.hpp which includes this file

// =============================================================================
// String Literal Matching (e.g., "abc")
// =============================================================================

template <auto... Chars>
struct string_matcher {
    static constexpr size_t length = sizeof...(Chars);

    // Match: entire input must equal the string
    static inline bool match(std::string_view input) {
        if (input.size() != length)
            return false;

        size_t i = 0;
        return ((input[i++] == static_cast<char>(Chars)) && ...);
    }

    // Search: find first occurrence
    template <typename Result>
    static inline Result search(std::string_view input) {
        if (input.size() < length) {
            return {0, 0, false};
        }

        // Simple scan for full pattern
        for (size_t pos = 0; pos <= input.size() - length; ++pos) {
            // Check if pattern matches at this position
            size_t i = 0;
            bool matches = ((input[pos + (i++)] == static_cast<char>(Chars)) && ...);
            if (matches) {
                return {pos, length, true};
            }
        }

        return {0, 0, false};
    }
};

// Specialization for string type
template <auto... Chars>
inline bool fast_match(std::string_view input, ctre::string<Chars...>*) {
    return string_matcher<Chars...>::match(input);
}

template <auto... Chars, typename Result>
inline Result fast_search(std::string_view input, ctre::string<Chars...>*, Result*) {
    return string_matcher<Chars...>::template search<Result>(input);
}

// =============================================================================
// Single Character Matching (e.g., 'a')
// =============================================================================

template <auto C>
struct char_matcher {
    static constexpr char target = static_cast<char>(C);

    static inline bool match(std::string_view input) {
        return input.size() == 1 && input[0] == target;
    }

    template <typename Result>
    static inline Result search(std::string_view input) {
        for (size_t i = 0; i < input.size(); ++i) {
            if (input[i] == target) {
                return {i, 1, true};
            }
        }
        return {0, 0, false};
    }
};

template <auto C>
inline bool fast_match(std::string_view input, ctre::character<C>*) {
    return char_matcher<C>::match(input);
}

template <auto C, typename Result>
inline Result fast_search(std::string_view input, ctre::character<C>*, Result*) {
    return char_matcher<C>::template search<Result>(input);
}

// =============================================================================
// Simple Sequence Matching (e.g., sequence of characters)
// =============================================================================

// Match sequence of simple characters
template <typename... Content>
inline bool fast_match_sequence(std::string_view input, ctre::sequence<Content...>*) {
    // For now, fall back to generic (can be optimized later)
    return false; // Signal to use generic path
}

template <typename... Content, typename Result>
inline Result fast_search_sequence(std::string_view input, ctre::sequence<Content...>*, Result*) {
    return {0, 0, false}; // Signal to use generic path
}

} // namespace ctre::bitnfa

#endif // CTRE_BITNFA_SPECIALIZED_MATCHERS_HPP
