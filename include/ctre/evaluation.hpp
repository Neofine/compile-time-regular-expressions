#ifndef CTRE__EVALUATION__HPP
#define CTRE__EVALUATION__HPP

#include "atoms.hpp"
#include "atoms_unicode.hpp"
#include "find_captures.hpp"
#include "first.hpp"
#include "flags_and_modes.hpp"
#include "return_type.hpp"
#include "simd_character_classes.hpp"
#include "simd_detection.hpp"
#include "simd_multirange.hpp"
#include "simd_pattern_analysis.hpp"
#include "simd_repetition.hpp"
#include "simd_sequence_fusion.hpp"
#include "simd_shufti.hpp"
#include "simd_single_char.hpp"
#include "simd_string_matching.hpp"
#include "starts_with_anchor.hpp"
#include "utility.hpp"
#include <cstdio>
#ifndef CTRE_IN_A_MODULE
#include <iterator>
#endif

// remove me when MSVC fix the constexpr bug
#ifdef _MSC_VER
#ifndef CTRE_MSVC_GREEDY_WORKAROUND
#define CTRE_MSVC_GREEDY_WORKAROUND
#endif
#endif

namespace ctre {

template <size_t Limit>
constexpr CTRE_FORCE_INLINE bool less_than_or_infinite([[maybe_unused]] size_t i) noexcept {
    if constexpr (Limit == 0) {
        // infinite
        return true;
    } else {
        return i < Limit;
    }
}

template <size_t Limit>
constexpr CTRE_FORCE_INLINE bool less_than([[maybe_unused]] size_t i) noexcept {
    if constexpr (Limit == 0) {
        // infinite
        return false;
    } else {
        return i < Limit;
    }
}

constexpr bool is_bidirectional(const std::bidirectional_iterator_tag&) {
    return true;
}
constexpr bool is_bidirectional(...) {
    return false;
}

// sink for making the errors shorter
template <typename R, typename BeginIterator, typename Iterator, typename EndIterator>
constexpr CTRE_FORCE_INLINE R evaluate(const BeginIterator, Iterator, const EndIterator, flags, R, ...) noexcept {
    return not_matched;
}

// if we found "accept" object on stack => ACCEPT
template <typename R, typename BeginIterator, typename Iterator, typename EndIterator>
constexpr CTRE_FORCE_INLINE R evaluate(const BeginIterator, Iterator, const EndIterator, flags, R captures,
                                       ctll::list<accept>) noexcept {
    return captures.matched();
}

// if we found "reject" object on stack => REJECT
template <typename R, typename... Rest, typename BeginIterator, typename Iterator, typename EndIterator>
constexpr CTRE_FORCE_INLINE R evaluate(const BeginIterator, Iterator, const EndIterator, flags, R,
                                       ctll::list<reject, Rest...>) noexcept {
    return not_matched;
}

// mark start of outer capture
template <typename R, typename BeginIterator, typename Iterator, typename EndIterator, typename... Tail>
constexpr CTRE_FORCE_INLINE R evaluate(const BeginIterator begin, Iterator current, const EndIterator last,
                                       const flags& f, R captures, ctll::list<start_mark, Tail...>) noexcept {
    return evaluate(begin, current, last, f, captures.set_start_mark(current), ctll::list<Tail...>());
}

// mark end of outer capture
template <typename R, typename BeginIterator, typename Iterator, typename EndIterator, typename... Tail>
constexpr CTRE_FORCE_INLINE R evaluate(const BeginIterator begin, Iterator current, const EndIterator last,
                                       const flags& f, R captures, ctll::list<end_mark, Tail...>) noexcept {
    return evaluate(begin, current, last, f, captures.set_end_mark(current), ctll::list<Tail...>());
}

// mark end of cycle
template <typename R, typename BeginIterator, typename Iterator, typename EndIterator, typename... Tail>
constexpr CTRE_FORCE_INLINE R evaluate(const BeginIterator, Iterator current, const EndIterator,
                                       [[maybe_unused]] const flags& f, R captures,
                                       ctll::list<end_cycle_mark>) noexcept {
    if (cannot_be_empty_match(f)) {
        return not_matched;
    }

    return captures.set_end_mark(current).matched();
}

// matching everything which behave as a one character matcher

template <
    typename R, typename BeginIterator, typename Iterator, typename EndIterator, typename CharacterLike,
    typename... Tail,
    typename = std::enable_if_t<(MatchesCharacter<CharacterLike>::template value<decltype(*std::declval<Iterator>())>)>>
constexpr CTRE_FORCE_INLINE R evaluate(const BeginIterator begin, Iterator current, const EndIterator last,
                                       const flags& f, R captures, ctll::list<CharacterLike, Tail...>) noexcept {

    if (current == last)
        return not_matched;
    if (!CharacterLike::match_char(*current, f))
        return not_matched;

    return evaluate(begin, ++current, last, consumed_something(f), captures, ctll::list<Tail...>());
}

template <typename R, typename BeginIterator, typename Iterator, typename EndIterator, typename... Tail>
constexpr CTRE_FORCE_INLINE R evaluate(const BeginIterator begin, Iterator current, const EndIterator last,
                                       const flags& f, R captures, ctll::list<any, Tail...>) noexcept {
    if (current == last)
        return not_matched;

    if (multiline_mode(f)) {
        // TODO add support for different line ending and unicode (in a future unicode mode)
        if (*current == '\n')
            return not_matched;
    }
    return evaluate(begin, ++current, last, consumed_something(f), captures, ctll::list<Tail...>());
}

// matching strings in patterns
template <auto... String, typename Iterator, typename EndIterator>
constexpr CTRE_FORCE_INLINE bool match_string([[maybe_unused]] Iterator& current,
                                              [[maybe_unused]] const EndIterator last,
                                              [[maybe_unused]] const flags& f) {
    [[maybe_unused]] constexpr size_t string_length = sizeof...(String);

    // SIMD optimization for long strings at runtime (invisible to user)
    if constexpr (string_length >= simd::SIMD_STRING_THRESHOLD) {
        if (!std::is_constant_evaluated() && simd::can_use_simd()) {
            // Use SIMD-optimized string matching
            return simd::match_string_simd<String...>(current, last, f);
        }
    }

    // Original implementation for compile-time or short strings
    return ((current != last && character<String>::match_char(*current++, f)) && ... && true);
}

template <typename R, typename BeginIterator, typename Iterator, typename EndIterator, auto... String, typename... Tail>
constexpr CTRE_FORCE_INLINE R evaluate(const BeginIterator begin, Iterator current, const EndIterator last,
                                       [[maybe_unused]] const flags& f, R captures,
                                       ctll::list<string<String...>, Tail...>) noexcept {
    if (!match_string<String...>(current, last, f)) {
        return not_matched;
    }

    return evaluate(begin, current, last, consumed_something(f, sizeof...(String) > 0), captures,
                    ctll::list<Tail...>());
}

// ============================================================================
// ALTERNATION OPTIMIZATION: select<character<C1>, character<C2>, ...>
// Convert recursive select evaluation to simple membership check
// ============================================================================

// Trait to detect single-character alternations
template <typename T>
struct is_char_alternation : std::false_type {};

template <auto... Cs>
struct is_char_alternation<select<character<Cs>...>> : std::true_type {
    // Fast membership check - equivalent to set<character<Cs>...>::match_char
    template <typename CharT>
    static constexpr CTRE_FORCE_INLINE bool match_char(CharT c, const flags& f) noexcept {
        return (character<Cs>::match_char(c, f) || ...);
    }
};

// Handle captured alternations: (a|b) -> capture<Index, select<character<...>>>
template <size_t Index, auto... Cs>
struct is_char_alternation<capture<Index, select<character<Cs>...>>> : std::true_type {
    template <typename CharT>
    static constexpr CTRE_FORCE_INLINE bool match_char(CharT c, const flags& f) noexcept {
        return (character<Cs>::match_char(c, f) || ...);
    }
};

// matching select in patterns
template <typename R, typename BeginIterator, typename Iterator, typename EndIterator, typename HeadOptions,
          typename... TailOptions, typename... Tail>
constexpr CTRE_FORCE_INLINE R evaluate(const BeginIterator begin, Iterator current, const EndIterator last,
                                       const flags& f, R captures,
                                       ctll::list<select<HeadOptions, TailOptions...>, Tail...>) noexcept {
    if (auto r = evaluate(begin, current, last, f, captures, ctll::list<HeadOptions, Tail...>())) {
        return r;
    } else {
        return evaluate(begin, current, last, f, captures, ctll::list<select<TailOptions...>, Tail...>());
    }
}

template <typename R, typename BeginIterator, typename Iterator, typename EndIterator, typename... Tail>
constexpr CTRE_FORCE_INLINE R evaluate(const BeginIterator, Iterator, const EndIterator, flags, R,
                                       ctll::list<select<>, Tail...>) noexcept {
    // no previous option was matched => REJECT
    return not_matched;
}

// matching sequence in patterns
template <typename R, typename BeginIterator, typename Iterator, typename EndIterator, typename HeadContent,
          typename... TailContent, typename... Tail>
constexpr CTRE_FORCE_INLINE R evaluate(const BeginIterator begin, Iterator current, const EndIterator last,
                                       const flags& f, R captures,
                                       ctll::list<sequence<HeadContent, TailContent...>, Tail...>) noexcept {

    // SIMD SEQUENCE FUSION: Generic fast path for bounded sequences
    // Generates SIMD code automatically for ANY bounded sequence pattern
    using SequenceType = sequence<HeadContent, TailContent...>;

    if constexpr (simd::can_use_simd() && std::is_pointer_v<Iterator>) {
        if (!std::is_constant_evaluated()) {
            // Try generic SIMD fusion (works for IPv4, MAC, UUID, phone, date, etc.)
            // Pass nullptr as type tag for template dispatch
            Iterator fusion_result = simd::match_sequence_fused(static_cast<SequenceType*>(nullptr), current, last);
            if (fusion_result != current) {
                return evaluate(begin, fusion_result, last, f, captures, ctll::list<Tail...>());
            }
            // No match - fall through to normal evaluation
        }
    }

    // Normal sequence decomposition
    if constexpr (sizeof...(TailContent) > 0) {
        return evaluate(begin, current, last, f, captures,
                        ctll::list<HeadContent, sequence<TailContent...>, Tail...>());
    } else {
        return evaluate(begin, current, last, f, captures, ctll::list<HeadContent, Tail...>());
    }
}

// matching empty in patterns
template <typename R, typename BeginIterator, typename Iterator, typename EndIterator, typename... Tail>
constexpr CTRE_FORCE_INLINE R evaluate(const BeginIterator begin, Iterator current, const EndIterator last,
                                       const flags& f, R captures, ctll::list<empty, Tail...>) noexcept {
    return evaluate(begin, current, last, f, captures, ctll::list<Tail...>());
}

// matching asserts
template <typename R, typename BeginIterator, typename Iterator, typename EndIterator, typename... Tail>
constexpr CTRE_FORCE_INLINE R evaluate(const BeginIterator begin, Iterator current, const EndIterator last,
                                       const flags& f, R captures, ctll::list<assert_subject_begin, Tail...>) noexcept {
    if (begin != current) {
        return not_matched;
    }
    return evaluate(begin, current, last, f, captures, ctll::list<Tail...>());
}

template <typename R, typename BeginIterator, typename Iterator, typename EndIterator, typename... Tail>
constexpr CTRE_FORCE_INLINE R evaluate(const BeginIterator begin, Iterator current, const EndIterator last,
                                       const flags& f, R captures, ctll::list<assert_subject_end, Tail...>) noexcept {
    if (last != current) {
        return not_matched;
    }
    return evaluate(begin, current, last, f, captures, ctll::list<Tail...>());
}

template <typename R, typename BeginIterator, typename Iterator, typename EndIterator, typename... Tail>
constexpr CTRE_FORCE_INLINE R evaluate(const BeginIterator begin, Iterator current, const EndIterator last,
                                       const flags& f, R captures,
                                       ctll::list<assert_subject_end_line, Tail...>) noexcept {
    if (multiline_mode(f)) {
        if (last == current) {
            return evaluate(begin, current, last, f, captures, ctll::list<Tail...>());
        } else if (*current == '\n' && std::next(current) == last) {
            return evaluate(begin, current, last, f, captures, ctll::list<Tail...>());
        } else {
            return not_matched;
        }
    } else {
        if (last != current) {
            return not_matched;
        }
        return evaluate(begin, current, last, f, captures, ctll::list<Tail...>());
    }
}

template <typename R, typename BeginIterator, typename Iterator, typename EndIterator, typename... Tail>
constexpr CTRE_FORCE_INLINE R evaluate(const BeginIterator begin, Iterator current, const EndIterator last,
                                       const flags& f, R captures, ctll::list<assert_line_begin, Tail...>) noexcept {
    if (multiline_mode(f)) {
        if (begin == current) {
            return evaluate(begin, current, last, f, captures, ctll::list<Tail...>());
        } else if (*std::prev(current) == '\n') {
            return evaluate(begin, current, last, f, captures, ctll::list<Tail...>());
        } else {
            return not_matched;
        }
    } else {
        if (begin != current) {
            return not_matched;
        }
        return evaluate(begin, current, last, f, captures, ctll::list<Tail...>());
    }
}

template <typename R, typename BeginIterator, typename Iterator, typename EndIterator, typename... Tail>
constexpr CTRE_FORCE_INLINE R evaluate(const BeginIterator begin, Iterator current, const EndIterator last,
                                       const flags& f, R captures, ctll::list<assert_line_end, Tail...>) noexcept {
    if (multiline_mode(f)) {
        if (last == current) {
            return evaluate(begin, current, last, f, captures, ctll::list<Tail...>());
        } else if (*current == '\n') {
            return evaluate(begin, current, last, f, captures, ctll::list<Tail...>());
        } else {
            return not_matched;
        }
    }

    // TODO properly match line end
    if (last != current) {
        return not_matched;
    }
    return evaluate(begin, current, last, f, captures, ctll::list<Tail...>());
}

// matching boundary
template <typename R, typename BeginIterator, typename Iterator, typename EndIterator, typename CharacterLike,
          typename... Tail>
constexpr CTRE_FORCE_INLINE R evaluate(const BeginIterator begin, Iterator current, const EndIterator last,
                                       const flags& f, R captures,
                                       ctll::list<boundary<CharacterLike>, Tail...>) noexcept {

    // reason why I need bidirectional iterators or some clever hack
    bool before = false;
    bool after = false;

    static_assert(is_bidirectional(typename std::iterator_traits<Iterator>::iterator_category{}),
                  "To use boundary in regex you need to provide bidirectional iterator or range.");

    if (last != current) {
        after = CharacterLike::match_char(*current, f);
    }
    if (begin != current) {
        before = CharacterLike::match_char(*std::prev(current), f);
    }

    if (before == after)
        return not_matched;

    return evaluate(begin, current, last, f, captures, ctll::list<Tail...>());
}

// matching not_boundary
template <typename R, typename BeginIterator, typename Iterator, typename EndIterator, typename CharacterLike,
          typename... Tail>
constexpr CTRE_FORCE_INLINE R evaluate(const BeginIterator begin, Iterator current, const EndIterator last,
                                       const flags& f, R captures,
                                       ctll::list<not_boundary<CharacterLike>, Tail...>) noexcept {

    // reason why I need bidirectional iterators or some clever hack
    bool before = false;
    bool after = false;

    static_assert(is_bidirectional(typename std::iterator_traits<Iterator>::iterator_category{}),
                  "To use boundary in regex you need to provide bidirectional iterator or range.");

    if (last != current) {
        after = CharacterLike::match_char(*current, f);
    }
    if (begin != current) {
        before = CharacterLike::match_char(*std::prev(current), f);
    }

    if (before != after)
        return not_matched;

    return evaluate(begin, current, last, f, captures, ctll::list<Tail...>());
}

// lazy repeat
template <typename R, typename BeginIterator, typename Iterator, typename EndIterator, size_t A, size_t B,
          typename... Content, typename... Tail>
constexpr CTRE_FORCE_INLINE R evaluate(const BeginIterator begin, Iterator current, const EndIterator last,
                                       [[maybe_unused]] const flags& f, R captures,
                                       ctll::list<lazy_repeat<A, B, Content...>, Tail...>) noexcept {
    // Debug: lazy_repeat function called

    if constexpr (B != 0 && A > B) {
        return not_matched;
    } else {
        const Iterator backup_current = current;

        size_t i{0};

        while (less_than<A>(i)) {
            auto outer_result =
                evaluate(begin, current, last, not_empty_match(f), captures, ctll::list<Content..., end_cycle_mark>());

            if (!outer_result)
                return not_matched;

            captures = outer_result.unmatch();
            current = outer_result.get_end_position();

            ++i;
        }

        if (auto outer_result = evaluate(begin, current, last, consumed_something(f, backup_current != current),
                                         captures, ctll::list<Tail...>())) {
            return outer_result;
        }

        while (less_than_or_infinite<B>(i)) {
            auto inner_result =
                evaluate(begin, current, last, not_empty_match(f), captures, ctll::list<Content..., end_cycle_mark>());

            if (!inner_result)
                return not_matched;

            auto outer_result = evaluate(begin, inner_result.get_end_position(), last, consumed_something(f),
                                         inner_result.unmatch(), ctll::list<Tail...>());

            if (outer_result) {
                return outer_result;
            }

            captures = inner_result.unmatch();
            current = inner_result.get_end_position();

            ++i;
        }

        // rest of regex
        return evaluate(begin, current, last, consumed_something(f), captures, ctll::list<Tail...>());
    }
}

// possessive repeat
template <typename R, typename BeginIterator, typename Iterator, typename EndIterator, size_t A, size_t B,
          typename... Content, typename... Tail>
constexpr CTRE_FORCE_INLINE R evaluate(const BeginIterator begin, Iterator current, const EndIterator last,
                                       [[maybe_unused]] const flags& f, R captures,
                                       ctll::list<possessive_repeat<A, B, Content...>, Tail...>) noexcept {

    if constexpr ((B != 0) && (A > B)) {
        return not_matched;
    }

    // FAST PATH: Character alternations like (a|b|c)+
    // Use scalar only when SIMD is not available or at compile time
    // At runtime with SIMD, the Shufti path below handles this ~10x faster
    if constexpr (sizeof...(Content) == 1) {
        using ContentType = std::tuple_element_t<0, std::tuple<Content...>>;
        if constexpr (is_char_alternation<ContentType>::value) {
            if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<decltype(*std::declval<Iterator>())>>, char>) {
                // Skip scalar path if SIMD is available - let Shufti handle it
                if constexpr (!simd::can_use_simd()) {
                    const auto backup_current = current;
                    size_t count = 0;

                    while (current != last && less_than_or_infinite<B>(count)) {
                        if (!is_char_alternation<ContentType>::match_char(*current, f)) {
                            break;
                        }
                        ++current;
                        ++count;
                    }

                    if (count >= A) {
                        return evaluate(begin, current, last, consumed_something(f, backup_current != current), captures,
                                        ctll::list<Tail...>());
                    }
                    return not_matched;
                }
                // When SIMD available: fall through to SIMD Shufti path below
            }
        }
    }

