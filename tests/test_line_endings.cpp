#include <ctre.hpp>
#include <cassert>

// Test for line ending support in multiline mode
// In multiline mode, . (any) should NOT match line endings: \n, \r, \r\n

int main() {
    // Singleline mode (default): . matches everything including line endings
    assert(ctre::match<"a.c">("a\nc"));
    assert(ctre::match<"a.c">("axc"));
    
    // Multiline mode: . should NOT match \n
    assert(!ctre::multiline_match<"a.c">("a\nc"));
    assert(ctre::multiline_match<"a.c">("axc"));
    
    // CURRENT BEHAVIOR (will fail): multiline . doesn't reject \r or \r\n
    // These should fail but currently pass:
    
    // Test \r (carriage return)
    assert(!ctre::multiline_match<"a.c">("a\rc"));  // Should NOT match
    
    // Test \r\n (Windows line ending) - . should not match \r
    assert(!ctre::multiline_match<"a.c">("a\r\nc"));  // Should NOT match (. can't match \r)
    
    // Additional tests with search
    auto s1 = ctre::multiline_search<"a.c">("a\rc");
    assert(!s1);  // Should NOT find match in multiline mode
    
    auto s2 = ctre::multiline_search<"a.c">("a\nc");
    assert(!s2);  // Should NOT find match in multiline mode
    
    auto s3 = ctre::search<"a.c">("a\rc");
    assert(s3);  // Should find in singleline mode
    
    return 0;
}
