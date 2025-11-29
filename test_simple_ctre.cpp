#include <ctre.hpp>
#include <iostream>

int main() {
    std::string input = "aaaa";
    
    // Just standard ctre::match
    auto result = ctre::match<"a+">(input);
    
    std::cout << "Standard CTRE works: " << (bool)result << std::endl;
    
    // Check: does ctre::match internally use any BitNFA or smart_dispatch?
    // Or is it purely the base evaluation.hpp template recursion?
    
    return 0;
}
