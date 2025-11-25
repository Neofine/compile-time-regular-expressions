#ifndef CTRE__LITERAL_EXTRACTION_SIMPLE_MULTI__HPP
#define CTRE__LITERAL_EXTRACTION_SIMPLE_MULTI__HPP

#include "char_class_expansion.hpp"
#include "multi_literal.hpp"

namespace ctre {
namespace extraction {

// ============================================================================
// Simple Multi-Path Extraction
// ============================================================================
//
// Key idea: Track multiple "active" literals simultaneously
// When we encounter a char class, multiply paths
// When we encounter a regular char, add to ALL paths
//
// ============================================================================

// Active paths tracker
template <size_t MaxPaths, size_t MaxLiteralLen>
struct active_paths {
    std::array<literal_result<MaxLiteralLen>, MaxPaths> paths;
    size_t count = 0;

    // Initialize with one empty path
    constexpr active_paths() {
        paths[0] = literal_result<MaxLiteralLen>{};
        count = 1;
    }

    // Add a character to ALL active paths
    constexpr void add_char_to_all(char c) {
        for (size_t i = 0; i < count; ++i) {
            paths[i].add_char(c);
        }
    }

    // Multiply paths: for each current path, create N variations (one per char)
    constexpr void multiply_paths(const char* chars, size_t char_count) {
        if (char_count == 0) return;

        // Calculate new path count
        size_t new_count = count * char_count;
        if (new_count > MaxPaths) {
            // Too many paths, just take the first char for all paths
            add_char_to_all(chars[0]);
            return;
        }

        // Create new paths array
        std::array<literal_result<MaxLiteralLen>, MaxPaths> new_paths;
        size_t new_idx = 0;

        // For each existing path
        for (size_t i = 0; i < count; ++i) {
            // Create variation for each character
            for (size_t j = 0; j < char_count && new_idx < MaxPaths; ++j) {
                new_paths[new_idx] = paths[i];  // Copy existing path
                new_paths[new_idx].add_char(chars[j]);  // Add this variation
                ++new_idx;
            }
        }

        // Replace paths
        paths = new_paths;
        count = new_idx;
    }

