// Unit Test: nullable() function
#include <ctre.hpp>
#include <iostream>
#include <cassert>

template <typename Pattern>
void test(const char* name, bool expected) {
    constexpr bool result = ctre::glushkov::nullable<Pattern>();
    std::cout << "  " << name << ": ";
    assert(result == expected);
    std::cout << "✓ (" << (result ? "nullable" : "not nullable") << ")\n";
}

int main() {
    std::cout << "=== Unit Test: nullable() ===\n\n";

    // Test 1: Never nullable
    test<ctre::character<'a'>>("character 'a'", false);
    test<ctre::string<'a','b','c'>>("string 'abc'", false);
    test<ctre::any>("any (.)", false);

    // Test 2: Always nullable
    test<ctre::empty>("empty", true);
    test<ctre::repeat<0,0,ctre::character<'a'>>>("repeat<0,0> (a*)", true);
    test<ctre::repeat<0,5,ctre::character<'a'>>>("repeat<0,5> (a{0,5})", true);

    // Test 3: Never nullable (repeat with min > 0)
    test<ctre::repeat<1,0,ctre::character<'a'>>>("repeat<1,0> (a+)", false);
    test<ctre::repeat<2,5,ctre::character<'a'>>>("repeat<2,5> (a{2,5})", false);

    // Test 4: Sequence nullable if ALL parts nullable
    using SeqBothNull = ctre::sequence<
        ctre::repeat<0,0,ctre::character<'a'>>,
        ctre::repeat<0,0,ctre::character<'b'>>>;
    test<SeqBothNull>("sequence (a*.b*) - both nullable", true);

    using SeqFirstNotNull = ctre::sequence<
        ctre::character<'a'>,
        ctre::repeat<0,0,ctre::character<'b'>>>;
    test<SeqFirstNotNull>("sequence (a.b*) - first NOT nullable", false);

    using SeqSecondNotNull = ctre::sequence<
        ctre::repeat<0,0,ctre::character<'a'>>,
        ctre::character<'b'>>;
    test<SeqSecondNotNull>("sequence (a*.b) - second NOT nullable", false);

    // Test 5: Select nullable if ANY part nullable
    using SelSecondNull = ctre::select<
        ctre::character<'a'>,
        ctre::repeat<0,0,ctre::character<'b'>>>;
    test<SelSecondNull>("select (a|b*) - second nullable", true);

    using SelFirstNull = ctre::select<
        ctre::repeat<0,0,ctre::character<'a'>>,
        ctre::character<'b'>>;
    test<SelFirstNull>("select (a*|b) - first nullable", true);

    using SelNeitherNull = ctre::select<ctre::character<'a'>, ctre::character<'b'>>;
    test<SelNeitherNull>("select (a|b) - both NOT nullable", false);

    using SelBothNull = ctre::select<
        ctre::repeat<0,0,ctre::character<'a'>>,
        ctre::repeat<0,0,ctre::character<'b'>>>;
    test<SelBothNull>("select (a*|b*) - both nullable", true);

    // Test 6: Capture (should match inner)
    test<ctre::capture<1, ctre::repeat<0,0,ctre::character<'a'>>>>("capture<1, a*>", true);
    test<ctre::capture<1, ctre::character<'a'>>>("capture<1, a>", false);

    // Test 7: Complex patterns
    using RepSel = ctre::repeat<0,0, ctre::select<ctre::character<'a'>, ctre::character<'b'>>>;
    test<RepSel>("(a|b)*", true);

    using RepSelPlus = ctre::repeat<1,0, ctre::select<ctre::character<'a'>, ctre::character<'b'>>>;
    test<RepSelPlus>("(a|b)+", false);

    test<ctre::repeat<0,0,ctre::any>>(".*", true);
    test<ctre::repeat<1,0,ctre::any>>(".+", false);

    std::cout << "\n✓ All 21 nullable tests passed!\n";
    return 0;
}
