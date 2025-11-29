#include <ctre.hpp>
#include <ctre/smart_dispatch.hpp>
#include <iostream>
#include <chrono>

template<typename Func>
double bench(Func&& f, int iters = 100000) {
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iters; ++i) { f(); }
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / (double)iters;
}

int main() {
    std::string alt_input = "Huckleberry";
    std::string rep_input(64, 'a');
    volatile bool result;
    
    std::cout << "Smart Dispatch Performance Test\n" << std::endl;
    
    // Test 1: Alternation pattern (should use BitNFA)
    std::cout << "Pattern: Huck[a-zA-Z]+|Saw[a-zA-Z]+ (alternation)" << std::endl;
    
    auto t1_ctre = bench([&]() {
        result = (bool)ctre::match<"Huck[a-zA-Z]+|Saw[a-zA-Z]+">(alt_input);
    });
    
    auto t1_smart = bench([&]() {
        result = (bool)ctre::smart_dispatch::match<"Huck[a-zA-Z]+|Saw[a-zA-Z]+">(alt_input);
    });
    
    std::cout << "  Base CTRE:      " << t1_ctre << " ns" << std::endl;
    std::cout << "  Smart dispatch: " << t1_smart << " ns" << std::endl;
    std::cout << "  Change:         " << (t1_smart/t1_ctre) << "x" << std::endl;
    std::cout << std::endl;
    
    // Test 2: Repetition pattern (should use base CTRE + SIMD)
    std::cout << "Pattern: [a-z]+ (repetition)" << std::endl;
    
    auto t2_ctre = bench([&]() {
        result = (bool)ctre::match<"[a-z]+">(rep_input);
    });
    
    auto t2_smart = bench([&]() {
        result = (bool)ctre::smart_dispatch::match<"[a-z]+">(rep_input);
    });
    
    std::cout << "  Base CTRE:      " << t2_ctre << " ns" << std::endl;
    std::cout << "  Smart dispatch: " << t2_smart << " ns" << std::endl;
    std::cout << "  Change:         " << (t2_smart/t2_ctre) << "x" << std::endl;
    std::cout << std::endl;
    
    // Summary
    bool no_regression = (t1_smart / t1_ctre < 1.1) && (t2_smart / t2_ctre < 1.1);
    
    if (no_regression) {
        std::cout << "✅ Smart dispatch has no significant regressions!" << std::endl;
    } else {
        std::cout << "⚠️  Smart dispatch may have regressions" << std::endl;
    }
    
    return no_regression ? 0 : 1;
}
