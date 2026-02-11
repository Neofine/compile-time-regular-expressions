#ifndef CTRE_SMART_DISPATCH_HPP
#define CTRE_SMART_DISPATCH_HPP

#include "bitnfa/integration.hpp"
#include <type_traits>

namespace ctre::smart_dispatch {

// Count top-level alternations
template <typename T>
constexpr size_t count_alternations_simple() {
    if constexpr (glushkov::is_select<T>::value) {
        return 1;
    } else {
        return 0;
    }
}

// Pattern analysis for dispatch decision
template <typename Pattern>
struct smart_pattern_analysis {
    static constexpr size_t alternation_count = count_alternations_simple<Pattern>();
    static constexpr bool is_alternation = glushkov::is_select<Pattern>::value;
    static constexpr bool is_repetition = glushkov::is_repeat<Pattern>::value;

    // Use BitNFA for alternation patterns
    static constexpr bool use_bitnfa = is_alternation && (alternation_count >= 1);

    static constexpr const char* strategy_name() {
        if constexpr (use_bitnfa) return "BitNFA";
        else if constexpr (is_repetition) return "SIMD";
        else return "Glushkov";
    }
};

// Query helpers
template <ctll::fixed_string Pattern>
consteval bool would_use_bitnfa() {
    using tmp = typename ctll::parser<ctre::pcre, Pattern, ctre::pcre_actions>::template output<ctre::pcre_context<>>;
    using AST = decltype(ctll::front(typename tmp::output_type::stack_type()));
    return smart_pattern_analysis<AST>::use_bitnfa;
}

template <ctll::fixed_string Pattern>
consteval const char* get_strategy_name() {
    using tmp = typename ctll::parser<ctre::pcre, Pattern, ctre::pcre_actions>::template output<ctre::pcre_context<>>;
    using AST = decltype(ctll::front(typename tmp::output_type::stack_type()));
    return smart_pattern_analysis<AST>::strategy_name();
}

} // namespace ctre::smart_dispatch

#endif // CTRE_SMART_DISPATCH_HPP
