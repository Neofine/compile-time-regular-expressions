#ifndef CTRE_BITNFA_COMPILE_FROM_GLUSHKOV_HPP
#define CTRE_BITNFA_COMPILE_FROM_GLUSHKOV_HPP

#include "bitnfa_types.hpp"

// Note: This header requires ctre.hpp to be included before it
// (compile_from_glushkov.hpp uses CTRE's parser and Glushkov NFA types)

// Phase 2b: Automatic Compilation from Glushkov NFA
// Convert CTRE's Glushkov NFA to BitNFA format

namespace ctre::bitnfa {

// Compile a Glushkov NFA to BitNFA
// This is a runtime function (but uses constexpr-constructed BitNFA)
template <typename GlushkovNFA>
inline BitNFA128 compile_from_glushkov(const GlushkovNFA& glushkov_nfa) {
    BitNFA128 nfa;
    nfa.state_count = glushkov_nfa.state_count;

    // Step 1: Build shift masks and mark exceptions from transitions
    for (size_t from = 0; from < glushkov_nfa.state_count; ++from) {
        const auto& state = glushkov_nfa.states[from];

        for (size_t i = 0; i < state.successor_count; ++i) {
            size_t to = state.successors[i];

            if (to > from) {
                // Forward transition
                size_t span = to - from;
                if (span <= 7) {
                    // Typical transition - add to shift masks
                    nfa.shift_masks.set_transition(from, to);
                } else {
                    // Large forward span - exception
                    nfa.set_exception(from);
                    nfa.add_exception_successor(from, to);
                }
            } else {
                // Backward transition (self-loops, etc.) - exception
                nfa.set_exception(from);
                nfa.add_exception_successor(from, to);
            }
        }
    }

    // Step 2: Build reachability table from state symbols
    for (size_t state = 1; state < glushkov_nfa.state_count; ++state) {
        const auto& state_info = glushkov_nfa.states[state];
        char symbol = state_info.symbol;

        if (symbol == '.') {
            // Dot - matches any character
            nfa.reachability.set_reachable_any(state);
        } else if (symbol == '?') {
            // Character class placeholder
            // For now, treat as any character
            // TODO: Use actual character class info from AST
            nfa.reachability.set_reachable_any(state);
        } else if (symbol >= 'a' && symbol <= 'z') {
            // Lowercase letter
            nfa.reachability.set_reachable(symbol, state);
        } else if (symbol >= 'A' && symbol <= 'Z') {
            // Uppercase letter
            nfa.reachability.set_reachable(symbol, state);
        } else if (symbol >= '0' && symbol <= '9') {
            // Digit
            nfa.reachability.set_reachable(symbol, state);
        } else if (symbol != '\0') {
            // Other printable character
            nfa.reachability.set_reachable(symbol, state);
        }
    }

    // Step 3: Mark accept states
    for (size_t i = 0; i < glushkov_nfa.accept_count; ++i) {
        size_t accept_state = glushkov_nfa.accept_states[i];
        nfa.set_accept(accept_state);
    }

    return nfa;
}

// Helper: Parse pattern string and compile to BitNFA
// Takes ctll::fixed_string at compile-time, returns BitNFA at runtime
template <ctll::fixed_string Pattern>
inline BitNFA128 compile_pattern_string() {
    // Parse the pattern to get CTRE AST
    using tmp = typename ctll::parser<ctre::pcre, Pattern, ctre::pcre_actions>::template output<ctre::pcre_context<>>;
    static_assert(tmp(), "Regular Expression contains syntax error.");
    using AST = decltype(ctll::front(typename tmp::output_type::stack_type()));

    // Get Glushkov NFA from AST
    constexpr auto nfa_glushkov = ctre::glushkov::glushkov_nfa<AST>();

    // Convert to BitNFA
    return compile_from_glushkov(nfa_glushkov);
}

} // namespace ctre::bitnfa

#endif // CTRE_BITNFA_COMPILE_FROM_GLUSHKOV_HPP
