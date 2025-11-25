#ifndef CTRE__MULTI_LITERAL__HPP
#define CTRE__MULTI_LITERAL__HPP

#include <array>

namespace ctre {

// Single literal result (already exists in dominator_analysis.hpp)
// Reproduced here for reference
template <size_t MaxLen>
struct literal_result {
    std::array<char, MaxLen> chars{};
    size_t length = 0;
    size_t start_position = 0;
    bool has_literal = false;

    constexpr void add_char(char c) {
        if (length < MaxLen) {
            chars[length++] = c;
            has_literal = true;
        }
    }
};

// Multiple literals result - for character class expansion
template <size_t MaxLiterals, size_t MaxLiteralLen>
struct multi_literal_result {
    std::array<literal_result<MaxLiteralLen>, MaxLiterals> literals;
    size_t count = 0;
    bool has_literals = false;

    // Add a new literal
    constexpr void add_literal(const literal_result<MaxLiteralLen>& lit) {
        if (count < MaxLiterals && lit.has_literal) {
            literals[count++] = lit;
            has_literals = true;
        }
    }

    // Get the longest literal
    constexpr literal_result<MaxLiteralLen> get_longest() const {
        if (count == 0) return literal_result<MaxLiteralLen>{};

        size_t best_idx = 0;
        for (size_t i = 1; i < count; ++i) {
            if (literals[i].length > literals[best_idx].length) {
                best_idx = i;
            }
        }
        return literals[best_idx];
    }

    // Get the first literal (for simple cases)
    constexpr literal_result<MaxLiteralLen> get_first() const {
        if (count > 0) return literals[0];
        return literal_result<MaxLiteralLen>{};
    }

    // Convert to single literal (for backward compatibility)
    constexpr operator literal_result<MaxLiteralLen>() const {
        return get_longest();
    }
};

} // namespace ctre

#endif
