#ifndef CTRE_BITNFA_COMPILE_HPP
#define CTRE_BITNFA_COMPILE_HPP

#include "bitnfa_types.hpp"
#include "../glushkov_nfa.hpp"

// Phase 2: Compilation from CTRE Glushkov NFA to BitNFA
// Convert compile-time Glushkov NFA to runtime bit-based NFA

namespace ctre::bitnfa {

// Helper to unwrap CTRE pattern to raw AST (same as decomposition.hpp)
template <typename T>
struct unwrap_pattern {
    using type = T;
};

template <size_t Index, typename... Content>
struct unwrap_pattern<ctre::capture<Index, Content...>> {
    using type = ctre::sequence<Content...>;
};

template <size_t Index, typename Content>
struct unwrap_pattern<ctre::capture<Index, Content>> {
    using type = Content;
};

template <typename T>
using unwrap_pattern_t = typename unwrap_pattern<T>::type;

// Compile a pattern to BitNFA
// Takes CTRE's AST and converts it to BitNFA format
// Note: Returns runtime object, but uses compile-time info
template <typename Pattern>
inline auto compile_pattern() {
    // Unwrap captures and get raw AST
    using RawAST = unwrap_pattern_t<Pattern>;

    // Get the Glushkov NFA for this pattern
    constexpr auto glushkov = glushkov::glushkov_nfa<RawAST>();

    BitNFA128 nfa;
    nfa.state_count = glushkov.state_count;

    // Step 1: Build shift masks from transitions
    for (size_t from = 0; from < glushkov.state_count; ++from) {
        for (size_t i = 0; i < glushkov.states[from].successor_count; ++i) {
            size_t to = glushkov.states[from].successors[i];

            // Calculate span
            if (to > from) {
                size_t span = to - from;
                if (span <= 7) {
                    // Typical transition (forward, span ≤ 7)
                    nfa.shift_masks.set_transition(from, to);
                } else {
                    // Exception: large forward span
                    nfa.set_exception(from);
                    nfa.add_exception_successor(from, to);
                }
            } else {
                // Exception: backward transition (loops, etc.)
                nfa.set_exception(from);
                nfa.add_exception_successor(from, to);
            }
        }
    }

    // Step 2: Build reachability table from state symbols
    for (size_t state = 1; state < glushkov.state_count; ++state) {
        char symbol = glushkov.states[state].symbol;

        if (symbol == '?') {
            // Character class placeholder - for now, treat as any character
            // (Real implementation would need more info from AST)
            nfa.reachability.set_reachable_any(state);
        } else if (symbol == '.') {
            // Dot - matches any character
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
    for (size_t i = 0; i < glushkov.accept_count; ++i) {
        size_t accept_state = glushkov.accept_states[i];
        nfa.set_accept(accept_state);
    }

    return nfa;
}

} // namespace ctre::bitnfa

#endif // CTRE_BITNFA_COMPILE_HPP
