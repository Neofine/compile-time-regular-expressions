// Integration Test: Glushkov NFA with CTRE patterns
#include <ctre.hpp>
#include <iostream>
#include <string>
#include <cassert>

#define TEST(name) \
    std::cout << "\nTest: " << #name << "\n"; \
    test_##name();

// Test 1: Simple pattern - verify NFA can be constructed from CTRE pattern
void test_simple_pattern_construction() {
    constexpr auto pattern = ctre::search<"hello">;
    using Pattern = decltype(pattern);
    using AST = ctre::decomposition::unwrap_regex_t<Pattern>;

    // Should be able to construct NFA
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<AST>();

    std::cout << "  NFA states: " << nfa.state_count << "\n";
    std::cout << "  NFA accepts: " << nfa.accept_count << "\n";
    assert(nfa.state_count > 0);

    // Should still match correctly
    assert(pattern("hello world"));
    assert(!pattern("goodbye"));
    std::cout << "  ✓ NFA construction + matching works\n";
}

// Test 2: Alternation - verify NFA construction with select
void test_alternation_nfa() {
    constexpr auto pattern = ctre::search<"(foo|bar)">;
    using Pattern = decltype(pattern);
    using AST = ctre::decomposition::unwrap_regex_t<Pattern>;

    constexpr auto nfa = ctre::glushkov::glushkov_nfa<AST>();
    std::cout << "  (foo|bar) NFA: " << nfa.state_count << " states\n";

    // Verify pattern still works
    assert(pattern("foo"));
    assert(pattern("bar"));
    assert(!pattern("baz"));
    std::cout << "  ✓ Alternation NFA works\n";
}

// Test 3: Literal extraction from NFA
void test_literal_extraction_from_nfa() {
    constexpr auto pattern = ctre::search<"prefix_hello_suffix">;
    using Pattern = decltype(pattern);
    using AST = ctre::decomposition::unwrap_regex_t<Pattern>;

    constexpr auto nfa = ctre::glushkov::glushkov_nfa<AST>();
    constexpr auto literal = ctre::dominators::extract_literal_from_dominators(nfa);

    std::cout << "  Has literal: " << literal.has_literal << "\n";
    std::cout << "  Length: " << literal.length << "\n";

    if (literal.has_literal) {
        std::cout << "  Literal: \"";
        for (size_t i = 0; i < literal.length; ++i) {
            std::cout << literal.chars[i];
        }
        std::cout << "\"\n";
    }

    assert(literal.has_literal);
    std::cout << "  ✓ Literal extraction works\n";
}

// Test 4: Decomposition uses NFA
void test_decomposition_uses_nfa() {
    using Pattern = decltype(ctre::search<"test_pattern">);
    using AST = ctre::decomposition::unwrap_regex_t<Pattern>;

    // Verify we can build NFA
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<AST>();
    assert(nfa.state_count > 0);

    // Verify literal extraction works
    constexpr auto literal = ctre::dominators::extract_literal<AST>();
    assert(literal.has_literal);

    std::cout << "  ✓ Decomposition pipeline works with NFA\n";
}

// Test 5: Complex pattern with repeats
void test_complex_pattern_with_repeats() {
    constexpr auto pattern = ctre::search<"a.*b.*c">;
    using Pattern = decltype(pattern);
    using AST = ctre::decomposition::unwrap_regex_t<Pattern>;

    [[maybe_unused]] constexpr auto nfa = ctre::glushkov::glushkov_nfa<AST>();
    std::cout << "  a.*b.*c NFA: " << nfa.state_count << " states\n";

    // Pattern should work correctly
    assert(pattern("a_b_c"));
    assert(pattern("axxxxxbxxxxxc"));
    assert(!pattern("cab"));
    std::cout << "  ✓ Complex pattern works\n";
}

// Test 6: NFA state transitions are correct
void test_nfa_state_transitions() {
    // Simple pattern: "ab"
    using Pattern = ctre::string<'a', 'b'>;
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<Pattern>();

    std::cout << "  Pattern 'ab':\n";
    std::cout << "    States: " << nfa.state_count << " (expected: 3)\n";
    std::cout << "    State 0: start\n";
    std::cout << "    State 1: symbol='a'\n";
    std::cout << "    State 2: symbol='b', accept\n";

    assert(nfa.state_count == 3);
    assert(nfa.accept_count == 1);

    // Verify state 1 symbol
    assert(nfa.states[1].symbol == 'a');
    assert(nfa.states[2].symbol == 'b');

    std::cout << "  ✓ State structure correct\n";
}

// Test 7: Integration with search - literal prefiltering
void test_search_with_literal_prefiltering() {
    // Pattern that should benefit from literal extraction
    constexpr auto pattern = ctre::search<"test_literal">;

    const char* text = "xxxxxxtest_literalxxxxxx";

    // Should find it efficiently
    auto result = pattern(text);
    assert(static_cast<bool>(result));

    std::string matched(result.to_view().begin(), result.to_view().end());
    assert(matched == "test_literal");

    std::cout << "  ✓ Search with literal prefiltering works\n";
}

// Test 8: Integration with match (not search)
void test_match_with_nfa() {
    constexpr auto pattern = ctre::match<"hello">;
    using Pattern = decltype(pattern);
    using AST = ctre::decomposition::unwrap_regex_t<Pattern>;

    // Should be able to build NFA
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<AST>();
    assert(nfa.state_count > 0);

    // Match should still work
    assert(pattern("hello"));
    assert(!pattern("hello world"));  // match requires full string

    std::cout << "  ✓ Match (not search) works with NFA\n";
}

// Test 9: Complex patterns with leading .*
void test_leading_dot_star_patterns() {
    // Pattern with leading .* should still match correctly
    constexpr auto pattern = ctre::search<".*(hello|world).*test">;
    assert(pattern("hello world test"));
    assert(pattern("world test"));
    assert(!pattern("test"));

    std::cout << "  ✓ Leading .* patterns work correctly\n";
}

// Test 10: End-to-end with real-world pattern
void test_real_world_url_pattern() {
    constexpr auto pattern = ctre::search<"(http|https)://[a-z]+\\.[a-z]+">;
    using Pattern = decltype(pattern);
    using AST = ctre::decomposition::unwrap_regex_t<Pattern>;

    // Build NFA
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<AST>();
    std::cout << "  URL pattern NFA: " << nfa.state_count << " states\n";

    // Should match URLs
    assert(pattern("http://example.com"));
    assert(pattern("https://test.org"));
    assert(!pattern("ftp://server.net"));

    std::cout << "  ✓ Real-world URL pattern works\n";
}

int main() {
    std::cout << "====================================================================\n";
    std::cout << "Integration Test: Glushkov NFA with CTRE\n";
    std::cout << "====================================================================\n";

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

    std::cout << "\n====================================================================\n";
    std::cout << "✓ All 10 Glushkov-CTRE integration tests passed!\n";
    std::cout << "====================================================================\n";

    return 0;
}
