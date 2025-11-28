#include <ctre.hpp>
#include <ctre/bitnfa/integration.hpp>
#include <iostream>
#include <chrono>
#include <string>
#include <iomanip>

// Comprehensive test to find optimal BitNFA thresholds

template<ctll::fixed_string Pattern>
double benchmark_ctre(const std::string& input, int iterations = 50000) {
    auto start = std::chrono::high_resolution_clock::now();
    volatile bool result = false;
    for (int i = 0; i < iterations; ++i) {
        result = (bool)ctre::match<Pattern>(input);
    }
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / (double)iterations;
}

template<ctll::fixed_string Pattern>
double benchmark_bitnfa(const std::string& input, int iterations = 50000) {
    auto start = std::chrono::high_resolution_clock::now();
    volatile bool result = false;
    for (int i = 0; i < iterations; ++i) {
        result = ctre::bitnfa::match<Pattern>(input).matched;
    }
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / (double)iterations;
}

template<ctll::fixed_string Pattern>
void test_pattern(const char* name, const std::string& input_template) {
    std::cout << "Pattern: " << name << std::endl;
    std::cout << "  Size │ CTRE (ns) │ BitNFA (ns) │ Ratio │ Winner" << std::endl;
    std::cout << "───────┼───────────┼─────────────┼───────┼────────" << std::endl;
    
    for (size_t size : {10, 15, 20, 30, 50, 100, 200, 500}) {
        std::string input = input_template;
        input.resize(size, 'x');
        
        double ctre_time = benchmark_ctre<Pattern>(input);
        double bitnfa_time = benchmark_bitnfa<Pattern>(input);
        double ratio = bitnfa_time / ctre_time;
        
        std::cout << std::setw(5) << size << "  │ "
                  << std::setw(9) << std::fixed << std::setprecision(2) << ctre_time << " │ "
                  << std::setw(11) << bitnfa_time << " │ "
                  << std::setw(5) << std::setprecision(3) << ratio << " │ ";
        
        if (ratio < 1.0) {
            std::cout << "✅ BitNFA (" << std::setprecision(1) << (1.0 - ratio) * 100 << "% faster)";
        } else if (ratio < 1.05) {
            std::cout << "⚠️ Tie";
        } else {
            std::cout << "❌ CTRE (" << std::setprecision(1) << (ratio - 1.0) * 100 << "% faster)";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

int main() {
    std::cout << "╔═══════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║    Finding Optimal BitNFA Thresholds                                 ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;
    
    // Test alternation patterns
    std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << " ALTERNATION PATTERNS (BitNFA should help!)" << std::endl;
    std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << std::endl;
    
    test_pattern<"Tom|Sawyer|Huckleberry|Finn">("alternation_4", "Huckleberry");
    test_pattern<"Huck[a-zA-Z]+|Saw[a-zA-Z]+">("complex_alt", "Huckleberry");
    test_pattern<"([A-Za-z]awyer|[A-Za-z]inn)\\s">("group_alt", "Sawyer ");
    
    // Test non-alternation patterns (BitNFA should NOT help)
    std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << " NON-ALTERNATION PATTERNS (BitNFA should NOT help)" << std::endl;
    std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << std::endl;
    
    test_pattern<"[a-zA-Z]+ing">("suffix_ing", "running");
    test_pattern<"[a-q][^u-z]{13}x">("negated_class", "abcdefghijklmnx");
    
    std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << " RECOMMENDATIONS" << std::endl;
    std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << std::endl;
    std::cout << "Based on these results, we can set smart thresholds:" << std::endl;
    std::cout << std::endl;
    std::cout << "  IF pattern has alternations (A|B|C):" << std::endl;
    std::cout << "    IF alternation_count >= 3 AND input_size >= threshold:" << std::endl;
    std::cout << "      → Use BitNFA" << std::endl;
    std::cout << "    ELSE:" << std::endl;
    std::cout << "      → Use Glushkov NFA" << std::endl;
    std::cout << "  ELSE:" << std::endl;
    std::cout << "    → Use SIMD (for repetitions) or Glushkov NFA" << std::endl;
    std::cout << std::endl;
    std::cout << "The threshold appears to be around 15-20 bytes for alternations!" << std::endl;
    std::cout << std::endl;
    
    return 0;
}

