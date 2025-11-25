#include <ctre.hpp>
#include <iostream>
#include <chrono>
#include <string>
#include <vector>
#include <cstring>

// Benchmark a pattern against text
template <auto Pattern>
double benchmark(const std::string& text, size_t iterations = 10000) {
    auto start = std::chrono::high_resolution_clock::now();

    volatile size_t match_count = 0;
    for (size_t i = 0; i < iterations; ++i) {
        if (auto result = Pattern(text)) {
            match_count = match_count + 1;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    return duration.count() / double(iterations);
}

int main() {
    std::cout << "=== PERFORMANCE AUDIT BENCHMARK ===\n\n";
    std::cout << "Pattern                          | Time (ns) | Match\n";
    std::cout << "----------------------------------------------------------------\n";

    // Hyperscan paper pattern
    {
        constexpr auto pat = ctre::search<"(abc|def).*ghi">;
        double time1 = benchmark<pat>("prefix abc something ghi suffix");
        double time2 = benchmark<pat>("prefix def something ghi suffix");
        std::cout << "(abc|def).*ghi (abc)            | " << time1 << " | MATCH\n";
        std::cout << "(abc|def).*ghi (def)            | " << time2 << " | MATCH\n";
    }

    // Dominant path
    {
        constexpr auto pat = ctre::search<"test.*hello.*world.*test">;
        double time = benchmark<pat>("test hello world test");
        std::cout << "test.*hello.*world.*test        | " << time << " | MATCH\n";
    }

    // Alternation with suffix
    {
        constexpr auto pat = ctre::search<"(foo|bar)suffix">;
        double time1 = benchmark<pat>("foo suffix");  // Space to avoid match
        double time2 = benchmark<pat>("foosuffix");
        double time3 = benchmark<pat>("barsuffix");
        std::cout << "(foo|bar)suffix (no match)      | " << time1 << " | NO MATCH\n";
        std::cout << "(foo|bar)suffix (foo)           | " << time2 << " | MATCH\n";
        std::cout << "(foo|bar)suffix (bar)           | " << time3 << " | MATCH\n";
    }

    // Character class expansion
    {
        constexpr auto pat = ctre::search<"[0-3]test">;
        double time1 = benchmark<pat>("0test");
        double time2 = benchmark<pat>("3test");
        double time3 = benchmark<pat>("9test");
        std::cout << "[0-3]test (0)                   | " << time1 << " | MATCH\n";
        std::cout << "[0-3]test (3)                   | " << time2 << " | MATCH\n";
        std::cout << "[0-3]test (9)                   | " << time3 << " | NO MATCH\n";
    }

    // Stress test: long literal
    {
        constexpr auto pat = ctre::search<"verylongliteralstring">;
        std::string text = std::string(1000, 'x') + "verylongliteralstring" + std::string(1000, 'y');
        double time = benchmark<pat>(text, 1000);  // Fewer iterations for long text
        std::cout << "verylongliteralstring (1000+)   | " << time << " | MATCH\n";
    }

    // Many alternations
    {
        constexpr auto pat = ctre::search<"(option1|option2|option3|option4|option5|option6)">;
        double time = benchmark<pat>("option5");
        std::cout << "(opt1|...|opt6) (opt5)          | " << time << " | MATCH\n";
    }

    // Test decomposition vs no decomposition
    std::cout << "\n=== DECOMPOSITION EFFECTIVENESS ===\n\n";

    // Pattern with good literal
    {
        constexpr auto pat = ctre::search<"prefix.*middlestuff.*suffix">;
        std::string text = "prefix some data middlestuff more data suffix";
        double time = benchmark<pat>(text);
        std::cout << "prefix.*middle.*suffix          | " << time << " | Should use decomposition\n";
    }

    // Pattern with short literal (< 4 chars)
    {
        constexpr auto pat = ctre::search<"a.*b.*c">;
        std::string text = "a something b something c";
        double time = benchmark<pat>(text);
        std::cout << "a.*b.*c (short literals)        | " << time << " | Should NOT decompose\n";
    }

    // Pattern with leading .*
    {
        constexpr auto pat = ctre::search<".*(hello|world).*test">;
        std::string text = "anything hello anything test";
        double time = benchmark<pat>(text);
        std::cout << ".*(hello|world).*test           | " << time << " | Should NOT decompose\n";
    }

    std::cout << "\n=== GLUSHKOV NFA VERIFICATION ===\n\n";

    // Verify NFA properties
    {
        using Pattern = decltype(ctre::search<"(abc|def).*ghi">);
        using AST = ctre::decomposition::unwrap_regex_t<Pattern>;
        constexpr auto nfa = ctre::glushkov::glushkov_nfa<AST>();

        std::cout << "Pattern: (abc|def).*ghi\n";
        std::cout << "  State count: " << nfa.state_count << " (paper says 10)\n";
        std::cout << "  Accept count: " << nfa.accept_count << "\n";

        // Check state properties
        for (size_t i = 0; i < nfa.state_count; ++i) {
            std::cout << "  State " << i << ": symbol='" << nfa.states[i].symbol
                      << "' successors=" << nfa.states[i].successor_count;
            if (nfa.states[i].successor_count > 0) {
                std::cout << " [";
                for (size_t j = 0; j < nfa.states[i].successor_count; ++j) {
                    std::cout << nfa.states[i].successors[j];
                    if (j + 1 < nfa.states[i].successor_count) std::cout << ", ";
                }
                std::cout << "]";
            }
            std::cout << "\n";
        }
    }

    std::cout << "\n=== BENCHMARK COMPLETE ===\n";

    return 0;
}
