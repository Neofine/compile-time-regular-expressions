#include <chrono>
#include <ctre.hpp>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <vector>

// Test cases for Shift-Or patterns
struct TestCase {
    std::string name;
    std::string pattern;
    std::string description;
};

// Generate test strings for different Shift-Or patterns
std::string generate_test_string(const std::string& pattern, size_t length) {
    std::random_device rd;
    std::mt19937 gen(rd());

    if (pattern == "A") {
        std::uniform_int_distribution<> dis(32, 126);
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += static_cast<char>(dis(gen));
        }
        // Insert 'A' at random position
        std::uniform_int_distribution<size_t> pos_dis(0, length - 1);
        result[pos_dis(gen)] = 'A';
        return result;
    } else if (pattern == "AB") {
        std::uniform_int_distribution<> dis(32, 126);
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += static_cast<char>(dis(gen));
        }
        // Insert 'AB' at random position
        std::uniform_int_distribution<size_t> pos_dis(0, length - 2);
        size_t pos = pos_dis(gen);
        result[pos] = 'A';
        result[pos + 1] = 'B';
        return result;
    } else if (pattern == "ABC") {
        std::uniform_int_distribution<> dis(32, 126);
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += static_cast<char>(dis(gen));
        }
        // Insert 'ABC' at random position
        std::uniform_int_distribution<size_t> pos_dis(0, length - 3);
        size_t pos = pos_dis(gen);
        result[pos] = 'A';
        result[pos + 1] = 'B';
        result[pos + 2] = 'C';
        return result;
    } else if (pattern == "ABCD") {
        std::uniform_int_distribution<> dis(32, 126);
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += static_cast<char>(dis(gen));
        }
        // Insert 'ABCD' at random position
        std::uniform_int_distribution<size_t> pos_dis(0, length - 4);
        size_t pos = pos_dis(gen);
        result[pos] = 'A';
        result[pos + 1] = 'B';
        result[pos + 2] = 'C';
        result[pos + 3] = 'D';
        return result;
    } else if (pattern == "ABCDE") {
        std::uniform_int_distribution<> dis(32, 126);
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += static_cast<char>(dis(gen));
        }
        // Insert 'ABCDE' at random position
        std::uniform_int_distribution<size_t> pos_dis(0, length - 5);
        size_t pos = pos_dis(gen);
        result[pos] = 'A';
        result[pos + 1] = 'B';
        result[pos + 2] = 'C';
        result[pos + 3] = 'D';
        result[pos + 4] = 'E';
        return result;
    } else if (pattern == "ABCDEF") {
        std::uniform_int_distribution<> dis(32, 126);
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += static_cast<char>(dis(gen));
        }
        // Insert 'ABCDEF' at random position
        std::uniform_int_distribution<size_t> pos_dis(0, length - 6);
        size_t pos = pos_dis(gen);
        result[pos] = 'A';
        result[pos + 1] = 'B';
        result[pos + 2] = 'C';
        result[pos + 3] = 'D';
        result[pos + 4] = 'E';
        result[pos + 5] = 'F';
        return result;
    } else if (pattern == "ABCDEFG") {
        std::uniform_int_distribution<> dis(32, 126);
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += static_cast<char>(dis(gen));
        }
        // Insert 'ABCDEFG' at random position
        std::uniform_int_distribution<size_t> pos_dis(0, length - 7);
        size_t pos = pos_dis(gen);
        result[pos] = 'A';
        result[pos + 1] = 'B';
        result[pos + 2] = 'C';
        result[pos + 3] = 'D';
        result[pos + 4] = 'E';
        result[pos + 5] = 'F';
        result[pos + 6] = 'G';
        return result;
    } else if (pattern == "ABCDEFGH") {
        std::uniform_int_distribution<> dis(32, 126);
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += static_cast<char>(dis(gen));
        }
        // Insert 'ABCDEFGH' at random position
        std::uniform_int_distribution<size_t> pos_dis(0, length - 8);
        size_t pos = pos_dis(gen);
        result[pos] = 'A';
        result[pos + 1] = 'B';
        result[pos + 2] = 'C';
        result[pos + 3] = 'D';
        result[pos + 4] = 'E';
        result[pos + 5] = 'F';
        result[pos + 6] = 'G';
        result[pos + 7] = 'H';
        return result;
    } else if (pattern == "CTRE") {
        std::uniform_int_distribution<> dis(32, 126);
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += static_cast<char>(dis(gen));
        }
        // Insert 'CTRE' at random position
        std::uniform_int_distribution<size_t> pos_dis(0, length - 4);
        size_t pos = pos_dis(gen);
        result[pos] = 'C';
        result[pos + 1] = 'T';
        result[pos + 2] = 'R';
        result[pos + 3] = 'E';
        return result;
    } else if (pattern == "REGX") {
        std::uniform_int_distribution<> dis(32, 126);
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += static_cast<char>(dis(gen));
        }
        // Insert 'REGX' at random position
        std::uniform_int_distribution<size_t> pos_dis(0, length - 4);
        size_t pos = pos_dis(gen);
        result[pos] = 'R';
        result[pos + 1] = 'E';
        result[pos + 2] = 'G';
        result[pos + 3] = 'X';
        return result;
    } else if (pattern == "SCAN") {
        std::uniform_int_distribution<> dis(32, 126);
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += static_cast<char>(dis(gen));
        }
        // Insert 'SCAN' at random position
        std::uniform_int_distribution<size_t> pos_dis(0, length - 4);
        size_t pos = pos_dis(gen);
        result[pos] = 'S';
        result[pos + 1] = 'C';
        result[pos + 2] = 'A';
        result[pos + 3] = 'N';
        return result;
    } else if (pattern == "FIND") {
        std::uniform_int_distribution<> dis(32, 126);
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += static_cast<char>(dis(gen));
        }
        // Insert 'FIND' at random position
        std::uniform_int_distribution<size_t> pos_dis(0, length - 4);
        size_t pos = pos_dis(gen);
        result[pos] = 'F';
        result[pos + 1] = 'I';
        result[pos + 2] = 'N';
        result[pos + 3] = 'D';
        return result;
    } else if (pattern == "HELLO") {
        std::uniform_int_distribution<> dis(32, 126);
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += static_cast<char>(dis(gen));
        }
        // Insert 'HELLO' at random position
        std::uniform_int_distribution<size_t> pos_dis(0, length - 5);
        size_t pos = pos_dis(gen);
        result[pos] = 'H';
        result[pos + 1] = 'E';
        result[pos + 2] = 'L';
        result[pos + 3] = 'L';
        result[pos + 4] = 'O';
        return result;
    } else if (pattern == "WORLD") {
        std::uniform_int_distribution<> dis(32, 126);
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += static_cast<char>(dis(gen));
        }
        // Insert 'WORLD' at random position
        std::uniform_int_distribution<size_t> pos_dis(0, length - 5);
        size_t pos = pos_dis(gen);
        result[pos] = 'W';
        result[pos + 1] = 'O';
        result[pos + 2] = 'R';
        result[pos + 3] = 'L';
        result[pos + 4] = 'D';
        return result;
    } else if (pattern == "TEST") {
        std::uniform_int_distribution<> dis(32, 126);
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += static_cast<char>(dis(gen));
        }
        // Insert 'TEST' at random position
        std::uniform_int_distribution<size_t> pos_dis(0, length - 4);
        size_t pos = pos_dis(gen);
        result[pos] = 'T';
        result[pos + 1] = 'E';
        result[pos + 2] = 'S';
        result[pos + 3] = 'T';
        return result;
    } else if (pattern == "DATA") {
        std::uniform_int_distribution<> dis(32, 126);
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += static_cast<char>(dis(gen));
        }
        // Insert 'DATA' at random position
        std::uniform_int_distribution<size_t> pos_dis(0, length - 4);
        size_t pos = pos_dis(gen);
        result[pos] = 'D';
        result[pos + 1] = 'A';
        result[pos + 2] = 'T';
        result[pos + 3] = 'A';
        return result;
    } else if (pattern == "CODE") {
        std::uniform_int_distribution<> dis(32, 126);
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += static_cast<char>(dis(gen));
        }
        // Insert 'CODE' at random position
        std::uniform_int_distribution<size_t> pos_dis(0, length - 4);
        size_t pos = pos_dis(gen);
        result[pos] = 'C';
        result[pos + 1] = 'O';
        result[pos + 2] = 'D';
        result[pos + 3] = 'E';
        return result;
    } else if (pattern == "BENCH") {
        std::uniform_int_distribution<> dis(32, 126);
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += static_cast<char>(dis(gen));
        }
        // Insert 'BENCH' at random position
        std::uniform_int_distribution<size_t> pos_dis(0, length - 5);
        size_t pos = pos_dis(gen);
        result[pos] = 'B';
        result[pos + 1] = 'E';
        result[pos + 2] = 'N';
        result[pos + 3] = 'C';
        result[pos + 4] = 'H';
        return result;
    } else if (pattern == "MARK") {
        std::uniform_int_distribution<> dis(32, 126);
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += static_cast<char>(dis(gen));
        }
        // Insert 'MARK' at random position
        std::uniform_int_distribution<size_t> pos_dis(0, length - 4);
        size_t pos = pos_dis(gen);
        result[pos] = 'M';
        result[pos + 1] = 'A';
        result[pos + 2] = 'R';
        result[pos + 3] = 'K';
        return result;
    } else if (pattern == "FAST") {
        std::uniform_int_distribution<> dis(32, 126);
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += static_cast<char>(dis(gen));
        }
        // Insert 'FAST' at random position
        std::uniform_int_distribution<size_t> pos_dis(0, length - 4);
        size_t pos = pos_dis(gen);
        result[pos] = 'F';
        result[pos + 1] = 'A';
        result[pos + 2] = 'S';
        result[pos + 3] = 'T';
        return result;
    }

    return std::string(length, 'a'); // fallback
}

