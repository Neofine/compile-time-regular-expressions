#include <ctre.hpp>
#include "ctre/region_analysis.hpp"
#include "ctre/glushkov_nfa.hpp"
#include <cassert>

// Test that region analysis correctly handles character sets

int main() {
    // Test 1: Pure literal string "abc" - should extract
    constexpr auto nfa1 = ctre::glushkov::glushkov_nfa<
        ctre::sequence<ctre::character<'a'>, ctre::character<'b'>, ctre::character<'c'>>>();
    
    constexpr auto result1 = ctre::region::extract_literal_from_regions(nfa1);
    static_assert(result1.has_literal, "Should extract literal from 'abc'");
    static_assert(result1.length == 3, "Should extract 3 characters");
    
    // Test 2: Pattern with character set "a[0-9]b" - should NOT extract
    // because [0-9] is not a literal
    constexpr auto nfa2 = ctre::glushkov::glushkov_nfa<
        ctre::sequence<ctre::character<'a'>, ctre::char_range<'0','9'>, ctre::character<'b'>>>();
    
    constexpr auto result2 = ctre::region::extract_literal_from_regions(nfa2);
    // CURRENT BUG: This would incorrectly extract "a?b" 
    // AFTER FIX: Should not extract because [0-9] is not a literal
    static_assert(!result2.has_literal, "Should NOT extract from 'a[0-9]b'");
    
    // Test 3: Just a character range [a-z] - definitely should not extract
    constexpr auto nfa3 = ctre::glushkov::glushkov_nfa<ctre::char_range<'a','z'>>();
    
    constexpr auto result3 = ctre::region::extract_literal_from_regions(nfa3);
    static_assert(!result3.has_literal, "Should NOT extract from '[a-z]'");
    
    // Test 4: Character set at end: "ab[xyz]" - should not extract
    constexpr auto nfa4 = ctre::glushkov::glushkov_nfa<
        ctre::sequence<ctre::character<'a'>, ctre::character<'b'>, 
                      ctre::set<ctre::character<'x'>, ctre::character<'y'>, ctre::character<'z'>>>>();
    
    constexpr auto result4 = ctre::region::extract_literal_from_regions(nfa4);
    static_assert(!result4.has_literal, "Should NOT extract from 'ab[xyz]'");
    
    return 0;
}
