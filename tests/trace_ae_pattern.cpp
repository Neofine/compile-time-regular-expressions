#include <iostream>
#include <string>
#include "ctre.hpp"

int main() {
    std::string ae_str = "abcdabcdabcdabcdabcdabcdabcdabcd";  // 32 chars, all in [a-e]
    
    std::cout << "Testing [a-e]* pattern (5 chars):\n";
    std::cout << "String: '" << ae_str << "'\n";
    std::cout << "Match result: " << (ctre::match<"[a-e]*">(ae_str) ? "YES" : "NO") << "\n";
    
    // Range size is 5, so it should SKIP the direct comparison (line 303 checks <= 3)
    // and use the regular range comparison code instead
    
    std::cout << "\nExpected: Uses regular range comparison (not direct)\n";
    std::cout << "Performance: Should be ~5ns like [a-z]*, not 62ns!\n";
    
    return 0;
}
