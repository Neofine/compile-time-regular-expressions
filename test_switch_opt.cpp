#include <iostream>
#include <chrono>
#include <string>
#include <cstring>
#include <ctre.hpp>

// Test switch-based literal matching

template<typename Func>
double bench(Func&& f) {
    // Warmup
    for (int i = 0; i < 10000; ++i) {
        f();
    }

    // Benchmark with 10 samples (like real benchmark)
    double min_time = 1e9;
    for (int sample = 0; sample < 10; ++sample) {
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < 100000; ++i) {
            f();
        }
        auto end = std::chrono::high_resolution_clock::now();
        double time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / 100000.0;
        if (time_ns < min_time) min_time = time_ns;
    }
    return min_time;
}

// Optimized switch-based matcher
inline bool match_switch_optimized(std::string_view input) noexcept {
    if (input.empty()) return false;

    switch (input[0]) {
        case 'T':
            return input == "Tom";
        case 'S':
            return input == "Sawyer";
        case 'H':
            return input == "Huckleberry";
        case 'F':
            return input == "Finn";
        default:
            return false;
    }
}

int main() {
    std::string input = "Tom";
    volatile bool result = false;

    std::cout << "╔═══════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║    Switch-Based Literal Matching - Final Test                       ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;

    // Test 1: Switch-based
    auto t_switch = bench([&]() {
        result = match_switch_optimized(input);
    });

    // Test 2: CTRE
    auto t_ctre = bench([&]() {
        result = (bool)ctre::match<"Tom|Sawyer|Huckleberry|Finn">(input);
    });

    std::cout << "Approach                 | Time (ns)   | vs CTRE" << std::endl;
    std::cout << "-------------------------|-------------|---------" << std::endl;
    std::cout << "Switch-based matching    | " << std::fixed << std::setprecision(3) << t_switch << " ns | " << (t_ctre / t_switch) << "x";

    if (t_switch < t_ctre) {
        std::cout << " ✅ FASTER!" << std::endl;
    } else {
        std::cout << " ⚠️  slower" << std::endl;
    }

    std::cout << "CTRE (baseline)          | " << t_ctre << " ns | 1.00x" << std::endl;
    std::cout << std::endl;

    if (t_switch < t_ctre) {
        std::cout << "🔥 SUCCESS! Switch-based matching is " << (t_ctre / t_switch) << "x faster!" << std::endl;
        std::cout << std::endl;
        std::cout << "This is the optimization we need!" << std::endl;
        std::cout << "  • Fastest approach for few literals" << std::endl;
        std::cout << "  • Beat CTRE's compile-time evaluation!" << std::endl;
    } else {
        std::cout << "ℹ️  CTRE is still faster (" << (t_switch / t_ctre) << "x)" << std::endl;
        std::cout << "   CTRE's compile-time optimization is hard to beat!" << std::endl;
    }

    std::cout << std::endl;

    return 0;
}
