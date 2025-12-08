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

template <auto Pattern>
bool verify_match(const std::string& text, bool should_match, const char* expected_match = nullptr) {
    auto result = Pattern(text);
    bool matches = static_cast<bool>(result);

    if (matches != should_match) return false;

    if (should_match && expected_match) {
        auto matched_text = result.to_view();
        std::string matched_str(matched_text.begin(), matched_text.end());
        if (matched_str != expected_match) return false;
    }
    return true;
}

bool test_simple_alternation() {
    constexpr auto pattern = ctre::search<"(foo|bar)">;
    return verify_match<pattern>("foo", true, "foo") &&
           verify_match<pattern>("bar", true, "bar") &&
           verify_match<pattern>("baz", false) &&
           verify_match<pattern>("xxfooxx", true, "foo") &&
           verify_match<pattern>("xxbarxx", true, "bar") &&
           verify_match<pattern>("qux", false);
}

bool test_complex_alternation() {
    constexpr auto pattern = ctre::search<"(http|https|ftp)://[a-z]+">;
    return pattern("http://example") &&
           pattern("https://test") &&
           pattern("ftp://server") &&
           !pattern("gopher://old");
}

bool test_leading_dot_star() {
    constexpr auto pattern = ctre::search<".*(hello|world).*test">;
    return verify_match<pattern>("hello world test", true) &&
           verify_match<pattern>("world test", true) &&
           verify_match<pattern>("hello test", true) &&
           verify_match<pattern>("test", false) &&
           verify_match<pattern>("hello", false) &&
           verify_match<pattern>("xxhelloxxtest", true);
}

bool test_prefix_suffix() {
    constexpr auto p1 = ctre::search<"prefix(foo|bar)">;
    constexpr auto p2 = ctre::search<"(foo|bar)suffix">;
    return verify_match<p1>("prefixfoo", true, "prefixfoo") &&
           verify_match<p1>("prefixbar", true, "prefixbar") &&
           verify_match<p1>("foo", false) &&
           verify_match<p2>("foosuffix", true, "foosuffix") &&
           verify_match<p2>("barsuffix", true, "barsuffix") &&
           verify_match<p2>("foo", false);
}

bool test_nested() {
    constexpr auto pattern = ctre::search<"((a|b)(c|d))">;
    return verify_match<pattern>("ac", true, "ac") &&
           verify_match<pattern>("ad", true, "ad") &&
           verify_match<pattern>("bc", true, "bc") &&
           verify_match<pattern>("bd", true, "bd") &&
           verify_match<pattern>("ab", false) &&
           verify_match<pattern>("cd", false);
}

bool test_character_class() {
    constexpr auto pattern = ctre::search<"([0-9]+|[a-z]+)">;
    return verify_match<pattern>("123", true, "123") &&
           verify_match<pattern>("abc", true, "abc") &&
           verify_match<pattern>("123abc", true, "123") &&
           verify_match<pattern>("ABC", false);
}

bool test_boundary() {
    constexpr auto p1 = ctre::search<"(foo|bar)">;
    return verify_match<p1>("foo", true) &&
           verify_match<p1>("bar", true) &&
           verify_match<p1>("xxxfoo", true) &&
           verify_match<p1>("xxxbar", true);
}

bool test_edge_cases() {
    constexpr auto p1 = ctre::search<"(a|b)">;
    constexpr auto p2 = ctre::search<"(foo|bar)">;
    
    std::string long_text(10000, 'x');
    long_text += "foo";
    
    return verify_match<p1>("", false) &&
           verify_match<p1>("c", false) &&
           verify_match<p1>("a", true, "a") &&
           verify_match<p1>("b", true, "b") &&
           verify_match<p2>(long_text, true);
}

bool test_multiple_matches() {
    constexpr auto pattern = ctre::search<"(foo|bar)">;
    auto result = pattern("foo bar");
    if (!result) return false;
    
    auto matched = result.to_view();
    std::string matched_str(matched.begin(), matched.end());
    return matched_str == "foo";  // First match
}

bool test_bug_regressions() {
    constexpr auto p1 = ctre::search<".*(hello|world).*test">;
    constexpr auto p2 = ctre::search<"(foo|bar)">;
    return verify_match<p1>("hello world test", true) &&
           verify_match<p2>("foo", true) &&
           verify_match<p2>("bar", true);
}

int main() {
    std::cout << "=== Correctness Integration Tests ===\n\n";

    TEST(simple_alternation);
    TEST(complex_alternation);
    TEST(leading_dot_star);
    TEST(prefix_suffix);
    TEST(nested);
    TEST(character_class);
    TEST(boundary);
    TEST(edge_cases);
    TEST(multiple_matches);
    TEST(bug_regressions);

    std::cout << "\nPassed: " << tests_passed << "/" << (tests_passed + tests_failed) << "\n";
    return tests_failed > 0 ? 1 : 0;
}