    // EARLY REJECTION: Check first char BEFORE any SIMD setup (outside SIMD block!)
    // For non-matching inputs (e.g., [0-9]+ on letters), reject in ~1ns vs ~15ns
    // This is the PREFILTER stage described in thesis - fast-fail before expensive evaluation
    if constexpr (sizeof...(Content) == 1) {
        using ContentType = std::tuple_element_t<0, std::tuple<Content...>>;
        if constexpr (requires { simd::simd_pattern_trait<ContentType>::min_char; }) {
            if (current != last) {
                constexpr char min_c = simd::simd_pattern_trait<ContentType>::min_char;
                constexpr char max_c = simd::simd_pattern_trait<ContentType>::max_char;
                unsigned char first = static_cast<unsigned char>(*current);
                if (first < static_cast<unsigned char>(min_c) || first > static_cast<unsigned char>(max_c)) {
                    if constexpr (A == 0) {
                        return evaluate(begin, current, last, f, captures, ctll::list<Tail...>());
                    } else {
                        return not_matched;
                    }
                }
            }
        }
    }

    // SIMD optimization for possessive repetition patterns at runtime (invisible to user)
    // NOTE: This SIMD decision logic is duplicated in both possessive_repeat and repeat
    // for performance. Extracting to helper causes 70% regression (1.82x → 1.07x) due to
    // template instantiation preventing compile-time optimization. If modifying, update BOTH!
    if constexpr (sizeof...(Content) == 1) {
        if (!std::is_constant_evaluated() && simd::can_use_simd()) {
            using ContentType = std::tuple_element_t<0, std::tuple<Content...>>;

            // Only use SIMD for char iterators (not wchar_t, char16_t, char32_t, etc.)
            if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<decltype(*std::declval<Iterator>())>>,
                                         char>) {

                // PERF: Special case for 'any' (wildcard .) - ultra-fast advance!
                // FIX: Only use when nothing follows (Tail empty), otherwise .*x patterns fail
                if constexpr (std::is_same_v<ContentType, any> && sizeof...(Tail) == 0) {
                    size_t count = 0;
                    Iterator start = current;

                    if (multiline_mode(f)) {
                        // In multiline: skip all non-\n characters
                        while (current != last && (B == 0 || count < B)) {
                            if (*current == '\n')
                                break;
                            ++current;
                            ++count;
                        }
                    } else {
                        // In singleline: skip ANY characters (fastest path!)
                        if constexpr (std::is_pointer_v<Iterator>) {
                            size_t remaining = last - current;
                            size_t to_advance = (B == 0) ? remaining : std::min(remaining, B);
                            current += to_advance;
                            count = to_advance;
                        } else {
                            while (current != last && (B == 0 || count < B)) {
                                ++current;
                                ++count;
                            }
                        }
                    }

                    if (count >= A) {
                        return evaluate(begin, current, last, consumed_something(f), captures, ctll::list<Tail...>());
                    } else {
                        return not_matched;
                    }
                }

                // SIMD optimization: Compile-time suitability check
                // Skip for: 1) Small bounded reps, 2) Sequence patterns (literal after repeat)
                //           3) Simple single-range patterns (to avoid code bloat overhead)
                //           4) Sentinel iterators (don't support pointer arithmetic)
                // This prevents binary bloat on patterns like IPv4: [0-9]+\.[0-9]+\.
                constexpr bool has_literal_after = simd::has_literal_next<Tail...>();

                // CRITICAL FIX: Only use SIMD with contiguous iterators that support subtraction
                // C++20: std::contiguous_iterator includes std::string::iterator and pointers
                constexpr bool can_use_pointer_arithmetic =
                    std::contiguous_iterator<Iterator> &&
                    (std::is_pointer_v<EndIterator> || std::contiguous_iterator<EndIterator>);

                if constexpr (simd::can_use_simd() && (B == 0 || B >= simd::SIMD_REPETITION_THRESHOLD) &&
                              !has_literal_after && can_use_pointer_arithmetic) {
                    // EARLY REJECTION: Check first char before SIMD setup
                    // For non-matching inputs, this avoids ~15ns SIMD overhead
                    if constexpr (requires { simd::simd_pattern_trait<ContentType>::min_char; }) {
                        if (current != last) {
                            constexpr char min_c = simd::simd_pattern_trait<ContentType>::min_char;
                            constexpr char max_c = simd::simd_pattern_trait<ContentType>::max_char;
                            char first = *current;
                            if (first < min_c || first > max_c) {
                                // First char doesn't match - reject without SIMD
                                if constexpr (A == 0) {
                                    return evaluate(begin, current, last, f, captures, ctll::list<Tail...>());
                                } else {
                                    return not_matched;
                                }
                            }
                        }
                    }

                    // Try multi-range SIMD first (handles any number of ranges: 2, 3, 4, ... N)
                    // Examples: [a-zA-Z], [0-9a-fA-F], [a-eA-Eg-kG-K], etc.
                    const auto remaining_input = last - current;
                    if constexpr (simd::is_multi_range<ContentType>::is_valid) {
                        if (remaining_input >= simd::SIMD_REPETITION_THRESHOLD) {
                            Iterator multirange_result =
                                simd::match_multirange_repeat<ContentType, A, B>(current, last, f);
                            if (multirange_result != current) {
                                return evaluate(begin, multirange_result, last, f, captures, ctll::list<Tail...>());
                            }
                        }
                    }

                    // Try Shufti for sparse character sets (like [aeiou])
                    if constexpr (simd::shufti_pattern_trait<ContentType>::should_use_shufti) {
                        if (remaining_input >= simd::SIMD_SHUFTI_THRESHOLD) {
                            Iterator shufti_result =
                                simd::match_pattern_repeat_shufti<ContentType, A, B>(current, last, f);
                            if (shufti_result != current) {
                                return evaluate(begin, shufti_result, last, f, captures, ctll::list<Tail...>());
                            }
                        }
                    }

                    // Check range size - balance SIMD overhead vs scalar simplicity
                    if constexpr (requires {
                                      simd::simd_pattern_trait<ContentType>::min_char;
                                      simd::simd_pattern_trait<ContentType>::max_char;
                                  }) {
                        constexpr char min_char = simd::simd_pattern_trait<ContentType>::min_char;
                        constexpr char max_char = simd::simd_pattern_trait<ContentType>::max_char;
                        // FIX: Cast to unsigned char to avoid signed overflow in range calculation
                        constexpr size_t range_size =
                            static_cast<unsigned char>(max_char) - static_cast<unsigned char>(min_char) + 1;
                        constexpr size_t char_count = count_char_class_size(static_cast<ContentType*>(nullptr));

                        // Gap detection: skip SIMD if char_count < range_size (has gaps)
                        // Note: Multi-range patterns (handled above) won't reach here
                        constexpr bool has_gaps = (char_count > 0) && (char_count < range_size);

                        // Check if this is a negated range (always use SIMD for negated!)
                        constexpr bool is_negated = [] {
                            if constexpr (requires { simd::simd_pattern_trait<ContentType>::is_negated; }) {
                                return simd::simd_pattern_trait<ContentType>::is_negated;
                            } else {
                                return false;
                            }
                        }();

                        // PERF: Skip SIMD for very small inputs (overhead dominates)
                        const auto remaining = last - current;
                        // Check if pattern is contiguous (no gaps)
                        constexpr bool is_contiguous = [] {
                            if constexpr (requires { simd::simd_pattern_trait<ContentType>::is_contiguous; }) {
                                return simd::simd_pattern_trait<ContentType>::is_contiguous;
                            } else {
                                return !has_gaps; // Fallback: no gaps means contiguous
                            }
                        }();
                        // PERF: Use SIMD for:
                        // - Contiguous ranges of any size (efficient >= <= comparison)
                        // - Large sparse sets (>= 8 chars)
                        // - Negated patterns
                        constexpr bool use_simd = is_contiguous || (range_size >= 8) || is_negated;
                        if constexpr ((!has_gaps || is_negated) && use_simd) {
                            if (remaining >= simd::SIMD_REPETITION_THRESHOLD) {
                                // FIX: Removed is_ascii_range check - overflow bug is now fixed!
                                // We can now safely process high-bit characters (0x80-0xFF)
                                Iterator simd_result =
                                    simd::match_pattern_repeat_simd<ContentType, A, B>(current, last, f);
                                if (simd_result != current) {
                                    return evaluate(begin, simd_result, last, f, captures, ctll::list<Tail...>());
                                }
                            }
                        }
                    } else {
                        // For patterns without min_char/max_char, only use SIMD if explicitly optimizable
                        // PERF: Prevents code bloat for patterns like [^aeiou]+ that would fallback to scalar
                        constexpr bool explicitly_optimizable =
                            simd::simd_pattern_trait<ContentType>::is_simd_optimizable;

                        if constexpr (explicitly_optimizable) {
                            const auto remaining = last - current;
                            if (remaining >= simd::SIMD_REPETITION_THRESHOLD) {
                                Iterator simd_result = simd::match_pattern_repeat_simd<ContentType, A, B>(current, last, f);
                                if (simd_result != current) {
                                    return evaluate(begin, simd_result, last, f, captures, ctll::list<Tail...>());
                                }
                            }
                        }
                        // Else: fall through to scalar implementation below
                    }
                } // End: if constexpr (B == 0 || B >= threshold)
            }
        }
    }

    // Original implementation for non-SIMD patterns
    {
        const auto backup_current = current;

        for (size_t i{0}; less_than_or_infinite<B>(i); ++i) {
            // try as many of inner as possible and then try outer once
            auto inner_result =
                evaluate(begin, current, last, not_empty_match(f), captures, ctll::list<Content..., end_cycle_mark>());

            if (!inner_result) {
                if (!less_than<A>(i))
                    break;
                return not_matched;
            }

            captures = inner_result.unmatch();
            current = inner_result.get_end_position();
        }

        return evaluate(begin, current, last, consumed_something(f, backup_current != current), captures,
                        ctll::list<Tail...>());
    }
}

