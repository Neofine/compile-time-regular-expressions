#include <chrono>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <iomanip>
#include <fstream>

// CTRE-SIMD (our optimized version) - include first!
#include "../include/ctre.hpp"

// CTRE (original) - from system
// We need to use the vendor version separately
namespace ctre_original {
    #include "/usr/local/include/ctre.hpp"
}

// std::regex
#include <regex>

// PCRE2
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

// RE2
#ifdef INCLUDE_RE2
#include <re2/re2.h>
#endif

// Hyperscan
#ifdef INCLUDE_HYPERSCAN
#include <hs/hs.h>
#endif

using namespace std::chrono;

// Benchmark configuration
constexpr int ITERATIONS = 1000000;

struct BenchmarkResult {
    std::string engine;
    std::string pattern;
    double time_ns;
    bool matched;
};

std::vector<BenchmarkResult> results;

// Test inputs of various sizes
const std::vector<std::pair<std::string, std::string>> test_cases = {
    {"a+_32", std::string(32, 'a')},
    {"a+_64", std::string(64, 'a')},
    {"a+_128", std::string(128, 'a')},
    {"a+_256", std::string(256, 'a')},
    {"a*_32", std::string(32, 'a')},
    {"a*_64", std::string(64, 'a')},
    {"a*_128", std::string(128, 'a')},
    {"a*_256", std::string(256, 'a')},
    {"[a-z]+_64", std::string(64, 'x')},
    {"[a-z]+_128", std::string(128, 'x')},
    {"[a-z]+_256", std::string(256, 'x')},
    {"[a-z]+_512", std::string(512, 'x')},
    {"[0-9]+_64", std::string(64, '5')},
    {"[0-9]+_128", std::string(128, '5')},
    {"[0-9]+_256", std::string(256, '5')},
    {"[A-Z]+_64", std::string(64, 'X')},
    {"[A-Z]+_128", std::string(128, 'X')},
    {"[A-Z]+_256", std::string(256, 'X')},
    {"literal_32", "the quick brown fox jumps over"},
    {"literal_64", "the quick brown fox jumps over the lazy dog and runs away quickly"},
    {"email", "user@example.com"},
    {"complex_pattern", "test123_value_456"},
};

// Benchmark CTRE (original)
void benchmark_ctre() {
    std::cout << "Running CTRE (original)..." << std::flush;

    for (const auto& [name, input] : test_cases) {
        std::string_view sv(input);

        auto start = high_resolution_clock::now();
        bool matched = false;

        if (name.find("a+") == 0 || name.find("a*") == 0) {
            for (int i = 0; i < ITERATIONS; i++) {
                matched = static_cast<bool>(ctre::match<"a+">(sv));
            }
        } else if (name.find("[a-z]+") == 0) {
            for (int i = 0; i < ITERATIONS; i++) {
                matched = static_cast<bool>(ctre::match<"[a-z]+">(sv));
            }
        } else if (name.find("[0-9]+") == 0) {
            for (int i = 0; i < ITERATIONS; i++) {
                matched = static_cast<bool>(ctre::match<"[0-9]+">(sv));
            }
        } else if (name.find("[A-Z]+") == 0) {
            for (int i = 0; i < ITERATIONS; i++) {
                matched = static_cast<bool>(ctre::match<"[A-Z]+">(sv));
            }
        } else if (name == "email") {
            for (int i = 0; i < ITERATIONS; i++) {
                matched = static_cast<bool>(ctre::match<"[a-zA-Z0-9.+\\-]+@[a-zA-Z0-9.\\-]+\\.[a-zA-Z0-9.\\-]+">(sv));
            }
        } else {
            // Skip complex patterns for now
            continue;
        }

        auto end = high_resolution_clock::now();
        double time_ns = duration_cast<nanoseconds>(end - start).count() / (double)ITERATIONS;

        results.push_back({"CTRE", name, time_ns, matched});
    }

    std::cout << " done\n";
}

