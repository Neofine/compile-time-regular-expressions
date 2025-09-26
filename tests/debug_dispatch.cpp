#include "ctre.hpp"
#include <iostream>
#include <string>

int main() {
    std::cout << "Debug Dispatch Logic\n";
    std::cout << "===================\n\n";
    
    // Test what type of pattern a* is
    std::cout << "Testing pattern type detection:\n";
    std::cout << "------------------------------\n";
    
    // This should help us understand what type of pattern a* is
    std::string test_string = "aaaaaaaa";
    
    std::cout << "Testing a* pattern...\n";
    auto result = ctre::match<"a*">(test_string);
    
    if (result) {
        std::cout << "✅ a* pattern works\n";
        std::cout << "Matched: \"" << result.to_view() << "\"\n";
    } else {
        std::cout << "❌ a* pattern failed\n";
    }
    
    std::cout << "\nTesting [a]* pattern...\n";
    auto result2 = ctre::match<"[a]*">(test_string);
    
    if (result2) {
        std::cout << "✅ [a]* pattern works\n";
        std::cout << "Matched: \"" << result2.to_view() << "\"\n";
    } else {
        std::cout << "❌ [a]* pattern failed\n";
    }
    
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
