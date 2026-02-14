#include <ctre.hpp>
#include <string>
#include <cassert>

int main() {
    // Test with string_view (not null-terminated)
    // This should use proper end iterator, not sentinel
    std::string_view sv1 = "test pattern";
    auto r1 = ctre::match<"[a-z ]+">(sv1);
    assert(r1);
    
    // Test with std::string (null-terminated but might not be aligned)
    std::string s2 = "x";  // Minimal string
    auto r2 = ctre::match<"[a-z]">(s2);
    assert(r2);
    
    // Test with char* (null-terminated)
    const char* s3 = "test";
    auto r3 = ctre::match<"[a-z]+">(s3);
    assert(r3);
    
    return 0;
}
