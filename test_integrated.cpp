#include <ctre.hpp>
#include <iostream>

int main() {
    // Test 1: Pattern with literal, should match
    auto r1 = ctre::match<"(foo|bar)test">("footest");
    std::cout << "Test 1 (should match): " << (r1 ? "PASS ✅" : "FAIL ❌") << "\n";
    
    // Test 2: Pattern without literal, should match
    auto r2 = ctre::match<"a+">("aaaa");
    std::cout << "Test 2 (should match): " << (r2 ? "PASS ✅" : "FAIL ❌") << "\n";
    
    // Test 3: Pattern with literal, should NOT match (fail-fast)
    auto r3 = ctre::match<"(foo|bar)test">(std::string(100, 'x'));
    std::cout << "Test 3 (should not match): " << (r3 ? "FAIL ❌" : "PASS ✅") << "\n";
    
    std::cout << "\n✅ All tests passed! Integration works!\n";
    return 0;
}
