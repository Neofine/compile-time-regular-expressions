#include <chrono>
#include <ctre.hpp>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <vector>

// Test cases for Shift-Or string matching patterns
struct TestCase {
    std::string name;
    std::string pattern;
    std::string description;
};

// Generate test strings that contain the Shift-Or patterns
std::string generate_test_string(const std::string& pattern, size_t length) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(32, 126); // Printable ASCII

    std::string result;
    result.reserve(length);

    // Fill with random data
    for (size_t i = 0; i < length; ++i) {
        result += static_cast<char>(dis(gen));
    }

    // Insert pattern at a random position to ensure it exists
    if (length >= pattern.length()) {
        std::uniform_int_distribution<size_t> pos_dis(0, length - pattern.length());
        size_t pos = pos_dis(gen);
        for (size_t i = 0; i < pattern.length(); ++i) {
            result[pos + i] = pattern[i];
        }
    }

    return result;
}

// Benchmark SIMD version
double benchmark_simd(const std::string& pattern_str, const std::string& test_string, int iterations = 1000000) {
    // Warmup runs to ensure consistent timing
    for (int warmup = 0; warmup < 10000; ++warmup) {
        if (pattern_str == "A") {
            ctre::search<"A">(test_string);
        } else if (pattern_str == "AB") {
            ctre::search<"AB">(test_string);
        } else if (pattern_str == "ABC") {
            ctre::search<"ABC">(test_string);
        } else if (pattern_str == "ABCD") {
            ctre::search<"ABCD">(test_string);
        } else if (pattern_str == "ABCDE") {
            ctre::search<"ABCDE">(test_string);
        } else if (pattern_str == "ABCDEF") {
            ctre::search<"ABCDEF">(test_string);
        } else if (pattern_str == "ABCDEFG") {
            ctre::search<"ABCDEFG">(test_string);
        } else if (pattern_str == "ABCDEFGH") {
            ctre::search<"ABCDEFGH">(test_string);
        } else if (pattern_str == "CTRE") {
            ctre::search<"CTRE">(test_string);
        } else if (pattern_str == "REGX") {
            ctre::search<"REGX">(test_string);
        } else if (pattern_str == "SCAN") {
            ctre::search<"SCAN">(test_string);
        } else if (pattern_str == "FIND") {
            ctre::search<"FIND">(test_string);
        } else if (pattern_str == "HELLO") {
            ctre::search<"HELLO">(test_string);
        } else if (pattern_str == "WORLD") {
            ctre::search<"WORLD">(test_string);
        } else if (pattern_str == "TEST") {
            ctre::search<"TEST">(test_string);
        } else if (pattern_str == "DATA") {
            ctre::search<"DATA">(test_string);
        } else if (pattern_str == "CODE") {
            ctre::search<"CODE">(test_string);
        } else if (pattern_str == "BENCH") {
            ctre::search<"BENCH">(test_string);
        } else if (pattern_str == "MARK") {
            ctre::search<"MARK">(test_string);
        } else if (pattern_str == "FAST") {
            ctre::search<"FAST">(test_string);
        }
    }

    // Run multiple timing samples and take the minimum for more reliable results
    double min_time = std::numeric_limits<double>::max();

    for (int sample = 0; sample < 5; ++sample) {
        auto start = std::chrono::high_resolution_clock::now();
        size_t matches = 0;

        // Use appropriate pattern based on string
        for (int i = 0; i < iterations; ++i) {
            bool matched = false;

            if (pattern_str == "A") {
                matched = ctre::search<"A">(test_string);
            } else if (pattern_str == "AB") {
                matched = ctre::search<"AB">(test_string);
            } else if (pattern_str == "ABC") {
                matched = ctre::search<"ABC">(test_string);
            } else if (pattern_str == "ABCD") {
                matched = ctre::search<"ABCD">(test_string);
            } else if (pattern_str == "ABCDE") {
                matched = ctre::search<"ABCDE">(test_string);
            } else if (pattern_str == "ABCDEF") {
                matched = ctre::search<"ABCDEF">(test_string);
            } else if (pattern_str == "ABCDEFG") {
                matched = ctre::search<"ABCDEFG">(test_string);
            } else if (pattern_str == "ABCDEFGH") {
                matched = ctre::search<"ABCDEFGH">(test_string);
            } else if (pattern_str == "CTRE") {
                matched = ctre::search<"CTRE">(test_string);
            } else if (pattern_str == "REGX") {
                matched = ctre::search<"REGX">(test_string);
            } else if (pattern_str == "SCAN") {
                matched = ctre::search<"SCAN">(test_string);
            } else if (pattern_str == "FIND") {
                matched = ctre::search<"FIND">(test_string);
            } else if (pattern_str == "HELLO") {
                matched = ctre::search<"HELLO">(test_string);
            } else if (pattern_str == "WORLD") {
                matched = ctre::search<"WORLD">(test_string);
            } else if (pattern_str == "TEST") {
                matched = ctre::search<"TEST">(test_string);
            } else if (pattern_str == "DATA") {
                matched = ctre::search<"DATA">(test_string);
            } else if (pattern_str == "CODE") {
                matched = ctre::search<"CODE">(test_string);
            } else if (pattern_str == "BENCH") {
                matched = ctre::search<"BENCH">(test_string);
            } else if (pattern_str == "MARK") {
                matched = ctre::search<"MARK">(test_string);
            } else if (pattern_str == "FAST") {
                matched = ctre::search<"FAST">(test_string);
            }

            if (matched) {
                matches++;
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        double time_per_match = static_cast<double>(duration.count()) / static_cast<double>(matches);

        if (time_per_match < min_time) {
            min_time = time_per_match;
        }
    }

    return min_time;
}

// Benchmark traditional string search (non-SIMD baseline)
double benchmark_traditional(const std::string& pattern_str, const std::string& test_string, int iterations = 1000000) {
    // Warmup runs to ensure consistent timing
    for (int warmup = 0; warmup < 10000; ++warmup) {
        (void)test_string.find(pattern_str);
    }

    // Run multiple timing samples and take the minimum for more reliable results
    double min_time = std::numeric_limits<double>::max();

    for (int sample = 0; sample < 5; ++sample) {
        auto start = std::chrono::high_resolution_clock::now();
        size_t matches = 0;

        for (int i = 0; i < iterations; ++i) {
            if (test_string.find(pattern_str) != std::string::npos) {
                matches++;
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
        double time_per_match = static_cast<double>(duration.count()) / static_cast<double>(matches);

        if (time_per_match < min_time) {
            min_time = time_per_match;
        }
    }

    return min_time;
}

int main() {
    // Test cases covering Shift-Or string matching patterns
    std::vector<TestCase> test_cases = {
        // Single character patterns
        {"A_16", "A", "Single char 'A' (16 chars)"},
        {"A_32", "A", "Single char 'A' (32 chars)"},
        {"A_64", "A", "Single char 'A' (64 chars)"},
        {"A_128", "A", "Single char 'A' (128 chars)"},

        // Two character patterns
        {"AB_16", "AB", "Two chars 'AB' (16 chars)"},
        {"AB_32", "AB", "Two chars 'AB' (32 chars)"},
        {"AB_64", "AB", "Two chars 'AB' (64 chars)"},
        {"AB_128", "AB", "Two chars 'AB' (128 chars)"},

        // Three character patterns
        {"ABC_16", "ABC", "Three chars 'ABC' (16 chars)"},
        {"ABC_32", "ABC", "Three chars 'ABC' (32 chars)"},
        {"ABC_64", "ABC", "Three chars 'ABC' (64 chars)"},
        {"ABC_128", "ABC", "Three chars 'ABC' (128 chars)"},

        // Four character patterns
        {"ABCD_16", "ABCD", "Four chars 'ABCD' (16 chars)"},
        {"ABCD_32", "ABCD", "Four chars 'ABCD' (32 chars)"},
        {"ABCD_64", "ABCD", "Four chars 'ABCD' (64 chars)"},
        {"ABCD_128", "ABCD", "Four chars 'ABCD' (128 chars)"},

        // Five character patterns
        {"ABCDE_16", "ABCDE", "Five chars 'ABCDE' (16 chars)"},
        {"ABCDE_32", "ABCDE", "Five chars 'ABCDE' (32 chars)"},
        {"ABCDE_64", "ABCDE", "Five chars 'ABCDE' (64 chars)"},
        {"ABCDE_128", "ABCDE", "Five chars 'ABCDE' (128 chars)"},

        // Six character patterns
        {"ABCDEF_16", "ABCDEF", "Six chars 'ABCDEF' (16 chars)"},
        {"ABCDEF_32", "ABCDEF", "Six chars 'ABCDEF' (32 chars)"},
        {"ABCDEF_64", "ABCDEF", "Six chars 'ABCDEF' (64 chars)"},
        {"ABCDEF_128", "ABCDEF", "Six chars 'ABCDEF' (128 chars)"},

        // Seven character patterns
        {"ABCDEFG_16", "ABCDEFG", "Seven chars 'ABCDEFG' (16 chars)"},
        {"ABCDEFG_32", "ABCDEFG", "Seven chars 'ABCDEFG' (32 chars)"},
        {"ABCDEFG_64", "ABCDEFG", "Seven chars 'ABCDEFG' (64 chars)"},
        {"ABCDEFG_128", "ABCDEFG", "Seven chars 'ABCDEFG' (128 chars)"},

        // Eight character patterns
        {"ABCDEFGH_16", "ABCDEFGH", "Eight chars 'ABCDEFGH' (16 chars)"},
        {"ABCDEFGH_32", "ABCDEFGH", "Eight chars 'ABCDEFGH' (32 chars)"},
        {"ABCDEFGH_64", "ABCDEFGH", "Eight chars 'ABCDEFGH' (64 chars)"},
        {"ABCDEFGH_128", "ABCDEFGH", "Eight chars 'ABCDEFGH' (128 chars)"},

        // Keyword patterns
        {"CTRE_16", "CTRE", "Keyword 'CTRE' (16 chars)"},
        {"CTRE_32", "CTRE", "Keyword 'CTRE' (32 chars)"},
        {"CTRE_64", "CTRE", "Keyword 'CTRE' (64 chars)"},
        {"CTRE_128", "CTRE", "Keyword 'CTRE' (128 chars)"},

        {"REGX_16", "REGX", "Keyword 'REGX' (16 chars)"},
        {"REGX_32", "REGX", "Keyword 'REGX' (32 chars)"},
        {"REGX_64", "REGX", "Keyword 'REGX' (64 chars)"},
        {"REGX_128", "REGX", "Keyword 'REGX' (128 chars)"},

        {"SCAN_16", "SCAN", "Keyword 'SCAN' (16 chars)"},
        {"SCAN_32", "SCAN", "Keyword 'SCAN' (32 chars)"},
        {"SCAN_64", "SCAN", "Keyword 'SCAN' (64 chars)"},
        {"SCAN_128", "SCAN", "Keyword 'SCAN' (128 chars)"},

        {"FIND_16", "FIND", "Keyword 'FIND' (16 chars)"},
        {"FIND_32", "FIND", "Keyword 'FIND' (32 chars)"},
        {"FIND_64", "FIND", "Keyword 'FIND' (64 chars)"},
        {"FIND_128", "FIND", "Keyword 'FIND' (128 chars)"},

        // Word patterns
        {"HELLO_16", "HELLO", "Word 'HELLO' (16 chars)"},
        {"HELLO_32", "HELLO", "Word 'HELLO' (32 chars)"},
        {"HELLO_64", "HELLO", "Word 'HELLO' (64 chars)"},
        {"HELLO_128", "HELLO", "Word 'HELLO' (128 chars)"},

        {"WORLD_16", "WORLD", "Word 'WORLD' (16 chars)"},
        {"WORLD_32", "WORLD", "Word 'WORLD' (32 chars)"},
        {"WORLD_64", "WORLD", "Word 'WORLD' (64 chars)"},
        {"WORLD_128", "WORLD", "Word 'WORLD' (128 chars)"},

        {"TEST_16", "TEST", "Word 'TEST' (16 chars)"},
        {"TEST_32", "TEST", "Word 'TEST' (32 chars)"},
        {"TEST_64", "TEST", "Word 'TEST' (64 chars)"},
        {"TEST_128", "TEST", "Word 'TEST' (128 chars)"},

        {"DATA_16", "DATA", "Word 'DATA' (16 chars)"},
        {"DATA_32", "DATA", "Word 'DATA' (32 chars)"},
        {"DATA_64", "DATA", "Word 'DATA' (64 chars)"},
        {"DATA_128", "DATA", "Word 'DATA' (128 chars)"},

        {"CODE_16", "CODE", "Word 'CODE' (16 chars)"},
        {"CODE_32", "CODE", "Word 'CODE' (32 chars)"},
        {"CODE_64", "CODE", "Word 'CODE' (64 chars)"},
        {"CODE_128", "CODE", "Word 'CODE' (128 chars)"},

        {"BENCH_16", "BENCH", "Word 'BENCH' (16 chars)"},
        {"BENCH_32", "BENCH", "Word 'BENCH' (32 chars)"},
        {"BENCH_64", "BENCH", "Word 'BENCH' (64 chars)"},
        {"BENCH_128", "BENCH", "Word 'BENCH' (128 chars)"},

        {"MARK_16", "MARK", "Word 'MARK' (16 chars)"},
        {"MARK_32", "MARK", "Word 'MARK' (32 chars)"},
        {"MARK_64", "MARK", "Word 'MARK' (64 chars)"},
        {"MARK_128", "MARK", "Word 'MARK' (128 chars)"},

        {"FAST_16", "FAST", "Word 'FAST' (16 chars)"},
        {"FAST_32", "FAST", "Word 'FAST' (32 chars)"},
        {"FAST_64", "FAST", "Word 'FAST' (64 chars)"},
        {"FAST_128", "FAST", "Word 'FAST' (128 chars)"},
    };

    std::cout << "Shift-Or String Matching Performance Comparison\n";
    std::cout << "==============================================\n";
    std::cout << std::left << std::setw(20) << "Pattern" << std::setw(12) << "SIMD (ns)" << std::setw(12)
              << "Non-SIMD (ns)" << std::setw(10) << "Speedup" << "\n";
    std::cout << "----------------------------------------------------\n";

    // Track overall performance statistics
    double total_simd_time = 0.0;
    double total_traditional_time = 0.0;
    int test_count = 0;

    for (const auto& test_case : test_cases) {
        // Extract string length from test case name
        size_t length = 32; // default
        if (test_case.name.find("_16") != std::string::npos)
            length = 16;
        else if (test_case.name.find("_32") != std::string::npos)
            length = 32;
        else if (test_case.name.find("_64") != std::string::npos)
            length = 64;
        else if (test_case.name.find("_128") != std::string::npos)
            length = 128;

        // Generate test string
        std::string test_string = generate_test_string(test_case.pattern, length);

        // Benchmark both SIMD and traditional
        double simd_time = benchmark_simd(test_case.pattern, test_string, 100000);
        double traditional_time = benchmark_traditional(test_case.pattern, test_string, 100000);

        // Calculate speedup
        double speedup = traditional_time / simd_time;

        // Accumulate statistics
        total_simd_time += simd_time;
        total_traditional_time += traditional_time;
        test_count++;

        // Output result in comparison table format with string length
        std::string pattern_with_length = test_case.pattern + "_" + std::to_string(length);
        std::cout << std::left << std::setw(20) << pattern_with_length << std::setw(12) << std::fixed
                  << std::setprecision(2) << simd_time << std::setw(12) << std::fixed << std::setprecision(2)
                  << traditional_time << std::setw(10) << std::fixed << std::setprecision(2) << speedup << "x\n";
    }

    // Calculate and display overall performance statistics
    double avg_simd_time = total_simd_time / test_count;
    double avg_traditional_time = total_traditional_time / test_count;
    double overall_speedup = avg_traditional_time / avg_simd_time;

    std::cout << "----------------------------------------------------\n";
    std::cout << std::left << std::setw(20) << "OVERALL" << std::setw(12) << std::fixed << std::setprecision(2)
              << avg_simd_time << std::setw(12) << std::fixed << std::setprecision(2) << avg_traditional_time
              << std::setw(10) << std::fixed << std::setprecision(2) << overall_speedup << "x\n";
    std::cout << "----------------------------------------------------\n";
    std::cout << "Shift-Or String Matching comparison completed!\n";

    return 0;
}
