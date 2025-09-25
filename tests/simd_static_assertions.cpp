#include <ctre.hpp>
#include <string_view>
#include <iostream>
#include <cassert>

using namespace ctre::literals;
using namespace std::string_view_literals;

// Test that static assertions still work with SIMD optimizations
void test_static_assertions() {
    // These should all compile and work correctly
    
    // Test 1: Basic literal matching
    static_assert("hello"_ctre.match("hello"sv));
    static_assert(!"hello"_ctre.match("world"sv));
    
    // Test 2: Long literal string (should use SIMD at runtime)
    static_assert("this_is_a_very_long_string_that_should_benefit_from_simd_optimizations_and_avx2_instructions_for_better_performance"_ctre.match("this_is_a_very_long_string_that_should_benefit_from_simd_optimizations_and_avx2_instructions_for_better_performance"sv));
    
    // Test 3: Character repetition patterns
    static_assert("a*"_ctre.match("aaaaa"sv));
    static_assert("a+"_ctre.match("aaaaa"sv));
    static_assert(!"a+"_ctre.match(""sv));
    
    // Test 4: Character classes
    static_assert("[a-z]*"_ctre.match("hello"sv));
    static_assert("[0-9]+"_ctre.match("12345"sv));
    static_assert(!"[0-9]+"_ctre.match("hello"sv));
    
    // Test 5: Case insensitive patterns
    static_assert("(?i)hello"_ctre.match("HELLO"sv));
    static_assert("(?i)hello"_ctre.match("hello"sv));
    static_assert("(?i)hello"_ctre.match("Hello"sv));
    
    // Test 6: Mixed patterns
    static_assert("a*b+"_ctre.match("aaabbb"sv));
    static_assert("[a-z]+[0-9]*"_ctre.match("hello123"sv));
    
    // Test 7: String repetition
    static_assert("(hello)*"_ctre.match("hellohello"sv));
    static_assert("(ab)+"_ctre.match("abab"sv));
    
    // Test 8: Search vs match
    static_assert("hello"_ctre.search("worldhelloworld"sv));
    static_assert(!"hello"_ctre.match("worldhelloworld"sv));
    
    // Test 9: Complex patterns
    static_assert("^[a-z]+@[a-z]+\\.[a-z]+$"_ctre.match("user@domain.com"sv));
    static_assert("\\d{3}-\\d{3}-\\d{4}"_ctre.match("123-456-7890"sv));
    
    // Test 10: Edge cases
    static_assert(""_ctre.match(""sv));
    static_assert(".*"_ctre.match("anything"sv));
    static_assert("a?"_ctre.match(""sv));
    static_assert("a?"_ctre.match("a"sv));
}

// Test that regex syntax validation still works
void test_syntax_validation() {
    // These should all compile (valid syntax)
    // Note: _ctre_syntax is not available in all configurations
    // We'll test with actual regex creation instead
    auto regex1 = "hello"_ctre;
    auto regex2 = "a*"_ctre;
    auto regex3 = "[a-z]+"_ctre;
    auto regex4 = "(?i)hello"_ctre;
    auto regex5 = "(hello)*"_ctre;
    
    // Test that they work
    assert(regex1.match("hello"));
    assert(regex2.match("aaaaa"));
    assert(regex3.match("hello"));
    assert(regex4.match("HELLO"));
    assert(regex5.match("hellohello"));
}

// Test that the regex generation still works
void test_regex_generation() {
    // These should all compile and generate the correct regex types
    auto regex1 = "hello"_ctre;
    auto regex2 = "a*"_ctre;
    auto regex3 = "[a-z]+"_ctre;
    auto regex4 = "(?i)hello"_ctre;
    auto regex5 = "(hello)*"_ctre;
    
    // Test that they work at runtime
    std::string test_string = "hello";
    std::string test_string2 = "aaaaa";
    std::string test_string3 = "";
    
    assert(regex1.match(test_string));  // "hello" matches "hello"
    assert(regex2.match(test_string3)); // "a*" matches empty string
    assert(regex2.match(test_string2)); // "a*" matches "aaaaa"
    assert(regex3.match(test_string));  // "[a-z]+" matches "hello"
    assert(regex4.match(test_string));  // "(?i)hello" matches "hello"
    assert(regex5.match(test_string3)); // "(hello)*" matches empty string
}

// Test that constexpr evaluation still works
void test_constexpr_evaluation() {
    // These should all be evaluated at compile time
    constexpr bool result1 = "hello"_ctre.match("hello"sv);
    constexpr bool result2 = "a*"_ctre.match("aaaaa"sv);
    constexpr bool result3 = "[a-z]+"_ctre.match("hello"sv);
    constexpr bool result4 = "(?i)hello"_ctre.match("HELLO"sv);
    
    assert(result1);
    assert(result2);
    assert(result3);
    assert(result4);
}

int main() {
    test_static_assertions();
    test_syntax_validation();
    test_regex_generation();
    
    std::cout << "All static assertion tests passed!" << std::endl;
    std::cout << "SIMD optimizations preserve compile-time functionality." << std::endl;
    
    return 0;
}
