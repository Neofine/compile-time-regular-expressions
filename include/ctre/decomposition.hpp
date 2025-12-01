#ifndef CTRE__DECOMPOSITION__HPP
#define CTRE__DECOMPOSITION__HPP

// Pattern Decomposition API
// Public interface for literal extraction and prefiltering

#include "glushkov_nfa.hpp"
#include "dominator_analysis.hpp"
#include "region_analysis.hpp"
#include "literal_extraction_simple_multi.hpp"

namespace ctre {
// Forward declaration for unwrapper
template <typename RE, typename Method, typename Modifier> struct regular_expression;

namespace decomposition {

// =============================================================================
// Helper: Extract raw AST from regular_expression wrapper
// =============================================================================

// Unwrap captures (CTRE adds implicit captures around parentheses)
template <typename T>
struct unwrap_capture {
    using type = T;
};

template <size_t Index, typename... Content>
struct unwrap_capture<ctre::capture<Index, Content...>> {
    using type = ctre::sequence<Content...>; // Extract the content
};

// Single content - no need for sequence
template <size_t Index, typename Content>
struct unwrap_capture<ctre::capture<Index, Content>> {
    using type = Content;
};

template <typename T>
using unwrap_capture_t = typename unwrap_capture<T>::type;

// Unwrap regular_expression wrapper using helper functions
// This avoids partial specialization issues with forward declarations

// Helper function to extract RE from regular_expression
template <typename RE, typename Method, typename Modifier>
constexpr auto unwrap_regex_impl(const ctre::regular_expression<RE, Method, Modifier>*) -> unwrap_capture_t<RE>;

// Default: return as-is
template <typename T>
constexpr auto unwrap_regex_impl(const T*) -> T;

// Wrapper to use the function-based approach
template <typename T>
using unwrap_regex_t = decltype(unwrap_regex_impl(static_cast<T*>(nullptr)));

// =============================================================================
// Public API: Pattern Analysis
// =============================================================================

// Extract literal with character-set expansion and fallback
template <typename Pattern>
inline constexpr auto extract_literal_with_expansion_and_fallback() -> dominators::literal_result<64> {
    using RawAST = unwrap_regex_t<Pattern>;

    // Always compute NFA-based result for comparison
    constexpr auto nfa = glushkov::glushkov_nfa<RawAST>();
    constexpr auto nfa_result = dominators::extract_literal_from_dominators(nfa);

    // Step 0: Try AST-based extraction with character-set expansion
    constexpr auto multi_result = extraction::extract_literals_simple_multi<RawAST>();
    if constexpr (multi_result.has_literals && multi_result.count > 0) {
        constexpr auto longest = multi_result.get_longest();

        // BUG FIX #25: Always prefer the longest expansion literal for SIMD prefiltering!
        // Even if expansion creates multiple variants (e.g., [0-3]test → "0test", "1test", ...),
        // the longest literal (5 chars) is MORE selective for SIMD than the common substring (4 chars).
        //
        // Example: [0-3]test
        //   - Expansion: "0test" (5 chars, 1/4 coverage but very selective)
        //   - NFA: "test" (4 chars, 100% coverage but less selective, more false positives)
        //   - Prefer "0test"! It filters out "test", "atest", "btest", etc.
        //
        // Previous "BUG FIX #18" incorrectly preferred NFA literals when they were within
        // 1 character of the expansion length, which defeated the purpose of char expansion.
        // The Hyperscan paper explicitly recommends using the longest literal from expansion.

        // Use expansion result
        dominators::literal_result<64> result{};
        result.has_literal = longest.has_literal;
        result.length = longest.length;
        result.start_position = longest.start_position;
        result.nfa_dominator_length = nfa_result.has_literal ? nfa_result.length : 0;  // Store NFA length for validation
        for (size_t i = 0; i < longest.length; ++i) {
            result.chars[i] = longest.chars[i];
        }
        return result;
    }

    // Step 1: Use NFA-based result if available AND it's long enough
    if constexpr (nfa_result.has_literal && nfa_result.length >= 16) {
        dominators::literal_result<64> result = nfa_result;
        result.nfa_dominator_length = nfa_result.length;  // Same as literal length (not from expansion)
        return result;
    }

    // Step 2: Fallback to dominant region analysis
    // This runs if:
    // - Dominator analysis found no literal, OR
    // - Dominator found a literal < 16 bytes (too short for effective prefiltering)
    constexpr auto region_result = region::extract_literal_from_regions(nfa);

    // Step 3: Choose the better result between short dominator literal and region result
    if constexpr (nfa_result.has_literal && nfa_result.length < 16) {
        // We have a short dominator literal - compare with region result
        if constexpr (region_result.has_literal && region_result.length > nfa_result.length) {
            // Region analysis found a longer literal - use it!
            return region_result;
        } else {
            // Keep the dominator literal (even if short)
            dominators::literal_result<64> result = nfa_result;
            result.nfa_dominator_length = nfa_result.length;
            return result;
        }
    }

    // Step 4: Return region result (either dominator found nothing, or region is better)
    return region_result;
}

// Extract literal with fallback to region analysis (original version, no expansion)
template <typename Pattern>
inline auto extract_literal_with_fallback() {
    using RawAST = unwrap_regex_t<Pattern>;
    constexpr auto nfa = glushkov::glushkov_nfa<RawAST>();

    // Step 1: Try dominant path analysis (fast, covers 97%+)
    constexpr auto path_result = dominators::extract_literal_from_dominators(nfa);
    if constexpr (path_result.has_literal && path_result.length >= 16) {
        return path_result;  // Path analysis succeeded with good literal!
    }

    // Step 2: Fallback to dominant region analysis
    // Runs if dominator found nothing OR found a literal < 16 bytes
    constexpr auto region_result = region::extract_literal_from_regions(nfa);

    // Step 3: Choose better result
    if constexpr (path_result.has_literal && path_result.length < 16) {
        // Have short dominator literal - compare with region
        if constexpr (region_result.has_literal && region_result.length > path_result.length) {
            return region_result;  // Region found longer literal
        }
        return path_result;  // Keep dominator literal
    }

    return region_result;  // Either dominator found nothing, or region is better
}

// Check if a pattern has an extractable prefilter literal
template <typename Pattern>
inline constexpr bool has_prefilter_literal = dominators::has_extractable_literal<unwrap_regex_t<Pattern>>();

// Extract the prefilter literal from a pattern (path only for now)
template <typename Pattern>
inline constexpr auto prefilter_literal = dominators::extract_literal<unwrap_regex_t<Pattern>>();

// Extract with region fallback (runtime, not constexpr)
template <typename Pattern>
inline auto prefilter_literal_with_fallback = extract_literal_with_fallback<unwrap_regex_t<Pattern>>();

// Get the Glushkov NFA for a pattern (for advanced use)
template <typename Pattern>
inline constexpr auto glushkov_nfa = glushkov::glushkov_nfa<Pattern>();

// =============================================================================
// Compile-Time Literal String Type
// =============================================================================

// Represents a literal string extracted at compile-time
template <char... Cs>
struct literal_string {
    static constexpr size_t length = sizeof...(Cs);
    static constexpr char value[length + 1] = {Cs..., '\0'};

    // Array of characters for template parameter pack expansion
    static constexpr std::array<char, length> chars = {Cs...};

    // Convert to string_view at runtime
    constexpr operator std::string_view() const {
        return std::string_view(value, length);
    }
};

// Helper: Convert literal_result to literal_string at compile-time
template <typename Pattern, auto Literal = prefilter_literal<Pattern>>
struct literal_for_pattern {
    static constexpr bool has_literal = Literal.has_literal;
    static constexpr size_t length = Literal.length;

    // Helper to extract chars as template parameter pack
    template <size_t... Is>
    static constexpr auto make_string(std::index_sequence<Is...>) {
        return literal_string<Literal.chars[Is]...>{};
    }

    using type = decltype(make_string(std::make_index_sequence<length>{}));
};

// Convenience alias
template <typename Pattern>
using literal_string_for = typename literal_for_pattern<Pattern>::type;

// =============================================================================
// Pattern Statistics (for debugging/analysis)
// =============================================================================

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
