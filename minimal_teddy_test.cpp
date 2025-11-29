#include <iostream>
#include <chrono>
#include <string>
#include <cstring>

// Minimal test to find the exact overhead source

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
    std::string input = "Tom";
    volatile bool result = false;

    std::cout << "Minimal overhead test:" << std::endl;
    std::cout << std::endl;

    // Test 1: Direct comparison (what CTRE compiles to)
    auto t1 = bench([&]() {
        result = (input == "Tom") || (input == "Sawyer") || (input == "Huckleberry") || (input == "Finn");
    });
    std::cout << "1. Direct comparison (||):       " << t1 << " ns" << std::endl;

    // Test 2: Memcmp in if-else chain
    auto t2 = bench([&]() {
        if (input.size() == 3 && std::memcmp(input.data(), "Tom", 3) == 0) result = true;
        else if (input.size() == 6 && std::memcmp(input.data(), "Sawyer", 6) == 0) result = true;
        else if (input.size() == 11 && std::memcmp(input.data(), "Huckleberry", 11) == 0) result = true;
        else if (input.size() == 4 && std::memcmp(input.data(), "Finn", 4) == 0) result = true;
        else result = false;
    });
    std::cout << "2. Memcmp if-else chain:          " << t2 << " ns" << std::endl;

    // Test 3: Array of lengths + loop
    struct lit { const char* data; size_t len; };
    constexpr lit lits[] = {{"Tom", 3}, {"Sawyer", 6}, {"Huckleberry", 11}, {"Finn", 4}};

    auto t3 = bench([&]() {
        result = false;
        for (int i = 0; i < 4; ++i) {
            if (input.size() == lits[i].len && std::memcmp(input.data(), lits[i].data, lits[i].len) == 0) {
                result = true;
                break;
            }
        }
    });
    std::cout << "3. Array + loop with break:       " << t3 << " ns" << std::endl;

    // Test 4: Array without break (check all)
    auto t4 = bench([&]() {
        result = false;
        for (int i = 0; i < 4; ++i) {
            if (input.size() == lits[i].len && std::memcmp(input.data(), lits[i].data, lits[i].len) == 0) {
                result = true;
            }
        }
    });
    std::cout << "4. Array + loop without break:    " << t4 << " ns" << std::endl;

    // Test 5: Switch on first char
    auto t5 = bench([&]() {
        if (input.empty()) {
            result = false;
        } else {
            switch (input[0]) {
                case 'T':
                    result = (input == "Tom");
                    break;
                case 'S':
                    result = (input == "Sawyer");
                    break;
                case 'H':
                    result = (input == "Huckleberry");
                    break;
                case 'F':
                    result = (input == "Finn");
                    break;
                default:
                    result = false;
            }
        }
    });
    std::cout << "5. Switch on first char:          " << t5 << " ns" << std::endl;

    std::cout << std::endl;
    std::cout << "Best: " << std::min({t1, t2, t3, t4, t5}) << " ns" << std::endl;
    std::cout << std::endl;

    std::cout << "Analysis:" << std::endl;
    if (t1 < t2 && t1 < t3) {
        std::cout << "  ✅ Direct || comparison wins!" << std::endl;
        std::cout << "     This is what CTRE compiles to!" << std::endl;
    } else if (t5 < t1) {
        std::cout << "  ✅ Switch on first char wins!" << std::endl;
        std::cout << "     Opportunity to optimize!" << std::endl;
    }

    std::cout << std::endl;
    std::cout << "Overhead from data structure approach:" << std::endl;
    std::cout << "  Direct vs Array+loop: +" << (t3 - t1) << " ns (" << ((t3/t1 - 1) * 100) << "% slower)" << std::endl;

    return 0;
}
