#include <chrono>
#include <iostream>
#include <string>
#include <ctre.hpp>
#include <ctre/literal_optimization.hpp>

int main() {
    const int ITERATIONS = 100000;
    std::string test_input = std::string("Tom");

    using tmp = typename ctll::parser<ctre::pcre, "Tom|Sawyer|Huckleberry|Finn", ctre::pcre_actions>::template output<ctre::pcre_context<>>;
    using Pattern = decltype(ctll::front(typename tmp::output_type::stack_type()));
    constexpr auto literals = ctre::get_literal_list<Pattern>();

    std::cout << "╔═══════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║    Exact Benchmark Replica - Finding the Bug                         ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;

    // =========================================================================
    // TEST 1: Complete Teddy (what we're testing)
    // =========================================================================

    std::cout << "TEST 1: Complete Teddy" << std::endl;
    std::cout << "======================" << std::endl;

    // Warmup
    for (int i = 0; i < 10000; ++i) {
        volatile bool r = (ctre::teddy_complete::match(test_input, literals) > 0);
    }

    // Benchmark (same as real benchmark)
    double min_time1 = 1e9;
    for (int sample = 0; sample < 10; ++sample) {
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < ITERATIONS; ++i) {
            volatile bool r = (ctre::teddy_complete::match(test_input, literals) > 0);
        }
        auto end = std::chrono::high_resolution_clock::now();
        double time_ns = std::chrono::duration<double, std::nano>(end - start).count() / ITERATIONS;
        if (time_ns < min_time1) min_time1 = time_ns;
    }

    std::cout << "Time: " << min_time1 << " ns" << std::endl;
    std::cout << std::endl;

    // =========================================================================
    // TEST 2: CTRE (baseline)
    // =========================================================================

    std::cout << "TEST 2: CTRE (baseline)" << std::endl;
    std::cout << "=======================" << std::endl;

    // Warmup
    for (int i = 0; i < 10000; ++i) {
        volatile bool r = static_cast<bool>(ctre::match<"Tom|Sawyer|Huckleberry|Finn">(test_input));
    }

    // Benchmark
    double min_time2 = 1e9;
    for (int sample = 0; sample < 10; ++sample) {
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < ITERATIONS; ++i) {
            volatile bool r = static_cast<bool>(ctre::match<"Tom|Sawyer|Huckleberry|Finn">(test_input));
        }
        auto end = std::chrono::high_resolution_clock::now();
        double time_ns = std::chrono::duration<double, std::nano>(end - start).count() / ITERATIONS;
        if (time_ns < min_time2) min_time2 = time_ns;
    }

    std::cout << "Time: " << min_time2 << " ns" << std::endl;
    std::cout << std::endl;

    // =========================================================================
    // TEST 3: Simple literal scan
    // =========================================================================

    std::cout << "TEST 3: Simple literal scan" << std::endl;
    std::cout << "============================" << std::endl;

    // Warmup
    for (int i = 0; i < 10000; ++i) {
        volatile bool r = (literals.fast_match(test_input) > 0);
    }

    // Benchmark
    double min_time3 = 1e9;
    for (int sample = 0; sample < 10; ++sample) {
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < ITERATIONS; ++i) {
            volatile bool r = (literals.fast_match(test_input) > 0);
        }
        auto end = std::chrono::high_resolution_clock::now();
        double time_ns = std::chrono::duration<double, std::nano>(end - start).count() / ITERATIONS;
        if (time_ns < min_time3) min_time3 = time_ns;
    }

    std::cout << "Time: " << min_time3 << " ns" << std::endl;
    std::cout << std::endl;

    // =========================================================================
    // RESULTS
    // =========================================================================

    std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << " RESULTS" << std::endl;
    std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << std::endl;

    std::cout << "Complete Teddy:     " << min_time1 << " ns" << std::endl;
    std::cout << "CTRE (baseline):    " << min_time2 << " ns" << std::endl;
    std::cout << "Simple scan:        " << min_time3 << " ns" << std::endl;
    std::cout << std::endl;

    std::cout << "Speedup vs CTRE:" << std::endl;
    std::cout << "  Complete Teddy:  " << (min_time2 / min_time1) << "x" << std::endl;
    std::cout << "  Simple scan:     " << (min_time2 / min_time3) << "x" << std::endl;
    std::cout << std::endl;

    if (min_time1 < min_time2) {
        std::cout << "✅ Complete Teddy is FASTER!" << std::endl;
    } else {
        std::cout << "❌ Complete Teddy is SLOWER!" << std::endl;
        std::cout << "   Overhead: +" << (min_time1 - min_time2) << " ns" << std::endl;
    }

    std::cout << std::endl;

    return 0;
}
