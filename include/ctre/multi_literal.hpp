#ifndef CTRE__MULTI_LITERAL__HPP
#define CTRE__MULTI_LITERAL__HPP

#include <array>

namespace ctre {

template <size_t MaxLen>
struct literal_result {
    std::array<char, MaxLen> chars{};
    size_t length = 0;
    bool has_literal = false;
    size_t start_position = 0;
    size_t nfa_dominator_length = 0;  // For expansion validation

    constexpr void add_char(char c) noexcept {
        if (length < MaxLen) {
            chars[length++] = c;
            has_literal = true;
        }
    }
};

template <size_t MaxLiterals, size_t MaxLiteralLen>
struct multi_literal_result {
    std::array<literal_result<MaxLiteralLen>, MaxLiterals> literals{};
    size_t count = 0;
    bool has_literals = false;

    constexpr void add_literal(const literal_result<MaxLiteralLen>& lit) noexcept {
        if (count < MaxLiterals && lit.has_literal) {
            literals[count++] = lit;
            has_literals = true;
        }
    }

    [[nodiscard]] constexpr literal_result<MaxLiteralLen> get_longest() const noexcept {
        if (count == 0) return {};
        size_t best_idx = 0;
        for (size_t i = 1; i < count; ++i)
            if (literals[i].length > literals[best_idx].length) best_idx = i;
        return literals[best_idx];
    }

    [[nodiscard]] constexpr literal_result<MaxLiteralLen> get_first() const noexcept {
        return count > 0 ? literals[0] : literal_result<MaxLiteralLen>{};
    }

    [[nodiscard]] constexpr operator literal_result<MaxLiteralLen>() const noexcept {
        return get_longest();
    }
};

} // namespace ctre

#endif // CTRE__MULTI_LITERAL__HPP
