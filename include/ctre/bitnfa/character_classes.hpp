#ifndef CTRE_BITNFA_CHARACTER_CLASSES_HPP
#define CTRE_BITNFA_CHARACTER_CLASSES_HPP

#include "bitnfa_types.hpp"
#include "../atoms_characters.hpp"

// Phase 5a: Character Class Support for BitNFA
// Properly expand character classes ([a-z], [0-9], [^abc]) into reachability table

namespace ctre::bitnfa {

// =============================================================================
// Character Class Expansion
// =============================================================================

// Expand a single character into reachability table
template <auto V>
constexpr void expand_character(ReachabilityTable& table, size_t state) {
    table.set_reachable_mut(static_cast<char>(V), state);
}

// Expand a character range into reachability table
template <auto A, auto B>
constexpr void expand_char_range(ReachabilityTable& table, size_t state) {
    char min_c = static_cast<char>(A);
    char max_c = static_cast<char>(B);
    table.set_reachable_range(min_c, max_c, state);
}

// Expand character set (positive) into reachability table
template <typename... Content>
constexpr void expand_set(ReachabilityTable& table, size_t state) {
    // Expand each content element
    (expand_char_class_element<Content>(table, state), ...);
}

// Forward declaration for recursion
template <typename T>
constexpr void expand_char_class_element(ReachabilityTable& table, size_t state);

// Expand a single character class element (generic)
template <typename T>
constexpr void expand_char_class_element(ReachabilityTable& table, size_t state) {
    if constexpr (requires { T::template match_char<char>; }) {
        // General case: test all 256 characters
        for (int c = 0; c < 256; ++c) {
            if (T::match_char(static_cast<char>(c), ctre::flags{})) {
                table.set_reachable_mut(static_cast<char>(c), state);
            }
        }
    }
}

// Note: Specialized versions removed - generic template function above handles all cases

// =============================================================================
// High-Level API
// =============================================================================

// Check if a type is a character class
template <typename T>
constexpr bool is_char_class_type() {
    // Check for various character class patterns
    if constexpr (requires { typename T::is_char_class; }) {
        return true;
    } else if constexpr (requires { T::template match_char<char>; }) {
        // Has match_char method - likely a character class
        return true;
    } else {
        return false;
    }
}

// Expand any character class type into reachability table
template <typename T>
constexpr void expand_any_char_class(ReachabilityTable& table, size_t state) {
    // Generic expansion: test all 256 characters
    for (int c = 0; c < 256; ++c) {
        if (T::match_char(static_cast<char>(c), ctre::flags{})) {
            table.set_reachable_mut(static_cast<char>(c), state);
        }
    }
}

} // namespace ctre::bitnfa

#endif // CTRE_BITNFA_CHARACTER_CLASSES_HPP
