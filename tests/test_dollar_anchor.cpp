#include <ctre.hpp>
#include <cassert>

// Test for $ anchor semantics (assert_subject_end_line)
// Standard regex behavior: $ matches at end of string OR before optional final \n

int main() {
    // Test 1: $ at end of string (no newline)
    assert(ctre::match<"abc$">("abc"));
    
    // Test 2: $ before optional final newline
    // Python/PCRE: $ matches "abc" in "abc\n"
    assert(ctre::match<"abc$">("abc\n"));
    
    // Test 3: $ should NOT match when \n is in middle
    assert(!ctre::match<"abc$">("abc\nxyz"));
    
    // Test 4: match requires start of string, so this should NOT match
    assert(!ctre::match<"xyz$">("abc\nxyz"));
    
    // Test 5: But search should find it
    assert(ctre::search<"xyz$">("abc\nxyz"));
    assert(ctre::search<"xyz$">("abc\nxyz\n"));
    
    // Test 6: Should NOT match when not at end
    assert(!ctre::match<"abc$">("abc\nxyz\n"));
    
    // Test 7: Empty string with just \n
    assert(ctre::match<"$">("\n"));
    assert(ctre::match<"$">(""));
    
    // Test 8: Search mode
    auto s1 = ctre::search<"abc$">("xyz abc");
    assert(s1 && s1.view() == "abc");
    
    auto s2 = ctre::search<"abc$">("xyz abc\n");
    assert(s2 && s2.view() == "abc");
    
    auto s3 = ctre::search<"abc$">("xyz abc\nmore");
    assert(!s3);
    
    return 0;
}
