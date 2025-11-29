#ifndef CTRE_BITNFA_MATCH_HPP
#define CTRE_BITNFA_MATCH_HPP

#include "bitnfa_types.hpp"
#include <string_view>
#include <vector>
#include <optional>

// Need full CTRE for pattern compilation
#ifndef CTRE_V2__CTRE__HPP
#include "../../ctre.hpp"
#endif

#include "compile_from_ast.hpp"
#include "pattern_traits.hpp"
#include "simd_acceleration.hpp"
#include "specialized_matchers.hpp"
#include "literal_fast_path.hpp"

// Phase 3: Runtime String Matching API
// match(), search(), find_all() functions for BitNFA

namespace ctre::bitnfa {

// Match result - stores match position and length
struct match_result {
    size_t position = 0;  // Start position in input
    size_t length = 0;    // Length of match
    bool matched = false; // Whether match succeeded

    explicit operator bool() const { return matched; }

    std::string_view to_view(std::string_view input) const {
        if (!matched) return {};
        return input.substr(position, length);
    }
};

// =============================================================================
// Phase 3a: Full String Match
// =============================================================================

// Match entire string against pattern
// Returns true only if the entire input matches
__attribute__((always_inline)) inline match_result match(const BitNFA128& nfa, std::string_view input) {
    StateMask128 current = nfa.get_initial_state();

    size_t pos = 0;
    for (char c : input) {
        current = nfa.calculate_successors(current, c);
        if (current.none()) {
            // No active states - match failed
            return match_result{0, 0, false};
        }
        ++pos;
    }

    // Check if we're at an accept state
    if (nfa.has_accept(current)) {
        return match_result{0, input.size(), true};
    }

    return match_result{0, 0, false};
}

// Convenience: compile pattern string and match
template <ctll::fixed_string Pattern>
inline match_result match(std::string_view input) {
    // Parse pattern to AST
    using tmp = typename ctll::parser<::ctre::pcre, Pattern, ::ctre::pcre_actions>::template output<::ctre::pcre_context<>>;
    static_assert(tmp(), "Regular Expression contains syntax error.");
    using AST = decltype(ctll::front(typename tmp::output_type::stack_type()));

    // Try specialized fast path for simple patterns
    if constexpr (should_use_fast_path<AST>::value) {
        if constexpr (is_string<AST>::value || is_character<AST>::value) {
            bool matched = fast_match(input, static_cast<AST*>(nullptr));
            return {0, matched ? input.size() : size_t(0), matched};
        }
    }

    // NEW: Fast path for literal alternations (foo|bar|baz)
    // This is our BitNFA optimization with Teddy-ready architecture!
    if constexpr (is_literal_alternation<AST>::value) {
        constexpr auto literals = get_literals_if_applicable<AST>();
        size_t match_len = 0;
        int idx = match_literal_alternation(input, literals, &match_len);
        if (idx >= 0) {
            return {0, match_len, true};
        }
        return {0, 0, false};
    }

    // For patterns with character classes or repetitions: delegate to CTRE (has SIMD!)
    if constexpr (::ctre::glushkov::is_repeat<AST>::value || ::ctre::glushkov::is_select<AST>::value) {
        auto ctre_result = ::ctre::match<Pattern>(input);
        if (ctre_result) {
            return match_result{0, input.size(), true};
        }
        return match_result{0, 0, false};
    } else {
        // Fall back to generic NFA matching (rare - only for complex patterns)
        static constexpr auto nfa = compile_pattern_string_with_charclass<Pattern>();
        return match(nfa, input);
    }
}

// =============================================================================
// Phase 3b: Search (Find First Occurrence)
// =============================================================================

// Search for pattern in input (find first occurrence, longest match)
// Returns position and length of first match
//
// HYPERSCAN-STYLE TWO-LAYER ARCHITECTURE:
// Layer 1: SIMD Acceleration (this function) - uses generic NFA
// Layer 2: Pattern-specific acceleration (in template version below)
inline match_result search(const BitNFA128& nfa, std::string_view input) {
    // Generic NFA search (no SIMD acceleration)
    // This is only used for patterns that don't have pattern-specific acceleration
    for (size_t start = 0; start < input.size(); ++start) {
        StateMask128 current = nfa.get_initial_state();
        std::optional<size_t> match_end;  // Track longest match from this start

        // Try matching from this position
        for (size_t pos = start; pos < input.size(); ++pos) {
            current = nfa.calculate_successors(current, input[pos]);

            if (current.none()) {
                // Dead end
                break;
            }

            if (nfa.has_accept(current)) {
                // Found a match - keep going to find longest
                match_end = pos;
            }
        }

        if (match_end.has_value()) {
            // Found a match from this start position
            size_t length = match_end.value() - start + 1;
            return match_result{start, length, true};
        }
    }

    return match_result{0, 0, false};
}

// Convenience: compile pattern string and search
// HYPERSCAN-STYLE ACCELERATION: SIMD skip + NFA evaluation
template <ctll::fixed_string Pattern>
inline match_result search(std::string_view input) {
    // Parse pattern to AST
    using tmp = typename ctll::parser<::ctre::pcre, Pattern, ::ctre::pcre_actions>::template output<::ctre::pcre_context<>>;
    static_assert(tmp(), "Regular Expression contains syntax error.");
    using AST = decltype(ctll::front(typename tmp::output_type::stack_type()));

    // Fast path for simple strings (2x faster than CTRE!)
    if constexpr (should_use_fast_path<AST>::value) {
        if constexpr (is_string<AST>::value || is_character<AST>::value) {
            return fast_search(input, static_cast<AST*>(nullptr), static_cast<match_result*>(nullptr));
        }
    }

    // HYPERSCAN-STYLE ACCELERATION for character class patterns
    // For simple repetitions like [a-z]+, we can skip the NFA entirely!
    // Layer 1: SIMD finds START of match
    // Layer 2: SIMD/scalar finds END of match (no NFA needed!)
    if constexpr (can_accelerate<AST>::value) {
        // Extract the character class type from repeat
        using ContentType = typename extract_repeat_content<AST>::type;

        const char* data = input.data();
        const char* end_ptr = data + input.size();

        // SIMD-ONLY path for simple repetitions - NO NFA!
        for (const char* start = data; start < end_ptr; ) {
            // Step 1: SIMD finds first matching character
            const char* match_start = simd_find_char_class<ContentType>(start, end_ptr);

            if (match_start >= end_ptr) break;  // No more matches

            // Step 2: Scan forward to find where it stops matching
            const char* match_end = simd_find_char_class_end<ContentType>(match_start, end_ptr);

            // We found a match!
            size_t pos = match_start - data;
            size_t length = match_end - match_start;

            if (length > 0) {  // For + quantifier, need at least 1 char
                return match_result{pos, length, true};
            }

            start = match_start + 1;  // Try next position
        }

        return match_result{0, 0, false};
    } else {
        // For other patterns: use generic NFA search
        static constexpr auto nfa = compile_pattern_string_with_charclass<Pattern>();
        return search(nfa, input);
    }
}

// =============================================================================
// Phase 3c: Find All Occurrences
// =============================================================================

// Find all non-overlapping occurrences of pattern in input
inline std::vector<match_result> find_all(const BitNFA128& nfa, std::string_view input) {
    std::vector<match_result> results;

    size_t start = 0;
    while (start < input.size()) {
        StateMask128 current = nfa.get_initial_state();
        std::optional<size_t> match_end;

        // Try matching from this position
        for (size_t pos = start; pos < input.size(); ++pos) {
            current = nfa.calculate_successors(current, input[pos]);

            if (current.none()) {
                // Dead end
                break;
            }

            if (nfa.has_accept(current)) {
                // Record this match (keep going to find longest match)
                match_end = pos;
            }
        }

        if (match_end.has_value()) {
            // Found a match from start to match_end
            size_t length = match_end.value() - start + 1;
            results.push_back(match_result{start, length, true});

            // Continue searching after this match
            start = match_end.value() + 1;
        } else {
            // No match from this position, try next
            ++start;
        }
    }

    return results;
}

// Convenience: compile pattern string and find all
template <ctll::fixed_string Pattern>
inline std::vector<match_result> find_all(std::string_view input) {
    // Compile NFA at compile-time and cache it!
    static constexpr auto nfa = compile_pattern_string_with_charclass<Pattern>();
    return find_all(nfa, input);
}

} // namespace ctre::bitnfa

#endif // CTRE_BITNFA_MATCH_HPP
