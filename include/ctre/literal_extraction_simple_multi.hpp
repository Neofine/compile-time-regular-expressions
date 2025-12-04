#ifndef CTRE__LITERAL_EXTRACTION_SIMPLE_MULTI__HPP
#define CTRE__LITERAL_EXTRACTION_SIMPLE_MULTI__HPP

#include "char_class_expansion.hpp"
#include "multi_literal.hpp"
#include "pattern_traits.hpp"

namespace ctre::extraction {

using namespace ctre::traits;

// Active paths tracker - track multiple "active" literals simultaneously
template <size_t MaxPaths, size_t MaxLiteralLen>
struct active_paths {
    std::array<literal_result<MaxLiteralLen>, MaxPaths> paths{};
    size_t count = 0;

    constexpr active_paths() noexcept { paths[0] = literal_result<MaxLiteralLen>{}; count = 1; }

    constexpr void add_char_to_all(char c) noexcept {
        for (size_t i = 0; i < count; ++i) paths[i].add_char(c);
    }

    constexpr void multiply_paths(const char* chars, size_t char_count) noexcept {
        if (char_count == 0) return;
        size_t new_count = count * char_count;
        if (new_count > MaxPaths) { add_char_to_all(chars[0]); return; }

        std::array<literal_result<MaxLiteralLen>, MaxPaths> new_paths{};
        size_t new_idx = 0;
        for (size_t i = 0; i < count; ++i) {
            for (size_t j = 0; j < char_count && new_idx < MaxPaths; ++j) {
                new_paths[new_idx] = paths[i];
                new_paths[new_idx].add_char(chars[j]);
                ++new_idx;
            }
        }
        paths = new_paths;
        count = new_idx;
    }

    template <size_t MaxLiterals>
    constexpr void finalize_to(multi_literal_result<MaxLiterals, MaxLiteralLen>& result) const noexcept {
        for (size_t i = 0; i < count && i < MaxLiterals; ++i)
            if (paths[i].length > 0) result.add_literal(paths[i]);
    }
};

// Forward declaration
template <typename T, size_t MaxPaths, size_t MaxLiteralLen>
constexpr void extract_simple(active_paths<MaxPaths, MaxLiteralLen>& paths, size_t& depth) noexcept;

// Extraction helpers
template <auto V, size_t MaxPaths, size_t MaxLiteralLen>
constexpr void extract_character(active_paths<MaxPaths, MaxLiteralLen>& paths, character<V>*) noexcept {
    paths.add_char_to_all(static_cast<char>(V));
}

template <typename CharClass, size_t MaxPaths, size_t MaxLiteralLen>
constexpr void extract_char_class(active_paths<MaxPaths, MaxLiteralLen>& paths) noexcept {
    if constexpr (is_expandable_char_class<CharClass>()) {
        constexpr auto expanded = expand_char_class<CharClass>();
        paths.multiply_paths(expanded.chars.data(), expanded.count);
    }
}

template <auto... Str, size_t MaxPaths, size_t MaxLiteralLen>
constexpr void extract_string(active_paths<MaxPaths, MaxLiteralLen>& paths, string<Str...>*) noexcept {
    (paths.add_char_to_all(static_cast<char>(Str)), ...);
}

template <typename... Content, size_t MaxPaths, size_t MaxLiteralLen>
constexpr void extract_sequence(active_paths<MaxPaths, MaxLiteralLen>& paths, size_t& depth, sequence<Content...>*) noexcept {
    (extract_simple<Content>(paths, depth), ...);
}

template <typename... Opts, size_t MaxPaths, size_t MaxLiteralLen>
constexpr void extract_select(active_paths<MaxPaths, MaxLiteralLen>& paths, size_t& depth, select<Opts...>*) noexcept {
    active_paths<MaxPaths, MaxLiteralLen> all_branches;
    all_branches.count = 0;
    ([&]() {
        active_paths<MaxPaths, MaxLiteralLen> branch_paths;
        extract_simple<Opts>(branch_paths, depth);
        for (size_t i = 0; i < branch_paths.count && all_branches.count < MaxPaths; ++i)
            all_branches.paths[all_branches.count++] = branch_paths.paths[i];
    }(), ...);
    paths = all_branches;
}

template <size_t Index, typename Content, size_t MaxPaths, size_t MaxLiteralLen>
constexpr void extract_capture(active_paths<MaxPaths, MaxLiteralLen>& paths, size_t& depth, capture<Index, Content>*) noexcept {
    extract_simple<Content>(paths, depth);
}

template <size_t Index, typename First, typename... Rest, size_t MaxPaths, size_t MaxLiteralLen>
constexpr void extract_capture(active_paths<MaxPaths, MaxLiteralLen>& paths, size_t& depth, capture<Index, First, Rest...>*) noexcept {
    extract_simple<sequence<First, Rest...>>(paths, depth);
}

// Main dispatcher
template <typename T, size_t MaxPaths, size_t MaxLiteralLen>
constexpr void extract_simple(active_paths<MaxPaths, MaxLiteralLen>& paths, size_t& depth) noexcept {
    if (depth++ > 100) return;
    if constexpr (is_capture_v<T>)
        extract_capture(paths, depth, static_cast<T*>(nullptr));
    else if constexpr (is_string_v<T>)
        extract_string(paths, static_cast<T*>(nullptr));
    else if constexpr (is_sequence_v<T>)
        extract_sequence(paths, depth, static_cast<T*>(nullptr));
    else if constexpr (is_select_v<T>)
        extract_select(paths, depth, static_cast<T*>(nullptr));
    else if constexpr (is_character_v<T>)
        extract_character(paths, static_cast<T*>(nullptr));
    else if constexpr (is_expandable_char_class<T>())
        extract_char_class<T>(paths);
}

// Public API
template <typename AST, size_t MaxLiterals = 16, size_t MaxLiteralLen = 64>
[[nodiscard]] constexpr auto extract_literals_simple_multi() noexcept {
    active_paths<MaxLiterals, MaxLiteralLen> paths;
    size_t depth = 0;
    extract_simple<AST>(paths, depth);
    multi_literal_result<MaxLiterals, MaxLiteralLen> result;
    paths.finalize_to(result);
    return result;
}

} // namespace ctre::extraction

#endif // CTRE__LITERAL_EXTRACTION_SIMPLE_MULTI__HPP
