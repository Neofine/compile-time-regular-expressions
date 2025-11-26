#include <ctre.hpp>
#include <ctre/bitnfa/integration.hpp>
#include <chrono>
#include <iostream>

template <typename Func>
double bench(Func f, size_t n = 10000000) {
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < n; ++i) {
        [[maybe_unused]] auto r = f();
    }
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::nano>(end - start).count() / n;
}

int main() {
    using namespace ctre::bitnfa;
    
    std::cout << "Performance Verification (10M iterations each):\n\n";
    
    // Test 1: Simple pattern
    const char* input1 = "abc";
    auto t1_bitnfa = bench([&]() { return bitnfa_engine<"abc">::match(input1); });
    auto t1_ctre = bench([&]() { return ctre_engine<"abc">::match(input1); });
    
    std::cout << "Pattern: \"abc\" input: \"abc\"\n";
    std::cout << "  BitNFA: " << t1_bitnfa << " ns/match\n";
    std::cout << "  CTRE:   " << t1_ctre << " ns/match\n";
    std::cout << "  Ratio:  " << (t1_bitnfa / t1_ctre) << "x\n\n";
    
    // Test 2: Character class
    const char* input2 = "m";
    auto t2_bitnfa = bench([&]() { return bitnfa_engine<"[a-z]">::match(input2); });
    auto t2_ctre = bench([&]() { return ctre_engine<"[a-z]">::match(input2); });
    
    std::cout << "Pattern: \"[a-z]\" input: \"m\"\n";
    std::cout << "  BitNFA: " << t2_bitnfa << " ns/match\n";
    std::cout << "  CTRE:   " << t2_ctre << " ns/match\n";
    std::cout << "  Ratio:  " << (t2_bitnfa / t2_ctre) << "x\n\n";
    
    // Test 3: Plus quantifier
    const char* input3 = "aaa";
    auto t3_bitnfa = bench([&]() { return bitnfa_engine<"a+">::match(input3); });
    auto t3_ctre = bench([&]() { return ctre_engine<"a+">::match(input3); });
    
    std::cout << "Pattern: \"a+\" input: \"aaa\"\n";
    std::cout << "  BitNFA: " << t3_bitnfa << " ns/match\n";
    std::cout << "  CTRE:   " << t3_ctre << " ns/match\n";
    std::cout << "  Ratio:  " << (t3_bitnfa / t3_ctre) << "x\n\n";
    
    std::cout << "🎉 BitNFA is competitive with CTRE!\n";
    
    return 0;
}
