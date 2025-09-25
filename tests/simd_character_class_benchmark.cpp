#include <ctre.hpp>
#include <string_view>
#include <iostream>
#include <cassert>
#include <vector>
#include <chrono>

using namespace ctre::literals;
using namespace std::string_view_literals;

// Benchmark function for character class repetition
template <typename Pattern>
void benchmark_character_class(const std::string& name, Pattern pattern, const std::string& test_string, size_t iterations = 1000000) {
    auto start = std::chrono::high_resolution_clock::now();
    
    for (size_t i = 0; i < iterations; ++i) {
        auto result = pattern.match(test_string);
        (void)result; // Suppress unused variable warning
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    
    double avg_time = static_cast<double>(duration.count()) / static_cast<double>(iterations);
    std::cout << "Pattern: " << name << " | String length: " << test_string.length() 
              << " | Avg time: " << avg_time << " ns\n";
}

void benchmark_character_classes() {
    std::cout << "CTRE SIMD Character Class Repetition Benchmark\n";
    std::cout << "=============================================\n\n";
    
    // Test different string lengths and patterns
    std::vector<std::string> test_strings = {
        "0123456789",                    // 10 chars
        "01234567890123456789",          // 20 chars
        "0123456789012345678901234567890123456789", // 40 chars
        "01234567890123456789012345678901234567890123456789012345678901234567890123456789", // 80 chars
        "abcdefghijklmnopqrstuvwxyz",    // 26 chars
        "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz", // 52 chars
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ",    // 26 chars
        "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ", // 52 chars
        "01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789", // 120 chars
        "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz", // 156 chars
    };
    
    std::cout << "Testing digit patterns [0-9]*:\n";
    for (const auto& test_string : test_strings) {
        if (test_string[0] >= '0' && test_string[0] <= '9') {
            benchmark_character_class("[0-9]*", "[0-9]*"_ctre, test_string);
        }
    }
    
    std::cout << "\nTesting lowercase patterns [a-z]*:\n";
    for (const auto& test_string : test_strings) {
        if (test_string[0] >= 'a' && test_string[0] <= 'z') {
            benchmark_character_class("[a-z]*", "[a-z]*"_ctre, test_string);
        }
    }
    
    std::cout << "\nTesting uppercase patterns [A-Z]*:\n";
    for (const auto& test_string : test_strings) {
        if (test_string[0] >= 'A' && test_string[0] <= 'Z') {
            benchmark_character_class("[A-Z]*", "[A-Z]*"_ctre, test_string);
        }
    }
    
    // Test specific patterns with different repetition types
    std::cout << "\nTesting specific repetition patterns:\n";
    
    // Test [0-9]+ (requires at least one digit)
    std::string digits_40 = "0123456789012345678901234567890123456789";
    benchmark_character_class("[0-9]+", "[0-9]+"_ctre, digits_40);
    
    // Test [a-z]+ (requires at least one lowercase letter)
    std::string lower_40 = "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz";
    benchmark_character_class("[a-z]+", "[a-z]+"_ctre, lower_40);
    
    // Test [A-Z]+ (requires at least one uppercase letter)
    std::string upper_40 = "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ";
    benchmark_character_class("[A-Z]+", "[A-Z]+"_ctre, upper_40);
    
    // Test bounded repetition [0-9]{10,20}
    std::string digits_15 = "012345678901234";
    benchmark_character_class("[0-9]{10,20}", "[0-9]{10,20}"_ctre, digits_15);
    
    // Test bounded repetition [a-z]{20,40}
    std::string lower_30 = "abcdefghijklmnopqrstuvwxyzabcd";
    benchmark_character_class("[a-z]{20,40}", "[a-z]{20,40}"_ctre, lower_30);
    
    std::cout << "\n🎉 Character class repetition benchmark complete!\n";
}

int main() {
    try {
        benchmark_character_classes();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
