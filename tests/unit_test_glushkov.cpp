#include <ctre.hpp>
#include <iostream>
#include <cassert>
#include <algorithm>

int tests_passed = 0;

template <typename Pattern>
void test_count(const char* name, size_t expected) {
    constexpr auto result = ctre::glushkov::count_positions<Pattern>();
    assert(result == expected);
    std::cout << "  [count] " << name << ": " << result << "\n";
    tests_passed++;
}

template <typename Pattern>
void test_nullable(const char* name, bool expected) {
    constexpr bool result = ctre::glushkov::nullable<Pattern>();
    assert(result == expected);
    std::cout << "  [nullable] " << name << ": " << (result ? "yes" : "no") << "\n";
    tests_passed++;
}

template <size_t N>
bool contains(const std::array<size_t, N>& arr, size_t count, size_t pos) {
    return std::find(arr.begin(), arr.begin() + count, pos) != (arr.begin() + count);
}

template <typename Pattern, size_t N>
void test_first(const char* name, const std::array<size_t, N>& expected) {
    constexpr auto result = ctre::glushkov::first_positions<Pattern>(0);
    assert(result.second == expected.size());
    for (auto pos : expected) {
        assert(contains(result.first, result.second, pos));
    }
    std::cout << "  [first] " << name << ": {";
    for (size_t i = 0; i < result.second; ++i) {
        if (i > 0) std::cout << ",";
        std::cout << result.first[i];
    }
    std::cout << "}\n";
    tests_passed++;
}

int main() {
    std::cout << "=== Glushkov NFA Unit Tests ===\n\n";

    // Common type aliases
    using Seq_ab_cd = ctre::sequence<ctre::string<'a','b'>, ctre::string<'c','d'>>;
    using Sel_ab_cd = ctre::select<ctre::string<'a','b'>, ctre::string<'c','d'>>;
    using Rep_abcd = ctre::repeat<0,0, ctre::select<ctre::string<'a','b'>, ctre::string<'c','d'>>>;
    using Hyperscan = ctre::sequence<
        ctre::select<ctre::string<'a','b','c'>, ctre::string<'d','e','f'>>,
        ctre::repeat<0,0,ctre::any>,
        ctre::string<'g','h','i'>>;

    std::cout << "count_positions:\n";
    test_count<ctre::character<'a'>>("'a'", 1);
    test_count<ctre::string<'a','b','c'>>("'abc'", 3);
    test_count<ctre::empty>("empty", 0);
    test_count<Seq_ab_cd>("'ab'.'cd'", 4);
    test_count<Sel_ab_cd>("'ab'|'cd'", 4);
    test_count<ctre::repeat<0,0,ctre::character<'a'>>>("a*", 1);
    test_count<Rep_abcd>("(ab|cd)*", 4);
    test_count<ctre::set<ctre::char_range<'a','z'>>>("[a-z]", 1);
    test_count<ctre::any>(".", 1);
    test_count<Hyperscan>("(abc|def).*ghi", 10);

    std::cout << "\nnullable:\n";
    test_nullable<ctre::character<'a'>>("'a'", false);
    test_nullable<ctre::string<'a','b','c'>>("'abc'", false);
    test_nullable<ctre::any>(".", false);
    test_nullable<ctre::empty>("empty", true);
    test_nullable<ctre::repeat<0,0,ctre::character<'a'>>>("a*", true);
    test_nullable<ctre::repeat<1,0,ctre::character<'a'>>>("a+", false);
    test_nullable<ctre::repeat<0,5,ctre::character<'a'>>>("a{0,5}", true);
    test_nullable<ctre::repeat<2,5,ctre::character<'a'>>>("a{2,5}", false);

    using SeqBothNull = ctre::sequence<ctre::repeat<0,0,ctre::character<'a'>>, ctre::repeat<0,0,ctre::character<'b'>>>;
    using SeqFirstNotNull = ctre::sequence<ctre::character<'a'>, ctre::repeat<0,0,ctre::character<'b'>>>;
    test_nullable<SeqBothNull>("a*.b*", true);
    test_nullable<SeqFirstNotNull>("a.b*", false);

    using SelOneNull = ctre::select<ctre::character<'a'>, ctre::repeat<0,0,ctre::character<'b'>>>;
    using SelNeitherNull = ctre::select<ctre::character<'a'>, ctre::character<'b'>>;
    test_nullable<SelOneNull>("a|b*", true);
    test_nullable<SelNeitherNull>("a|b", false);

    test_nullable<ctre::capture<1, ctre::repeat<0,0,ctre::character<'a'>>>>("(a*)", true);
    test_nullable<ctre::capture<1, ctre::character<'a'>>>("(a)", false);

    std::cout << "\nfirst_positions:\n";
    test_first<ctre::character<'a'>>("'a'", std::array<size_t, 1>{1});
    test_first<ctre::string<'a','b','c'>>("'abc'", std::array<size_t, 1>{1});
    test_first<ctre::any>(".", std::array<size_t, 1>{1});
    test_first<Seq_ab_cd>("'ab'.'cd'", std::array<size_t, 1>{1});
    test_first<Sel_ab_cd>("'ab'|'cd'", std::array<size_t, 2>{1, 3});

    using SeqNullFirst = ctre::sequence<ctre::repeat<0,0,ctre::character<'a'>>, ctre::character<'b'>>;
    test_first<SeqNullFirst>("a*.b", std::array<size_t, 2>{1, 2});

    test_first<ctre::repeat<0,0,ctre::character<'a'>>>("a*", std::array<size_t, 1>{1});
    test_first<Rep_abcd>("(ab|cd)*", std::array<size_t, 2>{1, 3});
    test_first<Hyperscan>("(abc|def).*ghi", std::array<size_t, 2>{1, 4});

    test_first<ctre::capture<1, ctre::string<'a','b'>>>("('ab')", std::array<size_t, 1>{1});
    test_first<ctre::capture<1, ctre::select<ctre::character<'a'>, ctre::character<'b'>>>>("(a|b)", std::array<size_t, 2>{1, 2});

    std::cout << "\nPassed: " << tests_passed << " tests\n";
    return 0;
}

