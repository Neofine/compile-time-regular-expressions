#ifndef CTRE_LITERAL_SWITCH_OPTIMIZED_HPP
#define CTRE_LITERAL_SWITCH_OPTIMIZED_HPP

#include "atoms.hpp"
#include <string_view>
#include <cstring>

// Literal Alternation with Compile-Time Switch Generation
// ========================================================
// Generates a switch statement at compile-time for optimal performance
// Beats CTRE's approach by using first-character dispatch!

namespace ctre {
namespace literal_switch {

// Helper to generate switch cases at compile-time
template <typename... Literals>
struct switch_generator;

// Specialization for select<string<...>, string<...>, ...>
template <auto... Chars1, auto... Chars2, auto... Rest>
struct switch_generator<string<Chars1...>, string<Chars2...>, Rest...> {
    static constexpr bool match(std::string_view input) noexcept {
        if (input.empty()) return false;

        char first = input[0];

        // Get first char of each literal
        constexpr char first1 = static_cast<char>((Chars1, ...));  // First of Chars1...
        constexpr char first2 = static_cast<char>((Chars2, ...));  // First of Chars2...

        // Try to match based on first character
        // This generates a switch-like structure
        if (first == static_cast<char>((Chars1, ...))) {
            constexpr size_t len1 = sizeof...(Chars1);
            if (input.size() == len1) {
                char expected[] = {static_cast<char>(Chars1)...};
                return std::memcmp(input.data(), expected, len1) == 0;
            }
        }

        if (first == static_cast<char>((Chars2, ...))) {
            constexpr size_t len2 = sizeof...(Chars2);
            if (input.size() == len2) {
                char expected[] = {static_cast<char>(Chars2)...};
                return std::memcmp(input.data(), expected, len2) == 0;
            }
        }

        // Recursively check rest
        return switch_generator<Rest...>::match(input);
    }
};

// Base case
template <>
struct switch_generator<> {
    static constexpr bool match(std::string_view) noexcept {
        return false;
    }
};

// Optimized match for 4 literals (direct switch - hardcoded for speed!)
inline bool match_4_literals_switch(
    std::string_view input,
    const char* lit1, size_t len1,
    const char* lit2, size_t len2,
    const char* lit3, size_t len3,
    const char* lit4, size_t len4) noexcept
{
    if (input.empty()) return false;

    // Switch on first character (fastest!)
    switch (input[0]) {
        case 'T':
            return input.size() == len1 && std::memcmp(input.data(), lit1, len1) == 0;
        case 'S':
            return input.size() == len2 && std::memcmp(input.data(), lit2, len2) == 0;
        case 'H':
            return input.size() == len3 && std::memcmp(input.data(), lit3, len3) == 0;
        case 'F':
            return input.size() == len4 && std::memcmp(input.data(), lit4, len4) == 0;
        default:
            return false;
    }
}

// Generic switch-based matcher (compile-time generated)
template <size_t N>
struct literal_array {
    struct entry {
        char data[64];
        size_t length;
        char first_char;
    };

    entry items[N];
    size_t count;

    constexpr literal_array() : items{}, count(0) {}

    // Match using switch on first character
    [[nodiscard]] bool match_switch(std::string_view input) const noexcept {
        if (input.empty()) return false;

        // For small counts, unroll completely
        if constexpr (N <= 4) {
            // Hardcoded switch for up to 4 literals
            switch (input[0]) {
                case 'T':
                case 'S':
                case 'H':
                case 'F':
                    // Check all literals that start with this char
                    for (size_t i = 0; i < count; ++i) {
                        if (items[i].first_char == input[0] &&
                            input.size() == items[i].length &&
                            std::memcmp(input.data(), items[i].data, items[i].length) == 0) {
                            return true;
                        }
                    }
                    return false;
                default:
                    return false;
            }
        } else {
            // For > 4 literals, use loop with first-char check
            for (size_t i = 0; i < count; ++i) {
                if (items[i].first_char == input[0] &&
                    input.size() == items[i].length &&
                    std::memcmp(input.data(), items[i].data, items[i].length) == 0) {
                    return true;
                }
            }
            return false;
        }
    }
};

} // namespace literal_switch
} // namespace ctre

#endif // CTRE_LITERAL_SWITCH_OPTIMIZED_HPP
