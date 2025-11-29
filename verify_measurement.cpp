#include <ctre.hpp>
#include <iostream>
#include <chrono>
#include <string>

int main() {
    std::string input(32, 'a');
    
    // Measure ONLY the runtime execution (binary is already compiled)
    std::cout << "This binary was compiled at: " << __DATE__ << " " << __TIME__ << "\n";
    std::cout << "Now measuring RUNTIME performance...\n\n";
    
    // Warmup
    volatile bool dummy = false;
    for (int i = 0; i < 100000; ++i) {
        dummy = ctre::match<"a+">(input);
    }
    
    // Measure
    constexpr int ITERATIONS = 1000000;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < ITERATIONS; ++i) {
        dummy = ctre::match<"a+">(input);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto total_ns = std::chrono::duration<double, std::nano>(end - start).count();
    
    std::cout << "Total time: " << total_ns << " ns\n";
    std::cout << "Per iteration: " << (total_ns / ITERATIONS) << " ns\n";
    std::cout << "Iterations: " << ITERATIONS << "\n";
    std::cout << "\nThis is PURE RUNTIME - compile happened before ./program\n";
    
    return 0;
}
