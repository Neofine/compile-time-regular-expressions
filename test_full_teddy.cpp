#include <ctre.hpp>
#include "include/ctre/literal_alternation_fast_path.hpp"
#include "include/ctre/teddy_simple.hpp"
#include "include/ctre/teddy_full.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <string>

// Test Full Teddy with pshufb shuffles

template<typename Func>
double benchmark(Func&& f, int iterations = 50000) {
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        f();
    }
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / (double)iterations;
}

int main() {
    std::cout << "╔═══════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                                                                       ║" << std::endl;
    std::cout << "║         🔥 FULL TEDDY with pshufb - ULTIMATE TEST 🔥                 ║" << std::endl;
    std::cout << "║                                                                       ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;

    // Parse pattern
    using tmp = typename ctll::parser<ctre::pcre, "Tom|Sawyer|Huckleberry|Finn", ctre::pcre_actions>::template output<ctre::pcre_context<>>;
    using AST = decltype(ctll::front(typename tmp::output_type::stack_type()));

    constexpr auto literals = ctre::get_literal_list<AST>();
    constexpr auto simple_mask = ctre::teddy::build_teddy_mask(literals);
    constexpr auto full_masks = ctre::teddy_full::build_full_teddy_masks(literals);

    std::cout << "Pattern: \"Tom|Sawyer|Huckleberry|Finn\"" << std::endl;
    std::cout << "Literals: " << literals.count << std::endl;
    std::cout << std::endl;

    // =========================================================================
    // CORRECTNESS TESTS
    // =========================================================================

    std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << " CORRECTNESS TESTS" << std::endl;
    std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << std::endl;

    std::vector<std::pair<std::string, bool>> test_cases = {
        {"Tom", true},
        {"Sawyer", true},
        {"Huckleberry", true},
        {"Finn", true},
        {"NoMatch", false},
        {"Tomato", false},
    };

    bool all_correct = true;
    for (const auto& [input, should_match] : test_cases) {
        bool simple_match = literals.fast_match(input) > 0;
        bool teddy_simple_match = ctre::teddy::teddy_match(input, literals, simple_mask) > 0;
        bool teddy_full_match = ctre::teddy_full::teddy_match(input, literals, full_masks) > 0;
        bool ctre_match = (bool)ctre::match<"Tom|Sawyer|Huckleberry|Finn">(input);

        bool all_agree = (simple_match == should_match) &&
                        (teddy_simple_match == should_match) &&
                        (teddy_full_match == should_match) &&
                        (ctre_match == should_match);

        std::cout << "  \"" << std::left << std::setw(12) << input << "\" : ";
        std::cout << (all_agree ? "✅ ALL CORRECT" : "❌ MISMATCH");
        std::cout << " (expect: " << (should_match ? "match" : "no match") << ")" << std::endl;

        if (!all_agree) {
            all_correct = false;
            std::cout << "    Simple: " << simple_match
                     << ", Teddy-Simple: " << teddy_simple_match
                     << ", Teddy-Full: " << teddy_full_match
                     << ", CTRE: " << ctre_match << std::endl;
        }
    }

    std::cout << std::endl;
    if (!all_correct) {
        std::cout << "❌ CORRECTNESS FAILED!" << std::endl;
        return 1;
    }

    // =========================================================================
    // PERFORMANCE TEST 1: Short Match (alternation_4 benchmark)
    // =========================================================================

    std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << " TEST 1: Short MATCH (11 bytes) - alternation_4 benchmark" << std::endl;
    std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << std::endl;

    std::string short_input = "Huckleberry";
    volatile size_t result = 0;
    volatile bool result2 = false;

    auto t_simple = benchmark([&]() {
        result = literals.fast_match(short_input);
    });

    auto t_teddy_simple = benchmark([&]() {
        result = ctre::teddy::teddy_match(short_input, literals, simple_mask);
    });

    auto t_teddy_full = benchmark([&]() {
        result = ctre::teddy_full::teddy_match(short_input, literals, full_masks);
    });

    auto t_ctre = benchmark([&]() {
        result2 = (bool)ctre::match<"Tom|Sawyer|Huckleberry|Finn">(short_input);
    });

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Approach                      | Time (ns)   | vs CTRE" << std::endl;
    std::cout << "------------------------------|-----------|-----------" << std::endl;
    std::cout << "Simple Sequential Scan        | " << std::setw(8) << t_simple << " ns | " << (t_ctre / t_simple) << "x";
    if (t_simple < t_ctre) std::cout << " ✅";
    std::cout << std::endl;

    std::cout << "Teddy Simple (direct SIMD)    | " << std::setw(8) << t_teddy_simple << " ns | " << (t_ctre / t_teddy_simple) << "x" << std::endl;

    std::cout << "Teddy Full (pshufb shuffle)   | " << std::setw(8) << t_teddy_full << " ns | " << (t_ctre / t_teddy_full) << "x";
    if (t_teddy_full < t_ctre) std::cout << " ✅";
    std::cout << std::endl;

    std::cout << "CTRE (Glushkov NFA, baseline) | " << std::setw(8) << t_ctre << " ns | 1.00x" << std::endl;
    std::cout << std::endl;

    double best_short = std::min({t_simple, t_teddy_simple, t_teddy_full});
    std::cout << "🏆 WINNER for short match: ";
    if (best_short == t_simple) {
        std::cout << "Simple Scan (" << (t_ctre/t_simple) << "x vs CTRE)" << std::endl;
    } else if (best_short == t_teddy_full) {
        std::cout << "Full Teddy (" << (t_ctre/t_teddy_full) << "x vs CTRE)" << std::endl;
    } else {
        std::cout << "Simple Teddy (" << (t_ctre/t_teddy_simple) << "x vs CTRE)" << std::endl;
    }
    std::cout << std::endl;

    // =========================================================================
    // PERFORMANCE TEST 2: Long Search (THE KILLER USE CASE!)
    // =========================================================================

    std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << " TEST 2: Long SEARCH (611 bytes) - Finding literals in text" << std::endl;
    std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << std::endl;

    std::string long_text = std::string(500, 'x') + "Huckleberry" + std::string(100, 'y');
    std::cout << "Searching for literal in " << long_text.size() << " byte text (literal at position 500)" << std::endl;
    std::cout << std::endl;

    volatile size_t search_result = 0;

    auto t_teddy_simple_search = benchmark([&]() {
        size_t len = 0;
        const char* pos = ctre::teddy::teddy_search(long_text, literals, simple_mask, &len);
        search_result = (pos != nullptr) ? (pos - long_text.data()) : 0;
    }, 10000);

    auto t_teddy_full_search = benchmark([&]() {
        size_t len = 0;
        const char* pos = ctre::teddy_full::teddy_search(long_text, literals, full_masks, &len);
        search_result = (pos != nullptr) ? (pos - long_text.data()) : 0;
    }, 10000);

    auto t_ctre_search = benchmark([&]() {
        auto result = ctre::search<"Tom|Sawyer|Huckleberry|Finn">(long_text);
        search_result = result ? result.to_view().data() - long_text.data() : 0;
    }, 10000);

    std::cout << "Approach                      | Time (ns)      | vs CTRE" << std::endl;
    std::cout << "------------------------------|--------------|-----------" << std::endl;
    std::cout << "Teddy Simple (direct SIMD)    | " << std::setw(11) << t_teddy_simple_search << " ns | " << (t_ctre_search / t_teddy_simple_search) << "x 🔥" << std::endl;
    std::cout << "Teddy Full (pshufb shuffle)   | " << std::setw(11) << t_teddy_full_search << " ns | " << (t_ctre_search / t_teddy_full_search) << "x 🔥";

    if (t_teddy_full_search < t_teddy_simple_search) {
        std::cout << " ✅ BEST!" << std::endl;
    } else {
        std::cout << std::endl;
    }

    std::cout << "CTRE Search (baseline)        | " << std::setw(11) << t_ctre_search << " ns | 1.00x" << std::endl;
    std::cout << std::endl;

    double best_search = std::min(t_teddy_simple_search, t_teddy_full_search);
    std::cout << "🏆 WINNER for search: ";
    if (best_search == t_teddy_full_search) {
        std::cout << "Full Teddy (" << (t_ctre_search/t_teddy_full_search) << "x vs CTRE) 🔥🔥🔥" << std::endl;
    } else {
        std::cout << "Simple Teddy (" << (t_ctre_search/t_teddy_simple_search) << "x vs CTRE) 🔥🔥🔥" << std::endl;
    }
    std::cout << std::endl;

    // =========================================================================
    // FINAL SUMMARY
    // =========================================================================

    std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << " FINAL VERDICT" << std::endl;
    std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << std::endl;

    std::cout << "PERFORMANCE COMPARISON:" << std::endl;
    std::cout << "----------------------" << std::endl;
    std::cout << std::endl;

    std::cout << "Short MATCH (11 bytes):" << std::endl;
    std::cout << "  Simple Scan:       " << t_simple << " ns (" << (t_ctre/t_simple) << "x)" << std::endl;
    std::cout << "  Teddy Simple:      " << t_teddy_simple << " ns (" << (t_ctre/t_teddy_simple) << "x)" << std::endl;
    std::cout << "  Teddy Full:        " << t_teddy_full << " ns (" << (t_ctre/t_teddy_full) << "x)" << std::endl;
    std::cout << "  CTRE:              " << t_ctre << " ns (1.00x)" << std::endl;
    std::cout << "  Winner: " << (best_short == t_simple ? "Simple" : (best_short == t_teddy_full ? "Full Teddy" : "Simple Teddy")) << std::endl;
    std::cout << std::endl;

    std::cout << "Long SEARCH (611 bytes):" << std::endl;
    std::cout << "  Teddy Simple:      " << t_teddy_simple_search << " ns (" << (t_ctre_search/t_teddy_simple_search) << "x) 🔥" << std::endl;
    std::cout << "  Teddy Full:        " << t_teddy_full_search << " ns (" << (t_ctre_search/t_teddy_full_search) << "x) 🔥" << std::endl;
    std::cout << "  CTRE:              " << t_ctre_search << " ns (1.00x)" << std::endl;
    std::cout << "  Winner: " << (best_search == t_teddy_full_search ? "Full Teddy" : "Simple Teddy") << std::endl;
    std::cout << std::endl;

    std::cout << "KEY FINDINGS:" << std::endl;
    std::cout << "-------------" << std::endl;
    std::cout << std::endl;

    if (t_teddy_full_search < t_teddy_simple_search) {
        double improvement = t_teddy_simple_search / t_teddy_full_search;
        std::cout << "🔥 Full Teddy (pshufb) is " << improvement << "x faster than Simple Teddy for SEARCH!" << std::endl;
        std::cout << "   The pshufb shuffle makes a HUGE difference!" << std::endl;
    } else {
        std::cout << "ℹ️  Simple Teddy is competitive with Full Teddy for this pattern." << std::endl;
        std::cout << "   Full Teddy may shine with more complex patterns." << std::endl;
    }

    std::cout << std::endl;
    std::cout << "RECOMMENDATION:" << std::endl;
    std::cout << "---------------" << std::endl;
    std::cout << std::endl;

    if (best_search == t_teddy_full_search && t_teddy_full_search < t_teddy_simple_search * 0.9) {
        std::cout << "✅ USE FULL TEDDY!" << std::endl;
        std::cout << "   The pshufb shuffle optimization is worth it!" << std::endl;
        std::cout << "   Full Teddy dominates for search operations!" << std::endl;
    } else {
        std::cout << "✅ BOTH ARE EXCELLENT!" << std::endl;
        std::cout << "   Simple Teddy and Full Teddy have similar performance." << std::endl;
        std::cout << "   Choose based on code complexity preference." << std::endl;
    }

    std::cout << std::endl;
    std::cout << "Overall: " << (t_ctre_search / best_search) << "x faster than CTRE for search! 🔥🔥🔥" << std::endl;
    std::cout << std::endl;

    return 0;
}
