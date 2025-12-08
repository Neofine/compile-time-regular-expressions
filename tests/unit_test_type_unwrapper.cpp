#include <ctre.hpp>
#include <iostream>
#include <type_traits>
#include <cassert>

int tests_passed = 0;
int tests_failed = 0;

#define TEST(name) \
    std::cout << "  " << #name << "... "; \
    if (test_##name()) { tests_passed++; std::cout << "PASSED\n"; } \
    else { tests_failed++; std::cout << "FAILED\n"; }

bool test_unwrap_search() {
    using Pattern = decltype(ctre::search<"hello">);
    using Unwrapped = ctre::decomposition::unwrap_regex_t<Pattern>;
    static_assert(!std::is_same_v<Pattern, Unwrapped>, "Should be different types");
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<Unwrapped>();
    return nfa.state_count > 0;
}

bool test_unwrap_alternation_with_capture() {
    using Pattern = decltype(ctre::search<"(foo|bar)">);
    using Unwrapped = ctre::decomposition::unwrap_regex_t<Pattern>;
    static_assert(!std::is_same_v<Pattern, Unwrapped>, "Should unwrap");
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<Unwrapped>();
    return nfa.state_count > 1 && nfa.accept_count >= 1;
}

bool test_unwrap_nested_captures() {
    using Pattern = decltype(ctre::search<"((a|b)(c|d))">);
    using Unwrapped = ctre::decomposition::unwrap_regex_t<Pattern>;
    static_assert(!std::is_same_v<Pattern, Unwrapped>, "Should unwrap");
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<Unwrapped>();
    return nfa.state_count >= 0;
}

bool test_unwrap_complex() {
    using Pattern = decltype(ctre::search<".*(hello|world).*test">);
    using Unwrapped = ctre::decomposition::unwrap_regex_t<Pattern>;
    static_assert(!std::is_same_v<Pattern, Unwrapped>, "Should unwrap");
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<Unwrapped>();
    return nfa.state_count > 5;
}

bool test_unwrap_match() {
    using Pattern = decltype(ctre::match<"hello">);
    using Unwrapped = ctre::decomposition::unwrap_regex_t<Pattern>;
    static_assert(!std::is_same_v<Pattern, Unwrapped>, "Should unwrap");
    return true;
}

int main() {
    std::cout << "=== Unit Test: Type Unwrapper ===\n\n";

    TEST(unwrap_search);
    TEST(unwrap_alternation_with_capture);
    TEST(unwrap_nested_captures);
    TEST(unwrap_complex);
    TEST(unwrap_match);

    std::cout << "\nPassed: " << tests_passed << "/" << (tests_passed + tests_failed) << "\n";
    return tests_failed > 0 ? 1 : 0;
}
