#include <iostream>
#include <ctre.hpp>
#include "../include/ctre/char_class_expansion.hpp"

int tests_passed = 0;
int tests_failed = 0;

#define TEST(name, cond) do { \
    if (cond) { \
        std::cout << "[PASS] " << name << std::endl; \
        tests_passed++; \
    } else { \
        std::cout << "[FAIL] " << name << std::endl; \
        tests_failed++; \
    } \
} while(0)

int main() {
    std::cout << "=== Character Class Expansion Tests ===" << std::endl << std::endl;

    // Test 1: Single character
    {
        using CharClass = ctre::character<'a'>;
        constexpr auto result = ctre::expand_char_class<CharClass>();

        TEST("Single char 'a' - expandable", result.is_expandable);
        TEST("Single char 'a' - count = 1", result.count == 1);
        TEST("Single char 'a' - value", result.count > 0 && result.chars[0] == 'a');
    }

    // Test 2: Small character range [0-3]
    {
        using CharClass = ctre::char_range<'0', '3'>;
        constexpr auto result = ctre::expand_char_class<CharClass>();

        TEST("Range [0-3] - expandable", result.is_expandable);
        TEST("Range [0-3] - count = 4", result.count == 4);
        TEST("Range [0-3] - chars",
             result.count == 4 &&
             result.chars[0] == '0' &&
             result.chars[1] == '1' &&
             result.chars[2] == '2' &&
             result.chars[3] == '3');
    }

    // Test 3: Enumeration [abc]
    {
        using CharClass = ctre::enumeration<'a', 'b', 'c'>;
        constexpr auto result = ctre::expand_char_class<CharClass>();

        TEST("Enumeration [abc] - expandable", result.is_expandable);
        TEST("Enumeration [abc] - count = 3", result.count == 3);
        TEST("Enumeration [abc] - chars",
             result.count == 3 &&
             result.chars[0] == 'a' &&
             result.chars[1] == 'b' &&
             result.chars[2] == 'c');
    }

    // Test 4: Set [a-cX] (range + character)
    {
        using CharClass = ctre::set<ctre::char_range<'a', 'c'>, ctre::character<'X'>>;
        constexpr auto result = ctre::expand_char_class<CharClass>();

        TEST("Set [a-cX] - expandable", result.is_expandable);
        TEST("Set [a-cX] - count = 4", result.count == 4);
        TEST("Set [a-cX] - chars",
             result.count == 4 &&
             result.chars[0] == 'a' &&
             result.chars[1] == 'b' &&
             result.chars[2] == 'c' &&
             result.chars[3] == 'X');
    }

    // Test 5: Range [0-9] (10 chars, threshold is 11)
    {
        using CharClass = ctre::char_range<'0', '9'>;
        constexpr auto result = ctre::expand_char_class<CharClass>();

        // Should be expandable since 10 <= 11
        TEST("Range [0-9] - expandable (10 <= 11)", result.is_expandable);
        TEST("Range [0-9] - count = 10", result.count == 10);
    }

    // Test 6: Check expandability without expanding
    {
        constexpr bool exp_0_3 = ctre::is_expandable_char_class<ctre::char_range<'0', '3'>>();
        constexpr bool exp_0_9 = ctre::is_expandable_char_class<ctre::char_range<'0', '9'>>();
        constexpr bool exp_a_z = ctre::is_expandable_char_class<ctre::char_range<'a', 'z'>>();

        TEST("is_expandable [0-3]", exp_0_3);
        TEST("is_expandable [0-9]", exp_0_9);  // 10 <= 11
        TEST("is_expandable [a-z] - NO", !exp_a_z);  // 26 > 11
    }

    // Test 7: Count without expanding
    {
        constexpr size_t count_0_3 = ctre::count_char_class_size<ctre::char_range<'0', '3'>>();
        constexpr size_t count_0_9 = ctre::count_char_class_size<ctre::char_range<'0', '9'>>();
        constexpr size_t count_a_z = ctre::count_char_class_size<ctre::char_range<'a', 'z'>>();
        constexpr size_t count_x = ctre::count_char_class_size<ctre::character<'x'>>();

        TEST("count [0-3] = 4", count_0_3 == 4);
        TEST("count [0-9] = 10", count_0_9 == 10);
        TEST("count [a-z] = 26", count_a_z == 26);
        TEST("count 'x' = 1", count_x == 1);
    }

    // Test 8: Built-in character classes
    {
        // digit_chars is char_range<'0','9'> which is 10 chars (10 <= 11)
        TEST("digit_chars expandable", ctre::is_expandable_char_class<ctre::digit_chars>());

        // space_chars is enumeration with 6 chars
        TEST("space_chars expandable", ctre::is_expandable_char_class<ctre::space_chars>());
    }

    // Test 9: Hex digits [0-9a-fA-F] (too large)
    {
        using HexDigits = ctre::xdigit_chars; // set of [0-9], [a-f], [A-F]
        TEST("xdigit_chars not expandable (too large)", !ctre::is_expandable_char_class<HexDigits>());
    }

    // Test 10: Small custom set
    {
        using SmallSet = ctre::set<ctre::character<'!'>, ctre::character<'?'>, ctre::character<'.'>>;
        constexpr auto result = ctre::expand_char_class<SmallSet>();

        TEST("Small set [!?.] - expandable", result.is_expandable);
        TEST("Small set [!?.] - count = 3", result.count == 3);
        TEST("Small set [!?.] - chars",
             result.count == 3 &&
             result.chars[0] == '!' &&
             result.chars[1] == '?' &&
             result.chars[2] == '.');
    }

    std::cout << "\n========================================" << std::endl;
    std::cout << "Tests passed: " << tests_passed << std::endl;
    std::cout << "Tests failed: " << tests_failed << std::endl;
    std::cout << "========================================" << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
