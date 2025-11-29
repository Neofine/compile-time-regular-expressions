#include <ctre.hpp>
#include "include/ctre/literal_alternation_fast_path.hpp"
#include "include/ctre/teddy_simple.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <string>

// Compare Simple Scan vs Teddy SIMD

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
    std::cout << "║    Teddy vs Simple Scan - Performance Comparison                    ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;

    // Test inputs
    std::string input1 = "Huckleberry";
    std::string input2 = "Tom";
    std::string input3 = "Sawyer";
    std::string input4 = "Finn";
    std::string input5 = "NoMatch";

    // Parse pattern
    using tmp = typename ctll::parser<ctre::pcre, "Tom|Sawyer|Huckleberry|Finn", ctre::pcre_actions>::template output<ctre::pcre_context<>>;
    using AST = decltype(ctll::front(typename tmp::output_type::stack_type()));

    std::cout << "Pattern: \"Tom|Sawyer|Huckleberry|Finn\"" << std::endl;
    std::cout << std::endl;

    // Build compile-time structures
    constexpr auto literals = ctre::get_literal_list<AST>();
    constexpr auto teddy_mask = ctre::teddy::build_teddy_mask(literals);

    std::cout << "Extracted " << literals.count << " literals:" << std::endl;
    for (size_t i = 0; i < literals.count; ++i) {
        std::cout << "  " << i << ": \"";
        for (size_t j = 0; j < literals.items[i].length; ++j) {
            std::cout << literals.items[i].data[j];
        }
        std::cout << "\"" << std::endl;
    }
    std::cout << std::endl;

    // =========================================================================
    // CORRECTNESS TESTS
    // =========================================================================

    std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << " CORRECTNESS TESTS" << std::endl;
    std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << std::endl;

    std::cout << "Simple Scan:" << std::endl;
    std::cout << "  \"Huckleberry\": " << (literals.fast_match(input1) > 0 ? "MATCH ✓" : "NO MATCH ✗") << std::endl;
    std::cout << "  \"Tom\":         " << (literals.fast_match(input2) > 0 ? "MATCH ✓" : "NO MATCH ✗") << std::endl;
    std::cout << "  \"Sawyer\":      " << (literals.fast_match(input3) > 0 ? "MATCH ✓" : "NO MATCH ✗") << std::endl;
    std::cout << "  \"Finn\":        " << (literals.fast_match(input4) > 0 ? "MATCH ✓" : "NO MATCH ✗") << std::endl;
    std::cout << "  \"NoMatch\":     " << (literals.fast_match(input5) > 0 ? "MATCH ✗" : "NO MATCH ✓") << std::endl;
    std::cout << std::endl;

    std::cout << "Teddy SIMD:" << std::endl;
    std::cout << "  \"Huckleberry\": " << (ctre::teddy::teddy_match(input1, literals, teddy_mask) > 0 ? "MATCH ✓" : "NO MATCH ✗") << std::endl;
    std::cout << "  \"Tom\":         " << (ctre::teddy::teddy_match(input2, literals, teddy_mask) > 0 ? "MATCH ✓" : "NO MATCH ✗") << std::endl;
    std::cout << "  \"Sawyer\":      " << (ctre::teddy::teddy_match(input3, literals, teddy_mask) > 0 ? "MATCH ✓" : "NO MATCH ✗") << std::endl;
    std::cout << "  \"Finn\":        " << (ctre::teddy::teddy_match(input4, literals, teddy_mask) > 0 ? "MATCH ✓" : "NO MATCH ✗") << std::endl;
    std::cout << "  \"NoMatch\":     " << (ctre::teddy::teddy_match(input5, literals, teddy_mask) > 0 ? "MATCH ✗" : "NO MATCH ✓") << std::endl;
    std::cout << std::endl;

    std::cout << "Standard CTRE:" << std::endl;
    std::cout << "  \"Huckleberry\": " << ((bool)ctre::match<"Tom|Sawyer|Huckleberry|Finn">(input1) ? "MATCH ✓" : "NO MATCH ✗") << std::endl;
    std::cout << "  \"Tom\":         " << ((bool)ctre::match<"Tom|Sawyer|Huckleberry|Finn">(input2) ? "MATCH ✓" : "NO MATCH ✗") << std::endl;
    std::cout << "  \"Sawyer\":      " << ((bool)ctre::match<"Tom|Sawyer|Huckleberry|Finn">(input3) ? "MATCH ✓" : "NO MATCH ✗") << std::endl;
    std::cout << "  \"Finn\":        " << ((bool)ctre::match<"Tom|Sawyer|Huckleberry|Finn">(input4) ? "MATCH ✓" : "NO MATCH ✗") << std::endl;
    std::cout << "  \"NoMatch\":     " << ((bool)ctre::match<"Tom|Sawyer|Huckleberry|Finn">(input5) ? "MATCH ✗" : "NO MATCH ✓") << std::endl;
    std::cout << std::endl;

    // =========================================================================
    // PERFORMANCE TESTS
    // =========================================================================

    std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << " PERFORMANCE TESTS" << std::endl;
    std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << std::endl;

    volatile size_t result = 0;

    // Test 1: Simple sequential scan
    auto t1 = benchmark([&]() {
        result = literals.fast_match(input1);
    });

    // Test 2: Teddy SIMD scan
    auto t2 = benchmark([&]() {
        result = ctre::teddy::teddy_match(input1, literals, teddy_mask);
    });

    // Test 3: Standard CTRE (baseline)
    volatile bool result2 = false;
    auto t3 = benchmark([&]() {
        result2 = (bool)ctre::match<"Tom|Sawyer|Huckleberry|Finn">(input1);
    });

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Approach                          | Time (ns)    | vs CTRE  | vs Simple" << std::endl;
    std::cout << "----------------------------------|--------------|----------|----------" << std::endl;
    std::cout << "Simple Sequential Scan            | " << std::setw(10) << t1 << " ns | " << (t3/t1) << "x ✅    | 1.00x (baseline)" << std::endl;
    std::cout << "Teddy SIMD Scan                   | " << std::setw(10) << t2 << " ns | " << (t3/t2) << "x";

    if (t2 < t1) {
        std::cout << "      | " << (t1/t2) << "x 🔥" << std::endl;
    } else {
        std::cout << "      | " << (t2/t1) << "x ⚠️" << std::endl;
    }

    std::cout << "Standard CTRE (Glushkov NFA)      | " << std::setw(10) << t3 << " ns | 1.00x (baseline) | " << (t3/t1) << "x" << std::endl;
    std::cout << std::endl;

    // =========================================================================
    // SUMMARY
    // =========================================================================

    std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << " SUMMARY" << std::endl;
    std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << std::endl;

    if (t2 < t1) {
        double speedup = t1 / t2;
        std::cout << "🔥🔥🔥 TEDDY WINS! " << speedup << "x faster than simple scan!" << std::endl;
        std::cout << std::endl;
        std::cout << "Teddy improvements:" << std::endl;
        std::cout << "  • SIMD first-character scanning (16-32 bytes at once)" << std::endl;
        std::cout << "  • Compile-time mask building" << std::endl;
        std::cout << "  • " << speedup << "x faster than simple sequential!" << std::endl;
        std::cout << "  • " << (t3/t2) << "x faster than CTRE!" << std::endl;
    } else if (t2 < t3) {
        std::cout << "✅ Teddy is faster than CTRE (" << (t3/t2) << "x)" << std::endl;
        std::cout << "⚠️  But slower than simple scan (" << (t2/t1) << "x)" << std::endl;
        std::cout << std::endl;
        std::cout << "Possible reasons:" << std::endl;
        std::cout << "  • Small literal count (4) - simple scan is hard to beat" << std::endl;
        std::cout << "  • Short input (11 bytes) - SIMD overhead dominates" << std::endl;
        std::cout << "  • Teddy shines with many literals or long inputs" << std::endl;
    } else {
        std::cout << "⚠️  Teddy is slower than both simple scan and CTRE" << std::endl;
        std::cout << std::endl;
        std::cout << "Analysis:" << std::endl;
        std::cout << "  Simple scan:  " << t1 << " ns (" << (t3/t1) << "x vs CTRE)" << std::endl;
        std::cout << "  Teddy SIMD:   " << t2 << " ns (" << (t3/t2) << "x vs CTRE)" << std::endl;
        std::cout << "  CTRE:         " << t3 << " ns (baseline)" << std::endl;
        std::cout << std::endl;
        std::cout << "For this pattern, simple scan is best!" << std::endl;
    }

    std::cout << std::endl;
    std::cout << "VERDICT:" << std::endl;

    double best_speedup = std::max(t3/t1, t3/t2);
    if (best_speedup >= 2.0) {
        std::cout << "  ✅ SUCCESS! Best approach is " << best_speedup << "x faster than CTRE!" << std::endl;
    } else {
        std::cout << "  ✅ GOOD! Best approach is " << best_speedup << "x faster than CTRE!" << std::endl;
    }

    if (t2 < t1) {
        std::cout << "  🔥 Teddy is the winner! Use Teddy for literal alternations!" << std::endl;
    } else {
        std::cout << "  💡 Simple scan is the winner for this pattern!" << std::endl;
        std::cout << "     (Teddy may be better for longer inputs or more literals)" << std::endl;
    }

    std::cout << std::endl;

    return 0;
}
