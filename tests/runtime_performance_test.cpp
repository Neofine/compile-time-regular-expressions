#include <iostream>
#include <chrono>
#include <string>
#include <ctre.hpp>
#include <random>

int main() {
    std::cout << "Runtime Performance Test for a* pattern\n";
    std::cout << "======================================\n";
    
    // Generate random strings at runtime to prevent compile-time evaluation
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 1); // 0 or 1
    
    // Create test strings at runtime - all 'a' characters to match a*
    std::vector<std::string> test_strings;
    for (int i = 0; i < 100; ++i) {
        std::string str;
        for (int j = 0; j < 1000; ++j) {
            str += 'a';  // All 'a' characters to ensure a* matches
        }
        test_strings.push_back(str);
    }
    
    std::cout << "Generated " << test_strings.size() << " random test strings\n";
    
    // Test with SIMD enabled (default)
    auto start = std::chrono::high_resolution_clock::now();
    int match_count = 0;
    for (int i = 0; i < 1000; ++i) {
        for (const auto& test_string : test_strings) {
            auto match = ctre::match<"a*">(test_string);
            if (match) match_count++;
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto simd_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    
    std::cout << "SIMD enabled time: " << simd_time << " ns\n";
    std::cout << "Total matches: " << match_count << "\n";
    std::cout << "Average per match: " << simd_time / (double)match_count << " ns\n";
    
    return 0;
}
