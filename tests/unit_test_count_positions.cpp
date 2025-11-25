// Unit Test: count_positions() function
#include <ctre.hpp>
#include <iostream>
#include <cassert>

#define TEST(name, expected, ...) \
    std::cout << "  " << name << ": "; \
    { constexpr auto result = (__VA_ARGS__); \
    assert(result == (expected)); \
    std::cout << "✓ (" << result << " == " << (expected) << ")\n"; }

int main() {
    std::cout << "=== Unit Test: count_positions() ===\n\n";

    // Test 1: Single character
    TEST("character 'a'",
         ctre::glushkov::count_positions<ctre::character<'a'>>(),
         1);

    // Test 2: String literal
    TEST("string 'abc'",
         ctre::glushkov::count_positions<ctre::string<'a','b','c'>>(),
         3);

    TEST("string 'hello'",
         ctre::glushkov::count_positions<ctre::string<'h','e','l','l','o'>>(),
         5);

    // Test 3: Empty
    TEST("empty",
         ctre::glushkov::count_positions<ctre::empty>(),
         0);

    // Test 4: Sequence
    TEST("sequence 'ab'.'cd'",
         ctre::glushkov::count_positions<ctre::sequence<
             ctre::string<'a','b'>,
             ctre::string<'c','d'>>>(),
         4);

    // Test 5: Select (alternation)
    TEST("select 'ab'|'cd'",
         ctre::glushkov::count_positions<ctre::select<
             ctre::string<'a','b'>,
             ctre::string<'c','d'>>>(),
         4);

    // Test 6: Repeat
    TEST("repeat 'a'*",
         ctre::glushkov::count_positions<ctre::repeat<0,0,ctre::character<'a'>>>(),
         1);

    TEST("repeat 'abc'*",
         ctre::glushkov::count_positions<ctre::repeat<0,0,ctre::string<'a','b','c'>>>(),
         3);

    // Test 7: Complex nesting
    TEST("(ab|cd)*",
         ctre::glushkov::count_positions<ctre::repeat<0,0,
             ctre::select<
                 ctre::string<'a','b'>,
                 ctre::string<'c','d'>>>>(),
         4);

    TEST("((a|b)(c|d))*",
         ctre::glushkov::count_positions<ctre::repeat<0,0,
             ctre::sequence<
                 ctre::select<ctre::character<'a'>, ctre::character<'b'>>,
                 ctre::select<ctre::character<'c'>, ctre::character<'d'>>>>>(),
         4);

    // Test 8: Character classes (counted as 1 position)
    TEST("character class [a-z]",
         ctre::glushkov::count_positions<ctre::set<ctre::char_range<'a','z'>>>(),
         1);

    // Test 9: Any (.)
    TEST("any (.)",
         ctre::glushkov::count_positions<ctre::any>(),
         1);

    // Test 10: Large pattern
    TEST("(abc|def).*ghi",
         ctre::glushkov::count_positions<ctre::sequence<
             ctre::select<
                 ctre::string<'a','b','c'>,
                 ctre::string<'d','e','f'>>,
             ctre::repeat<0,0,ctre::any>,
             ctre::string<'g','h','i'>>>(),
         10);

    std::cout << "\n✓ All 14 count_positions tests passed!\n";
    return 0;
}
