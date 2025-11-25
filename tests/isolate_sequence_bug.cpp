#include <ctre.hpp>
#include <iostream>

template <typename Pattern>
void test_pattern(const char* name) {
    using RawAST = ctre::decomposition::unwrap_regex_t<Pattern>;
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<RawAST>();
    std::cout << name << ": " << nfa.state_count << " states\n";
}

int main() {
    std::cout << "Isolating the bug:\n\n";

    // These should all have 7 states (a,b,c,d,e,f + start)
    test_pattern<decltype(ctre::search<"abc|def">)>("abc|def (no seq)");
    test_pattern<decltype(ctre::search<"(abc|def)">)>("(abc|def) (capture only)");

    // Now add a character AFTER the alternation
    test_pattern<decltype(ctre::match<"abc|defx">)>("abc|defx (alt at top)");  // Should be 8
    test_pattern<decltype(ctre::match<"(abc|def)x">)>("(abc|def)x (capture+seq)");  // Should be 8

    // More complex
    test_pattern<decltype(ctre::match<"x(abc|def)y">)>("x(abc|def)y");  // Should be 9

    return 0;
}
