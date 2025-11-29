#include <ctre.hpp>
#include <ctre/wrapper_with_prefilter.hpp>
#include <iostream>
#include <chrono>
#include <string>

template<typename Func>
double bench(Func&& f, int iter = 100000) {
    volatile bool dummy = false;
    for (int w = 0; w < 10000; ++w) dummy = f();
    
    double min_time = 1e9;
    for (int sample = 0; sample < 10; ++sample) {
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iter; ++i) {
            dummy = f();
        }
        auto end = std::chrono::high_resolution_clock::now();
        double t = std::chrono::duration<double, std::nano>(end - start).count() / iter;
        if (t < min_time) min_time = t;
    }
    return min_time;
}

int main() {
    std::cout << "MICRO-BENCHMARK: Prefilter vs No Prefilter\n";
    std::cout << std::string(70, '=') << "\n\n";
    
    // Test 1: Pattern WITHOUT literal (a+)
    {
        std::string input(32, 'a');
        double t1 = bench([&]() { return ctre::match<"a+">(input); });
        double t2 = bench([&]() { return ctre::match_with_prefilter<"a+">(input); });
        
        std::cout << "a+_32 (no literal to extract):\n";
        std::cout << "  Standard:    " << t1 << " ns\n";
        std::cout << "  Prefilter:   " << t2 << " ns\n";
        std::cout << "  Overhead:    " << (t2 - t1) << " ns (" << (t2/t1*100-100) << "%)\n\n";
    }
    
    // Test 2: Pattern WITH literal, HAS match
    {
        std::string input = "footest";
        double t1 = bench([&]() { return ctre::match<"(foo|bar)test">(input); });
        double t2 = bench([&]() { return ctre::match_with_prefilter<"(foo|bar)test">(input); });
        
        std::cout << "(foo|bar)test WITH match:\n";
        std::cout << "  Standard:    " << t1 << " ns\n";
        std::cout << "  Prefilter:   " << t2 << " ns\n";
        std::cout << "  Overhead:    " << (t2 - t1) << " ns (" << (t2/t1*100-100) << "%)\n\n";
    }
    
    // Test 3: Pattern WITH literal, NO match (should be faster)
    {
        std::string input(100, 'x');
        double t1 = bench([&]() { return ctre::match<"(foo|bar)test">(input); }, 10000);
        double t2 = bench([&]() { return ctre::match_with_prefilter<"(foo|bar)test">(input); }, 10000);
        
        std::cout << "(foo|bar)test NO match (100 bytes):\n";
        std::cout << "  Standard:    " << t1 << " ns\n";
        std::cout << "  Prefilter:   " << t2 << " ns\n";
        std::cout << "  Speedup:     " << (t1/t2) << "x\n\n";
    }
    
    return 0;
}
