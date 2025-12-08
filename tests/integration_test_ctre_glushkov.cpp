#include <ctre.hpp>
#include <iostream>
#include <string>
#include <cassert>

int tests_passed = 0;
int tests_failed = 0;

#define TEST(name) \
    std::cout << "  " << #name << "... "; \
    if (test_##name()) { tests_passed++; std::cout << "PASSED\n"; } \
    else { tests_failed++; std::cout << "FAILED\n"; }

bool test_simple_pattern_construction() {
    constexpr auto pattern = ctre::search<"hello">;
    using Pattern = decltype(pattern);
    using AST = ctre::decomposition::unwrap_regex_t<Pattern>;
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<AST>();

    return nfa.state_count > 0 && pattern("hello world") && !pattern("goodbye");
}

bool test_alternation_nfa() {
    constexpr auto pattern = ctre::search<"(foo|bar)">;
    using Pattern = decltype(pattern);
    using AST = ctre::decomposition::unwrap_regex_t<Pattern>;
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<AST>();

    return nfa.state_count > 1 && pattern("foo") && pattern("bar") && !pattern("baz");
}

bool test_literal_extraction_from_nfa() {
    constexpr auto pattern = ctre::search<"prefix_hello_suffix">;
    using Pattern = decltype(pattern);
    using AST = ctre::decomposition::unwrap_regex_t<Pattern>;
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<AST>();
    constexpr auto literal = ctre::dominators::extract_literal_from_dominators(nfa);

    return literal.has_literal;
}

bool test_decomposition_uses_nfa() {
    using Pattern = decltype(ctre::search<"test_pattern">);
    using AST = ctre::decomposition::unwrap_regex_t<Pattern>;
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<AST>();
    constexpr auto literal = ctre::dominators::extract_literal<AST>();

    return nfa.state_count > 0 && literal.has_literal;
}

bool test_complex_pattern_with_repeats() {
    constexpr auto pattern = ctre::search<"a.*b.*c">;
    using Pattern = decltype(pattern);
    using AST = ctre::decomposition::unwrap_regex_t<Pattern>;
    [[maybe_unused]] constexpr auto nfa = ctre::glushkov::glushkov_nfa<AST>();

    return pattern("a_b_c") && pattern("axxxxxbxxxxxc") && !pattern("cab");
}

bool test_nfa_state_transitions() {
    using Pattern = ctre::string<'a', 'b'>;
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<Pattern>();

    return nfa.state_count == 3 &&
           nfa.accept_count == 1 &&
           nfa.states[1].symbol == 'a' &&
           nfa.states[2].symbol == 'b';
}

bool test_search_with_literal_prefiltering() {
    constexpr auto pattern = ctre::search<"test_literal">;
    const char* text = "xxxxxxtest_literalxxxxxx";
    auto result = pattern(text);
    if (!result) return false;

    std::string matched(result.to_view().begin(), result.to_view().end());
    return matched == "test_literal";
}

bool test_match_with_nfa() {
    constexpr auto pattern = ctre::match<"hello">;
    using Pattern = decltype(pattern);
    using AST = ctre::decomposition::unwrap_regex_t<Pattern>;
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<AST>();

    return nfa.state_count > 0 && pattern("hello") && !pattern("hello world");
}

bool test_leading_dot_star_patterns() {
    constexpr auto pattern = ctre::search<".*(hello|world).*test">;
    return pattern("hello world test") && pattern("world test") && !pattern("test");
}

bool test_real_world_url_pattern() {
    constexpr auto pattern = ctre::search<"(http|https)://[a-z]+\\.[a-z]+">;
    using Pattern = decltype(pattern);
    using AST = ctre::decomposition::unwrap_regex_t<Pattern>;
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<AST>();

    return nfa.state_count > 5 &&
           pattern("http://example.com") &&
           pattern("https://test.org") &&
           !pattern("ftp://server.net");
}

int main() {
    std::cout << "=== Glushkov NFA Integration Tests ===\n\n";

    TEST(simple_pattern_construction);
    TEST(alternation_nfa);
    TEST(literal_extraction_from_nfa);
    TEST(decomposition_uses_nfa);
    TEST(complex_pattern_with_repeats);
    TEST(nfa_state_transitions);
    TEST(search_with_literal_prefiltering);
    TEST(match_with_nfa);
    TEST(leading_dot_star_patterns);
    TEST(real_world_url_pattern);

    std::cout << "\nPassed: " << tests_passed << "/" << (tests_passed + tests_failed) << "\n";
    return tests_failed > 0 ? 1 : 0;
}
