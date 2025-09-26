#include "ctre.hpp"
#include <iostream>
#include <string>

int main() {
    std::cout << "Debug Single Character SIMD\n";
    std::cout << "==========================\n\n";
    
    std::string test_string = "aaaaaaaaaaaaaaaa"; // 16 a's
    
    // Test a* pattern
    std::cout << "Testing a* pattern against: \"" << test_string << "\"\n";
    
    auto result = ctre::match<"a*">(test_string);
    
    if (result) {
        std::cout << "✅ Match found!\n";
        std::cout << "Matched: \"" << result.to_view() << "\"\n";
        std::cout << "Match length: " << result.to_view().length() << "\n";
    } else {
        std::cout << "❌ No match found!\n";
    }
    
    // Test [a]* pattern for comparison
    std::cout << "\nTesting [a]* pattern against: \"" << test_string << "\"\n";
    
    auto result2 = ctre::match<"[a]*">(test_string);
    
    if (result2) {
        std::cout << "✅ Match found!\n";
        std::cout << "Matched: \"" << result2.to_view() << "\"\n";
        std::cout << "Match length: " << result2.to_view().length() << "\n";
    } else {
        std::cout << "❌ No match found!\n";
    }
    
    // Test a+ pattern
    std::cout << "\nTesting a+ pattern against: \"" << test_string << "\"\n";
    
    auto result3 = ctre::match<"a+">(test_string);
    
    if (result3) {
        std::cout << "✅ Match found!\n";
        std::cout << "Matched: \"" << result3.to_view() << "\"\n";
        std::cout << "Match length: " << result3.to_view().length() << "\n";
    } else {
        std::cout << "❌ No match found!\n";
    }
    
    return 0;
}