// Benchmark CTRE-SIMD (our optimized version)
void benchmark_ctre_simd() {
    std::cout << "Running CTRE-SIMD..." << std::flush;

    for (const auto& [name, input] : test_cases) {
        std::string_view sv(input);

        auto start = high_resolution_clock::now();
        bool matched = false;

        if (name.find("a+") == 0 || name.find("a*") == 0) {
            for (int i = 0; i < ITERATIONS; i++) {
                matched = static_cast<bool>(ctre_simd::match<"a+">(sv));
            }
        } else if (name.find("[a-z]+") == 0) {
            for (int i = 0; i < ITERATIONS; i++) {
                matched = static_cast<bool>(ctre_simd::match<"[a-z]+">(sv));
            }
        } else if (name.find("[0-9]+") == 0) {
            for (int i = 0; i < ITERATIONS; i++) {
                matched = static_cast<bool>(ctre_simd::match<"[0-9]+">(sv));
            }
        } else if (name.find("[A-Z]+") == 0) {
            for (int i = 0; i < ITERATIONS; i++) {
                matched = static_cast<bool>(ctre_simd::match<"[A-Z]+">(sv));
            }
        } else if (name == "email") {
            for (int i = 0; i < ITERATIONS; i++) {
                matched = static_cast<bool>(ctre_simd::match<"[a-zA-Z0-9.+\\-]+@[a-zA-Z0-9.\\-]+\\.[a-zA-Z0-9.\\-]+">(sv));
            }
        } else {
            continue;
        }

        auto end = high_resolution_clock::now();
        double time_ns = duration_cast<nanoseconds>(end - start).count() / (double)ITERATIONS;

        results.push_back({"CTRE-SIMD", name, time_ns, matched});
    }

    std::cout << " done\n";
}

// Benchmark std::regex
void benchmark_std_regex() {
    std::cout << "Running std::regex..." << std::flush;

    for (const auto& [name, input] : test_cases) {
        std::regex re;

        if (name.find("a+") == 0 || name.find("a*") == 0) {
            re = std::regex("a+");
        } else if (name.find("[a-z]+") == 0) {
            re = std::regex("[a-z]+");
        } else if (name.find("[0-9]+") == 0) {
            re = std::regex("[0-9]+");
        } else if (name.find("[A-Z]+") == 0) {
            re = std::regex("[A-Z]+");
        } else if (name == "email") {
            re = std::regex("[a-zA-Z0-9.+\\-]+@[a-zA-Z0-9.\\-]+\\.[a-zA-Z0-9.\\-]+");
        } else {
            continue;
        }

        auto start = high_resolution_clock::now();
        bool matched = false;

        for (int i = 0; i < ITERATIONS; i++) {
            matched = std::regex_match(input, re);
        }

        auto end = high_resolution_clock::now();
        double time_ns = duration_cast<nanoseconds>(end - start).count() / (double)ITERATIONS;

        results.push_back({"std::regex", name, time_ns, matched});
    }

    std::cout << " done\n";
}

// Benchmark PCRE2
void benchmark_pcre2() {
    std::cout << "Running PCRE2..." << std::flush;

    for (const auto& [name, input] : test_cases) {
        const char* pattern_str = nullptr;

        if (name.find("a+") == 0 || name.find("a*") == 0) {
            pattern_str = "a+";
        } else if (name.find("[a-z]+") == 0) {
            pattern_str = "[a-z]+";
        } else if (name.find("[0-9]+") == 0) {
            pattern_str = "[0-9]+";
        } else if (name.find("[A-Z]+") == 0) {
            pattern_str = "[A-Z]+";
        } else if (name == "email") {
            pattern_str = "[a-zA-Z0-9.+\\-]+@[a-zA-Z0-9.\\-]+\\.[a-zA-Z0-9.\\-]+";
        } else {
            continue;
        }

        int errornumber;
        PCRE2_SIZE erroroffset;
        pcre2_code* re = pcre2_compile(
            (PCRE2_SPTR)pattern_str, PCRE2_ZERO_TERMINATED, 0,
            &errornumber, &erroroffset, nullptr);

        if (re == nullptr) continue;

        pcre2_match_data* match_data = pcre2_match_data_create_from_pattern(re, nullptr);

        auto start = high_resolution_clock::now();
        bool matched = false;

        for (int i = 0; i < ITERATIONS; i++) {
            int rc = pcre2_match(re, (PCRE2_SPTR)input.c_str(), input.length(),
                                 0, PCRE2_ANCHORED, match_data, nullptr);
            matched = (rc >= 0);
        }

        auto end = high_resolution_clock::now();
        double time_ns = duration_cast<nanoseconds>(end - start).count() / (double)ITERATIONS;

        results.push_back({"PCRE2", name, time_ns, matched});

        pcre2_match_data_free(match_data);
        pcre2_code_free(re);
    }

    std::cout << " done\n";
}

