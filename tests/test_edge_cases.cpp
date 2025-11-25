#include <ctre.hpp>
#include <iostream>

template <typename Pattern>
void test_nfa_construction(const char* name, const char* regex_str) {
    using RawAST = ctre::decomposition::unwrap_regex_t<Pattern>;

    // Try to construct NFA
    try {
        constexpr auto nfa = ctre::glushkov::glushkov_nfa<RawAST>();
        std::cout << "✓ " << name << " (" << regex_str << "): "
                  << nfa.state_count << " states\n";
    } catch (...) {
        std::cout << "✗ " << name << " (" << regex_str << "): FAILED\n";
    }
}

int main() {
    std::cout << "=== Edge Case Testing ===\n\n";

    // Basic types (should all work)
    test_nfa_construction<decltype(ctre::match<"abc">)>("String", "abc");
    test_nfa_construction<decltype(ctre::match<".">)>("Any", ".");
    test_nfa_construction<decltype(ctre::match<"a*">)>("Star", "a*");
    test_nfa_construction<decltype(ctre::match<"a+">)>("Plus", "a+");
    test_nfa_construction<decltype(ctre::match<"a?">)>("Question", "a?");
    test_nfa_construction<decltype(ctre::match<"a{2,5}">)>("Bounded", "a{2,5}");

    // Captures (recently fixed)
    test_nfa_construction<decltype(ctre::match<"(abc)">)>("Capture", "(abc)");
    test_nfa_construction<decltype(ctre::match<"(abc|def)">)>("Capture+Alt", "(abc|def)");
    test_nfa_construction<decltype(ctre::match<"(abc)ghi">)>("Capture+Seq", "(abc)ghi");
    test_nfa_construction<decltype(ctre::match<"(a)(b)(c)">)>("Multi-Capture", "(a)(b)(c)");

    // Alternations
    test_nfa_construction<decltype(ctre::match<"abc|def">)>("Alternation", "abc|def");
    test_nfa_construction<decltype(ctre::match<"(abc|def|ghi)">)>("3-way Alt", "(abc|def|ghi)");

    // Character classes
    test_nfa_construction<decltype(ctre::match<"[a-z]">)>("Range", "[a-z]");
    test_nfa_construction<decltype(ctre::match<"[0-3]">)>("Small range", "[0-3]");
    test_nfa_construction<decltype(ctre::match<"[abc]">)>("Enumeration", "[abc]");

    // Complex patterns
    test_nfa_construction<decltype(ctre::match<"(abc|def).*ghi">)>("Paper pattern", "(abc|def).*ghi");
    test_nfa_construction<decltype(ctre::match<"a.*b.*c">)>("Multi-.*", "a.*b.*c");
    test_nfa_construction<decltype(ctre::match<"^abc$">)>("Anchored", "^abc$");

    // Boundaries (might not extract well but should not crash)
    test_nfa_construction<decltype(ctre::match<R"(\bword\b)">)>("Word boundary", R"(\bword\b)");

    std::cout << "\n=== All Edge Cases Tested ===\n";

    return 0;
}
