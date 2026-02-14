#include <ctre.hpp>
#include <string_view>
#include <cassert>

// Test for potential buffer overrun in shufti.hpp line 694-695
// Bug: for (int i = 0; i < 16 && rem > 0; ++i) { ... p[i] ... }
// Problem: Accesses p[i] even when i >= rem

int main() {
    // Test with 17 bytes - triggers SSE path with rem=17, then rem=1
    // After first 16 bytes processed, rem=1 but loop tries to check 16 bytes
    std::string_view s1 = "abcdefghijklmnopq";  // 17 bytes
    auto r1 = ctre::match<"[a-z]+">(s1);
    assert(r1);
    
    // Test with 18 bytes
    std::string_view s2 = "abcdefghijklmnopqr";  // 18 bytes  
    auto r2 = ctre::match<"[a-z]+">(s2);
    assert(r2);
    
    // Test with search
    std::string_view s3 = "123abcdefghijklmnopq456";
    auto r3 = ctre::search<"[a-z]+">(s3);
    assert(r3);
    
    return 0;
}
