#ifndef CTRE__DECOMPOSITION__HPP
#define CTRE__DECOMPOSITION__HPP

// Pattern decomposition API

#include "dominator_analysis.hpp"
#include "glushkov_nfa.hpp"
#include "literal_extraction_simple_multi.hpp"
#include "region_analysis.hpp"

namespace ctre {
template <typename RE, typename Method, typename Modifier>
struct regular_expression;

namespace decomposition {

// Unwrap captures (CTRE adds implicit captures around parentheses)
template <typename T>
struct unwrap_capture {
    using type = T;
};
template <size_t Index, typename... Content>
struct unwrap_capture<ctre::capture<Index, Content...>> {
    using type = ctre::sequence<Content...>;
};
template <size_t Index, typename Content>
struct unwrap_capture<ctre::capture<Index, Content>> {
    using type = Content;
};
template <typename T>
using unwrap_capture_t = typename unwrap_capture<T>::type;

// Unwrap regular_expression wrapper
template <typename RE, typename Method, typename Modifier>
constexpr auto unwrap_regex_impl(const ctre::regular_expression<RE, Method, Modifier>*) -> unwrap_capture_t<RE>;
template <typename T>
constexpr auto unwrap_regex_impl(const T*) -> T;
template <typename T>
using unwrap_regex_t = decltype(unwrap_regex_impl(static_cast<T*>(nullptr)));

// Extract literal with character-set expansion and fallback
template <typename Pattern>
[[nodiscard]] inline consteval auto extract_literal_with_expansion_and_fallback() -> dominators::literal_result<64> {
    using RawAST = unwrap_regex_t<Pattern>;

    constexpr auto nfa = glushkov::glushkov_nfa<RawAST>();
    constexpr auto nfa_result = dominators::extract_literal_from_dominators(nfa);

    // Try AST-based extraction with character-set expansion
    constexpr auto multi_result = extraction::extract_literals_simple_multi<RawAST>();
    if constexpr (multi_result.has_literals && multi_result.count > 0) {
        constexpr auto longest = multi_result.get_longest();
        // Prefer longest expansion literal for SIMD prefiltering (more selective)
        dominators::literal_result<64> result{};
        result.has_literal = longest.has_literal;
        result.length = longest.length;
        result.start_position = longest.start_position;
        result.nfa_dominator_length = nfa_result.has_literal ? nfa_result.length : 0;
        for (size_t i = 0; i < longest.length; ++i)
            result.chars[i] = longest.chars[i];
        return result;
    }

    // Use NFA result if available and long enough
    if constexpr (nfa_result.has_literal && nfa_result.length >= 16) {
        dominators::literal_result<64> result = nfa_result;
        result.nfa_dominator_length = nfa_result.length;
        return result;
    }

    // Fallback to region analysis
    constexpr auto region_result = region::extract_literal_from_regions(nfa);

    if constexpr (nfa_result.has_literal && nfa_result.length < 16) {
        if constexpr (region_result.has_literal && region_result.length > nfa_result.length)
            return region_result;
        else {
            dominators::literal_result<64> result = nfa_result;
            result.nfa_dominator_length = nfa_result.length;
            return result;
        }
    }

    return region_result;
}

// Extract literal with fallback to region analysis (no expansion)
template <typename Pattern>
[[nodiscard]] inline auto extract_literal_with_fallback() {
    using RawAST = unwrap_regex_t<Pattern>;
    constexpr auto nfa = glushkov::glushkov_nfa<RawAST>();

    constexpr auto path_result = dominators::extract_literal_from_dominators(nfa);
    if constexpr (path_result.has_literal && path_result.length >= 16)
        return path_result;

    constexpr auto region_result = region::extract_literal_from_regions(nfa);
    if constexpr (path_result.has_literal && path_result.length < 16) {
        if constexpr (region_result.has_literal && region_result.length > path_result.length)
            return region_result;
        return path_result;
    }
    return region_result;
}

// Public API
template <typename Pattern>
inline constexpr bool has_prefilter_literal = dominators::has_extractable_literal<unwrap_regex_t<Pattern>>();

template <typename Pattern>
inline constexpr auto prefilter_literal = dominators::extract_literal<unwrap_regex_t<Pattern>>();

template <typename Pattern>
inline auto prefilter_literal_with_fallback = extract_literal_with_fallback<unwrap_regex_t<Pattern>>();

template <typename Pattern>
inline constexpr auto glushkov_nfa = glushkov::glushkov_nfa<Pattern>();

// Compile-time literal string type
template <char... Cs>
struct literal_string {
    static constexpr size_t length = sizeof...(Cs);
    static constexpr char value[length + 1] = {Cs..., '\0'};
    static constexpr std::array<char, length> chars = {Cs...};
    [[nodiscard]] constexpr operator std::string_view() const noexcept {
        return {value, length};
    }
};

template <typename Pattern, auto Literal = prefilter_literal<Pattern>>
struct literal_for_pattern {
    static constexpr bool has_literal = Literal.has_literal;
    static constexpr size_t length = Literal.length;
    template <size_t... Is>
    static constexpr auto make_string(std::index_sequence<Is...>) {
        return literal_string<Literal.chars[Is]...>{};
    }
    using type = decltype(make_string(std::make_index_sequence<length>{}));
};

template <typename Pattern>
using literal_string_for = typename literal_for_pattern<Pattern>::type;

// Pattern statistics
template <typename Pattern>
struct pattern_stats {
    static constexpr size_t position_count = glushkov::count_positions<Pattern>();
    static constexpr bool is_nullable = glushkov::nullable<Pattern>();
    static constexpr bool has_literal = has_prefilter_literal<Pattern>;
    static constexpr size_t literal_length = prefilter_literal<Pattern>.length;
};

} // namespace decomposition
} // namespace ctre

#endif // CTRE__DECOMPOSITION__HPP
