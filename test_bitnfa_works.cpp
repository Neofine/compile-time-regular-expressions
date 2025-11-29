#include <ctre.hpp>
#include <ctre/bitnfa/integration.hpp>
#include <iostream>

int main() {
    std::string input1 = "Huckleberry";
    std::string input2 = "Sawyer";
    std::string input3 = "nothing";
    
    // Test BitNFA directly
    auto r1 = ctre::bitnfa::match<"Huck[a-zA-Z]+|Saw[a-zA-Z]+">(input1);
    auto r2 = ctre::bitnfa::match<"Huck[a-zA-Z]+|Saw[a-zA-Z]+">(input2);
    auto r3 = ctre::bitnfa::match<"Huck[a-zA-Z]+|Saw[a-zA-Z]+">(input3);
    
    std::cout << "BitNFA test:" << std::endl;
    std::cout << "  'Huckleberry' matches: " << r1.matched << std::endl;
    std::cout << "  'Sawyer' matches: " << r2.matched << std::endl;
    std::cout << "  'nothing' matches: " << r3.matched << std::endl;
    
    if (r1.matched && r2.matched && !r3.matched) {
        std::cout << "\n✅ BitNFA works correctly!" << std::endl;
        return 0;
    } else {
        std::cout << "\n❌ BitNFA not working" << std::endl;
        return 1;
    }
}
