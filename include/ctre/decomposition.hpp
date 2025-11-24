#ifndef CTRE__DECOMPOSITION__HPP
#define CTRE__DECOMPOSITION__HPP

// Pattern Decomposition API
// Public interface for literal extraction and prefiltering

#include "glushkov_nfa.hpp"
#include "dominator_analysis.hpp"

namespace ctre {
namespace decomposition {

// =============================================================================
// Public API: Pattern Analysis
// =============================================================================

// Check if a pattern has an extractable prefilter literal
template <typename Pattern>
inline constexpr bool has_prefilter_literal = dominators::has_extractable_literal<Pattern>();

// Extract the prefilter literal from a pattern
template <typename Pattern>
inline constexpr auto prefilter_literal = dominators::extract_literal<Pattern>();

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
