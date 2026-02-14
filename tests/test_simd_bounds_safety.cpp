#include <ctre.hpp>
#include <string_view>
#include <cassert>

// Test SIMD bounds checking for short strings
// The SIMD code should not read past the end of the buffer

int main() {
    // Test 1: Very short string (< 16 bytes)
    std::string_view s1 = "aaaa";
    auto r1 = ctre::match<"a+">(s1);
    assert(r1);
    
    // Test 2: Exactly 15 bytes (one less than SSE width)
    std::string_view s2 = "aaaaaaaaaaaaaaa";  // 15 'a's
    auto r2 = ctre::match<"a+">(s2);
    assert(r2);
    
    // Test 3: Exactly 31 bytes (one less than AVX2 width)
    std::string_view s3 = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";  // 31 'a's
    auto r3 = ctre::match<"a+">(s3);
    assert(r3);
    
    // Test 4: Search in short string
    std::string_view s4 = "xaaay";
    auto r4 = ctre::search<"a+">(s4);
    assert(r4 && r4.view() == "aaa");
    
    // Test 5: Character class repetition on short string
    std::string_view s5 = "abc123xyz";
    auto r5 = ctre::search<"[0-9]+">(s5);
    assert(r5 && r5.view() == "123");
    
    // Test 6: Short string with mixed content
    std::string_view s6 = "abc";
    auto r6 = ctre::search<"b+">(s6);
    assert(r6 && r6.view() == "b");
    
    return 0;
}
