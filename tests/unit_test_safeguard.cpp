// Unit Test: has_leading_greedy_repeat() safeguard
#include <ctre.hpp>
#include <iostream>
#include <cassert>

#define TEST(name) \
    std::cout << "Testing: " << #name << "... "; \
    test_##name(); \
    std::cout << "✓ PASSED\n";

// Test 1: Detect .* at start
void test_leading_dot_star() {
    using P1 = decltype(ctre::search<".*hello">);
    using AST1 = ctre::decomposition::unwrap_regex_t<P1>;
    assert(ctre::has_leading_greedy_repeat<AST1>() == true);
    
    using P2 = decltype(ctre::search<".*">);
    using AST2 = ctre::decomposition::unwrap_regex_t<P2>;
    assert(ctre::has_leading_greedy_repeat<AST2>() == true);
    
    using P3 = decltype(ctre::search<".*(hello|world).*test">);
    using AST3 = ctre::decomposition::unwrap_regex_t<P3>;
    assert(ctre::has_leading_greedy_repeat<AST3>() == true);
}

// Test 2: Don't detect .* in middle or end
void test_non_leading_dot_star() {
    using P1 = decltype(ctre::search<"hello.*">);
    using AST1 = ctre::decomposition::unwrap_regex_t<P1>;
    assert(ctre::has_leading_greedy_repeat<AST1>() == false);
    
    using P2 = decltype(ctre::search<"foo.*bar">);
    using AST2 = ctre::decomposition::unwrap_regex_t<P2>;
    assert(ctre::has_leading_greedy_repeat<AST2>() == false);
}

// Test 3: Don't detect non-greedy patterns
void test_non_greedy_patterns() {
    using P1 = decltype(ctre::search<"hello">);
    using AST1 = ctre::decomposition::unwrap_regex_t<P1>;
    assert(ctre::has_leading_greedy_repeat<AST1>() == false);
    
    using P2 = decltype(ctre::search<"(foo|bar)">);
    using AST2 = ctre::decomposition::unwrap_regex_t<P2>;
    assert(ctre::has_leading_greedy_repeat<AST2>() == false);
    
    using P3 = decltype(ctre::search<"[a-z]+">);
    using AST3 = ctre::decomposition::unwrap_regex_t<P3>;
    assert(ctre::has_leading_greedy_repeat<AST3>() == false);
}

// Test 4: Edge cases
void test_edge_cases() {
    using P1 = decltype(ctre::search<".+hello">);  // .+ not .*
    using AST1 = ctre::decomposition::unwrap_regex_t<P1>;
    // Currently only detects .*, not .+ (could be improved)
    // assert(ctre::has_leading_greedy_repeat<AST1>() == false);
    
    using P2 = decltype(ctre::search<".*.*">);  // Multiple .*
    using AST2 = ctre::decomposition::unwrap_regex_t<P2>;
    assert(ctre::has_leading_greedy_repeat<AST2>() == true);
}

int main() {
    std::cout << "=== Unit Tests: has_leading_greedy_repeat() ===\n\n";
    
    TEST(leading_dot_star);
    TEST(non_leading_dot_star);
    TEST(non_greedy_patterns);
    TEST(edge_cases);
    
    std::cout << "\n✓ All safeguard unit tests passed!\n";
    return 0;
}

