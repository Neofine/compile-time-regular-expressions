#include "ctre.hpp"
#include <iostream>

// Declare the debug counter
extern int debug_counter;

int main() {
    std::cout << "Debug Counter Test" << std::endl;
    std::cout << "=================" << std::endl;
    
    // Reset counter
    debug_counter = 0;
    std::cout << "Initial counter: " << debug_counter << std::endl;
    
    std::string test_string = "aaaa";
    
    // Test a* pattern
    std::cout << "Testing a* pattern..." << std::endl;
    auto result = ctre::match<"a*">(test_string);
    
    std::cout << "Final counter: " << debug_counter << std::endl;
    
    if (result) {
        std::cout << "✅ a* pattern works" << std::endl;
    } else {
        std::cout << "❌ a* pattern failed" << std::endl;
    }
    
    return 0;
}
