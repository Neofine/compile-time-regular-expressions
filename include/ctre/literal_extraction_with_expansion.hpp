#ifndef CTRE__LITERAL_EXTRACTION_WITH_EXPANSION__HPP
#define CTRE__LITERAL_EXTRACTION_WITH_EXPANSION__HPP

#include "char_class_expansion.hpp"
#include "multi_literal.hpp"
#include "glushkov_nfa.hpp"
#include "dominator_analysis.hpp"
#include "pattern_traits.hpp"

namespace ctre::extraction {

using namespace ctre::traits;

// Check if a symbol could be from a character class (heuristic)
[[nodiscard]] constexpr bool could_be_char_class(char symbol) noexcept {
    return (symbol != '\0' && symbol != '.' && symbol != '?');
}

// Forward declarations for recursive traversal
template <typename T, size_t MaxLiterals, size_t MaxLiteralLen>
constexpr void extract_literals_from_ast_impl(
    multi_literal_result<MaxLiterals, MaxLiteralLen>& result,
    literal_result<MaxLiteralLen>& current,
    size_t& depth);

// Single character
template <auto V, size_t MaxLiterals, size_t MaxLiteralLen>
constexpr void extract_from_character(
    multi_literal_result<MaxLiterals, MaxLiteralLen>& result,
    literal_result<MaxLiteralLen>& current,
    character<V>*)
{
    current.add_char(static_cast<char>(V));
}

// Character class - expand if small
template <typename CharClass, size_t MaxLiterals, size_t MaxLiteralLen>
constexpr void extract_from_char_class(
    multi_literal_result<MaxLiterals, MaxLiteralLen>& result,
    literal_result<MaxLiteralLen>& current,
    CharClass*)
{
    // Check if expandable
    if constexpr (is_expandable_char_class<CharClass>()) {
        constexpr auto expanded = expand_char_class<CharClass>();

        // If we have a current literal being built, branch for each expansion
        if (current.length > 0) {
            // For each expanded character, create a new literal path
            for (size_t i = 0; i < expanded.count && result.count < MaxLiterals; ++i) {
                literal_result<MaxLiteralLen> new_path = current;
                new_path.add_char(expanded.chars[i]);
                result.add_literal(new_path);
            }
            // Clear current since we've branched
            current = literal_result<MaxLiteralLen>{};
        } else {
            // No current literal, start multiple new ones
            for (size_t i = 0; i < expanded.count && result.count < MaxLiterals; ++i) {
                literal_result<MaxLiteralLen> new_lit;
                new_lit.add_char(expanded.chars[i]);
                result.add_literal(new_lit);
            }
        }
    } else {
        // Not expandable, can't extract literal from this
        // Terminate current literal if any
        if (current.length > 0) {
            result.add_literal(current);
            current = literal_result<MaxLiteralLen>{};
        }
    }
}

// String - add all characters
template <auto... Str, size_t MaxLiterals, size_t MaxLiteralLen>
constexpr void extract_from_string(
    multi_literal_result<MaxLiterals, MaxLiteralLen>& result,
    literal_result<MaxLiteralLen>& current,
    string<Str...>*)
{
    (current.add_char(static_cast<char>(Str)), ...);
}

// Sequence - process in order
template <typename... Content, size_t MaxLiterals, size_t MaxLiteralLen>
constexpr void extract_from_sequence(
    multi_literal_result<MaxLiterals, MaxLiteralLen>& result,
    literal_result<MaxLiteralLen>& current,
    sequence<Content...>*,
    size_t& depth)
{
    // Process each element in sequence
    (extract_literals_from_ast_impl<Content>(result, current, depth), ...);
}

// Select (alternation) - try each alternative
template <typename... Opts, size_t MaxLiterals, size_t MaxLiteralLen>
constexpr void extract_from_select(
    multi_literal_result<MaxLiterals, MaxLiteralLen>& result,
    literal_result<MaxLiteralLen>& current,
    select<Opts...>*,
    size_t& depth)
{
    // Save current state
    auto saved_current = current;

    // Try each alternative
    ([&]() {
        current = saved_current;  // Reset for each alternative
        extract_literals_from_ast_impl<Opts>(result, current, depth);
    }(), ...);

    // Clear current since we've explored alternatives
    current = literal_result<MaxLiteralLen>{};
}

// Main dispatch
template <typename T, size_t MaxLiterals, size_t MaxLiteralLen>
constexpr void extract_literals_from_ast_impl(
    multi_literal_result<MaxLiterals, MaxLiteralLen>& result,
    literal_result<MaxLiteralLen>& current,
    size_t& depth)
{
    // Prevent infinite recursion
    if (depth++ > 100) return;

    // Dispatch based on type
    if constexpr (is_string_v<T>)
        extract_from_string(result, current, static_cast<T*>(nullptr));
    else if constexpr (is_sequence_v<T>)
        extract_from_sequence(result, current, static_cast<T*>(nullptr), depth);
    else if constexpr (is_select_v<T>)
        extract_from_select(result, current, static_cast<T*>(nullptr), depth);
    else if constexpr (is_character_v<T>)
        extract_from_character(result, current, static_cast<T*>(nullptr));
    else if constexpr (is_expandable_char_class<T>())
        extract_from_char_class<T>(result, current, static_cast<T*>(nullptr));
}

// Main entry point
template <typename AST, size_t MaxLiterals = 16, size_t MaxLiteralLen = 64>
constexpr auto extract_literals_with_expansion() {
    multi_literal_result<MaxLiterals, MaxLiteralLen> result;
    literal_result<MaxLiteralLen> current;
    size_t depth = 0;

    extract_literals_from_ast_impl<AST>(result, current, depth);

    // Add any remaining current literal
    if (current.length > 0) {
        result.add_literal(current);
    }

    return result;
}

} // namespace ctre::extraction

#endif