// (gready) repeat
template <typename R, typename BeginIterator, typename Iterator, typename EndIterator, size_t A, size_t B,
          typename... Content, typename... Tail>
#ifdef CTRE_MSVC_GREEDY_WORKAROUND
constexpr inline void evaluate_recursive(R& result, size_t i, const BeginIterator begin, Iterator current,
                                         const EndIterator last, [[maybe_unused]] const flags& f, R captures,
                                         ctll::list<repeat<A, B, Content...>, Tail...> stack) noexcept {
#else
constexpr inline R evaluate_recursive(size_t i, const BeginIterator begin, Iterator current, const EndIterator last,
                                      [[maybe_unused]] const flags& f, R captures,
                                      ctll::list<repeat<A, B, Content...>, Tail...> stack) noexcept {
#endif
    if (less_than_or_infinite<B>(i)) {

        // a*ab
        // aab

        if (auto inner_result = evaluate(begin, current, last, not_empty_match(f), captures,
                                         ctll::list<Content..., end_cycle_mark>())) {
            // TODO MSVC issue:
            // if I uncomment this return it will not fail in constexpr (but the matching result will not be correct)
            //  return inner_result
            // I tried to add all constructors to R but without any success
            auto tmp_current = current;
            tmp_current = inner_result.get_end_position();
#ifdef CTRE_MSVC_GREEDY_WORKAROUND
            evaluate_recursive(result, i + 1, begin, tmp_current, last, f, inner_result.unmatch(), stack);
            if (result) {
                return;
            }
#else
            if (auto rec_result =
                    evaluate_recursive(i + 1, begin, tmp_current, last, f, inner_result.unmatch(), stack)) {
                return rec_result;
            }
#endif
        }
    }
#ifdef CTRE_MSVC_GREEDY_WORKAROUND
    result = evaluate(begin, current, last, consumed_something(f), captures, ctll::list<Tail...>());
#else
    return evaluate(begin, current, last, consumed_something(f), captures, ctll::list<Tail...>());
#endif
}

// SIMD-optimized single character repetition would be implemented here
// For now, we use the original implementation to avoid constexpr issues

// (greedy) repeat
template <typename R, typename BeginIterator, typename Iterator, typename EndIterator, size_t A, size_t B,
          typename... Content, typename... Tail>
constexpr CTRE_FORCE_INLINE R evaluate(const BeginIterator begin, Iterator current, const EndIterator last,
                                       [[maybe_unused]] const flags& f, R captures,
                                       [[maybe_unused]] ctll::list<repeat<A, B, Content...>, Tail...> stack) {

    if constexpr ((B != 0) && (A > B)) {
        return not_matched;
    }

    // EARLY REJECTION for greedy repeat: Check first char BEFORE any SIMD setup
    // For non-matching inputs (e.g., [0-9]+ on letters), reject in ~1ns vs ~15ns
    if constexpr (sizeof...(Content) == 1) {
        using ContentType = std::tuple_element_t<0, std::tuple<Content...>>;
        if constexpr (requires { simd::simd_pattern_trait<ContentType>::min_char; }) {
            if (current != last) {
                constexpr char min_c = simd::simd_pattern_trait<ContentType>::min_char;
                constexpr char max_c = simd::simd_pattern_trait<ContentType>::max_char;
                unsigned char first = static_cast<unsigned char>(*current);
                if (first < static_cast<unsigned char>(min_c) || first > static_cast<unsigned char>(max_c)) {
                    if constexpr (A == 0) {
                        return evaluate(begin, current, last, f, captures, ctll::list<Tail...>());
                    } else {
                        return not_matched;
                    }
                }
            }
        }
    }

    // SIMD optimization for repetition patterns at runtime (invisible to user)
    // NOTE: This SIMD decision logic is duplicated in both possessive_repeat and repeat
    // for performance. Extracting to helper causes 70% regression (1.82x → 1.07x) due to
    // template instantiation preventing compile-time optimization. If modifying, update BOTH!
    if constexpr (sizeof...(Content) == 1) {
        if (!std::is_constant_evaluated() && simd::can_use_simd()) {
            using ContentType = std::tuple_element_t<0, std::tuple<Content...>>;

            // Only use SIMD for char iterators (not wchar_t, char16_t, char32_t, etc.)
            if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<decltype(*std::declval<Iterator>())>>,
                                         char>) {

                // PERF: Special case for 'any' (wildcard .) - ultra-fast advance!
                // FIX: Only use when nothing follows (Tail empty), otherwise .*x patterns fail
                if constexpr (std::is_same_v<ContentType, any> && sizeof...(Tail) == 0) {
                    size_t count = 0;
                    Iterator start = current;

                    if (multiline_mode(f)) {
                        // In multiline: skip all non-\n characters
                        while (current != last && (B == 0 || count < B)) {
                            if (*current == '\n')
                                break;
                            ++current;
                            ++count;
                        }
                    } else {
                        // In singleline: skip ANY characters (fastest path!)
                        if constexpr (std::is_pointer_v<Iterator>) {
                            size_t remaining = last - current;
                            size_t to_advance = (B == 0) ? remaining : std::min(remaining, B);
                            current += to_advance;
                            count = to_advance;
                        } else {
                            while (current != last && (B == 0 || count < B)) {
                                ++current;
                                ++count;
                            }
                        }
                    }

                    if (count >= A) {
                        return evaluate(begin, current, last, consumed_something(f), captures, ctll::list<Tail...>());
                    } else {
                        return not_matched;
                    }
                }
                // SIMD optimization: Compile-time suitability check
                // Skip for: 1) Small bounded reps, 2) Sequence patterns (literal after repeat)
                //           3) Simple single-range patterns (to avoid code bloat overhead)
                //           4) Sentinel iterators (don't support pointer arithmetic)
                // This prevents binary bloat on patterns like IPv4: [0-9]+\.[0-9]+\.
                constexpr bool has_literal_after2 = simd::has_literal_next<Tail...>();

                // CRITICAL FIX: Only use SIMD with contiguous iterators that support subtraction
                // C++20: std::contiguous_iterator includes std::string::iterator and pointers
                constexpr bool can_use_pointer_arithmetic2 =
                    std::contiguous_iterator<Iterator> &&
                    (std::is_pointer_v<EndIterator> || std::contiguous_iterator<EndIterator>);

                if constexpr (simd::can_use_simd() && (B == 0 || B >= simd::SIMD_REPETITION_THRESHOLD) &&
                              !has_literal_after2 && can_use_pointer_arithmetic2) {
                    // Try multi-range SIMD first (handles any number of ranges: 2, 3, 4, ... N)
                    // Examples: [a-zA-Z], [0-9a-fA-F], [a-eA-Eg-kG-K], etc.
                    const auto remaining_input = last - current;
                    if constexpr (simd::is_multi_range<ContentType>::is_valid) {
                        if (remaining_input >= simd::SIMD_REPETITION_THRESHOLD) {
                            Iterator multirange_result =
                                simd::match_multirange_repeat<ContentType, A, B>(current, last, f);
                            if (multirange_result != current) {
                                return evaluate(begin, multirange_result, last, f, captures, ctll::list<Tail...>());
                            }
                        }
                    }

                    // Try Shufti for sparse character sets (like [aeiou])
                    if constexpr (simd::shufti_pattern_trait<ContentType>::should_use_shufti) {
                        if (remaining_input >= simd::SIMD_SHUFTI_THRESHOLD) {
                            Iterator shufti_result =
                                simd::match_pattern_repeat_shufti<ContentType, A, B>(current, last, f);
                            if (shufti_result != current) {
                                return evaluate(begin, shufti_result, last, f, captures, ctll::list<Tail...>());
                            }
                        }
                    }

                    // Check range size - balance SIMD overhead vs scalar simplicity
                    if constexpr (requires {
                                      simd::simd_pattern_trait<ContentType>::min_char;
                                      simd::simd_pattern_trait<ContentType>::max_char;
                                  }) {
                        constexpr char min_char = simd::simd_pattern_trait<ContentType>::min_char;
                        constexpr char max_char = simd::simd_pattern_trait<ContentType>::max_char;
                        // FIX: Cast to unsigned char to avoid signed overflow in range calculation
                        constexpr size_t range_size =
                            static_cast<unsigned char>(max_char) - static_cast<unsigned char>(min_char) + 1;
                        constexpr size_t char_count = count_char_class_size(static_cast<ContentType*>(nullptr));

                        // Gap detection: skip SIMD if char_count < range_size (has gaps)
                        // Note: Multi-range patterns (handled above) won't reach here
                        constexpr bool has_gaps = (char_count > 0) && (char_count < range_size);

                        // Check if this is a negated range (always use SIMD for negated!)
                        constexpr bool is_negated = [] {
                            if constexpr (requires { simd::simd_pattern_trait<ContentType>::is_negated; }) {
                                return simd::simd_pattern_trait<ContentType>::is_negated;
                            } else {
                                return false;
                            }
                        }();

                        // PERF: Skip SIMD for very small inputs (overhead dominates)
                        // For inputs <28 bytes, SIMD overhead (detection, setup, fallback) exceeds benefit
                        // 28 bytes = conservative threshold that avoids regressions on complex patterns
                        const auto remaining = last - current;
                        constexpr bool use_simd = true;
                        if constexpr ((!has_gaps || is_negated) && use_simd) {
                            if (remaining >= simd::SIMD_REPETITION_THRESHOLD) {
                                // FIX: Removed is_ascii_range check - overflow bug is now fixed!
                                // We can now safely process high-bit characters (0x80-0xFF)
                                Iterator simd_result =
                                    simd::match_pattern_repeat_simd<ContentType, A, B>(current, last, f);
                                if (simd_result != current) {
                                    return evaluate(begin, simd_result, last, f, captures, ctll::list<Tail...>());
                                }
                            }
                        }
                    } else {
                        // For patterns without min_char/max_char, only use SIMD if explicitly optimizable
                        // PERF: Prevents code bloat for patterns like [^aeiou]+ that would fallback to scalar
                        constexpr bool explicitly_optimizable =
                            simd::simd_pattern_trait<ContentType>::is_simd_optimizable;

                        if constexpr (explicitly_optimizable) {
                            const auto remaining = last - current;
                            if (remaining >= simd::SIMD_REPETITION_THRESHOLD) {
                                Iterator simd_result = simd::match_pattern_repeat_simd<ContentType, A, B>(current, last, f);
                                if (simd_result != current) {
                                    return evaluate(begin, simd_result, last, f, captures, ctll::list<Tail...>());
                                }
                            }
                        }
                        // Else: fall through to scalar implementation below
                    }
                } // End: if constexpr (B == 0 || B >= threshold)
            }
        }
    }

#ifndef CTRE_DISABLE_GREEDY_OPT
    if constexpr (!collides(calculate_first(Content{}...), calculate_first(Tail{}...))) {
        return evaluate(begin, current, last, f, captures, ctll::list<possessive_repeat<A, B, Content...>, Tail...>());
    } else
#endif
    {
        // A..B
        size_t i{0};
        while (less_than<A>(i)) {
            auto inner_result =
                evaluate(begin, current, last, not_empty_match(f), captures, ctll::list<Content..., end_cycle_mark>());

            if (!inner_result)
                return not_matched;

            captures = inner_result.unmatch();
            current = inner_result.get_end_position();

            ++i;
        }

#ifdef CTRE_MSVC_GREEDY_WORKAROUND
        R result;
        evaluate_recursive(result, i, begin, current, last, f, captures, stack);
        return result;
#else
        return evaluate_recursive(i, begin, current, last, f, captures, stack);
#endif
    }
}

