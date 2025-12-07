/**
 * Example Benchmark - Demonstrates the extensible benchmark framework
 * 
 * This file shows how to add new benchmark patterns using the framework.
 * 
 * To add a new pattern:
 *   1. Add a generator function in patterns.hpp (if needed)
 *   2. Add the pattern to the appropriate category in pattern_registry.hpp
 *   3. Add the CTRE template instantiation below
 * 
 * Build: g++ -std=c++20 -O3 -march=native -I ../include -I lib/include \
 *        -I benchmarks example_benchmark.cpp -o example_bench \
 *        -L lib/lib -lre2 -lpcre2-8 -lhs -lpthread
 */

#include "benchmark_main.hpp"
#include "pattern_registry.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    std::string filter = (argc > 1) ? argv[1] : "";
    
    std::cout << bench::config::CSV_HEADER << "\n";
    
    // Example: Run a single pattern category
    if (bench::utils::should_run_category("Simple", filter)) {
        // Each pattern needs its CTRE template instantiated
        bench::benchmark_pattern<"[0-9]+">(
            "Simple", "digits", "[0-9]+",
            bench::gen_digits,
            bench::config::sizes_standard()
        );
        
        bench::benchmark_pattern<"[a-z]+">(
            "Simple", "lowercase", "[a-z]+",
            bench::gen_letters,
            bench::config::sizes_standard()
        );
    }
    
    // Example: Custom pattern not in registry
    if (bench::utils::should_run_category("Custom", filter)) {
        // Define inline for one-off patterns
        bench::benchmark_pattern<"[0-9]{4}-[0-9]{2}-[0-9]{2}">(
            "Custom", "iso_date", "[0-9]{4}-[0-9]{2}-[0-9]{2}",
            bench::gen_date_full,
            {16, 32, 64}  // Custom sizes
        );
    }
    
    return 0;
}


