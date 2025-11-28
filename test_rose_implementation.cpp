#include "include/ctre/simd_rose.hpp"
#include <ctre.hpp>
#include <string>
#include <chrono>
#include <iostream>

template<typename Func>
double bench(Func f, int iters = 1000000) {
    // Warmup
    for (int i = 0; i < 10000; ++i) f();
    
    double best = 1e9;
    for (int run = 0; run < 5; ++run) {
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iters; ++i) {
            volatile auto result = f();
        }
        auto end = std::chrono::high_resolution_clock::now();
        double time = std::chrono::duration<double, std::nano>(end - start).count() / iters;
        if (time < best) best = time;
    }
    return best;
}

int main() {
    std::string test_input = "fishingfishingfishing";
    
    std::cout << "=== ROSE OPTIMIZATION TEST ===" << std::endl;
    std::cout << std::endl;
    std::cout << "Input: '" << test_input << "'" << std::endl;
    std::cout << "Pattern: [a-zA-Z]+ing" << std::endl;
    std::cout << std::endl;
    
    // Test Rose implementation
    auto rose_test = [&]() {
        const char* begin = test_input.data();
        const char* end = begin + test_input.size();
        ctre::flags f{};
        auto result = ctre::simd::rose_alpha_suffix_ing(begin, end, f);
        return result != end;
    };
    
    // Test standard CTRE
    auto ctre_test = [&]() {
        return static_cast<bool>(ctre::match<"[a-zA-Z]+ing">(test_input));
    };
    
    // Benchmark
    double rose_time = bench(rose_test);
    double ctre_time = bench(ctre_test);
    
    std::cout << "Results:" << std::endl;
    std::cout << "  Rose optimization: " << rose_time << " ns" << std::endl;
    std::cout << "  Standard CTRE:     " << ctre_time << " ns" << std::endl;
    std::cout << "  Speedup:           " << (ctre_time / rose_time) << "x" << std::endl;
    
    std::cout << std::endl;
    std::cout << "Correctness check:" << std::endl;
    std::cout << "  Rose:  " << rose_test() << std::endl;
    std::cout << "  CTRE:  " << ctre_test() << std::endl;
    
    // Test edge cases
    std::cout << std::endl;
    std::cout << "Edge case tests:" << std::endl;
    
    auto test_string = [](const std::string& s) {
        const char* begin = s.data();
        const char* end = begin + s.size();
        ctre::flags f{};
        auto rose_result = ctre::simd::rose_alpha_suffix_ing(begin, end, f) != end;
        auto ctre_result = static_cast<bool>(ctre::match<"[a-zA-Z]+ing">(s));
        
        std::cout << "  '" << s << "': Rose=" << rose_result 
                  << ", CTRE=" << ctre_result 
                  << " " << (rose_result == ctre_result ? "✓" : "✗") << std::endl;
    };
    
    test_string("fishing");
    test_string("running");
    test_string("ing");           // Just "ing", no prefix
    test_string("123ing");        // Non-alpha prefix
    test_string("no match");
    test_string("walkingandtalking");
    
    return 0;
}