// capture (numeric ID)
template <typename R, typename BeginIterator, typename Iterator, typename EndIterator, size_t Id, typename... Content,
          typename... Tail>
constexpr CTRE_FORCE_INLINE R evaluate(const BeginIterator begin, Iterator current, const EndIterator last,
                                       const flags& f, R captures,
                                       ctll::list<capture<Id, Content...>, Tail...>) noexcept {
    return evaluate(begin, current, last, f, captures.template start_capture<Id>(current),
                    ctll::list<sequence<Content...>, numeric_mark<Id>, Tail...>());
}

// capture end mark (numeric and string ID)
template <typename R, typename BeginIterator, typename Iterator, typename EndIterator, size_t Id, typename... Tail>
constexpr CTRE_FORCE_INLINE R evaluate(const BeginIterator begin, Iterator current, const EndIterator last,
                                       const flags& f, R captures, ctll::list<numeric_mark<Id>, Tail...>) noexcept {
    return evaluate(begin, current, last, f, captures.template end_capture<Id>(current), ctll::list<Tail...>());
}

// capture (string ID)
template <typename R, typename BeginIterator, typename Iterator, typename EndIterator, size_t Id, typename Name,
          typename... Content, typename... Tail>
constexpr CTRE_FORCE_INLINE R evaluate(const BeginIterator begin, Iterator current, const EndIterator last,
                                       const flags& f, R captures,
                                       ctll::list<capture_with_name<Id, Name, Content...>, Tail...>) noexcept {
    return evaluate(begin, current, last, f, captures.template start_capture<Id>(current),
                    ctll::list<sequence<Content...>, numeric_mark<Id>, Tail...>());
}

