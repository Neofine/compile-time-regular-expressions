#include <ctre.hpp>
#include "include/ctre/literal_alternation_fast_path.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>

// Test the direct literal optimization (no BitNFA wrapper!)

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
    std::cout << "║    Direct Literal Optimization Test (NO BitNFA wrapper!)            ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;

    std::string input = "Huckleberry";

    // Parse pattern at compile-time
    using tmp = typename ctll::parser<ctre::pcre, "Tom|Sawyer|Huckleberry|Finn", ctre::pcre_actions>::template output<ctre::pcre_context<>>;
    using AST = decltype(ctll::front(typename tmp::output_type::stack_type()));

    std::cout << "Pattern: \"Tom|Sawyer|Huckleberry|Finn\"" << std::endl;
    std::cout << "Input: \"" << input << "\"" << std::endl;
    std::cout << std::endl;

    // Check if it's a literal alternation
    std::cout << "Is literal alternation? " << (ctre::is_literal_alt<AST>::value ? "YES ✓" : "NO ✗") << std::endl;
    std::cout << std::endl;

    if constexpr (ctre::is_literal_alt<AST>::value) {
        constexpr auto literals = ctre::get_literal_list<AST>();

        std::cout << "Extracted " << literals.count << " literals:" << std::endl;
        for (size_t i = 0; i < literals.count; ++i) {
            std::cout << "  " << i << ": \"";
            for (size_t j = 0; j < literals.items[i].length; ++j) {
                std::cout << literals.items[i].data[j];
            }
            std::cout << "\" (length: " << literals.items[i].length << ")" << std::endl;
        }
        std::cout << std::endl;

        // Test correctness
        std::cout << "Correctness test:" << std::endl;
        size_t len = literals.fast_match("Tom");
        std::cout << "  \"Tom\": " << (len > 0 ? "MATCH ✓" : "NO MATCH ✗") << std::endl;

        len = literals.fast_match("Sawyer");
        std::cout << "  \"Sawyer\": " << (len > 0 ? "MATCH ✓" : "NO MATCH ✗") << std::endl;

        len = literals.fast_match("Huckleberry");
        std::cout << "  \"Huckleberry\": " << (len > 0 ? "MATCH ✓" : "NO MATCH ✗") << std::endl;

        len = literals.fast_match("Finn");
        std::cout << "  \"Finn\": " << (len > 0 ? "MATCH ✓" : "NO MATCH ✗") << std::endl;

        len = literals.fast_match("NoMatch");
        std::cout << "  \"NoMatch\": " << (len > 0 ? "MATCH ✗" : "NO MATCH ✓") << std::endl;
        std::cout << std::endl;

        // Performance test
        std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
        std::cout << " PERFORMANCE TEST" << std::endl;
        std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
        std::cout << std::endl;

        volatile size_t result = 0;

        // Test 1: Direct literal matching (compile-time literals)
        auto t1 = benchmark([&]() {
            result = literals.fast_match(input);
        });

        // Test 2: Standard CTRE (Glushkov NFA)
        volatile bool result2 = false;
        auto t2 = benchmark([&]() {
            result2 = (bool)ctre::match<"Tom|Sawyer|Huckleberry|Finn">(input);
        });

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Approach                          | Time (ns)    | vs CTRE" << std::endl;
        std::cout << "----------------------------------|--------------|---------" << std::endl;
        std::cout << "Direct literal matching (FAST!)   | " << std::setw(10) << t1 << " ns | " << (t2/t1) << "x ✅" << std::endl;
        std::cout << "Standard CTRE (Glushkov NFA)      | " << std::setw(10) << t2 << " ns | 1.00x (baseline)" << std::endl;
        std::cout << std::endl;

        if (t1 < t2) {
            double speedup = t2 / t1;
            std::cout << "🔥🔥🔥 SUCCESS! Direct literal matching is " << speedup << "x FASTER!" << std::endl;
            std::cout << std::endl;
            std::cout << "This is the optimization we want!" << std::endl;
            std::cout << "  • Zero BitNFA overhead" << std::endl;
            std::cout << "  • Direct memcmp loop" << std::endl;
            std::cout << "  • Compile-time literal extraction" << std::endl;
            std::cout << "  • " << speedup << "x faster than Glushkov NFA!" << std::endl;
        } else {
            std::cout << "⚠️  Still slower than CTRE (" << (t1/t2) << "x)" << std::endl;
        }

        std::cout << std::endl;
        std::cout << "Next step: Integrate this into smart_dispatch without wrapper overhead!" << std::endl;
    } else {
        std::cout << "❌ Pattern is NOT a literal alternation!" << std::endl;
    }

    std::cout << std::endl;

    return 0;
}
