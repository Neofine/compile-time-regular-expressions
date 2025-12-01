#ifndef CTRE_BITNFA_TRAITS_HPP
#define CTRE_BITNFA_TRAITS_HPP

#include "../glushkov_nfa.hpp"

// BitNFA Pattern Analysis Traits
// HEADER-ONLY, NO CIRCULAR DEPENDENCIES
// Safe to include from wrapper.hpp

namespace ctre {
namespace bitnfa {

// Helper: Check if pattern is an alternation
template <typename T>
constexpr bool is_alternation_pattern() {
    return glushkov::is_select<T>::value;
}

// Helper: Check if pattern is a repetition
template <typename T>
constexpr bool is_repetition_pattern() {
    return glushkov::is_repeat<T>::value;
}

// BitNFA suitability analysis
// Based on empirical testing from smart_dispatch.hpp:
// - BitNFA WINS on alternations: 15-39% faster
// - BitNFA LOSES on everything else: 6-140x slower
template <typename Pattern>
struct bitnfa_suitability {
    // Is it an alternation pattern?
    static constexpr bool is_alternation = is_alternation_pattern<Pattern>();

    // Is it a repetition pattern?
    static constexpr bool is_repetition = is_repetition_pattern<Pattern>();

    // Should we use BitNFA?
    // YES if: alternation pattern
    // NO if: repetition or other pattern
    static constexpr bool should_use_bitnfa = is_alternation;

    // Strategy name for debugging
    static constexpr const char* strategy_name() {
        if constexpr (should_use_bitnfa) {
            return "BitNFA";
        } else if constexpr (is_repetition) {
            return "SIMD/Glushkov";
        } else {
            return "Glushkov";
        }
    }
};

} // namespace bitnfa
} // namespace ctre

#endif // CTRE_BITNFA_TRAITS_HPP
