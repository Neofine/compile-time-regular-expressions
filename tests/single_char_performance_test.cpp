#include "ctre.hpp"
#include <iostream>
#include <chrono>
#include <string>

int main() {
    std::cout << "Single Character Performance Comparison Test\n";
    std::cout << "============================================\n\n";
    
    // Test single character patterns vs character class patterns
    std::string test_string = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"; // 32 a's
    
    const int iterations = 1000000;
    
    // Test a* (single character)
    auto start = std::chrono::high_resolution_clock::now();
    volatile long long a_star_matches = 0;
    for (int i = 0; i < iterations; ++i) {
        if ("a*"_ctre.match(test_string)) {
            a_star_matches++;
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto a_star_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double a_star_avg = static_cast<double>(a_star_time.count()) / static_cast<double>(iterations);
    
    // Test [a]* (character class)
    start = std::chrono::high_resolution_clock::now();
    volatile long long a_class_matches = 0;
    for (int i = 0; i < iterations; ++i) {
        if ("[a]*"_ctre.match(test_string)) {
            a_class_matches++;
        }
    }
    end = std::chrono::high_resolution_clock::now();
    auto a_class_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double a_class_avg = static_cast<double>(a_class_time.count()) / static_cast<double>(iterations);
    
    // Test a+ (single character)
    start = std::chrono::high_resolution_clock::now();
    volatile long long a_plus_matches = 0;
    for (int i = 0; i < iterations; ++i) {
        if ("a+"_ctre.match(test_string)) {
            a_plus_matches++;
        }
    }
    end = std::chrono::high_resolution_clock::now();
    auto a_plus_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double a_plus_avg = static_cast<double>(a_plus_time.count()) / static_cast<double>(iterations);
    
    // Test [a]+ (character class)
    start = std::chrono::high_resolution_clock::now();
    volatile long long a_class_plus_matches = 0;
    for (int i = 0; i < iterations; ++i) {
        if ("[a]+"_ctre.match(test_string)) {
            a_class_plus_matches++;
        }
    }
    end = std::chrono::high_resolution_clock::now();
    auto a_class_plus_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    double a_class_plus_avg = static_cast<double>(a_class_plus_time.count()) / static_cast<double>(iterations);
    
    std::cout << "Test string: \"" << test_string << "\" (32 characters)\n";
    std::cout << "Iterations: " << iterations << "\n\n";
    
    std::cout << "Results:\n";
    std::cout << "--------\n";
    std::cout << "a* (single char):     " << a_star_avg << " ns (matches: " << a_star_matches << ")\n";
    std::cout << "[a]* (char class):    " << a_class_avg << " ns (matches: " << a_class_matches << ")\n";
    std::cout << "a+ (single char):     " << a_plus_avg << " ns (matches: " << a_plus_matches << ")\n";
    std::cout << "[a]+ (char class):    " << a_class_plus_avg << " ns (matches: " << a_class_plus_matches << ")\n\n";
    
    // Check if results are consistent
    bool results_consistent = (a_star_matches == a_class_matches) && 
                             (a_plus_matches == a_class_plus_matches) &&
                             (a_star_matches > 0) && (a_plus_matches > 0);
    
    if (results_consistent) {
        std::cout << "✅ All patterns produce consistent results!\n";
        
        // Compare performance
        if (a_star_avg < a_class_avg) {
            double speedup = a_class_avg / a_star_avg;
            std::cout << "🚀 Single character a* is " << speedup << "x faster than [a]*\n";
        } else {
            double speedup = a_star_avg / a_class_avg;
            std::cout << "🚀 Character class [a]* is " << speedup << "x faster than a*\n";
        }
        
        if (a_plus_avg < a_class_plus_avg) {
            double speedup = a_class_plus_avg / a_plus_avg;
            std::cout << "🚀 Single character a+ is " << speedup << "x faster than [a]+\n";
        } else {
            double speedup = a_plus_avg / a_class_plus_avg;
            std::cout << "🚀 Character class [a]+ is " << speedup << "x faster than a+\n";
        }
        
        return 0;
    } else {
        std::cout << "❌ Results are inconsistent! This indicates a problem.\n";
        return 1;
    }
}
