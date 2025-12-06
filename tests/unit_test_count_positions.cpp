// Unit Test: count_positions() function
#include <ctre.hpp>
#include <iostream>
#include <cassert>

template <typename Pattern>
void test(const char* name, size_t expected) {
    constexpr auto result = ctre::glushkov::count_positions<Pattern>();
    std::cout << "  " << name << ": ";
    assert(result == expected);
    std::cout << "✓ (" << result << " == " << expected << ")\n";
}

int main() {
    std::cout << "=== Unit Test: count_positions() ===\n\n";

    // Test 1: Single character
    test<ctre::character<'a'>>("character 'a'", 1);

    // Test 2: String literal
    test<ctre::string<'a','b','c'>>("string 'abc'", 3);
    test<ctre::string<'h','e','l','l','o'>>("string 'hello'", 5);

    // Test 3: Empty
    test<ctre::empty>("empty", 0);

    // Test 4: Sequence
    using Seq1 = ctre::sequence<ctre::string<'a','b'>, ctre::string<'c','d'>>;
    test<Seq1>("sequence 'ab'.'cd'", 4);

    // Test 5: Select (alternation)
    using Sel1 = ctre::select<ctre::string<'a','b'>, ctre::string<'c','d'>>;
    test<Sel1>("select 'ab'|'cd'", 4);

    // Test 6: Repeat
    test<ctre::repeat<0,0,ctre::character<'a'>>>("repeat 'a'*", 1);
    test<ctre::repeat<0,0,ctre::string<'a','b','c'>>>("repeat 'abc'*", 3);

    // Test 7: Complex nesting
    using Rep1 = ctre::repeat<0,0, ctre::select<ctre::string<'a','b'>, ctre::string<'c','d'>>>;
    test<Rep1>("(ab|cd)*", 4);

    using Rep2 = ctre::repeat<0,0, ctre::sequence<
        ctre::select<ctre::character<'a'>, ctre::character<'b'>>,
        ctre::select<ctre::character<'c'>, ctre::character<'d'>>>>;
    test<Rep2>("((a|b)(c|d))*", 4);

    // Test 8: Character classes (counted as 1 position)
    test<ctre::set<ctre::char_range<'a','z'>>>("character class [a-z]", 1);

    // Test 9: Any (.)
    test<ctre::any>("any (.)", 1);

    // Test 10: Large pattern
    using Large = ctre::sequence<
        ctre::select<ctre::string<'a','b','c'>, ctre::string<'d','e','f'>>,
        ctre::repeat<0,0,ctre::any>,
        ctre::string<'g','h','i'>>;
    test<Large>("(abc|def).*ghi", 10);

    std::cout << "\n✓ All 14 count_positions tests passed!\n";
    return 0;
}