// backreference support (match agains content of iterators)
template <typename Iterator>
struct string_match {
    Iterator position;
    bool match;
};

template <typename Iterator, typename EndIterator>
constexpr CTRE_FORCE_INLINE string_match<Iterator> match_against_range(Iterator current, const EndIterator last,
                                                                       Iterator range_current, const Iterator range_end,
                                                                       flags) noexcept {
    while (last != current && range_end != range_current) {
        if (*current == *range_current) {
            current++;
            range_current++;
        } else {
            return {current, false};
        }
    }
    return {current, range_current == range_end};
}

// backreference with name
template <typename R, typename Id, typename BeginIterator, typename Iterator, typename EndIterator, typename... Tail>
constexpr CTRE_FORCE_INLINE R evaluate(const BeginIterator begin, Iterator current, const EndIterator last,
                                       const flags& f, R captures,
                                       ctll::list<back_reference_with_name<Id>, Tail...>) noexcept {

    if (const auto ref = captures.template get<Id>()) {
        if (auto result = match_against_range(current, last, ref.begin(), ref.end(), f); result.match) {
            return evaluate(begin, result.position, last, consumed_something(f, ref.begin() != ref.end()), captures,
                            ctll::list<Tail...>());
        }
    }
    return not_matched;
}

// backreference
template <typename R, size_t Id, typename BeginIterator, typename Iterator, typename EndIterator, typename... Tail>
constexpr CTRE_FORCE_INLINE R evaluate(const BeginIterator begin, Iterator current, const EndIterator last,
                                       const flags& f, R captures, ctll::list<back_reference<Id>, Tail...>) noexcept {

    if (const auto ref = captures.template get<Id>()) {
        if (auto result = match_against_range(current, last, ref.begin(), ref.end(), f); result.match) {
            return evaluate(begin, result.position, last, consumed_something(f, ref.begin() != ref.end()), captures,
                            ctll::list<Tail...>());
        }
    }
    return not_matched;
}

