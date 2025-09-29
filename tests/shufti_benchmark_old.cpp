#include <chrono>
#include <ctre.hpp>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

// Include our SHUFTI implementation
#include "ctre/simd_shufti.hpp"

using namespace std::chrono;

// Benchmark configuration
constexpr size_t ITERATIONS = 1000000;
constexpr size_t STRING_LENGTH = 1000;

// Generate test data with specific character classes
std::string generate_test_string(size_t length, const std::string& char_class_type) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 25);

    std::string result;
    result.reserve(length);

    for (size_t i = 0; i < length; ++i) {
        if (char_class_type == "alnum") {
            // Mix of alphanumeric characters
            if (i % 3 == 0) {
                result += static_cast<char>('0' + (i % 10)); // digits
            } else if (i % 3 == 1) {
                result += static_cast<char>('A' + (i % 26)); // uppercase
            } else {
                result += static_cast<char>('a' + (i % 26)); // lowercase
            }
        } else if (char_class_type == "whitespace") {
            // Mix with whitespace
            if (i % 5 == 0) {
                result += ' '; // space
            } else if (i % 5 == 1) {
                result += '\t'; // tab
            } else if (i % 5 == 2) {
                result += '\n'; // newline
            } else {
                result += static_cast<char>('a' + (i % 26)); // letters
            }
        } else if (char_class_type == "digits") {
            result += static_cast<char>('0' + (i % 10)); // only digits
        } else if (char_class_type == "letters") {
            if (i % 2 == 0) {
                result += static_cast<char>('A' + (i % 26)); // uppercase
            } else {
                result += static_cast<char>('a' + (i % 26)); // lowercase
            }
        } else {
            result += static_cast<char>('a' + (i % 26));
        }
    }

    return result;
}

// Benchmark function template
template <typename Func>
double benchmark_function(Func&& func, const std::string& name) {
    volatile int dummy = 0; // Prevent optimization
    auto start = high_resolution_clock::now();

    for (size_t i = 0; i < ITERATIONS; ++i) {
        dummy += func() ? 1 : 0; // Use result to prevent optimization
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<nanoseconds>(end - start);

    double avg_time = static_cast<double>(duration.count()) / ITERATIONS;

    // Output in CSV format for easy parsing
    std::cout << name << "," << std::fixed << std::setprecision(2) << avg_time << std::endl;

    // Prevent unused variable warning
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
    benchmark_function([&]() -> bool { return ctre::match<"[!@#$%^&*()_+-=[]{}|;':\",./<>?]">(complex_data); }, "complex_chars_32");
    
    // Test a pattern that definitely won't trigger SHUFTI (range)
    benchmark_function([&]() -> bool { return ctre::match<"[a-z]">(letters_data); }, "[a-z]_range_32");

    benchmark_function(
        [&]() {
            auto it = short_alnum_data.begin();
            return ctre::simd::match_alnum_shufti(it, short_alnum_data.end(), ctre::flags{});
        },
        "[A-Za-z0-9_]_shufti_16");

    auto long_alnum_data = generate_test_string(128, "alnum");
    benchmark_function([&]() -> bool { return ctre::match<"[A-Za-z0-9_]">(long_alnum_data); }, "[A-Za-z0-9_]_128");

    benchmark_function(
        [&]() {
            auto it = long_alnum_data.begin();
            return ctre::simd::match_alnum_shufti(it, long_alnum_data.end(), ctre::flags{});
        },
        "[A-Za-z0-9_]_shufti_128");

    auto very_long_alnum_data = generate_test_string(64, "alnum");
    benchmark_function([&]() -> bool { return ctre::match<"[A-Za-z0-9_]">(very_long_alnum_data); }, "[A-Za-z0-9_]_64");

    benchmark_function(
        [&]() {
            auto it = very_long_alnum_data.begin();
            return ctre::simd::match_alnum_shufti(it, very_long_alnum_data.end(), ctre::flags{});
        },
        "[A-Za-z0-9_]_shufti_64");

    // Test whitespace with different lengths
    auto short_whitespace_data = generate_test_string(16, "whitespace");
    benchmark_function([&]() -> bool { return ctre::match<"\\s">(short_whitespace_data); }, "\\s_16");

    benchmark_function(
        [&]() {
            auto it = short_whitespace_data.begin();
            return ctre::simd::match_whitespace_shufti(it, short_whitespace_data.end(), ctre::flags{});
        },
        "\\s_shufti_16");

    auto long_whitespace_data = generate_test_string(128, "whitespace");
    benchmark_function([&]() -> bool { return ctre::match<"\\s">(long_whitespace_data); }, "\\s_128");

    benchmark_function(
        [&]() {
            auto it = long_whitespace_data.begin();
            return ctre::simd::match_whitespace_shufti(it, long_whitespace_data.end(), ctre::flags{});
        },
        "\\s_shufti_128");
}

int main() {
    try {
        benchmark_shufti_char_classes();
    } catch (const std::exception& e) {
        std::cerr << "Benchmark failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