// Benchmark a single test case
double benchmark_case(const std::string& pattern_str, const std::string& test_string, int iterations = 1000000) {
    // Warmup runs to ensure consistent timing
    for (int warmup = 0; warmup < 10000; ++warmup) {
        if (pattern_str == "A") {
            ctre::match<"A">(test_string);
        } else if (pattern_str == "AB") {
            ctre::match<"AB">(test_string);
        } else if (pattern_str == "ABC") {
            ctre::match<"ABC">(test_string);
        } else if (pattern_str == "ABCD") {
            ctre::match<"ABCD">(test_string);
        } else if (pattern_str == "ABCDE") {
            ctre::match<"ABCDE">(test_string);
        } else if (pattern_str == "ABCDEF") {
            ctre::match<"ABCDEF">(test_string);
        } else if (pattern_str == "ABCDEFG") {
            ctre::match<"ABCDEFG">(test_string);
        } else if (pattern_str == "ABCDEFGH") {
            ctre::match<"ABCDEFGH">(test_string);
        } else if (pattern_str == "CTRE") {
            ctre::match<"CTRE">(test_string);
        } else if (pattern_str == "REGX") {
            ctre::match<"REGX">(test_string);
        } else if (pattern_str == "SCAN") {
            ctre::match<"SCAN">(test_string);
        } else if (pattern_str == "FIND") {
            ctre::match<"FIND">(test_string);
        } else if (pattern_str == "HELLO") {
            ctre::match<"HELLO">(test_string);
        } else if (pattern_str == "WORLD") {
            ctre::match<"WORLD">(test_string);
        } else if (pattern_str == "TEST") {
            ctre::match<"TEST">(test_string);
        } else if (pattern_str == "DATA") {
            ctre::match<"DATA">(test_string);
        } else if (pattern_str == "CODE") {
            ctre::match<"CODE">(test_string);
        } else if (pattern_str == "BENCH") {
            ctre::match<"BENCH">(test_string);
        } else if (pattern_str == "MARK") {
            ctre::match<"MARK">(test_string);
        } else if (pattern_str == "FAST") {
            ctre::match<"FAST">(test_string);
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
                matched = ctre::match<"A">(test_string);
            } else if (pattern_str == "AB") {
                matched = ctre::match<"AB">(test_string);
            } else if (pattern_str == "ABC") {
                matched = ctre::match<"ABC">(test_string);
            } else if (pattern_str == "ABCD") {
                matched = ctre::match<"ABCD">(test_string);
            } else if (pattern_str == "ABCDE") {
                matched = ctre::match<"ABCDE">(test_string);
            } else if (pattern_str == "ABCDEF") {
                matched = ctre::match<"ABCDEF">(test_string);
            } else if (pattern_str == "ABCDEFG") {
                matched = ctre::match<"ABCDEFG">(test_string);
            } else if (pattern_str == "ABCDEFGH") {
                matched = ctre::match<"ABCDEFGH">(test_string);
            } else if (pattern_str == "CTRE") {
                matched = ctre::match<"CTRE">(test_string);
            } else if (pattern_str == "REGX") {
                matched = ctre::match<"REGX">(test_string);
            } else if (pattern_str == "SCAN") {
                matched = ctre::match<"SCAN">(test_string);
            } else if (pattern_str == "FIND") {
                matched = ctre::match<"FIND">(test_string);
            } else if (pattern_str == "HELLO") {
                matched = ctre::match<"HELLO">(test_string);
            } else if (pattern_str == "WORLD") {
                matched = ctre::match<"WORLD">(test_string);
            } else if (pattern_str == "TEST") {
                matched = ctre::match<"TEST">(test_string);
            } else if (pattern_str == "DATA") {
                matched = ctre::match<"DATA">(test_string);
            } else if (pattern_str == "CODE") {
                matched = ctre::match<"CODE">(test_string);
            } else if (pattern_str == "BENCH") {
                matched = ctre::match<"BENCH">(test_string);
            } else if (pattern_str == "MARK") {
                matched = ctre::match<"MARK">(test_string);
            } else if (pattern_str == "FAST") {
                matched = ctre::match<"FAST">(test_string);
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

int main() {
    // Test cases covering different Shift-Or patterns
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

        // Benchmark with SIMD enabled (default)
        double simd_time = benchmark_case(test_case.pattern, test_string, 100000);

        // Output result
        std::cout << test_case.name << "," << simd_time << "\n";
    }

    return 0;
}