int main() {
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║       Regex Match Benchmark - SIMD vs Competitors         ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    std::cout << "Configuration:\n";
    std::cout << "  Iterations: " << ITERATIONS << "\n";
    std::cout << "  Test cases: " << test_cases.size() << "\n";
    std::cout << "  Operation: match (anchored)\n\n";

    // Run benchmarks
    benchmark_ctre_simd();
    benchmark_ctre();
    benchmark_std_regex();
    benchmark_pcre2();

    // Print results
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                         RESULTS                            ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";

    // Group by pattern
    std::cout << std::setw(25) << "Pattern" << " | ";
    std::cout << std::setw(12) << "CTRE-SIMD" << " | ";
    std::cout << std::setw(12) << "CTRE" << " | ";
    std::cout << std::setw(12) << "std::regex" << " | ";
    std::cout << std::setw(12) << "PCRE2" << " | ";
    std::cout << std::setw(10) << "Speedup" << "\n";
    std::cout << std::string(100, '-') << "\n";

    for (const auto& [name, input] : test_cases) {
        double ctre_simd_time = 0, ctre_time = 0, std_time = 0, pcre2_time = 0;

        for (const auto& r : results) {
            if (r.pattern == name) {
                if (r.engine == "CTRE-SIMD") ctre_simd_time = r.time_ns;
                else if (r.engine == "CTRE") ctre_time = r.time_ns;
                else if (r.engine == "std::regex") std_time = r.time_ns;
                else if (r.engine == "PCRE2") pcre2_time = r.time_ns;
            }
        }

        if (ctre_time == 0) continue;

        double speedup = ctre_time / ctre_simd_time;

        std::cout << std::setw(25) << name << " | ";
        std::cout << std::setw(10) << std::fixed << std::setprecision(2) << ctre_simd_time << " ns | ";
        std::cout << std::setw(10) << ctre_time << " ns | ";
        std::cout << std::setw(10) << std_time << " ns | ";
        std::cout << std::setw(10) << pcre2_time << " ns | ";
        std::cout << std::setw(8) << speedup << "x\n";
    }

    // Calculate averages
    double total_speedup = 0;
    int count = 0;

    for (const auto& [name, input] : test_cases) {
        double ctre_simd_time = 0, ctre_time = 0;

        for (const auto& r : results) {
            if (r.pattern == name) {
                if (r.engine == "CTRE-SIMD") ctre_simd_time = r.time_ns;
                else if (r.engine == "CTRE") ctre_time = r.time_ns;
            }
        }

        if (ctre_time > 0 && ctre_simd_time > 0) {
            total_speedup += ctre_time / ctre_simd_time;
            count++;
        }
    }

    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                    SUMMARY                                 ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
    std::cout << "Average CTRE-SIMD speedup: " << std::fixed << std::setprecision(2)
              << (total_speedup / count) << "x\n";

    // Save CSV
    std::ofstream csv("match_benchmark_results.csv");
    csv << "Pattern,CTRE-SIMD (ns),CTRE (ns),std::regex (ns),PCRE2 (ns),Speedup\n";

    for (const auto& [name, input] : test_cases) {
        double ctre_simd_time = 0, ctre_time = 0, std_time = 0, pcre2_time = 0;

        for (const auto& r : results) {
            if (r.pattern == name) {
                if (r.engine == "CTRE-SIMD") ctre_simd_time = r.time_ns;
                else if (r.engine == "CTRE") ctre_time = r.time_ns;
                else if (r.engine == "std::regex") std_time = r.time_ns;
                else if (r.engine == "PCRE2") pcre2_time = r.time_ns;
            }
        }

        if (ctre_time == 0) continue;

        csv << name << "," << ctre_simd_time << "," << ctre_time << ","
            << std_time << "," << pcre2_time << "," << (ctre_time / ctre_simd_time) << "\n";
    }

    csv.close();
    std::cout << "\n✅ Results saved to match_benchmark_results.csv\n\n";

    return 0;
}