// end of lookahead
template <typename R, typename BeginIterator, typename Iterator, typename EndIterator, typename... Tail>
constexpr CTRE_FORCE_INLINE R evaluate(const BeginIterator, Iterator, const EndIterator, flags, R captures,
                                       ctll::list<end_lookahead_mark>) noexcept {
    // TODO check interaction with non-empty flag
    return captures.matched();
}

template <typename R, typename BeginIterator, typename Iterator, typename EndIterator, typename... Tail>
constexpr CTRE_FORCE_INLINE R evaluate(const BeginIterator, Iterator, const EndIterator, flags, R captures,
                                       ctll::list<end_lookbehind_mark>) noexcept {
    // TODO check interaction with non-empty flag
    return captures.matched();
}

// lookahead positive
template <typename R, typename BeginIterator, typename Iterator, typename EndIterator, typename... Content,
          typename... Tail>
constexpr CTRE_FORCE_INLINE R evaluate(const BeginIterator begin, Iterator current, const EndIterator last,
                                       const flags& f, R captures,
                                       ctll::list<lookahead_positive<Content...>, Tail...>) noexcept {

    if (auto lookahead_result =
            evaluate(begin, current, last, f, captures, ctll::list<sequence<Content...>, end_lookahead_mark>())) {
        captures = lookahead_result.unmatch();
        return evaluate(begin, current, last, f, captures, ctll::list<Tail...>());
    } else {
        return not_matched;
    }
}

