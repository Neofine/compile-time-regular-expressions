#ifndef CTRE__CHAR_CLASS_EXPANSION__HPP
#define CTRE__CHAR_CLASS_EXPANSION__HPP

// Forward declarations to avoid circular dependencies
#ifndef CTRE_IN_A_MODULE
#include <array>
#endif

namespace ctre {
    // Forward declarations of character class types from atoms_characters.hpp
    template <auto V> struct character;
    template <auto A, auto B> struct char_range;
    template <typename... Content> struct set;
    template <auto... Cs> struct enumeration;
    template <typename... Content> struct negative_set;
}

namespace ctre {

// Maximum number of characters to expand a character class
// If a class has more characters than this, we don't expand it
// Paper (NSDI'19 Section 3.2): Hyperscan uses ≤11 strings
constexpr size_t MAX_CHAR_CLASS_EXPANSION = 11;

// Result of character class analysis
template <size_t MaxChars>
struct char_class_expansion_result {
    std::array<char, MaxChars> chars{};
    size_t count = 0;
    bool is_expandable = false;
};

// Forward declarations
template <typename T>
constexpr bool is_character_class();

template <typename T>
constexpr size_t count_char_class_size();

template <typename T, size_t MaxChars>
constexpr char_class_expansion_result<MaxChars> expand_char_class();

// ============================================================================
// Type Checkers: Is this a character class?
// ============================================================================

template <typename T>
constexpr bool is_character_class() {
    if constexpr (requires { typename T::is_character_class_tag; }) {
        return true;
    } else {
        return false;
    }
}

// Specializations for known character class types
template <auto V>
constexpr bool is_character_class(character<V>*) { return true; }

template <auto A, auto B>
constexpr bool is_character_class(char_range<A, B>*) { return true; }

template <typename... Content>
constexpr bool is_character_class(set<Content...>*) { return true; }

template <auto... Cs>
constexpr bool is_character_class(enumeration<Cs...>*) { return true; }

template <typename... Content>
constexpr bool is_character_class(negative_set<Content...>*) { return false; } // Don't expand negated sets

// Fallback for unknown types
template <typename T>
constexpr bool is_character_class(T*) { return false; }

// ============================================================================
// Count: How many characters in this class?
// ============================================================================

// Single character
template <auto V>
constexpr size_t count_char_class_size(character<V>*) {
    return 1;
}

// Character range
template <auto A, auto B>
constexpr size_t count_char_class_size(char_range<A, B>*) {
    static_assert(B >= A, "Invalid character range");
    return static_cast<size_t>(B - A) + 1;
}

// Enumeration (list of characters)
template <auto... Cs>
constexpr size_t count_char_class_size(enumeration<Cs...>*) {
    return sizeof...(Cs);
}

// Set (union of character classes)
template <typename... Content>
constexpr size_t count_char_class_size(set<Content...>*) {
    return (count_char_class_size(static_cast<Content*>(nullptr)) + ...);
}

// Negated set (NOT operation on character class)
template <typename... Content>
constexpr size_t count_char_class_size(negative_set<Content...>*) {
    // For negated set, count is (256 - inner_count)
    constexpr size_t inner_count = (count_char_class_size(static_cast<Content*>(nullptr)) + ...);
    return (inner_count > 0) ? (256 - inner_count) : 256;
}

// Fallback
template <typename T>
constexpr size_t count_char_class_size(T*) {
    return 0; // Unknown or unsupported type
}

// Convenience wrapper
template <typename T>
constexpr size_t count_char_class_size() {
    return count_char_class_size(static_cast<T*>(nullptr));
}

// ============================================================================
// Expand: Extract all characters from this class
// ============================================================================

// Helper: Add a single character to result
template <size_t MaxChars>
constexpr void add_char_to_result(char_class_expansion_result<MaxChars>& result, char c) {
    if (result.count < MaxChars) {
        result.chars[result.count++] = c;
    }
}

// Single character
template <auto V, size_t MaxChars>
constexpr void expand_char_class_impl(character<V>*, char_class_expansion_result<MaxChars>& result) {
    add_char_to_result(result, static_cast<char>(V));
}

// Character range
template <auto A, auto B, size_t MaxChars>
constexpr void expand_char_class_impl(char_range<A, B>*, char_class_expansion_result<MaxChars>& result) {
    for (auto c = A; c <= B; ++c) {
        add_char_to_result(result, static_cast<char>(c));
        if (result.count >= MaxChars) break;
    }
}

// Enumeration (list of characters)
template <auto... Cs, size_t MaxChars>
constexpr void expand_char_class_impl(enumeration<Cs...>*, char_class_expansion_result<MaxChars>& result) {
    (add_char_to_result(result, static_cast<char>(Cs)), ...);
}

// Set (union of character classes) - recursively expand each component
template <typename... Content, size_t MaxChars>
constexpr void expand_char_class_impl(set<Content...>*, char_class_expansion_result<MaxChars>& result) {
    (expand_char_class_impl(static_cast<Content*>(nullptr), result), ...);
}

// Main expansion function
template <typename T, size_t MaxChars = MAX_CHAR_CLASS_EXPANSION>
constexpr char_class_expansion_result<MaxChars> expand_char_class() {
    char_class_expansion_result<MaxChars> result;

    // Check if it's a character class
    if constexpr (!is_character_class(static_cast<T*>(nullptr))) {
        result.is_expandable = false;
        return result;
    }

    // Check if it's small enough to expand
    constexpr size_t size = count_char_class_size<T>();
    if constexpr (size == 0 || size > MaxChars) {
        result.is_expandable = false;
        return result;
    }

    // Expand it
    expand_char_class_impl(static_cast<T*>(nullptr), result);
    result.is_expandable = (result.count > 0 && result.count <= MaxChars);

    return result;
}

// ============================================================================
// Check if type is expandable
// ============================================================================

template <typename T>
constexpr bool is_expandable_char_class() {
    if constexpr (!is_character_class(static_cast<T*>(nullptr))) {
        return false;
    }
    constexpr size_t size = count_char_class_size<T>();
    return (size > 0 && size <= MAX_CHAR_CLASS_EXPANSION);
}

} // namespace ctre

#endif
