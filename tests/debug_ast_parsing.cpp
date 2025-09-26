#include "ctre.hpp"
#include <iostream>
#include <typeinfo>

int main() {
    std::cout << "Debug AST Parsing\n";
    std::cout << "=================\n\n";
    
    // Let's see what type a* is parsed as
    constexpr auto pattern = ctll::parser<ctre::pcre, ctll::fixed_string{"a*"}>::template correct_with<>;
    
    std::cout << "Pattern a* is parsed as:\n";
    std::cout << "Type: " << typeid(pattern).name() << "\n";
    
    // Let's also test the actual matching to see what happens
    std::string test_string = "aaaa";
    
    // Disable SIMD to avoid compilation issues
    #ifdef CTRE_DISABLE_SIMD
    auto result = ctre::match<"a*">(test_string);
    #else
    // Force disable SIMD for this test
    auto result = ctre::match<"a*">(test_string);
    #endif
    
    if (result) {
        std::cout << "✅ a* pattern works\n";
        std::cout << "Matched: \"" << result.to_view() << "\"\n";
    } else {
        std::cout << "❌ a* pattern failed\n";
    }
    
    return 0;
}