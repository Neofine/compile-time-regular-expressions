#ifndef CTRE_V2__CTRE__FAST_SEARCH__HPP
#define CTRE_V2__CTRE__FAST_SEARCH__HPP

// Fast Search with Literal Prefiltering
// Optimizes search using decomposition and SIMD literal matching

#include "decomposition.hpp"
#include "wrapper.hpp"
#include "evaluation.hpp"
#include "simd_shift_or.hpp"
#include <iterator>

namespace ctre {

// =============================================================================
// Fast Search Method (similar to search_method but with prefiltering)
// =============================================================================

struct fast_search_method {
    // Helper: Expand literal chars into template parameter pack for SIMD
    template <auto Literal, size_t... Is>
    static constexpr auto make_simd_finder(std::index_sequence<Is...>) noexcept {
        return []<typename Iterator, typename EndIterator>(Iterator& it, EndIterator end) -> bool {
            // Use existing SIMD implementation!
            return simd::match_string_shift_or<Literal.chars[Is]...>(it, end, flags{});
        };
    }

    // Naive fallback for compile-time contexts or when SIMD unavailable
    template <typename Iterator, typename EndIterator, size_t LitLength>
    static constexpr bool find_literal_naive(Iterator& it, EndIterator end, const char (&literal)[LitLength]) noexcept {
        if (it == end) return false;

        const size_t len = LitLength - 1; // Exclude null terminator

        for (auto search_it = it; search_it != end; ++search_it) {
            bool match = true;
            auto check_it = search_it;
            for (size_t i = 0; i < len && check_it != end; ++i, ++check_it) {
                if (*check_it != literal[i]) {
                    match = false;
                    break;
                }
            }

            if (match && std::distance(search_it, check_it) == static_cast<std::ptrdiff_t>(len)) {
                it = search_it;
                return true;
            }
        }

        return false;
    }

    // Fast search with literal prefiltering
    template <typename Modifier = singleline, typename ResultIterator = void, typename RE, typename IteratorBegin, typename IteratorEnd>
    static constexpr CTRE_FORCE_INLINE auto exec(IteratorBegin orig_begin, IteratorBegin begin, IteratorEnd end, RE) noexcept {
        using result_iterator = std::conditional_t<std::is_same_v<ResultIterator, void>, IteratorBegin, ResultIterator>;

        // Check if we have an extractable literal
        constexpr bool has_literal = decomposition::has_prefilter_literal<RE>;

        if constexpr (has_literal) {
            constexpr auto literal = decomposition::prefilter_literal<RE>;

            if constexpr (literal.length >= 2) {
                // USE PREFILTERING with SIMD literal search! 🚀
                auto it = begin;

                // Create SIMD finder at compile-time
                constexpr auto simd_finder = make_simd_finder<literal>(std::make_index_sequence<literal.length>{});

                while (it != end) {
                    // SIMD-accelerated literal search!
                    bool found;

                    if (std::is_constant_evaluated()) {
                        // Compile-time: use naive fallback
                        char lit_array[literal.length + 1];
                        for (size_t i = 0; i < literal.length; ++i) {
                            lit_array[i] = literal.chars[i];
                        }
                        lit_array[literal.length] = '\0';
                        found = find_literal_naive(it, end, lit_array);
                    } else {
                        // Runtime: use SIMD!
                        found = simd_finder(it, end);

                        // Note: match_string_shift_or advances 'it' to END of match
                        // We need to move it back to START of match
                        if (found && literal.length > 0) {
                            it = it - literal.length;
                        }
                    }

                    if (!found) {
                        // Literal not found, no match possible
                        break;
                    }

                    // Found literal at position 'it'
                    // Try regex match starting at positions that could contain this literal
                    // TODO Phase 4 Step 5: Calculate optimal search range based on pattern structure

                    // For now: Try matching from literal position and a bit before
                    // Most patterns have literal near the end, so we search backwards
                    constexpr size_t max_lookback = 64; // Reasonable default
                    auto search_start = (it > begin + max_lookback) ? (it - max_lookback) : begin;

                    for (auto try_pos = it; try_pos >= search_start; --try_pos) {
                        if (auto out = evaluate(orig_begin, try_pos, end, Modifier{}, return_type<result_iterator, RE>{}, ctll::list<start_mark, RE, end_mark, accept>())) {
                            // Got a match!
                            return out;
                        }

                        if (try_pos == search_start) break; // Avoid underflow
                    }

                    // No match at this literal occurrence, try next
                    ++it;
                }

                // No match found
                auto out = evaluate(orig_begin, end, end, Modifier{}, return_type<result_iterator, RE>{}, ctll::list<start_mark, RE, end_mark, accept>());
                // Always track search end position (needed for split iterator)
                out.set_end_mark(end);
                return out;
            }
        }

        // NO PREFILTER or literal too short - fall back to regular search
        constexpr bool fixed = starts_with_anchor(Modifier{}, ctll::list<RE>{});

        auto it = begin;

        for (; end != it && !fixed; ++it) {
            if (auto out = evaluate(orig_begin, it, end, Modifier{}, return_type<result_iterator, RE>{}, ctll::list<start_mark, RE, end_mark, accept>())) {
                return out;
            }
        }

        auto out = evaluate(orig_begin, it, end, Modifier{}, return_type<result_iterator, RE>{}, ctll::list<start_mark, RE, end_mark, accept>());
        // Always track search end position (needed for split iterator)
        out.set_end_mark(it);
        return out;
    }

    template <typename Modifier = singleline, typename ResultIterator = void, typename RE, typename IteratorBegin, typename IteratorEnd>
    static constexpr CTRE_FORCE_INLINE auto exec(IteratorBegin begin, IteratorEnd end, RE) noexcept {
        return exec<Modifier, ResultIterator>(begin, begin, end, RE{});
    }
};

// =============================================================================
// Public API
// =============================================================================

// Template variable for fast_search
CTRE_EXPORT template <CTRE_REGEX_INPUT_TYPE input, typename... Modifiers>
constexpr auto fast_search = regular_expression<typename regex_builder<input>::type, fast_search_method, ctll::list<singleline, Modifiers...>>();

// Multiline version
CTRE_EXPORT template <CTRE_REGEX_INPUT_TYPE input, typename... Modifiers>
constexpr auto multiline_fast_search = regular_expression<typename regex_builder<input>::type, fast_search_method, ctll::list<multiline, Modifiers...>>();

} // namespace ctre

#endif // CTRE_V2__CTRE__FAST_SEARCH__HPP
