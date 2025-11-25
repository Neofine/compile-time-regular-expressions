#include <iostream>
#include <ctre.hpp>
#include "../include/ctre/literal_extraction_simple_multi.hpp"

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
    std::cout << "=== Simple Multi-Path Extraction Tests ===" << std::endl << std::endl;

    // Test 1: Simple string
    {
        using AST = ctre::string<'h','e','l','l','o'>;
        constexpr auto result = ctre::extraction::extract_literals_simple_multi<AST>();

        TEST("String 'hello' - count = 1", result.count == 1);
        TEST("String 'hello' - correct",
             result.count == 1 &&
             result.literals[0].length == 5 &&
             result.literals[0].chars[0] == 'h' &&
             result.literals[0].chars[4] == 'o');
    }

    // Test 2: Character class [0-3]
    {
        using AST = ctre::char_range<'0', '3'>;
        constexpr auto result = ctre::extraction::extract_literals_simple_multi<AST>();

        TEST("Range [0-3] - count = 4", result.count == 4);
    }

    // Test 3: CRITICAL TEST: [0-2]test (sequence with char class)
    {
        using AST = ctre::sequence<
            ctre::char_range<'0', '2'>,
            ctre::string<'t','e','s','t'>
        >;
        constexpr auto result = ctre::extraction::extract_literals_simple_multi<AST>();

        std::cout << "  Debug: count = " << result.count << std::endl;
        for (size_t i = 0; i < result.count; ++i) {
            std::cout << "    [" << i << "]: ";
            for (size_t j = 0; j < result.literals[i].length; ++j) {
                std::cout << result.literals[i].chars[j];
            }
            std::cout << " (len=" << result.literals[i].length << ")" << std::endl;
        }

        TEST("Sequence [0-2]test - count = 3", result.count == 3);

        // Check each literal
        bool all_correct = true;
        if (result.count == 3) {
            for (size_t i = 0; i < 3; ++i) {
                if (result.literals[i].length != 5 ||
                    result.literals[i].chars[0] != '0' + i ||
                    result.literals[i].chars[1] != 't' ||
                    result.literals[i].chars[2] != 'e' ||
                    result.literals[i].chars[3] != 's' ||
                    result.literals[i].chars[4] != 't') {
                    all_correct = false;
                    break;
                }
            }
        } else {
            all_correct = false;
        }
        TEST("Sequence [0-2]test - literals correct", all_correct);
    }

    // Test 4: Alternation (foo|bar)
    {
        using AST = ctre::select<
            ctre::string<'f','o','o'>,
            ctre::string<'b','a','r'>
        >;
        constexpr auto result = ctre::extraction::extract_literals_simple_multi<AST>();

        std::cout << "  Debug alternation: count = " << result.count << std::endl;
        for (size_t i = 0; i < result.count; ++i) {
            std::cout << "    [" << i << "]: ";
            for (size_t j = 0; j < result.literals[i].length; ++j) {
                std::cout << result.literals[i].chars[j];
            }
            std::cout << std::endl;
        }

        TEST("Alternation (foo|bar) - count = 2", result.count == 2);

        bool has_foo = false, has_bar = false;
        for (size_t i = 0; i < result.count; ++i) {
            if (result.literals[i].length == 3) {
                if (result.literals[i].chars[0] == 'f' &&
                    result.literals[i].chars[1] == 'o' &&
                    result.literals[i].chars[2] == 'o') {
                    has_foo = true;
                }
                if (result.literals[i].chars[0] == 'b' &&
                    result.literals[i].chars[1] == 'a' &&
                    result.literals[i].chars[2] == 'r') {
                    has_bar = true;
                }
            }
        }
        TEST("Alternation (foo|bar) - has both", has_foo && has_bar);
    }

    // Test 5: Complex - ([0-1]a|[2-3]b)
    {
        using AST = ctre::select<
            ctre::sequence<ctre::char_range<'0', '1'>, ctre::character<'a'>>,
            ctre::sequence<ctre::char_range<'2', '3'>, ctre::character<'b'>>
        >;
        constexpr auto result = ctre::extraction::extract_literals_simple_multi<AST>();

        TEST("Complex ([0-1]a|[2-3]b) - count = 4", result.count == 4);
        // Should get: "0a", "1a", "2b", "3b"
    }

    // Test 6: get_longest
    {
        using AST = ctre::select<
            ctre::string<'a','b'>,
            ctre::string<'x','y','z'>
        >;
        constexpr auto result = ctre::extraction::extract_literals_simple_multi<AST>();
        constexpr auto longest = result.get_longest();

        TEST("get_longest - length = 3", longest.length == 3);
    }

    // Test 7: Prefix + char class + suffix: x[0-1]y
    {
        using AST = ctre::sequence<
            ctre::character<'x'>,
            ctre::char_range<'0', '1'>,
            ctre::character<'y'>
        >;
        constexpr auto result = ctre::extraction::extract_literals_simple_multi<AST>();

        TEST("x[0-1]y - count = 2", result.count == 2);
        // Should get: "x0y", "x1y"

        bool correct = false;
        if (result.count == 2) {
            bool has_x0y = false, has_x1y = false;
            for (size_t i = 0; i < 2; ++i) {
                if (result.literals[i].length == 3 && result.literals[i].chars[0] == 'x' && result.literals[i].chars[2] == 'y') {
                    if (result.literals[i].chars[1] == '0') has_x0y = true;
                    if (result.literals[i].chars[1] == '1') has_x1y = true;
                }
            }
            correct = has_x0y && has_x1y;
        }
        TEST("x[0-1]y - correct literals", correct);
    }

    std::cout << "\n========================================" << std::endl;
    std::cout << "Tests passed: " << tests_passed << std::endl;
    std::cout << "Tests failed: " << tests_failed << std::endl;
    std::cout << "========================================" << std::endl;

    return tests_failed > 0 ? 1 : 0;
}
