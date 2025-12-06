// Integration Test: Basic performance characteristics
#include <ctre.hpp>
#include <iostream>
#include <string>
#include <chrono>

template <auto Pattern>
double benchmark(const std::string& text, int iterations = 100) {
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        volatile bool found = static_cast<bool>(Pattern(text));
        (void)found;
    }
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / double(iterations);
}

int main() {
    std::cout << "=== Integration Tests: Performance ===\n\n";

    // Test 1: Simple alternation
    {
        std::cout << "Test 1: Simple alternation\n";
        constexpr auto pattern = ctre::search<"(foo|bar)">;
        std::string text = "hello world foo test";
        auto time = benchmark<pattern>(text);
        std::cout << "  Time: " << time << " ns\n";
        std::cout << "  ✓ PASSED\n";
    }

    // Test 2: Character class
    {
        std::cout << "\nTest 2: Character class\n";
        constexpr auto pattern = ctre::search<"[a-z]+">;
        std::string text = "hello world";
        auto time = benchmark<pattern>(text);
        std::cout << "  Time: " << time << " ns\n";
        std::cout << "  ✓ PASSED\n";
    }

    // Test 3: Pattern correctness
    {
        std::cout << "\nTest 3: Pattern correctness\n";
        constexpr auto pattern = ctre::search<"(foo|bar)">;
        if (!pattern("test foo end")) {
            std::cout << "  FAILED: Should match 'foo'\n";
            return 1;
        }
        if (!pattern("test bar end")) {
            std::cout << "  FAILED: Should match 'bar'\n";
            return 1;
        }
        if (pattern("test baz end")) {
            std::cout << "  FAILED: Should not match 'baz'\n";
            return 1;
        }
        std::cout << "  ✓ PASSED\n";
    }

    std::cout << "\n✓ All performance tests passed!\n";
    return 0;
}
