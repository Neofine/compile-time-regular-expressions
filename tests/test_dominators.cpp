#include <ctre.hpp>
#include "../include/ctre/decomposition.hpp"
#include <iostream>
#include <cassert>

namespace test_dominators {
    using namespace ctre::dominators;
    using namespace ctre::glushkov;

    constexpr bool contains(const auto& dom_set, size_t value) {
        for (size_t i = 0; i < dom_set.count; ++i) {
            if (dom_set.dominators[i] == value) return true;
        }
        return false;
    }

    // Reachability tests
    using Pat_String = ctre::string<'a', 'b', 'c'>;
    constexpr auto nfa1 = glushkov_nfa<Pat_String>();
    static_assert(is_reachable(nfa1, 0, 1), "Start can reach position 1");
    static_assert(is_reachable(nfa1, 0, 3), "Start can reach position 3 (accept)");
    static_assert(is_reachable(nfa1, 1, 2), "Position 1 can reach position 2");
    static_assert(is_reachable(nfa1, 1, 3), "Position 1 can reach position 3");
    static_assert(!is_reachable(nfa1, 2, 1), "Position 2 cannot reach position 1");

    // Dominators for simple string
    constexpr auto doms1 = find_dominators(nfa1);
    static_assert(doms1.count == 3, "String 'abc' has 3 dominators");
    static_assert(contains(doms1, 1), "Position 1 ('a') is a dominator");
    static_assert(contains(doms1, 2), "Position 2 ('b') is a dominator");
    static_assert(contains(doms1, 3), "Position 3 ('c') is a dominator");

    // Dominators for select (should have none)
    using Pat_Select = ctre::select<ctre::string<'a', 'b'>, ctre::string<'c', 'd'>>;
    constexpr auto nfa2 = glushkov_nfa<Pat_Select>();
    constexpr auto doms2 = find_dominators(nfa2);
    static_assert(doms2.count == 0, "Select 'ab|cd' has no dominators");

    // Dominators for Hyperscan paper pattern: (abc|def).*ghi
    using Pat_Hyperscan = ctre::sequence<
        ctre::select<ctre::string<'a','b','c'>, ctre::string<'d','e','f'>>,
        ctre::star<ctre::any>,
        ctre::string<'g','h','i'>
    >;
    constexpr auto nfa3 = glushkov_nfa<Pat_Hyperscan>();
    constexpr auto doms3 = find_dominators(nfa3);
    static_assert(doms3.count >= 3, "Hyperscan pattern has at least 3 dominators");
    static_assert(contains(doms3, 8), "Position 8 ('g') is a dominator");
    static_assert(contains(doms3, 9), "Position 9 ('h') is a dominator");
    static_assert(contains(doms3, 10), "Position 10 ('i') is a dominator");

    // Literal extraction
    constexpr auto lit1 = extract_literal_from_dominators(nfa1);
    static_assert(lit1.has_literal, "String 'abc' has extractable literal");
    static_assert(lit1.length == 3, "Literal length is 3");
    static_assert(lit1.chars[0] == 'a');
    static_assert(lit1.chars[1] == 'b');
    static_assert(lit1.chars[2] == 'c');

    // Hyperscan literal extraction
    constexpr auto lit3 = extract_literal_from_dominators(nfa3);
    static_assert(lit3.has_literal, "Hyperscan pattern has extractable literal");
    static_assert(lit3.length == 3, "Literal 'ghi' has length 3");
    static_assert(lit3.chars[0] == 'g');
    static_assert(lit3.chars[1] == 'h');
    static_assert(lit3.chars[2] == 'i');

    // No extractable literal for alternation
    constexpr auto lit2 = extract_literal_from_dominators(nfa2);
    static_assert(!lit2.has_literal, "Select 'ab|cd' has no extractable literal");
}

int main() {
    std::cout << "=== Dominator Analysis Tests ===\n\n";

    // Runtime verification of Hyperscan pattern
    using Pat = ctre::sequence<
        ctre::select<ctre::string<'a','b','c'>, ctre::string<'d','e','f'>>,
        ctre::star<ctre::any>,
        ctre::string<'g','h','i'>
    >;
    constexpr auto nfa = ctre::glushkov::glushkov_nfa<Pat>();
    constexpr auto doms = ctre::dominators::find_dominators(nfa);

    std::cout << "Pattern: (abc|def).*ghi\n";
    std::cout << "Dominators found: " << doms.count << "\n";
    for (size_t i = 0; i < doms.count; ++i) {
        size_t pos = doms.dominators[i];
        std::cout << "  Position " << pos << ": '" << nfa.states[pos].symbol << "'\n";
    }

    constexpr auto lit = ctre::dominators::extract_literal_from_dominators(nfa);
    std::cout << "Extracted literal: \"";
    for (size_t i = 0; i < lit.length; ++i) {
        std::cout << lit.chars[i];
    }
    std::cout << "\"\n";

    std::cout << "\nAll compile-time assertions passed.\n";
    return 0;
}
