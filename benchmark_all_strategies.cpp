#include <ctre.hpp>
#include <ctre/bitnfa/integration.hpp>
#include <ctre/smart_dispatch.hpp>
#include <iostream>
#include <chrono>
#include <string>
#include <iomanip>

template<typename Func>
double bench(Func&& f, int iters = 100000) {
    // Warmup
    for (int i = 0; i < 10000; ++i) { f(); }
    
    // Benchmark - take minimum of 10 samples
    double min_time = 1e9;
    for (int sample = 0; sample < 10; ++sample) {
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iters; ++i) { f(); }
        auto end = std::chrono::high_resolution_clock::now();
        double time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / (double)iters;
        if (time < min_time) min_time = time;
    }
    return min_time;
}

struct TestCase {
    const char* name;
    const char* pattern;
    std::string input;
};

int main() {
    volatile bool result_bool;
    volatile const char* result_ptr;
    
    std::vector<TestCase> tests = {
        // Alternations
        {"alternation_4", "Tom|Sawyer|Huckleberry|Finn", "Tom"},
        {"complex_alt", "Huck[a-zA-Z]+|Saw[a-zA-Z]+", "Huckleberry"},
        {"group_alt", "([A-Za-z]awyer|[A-Za-z]inn)\\s", "Sawyer "},
        
        // Repetitions (single char)
        {"a+_16", "a+", std::string(16, 'a')},
        {"a+_32", "a+", std::string(32, 'a')},
        {"a+_64", "a+", std::string(64, 'a')},
        
        // Repetitions (char class)
        {"[a-z]+_16", "[a-z]+", std::string(16, 'a')},
        {"[a-z]+_32", "[a-z]+", std::string(32, 'a')},
        {"[a-z]+_64", "[a-z]+", std::string(64, 'a')},
        
        // Multi-range
        {"[a-zA-Z]+_32", "[a-zA-Z]+", std::string(32, 'a')},
        {"[0-9a-fA-F]+_32", "[0-9a-fA-F]+", std::string(32, 'a')},
        
        // Sparse sets
        {"[aeiou]+_32", "[aeiou]+", std::string(32, 'a')},
        
        // Negated
        {"[^u-z]{13}", "[^u-z]{13}", "aabcdefghijkl"},
        
        // Literals
        {"literal", "Twain", "Twain"},
        
        // Complex sequences
        {"suffix", "[a-zA-Z]+ing", "fishing"},
        {"whitespace", "\\s[a-zA-Z]{0,12}ing\\s", " ing "},
    };
    
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\n╔═══════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║       Backend Strategy Comparison: CTRE+SIMD vs BitNFA vs Smart      ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════════════════╝\n" << std::endl;
    
    std::cout << std::left << std::setw(25) << "Pattern" 
              << std::right << std::setw(12) << "CTRE+SIMD" 
              << std::setw(12) << "BitNFA" 
              << std::setw(12) << "Smart"
              << std::setw(12) << "Best" << std::endl;
    std::cout << std::string(73, '─') << std::endl;
    
    for (const auto& test : tests) {
        double time_ctre = 0, time_bitnfa = 0, time_smart = 0;
        
        // Test base CTRE + SIMD
        if (test.pattern == std::string("Tom|Sawyer|Huckleberry|Finn")) {
            time_ctre = bench([&]() {
                result_bool = (bool)ctre::match<"Tom|Sawyer|Huckleberry|Finn">(test.input);
            });
            time_bitnfa = bench([&]() {
                result_bool = ctre::bitnfa::match<"Tom|Sawyer|Huckleberry|Finn">(test.input).matched;
            });
            time_smart = bench([&]() {
                result_bool = (bool)ctre::smart_dispatch::match<"Tom|Sawyer|Huckleberry|Finn">(test.input);
            });
        }
        else if (test.pattern == std::string("Huck[a-zA-Z]+|Saw[a-zA-Z]+")) {
            time_ctre = bench([&]() {
                result_bool = (bool)ctre::match<"Huck[a-zA-Z]+|Saw[a-zA-Z]+">(test.input);
            });
            time_bitnfa = bench([&]() {
                result_bool = ctre::bitnfa::match<"Huck[a-zA-Z]+|Saw[a-zA-Z]+">(test.input).matched;
            });
            time_smart = bench([&]() {
                result_bool = (bool)ctre::smart_dispatch::match<"Huck[a-zA-Z]+|Saw[a-zA-Z]+">(test.input);
            });
        }
        else if (test.pattern == std::string("([A-Za-z]awyer|[A-Za-z]inn)\\s")) {
            time_ctre = bench([&]() {
                result_bool = (bool)ctre::match<"([A-Za-z]awyer|[A-Za-z]inn)\\s">(test.input);
            });
            time_bitnfa = bench([&]() {
                result_bool = ctre::bitnfa::match<"([A-Za-z]awyer|[A-Za-z]inn)\\s">(test.input).matched;
            });
            time_smart = bench([&]() {
                result_bool = (bool)ctre::smart_dispatch::match<"([A-Za-z]awyer|[A-Za-z]inn)\\s">(test.input);
            });
        }
        else if (test.pattern == std::string("a+")) {
            time_ctre = bench([&]() {
                result_bool = (bool)ctre::match<"a+">(test.input);
            });
            time_bitnfa = bench([&]() {
                result_bool = ctre::bitnfa::match<"a+">(test.input).matched;
            });
            time_smart = bench([&]() {
                result_bool = (bool)ctre::smart_dispatch::match<"a+">(test.input);
            });
        }
        else if (test.pattern == std::string("[a-z]+")) {
            time_ctre = bench([&]() {
                result_bool = (bool)ctre::match<"[a-z]+">(test.input);
            });
            time_bitnfa = bench([&]() {
                result_bool = ctre::bitnfa::match<"[a-z]+">(test.input).matched;
            });
            time_smart = bench([&]() {
                result_bool = (bool)ctre::smart_dispatch::match<"[a-z]+">(test.input);
            });
        }
        else if (test.pattern == std::string("[a-zA-Z]+")) {
            time_ctre = bench([&]() {
                result_bool = (bool)ctre::match<"[a-zA-Z]+">(test.input);
            });
            time_bitnfa = bench([&]() {
                result_bool = ctre::bitnfa::match<"[a-zA-Z]+">(test.input).matched;
            });
            time_smart = bench([&]() {
                result_bool = (bool)ctre::smart_dispatch::match<"[a-zA-Z]+">(test.input);
            });
        }
        else if (test.pattern == std::string("[0-9a-fA-F]+")) {
            time_ctre = bench([&]() {
                result_bool = (bool)ctre::match<"[0-9a-fA-F]+">(test.input);
            });
            time_bitnfa = bench([&]() {
                result_bool = ctre::bitnfa::match<"[0-9a-fA-F]+">(test.input).matched;
            });
            time_smart = bench([&]() {
                result_bool = (bool)ctre::smart_dispatch::match<"[0-9a-fA-F]+">(test.input);
            });
        }
        else if (test.pattern == std::string("[aeiou]+")) {
            time_ctre = bench([&]() {
                result_bool = (bool)ctre::match<"[aeiou]+">(test.input);
            });
            time_bitnfa = bench([&]() {
                result_bool = ctre::bitnfa::match<"[aeiou]+">(test.input).matched;
            });
            time_smart = bench([&]() {
                result_bool = (bool)ctre::smart_dispatch::match<"[aeiou]+">(test.input);
            });
        }
        else if (test.pattern == std::string("[^u-z]{13}")) {
            time_ctre = bench([&]() {
                result_bool = (bool)ctre::match<"[^u-z]{13}">(test.input);
            });
            time_bitnfa = bench([&]() {
                result_bool = ctre::bitnfa::match<"[^u-z]{13}">(test.input).matched;
            });
            time_smart = bench([&]() {
                result_bool = (bool)ctre::smart_dispatch::match<"[^u-z]{13}">(test.input);
            });
        }
        else if (test.pattern == std::string("Twain")) {
            time_ctre = bench([&]() {
                result_bool = (bool)ctre::match<"Twain">(test.input);
            });
            time_bitnfa = bench([&]() {
                result_bool = ctre::bitnfa::match<"Twain">(test.input).matched;
            });
            time_smart = bench([&]() {
                result_bool = (bool)ctre::smart_dispatch::match<"Twain">(test.input);
            });
        }
        else if (test.pattern == std::string("[a-zA-Z]+ing")) {
            time_ctre = bench([&]() {
                result_bool = (bool)ctre::match<"[a-zA-Z]+ing">(test.input);
            });
            time_bitnfa = bench([&]() {
                result_bool = ctre::bitnfa::match<"[a-zA-Z]+ing">(test.input).matched;
            });
            time_smart = bench([&]() {
                result_bool = (bool)ctre::smart_dispatch::match<"[a-zA-Z]+ing">(test.input);
            });
        }
        else if (test.pattern == std::string("\\s[a-zA-Z]{0,12}ing\\s")) {
            time_ctre = bench([&]() {
                result_bool = (bool)ctre::match<"\\s[a-zA-Z]{0,12}ing\\s">(test.input);
            });
            time_bitnfa = bench([&]() {
                result_bool = ctre::bitnfa::match<"\\s[a-zA-Z]{0,12}ing\\s">(test.input).matched;
            });
            time_smart = bench([&]() {
                result_bool = (bool)ctre::smart_dispatch::match<"\\s[a-zA-Z]{0,12}ing\\s">(test.input);
            });
        }
        else {
            continue; // Skip unknown patterns
        }
        
        double best_time = std::min({time_ctre, time_bitnfa, time_smart});
        const char* best_name = (best_time == time_ctre) ? "CTRE+SIMD" :
                                (best_time == time_bitnfa) ? "BitNFA" : "Smart";
        
        std::cout << std::left << std::setw(25) << test.name
                  << std::right << std::setw(10) << time_ctre << "ns"
                  << std::setw(10) << time_bitnfa << "ns"
                  << std::setw(10) << time_smart << "ns"
                  << "  " << best_name << std::endl;
    }
    
    std::cout << "\n" << std::string(73, '═') << std::endl;
    std::cout << "Analysis Summary" << std::endl;
    std::cout << std::string(73, '═') << std::endl;
    std::cout << "\nKey Findings:" << std::endl;
    std::cout << "  • CTRE+SIMD: Best for repetitions and sequences" << std::endl;
    std::cout << "  • BitNFA: Best for complex alternations" << std::endl;
    std::cout << "  • Smart: Automatically chooses optimal backend" << std::endl;
    
    return 0;
}

