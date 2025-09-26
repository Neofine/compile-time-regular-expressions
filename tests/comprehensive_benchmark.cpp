#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <random>
#include <ctre.hpp>

// Test cases for different SIMD optimizations
struct TestCase {
    std::string name;
    std::string pattern;
    std::string description;
};

// Generate test strings for different patterns
std::string generate_test_string(const std::string& pattern, size_t length) {
    std::random_device rd;
    std::mt19937 gen(rd());
    
    if (pattern == "a*") {
        return std::string(length, 'a');
    } else if (pattern == "A*") {
        return std::string(length, 'A');
    } else if (pattern == "[a-z]*") {
        std::uniform_int_distribution<> dis('a', 'z');
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += static_cast<char>(dis(gen));
        }
        return result;
    } else if (pattern == "[0-9]*") {
        std::uniform_int_distribution<> dis('0', '9');
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += static_cast<char>(dis(gen));
        }
        return result;
    } else if (pattern == "[A-Z]*") {
        std::uniform_int_distribution<> dis('A', 'Z');
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += static_cast<char>(dis(gen));
        }
        return result;
    } else if (pattern == "[a-zA-Z]*") {
        std::uniform_int_distribution<> dis(0, 51); // 26 lowercase + 26 uppercase
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            int val = dis(gen);
            result += (val < 26) ? ('a' + val) : ('A' + val - 26);
        }
        return result;
    } else if (pattern == "[0-9a-f]*") {
        std::uniform_int_distribution<> dis(0, 15);
        std::string hex_chars = "0123456789abcdef";
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += hex_chars[dis(gen)];
        }
        return result;
    } else if (pattern == "b*") {
        return std::string(length, 'b');
    } else if (pattern == "z*") {
        return std::string(length, 'z');
    } else if (pattern == "9*") {
        return std::string(length, '9');
    } else if (pattern == "a+") {
        return std::string(length, 'a');
    } else if (pattern == "A+") {
        return std::string(length, 'A');
    } else if (pattern == "b+") {
        return std::string(length, 'b');
    } else if (pattern == "z+") {
        return std::string(length, 'z');
    } else if (pattern == "9+") {
        return std::string(length, '9');
    } else if (pattern == "[a-z]+") {
        std::uniform_int_distribution<> dis('a', 'z');
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += static_cast<char>(dis(gen));
        }
        return result;
    } else if (pattern == "[0-9]+") {
        std::uniform_int_distribution<> dis('0', '9');
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += static_cast<char>(dis(gen));
        }
        return result;
    } else if (pattern == "[A-Z]+") {
        std::uniform_int_distribution<> dis('A', 'Z');
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += static_cast<char>(dis(gen));
        }
        return result;
    } else if (pattern == "[0-2]*") {
        std::uniform_int_distribution<> dis(0, 2);
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += static_cast<char>('0' + dis(gen));
        }
        return result;
    } else if (pattern == "[x-z]*") {
        std::uniform_int_distribution<> dis(0, 2);
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += static_cast<char>('x' + dis(gen));
        }
        return result;
    } else if (pattern == "[a-c]*") {
        std::uniform_int_distribution<> dis(0, 2);
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += static_cast<char>('a' + dis(gen));
        }
        return result;
    } else if (pattern == "[a-e]*") {
        std::uniform_int_distribution<> dis(0, 4);
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += static_cast<char>('a' + dis(gen));
        }
        return result;
    } else if (pattern == "[0-2]+") {
        std::uniform_int_distribution<> dis(0, 2);
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += static_cast<char>('0' + dis(gen));
        }
        return result;
    } else if (pattern == "[x-z]+") {
        std::uniform_int_distribution<> dis(0, 2);
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += static_cast<char>('x' + dis(gen));
        }
        return result;
    } else if (pattern == "[a-c]+") {
        std::uniform_int_distribution<> dis(0, 2);
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += static_cast<char>('a' + dis(gen));
        }
        return result;
    } else if (pattern == "[a-e]+") {
        std::uniform_int_distribution<> dis(0, 4);
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += static_cast<char>('a' + dis(gen));
        }
        return result;
    }
    
    return std::string(length, 'a'); // fallback
}


