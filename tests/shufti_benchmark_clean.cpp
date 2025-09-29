#include <chrono>
#include <iostream>
#include <random>
#include <string>

#include "ctre.hpp"

using namespace std::chrono;

constexpr size_t ITERATIONS = 10000000;
constexpr size_t STRING_LENGTH = 32;

// Generate test strings with different character types
std::string generate_test_string(size_t length, const std::string& type) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::string result;
    result.reserve(length);

    if (type == "alnum") {
        std::uniform_int_distribution<> dis(0, 61); // 0-9, A-Z, a-z
        const char alnum_chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        for (size_t i = 0; i < length; ++i) {
            result += alnum_chars[dis(gen)];
        }
    } else if (type == "whitespace") {
        std::uniform_int_distribution<> dis(0, 3);
        const char whitespace_chars[] = " \t\n\r";
        for (size_t i = 0; i < length; ++i) {
            result += whitespace_chars[dis(gen)];
        }
    } else if (type == "digits") {
        std::uniform_int_distribution<> dis(0, 9);
        for (size_t i = 0; i < length; ++i) {
            result += '0' + dis(gen);
        }
    } else if (type == "letters") {
        std::uniform_int_distribution<> dis(0, 51);
        const char letter_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        for (size_t i = 0; i < length; ++i) {
            result += letter_chars[dis(gen)];
        }
    } else if (type == "complex") {
        std::uniform_int_distribution<> dis(0, 31);
        const char complex_chars[] = "!@#$%^&*()_+-=[]{}|;':\",./<>?";
        for (size_t i = 0; i < length; ++i) {
            result += complex_chars[dis(gen)];
        }
    }

    return result;
}

// Benchmark function with proper optimization prevention
template <typename Func>
double benchmark_function(Func func, const std::string& name) {
    auto start = high_resolution_clock::now();
    volatile int dummy = 0;
    for (size_t i = 0; i < ITERATIONS; ++i) {
        dummy += func() ? 1 : 0;
    }
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<nanoseconds>(end - start);
    double avg_time = static_cast<double>(duration.count()) / ITERATIONS;
    std::cout << name << "," << avg_time << std::endl;
    (void)dummy;
    return avg_time;
}

// Test SHUFTI character class matching - END-TO-END TESTS ONLY
void benchmark_shufti_char_classes() {
    std::cout << "Pattern,Time(ns)" << std::endl;

    // Test case 1: Alphanumeric characters [A-Za-z0-9_] - END-TO-END TEST
    auto alnum_data = generate_test_string(STRING_LENGTH, "alnum");
    benchmark_function([&]() -> bool { return ctre::match<"[A-Za-z0-9_]">(alnum_data); }, "[A-Za-z0-9_]_32");

    auto alnum_data_64 = generate_test_string(64, "alnum");
    benchmark_function([&]() -> bool { return ctre::match<"[A-Za-z0-9_]">(alnum_data_64); }, "[A-Za-z0-9_]_64");

    auto alnum_data_128 = generate_test_string(128, "alnum");
    benchmark_function([&]() -> bool { return ctre::match<"[A-Za-z0-9_]">(alnum_data_128); }, "[A-Za-z0-9_]_128");

    // Test case 2: Whitespace characters \s - END-TO-END TEST
    auto whitespace_data = generate_test_string(STRING_LENGTH, "whitespace");
    benchmark_function([&]() -> bool { return ctre::match<"\\s">(whitespace_data); }, "\\s_32");

    auto whitespace_data_128 = generate_test_string(128, "whitespace");
    benchmark_function([&]() -> bool { return ctre::match<"\\s">(whitespace_data_128); }, "\\s_128");

    // Test case 3: Digits [0-9] - END-TO-END TEST
    auto digits_data = generate_test_string(STRING_LENGTH, "digits");
    benchmark_function([&]() -> bool { return ctre::match<"[0-9]">(digits_data); }, "[0-9]_32");

    // Test case 4: Letters [A-Za-z] - END-TO-END TEST
    auto letters_data = generate_test_string(STRING_LENGTH, "letters");
    benchmark_function([&]() -> bool { return ctre::match<"[A-Za-z]">(letters_data); }, "[A-Za-z]_32");

    // Test with different string lengths
    auto short_alnum_data = generate_test_string(16, "alnum");
    benchmark_function([&]() -> bool { return ctre::match<"[A-Za-z0-9_]">(short_alnum_data); }, "[A-Za-z0-9_]_16");

    // Test patterns that should actually trigger SHUFTI (individual characters, not ranges)
    // This pattern has many individual characters and should trigger SHUFTI
    auto complex_data = generate_test_string(STRING_LENGTH, "complex");
    benchmark_function([&]() -> bool { return ctre::match<"[!@#$%^&*()_+-=[]{}|;':\",./<>?]">(complex_data); },
                       "complex_chars_32");

    // Test a pattern that definitely won't trigger SHUFTI (range)
    benchmark_function([&]() -> bool { return ctre::match<"[a-z]">(letters_data); }, "[a-z]_range_32");
}

int main() {
    std::cout << "SHUFTI (SIMD Character-Class Matching) Performance Comparison" << std::endl;
    std::cout << "=============================================================" << std::endl;
    std::cout << "Testing END-TO-END integration through CTRE evaluation pipeline" << std::endl;
    std::cout << "All tests use ctre::match<pattern>(string) - no standalone functions" << std::endl;
    std::cout << std::endl;

    benchmark_shufti_char_classes();

    std::cout << std::endl;
    std::cout << "SHUFTI benchmark completed successfully!" << std::endl;
    std::cout << "Note: This tests the actual SHUFTI integration in CTRE's evaluation pipeline." << std::endl;
    std::cout << "Patterns like [a-z] are ranges and use existing SIMD optimizations." << std::endl;
    std::cout << "Patterns like [!@#$%^&*()...] are individual characters and may use SHUFTI." << std::endl;

    return 0;
}
