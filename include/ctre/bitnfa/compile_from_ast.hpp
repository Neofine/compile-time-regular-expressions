#ifndef CTRE_BITNFA_COMPILE_FROM_AST_HPP
#define CTRE_BITNFA_COMPILE_FROM_AST_HPP

#include "bitnfa_types.hpp"
#include "character_classes.hpp"
#include "../glushkov_nfa.hpp"

// Phase 5a: Direct AST to BitNFA compilation with character class support
// Extract character class type information directly from AST

namespace ctre::bitnfa {

// Helper: Extract reachability from AST pattern
// CONSTEXPR - Runs at compile-time!
template <typename Pattern>
constexpr void extract_reachability_from_ast(ReachabilityTable& table, size_t offset) {
    // Skip offset, count from 1 (state 0 is start)
    size_t state = offset + 1;

    if constexpr (glushkov::is_empty<Pattern>::value) {
        // No reachability
    } else if constexpr (glushkov::is_character<Pattern>::value) {
        // Single character
        []<auto C>(ctre::character<C>*, ReachabilityTable& tbl, size_t st) {
            tbl.set_reachable_mut(static_cast<char>(C), st);
        }(static_cast<Pattern*>(nullptr), table, state);
    } else if constexpr (glushkov::is_any<Pattern>::value) {
        // Dot - any character
        table.set_reachable_any(state);
    } else if constexpr (requires { Pattern::template match_char<char>; }) {
        // Character class! Use generic expansion
        expand_any_char_class<Pattern>(table, state);
    } else if constexpr (glushkov::is_string<Pattern>::value) {
        // String - expand each character
        []<auto... Cs>(ctre::string<Cs...>*, ReachabilityTable& tbl, size_t st) {
            size_t pos = st;
            ((tbl.set_reachable_mut(static_cast<char>(Cs), pos++)), ...);
        }(static_cast<Pattern*>(nullptr), table, state);
    } else if constexpr (glushkov::is_sequence<Pattern>::value) {
        // Sequence - recurse
        []<typename... Content>(ctre::sequence<Content...>*, ReachabilityTable& tbl, size_t off) {
            size_t current_offset = off;
            ((extract_reachability_from_ast<Content>(tbl, current_offset),
              current_offset += glushkov::count_positions<Content>()), ...);
        }(static_cast<Pattern*>(nullptr), table, offset);
    } else if constexpr (glushkov::is_select<Pattern>::value) {
        // Alternation - recurse into each option
        []<typename... Options>(ctre::select<Options...>*, ReachabilityTable& tbl, size_t off) {
            size_t current_offset = off;
            ((extract_reachability_from_ast<Options>(tbl, current_offset),
              current_offset += glushkov::count_positions<Options>()), ...);
        }(static_cast<Pattern*>(nullptr), table, offset);
    } else if constexpr (glushkov::is_repeat<Pattern>::value ||
                          glushkov::is_lazy_repeat<Pattern>::value ||
                          glushkov::is_possessive_repeat<Pattern>::value) {
        // Repeat - recurse into content
        []<size_t A, size_t B, typename... Content>(ctre::repeat<A, B, Content...>*, ReachabilityTable& tbl, size_t off) {
            (extract_reachability_from_ast<Content>(tbl, off), ...);
        }(static_cast<Pattern*>(nullptr), table, offset);
    }
}

// Compile pattern with character class support
// CONSTEXPR - Runs at compile-time!
template <typename Pattern>
constexpr BitNFA128 compile_with_charclass() {
    // Get Glushkov NFA for structure
    constexpr auto glushkov_nfa = ctre::glushkov::glushkov_nfa<Pattern>();

    // Build BitNFA from Glushkov structure
    BitNFA128 nfa;
    nfa.state_count = glushkov_nfa.state_count;

    // Step 1: Build transitions from Glushkov NFA
    for (size_t from = 0; from < glushkov_nfa.state_count; ++from) {
        const auto& state = glushkov_nfa.states[from];

        for (size_t i = 0; i < state.successor_count; ++i) {
            size_t to = state.successors[i];

            if (to > from) {
                size_t span = to - from;
                if (span <= 7) {
                    nfa.shift_masks.set_transition(from, to);
                } else {
                    nfa.set_exception(from);
                    nfa.add_exception_successor(from, to);
                }
            } else {
                nfa.set_exception(from);
                nfa.add_exception_successor(from, to);
            }
        }
    }

    // Step 2: Extract reachability from AST (with character class support!)
    extract_reachability_from_ast<Pattern>(nfa.reachability, 0);

    // Step 3: Mark accept states
    for (size_t i = 0; i < glushkov_nfa.accept_count; ++i) {
        nfa.set_accept(glushkov_nfa.accept_states[i]);
    }

    return nfa;
}

// Updated compile_pattern_string to use AST-based compilation
// CONSTEXPR - Runs at compile-time!
template <ctll::fixed_string Pattern>
constexpr BitNFA128 compile_pattern_string_with_charclass() {
    // Parse the pattern to get CTRE AST
    using tmp = typename ctll::parser<ctre::pcre, Pattern, ctre::pcre_actions>::template output<ctre::pcre_context<>>;
    static_assert(tmp(), "Regular Expression contains syntax error.");
    using AST = decltype(ctll::front(typename tmp::output_type::stack_type()));

    // Compile with character class support
    return compile_with_charclass<AST>();
}

} // namespace ctre::bitnfa

#endif // CTRE_BITNFA_COMPILE_FROM_AST_HPP
