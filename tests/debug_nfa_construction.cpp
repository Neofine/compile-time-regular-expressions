#include <ctre.hpp>
#include <iostream>

template <typename Pattern>
void print_nfa_structure(const char* name) {
    using RawAST = ctre::decomposition::unwrap_regex_t<Pattern>;
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<RawAST>();

    std::cout << "\n=== " << name << " ===\n";
    std::cout << "State count: " << nfa.state_count << "\n";
    std::cout << "Accept count: " << nfa.accept_count << "\n";

    for (size_t i = 0; i < nfa.state_count; ++i) {
        std::cout << "State " << i << ": ";
        std::cout << "symbol='" << (nfa.states[i].symbol ? nfa.states[i].symbol : ' ') << "'";
        std::cout << " successors=" << nfa.states[i].successor_count;
        if (nfa.states[i].successor_count > 0) {
            std::cout << " [";
            for (size_t j = 0; j < nfa.states[i].successor_count; ++j) {
                std::cout << nfa.states[i].successors[j];
                if (j + 1 < nfa.states[i].successor_count) std::cout << ", ";
            }
            std::cout << "]";
        }
        std::cout << "\n";
    }
}

int main() {
    std::cout << "NFA Construction Debug\n";
    std::cout << "======================\n";

    // Test 1: Simple string
    print_nfa_structure<decltype(ctre::search<"abc">)>("abc");

    // Test 2: Simple alternation WITHOUT capture
    print_nfa_structure<decltype(ctre::search<"abc|def">)>("abc|def");

    // Test 3: Simple alternation WITH capture
    print_nfa_structure<decltype(ctre::search<"(abc|def)">)>("(abc|def)");

    // Test 4: Sequence with alternation
    print_nfa_structure<decltype(ctre::search<"(abc|def)ghi">)>("(abc|def)ghi");

    // Test 5: Full pattern from paper
    print_nfa_structure<decltype(ctre::search<"(abc|def).*ghi">)>("(abc|def).*ghi");

    // Test 6: Without capture, without .*
    print_nfa_structure<decltype(ctre::search<"abc|defghi">)>("abc|defghi");

    return 0;
}
