#include <ctre.hpp>
#include <ctre/smart_dispatch.hpp>
#include <ctre/bitnfa/integration.hpp>
#include <iostream>
#include <chrono>

template<typename Func>
double bench(Func&& f) {
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 100000; ++i) {
        f();
    }
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / 100000.0;
}

int main() {
    std::string input = "Huckleberry";
    volatile bool result = false;
    
    std::cout << "Investigating which backends are being used..." << std::endl;
    std::cout << std::endl;
    
    // Test 1: Standard ctre::match (what benchmarks use)
    auto t1 = bench([&]() {
        result = (bool)ctre::match<"Huck[a-zA-Z]+|Saw[a-zA-Z]+">(input);
    });
    std::cout << "ctre::match<>:           " << t1 << " ns" << std::endl;
    
    // Test 2: BitNFA directly
    auto t2 = bench([&]() {
        result = ctre::bitnfa::match<"Huck[a-zA-Z]+|Saw[a-zA-Z]+">(input).matched();
    });
    std::cout << "bitnfa::match<>:         " << t2 << " ns" << std::endl;
    
    // Test 3: Smart dispatch
    auto t3 = bench([&]() {
        result = (bool)ctre::smart_dispatch::match<"Huck[a-zA-Z]+|Saw[a-zA-Z]+">(input);
    });
    std::cout << "smart_dispatch::match<>: " << t3 << " ns" << std::endl;
    
    std::cout << std::endl;
    std::cout << "Analysis:" << std::endl;
    std::cout << "  ctre::match uses: " << (t1 < t2 ? "base evaluation" : "unknown") << std::endl;
    std::cout << "  BitNFA is: " << (t2 < t1 ? "faster" : "slower") << " (" << (t1/t2) << "x)" << std::endl;
    std::cout << "  smart_dispatch chooses: " << (abs(t3 - t2) < 0.5 ? "BitNFA" : "base CTRE") << std::endl;
    
    return 0;
}
