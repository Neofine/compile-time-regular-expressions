#include <chrono>
#include <ctre.hpp>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <vector>

// Test cases for proper CTRE regex patterns
struct TestCase {
    std::string name;
    std::string pattern;
    std::string description;
};

// Generate test strings that will match the regex patterns
std::string generate_test_string(const std::string& pattern, size_t length) {
    std::random_device rd;
    std::mt19937 gen(rd());

    if (pattern == "a*") {
        return std::string(length, 'a');
    } else if (pattern == "A*") {
        return std::string(length, 'A');
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
            result += static_cast<char>((val < 26) ? ('a' + val) : ('A' + val - 26));
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
    } else if (pattern == "[aeiou]*" || pattern == "[aeiou]+") {
        const char vowels[] = "aeiou";
        std::uniform_int_distribution<> dis(0, 4);
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += vowels[dis(gen)];
        }
        return result;
    } else if (pattern == "[aeiouAEIOU]*") {
        const char vowels[] = "aeiouAEIOU";
        std::uniform_int_distribution<> dis(0, 9);
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += vowels[dis(gen)];
        }
        return result;
    } else if (pattern == "[02468]*" || pattern == "[02468]+") {
        const char evens[] = "02468";
        std::uniform_int_distribution<> dis(0, 4);
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += evens[dis(gen)];
        }
        return result;
    } else if (pattern == "[13579]*" || pattern == "[13579]+") {
        const char odds[] = "13579";
        std::uniform_int_distribution<> dis(0, 4);
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += odds[dis(gen)];
        }
        return result;
    } else if (pattern == "[0-9a-fA-F]*" || pattern == "[0-9a-fA-F]+") {
        const char hex[] = "0123456789abcdefABCDEF";
        std::uniform_int_distribution<> dis(0, 21);
        std::string result;
        for (size_t i = 0; i < length; ++i) {
            result += hex[dis(gen)];
        }
        return result;
    }

    return std::string(length, 'a'); // fallback
}

