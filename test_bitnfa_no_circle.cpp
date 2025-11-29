#include <ctre.hpp>
#include <ctre/bitnfa/integration.hpp>
#include <iostream>

int main() {
    // Test alternation
    auto r1 = ctre::bitnfa::match<"Huck[a-zA-Z]+|Saw[a-zA-Z]+">("Huckleberry");
    std::cout << "BitNFA alternation: " << r1.matched << std::endl;
    
    // Test repetition 
    auto r2 = ctre::bitnfa::match<"[a-z]+">("aaaa");
    std::cout << "BitNFA repetition: " << r2.matched << std::endl;
    
    return (r1.matched && r2.matched) ? 0 : 1;
}