    // Finalize: copy all paths to result
    template <size_t MaxLiterals>
    constexpr void finalize_to(multi_literal_result<MaxLiterals, MaxLiteralLen>& result) {
        for (size_t i = 0; i < count && i < MaxLiterals; ++i) {
            if (paths[i].length > 0) {
                result.add_literal(paths[i]);
            }
        }
    }
};

// ============================================================================
// Type-based extraction (recursive)
// ============================================================================

// Forward declaration
template <typename T, size_t MaxPaths, size_t MaxLiteralLen>
constexpr void extract_simple(active_paths<MaxPaths, MaxLiteralLen>& paths, size_t& depth);

// Single character - using pointer-based dispatch
template <auto V, size_t MaxPaths, size_t MaxLiteralLen>
constexpr void extract_character(active_paths<MaxPaths, MaxLiteralLen>& paths, character<V>*) {
    paths.add_char_to_all(static_cast<char>(V));
}

// Character class (expandable)
template <typename CharClass, size_t MaxPaths, size_t MaxLiteralLen>
constexpr void extract_char_class(active_paths<MaxPaths, MaxLiteralLen>& paths) {
    if constexpr (is_expandable_char_class<CharClass>()) {
        constexpr auto expanded = expand_char_class<CharClass>();
        paths.multiply_paths(expanded.chars.data(), expanded.count);
    }
    // If not expandable, we can't extract - leave paths as-is (will terminate)
}

// String - using pointer-based dispatch
template <auto... Str, size_t MaxPaths, size_t MaxLiteralLen>
constexpr void extract_string(active_paths<MaxPaths, MaxLiteralLen>& paths, string<Str...>*) {
    (paths.add_char_to_all(static_cast<char>(Str)), ...);
}

// Sequence - process each element in order
template <typename... Content, size_t MaxPaths, size_t MaxLiteralLen>
constexpr void extract_sequence(active_paths<MaxPaths, MaxLiteralLen>& paths, size_t& depth, sequence<Content...>*) {
    (extract_simple<Content>(paths, depth), ...);
}

// Select (alternation) - need to try each branch separately, then combine
template <typename... Opts, size_t MaxPaths, size_t MaxLiteralLen>
constexpr void extract_select(active_paths<MaxPaths, MaxLiteralLen>& paths, size_t& depth, select<Opts...>*) {
    // Storage for all branches
    active_paths<MaxPaths, MaxLiteralLen> all_branches;
    all_branches.count = 0;

    // Process each alternative
    ([&]() {
        // Start fresh for this alternative
        active_paths<MaxPaths, MaxLiteralLen> branch_paths;

        // Extract from this alternative
        extract_simple<Opts>(branch_paths, depth);

        // Add all paths from this branch to combined result
        for (size_t i = 0; i < branch_paths.count && all_branches.count < MaxPaths; ++i) {
            all_branches.paths[all_branches.count++] = branch_paths.paths[i];
        }
    }(), ...);

    // Replace paths with combined branches
    paths = all_branches;
}

// ============================================================================
// Capture extraction (unwrap and delegate)
// ============================================================================

// Single content in capture
template <size_t Index, typename Content, size_t MaxPaths, size_t MaxLiteralLen>
constexpr void extract_capture(active_paths<MaxPaths, MaxLiteralLen>& paths, size_t& depth, capture<Index, Content>*) {
    extract_simple<Content>(paths, depth);
}

// Multiple content in capture (treat as sequence)
template <size_t Index, typename First, typename... Rest, size_t MaxPaths, size_t MaxLiteralLen>
constexpr void extract_capture(active_paths<MaxPaths, MaxLiteralLen>& paths, size_t& depth, capture<Index, First, Rest...>*) {
    extract_simple<sequence<First, Rest...>>(paths, depth);
}

// Type checkers
template <typename T> struct is_string_t : std::false_type {};
template <auto... Str> struct is_string_t<string<Str...>> : std::true_type {};

template <typename T> struct is_sequence_t : std::false_type {};
template <typename... C> struct is_sequence_t<sequence<C...>> : std::true_type {};

template <typename T> struct is_select_t : std::false_type {};
template <typename... O> struct is_select_t<select<O...>> : std::true_type {};

template <typename T> struct is_character_t : std::false_type {};
template <auto V> struct is_character_t<character<V>> : std::true_type {};

template <typename T> struct is_capture_t : std::false_type {};
template <size_t Index, typename... Content> struct is_capture_t<capture<Index, Content...>> : std::true_type {};

// Main dispatcher
template <typename T, size_t MaxPaths, size_t MaxLiteralLen>
constexpr void extract_simple(active_paths<MaxPaths, MaxLiteralLen>& paths, size_t& depth) {
    // Prevent infinite recursion
    if (depth++ > 100) return;

    if constexpr (is_capture_t<T>::value) {
        extract_capture(paths, depth, static_cast<T*>(nullptr));
    } else if constexpr (is_string_t<T>::value) {
        extract_string(paths, static_cast<T*>(nullptr));
    } else if constexpr (is_sequence_t<T>::value) {
        extract_sequence(paths, depth, static_cast<T*>(nullptr));
    } else if constexpr (is_select_t<T>::value) {
        extract_select(paths, depth, static_cast<T*>(nullptr));
    } else if constexpr (is_character_t<T>::value) {
        extract_character(paths, static_cast<T*>(nullptr));
    } else if constexpr (is_expandable_char_class<T>()) {
        extract_char_class<T>(paths);
    }
    // For anything else (repeats, etc.), stop extraction
}

// Note: All dispatch happens through the type checkers above
// No need for duplicate specialization helpers

// ============================================================================
// Main entry point
// ============================================================================

template <typename AST, size_t MaxLiterals = 16, size_t MaxLiteralLen = 64>
constexpr auto extract_literals_simple_multi() {
    active_paths<MaxLiterals, MaxLiteralLen> paths;
    size_t depth = 0;

    extract_simple<AST>(paths, depth);

    multi_literal_result<MaxLiterals, MaxLiteralLen> result;
    paths.finalize_to(result);

    return result;
}

} // namespace extraction
} // namespace ctre

#endif
