#include <chrono>
#include <iostream>
#include <cstring>

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

// Current: Check distance every iteration
int loop_current() {
    char* ptr = data;
    char* end = data + 256;
    int count = 0;
    
    while (count < 256) {
        // This check happens EVERY iteration
        if ((end - ptr) < 32) break;
        
        // Simulate 32-byte processing
        ptr += 32;
        count += 32;
    }
    return count;
}

// Optimized: Cache remaining bytes
int loop_cached() {
    char* ptr = data;
    char* end = data + 256;
    int count = 0;
    size_t remaining = end - ptr;
    
    while (count < 256 && remaining >= 32) {
        // No distance check!
        ptr += 32;
        count += 32;
        remaining -= 32;
    }
    return count;
}

int main() {
    memset(data, 'a', 256);
    
    std::cout << "=== DISTANCE CACHING TEST ===" << std::endl;
    std::cout << std::endl;
    std::cout << "Current (check every iter): " << benchmark(loop_current) << " ns" << std::endl;
    std::cout << "Cached remaining bytes:     " << benchmark(loop_cached) << " ns" << std::endl;
    
    return 0;
}
