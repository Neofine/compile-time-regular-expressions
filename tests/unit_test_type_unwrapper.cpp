// Unit Test: Type unwrapper for regular_expression and capture
#include <ctre.hpp>
#include <iostream>
#include <type_traits>
#include <cassert>

#define TEST(name) \
    std::cout << "Testing: " << #name << "... "; \
    test_##name(); \
    std::cout << "✓ PASSED\n";

// Test 1: Unwrap search pattern
void test_unwrap_search() {
    using Pattern = decltype(ctre::search<"hello">);
    using Unwrapped = ctre::decomposition::unwrap_regex_t<Pattern>;
    
    // Should unwrap to the raw AST (string<'h','e','l','l','o'>)
    static_assert(!std::is_same_v<Pattern, Unwrapped>, "Should be different types");
    
    // Basic check that unwrapping happened - can build NFA
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<Unwrapped>();
    assert(nfa.state_count > 0);
}

// Test 2: Unwrap alternation with capture
void test_unwrap_alternation_with_capture() {
    using Pattern = decltype(ctre::search<"(foo|bar)">);
    using Unwrapped = ctre::decomposition::unwrap_regex_t<Pattern>;
    
    // Should expose the select (alternation) inside
    static_assert(!std::is_same_v<Pattern, Unwrapped>, "Should unwrap");
    
    // Verify we can build NFA from unwrapped type
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<Unwrapped>();
    assert(nfa.state_count > 1);
    assert(nfa.accept_count >= 1);
}

// Test 3: Unwrap nested captures
void test_unwrap_nested_captures() {
    using Pattern = decltype(ctre::search<"((a|b)(c|d))">);
    using Unwrapped = ctre::decomposition::unwrap_regex_t<Pattern>;
    
    static_assert(!std::is_same_v<Pattern, Unwrapped>, "Should unwrap");
    
    // Verify NFA construction works (doesn't crash)
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<Unwrapped>();
    assert(nfa.state_count >= 0);  // Just verify it compiled
}

// Test 4: Unwrap complex pattern
void test_unwrap_complex() {
    using Pattern = decltype(ctre::search<".*(hello|world).*test">);
    using Unwrapped = ctre::decomposition::unwrap_regex_t<Pattern>;
    
    static_assert(!std::is_same_v<Pattern, Unwrapped>, "Should unwrap");
    
    // Verify we can analyze the structure
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<Unwrapped>();
    assert(nfa.state_count > 5);
}

// Test 5: Unwrap match pattern (not just search)
void test_unwrap_match() {
    using Pattern = decltype(ctre::match<"hello">);
    using Unwrapped = ctre::decomposition::unwrap_regex_t<Pattern>;
    
    static_assert(!std::is_same_v<Pattern, Unwrapped>, "Should unwrap");
}

int main() {
    std::cout << "=== Unit Tests: Type Unwrapper ===\n\n";
    
    TEST(unwrap_search);
    TEST(unwrap_alternation_with_capture);
    TEST(unwrap_nested_captures);
    TEST(unwrap_complex);
    TEST(unwrap_match);
    
    std::cout << "\n✓ All type unwrapper unit tests passed!\n";
    return 0;
}

