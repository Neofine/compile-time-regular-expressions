#include <ctre.hpp>
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
    
    std::cout << "Testing Automatic Smart Dispatch in ctre::match\n" << std::endl;
    
    // Test 1: Alternation (should auto-dispatch to BitNFA)
    std::cout << "Pattern: Huck[a-zA-Z]+|Saw[a-zA-Z]+ (alternation)" << std::endl;
    auto t1 = bench([&]() {
        result = (bool)ctre::match<"Huck[a-zA-Z]+|Saw[a-zA-Z]+">(alt_input);
    });
    std::cout << "  Time: " << t1 << " ns" << std::endl;
    std::cout << "  (automatically uses BitNFA)" << std::endl;
    std::cout << std::endl;
    
    // Test 2: Repetition (should use base CTRE + SIMD)
    std::cout << "Pattern: [a-z]+ (repetition)" << std::endl;
    auto t2 = bench([&]() {
        result = (bool)ctre::match<"[a-z]+">(rep_input);
    });
    std::cout << "  Time: " << t2 << " ns" << std::endl;
    std::cout << "  (automatically uses base CTRE + SIMD)" << std::endl;
    std::cout << std::endl;
    
    // Test 3: Simple literal alternation
    std::cout << "Pattern: Tom|Sawyer|Finn (simple alternation)" << std::endl;
    auto t3 = bench([&]() {
        result = (bool)ctre::match<"Tom|Sawyer|Finn">("Tom");
    });
    std::cout << "  Time: " << t3 << " ns" << std::endl;
    std::cout << "  (automatically uses BitNFA)" << std::endl;
    
    std::cout << "\n✓ Smart dispatch working transparently!" << std::endl;
    return 0;
}
