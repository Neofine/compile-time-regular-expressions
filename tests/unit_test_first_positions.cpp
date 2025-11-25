// Unit Test: first_positions() function
#include <ctre.hpp>
#include <iostream>
#include <cassert>
#include <algorithm>

template <typename Set>
bool contains(const Set& s, size_t pos) {
    return std::find(s.begin(), s.begin() + s.size(), pos) != (s.begin() + s.size());
}

#define TEST_FIRST(name, pattern, ...) do { \
    std::cout << "  " << name << ": "; \
    constexpr auto first = ctre::glushkov::first_positions<pattern>(0); \
    std::array<size_t, sizeof..(__VA_ARGS__)> expected = {__VA_ARGS__}; \
    assert(first.size() == expected.size()); \
    for (auto pos : expected) { \
        assert(contains(first, pos)); \
    } \
    std::cout << "✓ {"; \
    for (size_t i = 0; i < first.size(); ++i) { \
        if (i > 0) std::cout << ", "; \
        std::cout << first[i]; \
    } \
    std::cout << "}\n"; \
} while(0)

int main() {
    std::cout << "=== Unit Test: first_positions() ===\n\n";

    // Test 1: Single elements
    TEST_FIRST("character 'a'", ctre::character<'a'>, 1);
    TEST_FIRST("any (.)", ctre::any, 1);

    // Test 2: String
    TEST_FIRST("string 'abc'", ctre::string<'a','b','c'>, 1);
    TEST_FIRST("string 'hello'", ctre::string<'h','e','l','l','o'>, 1);

    // Test 3: Empty (should be empty set)
    std::cout << "  empty: ";
    constexpr auto empty_first = ctre::glushkov::first_positions<ctre::empty>(0);
    assert(empty_first.size() == 0);
    std::cout << "✓ {}\n";

    // Test 4: Sequence - first of first element (if not nullable)
    TEST_FIRST("sequence 'ab'.'cd'",
               ctre::sequence<ctre::string<'a','b'>, ctre::string<'c','d'>>,
               1);

    // Test 5: Sequence - both if first is nullable
    TEST_FIRST("sequence 'a*'.'b'",
               ctre::sequence<ctre::repeat<0,0,ctre::character<'a'>>, ctre::character<'b'>>,
               1, 2);

    // Test 6: Select - union of both
    TEST_FIRST("select 'ab'|'cd'",
               ctre::select<ctre::string<'a','b'>, ctre::string<'c','d'>>,
               1, 3);

    TEST_FIRST("select 'a'|'b'|'c'",
               ctre::select<ctre::character<'a'>, ctre::character<'b'>, ctre::character<'c'>>,
               1, 2, 3);

    // Test 7: Repeat
    TEST_FIRST("repeat 'a'*", ctre::repeat<0,0,ctre::character<'a'>>, 1);
    TEST_FIRST("repeat 'abc'*", ctre::repeat<0,0,ctre::string<'a','b','c'>>, 1);

    // Test 8: Complex patterns
    TEST_FIRST("(ab|cd)*",
               ctre::repeat<0,0,ctre::select<ctre::string<'a','b'>, ctre::string<'c','d'>>>,
               1, 3);

    TEST_FIRST("(abc|def).*ghi",
               ctre::sequence<
                   ctre::select<ctre::string<'a','b','c'>, ctre::string<'d','e','f'>>,
                   ctre::repeat<0,0,ctre::any>,
                   ctre::string<'g','h','i'>>,
               1, 4);

    // Test 9: Capture
    TEST_FIRST("capture<1, 'ab'>",
               ctre::capture<1, ctre::string<'a','b'>>,
               1);

    TEST_FIRST("capture<1, 'a'|'b'>",
               ctre::capture<1, ctre::select<ctre::character<'a'>, ctre::character<'b'>>>,
               1, 2);

    std::cout << "\n✓ All 13 first_positions tests passed!\n";
    return 0;
}

