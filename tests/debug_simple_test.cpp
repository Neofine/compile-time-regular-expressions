#include <iostream>
#include <string>
#include "ctre.hpp"

int main() {
    std::cout << "Testing a* pattern" << std::endl;
    
    std::string test_string = "aaaa";
    
    // Test a* pattern
    auto result = ctre::match<"a*">(test_string);
    
    if (result) {
        std::cout << "a* pattern matched: " << result.to_string() << std::endl;
    } else {
        std::cout << "a* pattern did not match" << std::endl;
    }
    
    return 0;
}
