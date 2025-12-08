#include <ctre.hpp>
#include <iostream>
#include <cassert>
#include <cstring>

int tests_passed = 0;
int tests_failed = 0;

#define TEST(name) \
    std::cout << "  " << #name << "... "; \
    if (test_##name()) { tests_passed++; std::cout << "PASSED\n"; } \
    else { tests_failed++; std::cout << "FAILED\n"; }

bool check_literal(const auto& lit, const char* expected, size_t expected_len) {
    if (!lit.has_literal) return false;
    if (lit.length != expected_len) return false;
    return std::memcmp(lit.chars.data(), expected, expected_len) == 0;
}

bool test_simple_literal() {
    using Pattern = decltype(ctre::search<"hello">);
    using AST = ctre::decomposition::unwrap_regex_t<Pattern>;
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<AST>();
    constexpr auto path_lit = ctre::dominators::extract_literal_from_dominators(nfa);
    return check_literal(path_lit, "hello", 5);
}

bool test_alternation_extraction() {
    using Pattern = decltype(ctre::search<"(foo|bar)">);
    using AST = ctre::decomposition::unwrap_regex_t<Pattern>;
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<AST>();

    // Path analysis should fail (no common path)
    constexpr auto path_lit = ctre::dominators::extract_literal_from_dominators(nfa);
    if (path_lit.has_literal) return false;

    // Region analysis should extract "foo" or "bar"
    auto region_lit = ctre::region::extract_literal_from_regions(nfa);
    if (!region_lit.has_literal || region_lit.length != 3) return false;

    bool is_foo = std::memcmp(region_lit.chars.data(), "foo", 3) == 0;
    bool is_bar = std::memcmp(region_lit.chars.data(), "bar", 3) == 0;
    return is_foo || is_bar;
}

bool test_leading_dot_star() {
    using Pattern = decltype(ctre::search<".*(hello|world).*test">);
    using AST = ctre::decomposition::unwrap_regex_t<Pattern>;
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<AST>();
    constexpr auto path_lit = ctre::dominators::extract_literal_from_dominators(nfa);
    return path_lit.has_literal && check_literal(path_lit, "test", 4);
}

bool test_complex_alternation() {
    using Pattern = decltype(ctre::search<"(http|https|ftp)://">);
    using AST = ctre::decomposition::unwrap_regex_t<Pattern>;
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<AST>();
    constexpr auto path_lit = ctre::dominators::extract_literal_from_dominators(nfa);
    auto region_lit = ctre::region::extract_literal_from_regions(nfa);
    return region_lit.has_literal || path_lit.has_literal;
}

bool test_fallback_mechanism() {
    using P2 = decltype(ctre::search<"hello">);
    using AST2 = ctre::decomposition::unwrap_regex_t<P2>;
    auto fallback2 = ctre::dominators::extract_literal_with_fallback<AST2>();
    return fallback2.has_literal && check_literal(fallback2, "hello", 5);
}

bool test_edge_cases() {
    // Very short literal
    using P1 = decltype(ctre::search<"a">);
    using AST1 = ctre::decomposition::unwrap_regex_t<P1>;
    constexpr auto nfa1 = ctre::glushkov::glushkov_nfa<AST1>();
    constexpr auto lit1 = ctre::dominators::extract_literal_from_dominators(nfa1);
    return lit1.has_literal && lit1.length == 1;
}

bool test_no_extractable_literal() {
    using P1 = decltype(ctre::search<"[a-z]+">);
    using AST1 = ctre::decomposition::unwrap_regex_t<P1>;
    constexpr auto nfa1 = ctre::glushkov::glushkov_nfa<AST1>();
    constexpr auto lit1 = ctre::dominators::extract_literal_from_dominators(nfa1);
    return !lit1.has_literal;
}

int main() {
    std::cout << "=== Unit Test: Literal Extraction ===\n\n";

    TEST(simple_literal);
    TEST(alternation_extraction);
    TEST(leading_dot_star);
    TEST(complex_alternation);
    TEST(fallback_mechanism);
    TEST(edge_cases);
    TEST(no_extractable_literal);

    std::cout << "\nPassed: " << tests_passed << "/" << (tests_passed + tests_failed) << "\n";
    return tests_failed > 0 ? 1 : 0;
}
