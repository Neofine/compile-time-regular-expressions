#include <ctre.hpp>
#include <iostream>
#include <string_view>

using namespace std::string_view_literals;

// Helper to print literal_result
template <size_t N>
void print_literal(const auto& lit, const char* name, bool has_nfa_field = true) {
    std::cout << name << ":\n";
    std::cout << "  has_literal: " << lit.has_literal << "\n";
    std::cout << "  length: " << lit.length << "\n";
    if (has_nfa_field) {
        if constexpr (requires { lit.nfa_dominator_length; }) {
            std::cout << "  nfa_dominator_length: " << lit.nfa_dominator_length << "\n";
        }
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

    std::cout << "=== Pattern: doc[il1]ment ===\n";
    {
        using Pattern = decltype(ctre::search<"doc[il1]ment">);
        using RawAST = decomposition::unwrap_regex_t<Pattern>;

        // Check NFA result
        constexpr auto nfa = glushkov::glushkov_nfa<RawAST>();
        constexpr auto nfa_result = dominators::extract_literal_from_dominators(nfa);
        print_literal<64>(nfa_result, "NFA Result");

        // Check multi-literal expansion
        constexpr auto multi_result = extraction::extract_literals_simple_multi<RawAST>();
        std::cout << "Multi-literal expansion:\n";
        std::cout << "  has_literals: " << multi_result.has_literals << "\n";
        std::cout << "  count: " << multi_result.count << "\n";
        if constexpr (multi_result.has_literals && multi_result.count > 0) {
            constexpr auto longest = multi_result.get_longest();
            print_literal<32>(longest, "Longest from expansion");
        }

        // Check final result
        constexpr auto final_result = decomposition::extract_literal_with_expansion_and_fallback<Pattern>();
        print_literal<64>(final_result, "Final Result");
    }

    std::cout << "\n=== Pattern: [0-3]test ===\n";
    {
        using Pattern = decltype(ctre::search<"[0-3]test">);
        using RawAST = decomposition::unwrap_regex_t<Pattern>;

        // Check NFA result
        constexpr auto nfa = glushkov::glushkov_nfa<RawAST>();
        constexpr auto nfa_result = dominators::extract_literal_from_dominators(nfa);
        print_literal<64>(nfa_result, "NFA Result");

        // Check multi-literal expansion
        constexpr auto multi_result = extraction::extract_literals_simple_multi<RawAST>();
        std::cout << "Multi-literal expansion:\n";
        std::cout << "  has_literals: " << multi_result.has_literals << "\n";
        std::cout << "  count: " << multi_result.count << "\n";
        if constexpr (multi_result.has_literals && multi_result.count > 0) {
            constexpr auto longest = multi_result.get_longest();
            print_literal<32>(longest, "Longest from expansion");
        }

        // Check final result
        constexpr auto final_result = decomposition::extract_literal_with_expansion_and_fallback<Pattern>();
        print_literal<64>(final_result, "Final Result");
    }

    std::cout << "\n=== Pattern: test[0-3] ===\n";
    {
        using Pattern = decltype(ctre::search<"test[0-3]">);
        using RawAST = decomposition::unwrap_regex_t<Pattern>;

        // Check NFA result
        constexpr auto nfa = glushkov::glushkov_nfa<RawAST>();
        constexpr auto nfa_result = dominators::extract_literal_from_dominators(nfa);
        print_literal<64>(nfa_result, "NFA Result");

        // Check multi-literal expansion
        constexpr auto multi_result = extraction::extract_literals_simple_multi<RawAST>();
        std::cout << "Multi-literal expansion:\n";
        std::cout << "  has_literals: " << multi_result.has_literals << "\n";
        std::cout << "  count: " << multi_result.count << "\n";
        if constexpr (multi_result.has_literals && multi_result.count > 0) {
            constexpr auto longest = multi_result.get_longest();
            print_literal<32>(longest, "Longest from expansion");
        }

        // Check final result
        constexpr auto final_result = decomposition::extract_literal_with_expansion_and_fallback<Pattern>();
        print_literal<64>(final_result, "Final Result");
    }

    return 0;
}
