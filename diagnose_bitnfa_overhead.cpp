#include <ctre.hpp>
#include "include/ctre/bitnfa/integration.hpp"
#include "include/ctre/bitnfa/literal_fast_path.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>

// Diagnose where BitNFA overhead is coming from

template<typename Func>
double benchmark(Func&& f, int iterations = 100000) {
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        f();
    }
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / (double)iterations;
}

int main() {
    std::cout << "╔═══════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║    Diagnosing BitNFA Overhead                                        ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;

    std::string input = "Huckleberry";
    volatile bool result = false;

    // Parse pattern
    using tmp = typename ctll::parser<ctre::pcre, "Tom|Sawyer|Huckleberry|Finn", ctre::pcre_actions>::template output<ctre::pcre_context<>>;
    using AST = decltype(ctll::front(typename tmp::output_type::stack_type()));

    std::cout << "Pattern: \"Tom|Sawyer|Huckleberry|Finn\"" << std::endl;
    std::cout << "Input: \"" << input << "\"" << std::endl;
    std::cout << std::endl;

    // Test 1: Just literal extraction and matching (no BitNFA wrapper)
    std::cout << "Test 1: Pure literal matching (no wrappers)" << std::endl;

    constexpr auto literals = ctre::bitnfa::get_literals_if_applicable<AST>();

    auto t1 = benchmark([&]() {
        result = (ctre::bitnfa::match_literal_alternation(input, literals, nullptr) >= 0);
    });

    std::cout << "  Time: " << std::fixed << std::setprecision(2) << t1 << " ns" << std::endl;
    std::cout << std::endl;

    // Test 2: Just memcmp in a loop (absolute minimum)
    std::cout << "Test 2: Raw memcmp loop (theoretical minimum)" << std::endl;

    auto t2 = benchmark([&]() {
        result = (input == "Tom" || input == "Sawyer" || input == "Huckleberry" || input == "Finn");
    });

    std::cout << "  Time: " << t2 << " ns" << std::endl;
    std::cout << std::endl;

    // Test 3: BitNFA wrapper
    std::cout << "Test 3: Full BitNFA (with wrapper)" << std::endl;

    auto t3 = benchmark([&]() {
        result = ctre::bitnfa::match<"Tom|Sawyer|Huckleberry|Finn">(input).matched;
    });

    std::cout << "  Time: " << t3 << " ns" << std::endl;
    std::cout << std::endl;

    // Test 4: Standard CTRE
    std::cout << "Test 4: Standard CTRE (Glushkov NFA)" << std::endl;

    auto t4 = benchmark([&]() {
        result = (bool)ctre::match<"Tom|Sawyer|Huckleberry|Finn">(input);
    });

    std::cout << "  Time: " << t4 << " ns" << std::endl;
    std::cout << std::endl;

    // Analysis
    std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << " ANALYSIS" << std::endl;
    std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << std::endl;

    std::cout << "Overhead breakdown:" << std::endl;
    std::cout << "  Raw memcmp: " << t2 << " ns (minimum possible)" << std::endl;
    std::cout << "  Our literal matching: " << t1 << " ns (overhead: +" << (t1 - t2) << " ns)" << std::endl;
    std::cout << "  BitNFA wrapper: " << t3 << " ns (overhead: +" << (t3 - t1) << " ns)" << std::endl;
    std::cout << "  Standard CTRE: " << t4 << " ns" << std::endl;
    std::cout << std::endl;

    if (t3 < t4) {
        std::cout << "✅ BitNFA IS faster: " << (t4 / t3) << "x speedup!" << std::endl;
    } else {
        std::cout << "⚠️  BitNFA is slower: " << (t3 / t4) << "x" << std::endl;
        std::cout << "   Overhead from wrapper: " << (t3 - t1) << " ns" << std::endl;
    }

    std::cout << std::endl;
    std::cout << "Where is the overhead?" << std::endl;
    if ((t1 - t2) > 2.0) {
        std::cout << "  • Our literal matching adds: " << (t1 - t2) << " ns overhead ⚠️" << std::endl;
    }
    if ((t3 - t1) > 10.0) {
        std::cout << "  • BitNFA wrapper adds: " << (t3 - t1) << " ns overhead ⚠️⚠️" << std::endl;
    }
    std::cout << std::endl;

    return 0;
}
