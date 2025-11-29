#include <iostream>
#include <chrono>
#include <string>
#include <ctre.hpp>
#include <ctre/literal_optimization.hpp>

template<typename Func>
double bench(Func&& f) {
    // Warmup
    for (int i = 0; i < 1000; ++i) {
        f();
    }

    // Benchmark
    double min_time = 1e9;
    for (int sample = 0; sample < 10; ++sample) {
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < 10000; ++i) {
            f();
        }
        auto end = std::chrono::high_resolution_clock::now();
        double time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() / 10000.0;
        if (time_ns < min_time) min_time = time_ns;
    }
    return min_time;
}

int main() {
    // Create realistic search scenarios
    std::string haystack_short = "The quick brown fox jumps over the lazy dog. Tom went to the store.";
    std::string haystack_medium(500, 'x');
    haystack_medium += "Huckleberry";
    haystack_medium += std::string(500, 'y');

    std::string haystack_long(5000, 'a');
    haystack_long += "Sawyer";
    haystack_long += std::string(5000, 'b');

    volatile const char* result_ptr = nullptr;

    std::cout << "╔═══════════════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║         Teddy SEARCH Performance - Finding Its True Potential        ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;

   // Parse pattern
    using tmp = typename ctll::parser<ctre::pcre, "Tom|Sawyer|Huckleberry|Finn", ctre::pcre_actions>::template output<ctre::pcre_context<>>;
    using Pattern = decltype(ctll::front(typename tmp::output_type::stack_type()));
    constexpr auto literals = ctre::get_literal_list<Pattern>();

    // =========================================================================
    // TEST 1: Short haystack (68 bytes, literal at position 50)
    // =========================================================================

    std::cout << "TEST 1: SHORT HAYSTACK (68 bytes, 'Tom' at position 45)" << std::endl;
    std::cout << "=========================================================" << std::endl;

    auto t1_teddy = bench([&]() {
        size_t len = 0;
        result_ptr = ctre::teddy_complete::search(haystack_short, literals, &len);
        __asm__ volatile("" : : "r"(result_ptr) : "memory");
    });

    auto t1_ctre = bench([&]() {
        auto r = ctre::search<"Tom|Sawyer|Huckleberry|Finn">(haystack_short);
        if (r) {
            result_ptr = r.data();
            __asm__ volatile("" : : "r"(result_ptr) : "memory");  // Prevent optimization
        }
    });

    std::cout << "  CTRE search:        " << t1_ctre << " ns" << std::endl;
    std::cout << "  Teddy search:       " << t1_teddy << " ns" << std::endl;
    std::cout << "  Speedup:            " << (t1_ctre / t1_teddy) << "x";
    if (t1_teddy < t1_ctre) {
        std::cout << " ✅ TEDDY WINS!" << std::endl;
    } else {
        std::cout << " (CTRE wins)" << std::endl;
    }
    std::cout << std::endl;

    // =========================================================================
    // TEST 2: Medium haystack (1011 bytes, literal at position 500)
    // =========================================================================

    std::cout << "TEST 2: MEDIUM HAYSTACK (1011 bytes, 'Huckleberry' at 500)" << std::endl;
    std::cout << "===========================================================" << std::endl;

    auto t2_teddy = bench([&]() {
        size_t len = 0;
        result_ptr = ctre::teddy_complete::search(haystack_medium, literals, &len);
        __asm__ volatile("" : : "r"(result_ptr) : "memory");
    });

    auto t2_ctre = bench([&]() {
        auto r = ctre::search<"Tom|Sawyer|Huckleberry|Finn">(haystack_medium);
        if (r) {
            result_ptr = r.data();
            __asm__ volatile("" : : "r"(result_ptr) : "memory");
        }
    });

    std::cout << "  CTRE search:        " << t2_ctre << " ns" << std::endl;
    std::cout << "  Teddy search:       " << t2_teddy << " ns" << std::endl;
    std::cout << "  Speedup:            " << (t2_ctre / t2_teddy) << "x";
    if (t2_teddy < t2_ctre) {
        std::cout << " ✅ TEDDY WINS!" << std::endl;
    } else {
        std::cout << " (CTRE wins)" << std::endl;
    }
    std::cout << std::endl;

    // =========================================================================
    // TEST 3: Long haystack (10006 bytes, literal at position 5000)
    // =========================================================================

    std::cout << "TEST 3: LONG HAYSTACK (10006 bytes, 'Sawyer' at 5000)" << std::endl;
    std::cout << "======================================================" << std::endl;

    auto t3_teddy = bench([&]() {
        size_t len = 0;
        result_ptr = ctre::teddy_complete::search(haystack_long, literals, &len);
        __asm__ volatile("" : : "r"(result_ptr) : "memory");
    });

    auto t3_ctre = bench([&]() {
        auto r = ctre::search<"Tom|Sawyer|Huckleberry|Finn">(haystack_long);
        if (r) {
            result_ptr = r.data();
            __asm__ volatile("" : : "r"(result_ptr) : "memory");
        }
    });

    std::cout << "  CTRE search:        " << t3_ctre << " ns" << std::endl;
    std::cout << "  Teddy search:       " << t3_teddy << " ns" << std::endl;
    std::cout << "  Speedup:            " << (t3_ctre / t3_teddy) << "x";
    if (t3_teddy < t3_ctre) {
        std::cout << " ✅ TEDDY WINS!" << std::endl;
    } else {
        std::cout << " (CTRE wins)" << std::endl;
    }
    std::cout << std::endl;

    // =========================================================================
    // SUMMARY
    // =========================================================================

    std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << " SUMMARY: Teddy's Sweet Spot" << std::endl;
    std::cout << "═══════════════════════════════════════════════════════════════════════" << std::endl;
    std::cout << std::endl;

    std::cout << "Haystack Size | CTRE (ns) | Teddy (ns) | Speedup" << std::endl;
    std::cout << "--------------|-----------|------------|--------" << std::endl;
    std::cout << "68 bytes      | " << t1_ctre << " | " << t1_teddy << " | " << (t1_ctre / t1_teddy) << "x" << std::endl;
    std::cout << "1011 bytes    | " << t2_ctre << " | " << t2_teddy << " | " << (t2_ctre / t2_teddy) << "x" << std::endl;
    std::cout << "10006 bytes   | " << t3_ctre << " | " << t3_teddy << " | " << (t3_ctre / t3_teddy) << "x" << std::endl;
    std::cout << std::endl;

    double avg_speedup = ((t1_ctre / t1_teddy) + (t2_ctre / t2_teddy) + (t3_ctre / t3_teddy)) / 3.0;
    std::cout << "Average Teddy speedup for SEARCH: " << avg_speedup << "x 🔥" << std::endl;
    std::cout << std::endl;

    if (avg_speedup > 2.0) {
        std::cout << "✅ Teddy's TRUE POTENTIAL: " << avg_speedup << "x for search operations!" << std::endl;
        std::cout << "   This is what the 1150 lines of code were meant for!" << std::endl;
    } else {
        std::cout << "⚠️  Teddy not showing expected gains. Need to dig deeper!" << std::endl;
    }

    return 0;
}