// lookahead negative
template <typename R, typename BeginIterator, typename Iterator, typename EndIterator, typename... Content,
          typename... Tail>
constexpr CTRE_FORCE_INLINE R evaluate(const BeginIterator begin, Iterator current, const EndIterator last,
                                       const flags& f, R captures,
                                       ctll::list<lookahead_negative<Content...>, Tail...>) noexcept {

    if (auto lookahead_result =
            evaluate(begin, current, last, f, captures, ctll::list<sequence<Content...>, end_lookahead_mark>())) {
        return not_matched;
    } else {
        return evaluate(begin, current, last, f, captures, ctll::list<Tail...>());
    }
}

// lookbehind positive
constexpr bool is_at_least_bidirectional(std::input_iterator_tag) {
    return false;
}

constexpr bool is_at_least_bidirectional(std::bidirectional_iterator_tag) {
    return true;
}

template <typename R, typename BeginIterator, typename Iterator, typename EndIterator, typename... Content,
          typename... Tail>
constexpr CTRE_FORCE_INLINE R evaluate(const BeginIterator begin, Iterator current, const EndIterator last,
                                       const flags& f, R captures,
                                       ctll::list<lookbehind_positive<Content...>, Tail...>) noexcept {
    static_assert(is_at_least_bidirectional(typename std::iterator_traits<Iterator>::iterator_category{}),
                  "to use lookbehind you must provide bi-directional iterator");

    if (auto lookbehind_result = evaluate(std::make_reverse_iterator(last), std::make_reverse_iterator(current),
                                          std::make_reverse_iterator(begin), f, captures,
                                          ctll::list<sequence<Content...>, end_lookbehind_mark>())) {
        captures = lookbehind_result.unmatch();
        return evaluate(begin, current, last, f, captures, ctll::list<Tail...>());
    } else {
        return not_matched;
    }
}

