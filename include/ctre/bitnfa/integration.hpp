#ifndef CTRE_BITNFA_INTEGRATION_HPP
#define CTRE_BITNFA_INTEGRATION_HPP

#include "bitnfa_match.hpp"

// Phase 4: CTRE Integration
// Make BitNFA available as an alternative evaluation engine

namespace ctre {
namespace bitnfa {

// =============================================================================
// Phase 4a: Pattern Complexity Analysis
// =============================================================================

// Helper: Count alternations in pattern
template <typename T>
constexpr size_t count_alternations() {
    if constexpr (glushkov::is_select<T>::value) {
        return 1; // This is an alternation
    } else if constexpr (glushkov::is_sequence<T>::value) {
        // Count in sequence children
        return 0; // TODO: Recurse through children
    } else {
        return 0;
    }
}

// Determine if a pattern should use BitNFA or standard CTRE engine
// BitNFA is beneficial for:
// - Alternations with many branches
// - Patterns with many states
// - NFAs that don't DFA-ize well
// - Patterns with backreferences or complex lookarounds (future)

template <typename Pattern>
struct pattern_analysis {
    // Count alternation depth
    static constexpr size_t alternation_count = count_alternations<Pattern>();

    // Count total states in Glushkov NFA
    static constexpr size_t state_count = []() {
        if constexpr (requires { glushkov::glushkov_nfa<Pattern>().state_count; }) {
            return glushkov::glushkov_nfa<Pattern>().state_count;
        } else {
            return 0;
        }
    }();

    // Heuristic: Use BitNFA if pattern is complex
    static constexpr bool use_bitnfa =
        (state_count > 16) ||           // Many states
        (alternation_count > 3);        // Many alternations
};

// =============================================================================
// Phase 4b: Unified Match API
// =============================================================================

// Match using best engine (BitNFA or standard CTRE)
template <ctll::fixed_string Pattern>
constexpr auto match_auto(std::string_view input) {
    // Parse pattern
    using tmp = typename ctll::parser<ctre::pcre, Pattern, ctre::pcre_actions>::template output<ctre::pcre_context<>>;
    static_assert(tmp(), "Regular Expression contains syntax error.");
    using AST = decltype(ctll::front(typename tmp::output_type::stack_type()));

    // Analyze pattern complexity
    constexpr bool use_nfa = pattern_analysis<AST>::use_bitnfa;

    if constexpr (use_nfa) {
        // Use BitNFA for complex patterns
        return bitnfa::match<Pattern>(input);
    } else {
        // Use standard CTRE for simple patterns
        return ctre::match<Pattern>(input);
    }
}

// Search using best engine
template <ctll::fixed_string Pattern>
constexpr auto search_auto(std::string_view input) {
    using tmp = typename ctll::parser<ctre::pcre, Pattern, ctre::pcre_actions>::template output<ctre::pcre_context<>>;
    static_assert(tmp(), "Regular Expression contains syntax error.");
    using AST = decltype(ctll::front(typename tmp::output_type::stack_type()));

    constexpr bool use_nfa = pattern_analysis<AST>::use_bitnfa;

    if constexpr (use_nfa) {
        return bitnfa::search<Pattern>(input);
    } else {
        return ctre::search<Pattern>(input);
    }
}

// =============================================================================
// Phase 4c: Benchmark Helpers
// =============================================================================

// Force BitNFA engine (for benchmarking)
template <ctll::fixed_string Pattern>
struct bitnfa_engine {
    static auto match(std::string_view input) {
        return bitnfa::match<Pattern>(input);
    }

    static auto search(std::string_view input) {
        return bitnfa::search<Pattern>(input);
    }

    static auto find_all(std::string_view input) {
        return bitnfa::find_all<Pattern>(input);
    }
};

// Force standard CTRE engine (for benchmarking)
template <ctll::fixed_string Pattern>
struct ctre_engine {
    static auto match(std::string_view input) {
        return ctre::match<Pattern>(input);
    }

    static auto search(std::string_view input) {
        return ctre::search<Pattern>(input);
    }
};

} // namespace bitnfa
} // namespace ctre

#endif // CTRE_BITNFA_INTEGRATION_HPP
