#include <ctre.hpp>
#include "include/ctre/literal_alternation_fast_path.hpp"
#include "include/ctre/teddy_complete.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>

// Profile every component to find the overhead

template<typename Func>
double benchmark(Func&& f, int iterations = 1000000) {
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        f();
    }
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / (double)iterations;
}

int main() {
    std::cout << "╔═══════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║    Profiling Complete Teddy Overhead - Finding Inefficiencies        ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;

    std::string input = "Tom";  // 3 bytes - the problematic case

    using tmp = typename ctll::parser<ctre::pcre, "Tom|Sawyer|Huckleberry|Finn", ctre::pcre_actions>::template output<ctre::pcre_context<>>;
    using AST = decltype(ctll::front(typename tmp::output_type::stack_type()));
    constexpr auto literals = ctre::get_literal_list<AST>();

    volatile bool result = false;

    std::cout << "Testing with input: \"" << input << "\" (3 bytes)" << std::endl;
    std::cout << "Literal count: " << literals.count << std::endl;
    std::cout << std::endl;

    // =========================================================================
    // BASELINE: What's the absolute minimum?
    // =========================================================================

    std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << " BASELINE MEASUREMENTS" << std::endl;
    std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << std::endl;

    // Test 1: Direct string comparison (absolute minimum)
    auto t_direct = benchmark([&]() {
        result = (input == "Tom" || input == "Sawyer" || input == "Huckleberry" || input == "Finn");
    });
    std::cout << "1. Direct string comparison:        " << std::fixed << std::setprecision(3) << t_direct << " ns (absolute minimum)" << std::endl;

    // Test 2: CTRE (current baseline)
    auto t_ctre = benchmark([&]() {
        result = (bool)ctre::match<"Tom|Sawyer|Huckleberry|Finn">(input);
    });
    std::cout << "2. CTRE match:                      " << t_ctre << " ns (current baseline)" << std::endl;

    // Test 3: Simple literal scan (no overhead)
    auto t_simple = benchmark([&]() {
        result = (literals.fast_match(input) > 0);
    });
    std::cout << "3. Simple literal scan:             " << t_simple << " ns" << std::endl;

    std::cout << std::endl;

    // =========================================================================
    // COMPLETE TEDDY: Break down the overhead
    // =========================================================================

    std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << " COMPLETE TEDDY OVERHEAD BREAKDOWN" << std::endl;
    std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << std::endl;

    // Test 4: Complete Teddy search (full implementation)
    auto t_complete_search = benchmark([&]() {
        size_t len = 0;
        const char* pos = ctre::teddy_complete::search(input, literals, &len);
        result = (pos != nullptr);
    });
    std::cout << "4. Complete Teddy search:           " << t_complete_search << " ns" << std::endl;

    // Test 5: Complete Teddy match
    auto t_complete_match = benchmark([&]() {
        result = (ctre::teddy_complete::match(input, literals) > 0);
    });
    std::cout << "5. Complete Teddy match:            " << t_complete_match << " ns" << std::endl;

    std::cout << std::endl;

    // Test 6: Slim Teddy directly (2-4 literals path)
    volatile size_t result2 = 0;
    auto t_slim_24 = benchmark([&]() {
        size_t len = 0;
        const char* pos = ctre::teddy_complete::slim_teddy_2_4_literals(
            input.data(), input.data() + input.size(), literals, &len);
        result = (pos != nullptr);
    });
    std::cout << "6. Slim Teddy (2-4 literals) direct:" << t_slim_24 << " ns" << std::endl;

    std::cout << std::endl;

    // =========================================================================
    // OVERHEAD ANALYSIS
    // =========================================================================

    std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << " OVERHEAD ANALYSIS" << std::endl;
    std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << std::endl;

    std::cout << "Direct comparison: " << t_direct << " ns (minimum possible)" << std::endl;
    std::cout << "CTRE:              " << t_ctre << " ns (+" << (t_ctre - t_direct) << " ns overhead)" << std::endl;
    std::cout << "Simple scan:       " << t_simple << " ns (+" << (t_simple - t_direct) << " ns overhead)" << std::endl;
    std::cout << "Complete match:    " << t_complete_match << " ns (+" << (t_complete_match - t_direct) << " ns overhead)" << std::endl;
    std::cout << "Complete search:   " << t_complete_search << " ns (+" << (t_complete_search - t_direct) << " ns overhead)" << std::endl;
    std::cout << "Slim 2-4 direct:   " << t_slim_24 << " ns (+" << (t_slim_24 - t_direct) << " ns overhead)" << std::endl;
    std::cout << std::endl;

    std::cout << "BREAKDOWN:" << std::endl;
    std::cout << "----------" << std::endl;
    std::cout << "Dispatch overhead:         " << (t_complete_match - t_slim_24) << " ns" << std::endl;
    std::cout << "Slim Teddy overhead:       " << (t_slim_24 - t_simple) << " ns" << std::endl;
    std::cout << "Simple scan overhead:      " << (t_simple - t_direct) << " ns" << std::endl;
    std::cout << std::endl;

    // =========================================================================
    // DIAGNOSIS
    // =========================================================================

    std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << " DIAGNOSIS" << std::endl;
    std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << std::endl;

    if (t_complete_match > t_ctre) {
        double slowdown = t_complete_match / t_ctre;
        std::cout << "❌ Complete Teddy is " << slowdown << "x SLOWER than CTRE!" << std::endl;
        std::cout << "   Extra overhead: +" << (t_complete_match - t_ctre) << " ns" << std::endl;
        std::cout << std::endl;

        // Find the culprit
        if ((t_complete_match - t_slim_24) > 0.2) {
            std::cout << "🔍 MAIN CULPRIT: Dispatch overhead (+" << (t_complete_match - t_slim_24) << " ns)" << std::endl;
            std::cout << "   The runtime dispatch is adding significant overhead!" << std::endl;
        }

        if ((t_slim_24 - t_simple) > 0.2) {
            std::cout << "🔍 SECONDARY ISSUE: Slim Teddy overhead (+" << (t_slim_24 - t_simple) << " ns)" << std::endl;
            std::cout << "   Slim Teddy is slower than simple scan!" << std::endl;
        }

        if ((t_simple - t_direct) > 0.2) {
            std::cout << "🔍 BASE OVERHEAD: Literal list overhead (+" << (t_simple - t_direct) << " ns)" << std::endl;
        }
    }

    std::cout << std::endl;
    std::cout << "RECOMMENDATION:" << std::endl;
    std::cout << "---------------" << std::endl;

    // Find the best approach
    double best = std::min({t_direct, t_ctre, t_simple, t_complete_match});
    if (best == t_direct) {
        std::cout << "✅ Use direct string comparison (or equivalent)" << std::endl;
    } else if (best == t_ctre) {
        std::cout << "✅ Use CTRE (already optimal!)" << std::endl;
    } else if (best == t_simple) {
        std::cout << "✅ Use simple literal scan" << std::endl;
    } else {
        std::cout << "✅ Use Complete Teddy" << std::endl;
    }

    std::cout << std::endl;

    return 0;
}
