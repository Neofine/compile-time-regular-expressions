#include "ctre.hpp"
#include <fstream>
#include <iostream>

int main() {
    std::cout << "Creating debug file test..." << std::endl;
    
    // Create a debug file
    std::ofstream debug_file("debug_output.txt");
    debug_file << "Starting debug test\n";
    debug_file.flush();
    
    std::string test_string = "aaaa";
    
    // Test a* pattern
    debug_file << "Testing a* pattern\n";
    debug_file.flush();
    
    auto result = ctre::match<"a*">(test_string);
    
    debug_file << "Pattern test completed\n";
    debug_file.flush();
    
    if (result) {
        std::cout << "✅ a* pattern works" << std::endl;
        debug_file << "Match found: " << result.to_view() << "\n";
    } else {
        std::cout << "❌ a* pattern failed" << std::endl;
        debug_file << "No match found\n";
    }
    
    debug_file.close();
    std::cout << "Debug output written to debug_output.txt" << std::endl;
    
    return 0;
}
