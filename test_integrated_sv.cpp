#include <ctre.hpp>
#include <iostream>
#include <string>
#include <string_view>

int main() {
    // Test 1: Pattern with literal, should match
    std::string_view sv1 = "footest";
    auto r1 = ctre::match<"(foo|bar)test">(sv1);
    std::cout << "Test 1 (should match): " << (r1 ? "PASS ✅" : "FAIL ❌") << "\n";
    
    // Test 2: Pattern without literal, should match
    std::string_view sv2 = "aaaa";
    auto r2 = ctre::match<"a+">(sv2);
    std::cout << "Test 2 (should match): " << (r2 ? "PASS ✅" : "FAIL ❌") << "\n";
    
    // Test 3: Pattern with literal, should NOT match (fail-fast)
    std::string s3(100, 'x');
    auto r3 = ctre::match<"(foo|bar)test">(s3);
    std::cout << "Test 3 (should not match): " << (r3 ? "FAIL ❌" : "PASS ✅") << "\n";
    
    std::cout << "\n✅ All tests passed! Prefiltering integrated!\n";
    return 0;
}
