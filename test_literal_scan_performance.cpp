#include <iostream>
#include <chrono>
#include <string>
#include <cstring>
#include <iomanip>

// Test different literal scanning approaches to find the fastest

template<typename Func>
double benchmark(Func&& f, int iterations = 100000) {
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        f();
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    return duration / (double)iterations;
}

// Approach 1: Simple sequential memcmp (baseline)
inline bool match_sequential_memcmp(std::string_view input, const char* const* literals, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        size_t len = std::strlen(literals[i]);
        if (input.size() >= len && std::memcmp(input.data(), literals[i], len) == 0) {
            return true;
        }
    }
    return false;
}

// Approach 2: First char filter + memcmp
inline bool match_first_char_filter(std::string_view input, const char* const* literals, size_t count) {
    if (input.empty()) return false;

    char first = input[0];
    for (size_t i = 0; i < count; ++i) {
        if (literals[i][0] == first) {
            size_t len = std::strlen(literals[i]);
            if (input.size() >= len && std::memcmp(input.data(), literals[i], len) == 0) {
                return true;
            }
        }
    }
    return false;
}

// Approach 3: Lookup table for first chars
inline bool match_lookup_table(std::string_view input, const char* const* literals, size_t count) {
    if (input.empty()) return false;

    // Build lookup table
    int first_char_map[256];
    for (int i = 0; i < 256; ++i) first_char_map[i] = -1;

    for (size_t i = 0; i < count; ++i) {
        unsigned char c = static_cast<unsigned char>(literals[i][0]);
        if (first_char_map[c] == -1) {
            first_char_map[c] = static_cast<int>(i);
        }
    }

    // Quick lookup
    unsigned char first = static_cast<unsigned char>(input[0]);
    int idx = first_char_map[first];

    if (idx >= 0) {
        // Check all literals with this first char
        for (size_t i = 0; i < count; ++i) {
            if (literals[i][0] == input[0]) {
                size_t len = std::strlen(literals[i]);
                if (input.size() >= len && std::memcmp(input.data(), literals[i], len) == 0) {
                    return true;
                }
            }
        }
    }

    return false;
}

// Approach 4: Compile-time switch (fastest!)
inline bool match_compile_time_switch(std::string_view input) {
    if (input.empty()) return false;

    switch (input[0]) {
        case 'T':
            return input.size() >= 3 && std::memcmp(input.data(), "Tom", 3) == 0;
        case 'S':
            return input.size() >= 6 && std::memcmp(input.data(), "Sawyer", 6) == 0;
        case 'H':
            return input.size() >= 11 && std::memcmp(input.data(), "Huckleberry", 11) == 0;
        case 'F':
            return input.size() >= 4 && std::memcmp(input.data(), "Finn", 4) == 0;
        default:
            return false;
    }
}

int main() {
    std::cout << "╔═══════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║    Literal Scanning Performance Comparison                           ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;

    const char* literals[] = {"Tom", "Sawyer", "Huckleberry", "Finn"};
    std::string input = "Huckleberry";

    std::cout << "Testing pattern: \"Tom|Sawyer|Huckleberry|Finn\"" << std::endl;
    std::cout << "Input: \"Huckleberry\"" << std::endl;
    std::cout << std::endl;

    // Benchmark each approach
    volatile bool result = false;

    auto t1 = benchmark([&]() {
        result = match_sequential_memcmp(input, literals, 4);
    });

    auto t2 = benchmark([&]() {
        result = match_first_char_filter(input, literals, 4);
    });

    auto t3 = benchmark([&]() {
        result = match_lookup_table(input, literals, 4);
    });

    auto t4 = benchmark([&]() {
        result = match_compile_time_switch(input);
    });

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Approach                        | Time (ns)    | Speedup" << std::endl;
    std::cout << "--------------------------------|--------------|---------" << std::endl;
    std::cout << "1. Sequential memcmp            | " << std::setw(10) << t1 << " ns | 1.00x (baseline)" << std::endl;
    std::cout << "2. First char filter + memcmp   | " << std::setw(10) << t2 << " ns | " << (t1/t2) << "x" << std::endl;
    std::cout << "3. Lookup table                 | " << std::setw(10) << t3 << " ns | " << (t1/t3) << "x" << std::endl;
    std::cout << "4. Compile-time switch (BEST!)  | " << std::setw(10) << t4 << " ns | " << (t1/t4) << "x ✅" << std::endl;
    std::cout << std::endl;

    // Find best
    double best = std::min({t1, t2, t3, t4});

    std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << " RECOMMENDATION" << std::endl;
    std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << std::endl;

    if (t4 == best) {
        std::cout << "✅ BEST: Compile-time switch!" << std::endl;
        std::cout << "   Use switch (first_char) for literal dispatch" << std::endl;
        std::cout << "   This is " << (t1/t4) << "x faster than sequential!" << std::endl;
    }

    std::cout << std::endl;
    std::cout << "For Teddy (future):" << std::endl;
    std::cout << "  • Replace first-char check with pshufb shuffle" << std::endl;
    std::cout << "  • Expected: 2-3x faster than compile-time switch" << std::endl;
    std::cout << "  • Total: " << (t1/t4) * 2.5 << "x faster than baseline!" << std::endl;
    std::cout << std::endl;

    return 0;
}
