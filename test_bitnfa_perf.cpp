#include <ctre.hpp>
#include <ctre/bitnfa/integration.hpp>
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
    std::string rep_input(64, 'a');
    std::string alt_input = "Huckleberry";
    volatile bool result;
    
    std::cout << "BitNFA Performance After Removing Delegation\n" << std::endl;
    
    // Test 1: Repetition (BitNFA should be SLOWER without SIMD)
    std::cout << "Pattern: [a-z]+ (64 bytes)" << std::endl;
    auto t1_ctre = bench([&]() { result = (bool)ctre::match<"[a-z]+">(rep_input); });
    auto t1_bitnfa = bench([&]() { result = ctre::bitnfa::match<"[a-z]+">(rep_input).matched; });
    std::cout << "  CTRE (with SIMD):  " << t1_ctre << " ns" << std::endl;
    std::cout << "  BitNFA (no SIMD):  " << t1_bitnfa << " ns" << std::endl;
    std::cout << "  Ratio: " << (t1_bitnfa/t1_ctre) << "x" << std::endl;
    std::cout << std::endl;
    
    // Test 2: Alternation (BitNFA might be faster)
    std::cout << "Pattern: Huck[a-zA-Z]+|Saw[a-zA-Z]+" << std::endl;
    auto t2_ctre = bench([&]() { result = (bool)ctre::match<"Huck[a-zA-Z]+|Saw[a-zA-Z]+">(alt_input); });
    auto t2_bitnfa = bench([&]() { result = ctre::bitnfa::match<"Huck[a-zA-Z]+|Saw[a-zA-Z]+">(alt_input).matched; });
    std::cout << "  CTRE:   " << t2_ctre << " ns" << std::endl;
    std::cout << "  BitNFA: " << t2_bitnfa << " ns" << std::endl;
    std::cout << "  Ratio: " << (t2_bitnfa/t2_ctre) << "x" << std::endl;
    
    return 0;
}
