#include <chrono>
#include <iostream>

template<typename Func>
double benchmark(Func f, int iterations = 10000000) {
    for (int i = 0; i < 1000; ++i) f();
    
    double best = 1e9;
    for (int run = 0; run < 5; ++run) {
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iterations; ++i) f();
        auto end = std::chrono::high_resolution_clock::now();
        double time = std::chrono::duration<double, std::nano>(end - start).count() / iterations;
        if (time < best) best = time;
    }
    return best;
}

char data[256];

// Current: Separate check each iteration
int loop_separate_checks() {
    char* ptr = data;
    char* end = data + 256;
    int count = 0;
    
    while (count < 256) {
        // Check before each load (CURRENT approach)
        if ((end - ptr) < 32) break;
        
        // Simulate processing
        ptr += 32;
        count += 32;
    }
    return count;
}

// Optimized: Calculate loop count once
int loop_precalc_count() {
    char* ptr = data;
    char* end = data + 256;
    size_t remaining = end - ptr;
    size_t full_chunks = remaining / 32;  // Calculate once!
    
    for (size_t i = 0; i < full_chunks; ++i) {
        // No check needed inside loop!
        ptr += 32;
    }
    
    return full_chunks * 32;
}

int main() {
    std::cout << "=== BOUNDS CHECK OPTIMIZATION ===" << std::endl;
    std::cout << std::endl;
    std::cout << "Separate checks:  " << benchmark(loop_separate_checks) << " ns" << std::endl;
    std::cout << "Precalc count:    " << benchmark(loop_precalc_count) << " ns" << std::endl;
    std::cout << std::endl;
    std::cout << "Speedup: " << (benchmark(loop_separate_checks) / benchmark(loop_precalc_count)) << "x" << std::endl;
    
    return 0;
}
