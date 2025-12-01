#ifndef CTRE__SIMD_PATTERN_ANALYSIS__HPP
#define CTRE__SIMD_PATTERN_ANALYSIS__HPP

#include "../ctll/list.hpp"

namespace ctre {
namespace simd {

// Helper to get first type from parameter pack
template <typename T, typename...>
struct first_type { using type = T; };

// Detect if the next element in pattern is a literal or sequence
// This indicates a sequence pattern (e.g., [0-9]+\. in IPv4)
template <typename... Tail>
constexpr bool has_literal_next() {
    if constexpr (sizeof...(Tail) == 0) {
        return false;
    } else {
        using NextElement = typename first_type<Tail...>::type;

        // Check if it's a character literal (has value member)
        if constexpr (requires { NextElement::value; }) {
            return true;
        }
        // Check if it's a sequence wrapper (common in complex patterns)
        else if constexpr (requires { typename NextElement::head_type; } ||
                          requires { typename NextElement::tail_type; }) {
            return true;
        }

        return false;
    }
}

// Detect if pattern is a pure repetition (good for SIMD) or sequence (bad for SIMD)
template <typename Content, typename... Tail>
struct pattern_suitability {
    // Check if next element is a literal
    static constexpr bool is_sequence = has_literal_next<Tail...>();

    // SIMD is suitable if:
    // 1. Not a sequence pattern (no literal after repetition)
    // 2. Content is a character class that benefits from SIMD
    static constexpr bool is_simd_suitable = !is_sequence;
};

} // namespace simd
} // namespace ctre

#endif
