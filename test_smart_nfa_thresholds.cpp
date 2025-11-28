#include <ctre.hpp>
#include <ctre/bitnfa/integration.hpp>
#include <iostream>
#include <chrono>
#include <string>
#include <vector>

// Test different NFA thresholds to find optimal settings

struct TestCase {
    std::string name;
    std::string pattern;
    std::string input;
    size_t input_size;
    bool is_alternation;
    bool is_complex;
};

// Benchmark a pattern with CTRE engine
template<ctll::fixed_string Pattern>
double benchmark_ctre(const std::string& input, int iterations = 50000) {
    auto start = std::chrono::high_resolution_clock::now();

    volatile bool result = false;
    for (int i = 0; i < iterations; ++i) {
        result = (bool)ctre::match<Pattern>(input);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    return duration / (double)iterations;
}

// Benchmark a pattern with BitNFA engine
template<ctll::fixed_string Pattern>
double benchmark_bitnfa(const std::string& input, int iterations = 50000) {
    auto start = std::chrono::high_resolution_clock::now();

    volatile bool result = false;
    for (int i = 0; i < iterations; ++i) {
        result = ctre::bitnfa::match<Pattern>(input).matched;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    return duration / (double)iterations;
}

int main() {
    std::cout << "╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║     Smart NFA Threshold Exploration                      ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;

    std::cout << "Testing: When should we use BitNFA vs Glushkov NFA vs SIMD?" << std::endl;
    std::cout << std::endl;

    // Test cases: alternation patterns with varying input sizes
    std::vector<std::pair<std::string, std::vector<size_t>>> test_cases = {
        {"complex_alt", {10, 20, 50, 100, 200}},
        {"alternation_4", {10, 20, 50, 100, 200}},
        {"group_alt", {10, 20, 50, 100, 200}},
    };

    std::cout << "═══════════════════════════════════════════════════════════" << std::endl;
    std::cout << " TEST 1: complex_alt (Huck[a-zA-Z]+|Saw[a-zA-Z]+)" << std::endl;
    std::cout << "═══════════════════════════════════════════════════════════" << std::endl;
    std::cout << std::endl;

    // Test complex_alt with different input sizes
    for (size_t size : {10, 20, 50, 100}) {
        std::string input = "Huckleberry";
        input.resize(size, 'x');

        std::cout << "Input size: " << size << " bytes" << std::endl;

        // Test with standard CTRE (Glushkov NFA + SIMD)
        auto ctre_time = benchmark_ctre<"Huck[a-zA-Z]+|Saw[a-zA-Z]+">(input);
        std::cout << "  CTRE (Glushkov NFA + SIMD): " << ctre_time << " ns" << std::endl;

        // Test with BitNFA
        auto bitnfa_time = benchmark_bitnfa<"Huck[a-zA-Z]+|Saw[a-zA-Z]+">(input);
        std::cout << "  BitNFA:                      " << bitnfa_time << " ns" << std::endl;

        double ratio = bitnfa_time / ctre_time;
        std::cout << "  BitNFA/CTRE ratio: " << ratio << "x";

        if (ratio < 1.0) {
            std::cout << " ✅ BitNFA WINS!" << std::endl;
        } else if (ratio < 1.1) {
            std::cout << " ⚠️ Close (within 10%)" << std::endl;
        } else {
            std::cout << " ❌ CTRE WINS" << std::endl;
        }

        std::cout << std::endl;
    }

    std::cout << "═══════════════════════════════════════════════════════════" << std::endl;
    std::cout << " TEST 2: alternation_4 (Tom|Sawyer|Huckleberry|Finn)" << std::endl;
    std::cout << "═══════════════════════════════════════════════════════════" << std::endl;
    std::cout << std::endl;

    for (size_t size : {10, 20, 50, 100}) {
        std::string input = "Huckleberry";
        input.resize(size, 'x');

        std::cout << "Input size: " << size << " bytes" << std::endl;

        auto ctre_time = benchmark_ctre<"Tom|Sawyer|Huckleberry|Finn">(input);
        std::cout << "  CTRE:   " << ctre_time << " ns" << std::endl;

        auto bitnfa_time = benchmark_bitnfa<"Tom|Sawyer|Huckleberry|Finn">(input);
        std::cout << "  BitNFA: " << bitnfa_time << " ns" << std::endl;

        double ratio = bitnfa_time / ctre_time;
        std::cout << "  Ratio: " << ratio << "x";

        if (ratio < 1.0) {
            std::cout << " ✅ BitNFA WINS!" << std::endl;
        } else {
            std::cout << " ❌ CTRE WINS" << std::endl;
        }

        std::cout << std::endl;
    }

    std::cout << "═══════════════════════════════════════════════════════════" << std::endl;
    std::cout << " RECOMMENDATIONS" << std::endl;
    std::cout << "═══════════════════════════════════════════════════════════" << std::endl;
    std::cout << std::endl;
    std::cout << "Based on results, suggest optimal thresholds:" << std::endl;
    std::cout << "  • Use BitNFA if: alternation_count >= X && input_size >= Y" << std::endl;
    std::cout << "  • Use Glushkov NFA otherwise" << std::endl;
    std::cout << std::endl;

    return 0;
}
