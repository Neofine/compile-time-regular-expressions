#include "ctre.hpp"
#include <iostream>
#include <string>

int main() {
    std::cout << "Debug Pattern Type\n";
    std::cout << "==================\n\n";
    
    std::string test_string = "aaaaaaaa";
    
    // Test a* pattern
    std::cout << "Testing a* pattern...\n";
    auto result = ctre::match<"a*">(test_string);
    
    if (result) {
        std::cout << "✅ a* pattern works\n";
        std::cout << "Matched: \"" << result.to_view() << "\"\n";
    } else {
        std::cout << "❌ a* pattern failed\n";
    }
    
    // Test [a]* pattern
    std::cout << "\nTesting [a]* pattern...\n";
    auto result2 = ctre::match<"[a]*">(test_string);
    
    if (result2) {
        std::cout << "✅ [a]* pattern works\n";
        std::cout << "Matched: \"" << result2.to_view() << "\"\n";
    } else {
        std::cout << "❌ [a]* pattern failed\n";
    }
    
    // Test [a-a]* pattern
    std::cout << "\nTesting [a-a]* pattern...\n";
    auto result3 = ctre::match<"[a-a]*">(test_string);
    
    if (result3) {
        std::cout << "✅ [a-a]* pattern works\n";
        std::cout << "Matched: \"" << result3.to_view() << "\"\n";
    } else {
        std::cout << "❌ [a-a]* pattern failed\n";
    }
    
    return 0;
}
