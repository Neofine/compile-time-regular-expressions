#include <iostream>
#include <chrono>
#include <string>
#include <ctre.hpp>

int main() {
    std::cout << "Simple Performance Test for a* pattern\n";
    std::cout << "=====================================\n";
    
    // Test string with many 'a' characters
    std::string test_string = std::string(1000, 'a');
    
    // Test with SIMD enabled (default)
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 10000; ++i) {
        auto match = ctre::match<"a*">(test_string);
        (void)match; // Prevent optimization
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto simd_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    
    std::cout << "SIMD enabled time: " << simd_time << " ns\n";
    std::cout << "Average per match: " << simd_time / 10000.0 << " ns\n";
    
    return 0;
}
