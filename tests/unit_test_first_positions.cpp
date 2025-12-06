// Unit Test: first_positions() function
#include <ctre.hpp>
#include <iostream>
#include <cassert>
#include <algorithm>

template <size_t N>
bool contains(const std::array<size_t, N>& arr, size_t count, size_t pos) {
    return std::find(arr.begin(), arr.begin() + count, pos) != (arr.begin() + count);
}

template <typename Pattern, size_t N>
void test_first(const char* name, const std::array<size_t, N>& expected) {
    std::cout << "  " << name << ": ";
    constexpr auto result = ctre::glushkov::first_positions<Pattern>(0);
    const auto& arr = result.first;
    const auto count = result.second;
    assert(count == expected.size());
    for (auto pos : expected) {
        assert(contains(arr, count, pos));
    }
    std::cout << "✓ {";
    for (size_t i = 0; i < count; ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << arr[i];
    }
    std::cout << "}\n";
}

int main() {
    std::cout << "=== Unit Test: first_positions() ===\n\n";

    // Test 1: Single elements
    test_first<ctre::character<'a'>>("character 'a'", std::array<size_t, 1>{1});
    test_first<ctre::any>("any (.)", std::array<size_t, 1>{1});

    // Test 2: String
    test_first<ctre::string<'a','b','c'>>("string 'abc'", std::array<size_t, 1>{1});
    test_first<ctre::string<'h','e','l','l','o'>>("string 'hello'", std::array<size_t, 1>{1});

    // Test 3: Empty (should be empty set)
    std::cout << "  empty: ";
    constexpr auto empty_result = ctre::glushkov::first_positions<ctre::empty>(0);
    assert(empty_result.second == 0);
    std::cout << "✓ {}\n";

    // Test 4: Sequence - first of first element (if not nullable)
    using Seq1 = ctre::sequence<ctre::string<'a','b'>, ctre::string<'c','d'>>;
    test_first<Seq1>("sequence 'ab'.'cd'", std::array<size_t, 1>{1});

    // Test 5: Sequence - both if first is nullable
    using Seq2 = ctre::sequence<ctre::repeat<0,0,ctre::character<'a'>>, ctre::character<'b'>>;
    test_first<Seq2>("sequence 'a*'.'b'", std::array<size_t, 2>{1, 2});

    // Test 6: Select - union of both
    using Sel1 = ctre::select<ctre::string<'a','b'>, ctre::string<'c','d'>>;
    test_first<Sel1>("select 'ab'|'cd'", std::array<size_t, 2>{1, 3});

    using Sel2 = ctre::select<ctre::character<'a'>, ctre::character<'b'>, ctre::character<'c'>>;
    test_first<Sel2>("select 'a'|'b'|'c'", std::array<size_t, 3>{1, 2, 3});

    // Test 7: Repeat
    test_first<ctre::repeat<0,0,ctre::character<'a'>>>("repeat 'a'*", std::array<size_t, 1>{1});
    test_first<ctre::repeat<0,0,ctre::string<'a','b','c'>>>("repeat 'abc'*", std::array<size_t, 1>{1});

    // Test 8: Complex patterns
    using Rep1 = ctre::repeat<0,0, ctre::select<ctre::string<'a','b'>, ctre::string<'c','d'>>>;
    test_first<Rep1>("(ab|cd)*", std::array<size_t, 2>{1, 3});

    using Complex = ctre::sequence<
        ctre::select<ctre::string<'a','b','c'>, ctre::string<'d','e','f'>>,
        ctre::repeat<0,0,ctre::any>,
        ctre::string<'g','h','i'>>;
    test_first<Complex>("(abc|def).*ghi", std::array<size_t, 2>{1, 4});

    // Test 9: Capture
    test_first<ctre::capture<1, ctre::string<'a','b'>>>("capture<1, 'ab'>", std::array<size_t, 1>{1});

    using Cap1 = ctre::capture<1, ctre::select<ctre::character<'a'>, ctre::character<'b'>>>;
    test_first<Cap1>("capture<1, 'a'|'b'>", std::array<size_t, 2>{1, 2});

    std::cout << "\n✓ All 13 first_positions tests passed!\n";
    return 0;
}
