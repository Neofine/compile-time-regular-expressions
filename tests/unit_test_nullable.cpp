// Unit Test: nullable() function
#include <ctre.hpp>
#include <iostream>
#include <cassert>

#define TEST(name, expected, ...) \
    std::cout << "  " << name << ": "; \
    { constexpr bool result = (__VA_ARGS__); \
    assert(result == (expected)); \
    std::cout << "✓ (" << (result ? "nullable" : "not nullable") << ")\n"; }

int main() {
    std::cout << "=== Unit Test: nullable() ===\n\n";

    // Test 1: Never nullable
    TEST("character 'a'",
         ctre::glushkov::nullable<ctre::character<'a'>>(),
         false);

    TEST("string 'abc'",
         ctre::glushkov::nullable<ctre::string<'a','b','c'>>(),
         false);

    TEST("any (.)",
         ctre::glushkov::nullable<ctre::any>(),
         false);

    // Test 2: Always nullable
    TEST("empty",
         ctre::glushkov::nullable<ctre::empty>(),
         true);

    TEST("repeat<0,0> (a*)",
         ctre::glushkov::nullable<ctre::repeat<0,0,ctre::character<'a'>>>(),
         true);

    TEST("repeat<0,5> (a{0,5})",
         ctre::glushkov::nullable<ctre::repeat<0,5,ctre::character<'a'>>>(),
         true);

    // Test 3: Never nullable (repeat with min > 0)
    TEST("repeat<1,0> (a+)",
         ctre::glushkov::nullable<ctre::repeat<1,0,ctre::character<'a'>>>(),
         false);

    TEST("repeat<2,5> (a{2,5})",
         ctre::glushkov::nullable<ctre::repeat<2,5,ctre::character<'a'>>>(),
         false);

    // Test 4: Sequence nullable if ALL parts nullable
    TEST("sequence (a*.b*) - both nullable",
         ctre::glushkov::nullable<ctre::sequence<
             ctre::repeat<0,0,ctre::character<'a'>>,
             ctre::repeat<0,0,ctre::character<'b'>>>>(),
         true);

    TEST("sequence (a.b*) - first NOT nullable",
         ctre::glushkov::nullable<ctre::sequence<
             ctre::character<'a'>,
             ctre::repeat<0,0,ctre::character<'b'>>>>(),
         false);

    TEST("sequence (a*.b) - second NOT nullable",
         ctre::glushkov::nullable<ctre::sequence<
             ctre::repeat<0,0,ctre::character<'a'>>,
             ctre::character<'b'>>>(),
         false);

    // Test 5: Select nullable if ANY part nullable
    TEST("select (a|b*) - second nullable",
         ctre::glushkov::nullable<ctre::select<
             ctre::character<'a'>,
             ctre::repeat<0,0,ctre::character<'b'>>>>(),
         true);

    TEST("select (a*|b) - first nullable",
         ctre::glushkov::nullable<ctre::select<
             ctre::repeat<0,0,ctre::character<'a'>>,
             ctre::character<'b'>>>(),
         true);

    TEST("select (a|b) - both NOT nullable",
         ctre::glushkov::nullable<ctre::select<
             ctre::character<'a'>,
             ctre::character<'b'>>>(),
         false);

    TEST("select (a*|b*) - both nullable",
         ctre::glushkov::nullable<ctre::select<
             ctre::repeat<0,0,ctre::character<'a'>>,
             ctre::repeat<0,0,ctre::character<'b'>>>>(),
         true);

    // Test 6: Capture (should match inner)
    TEST("capture<1, a*>",
         ctre::glushkov::nullable<ctre::capture<1, ctre::repeat<0,0,ctre::character<'a'>>>>(),
         true);

    TEST("capture<1, a>",
         ctre::glushkov::nullable<ctre::capture<1, ctre::character<'a'>>>(),
         false);

    // Test 7: Complex patterns
    TEST("(a|b)*",
         ctre::glushkov::nullable<ctre::repeat<0,0,
             ctre::select<ctre::character<'a'>, ctre::character<'b'>>>>(),
         true);

    TEST("(a|b)+",
         ctre::glushkov::nullable<ctre::repeat<1,0,
             ctre::select<ctre::character<'a'>, ctre::character<'b'>>>>(),
         false);

    TEST(".*",
         ctre::glushkov::nullable<ctre::repeat<0,0,ctre::any>>(),
         true);

    TEST(".+",
         ctre::glushkov::nullable<ctre::repeat<1,0,ctre::any>>(),
         false);

    std::cout << "\n✓ All 21 nullable tests passed!\n";
    return 0;
}
