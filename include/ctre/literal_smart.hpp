#ifndef CTRE_LITERAL_SMART_HPP
#define CTRE_LITERAL_SMART_HPP

#include "literal_alternation_fast_path.hpp"
#include "teddy_simple.hpp"
#include <string_view>

// Smart Literal Alternation Matcher
// ==================================
// Automatically chooses the best algorithm based on use case:
//   - Short MATCH (< 50 bytes): Simple sequential scan
//   - Long MATCH (>= 50 bytes): Teddy SIMD
//   - SEARCH (any size): Always Teddy SIMD

namespace ctre {
namespace literal_smart {

// Threshold for switching from simple to Teddy (empirically determined)
constexpr size_t TEDDY_THRESHOLD = 50;

// =============================================================================
// Smart Match: Choose Best Algorithm
// =============================================================================

template <size_t MaxLiterals, size_t MaxLength>
inline size_t smart_match(
    std::string_view input,
    const literal_list<MaxLiterals, MaxLength>& literals,
    const teddy::teddy_mask<MaxLiterals>& mask) noexcept
{
    // For short inputs: use simple scan (lower overhead)
    if (input.size() < TEDDY_THRESHOLD) {
        return literals.fast_match(input);
    }

    // For long inputs: use Teddy SIMD (faster scan)
    return teddy::teddy_match(input, literals, mask);
}

// =============================================================================
// Smart Search: Always Use Teddy
// =============================================================================

template <size_t MaxLiterals, size_t MaxLength>
inline const char* smart_search(
    std::string_view input,
    const literal_list<MaxLiterals, MaxLength>& literals,
    const teddy::teddy_mask<MaxLiterals>& mask,
    size_t* out_length = nullptr) noexcept
{
    // Always use Teddy for search (34x faster!)
    return teddy::teddy_search(input, literals, mask, out_length);
}

// =============================================================================
// High-Level API
// =============================================================================

// Smart match with compile-time pattern
template <typename Pattern>
inline size_t match(std::string_view input) {
    if constexpr (is_literal_alt<Pattern>::value) {
        constexpr auto literals = get_literal_list<Pattern>();
        constexpr auto mask = teddy::build_teddy_mask(literals);
        return smart_match(input, literals, mask);
    } else {
        return 0;
    }
}

// Smart search with compile-time pattern
template <typename Pattern>
inline const char* search(std::string_view input, size_t* out_length = nullptr) {
    if constexpr (is_literal_alt<Pattern>::value) {
        constexpr auto literals = get_literal_list<Pattern>();
        constexpr auto mask = teddy::build_teddy_mask(literals);
        return smart_search(input, literals, mask, out_length);
    } else {
        return nullptr;
    }
}

} // namespace literal_smart
} // namespace ctre

#endif // CTRE_LITERAL_SMART_HPP
