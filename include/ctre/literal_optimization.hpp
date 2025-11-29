#ifndef CTRE_LITERAL_OPTIMIZATION_HPP
#define CTRE_LITERAL_OPTIMIZATION_HPP

#include "literal_alternation_fast_path.hpp"
#include "teddy_complete.hpp"
#include "atoms.hpp"

// Unified Literal Optimization Module
// ====================================
// Automatically optimizes literal alternation patterns
// Integrates Complete Teddy (~1150 lines) with CTRE

namespace ctre {
namespace literal_opt {

// Check if pattern is a literal alternation at compile-time
template <typename Pattern>
struct is_optimizable {
    static constexpr bool value = is_literal_alt<Pattern>::value;
};

// Extract literals and prepare for optimization
template <typename Pattern>
constexpr auto get_optimization_data() {
    if constexpr (is_literal_alt<Pattern>::value) {
        return get_literal_list<Pattern>();
    } else {
        return literal_list<1, 1>();  // Dummy
    }
}

// Optimized match for literal alternations
template <typename Pattern>
inline auto optimized_match(std::string_view input) {
    if constexpr (is_literal_alt<Pattern>::value) {
        constexpr auto literals = get_literal_list<Pattern>();
        size_t len = teddy_complete::match(input, literals);

        // Return compatible result
        if (len > 0) {
            // Matched! Create a successful result
            struct result {
                bool matched = true;
                operator bool() const { return matched; }
                std::string_view to_view() const { return {}; }
            };
            return result{};
        }
    }

    // No match
    struct result {
        bool matched = false;
        operator bool() const { return matched; }
        std::string_view to_view() const { return {}; }
    };
    return result{};
}

// Optimized search for literal alternations
template <typename Pattern>
inline auto optimized_search(std::string_view input) {
    if constexpr (is_literal_alt<Pattern>::value) {
        constexpr auto literals = get_literal_list<Pattern>();
        size_t match_len = 0;
        const char* pos = teddy_complete::search(input, literals, &match_len);

        if (pos != nullptr) {
            // Found! Create a successful result
            struct result {
                const char* match_begin;
                size_t match_length;
                bool matched = true;

                operator bool() const { return matched; }
                std::string_view to_view() const {
                    return std::string_view(match_begin, match_length);
                }
            };
            return result{pos, match_len, true};
        }
    }

    // Not found
    struct result {
        const char* match_begin = nullptr;
        size_t match_length = 0;
        bool matched = false;

        operator bool() const { return matched; }
        std::string_view to_view() const { return {}; }
    };
    return result{};
}

} // namespace literal_opt
} // namespace ctre

#endif // CTRE_LITERAL_OPTIMIZATION_HPP