// Benchmark a single test case
double benchmark_case(const std::string& pattern_str, const std::string& test_string, int iterations = 10000) {
    auto start = std::chrono::high_resolution_clock::now();
    size_t matches = 0;
    
    // Use appropriate pattern based on string
    for (int i = 0; i < iterations; ++i) {
        bool matched = false;
        
        if (pattern_str == "a*") {
            matched = ctre::match<"a*">(test_string);
        } else if (pattern_str == "A*") {
            matched = ctre::match<"A*">(test_string);
        } else if (pattern_str == "b*") {
            matched = ctre::match<"b*">(test_string);
        } else if (pattern_str == "z*") {
            matched = ctre::match<"z*">(test_string);
        } else if (pattern_str == "9*") {
            matched = ctre::match<"9*">(test_string);
        } else if (pattern_str == "a+") {
            matched = ctre::match<"a+">(test_string);
        } else if (pattern_str == "A+") {
            matched = ctre::match<"A+">(test_string);
        } else if (pattern_str == "b+") {
            matched = ctre::match<"b+">(test_string);
        } else if (pattern_str == "z+") {
            matched = ctre::match<"z+">(test_string);
        } else if (pattern_str == "9+") {
            matched = ctre::match<"9+">(test_string);
        } else if (pattern_str == "[a-z]*") {
            matched = ctre::match<"[a-z]*">(test_string);
        } else if (pattern_str == "[0-9]*") {
            matched = ctre::match<"[0-9]*">(test_string);
        } else if (pattern_str == "[A-Z]*") {
            matched = ctre::match<"[A-Z]*">(test_string);
        } else if (pattern_str == "[a-z]+") {
            matched = ctre::match<"[a-z]+">(test_string);
        } else if (pattern_str == "[0-9]+") {
            matched = ctre::match<"[0-9]+">(test_string);
        } else if (pattern_str == "[A-Z]+") {
            matched = ctre::match<"[A-Z]+">(test_string);
        } else if (pattern_str == "[a-c]*") {
            matched = ctre::match<"[a-c]*">(test_string);
        } else if (pattern_str == "[0-2]*") {
            matched = ctre::match<"[0-2]*">(test_string);
        } else if (pattern_str == "[x-z]*") {
            matched = ctre::match<"[x-z]*">(test_string);
        } else if (pattern_str == "[a-e]*") {
            matched = ctre::match<"[a-e]*">(test_string);
        } else if (pattern_str == "[a-c]+") {
            matched = ctre::match<"[a-c]+">(test_string);
        } else if (pattern_str == "[0-2]+") {
            matched = ctre::match<"[0-2]+">(test_string);
        } else if (pattern_str == "[x-z]+") {
            matched = ctre::match<"[x-z]+">(test_string);
        } else if (pattern_str == "[a-e]+") {
            matched = ctre::match<"[a-e]+">(test_string);
        }
        
        if (matched) {
            matches++;
        }
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    
    return static_cast<double>(duration.count()) / matches;
}

int main() {
    // Test cases covering different SIMD optimizations
    std::vector<TestCase> test_cases = {
        // Single character repetition (single char SIMD) - star patterns
        {"a*_16", "a*", "Single char 'a' (16 chars)"},
        {"a*_32", "a*", "Single char 'a' (32 chars)"},
        {"a*_64", "a*", "Single char 'a' (64 chars)"},
        {"a*_128", "a*", "Single char 'a' (128 chars)"},
        {"A*_32", "A*", "Single char 'A' (32 chars)"},
        {"b*_32", "b*", "Single char 'b' (32 chars)"},
        {"z*_32", "z*", "Single char 'z' (32 chars)"},
        {"9*_32", "9*", "Single char '9' (32 chars)"},
        
        // Single character repetition (single char SIMD) - plus patterns
        {"a+_16", "a+", "Single char 'a' plus (16 chars)"},
        {"a+_32", "a+", "Single char 'a' plus (32 chars)"},
        {"a+_64", "a+", "Single char 'a' plus (64 chars)"},
        {"a+_128", "a+", "Single char 'a' plus (128 chars)"},
        {"A+_32", "A+", "Single char 'A' plus (32 chars)"},
        {"b+_32", "b+", "Single char 'b' plus (32 chars)"},
        {"z+_32", "z+", "Single char 'z' plus (32 chars)"},
        {"9+_32", "9+", "Single char '9' plus (32 chars)"},
        
        // Character class repetition (character class SIMD) - star patterns
        {"[a-z]*_16", "[a-z]*", "Lowercase range (16 chars)"},
        {"[a-z]*_32", "[a-z]*", "Lowercase range (32 chars)"},
        {"[a-z]*_64", "[a-z]*", "Lowercase range (64 chars)"},
        {"[a-z]*_128", "[a-z]*", "Lowercase range (128 chars)"},
        {"[A-Z]*_32", "[A-Z]*", "Uppercase range (32 chars)"},
        
        // Character class repetition (character class SIMD) - plus patterns
        {"[a-z]+_16", "[a-z]+", "Lowercase range plus (16 chars)"},
        {"[a-z]+_32", "[a-z]+", "Lowercase range plus (32 chars)"},
        {"[a-z]+_64", "[a-z]+", "Lowercase range plus (64 chars)"},
        {"[a-z]+_128", "[a-z]+", "Lowercase range plus (128 chars)"},
        {"[A-Z]+_32", "[A-Z]+", "Uppercase range plus (32 chars)"},
        
        // Small ranges (small range optimization) - star patterns
        {"[a-c]*_32", "[a-c]*", "Small range a-c (32 chars)"},
        {"[0-2]*_32", "[0-2]*", "Small range 0-2 (32 chars)"},
        {"[x-z]*_32", "[x-z]*", "Small range x-z (32 chars)"},
        {"[a-e]*_32", "[a-e]*", "Small range a-e (32 chars)"},
        {"[0-9]*_32", "[0-9]*", "Small range 0-9 (32 chars)"},
        
        // Small ranges (small range optimization) - plus patterns
        {"[a-c]+_32", "[a-c]+", "Small range a-c plus (32 chars)"},
        {"[0-2]+_32", "[0-2]+", "Small range 0-2 plus (32 chars)"},
        {"[x-z]+_32", "[x-z]+", "Small range x-z plus (32 chars)"},
        {"[a-e]+_32", "[a-e]+", "Small range a-e plus (32 chars)"},
        {"[0-9]+_32", "[0-9]+", "Small range 0-9 plus (32 chars)"},
    };
    
    for (const auto& test_case : test_cases) {
        // Extract string length from test case name
        size_t length = 32; // default
        if (test_case.name.find("_16") != std::string::npos) length = 16;
        else if (test_case.name.find("_32") != std::string::npos) length = 32;
        else if (test_case.name.find("_64") != std::string::npos) length = 64;
        else if (test_case.name.find("_128") != std::string::npos) length = 128;
        
        // Generate test string
        std::string test_string = generate_test_string(test_case.pattern, length);
        
        // Benchmark with SIMD enabled (default)
        double simd_time = benchmark_case(test_case.pattern, test_string, 10000);
        
        // Output result
        std::cout << test_case.name << "," << simd_time << "\n";
    }
    
    return 0;
}
