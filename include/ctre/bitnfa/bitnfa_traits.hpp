#ifndef CTRE_BITNFA_TRAITS_HPP
#define CTRE_BITNFA_TRAITS_HPP

#include "../glushkov_nfa.hpp"

// BitNFA Pattern Analysis Traits
// HEADER-ONLY, NO CIRCULAR DEPENDENCIES
// Safe to include from wrapper.hpp

namespace ctre::bitnfa {

template <typename T>
constexpr bool is_alternation_pattern() {
    return glushkov::is_select<T>::value;
}

template <typename T>
constexpr bool is_repetition_pattern() {
    return glushkov::is_repeat<T>::value;
}

// BitNFA for alternations
template <typename Pattern>
struct bitnfa_suitability {
    static constexpr bool is_alternation = is_alternation_pattern<Pattern>();
    static constexpr bool is_repetition = is_repetition_pattern<Pattern>();
    static constexpr bool should_use_bitnfa = is_alternation;

    static constexpr const char* strategy_name() {
        if constexpr (should_use_bitnfa) return "BitNFA";
        else if constexpr (is_repetition) return "SIMD/Glushkov";
        else return "Glushkov";
    }
};

} // namespace ctre::bitnfa

#endif // CTRE_BITNFA_TRAITS_HPP
