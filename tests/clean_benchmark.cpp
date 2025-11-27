#include <ctre.hpp>
#include <iostream>
#include <chrono>
#include <random>
#include <string>
#include <vector>

// Generate test string for a given length using seed 42
template<char BaseChar, int Range>
std::string gen_range_string(size_t length) {
    std::mt19937 gen(42);
    std::uniform_int_distribution<> dis(0, Range - 1);
    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result += static_cast<char>(BaseChar + dis(gen));
    }
    return result;
}

// Generic benchmark function template - no if-else chain!
template<ctll::fixed_string Pattern>
double benchmark_pattern(const std::string& test_string, int iterations = 1000000) {
    // Warmup
    for (int i = 0; i < 10000; ++i) {
        ctre::match<Pattern>(test_string);
    }

    // Run multiple timing samples and take the minimum
    double min_time = 1e9;

    for (int sample = 0; sample < 5; ++sample) {
        auto start = std::chrono::high_resolution_clock::now();
        size_t matches = 0;

        for (int i = 0; i < iterations; ++i) {
            if (ctre::match<Pattern>(test_string)) {
                matches++;
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

        if (matches > 0) {
            double time_per_match = static_cast<double>(duration.count()) / static_cast<double>(matches);
            if (time_per_match < min_time) {
                min_time = time_per_match;
            }
        }
    }

    return min_time;
}

int main() {
    // Small range patterns that were showing regressions
    std::cout << "[0-2]+_32," << benchmark_pattern<"[0-2]+">(gen_range_string<'0', 3>(32)) << std::endl;
    std::cout << "[a-c]+_32," << benchmark_pattern<"[a-c]+">(gen_range_string<'a', 3>(32)) << std::endl;
    std::cout << "[x-z]+_32," << benchmark_pattern<"[x-z]+">(gen_range_string<'x', 3>(32)) << std::endl;
    std::cout << "[a-e]+_32," << benchmark_pattern<"[a-e]+">(gen_range_string<'a', 5>(32)) << std::endl;

    std::cout << "[0-2]*_32," << benchmark_pattern<"[0-2]*">(gen_range_string<'0', 3>(32)) << std::endl;
    std::cout << "[a-c]*_32," << benchmark_pattern<"[a-c]*">(gen_range_string<'a', 3>(32)) << std::endl;
    std::cout << "[x-z]*_32," << benchmark_pattern<"[x-z]*">(gen_range_string<'x', 3>(32)) << std::endl;
    std::cout << "[a-e]*_32," << benchmark_pattern<"[a-e]*">(gen_range_string<'a', 5>(32)) << std::endl;

    // Medium range patterns
    std::cout << "[0-9]+_32," << benchmark_pattern<"[0-9]+">(gen_range_string<'0', 10>(32)) << std::endl;
    std::cout << "[0-9]+_256," << benchmark_pattern<"[0-9]+">(gen_range_string<'0', 10>(256)) << std::endl;
    std::cout << "[0-9]*_32," << benchmark_pattern<"[0-9]*">(gen_range_string<'0', 10>(32)) << std::endl;
    std::cout << "[0-9]*_256," << benchmark_pattern<"[0-9]*">(gen_range_string<'0', 10>(256)) << std::endl;

    // Large range patterns (should show massive SIMD wins)
    std::cout << "[a-z]+_32," << benchmark_pattern<"[a-z]+">(gen_range_string<'a', 26>(32)) << std::endl;
    std::cout << "[a-z]+_64," << benchmark_pattern<"[a-z]+">(gen_range_string<'a', 26>(64)) << std::endl;
    std::cout << "[a-z]+_128," << benchmark_pattern<"[a-z]+">(gen_range_string<'a', 26>(128)) << std::endl;
    std::cout << "[a-z]+_256," << benchmark_pattern<"[a-z]+">(gen_range_string<'a', 26>(256)) << std::endl;
    std::cout << "[a-z]+_512," << benchmark_pattern<"[a-z]+">(gen_range_string<'a', 26>(512)) << std::endl;

    std::cout << "[a-z]*_32," << benchmark_pattern<"[a-z]*">(gen_range_string<'a', 26>(32)) << std::endl;
    std::cout << "[a-z]*_64," << benchmark_pattern<"[a-z]*">(gen_range_string<'a', 26>(64)) << std::endl;
    std::cout << "[a-z]*_128," << benchmark_pattern<"[a-z]*">(gen_range_string<'a', 26>(128)) << std::endl;
    std::cout << "[a-z]*_256," << benchmark_pattern<"[a-z]*">(gen_range_string<'a', 26>(256)) << std::endl;
    std::cout << "[a-z]*_512," << benchmark_pattern<"[a-z]*">(gen_range_string<'a', 26>(512)) << std::endl;

    std::cout << "[A-Z]*_256," << benchmark_pattern<"[A-Z]*">(gen_range_string<'A', 26>(256)) << std::endl;

    // Single character patterns
    std::cout << "a+_32," << benchmark_pattern<"a+">(std::string(32, 'a')) << std::endl;
    std::cout << "a+_128," << benchmark_pattern<"a+">(std::string(128, 'a')) << std::endl;
    std::cout << "a+_256," << benchmark_pattern<"a+">(std::string(256, 'a')) << std::endl;

    std::cout << "a*_32," << benchmark_pattern<"a*">(std::string(32, 'a')) << std::endl;
    std::cout << "a*_128," << benchmark_pattern<"a*">(std::string(128, 'a')) << std::endl;
    std::cout << "a*_256," << benchmark_pattern<"a*">(std::string(256, 'a')) << std::endl;

    return 0;
}