// lookbehind negative
template <typename R, typename BeginIterator, typename Iterator, typename EndIterator, typename... Content,
          typename... Tail>
constexpr CTRE_FORCE_INLINE R evaluate(const BeginIterator begin, Iterator current, const EndIterator last,
                                       const flags& f, R captures,
                                       ctll::list<lookbehind_negative<Content...>, Tail...>) noexcept {
    static_assert(is_at_least_bidirectional(typename std::iterator_traits<Iterator>::iterator_category{}),
                  "to use negative lookbehind you must provide bi-directional iterator");

    if (auto lookbehind_result = evaluate(std::make_reverse_iterator(last), std::make_reverse_iterator(current),
                                          std::make_reverse_iterator(begin), f, captures,
                                          ctll::list<sequence<Content...>, end_lookbehind_mark>())) {
        return not_matched;
    } else {
        return evaluate(begin, current, last, f, captures, ctll::list<Tail...>());
    }
}

template <typename...>
constexpr auto dependent_false = false;

// atomic_group<...> is just transformation to possessive_repeat<1,1,...>
template <typename R, typename BeginIterator, typename Iterator, typename EndIterator, typename... Content,
          typename... Tail>
constexpr CTRE_FORCE_INLINE R evaluate(const BeginIterator begin, Iterator current, const EndIterator last,
                                       const flags& f, R captures,
                                       ctll::list<atomic_group<Content...>, Tail...>) noexcept {
    return evaluate(begin, current, last, f, captures, ctll::list<possessive_repeat<1, 1, Content...>, Tail...>{});
}

// switching modes
template <typename R, typename BeginIterator, typename Iterator, typename EndIterator, typename Mode, typename... Tail>
constexpr CTRE_FORCE_INLINE R evaluate(const BeginIterator begin, Iterator current, const EndIterator last,
                                       const flags& f, R captures, ctll::list<mode_switch<Mode>, Tail...>) noexcept {
    return evaluate(begin, current, last, f + Mode{}, captures, ctll::list<Tail...>());
}

} // namespace ctre

#endif
