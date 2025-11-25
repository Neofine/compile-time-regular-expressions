#include <iostream>
#include <chrono>
#include <string>

// Simulate the two approaches

// Current: match with MinCount=1 check
template<typename Func>
size_t match_plus_current(const std::string& str, Func match_char) {
    size_t count = 0;
    size_t i = 0;
    constexpr size_t MinCount = 1;

    // SIMD loop (simulated with scalar)
    while (i < str.size()) {
        if (match_char(str[i])) {
            ++i;
            ++count;
        } else {
            break;
        }
    }

    // MinCount check
    return (count >= MinCount) ? count : 0;
}

// Optimized: a+ → aa*
template<typename Func>
size_t match_plus_optimized(const std::string& str, Func match_char) {
    size_t i = 0;

    // Match first char (required for +)
    if (i < str.size() && match_char(str[i])) {
        ++i;
    } else {
        return 0;  // Fail fast
    }

    // Now match * (MinCount=0, no check needed)
    while (i < str.size()) {
        if (match_char(str[i])) {
            ++i;
        } else {
            break;
        }
    }

    return i;
}

template<typename Func>
double benchmark(Func func, int iterations = 1000000) {
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        func();
    }
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::nano>(end - start).count() / iterations;
}

int main() {
    std::string test = "abcabcabcabcabcabcabcabcabcabcab";  // 32 chars
    auto match_ac = [](char c) { return c >= 'a' && c <= 'c'; };

    volatile size_t result;

    auto time_current = benchmark([&]() {
        result = match_plus_current(test, match_ac);
    });

    auto time_optimized = benchmark([&]() {
        result = match_plus_optimized(test, match_ac);
    });

    std::cout << "Current (with MinCount check):  " << time_current << " ns\n";
    std::cout << "Optimized (a+ → aa*):           " << time_optimized << " ns\n";
    std::cout << "Speedup:                        " << time_current / time_optimized << "x\n";

    return 0;
}