// Benchmark a single test case
double benchmark_case(const std::string& pattern_str, const std::string& test_string, int iterations = 1000000) {
    // Warmup runs to ensure consistent timing
    for (int warmup = 0; warmup < 10000; ++warmup) {
        if (pattern_str == "a*") {
            ctre::match<"a*">(test_string);
        } else if (pattern_str == "A*") {
            ctre::match<"A*">(test_string);
        } else if (pattern_str == "b*") {
            ctre::match<"b*">(test_string);
        } else if (pattern_str == "z*") {
            ctre::match<"z*">(test_string);
        } else if (pattern_str == "9*") {
            ctre::match<"9*">(test_string);
        } else if (pattern_str == "a+") {
            ctre::match<"a+">(test_string);
        } else if (pattern_str == "A+") {
            ctre::match<"A+">(test_string);
        } else if (pattern_str == "b+") {
            ctre::match<"b+">(test_string);
        } else if (pattern_str == "z+") {
            ctre::match<"z+">(test_string);
        } else if (pattern_str == "9+") {
            ctre::match<"9+">(test_string);
        } else if (pattern_str == "[a-z]*") {
            ctre::match<"[a-z]*">(test_string);
        } else if (pattern_str == "[0-9]*") {
            ctre::match<"[0-9]*">(test_string);
        } else if (pattern_str == "[A-Z]*") {
            ctre::match<"[A-Z]*">(test_string);
        } else if (pattern_str == "[a-zA-Z]*") {
            ctre::match<"[a-zA-Z]*">(test_string);
        } else if (pattern_str == "[0-9a-f]*") {
            ctre::match<"[0-9a-f]*">(test_string);
        } else if (pattern_str == "[a-z]+") {
            ctre::match<"[a-z]+">(test_string);
        } else if (pattern_str == "[0-9]+") {
            ctre::match<"[0-9]+">(test_string);
        } else if (pattern_str == "[A-Z]+") {
            ctre::match<"[A-Z]+">(test_string);
        } else if (pattern_str == "[0-2]*") {
            ctre::match<"[0-2]*">(test_string);
        } else if (pattern_str == "[x-z]*") {
            ctre::match<"[x-z]*">(test_string);
        } else if (pattern_str == "[a-c]*") {
            ctre::match<"[a-c]*">(test_string);
        } else if (pattern_str == "[a-e]*") {
            ctre::match<"[a-e]*">(test_string);
        } else if (pattern_str == "[0-2]+") {
            ctre::match<"[0-2]+">(test_string);
        } else if (pattern_str == "[x-z]+") {
            ctre::match<"[x-z]+">(test_string);
        } else if (pattern_str == "[a-c]+") {
            ctre::match<"[a-c]+">(test_string);
        } else if (pattern_str == "[a-e]+") {
            ctre::match<"[a-e]+">(test_string);
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
            } else if (pattern_str == "[a-zA-Z]*") {
                matched = ctre::match<"[a-zA-Z]*">(test_string);
            } else if (pattern_str == "[0-9a-f]*") {
                matched = ctre::match<"[0-9a-f]*">(test_string);
            } else if (pattern_str == "[a-z]+") {
                matched = ctre::match<"[a-z]+">(test_string);
            } else if (pattern_str == "[0-9]+") {
                matched = ctre::match<"[0-9]+">(test_string);
            } else if (pattern_str == "[A-Z]+") {
                matched = ctre::match<"[A-Z]+">(test_string);
            } else if (pattern_str == "[aeiou]*") {
                matched = ctre::match<"[aeiou]*">(test_string);
            } else if (pattern_str == "[aeiou]+") {
                matched = ctre::match<"[aeiou]+">(test_string);
            } else if (pattern_str == "[aeiouAEIOU]*") {
                matched = ctre::match<"[aeiouAEIOU]*">(test_string);
            } else if (pattern_str == "[02468]*") {
                matched = ctre::match<"[02468]*">(test_string);
            } else if (pattern_str == "[02468]+") {
                matched = ctre::match<"[02468]+">(test_string);
            } else if (pattern_str == "[13579]*") {
                matched = ctre::match<"[13579]*">(test_string);
            } else if (pattern_str == "[13579]+") {
                matched = ctre::match<"[13579]+">(test_string);
            } else if (pattern_str == "[0-9a-fA-F]*") {
                matched = ctre::match<"[0-9a-fA-F]*">(test_string);
            } else if (pattern_str == "[0-9a-fA-F]+") {
                matched = ctre::match<"[0-9a-fA-F]+">(test_string);
            } else if (pattern_str == "[0-2]*") {
                matched = ctre::match<"[0-2]*">(test_string);
            } else if (pattern_str == "[x-z]*") {
                matched = ctre::match<"[x-z]*">(test_string);
            } else if (pattern_str == "[a-c]*") {
                matched = ctre::match<"[a-c]*">(test_string);
            } else if (pattern_str == "[a-e]*") {
                matched = ctre::match<"[a-e]*">(test_string);
            } else if (pattern_str == "[0-2]+") {
                matched = ctre::match<"[0-2]+">(test_string);
            } else if (pattern_str == "[x-z]+") {
                matched = ctre::match<"[x-z]+">(test_string);
            } else if (pattern_str == "[a-c]+") {
                matched = ctre::match<"[a-c]+">(test_string);
            } else if (pattern_str == "[a-e]+") {
                matched = ctre::match<"[a-e]+">(test_string);
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
        
        // Mixed ranges
        {"[a-zA-Z]*_32", "[a-zA-Z]*", "Mixed case star (32 chars)"},
        {"[a-zA-Z]+_32", "[a-zA-Z]+", "Mixed case plus (32 chars)"},
        {"[a-zA-Z]*_64", "[a-zA-Z]*", "Mixed case star (64 chars)"},
        {"[a-zA-Z]+_64", "[a-zA-Z]+", "Mixed case plus (64 chars)"},
        {"[a-zA-Z]*_128", "[a-zA-Z]*", "Mixed case star (128 chars)"},
        {"[0-9a-f]*_32", "[0-9a-f]*", "Hex lowercase star (32 chars)"},
        {"[0-9a-f]+_32", "[0-9a-f]+", "Hex lowercase plus (32 chars)"},
        {"[0-9a-f]*_64", "[0-9a-f]*", "Hex lowercase star (64 chars)"},
        {"[0-9a-fA-F]*_32", "[0-9a-fA-F]*", "Hex mixed case star (32 chars)"},
        {"[0-9a-fA-F]+_32", "[0-9a-fA-F]+", "Hex mixed case plus (32 chars)"},
        
        // Sparse character sets (Shufti)
        {"[aeiou]*_32", "[aeiou]*", "Vowels star (32 chars)"},
        {"[aeiou]+_32", "[aeiou]+", "Vowels plus (32 chars)"},
        {"[aeiou]*_64", "[aeiou]*", "Vowels star (64 chars)"},
        {"[aeiouAEIOU]*_32", "[aeiouAEIOU]*", "All vowels star (32 chars)"},
        {"[02468]*_32", "[02468]*", "Even digits star (32 chars)"},
        {"[02468]+_32", "[02468]+", "Even digits plus (32 chars)"},
        {"[13579]*_32", "[13579]*", "Odd digits star (32 chars)"},
        {"[13579]+_32", "[13579]+", "Odd digits plus (32 chars)"},
        
        // Larger inputs for scaling tests
        {"[a-z]*_256", "[a-z]*", "Lowercase star (256 chars)"},
        {"[a-z]+_256", "[a-z]+", "Lowercase plus (256 chars)"},
        {"[0-9]*_256", "[0-9]*", "Digits star (256 chars)"},
        {"[0-9]+_256", "[0-9]+", "Digits plus (256 chars)"},
        {"[A-Z]*_256", "[A-Z]*", "Uppercase star (256 chars)"},
        {"a*_256", "a*", "Single char a (256 chars)"},
        {"a+_256", "a+", "Single char a plus (256 chars)"},
        {"[a-z]*_512", "[a-z]*", "Lowercase star (512 chars)"},
        {"[a-z]+_512", "[a-z]+", "Lowercase plus (512 chars)"},
    };

    // Output CSV format: pattern,time
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
        else if (test_case.name.find("_256") != std::string::npos)
            length = 256;
        else if (test_case.name.find("_512") != std::string::npos)
            length = 512;

        // Generate test string
        std::string test_string = generate_test_string(test_case.pattern, length);

        // Benchmark
        double time = benchmark_case(test_case.pattern, test_string, 100000);

        // Output CSV: pattern,time
        std::cout << test_case.name << "," << std::fixed << std::setprecision(2) << time << "\n";
    }

    return 0;
}
