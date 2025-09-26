#include "ctre.hpp"
#include <iostream>
#include <chrono>
#include <vector>
#include <string>


// Benchmark function that prevents compiler optimizations
template<typename Func>
double benchmark(Func func, const std::string& name, int iterations = 1000000, int rounds = 5) {
    (void)name; // Suppress unused parameter warning
    
    double total_time = 0.0;
    
    // Run multiple rounds and take the average for more accurate results
    for (int round = 0; round < rounds; ++round) {
        auto start = std::chrono::high_resolution_clock::now();
        volatile bool result = false; // Use volatile to prevent optimization
        for (int i = 0; i < iterations; ++i) {
            result = func();
        }
        (void)result; // Suppress unused variable warning
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::nano> duration = end - start;
        total_time += duration.count() / static_cast<double>(iterations);
    }
    
    return total_time / static_cast<double>(rounds);
}

int main() {
    std::cout << "CTRE SIMD Repetition Pattern Benchmark" << std::endl;
    std::cout << "======================================" << std::endl;
    std::cout << std::endl;
    
    // Test various repetition patterns with different string lengths
    std::vector<std::pair<std::string, std::string>> test_cases = {
        // Pattern, Test String
        {"a*", "aaaaaaaaaaaaaaaa"},  // 16 a's
        {"a*", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"},  // 32 a's
        {"a*", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"},  // 64 a's
        {"a*", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"},  // 128 a's
        
        {"a+", "aaaaaaaaaaaaaaaa"},  // 16 a's
        {"a+", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"},  // 32 a's
        {"a+", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"},  // 64 a's
        {"a+", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"},  // 128 a's
        
        {"a{10,20}", "aaaaaaaaaaaaaaaa"},  // 16 a's
        {"a{10,20}", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"},  // 32 a's
        {"a{10,20}", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"},  // 64 a's
        {"a{10,20}", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"},  // 128 a's
        
        {"a{50,100}", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"},  // 64 a's
        {"a{50,100}", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"},  // 128 a's
        
        // From x to infinity patterns (should benefit greatly from SIMD)
        {"a{10,}", "aaaaaaaaaaaaaaaa"},  // 16 a's
        {"a{10,}", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"},  // 32 a's
        {"a{10,}", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"},  // 64 a's
        {"a{10,}", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"},  // 128 a's
        
        {"a{50,}", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"},  // 64 a's
        {"a{50,}", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"},  // 128 a's
        
        // Non-matching cases
        {"a*", "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"},  // 32 b's
        {"a+", "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"},  // 32 b's
        {"a{10,20}", "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"},  // 32 b's
        {"a{10,}", "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"},  // 32 b's
    };
    
    for (const auto& [pattern, test_string] : test_cases) {
        std::cout << "Testing pattern: " << pattern << " against " << test_string.length() << "-character string" << std::endl;
        
        // Test matching case with multiple rounds for accuracy
        double match_time = benchmark([&]() {
            if (pattern == "a*") {
                return static_cast<bool>(ctre::match<"a*">(test_string));
            } else if (pattern == "a+") {
                return static_cast<bool>(ctre::match<"a+">(test_string));
            } else if (pattern == "a{10,20}") {
                return static_cast<bool>(ctre::match<"a{10,20}">(test_string));
            } else if (pattern == "a{50,100}") {
                return static_cast<bool>(ctre::match<"a{50,100}">(test_string));
            } else if (pattern == "a{10,}") {
                return static_cast<bool>(ctre::match<"a{10,}">(test_string));
            } else if (pattern == "a{50,}") {
                return static_cast<bool>(ctre::match<"a{50,}">(test_string));
            }
            return false;
        }, "Match", 1000000, 10); // 10 rounds for better accuracy
        
        std::cout << "  Match time: " << match_time << " ns" << std::endl;
        std::cout << std::endl;
    }
    
    return 0;
}
