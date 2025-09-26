#include <ctre.hpp>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

using namespace std::chrono;

// Simple test data generation without random
std::string generate_test_string(size_t length, const std::string& charset) {
    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result += charset[i % charset.size()];
    }
    return result;
}

// Benchmark template
template<typename Func>
double benchmark_function(Func&& func, int iterations = 1000000) {
    auto start = high_resolution_clock::now();
    volatile bool result = false; // Prevent optimization
    for (int i = 0; i < iterations; ++i) {
        result = func();
    }
    (void)result; // Suppress unused variable warning
    auto end = high_resolution_clock::now();
    
    auto duration = duration_cast<nanoseconds>(end - start);
    return static_cast<double>(duration.count()) / iterations;
}

void print_header(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "  " << title << "\n";
    std::cout << std::string(60, '=') << "\n";
}

void print_table_header() {
    std::cout << "Pattern     | Length | Time (ns) | Result\n";
    std::cout << "------------|--------|-----------|--------\n";
}

int main() {
    std::cout << "🚀 CTRE SIMD vs Non-SIMD Benchmark\n";
    std::cout << "===================================\n";
    
    // Test single character patterns
    print_header("SINGLE CHARACTER REPETITION PATTERNS");
    print_table_header();
    
    std::vector<size_t> lengths = {16, 32, 64, 128};
    std::string test_str_a = generate_test_string(64, "aaaaaaaaaaaaaaaa");
    std::string test_str_0 = generate_test_string(64, "0000000000000000");
    
    // Test a* pattern
    for (size_t len : lengths) {
        std::string test_str = test_str_a.substr(0, len);
        auto func = [&]() { return ctre::match<"a*">(test_str); };
        double time_ns = benchmark_function(func);
        bool result = ctre::match<"a*">(test_str);
        std::cout << "a*         | " << std::setw(6) << len 
                  << " | " << std::setw(9) << std::fixed << std::setprecision(2) << time_ns
                  << " | " << (result ? "✓" : "✗") << "\n";
    }
    
    // Test a+ pattern
    for (size_t len : lengths) {
        std::string test_str = test_str_a.substr(0, len);
        auto func = [&]() { return ctre::match<"a+">(test_str); };
        double time_ns = benchmark_function(func);
        bool result = ctre::match<"a+">(test_str);
        std::cout << "a+         | " << std::setw(6) << len 
                  << " | " << std::setw(9) << std::fixed << std::setprecision(2) << time_ns
                  << " | " << (result ? "✓" : "✗") << "\n";
    }
    
    // Test a{5,10} pattern
    for (size_t len : lengths) {
        std::string test_str = test_str_a.substr(0, len);
        auto func = [&]() { return ctre::match<"a{5,10}">(test_str); };
        double time_ns = benchmark_function(func);
        bool result = ctre::match<"a{5,10}">(test_str);
        std::cout << "a{5,10}    | " << std::setw(6) << len 
                  << " | " << std::setw(9) << std::fixed << std::setprecision(2) << time_ns
                  << " | " << (result ? "✓" : "✗") << "\n";
    }
    
    // Test character class patterns
    print_header("CHARACTER CLASS REPETITION PATTERNS");
    print_table_header();
    
    std::string test_str_digits = generate_test_string(64, "0123456789");
    std::string test_str_lower = generate_test_string(64, "abcdefghijklmnopqrstuvwxyz");
    std::string test_str_upper = generate_test_string(64, "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    std::string test_str_mixed = generate_test_string(64, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
    
    // Test [0-9]* pattern
    for (size_t len : lengths) {
        std::string test_str = test_str_digits.substr(0, len);
        auto func = [&]() { return ctre::match<"[0-9]*">(test_str); };
        double time_ns = benchmark_function(func);
        bool result = ctre::match<"[0-9]*">(test_str);
        std::cout << "[0-9]*     | " << std::setw(6) << len 
                  << " | " << std::setw(9) << std::fixed << std::setprecision(2) << time_ns
                  << " | " << (result ? "✓" : "✗") << "\n";
    }
    
    // Test [a-z]* pattern
    for (size_t len : lengths) {
        std::string test_str = test_str_lower.substr(0, len);
        auto func = [&]() { return ctre::match<"[a-z]*">(test_str); };
        double time_ns = benchmark_function(func);
        bool result = ctre::match<"[a-z]*">(test_str);
        std::cout << "[a-z]*     | " << std::setw(6) << len 
                  << " | " << std::setw(9) << std::fixed << std::setprecision(2) << time_ns
                  << " | " << (result ? "✓" : "✗") << "\n";
    }
    
    // Test [A-Z]* pattern
    for (size_t len : lengths) {
        std::string test_str = test_str_upper.substr(0, len);
        auto func = [&]() { return ctre::match<"[A-Z]*">(test_str); };
        double time_ns = benchmark_function(func);
        bool result = ctre::match<"[A-Z]*">(test_str);
        std::cout << "[A-Z]*     | " << std::setw(6) << len 
                  << " | " << std::setw(9) << std::fixed << std::setprecision(2) << time_ns
                  << " | " << (result ? "✓" : "✗") << "\n";
    }
    
    // Test [a-zA-Z]* pattern
    for (size_t len : lengths) {
        std::string test_str = test_str_mixed.substr(0, len);
        auto func = [&]() { return ctre::match<"[a-zA-Z]*">(test_str); };
        double time_ns = benchmark_function(func);
        bool result = ctre::match<"[a-zA-Z]*">(test_str);
        std::cout << "[a-zA-Z]*  | " << std::setw(6) << len 
                  << " | " << std::setw(9) << std::fixed << std::setprecision(2) << time_ns
                  << " | " << (result ? "✓" : "✗") << "\n";
    }
    
    // Test small range patterns
    print_header("SMALL RANGE PATTERNS (≤10 characters)");
    print_table_header();
    
    std::string test_str_small = generate_test_string(64, "abcdef");
    
    // Test [a-e]* pattern (5 characters - should use small range optimization)
    for (size_t len : lengths) {
        std::string test_str = test_str_small.substr(0, len);
        auto func = [&]() { return ctre::match<"[a-e]*">(test_str); };
        double time_ns = benchmark_function(func);
        bool result = ctre::match<"[a-e]*">(test_str);
        std::cout << "[a-e]*     | " << std::setw(6) << len 
                  << " | " << std::setw(9) << std::fixed << std::setprecision(2) << time_ns
                  << " | " << (result ? "✓" : "✗") << "\n";
    }
    
    // Test [0-9]* pattern (10 characters - should use small range optimization)
    for (size_t len : lengths) {
        std::string test_str = test_str_digits.substr(0, len);
        auto func = [&]() { return ctre::match<"[0-9]*">(test_str); };
        double time_ns = benchmark_function(func);
        bool result = ctre::match<"[0-9]*">(test_str);
        std::cout << "[0-9]*     | " << std::setw(6) << len 
                  << " | " << std::setw(9) << std::fixed << std::setprecision(2) << time_ns
                  << " | " << (result ? "✓" : "✗") << "\n";
    }
    
    // Test edge cases
    print_header("EDGE CASES");
    print_table_header();
    
    std::vector<std::pair<std::string, std::string>> edge_cases = {
        {"Empty", ""},
        {"Single char", "a"},
        {"All match", "aaaaaaaa"},
        {"No match", "bbbbbbbb"},
        {"Mixed", "a1b2c3d4"}
    };
    
    for (const auto& [desc, test_str] : edge_cases) {
        auto func = [&]() { return ctre::match<"[a-z]*">(test_str); };
        double time_ns = benchmark_function(func, 10000000); // More iterations for small strings
        bool result = ctre::match<"[a-z]*">(test_str);
        std::cout << std::setw(10) << desc 
                  << " | " << std::setw(6) << test_str.length()
                  << " | " << std::setw(9) << std::fixed << std::setprecision(2) << time_ns
                  << " | " << (result ? "✓" : "✗") << "\n";
    }
    
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "✅ BENCHMARK COMPLETE!\n";
    std::cout << "=====================\n";
    std::cout << "Legend: ✓ = Match found, ✗ = No match\n";
    std::cout << "Note: All patterns are using SIMD optimizations\n";
    std::cout << "Run with CTRE_DISABLE_SIMD to compare non-SIMD performance\n";
    
    return 0;
}
