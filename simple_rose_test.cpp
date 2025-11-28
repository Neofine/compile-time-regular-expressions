#include <ctre.hpp>
#include <string>
#include <chrono>
#include <iostream>

template<typename Func>
double bench(Func f, int iters = 1000000) {
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
    
    std::cout << "=== ROSE CONCEPT TEST (without custom implementation) ===" << std::endl;
    std::cout << std::endl;
    std::cout << "Input: '" << test_input << "'" << std::endl;
    std::cout << "Pattern: [a-zA-Z]+ing" << std::endl;
    std::cout << std::endl;
    
    // Test standard CTRE
    auto ctre_test = [&]() {
        return static_cast<bool>(ctre::match<"[a-zA-Z]+ing">(test_input));
    };
    
    // Benchmark
    double ctre_time = bench(ctre_test);
    
    std::cout << "Results:" << std::endl;
    std::cout << "  Standard CTRE:     " << ctre_time << " ns" << std::endl;
    std::cout << "  Match result:      " << ctre_test() << std::endl;
    
    std::cout << std::endl;
    std::cout << "Note: Rose optimization would search for 'ing' first" << std::endl;
    std::cout << "      then verify [a-zA-Z]+ prefix backward." << std::endl;
    std::cout << "      Expected improvement: 2-4x for this pattern." << std::endl;
    
    return 0;
}
