#include <ctre.hpp>
#include <iostream>

template <typename Pattern>
void test_nfa_construction(const char* name, const char* regex_str) {
    using RawAST = ctre::decomposition::unwrap_regex_t<Pattern>;
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<RawAST>();
    std::cout << "  " << name << " (" << regex_str << "): " << nfa.state_count << " states\n";
}

int main() {
    std::cout << "=== Edge Case Tests ===\n\n";

    std::cout << "Basic types:\n";
    test_nfa_construction<decltype(ctre::match<"abc">)>("String", "abc");
    test_nfa_construction<decltype(ctre::match<".">)>("Any", ".");
    test_nfa_construction<decltype(ctre::match<"a*">)>("Star", "a*");
    test_nfa_construction<decltype(ctre::match<"a+">)>("Plus", "a+");
    test_nfa_construction<decltype(ctre::match<"a?">)>("Question", "a?");
    test_nfa_construction<decltype(ctre::match<"a{2,5}">)>("Bounded", "a{2,5}");

    std::cout << "\nCaptures:\n";
    test_nfa_construction<decltype(ctre::match<"(abc)">)>("Capture", "(abc)");
    test_nfa_construction<decltype(ctre::match<"(abc|def)">)>("Capture+Alt", "(abc|def)");
    test_nfa_construction<decltype(ctre::match<"(abc)ghi">)>("Capture+Seq", "(abc)ghi");
    test_nfa_construction<decltype(ctre::match<"(a)(b)(c)">)>("Multi-Capture", "(a)(b)(c)");

    std::cout << "\nAlternations:\n";
    test_nfa_construction<decltype(ctre::match<"abc|def">)>("Alternation", "abc|def");
    test_nfa_construction<decltype(ctre::match<"(abc|def|ghi)">)>("3-way Alt", "(abc|def|ghi)");

    std::cout << "\nCharacter classes:\n";
    test_nfa_construction<decltype(ctre::match<"[a-z]">)>("Range", "[a-z]");
    test_nfa_construction<decltype(ctre::match<"[0-3]">)>("Small range", "[0-3]");
    test_nfa_construction<decltype(ctre::match<"[abc]">)>("Enumeration", "[abc]");

    std::cout << "\nComplex patterns:\n";
    test_nfa_construction<decltype(ctre::match<"(abc|def).*ghi">)>("Paper pattern", "(abc|def).*ghi");
    test_nfa_construction<decltype(ctre::match<"a.*b.*c">)>("Multi-.*", "a.*b.*c");
    test_nfa_construction<decltype(ctre::match<"^abc$">)>("Anchored", "^abc$");
    test_nfa_construction<decltype(ctre::match<R"(\bword\b)">)>("Word boundary", R"(\bword\b)");

    std::cout << "\nAll edge cases compiled successfully.\n";
    return 0;
}
