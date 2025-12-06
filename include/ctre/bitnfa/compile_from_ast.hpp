#ifndef CTRE_BITNFA_COMPILE_FROM_AST_HPP
#define CTRE_BITNFA_COMPILE_FROM_AST_HPP

#include "bitnfa_types.hpp"
#include "character_classes.hpp"
#include "../glushkov_nfa.hpp"

namespace ctre::bitnfa {

template <typename Pattern>
constexpr void extract_reachability_from_ast(ReachabilityTable& table, size_t offset) {
    size_t state = offset + 1;

    if constexpr (glushkov::is_empty<Pattern>::value) {
    } else if constexpr (glushkov::is_character<Pattern>::value) {
        []<auto C>(ctre::character<C>*, ReachabilityTable& tbl, size_t st) {
            tbl.set_reachable_mut(static_cast<char>(C), st);
        }(static_cast<Pattern*>(nullptr), table, state);
    } else if constexpr (glushkov::is_any<Pattern>::value) {
        table.set_reachable_any(state);
    } else if constexpr (requires { Pattern::template match_char<char>; }) {
        expand_any_char_class<Pattern>(table, state);
    } else if constexpr (glushkov::is_string<Pattern>::value) {
        []<auto... Cs>(ctre::string<Cs...>*, ReachabilityTable& tbl, size_t st) {
            size_t pos = st;
            ((tbl.set_reachable_mut(static_cast<char>(Cs), pos++)), ...);
        }(static_cast<Pattern*>(nullptr), table, state);
    } else if constexpr (glushkov::is_sequence<Pattern>::value) {
        []<typename... Content>(ctre::sequence<Content...>*, ReachabilityTable& tbl, size_t off) {
            size_t current_offset = off;
            ((extract_reachability_from_ast<Content>(tbl, current_offset),
              current_offset += glushkov::count_positions<Content>()), ...);
        }(static_cast<Pattern*>(nullptr), table, offset);
    } else if constexpr (glushkov::is_select<Pattern>::value) {
        []<typename... Options>(ctre::select<Options...>*, ReachabilityTable& tbl, size_t off) {
            size_t current_offset = off;
            ((extract_reachability_from_ast<Options>(tbl, current_offset),
              current_offset += glushkov::count_positions<Options>()), ...);
        }(static_cast<Pattern*>(nullptr), table, offset);
    } else if constexpr (glushkov::is_repeat<Pattern>::value ||
                          glushkov::is_lazy_repeat<Pattern>::value ||
                          glushkov::is_possessive_repeat<Pattern>::value) {
        []<size_t A, size_t B, typename... Content>(ctre::repeat<A, B, Content...>*, ReachabilityTable& tbl, size_t off) {
            (extract_reachability_from_ast<Content>(tbl, off), ...);
        }(static_cast<Pattern*>(nullptr), table, offset);
    }
}

template <typename Pattern>
constexpr BitNFA128 compile_with_charclass() {
    constexpr auto glushkov_nfa = ctre::glushkov::glushkov_nfa<Pattern>();
    BitNFA128 nfa;
    nfa.state_count = glushkov_nfa.state_count;

    for (size_t from = 0; from < glushkov_nfa.state_count; ++from) {
        const auto& state = glushkov_nfa.states[from];
        for (size_t i = 0; i < state.successor_count; ++i) {
            size_t to = state.successors[i];
            if (to > from) {
                size_t span = to - from;
                if (span <= 7) nfa.shift_masks.set_transition(from, to);
                else { nfa.set_exception(from); nfa.add_exception_successor(from, to); }
            } else {
                nfa.set_exception(from);
                nfa.add_exception_successor(from, to);
            }
        }
    }

    extract_reachability_from_ast<Pattern>(nfa.reachability, 0);

    for (size_t i = 0; i < glushkov_nfa.accept_count; ++i)
        nfa.set_accept(glushkov_nfa.accept_states[i]);

    return nfa;
}

template <ctll::fixed_string Pattern>
constexpr BitNFA128 compile_pattern_string_with_charclass() {
    using tmp = typename ctll::parser<ctre::pcre, Pattern, ctre::pcre_actions>::template output<ctre::pcre_context<>>;
    static_assert(tmp(), "Regular Expression contains syntax error.");
    using AST = decltype(ctll::front(typename tmp::output_type::stack_type()));
    return compile_with_charclass<AST>();
}

} // namespace ctre::bitnfa

#endif // CTRE_BITNFA_COMPILE_FROM_AST_HPP
