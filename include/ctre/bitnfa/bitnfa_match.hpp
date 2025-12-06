#ifndef CTRE_BITNFA_MATCH_HPP
#define CTRE_BITNFA_MATCH_HPP

#include "bitnfa_types.hpp"
#include <string_view>
#include <vector>
#include <optional>

#ifndef CTRE_V2__CTRE__HPP
#include "../../ctre.hpp"
#endif

#include "compile_from_ast.hpp"
#include "pattern_traits.hpp"
#include "simd_acceleration.hpp"
#include "specialized_matchers.hpp"

namespace ctre::bitnfa {

struct match_result {
    size_t position = 0;
    size_t length = 0;
    bool matched = false;

    explicit operator bool() const { return matched; }

    std::string_view to_view(std::string_view input) const {
        if (!matched) return {};
        return input.substr(position, length);
    }
};

[[gnu::always_inline]] inline match_result match(const BitNFA128& nfa, std::string_view input) {
    StateMask128 current = nfa.get_initial_state();

    size_t pos = 0;
    for (char c : input) {
        current = nfa.calculate_successors(current, c);
        if (current.none()) return match_result{0, 0, false};
        ++pos;
    }

    if (nfa.has_accept(current)) {
        return match_result{0, input.size(), true};
    }

    return match_result{0, 0, false};
}

template <typename AST>
inline match_result match_from_ast(std::string_view input) {
    static constexpr auto nfa = compile_with_charclass<AST>();
    return match(nfa, input);
}

template <typename AST>
inline match_result search_from_ast(std::string_view input) {
    static constexpr auto nfa = compile_with_charclass<AST>();
    return search(nfa, input);
}

template <ctll::fixed_string Pattern>
inline match_result match(std::string_view input) {
    using tmp = typename ctll::parser<::ctre::pcre, Pattern, ::ctre::pcre_actions>::template output<::ctre::pcre_context<>>;
    static_assert(tmp(), "Regular Expression contains syntax error.");
    using AST = decltype(ctll::front(typename tmp::output_type::stack_type()));
    return match_from_ast<AST>(input);
}

inline match_result search(const BitNFA128& nfa, std::string_view input) {
    for (size_t start = 0; start < input.size(); ++start) {
        StateMask128 current = nfa.get_initial_state();
        std::optional<size_t> match_end;

        for (size_t pos = start; pos < input.size(); ++pos) {
            current = nfa.calculate_successors(current, input[pos]);
            if (current.none()) break;
            if (nfa.has_accept(current)) match_end = pos;
        }

        if (match_end.has_value()) {
            return match_result{start, match_end.value() - start + 1, true};
        }
    }

    return match_result{0, 0, false};
}

template <ctll::fixed_string Pattern>
inline match_result search(std::string_view input) {
    using tmp = typename ctll::parser<::ctre::pcre, Pattern, ::ctre::pcre_actions>::template output<::ctre::pcre_context<>>;
    static_assert(tmp(), "Regular Expression contains syntax error.");
    using AST = decltype(ctll::front(typename tmp::output_type::stack_type()));

    if constexpr (should_use_fast_path<AST>::value) {
        if constexpr (is_string<AST>::value || is_character<AST>::value) {
            return fast_search(input, static_cast<AST*>(nullptr), static_cast<match_result*>(nullptr));
        }
    }

    if constexpr (can_accelerate<AST>::value) {
        using ContentType = typename extract_repeat_content<AST>::type;
        const char* data = input.data();
        const char* end_ptr = data + input.size();

        for (const char* start = data; start < end_ptr; ) {
            const char* match_start = simd_find_char_class<ContentType>(start, end_ptr);
            if (match_start >= end_ptr) break;

            const char* match_end = simd_find_char_class_end<ContentType>(match_start, end_ptr);
            size_t pos = match_start - data;
            size_t length = match_end - match_start;

            if (length > 0) return match_result{pos, length, true};
            start = match_start + 1;
        }
        return match_result{0, 0, false};
    } else {
        static constexpr auto nfa = compile_pattern_string_with_charclass<Pattern>();
        return search(nfa, input);
    }
}

inline std::vector<match_result> find_all(const BitNFA128& nfa, std::string_view input) {
    std::vector<match_result> results;
    size_t start = 0;

    while (start < input.size()) {
        StateMask128 current = nfa.get_initial_state();
        std::optional<size_t> match_end;

        for (size_t pos = start; pos < input.size(); ++pos) {
            current = nfa.calculate_successors(current, input[pos]);
            if (current.none()) break;
            if (nfa.has_accept(current)) match_end = pos;
        }

        if (match_end.has_value()) {
            results.push_back(match_result{start, match_end.value() - start + 1, true});
            start = match_end.value() + 1;
        } else {
            ++start;
        }
    }
    return results;
}

template <ctll::fixed_string Pattern>
inline std::vector<match_result> find_all(std::string_view input) {
    static constexpr auto nfa = compile_pattern_string_with_charclass<Pattern>();
    return find_all(nfa, input);
}

} // namespace ctre::bitnfa

#endif // CTRE_BITNFA_MATCH_HPP
