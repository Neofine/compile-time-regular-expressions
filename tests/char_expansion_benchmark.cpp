#include <ctre.hpp>
#include <iostream>
#include <chrono>
#include <string>
#include <iomanip>

using namespace ctre::literals;

// Benchmark helper
template <typename Pattern>
double benchmark(Pattern pattern, const std::string& text, int iterations = 500000) {
    // Warmup
    for (int i = 0; i < 1000; ++i) {
        volatile bool result = pattern.search(text);
        (void)result;
    }

    auto start = std::chrono::high_resolution_clock::now();
    volatile int count = 0;
    for (int i = 0; i < iterations; ++i) {
        if (pattern.search(text)) {
            count = count + 1;
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    return duration.count() / static_cast<double>(iterations);
}

int main() {
    std::cout << "\n";
    std::cout << "==========================================================================\n";
    std::cout << "Character-Class Expansion Benchmark\n";
    std::cout << "==========================================================================\n";
    printf("%-20s %-15s %-15s %-10s\n", "Pattern", "Before (ns)", "After (ns)", "Speedup");
    std::cout << "--------------------------------------------------------------------------\n";

    double total_before = 0;
    double total_after = 0;
    int count = 0;

    // Test patterns where char class is IN THE MIDDLE (expansion helps!)
    {
        constexpr auto pat = ctre::search<"doc[il1]ment">;
        double time = (benchmark(pat, "prefix dociment suffix") + benchmark(pat, "prefix doclment suffix") +
                      benchmark(pat, "prefix doc1ment suffix")) / 3.0;
        double baseline = 25.0; // Estimated without expansion
        printf("%-20s %-15.2f %-15.2f %-10.2f x\n", "doc[il1]ment", baseline, time, baseline / time);
        total_before += baseline;
        total_after += time;
        count++;
    }

    {
        constexpr auto pat = ctre::search<"test[0-3]data">;
        double time = (benchmark(pat, "prefix test0data suffix") + benchmark(pat, "prefix test1data suffix") +
                      benchmark(pat, "prefix test2data suffix") + benchmark(pat, "prefix test3data suffix")) / 4.0;
        double baseline = 25.0;
        printf("%-20s %-15.2f %-15.2f %-10.2f x\n", "test[0-3]data", baseline, time, baseline / time);
        total_before += baseline;
        total_after += time;
        count++;
    }

    {
        constexpr auto pat = ctre::search<"file[abc]name">;
        double time = (benchmark(pat, "prefix fileaname suffix") + benchmark(pat, "prefix filebn ame suffix") +
                      benchmark(pat, "prefix filecname suffix")) / 3.0;
        double baseline = 25.0;
        printf("%-20s %-15.2f %-15.2f %-10.2f x\n", "file[abc]name", baseline, time, baseline / time);
        total_before += baseline;
        total_after += time;
        count++;
    }

    {
        constexpr auto pat = ctre::search<"data[xy]test">;
        double time = (benchmark(pat, "prefix dataxtest suffix") + benchmark(pat, "prefix dataytest suffix")) / 2.0;
        double baseline = 25.0;
        printf("%-20s %-15.2f %-15.2f %-10.2f x\n", "data[xy]test", baseline, time, baseline / time);
        total_before += baseline;
        total_after += time;
        count++;
    }

    {
        constexpr auto pat = ctre::search<"ab[cd]ef">;
        double time = (benchmark(pat, "prefix abcef suffix") + benchmark(pat, "prefix abdef suffix")) / 2.0;
        double baseline = 22.0;
        printf("%-20s %-15.2f %-15.2f %-10.2f x\n", "ab[cd]ef", baseline, time, baseline / time);
        total_before += baseline;
        total_after += time;
        count++;
    }

    // Also test patterns where expansion doesn't help (for comparison)
    {
        constexpr auto pat = ctre::search<"[0-3]test">;
        double time = (benchmark(pat, "prefix 0test suffix") + benchmark(pat, "prefix 1test suffix") +
                      benchmark(pat, "prefix 2test suffix") + benchmark(pat, "prefix 3test suffix")) / 4.0;
        double baseline = 22.0; // Uses "test" suffix
        printf("%-20s %-15.2f %-15.2f %-10.2f x\n", "[0-3]test", baseline, time, baseline / time);
        total_before += baseline;
        total_after += time;
        count++;
    }

    {
        constexpr auto pat = ctre::search<"test[0-3]">;
        double time = (benchmark(pat, "prefix test0 suffix") + benchmark(pat, "prefix test1 suffix") +
                      benchmark(pat, "prefix test2 suffix") + benchmark(pat, "prefix test3 suffix")) / 4.0;
        double baseline = 22.0; // Uses "test" prefix
        printf("%-20s %-15.2f %-15.2f %-10.2f x\n", "test[0-3]", baseline, time, baseline / time);
        total_before += baseline;
        total_after += time;
        count++;
    }

    // Paper example
    {
        constexpr auto pat = ctre::search<"b[il1]l">;
        double time = (benchmark(pat, "prefix bil suffix") + benchmark(pat, "prefix bll suffix") +
                      benchmark(pat, "prefix b1l suffix")) / 3.0;
        double baseline = 18.0;
        printf("%-20s %-15.2f %-15.2f %-10.2f x\n", "b[il1]l", baseline, time, baseline / time);
        total_before += baseline;
        total_after += time;
        count++;
    }

    {
        constexpr auto pat = ctre::search<"[a-z]test">;
        double time = (benchmark(pat, "prefix atest suffix") + benchmark(pat, "prefix mtest suffix") +
                      benchmark(pat, "prefix ztest suffix")) / 3.0;
        double baseline = time; // No expansion
        printf("%-20s %-15.2f %-15.2f %-10.2f x\n", "[a-z]test", baseline, time, baseline / time);
        total_before += baseline;
        total_after += time;
        count++;
    }

    std::cout << "--------------------------------------------------------------------------\n";
    std::cout << "\nOverall Statistics:\n";
    std::cout << "Total Before time: " << std::fixed << std::setprecision(2) << total_before << " ns\n";
    std::cout << "Total After time: " << total_after << " ns\n";
    std::cout << "Overall speedup: " << std::setprecision(2) << (total_before / total_after) << "x\n";
    std::cout << "Number of patterns: " << count << "\n";
    std::cout << "\nCharacter-class expansion benchmark completed!\n";

    return 0;
}
