#include <chrono>
#include <ctre.hpp>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

// Benchmark a single pattern in complete isolation (no code bloat)
template<ctll::fixed_string Pattern>
double benchmark_isolated(const std::string& test_str, int iterations = 100000) {
    volatile bool result = false;

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        result = ctre::match<Pattern>(test_str);
    }
    auto end = std::chrono::high_resolution_clock::now();

    return std::chrono::duration<double, std::nano>(end - start).count() / iterations;
}

// Helper to generate strings
inline std::string gen_repeat(char c, size_t len) { return std::string(len, c); }
inline std::string gen_range(char start, size_t count, size_t len) {
    std::string result;
    for (size_t i = 0; i < len; ++i) result += static_cast<char>(start + (i % count));
    return result;
}

struct BenchResult {
    const char* name;
    double time_ns;
};

int main() {
    constexpr int ITER = 100000;
    std::vector<BenchResult> results;

    std::cout << "Isolated Pattern Benchmark (100k iterations, no I-cache interference)\n";
    std::cout << "Pattern                  Time (ns)\n";
    std::cout << "-------------------------------------------\n";

    // Single characters
    results.push_back({"a*", benchmark_isolated<"a*">(gen_repeat('a', 32), ITER)});
    results.push_back({"b*", benchmark_isolated<"b*">(gen_repeat('b', 32), ITER)});
    results.push_back({"z*", benchmark_isolated<"z*">(gen_repeat('z', 32), ITER)});
    results.push_back({"9*", benchmark_isolated<"9*">(gen_repeat('9', 32), ITER)});
    results.push_back({"A*", benchmark_isolated<"A*">(gen_repeat('A', 32), ITER)});

    results.push_back({"a+", benchmark_isolated<"a+">(gen_repeat('a', 32), ITER)});
    results.push_back({"b+", benchmark_isolated<"b+">(gen_repeat('b', 32), ITER)});
    results.push_back({"z+", benchmark_isolated<"z+">(gen_repeat('z', 32), ITER)});
    results.push_back({"9+", benchmark_isolated<"9+">(gen_repeat('9', 32), ITER)});
    results.push_back({"A+", benchmark_isolated<"A+">(gen_repeat('A', 32), ITER)});

    // Small ranges (2-5 chars) - THE CRITICAL TEST
    results.push_back({"[0-2]*", benchmark_isolated<"[0-2]*">(gen_range('0', 3, 32), ITER)});
    results.push_back({"[0-2]+", benchmark_isolated<"[0-2]+">(gen_range('0', 3, 32), ITER)});
    results.push_back({"[a-c]*", benchmark_isolated<"[a-c]*">(gen_range('a', 3, 32), ITER)});
    results.push_back({"[a-c]+", benchmark_isolated<"[a-c]+">(gen_range('a', 3, 32), ITER)});
    results.push_back({"[a-e]*", benchmark_isolated<"[a-e]*">(gen_range('a', 5, 32), ITER)});
    results.push_back({"[a-e]+", benchmark_isolated<"[a-e]+">(gen_range('a', 5, 32), ITER)});
    results.push_back({"[x-z]*", benchmark_isolated<"[x-z]*">(gen_range('x', 3, 32), ITER)});
    results.push_back({"[x-z]+", benchmark_isolated<"[x-z]+">(gen_range('x', 3, 32), ITER)});

    // Medium ranges (9-26 chars)
    results.push_back({"[0-9]*", benchmark_isolated<"[0-9]*">(gen_range('0', 10, 32), ITER)});
    results.push_back({"[0-9]+", benchmark_isolated<"[0-9]+">(gen_range('0', 10, 32), ITER)});
    results.push_back({"[a-z]*", benchmark_isolated<"[a-z]*">(gen_range('a', 26, 32), ITER)});
    results.push_back({"[a-z]+", benchmark_isolated<"[a-z]+">(gen_range('a', 26, 32), ITER)});
    results.push_back({"[A-Z]*", benchmark_isolated<"[A-Z]*">(gen_range('A', 26, 32), ITER)});
    results.push_back({"[A-Z]+", benchmark_isolated<"[A-Z]+">(gen_range('A', 26, 32), ITER)});

    // Print results
    for (const auto& r : results) {
        std::cout << std::left << std::setw(25) << r.name
                  << std::fixed << std::setprecision(4) << r.time_ns << "\n";
    }

    return 0;
}
