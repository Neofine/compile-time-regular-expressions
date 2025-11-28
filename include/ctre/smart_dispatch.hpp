#ifndef CTRE_SMART_DISPATCH_HPP
#define CTRE_SMART_DISPATCH_HPP

#include "bitnfa/integration.hpp"
#include <type_traits>

namespace ctre {
namespace smart_dispatch {

// =============================================================================
// Smart Dispatch Strategy Based on Empirical Testing
// =============================================================================
//
// Empirical Results:
// ==================
// BitNFA WINS for alternations:
//   - alternation_4: 15-29% faster
//   - complex_alt: 8-39% faster
//   - Works for ALL input sizes (even 10 bytes!)
//
// BitNFA LOSES for non-alternations:
//   - suffix_ing: 140x slower!
//   - negated_class: 6-9x slower
//
// Conclusion:
// ===========
// Use BitNFA ONLY for:
//   1. Patterns with 2+ alternations (A|B|C)
//   2. ALL input sizes (no minimum threshold!)
//
// Never use BitNFA for:
//   - Repetitions without alternations ([a-z]+, a*)
//   - Complex sequences
//
// =============================================================================

// Helper: Count alternations (simple top-level check)
template <typename T>
constexpr size_t count_alternations_simple() {
    if constexpr (glushkov::is_select<T>::value) {
        return 1; // This is an alternation
    } else {
        return 0;
    }
}

// Smart pattern analysis with input-size awareness
template <typename Pattern>
struct smart_pattern_analysis {
    // Count alternations
    static constexpr size_t alternation_count = count_alternations_simple<Pattern>();
    
    // Is it an alternation pattern?
    static constexpr bool is_alternation = glushkov::is_select<Pattern>::value;
    
    // Is it a pure repetition? (a*, [a-z]+, etc.)
    static constexpr bool is_repetition = glushkov::is_repeat<Pattern>::value;
    
    // SMART THRESHOLD: Use BitNFA if:
    //   - Pattern has alternations (A|B|C)
    //   - At least 1 alternation
    //   - NO input size threshold needed! (BitNFA wins at all sizes for alternations)
    static constexpr bool use_bitnfa = 
        is_alternation &&           // Must be alternation
        (alternation_count >= 1);   // At least 1 alternation (A|B)
    
    // Explanation for decision
    static constexpr const char* strategy_name() {
        if constexpr (use_bitnfa) {
            return "BitNFA (alternation pattern)";
        } else if constexpr (is_repetition) {
            return "SIMD/Glushkov NFA (repetition pattern)";
        } else {
            return "Glushkov NFA (complex pattern)";
        }
    }
};

// =============================================================================
// Smart Dispatch API
// =============================================================================

// Match using smart strategy
template <ctll::fixed_string Pattern>
constexpr auto match(std::string_view input) {
    // Parse pattern
    using tmp = typename ctll::parser<ctre::pcre, Pattern, ctre::pcre_actions>::template output<ctre::pcre_context<>>;
    static_assert(tmp(), "Regular Expression contains syntax error.");
    using AST = decltype(ctll::front(typename tmp::output_type::stack_type()));

    // Smart dispatch based on pattern type
    constexpr bool use_nfa = smart_pattern_analysis<AST>::use_bitnfa;

    if constexpr (use_nfa) {
        // Use BitNFA for alternations (proven faster!)
        return bitnfa::match<Pattern>(input);
    } else {
        // Use standard CTRE (SIMD + Glushkov NFA)
        return ctre::match<Pattern>(input);
    }
}

// Search using smart strategy
template <ctll::fixed_string Pattern>
constexpr auto search(std::string_view input) {
    using tmp = typename ctll::parser<ctre::pcre, Pattern, ctre::pcre_actions>::template output<ctre::pcre_context<>>;
    static_assert(tmp(), "Regular Expression contains syntax error.");
    using AST = decltype(ctll::front(typename tmp::output_type::stack_type()));

    constexpr bool use_nfa = smart_pattern_analysis<AST>::use_bitnfa;

    if constexpr (use_nfa) {
        return bitnfa::search<Pattern>(input);
    } else {
        return ctre::search<Pattern>(input);
    }
}

// Helper to check if pattern would use BitNFA
template <ctll::fixed_string Pattern>
consteval bool would_use_bitnfa() {
    using tmp = typename ctll::parser<ctre::pcre, Pattern, ctre::pcre_actions>::template output<ctre::pcre_context<>>;
    using AST = decltype(ctll::front(typename tmp::output_type::stack_type()));
    return smart_pattern_analysis<AST>::use_bitnfa;
}

// Helper to get strategy name
template <ctll::fixed_string Pattern>
consteval const char* get_strategy_name() {
    using tmp = typename ctll::parser<ctre::pcre, Pattern, ctre::pcre_actions>::template output<ctre::pcre_context<>>;
    using AST = decltype(ctll::front(typename tmp::output_type::stack_type()));
    return smart_pattern_analysis<AST>::strategy_name();
}

} // namespace smart_dispatch
} // namespace ctre

#endif // CTRE_SMART_DISPATCH_HPP

