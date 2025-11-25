#include <ctre.hpp>
#include <iostream>
#include <string_view>

using namespace std::string_view_literals;

// Helper to print literal_result
template <size_t N>
void print_literal(const auto& lit, const char* name) {
    std::cout << name << ":\n";
    std::cout << "  has_literal: " << lit.has_literal << "\n";
    std::cout << "  length: " << lit.length << "\n";
    if constexpr (requires { lit.nfa_dominator_length; }) {
        std::cout << "  nfa_dominator_length: " << lit.nfa_dominator_length << "\n";
    }
    if (lit.has_literal && lit.length > 0) {
        std::cout << "  chars: ";
        for (size_t i = 0; i < lit.length; ++i) {
            std::cout << lit.chars[i];
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

int main() {
    using namespace ctre;

    std::cout << "=== Pattern: [a-c]+ ===\n";
    {
        using Pattern = decltype(ctre::search<"[a-c]+">);
        constexpr auto final_result = decomposition::extract_literal_with_expansion_and_fallback<Pattern>();
        print_literal<64>(final_result, "Final Result");
    }

    std::cout << "\n=== Pattern: [x-z]+ ===\n";
    {
        using Pattern = decltype(ctre::search<"[x-z]+">);
        constexpr auto final_result = decomposition::extract_literal_with_expansion_and_fallback<Pattern>();
        print_literal<64>(final_result, "Final Result");
    }

    std::cout << "\n=== Pattern: [a-z]+ ===\n";
    {
        using Pattern = decltype(ctre::search<"[a-z]+">);
        constexpr auto final_result = decomposition::extract_literal_with_expansion_and_fallback<Pattern>();
        print_literal<64>(final_result, "Final Result");
    }

    std::cout << "\n=== Pattern: [a-c]* ===\n";
    {
        using Pattern = decltype(ctre::search<"[a-c]*">);
        constexpr auto final_result = decomposition::extract_literal_with_expansion_and_fallback<Pattern>();
        print_literal<64>(final_result, "Final Result");
    }

    return 0;
}
