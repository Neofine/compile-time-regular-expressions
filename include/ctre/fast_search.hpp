#ifndef CTRE__FAST_SEARCH__HPP
#define CTRE__FAST_SEARCH__HPP

#include "wrapper.hpp"
#include "evaluation.hpp"
#ifndef CTRE_DISABLE_SIMD
#include "decomposition.hpp"
#include "simd_shift_or.hpp"
#endif
#include <iterator>

namespace ctre {

#ifndef CTRE_DISABLE_SIMD
struct fast_search_method {
    template <auto Literal, size_t... Is>
    [[nodiscard]] static constexpr auto make_simd_finder(std::index_sequence<Is...>) noexcept {
        return []<typename Iterator, typename EndIterator>(Iterator& it, EndIterator end) -> bool {
            return simd::match_string_shift_or<Literal.chars[Is]...>(it, end, flags{});
        };
    }

    template <typename Iterator, typename EndIterator, size_t LitLength>
    [[nodiscard]] static constexpr bool find_literal_naive(Iterator& it, EndIterator end,
                                                            const char (&literal)[LitLength]) noexcept {
        if (it == end) return false;
        const size_t len = LitLength - 1;

        for (auto search_it = it; search_it != end; ++search_it) {
            bool match = true;
            auto check_it = search_it;
            for (size_t i = 0; i < len && check_it != end; ++i, ++check_it) {
                if (*check_it != literal[i]) { match = false; break; }
            }
            if (match && std::distance(search_it, check_it) == static_cast<std::ptrdiff_t>(len)) {
                it = search_it;
                return true;
            }
        }
        return false;
    }

    template <typename Modifier = singleline, typename ResultIterator = void, typename RE,
              typename IteratorBegin, typename IteratorEnd>
    [[nodiscard]] static constexpr CTRE_FORCE_INLINE auto exec(IteratorBegin orig_begin, IteratorBegin begin,
                                                                IteratorEnd end, RE) noexcept {
        using result_iterator = std::conditional_t<std::is_same_v<ResultIterator, void>, IteratorBegin, ResultIterator>;
        constexpr bool has_literal = decomposition::has_prefilter_literal<RE>;

        if constexpr (has_literal) {
            constexpr auto literal = decomposition::prefilter_literal<RE>;

            if constexpr (literal.length >= 2) {
                auto it = begin;
                constexpr auto simd_finder = make_simd_finder<literal>(std::make_index_sequence<literal.length>{});

                while (it != end) {
                    bool found;
                    if (std::is_constant_evaluated()) {
                        char lit_array[literal.length + 1];
                        for (size_t i = 0; i < literal.length; ++i) lit_array[i] = literal.chars[i];
                        lit_array[literal.length] = '\0';
                        found = find_literal_naive(it, end, lit_array);
                    } else {
                        found = simd_finder(it, end);
                        if (found && literal.length > 0) it = it - literal.length;
                    }

                    if (!found) break;

                    constexpr size_t max_lookback = 64;
                    auto search_start = (it > begin + max_lookback) ? (it - max_lookback) : begin;

                    for (auto try_pos = it; try_pos >= search_start; --try_pos) {
                        if (auto out = evaluate(orig_begin, try_pos, end, Modifier{},
                                                return_type<result_iterator, RE>{},
                                                ctll::list<start_mark, RE, end_mark, accept>())) {
                            return out;
                        }
                        if (try_pos == search_start) break;
                    }
                    ++it;
                }

                auto out = evaluate(orig_begin, end, end, Modifier{}, return_type<result_iterator, RE>{},
                                    ctll::list<start_mark, RE, end_mark, accept>());
                out.set_end_mark(end);
                return out;
            }
        }

        constexpr bool fixed = starts_with_anchor(Modifier{}, ctll::list<RE>{});
        auto it = begin;

        for (; end != it && !fixed; ++it) {
            if (auto out = evaluate(orig_begin, it, end, Modifier{}, return_type<result_iterator, RE>{},
                                    ctll::list<start_mark, RE, end_mark, accept>())) {
                return out;
            }
        }

        auto out = evaluate(orig_begin, it, end, Modifier{}, return_type<result_iterator, RE>{},
                            ctll::list<start_mark, RE, end_mark, accept>());
        out.set_end_mark(it);
        return out;
    }

    template <typename Modifier = singleline, typename ResultIterator = void, typename RE,
              typename IteratorBegin, typename IteratorEnd>
    [[nodiscard]] static constexpr CTRE_FORCE_INLINE auto exec(IteratorBegin begin, IteratorEnd end, RE) noexcept {
        return exec<Modifier, ResultIterator>(begin, begin, end, RE{});
    }
};

CTRE_EXPORT template <CTRE_REGEX_INPUT_TYPE input, typename... Modifiers>
constexpr auto fast_search = regular_expression<typename regex_builder<input>::type,
                                                 fast_search_method, ctll::list<singleline, Modifiers...>>();

CTRE_EXPORT template <CTRE_REGEX_INPUT_TYPE input, typename... Modifiers>
constexpr auto multiline_fast_search = regular_expression<typename regex_builder<input>::type,
                                                           fast_search_method, ctll::list<multiline, Modifiers...>>();
#endif // CTRE_DISABLE_SIMD

} // namespace ctre

#endif // CTRE__FAST_SEARCH__HPP
