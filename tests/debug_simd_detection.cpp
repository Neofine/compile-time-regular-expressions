#include "ctre.hpp"
#include <iostream>
#include <string>

int main() {
    std::cout << "Debug SIMD Detection\n";
    std::cout << "===================\n\n";
    
    // Check SIMD capabilities
    std::cout << "SIMD Detection Results:\n";
    std::cout << "----------------------\n";
    
    if (ctre::simd::can_use_simd()) {
        std::cout << "✅ SIMD is available\n";
        
        auto capability = ctre::simd::get_simd_capability();
        std::cout << "SIMD Capability: " << capability << "\n";
        
        if (capability >= ctre::simd::SIMD_CAPABILITY_AVX2) {
            std::cout << "✅ AVX2 is available\n";
        } else if (capability >= ctre::simd::SIMD_CAPABILITY_SSE42) {
            std::cout << "✅ SSE4.2 is available\n";
        } else {
            std::cout << "❌ No advanced SIMD available\n";
        }
    } else {
        std::cout << "❌ SIMD is NOT available\n";
    }
    
    std::cout << "\n";
    
    // Test if std::is_constant_evaluated() is working
    std::cout << "Constant Evaluation Check:\n";
    std::cout << "-------------------------\n";
    
    if (std::is_constant_evaluated()) {
        std::cout << "❌ Running in constant evaluation context (SIMD disabled)\n";
    } else {
        std::cout << "✅ Running in runtime context (SIMD should be enabled)\n";
    }
    
    std::cout << "\n";
    
    // Test a simple pattern to see if SIMD path is taken
    std::cout << "Pattern Testing:\n";
    std::cout << "---------------\n";
    
    std::string test_string = "aaaaaaaaaaaaaaaa"; // 16 a's
    
    std::cout << "Testing a* against: \"" << test_string << "\"\n";
    auto result = ctre::match<"a*">(test_string);
    
    if (result) {
        std::cout << "✅ Match found: \"" << result.to_view() << "\"\n";
    } else {
        std::cout << "❌ No match found\n";
    }
    
    std::cout << "\nTesting [a]* against: \"" << test_string << "\"\n";
    auto result2 = ctre::match<"[a]*">(test_string);
    
    if (result2) {
        std::cout << "✅ Match found: \"" << result2.to_view() << "\"\n";
    } else {
        std::cout << "❌ No match found\n";
    }
    
    return 0;
}
