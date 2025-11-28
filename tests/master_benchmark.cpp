#include <algorithm>
#include <chrono>
#include <ctre.hpp>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <thread>
#include <vector>

// Generate test strings that match patterns
inline std::string gen_repeat(char c, size_t len) {
    return std::string(len, c);
}

inline std::string gen_range(char start, size_t count, size_t len) {
    std::string result;
    for (size_t i = 0; i < len; ++i)
        result += static_cast<char>(start + (i % count));
    return result;
}

inline std::string gen_sparse(const char* chars, size_t char_count, size_t len) {
    std::string result;
    for (size_t i = 0; i < len; ++i)
        result += chars[i % char_count];
    return result;
}

// Benchmark a single pattern in isolation (no I-cache thrashing!)
template<ctll::fixed_string Pattern>
double benchmark_isolated(const std::string& test_string, int iterations = 100000) {
    volatile bool result = false;

    // Extended warmup to stabilize CPU frequency and caches
    for (int i = 0; i < 10000; ++i) {
        result = ctre::match<Pattern>(test_string);
    }

    // Run many samples and take minimum (best case = CPU at full speed)
    double min_time = std::numeric_limits<double>::max();

    for (int sample = 0; sample < 10; ++sample) {
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < iterations; ++i) {
            result = ctre::match<Pattern>(test_string);
        }

        auto end = std::chrono::high_resolution_clock::now();
        double time_ns = std::chrono::duration<double, std::nano>(end - start).count() / iterations;

        if (time_ns < min_time) {
            min_time = time_ns;
        }
    }

    return min_time;
}

struct BenchResult {
    const char* name;
    const char* description;
    double time_ns;
};

int main() {
    constexpr int ITER = 100000;
    std::vector<BenchResult> results;

    std::cout << "CTRE Master Benchmark (Isolated Testing - No I-cache Interference)\n";
    std::cout << "====================================================================\n\n";

    // Macro to make benchmarking concise
    #define BENCH(name, pattern, test_str, desc) \
        results.push_back({name, desc, benchmark_isolated<pattern>(test_str, ITER)})

    // Single character patterns
    BENCH("a*_32", "a*", gen_repeat('a', 32), "Single char a star");
    BENCH("b*_32", "b*", gen_repeat('b', 32), "Single char b star");
    BENCH("z*_32", "z*", gen_repeat('z', 32), "Single char z star");
    BENCH("9*_32", "9*", gen_repeat('9', 32), "Single char 9 star");
    BENCH("A*_32", "A*", gen_repeat('A', 32), "Single char A star");

    BENCH("a+_32", "a+", gen_repeat('a', 32), "Single char a plus");
    BENCH("b+_32", "b+", gen_repeat('b', 32), "Single char b plus");
    BENCH("z+_32", "z+", gen_repeat('z', 32), "Single char z plus");
    BENCH("9+_32", "9+", gen_repeat('9', 32), "Single char 9 plus");
    BENCH("A+_32", "A+", gen_repeat('A', 32), "Single char A plus");

    // Small ranges (2-5 chars) - Previously showed false slowdowns
    BENCH("[0-2]*_32", "[0-2]*", gen_range('0', 3, 32), "Tiny range 0-2 star");
    BENCH("[0-2]+_32", "[0-2]+", gen_range('0', 3, 32), "Tiny range 0-2 plus");
    BENCH("[a-c]*_32", "[a-c]*", gen_range('a', 3, 32), "Tiny range a-c star");
    BENCH("[a-c]+_32", "[a-c]+", gen_range('a', 3, 32), "Tiny range a-c plus");
    BENCH("[a-e]*_32", "[a-e]*", gen_range('a', 5, 32), "Small range a-e star");
    BENCH("[a-e]+_32", "[a-e]+", gen_range('a', 5, 32), "Small range a-e plus");
    BENCH("[x-z]*_32", "[x-z]*", gen_range('x', 3, 32), "Tiny range x-z star");
    BENCH("[x-z]+_32", "[x-z]+", gen_range('x', 3, 32), "Tiny range x-z plus");

    // Medium ranges (9-26 chars) - Should use SIMD
    BENCH("[0-9]*_32", "[0-9]*", gen_range('0', 10, 32), "Digits star (32)");
    BENCH("[0-9]+_32", "[0-9]+", gen_range('0', 10, 32), "Digits plus (32)");
    BENCH("[a-z]*_16", "[a-z]*", gen_range('a', 26, 16), "Lowercase star (16)");
    BENCH("[a-z]*_32", "[a-z]*", gen_range('a', 26, 32), "Lowercase star (32)");
    BENCH("[a-z]*_64", "[a-z]*", gen_range('a', 26, 64), "Lowercase star (64)");
    BENCH("[a-z]*_128", "[a-z]*", gen_range('a', 26, 128), "Lowercase star (128)");
    BENCH("[a-z]*_256", "[a-z]*", gen_range('a', 26, 256), "Lowercase star (256)");
    BENCH("[a-z]*_512", "[a-z]*", gen_range('a', 26, 512), "Lowercase star (512)");

    BENCH("[a-z]+_16", "[a-z]+", gen_range('a', 26, 16), "Lowercase plus (16)");
    BENCH("[a-z]+_32", "[a-z]+", gen_range('a', 26, 32), "Lowercase plus (32)");
    BENCH("[a-z]+_64", "[a-z]+", gen_range('a', 26, 64), "Lowercase plus (64)");
    BENCH("[a-z]+_128", "[a-z]+", gen_range('a', 26, 128), "Lowercase plus (128)");
    BENCH("[a-z]+_256", "[a-z]+", gen_range('a', 26, 256), "Lowercase plus (256)");
    BENCH("[a-z]+_512", "[a-z]+", gen_range('a', 26, 512), "Lowercase plus (512)");

    BENCH("[A-Z]*_32", "[A-Z]*", gen_range('A', 26, 32), "Uppercase star (32)");
    BENCH("[A-Z]*_256", "[A-Z]*", gen_range('A', 26, 256), "Uppercase star (256)");
    BENCH("[A-Z]+_32", "[A-Z]+", gen_range('A', 26, 32), "Uppercase plus (32)");

    BENCH("[0-9]*_256", "[0-9]*", gen_range('0', 10, 256), "Digits star (256)");
    BENCH("[0-9]+_256", "[0-9]+", gen_range('0', 10, 256), "Digits plus (256)");

    // Mixed ranges
    BENCH("[a-zA-Z]*_32", "[a-zA-Z]*", gen_range('a', 52, 32), "Mixed case star (32)");
    BENCH("[a-zA-Z]*_64", "[a-zA-Z]*", gen_range('a', 52, 64), "Mixed case star (64)");
    BENCH("[a-zA-Z]*_128", "[a-zA-Z]*", gen_range('a', 52, 128), "Mixed case star (128)");
    BENCH("[a-zA-Z]+_32", "[a-zA-Z]+", gen_range('a', 52, 32), "Mixed case plus (32)");
    BENCH("[a-zA-Z]+_64", "[a-zA-Z]+", gen_range('a', 52, 64), "Mixed case plus (64)");

    // Hex patterns (3 ranges)
    BENCH("[0-9a-f]*_32", "[0-9a-f]*", gen_sparse("0123456789abcdef", 16, 32), "Hex lowercase star (32)");
    BENCH("[0-9a-f]*_64", "[0-9a-f]*", gen_sparse("0123456789abcdef", 16, 64), "Hex lowercase star (64)");
    BENCH("[0-9a-f]+_32", "[0-9a-f]+", gen_sparse("0123456789abcdef", 16, 32), "Hex lowercase plus (32)");
    BENCH("[0-9a-fA-F]*_32", "[0-9a-fA-F]*", gen_sparse("0123456789abcdefABCDEF", 22, 32), "Hex mixed star (32)");
    BENCH("[0-9a-fA-F]+_32", "[0-9a-fA-F]+", gen_sparse("0123456789abcdefABCDEF", 22, 32), "Hex mixed plus (32)");

    // Multi-range patterns (4, 6, 8 ranges) - Tests N-range SIMD scalability
    BENCH("[a-dA-Dg-jG-J]*_32", "[a-dA-Dg-jG-J]*", gen_sparse("abcdABCDghijGHIJ", 16, 32), "4 ranges star (32)");
    BENCH("[a-dA-Dg-jG-J]*_64", "[a-dA-Dg-jG-J]*", gen_sparse("abcdABCDghijGHIJ", 16, 64), "4 ranges star (64)");
    BENCH("[a-cA-Ce-gE-Gj-lJ-Lm-oM-Op-rP-R]*_32", "[a-cA-Ce-gE-Gj-lJ-Lm-oM-Op-rP-R]*",
           gen_sparse("abcABCefgEFGjklJKLmnoMNOpqrPQR", 30, 32), "6 ranges star (32)");
    BENCH("[a-cA-Ce-gE-Gj-lJ-Lm-oM-Op-rP-R]*_64", "[a-cA-Ce-gE-Gj-lJ-Lm-oM-Op-rP-R]*",
           gen_sparse("abcABCefgEFGjklJKLmnoMNOpqrPQR", 30, 64), "6 ranges star (64)");
    BENCH("[a-bA-Bd-eD-Eg-hG-Hi-jI-Jl-mL-Mn-oN-Op-qP-Q]*_32", "[a-bA-Bd-eD-Eg-hG-Hi-jI-Jl-mL-Mn-oN-Op-qP-Q]*",
           gen_sparse("abABdeDEghGHijIJlmLMnoNOpqPQ", 28, 32), "8 ranges star (32)");
    BENCH("[a-bA-Bd-eD-Eg-hG-Hi-jI-Jl-mL-Mn-oN-Op-qP-Q]*_64", "[a-bA-Bd-eD-Eg-hG-Hi-jI-Jl-mL-Mn-oN-Op-qP-Q]*",
           gen_sparse("abABdeDEghGHijIJlmLMnoNOpqPQ", 28, 64), "8 ranges star (64)");

    // Sparse character sets (Shufti candidates)
    BENCH("[aeiou]*_32", "[aeiou]*", gen_sparse("aeiou", 5, 32), "Vowels star (32)");
    BENCH("[aeiou]*_64", "[aeiou]*", gen_sparse("aeiou", 5, 64), "Vowels star (64)");
    BENCH("[aeiou]+_32", "[aeiou]+", gen_sparse("aeiou", 5, 32), "Vowels plus (32)");
    BENCH("[aeiouAEIOU]*_32", "[aeiouAEIOU]*", gen_sparse("aeiouAEIOU", 10, 32), "All vowels star (32)");

    // Sparse digit patterns (poor nibble diversity)
    BENCH("[02468]*_32", "[02468]*", gen_sparse("02468", 5, 32), "Even digits star (32)");
    BENCH("[02468]+_32", "[02468]+", gen_sparse("02468", 5, 32), "Even digits plus (32)");
    BENCH("[13579]*_32", "[13579]*", gen_sparse("13579", 5, 32), "Odd digits star (32)");
    BENCH("[13579]+_32", "[13579]+", gen_sparse("13579", 5, 32), "Odd digits plus (32)");

    // Unicode/Extended character note:
    // CTRE currently focuses on ASCII (0x00-0x7F) patterns for maximum performance.
    // Future enhancements could include:
    //   - Extended ASCII (0x80-0xFF) with \xHH escape notation
    //   - Multi-byte UTF-8 (Greek: Α-Ω, Cyrillic: А-Я, etc.) with UTF-8 aware SIMD
    //   - Unicode property classes (\p{Greek}, \p{Cyrillic}, etc.)
    //
    // Current SIMD optimizations work excellently for:
    //   - ASCII letters [a-zA-Z]
    //   - ASCII digits [0-9]
    //   - ASCII symbols and punctuation
    //   - Mixed ASCII ranges like [0-9a-fA-F]

    // More sizes for scaling tests
    BENCH("a*_16", "a*", gen_repeat('a', 16), "Single a star (16)");
    BENCH("a*_64", "a*", gen_repeat('a', 64), "Single a star (64)");
    BENCH("a*_128", "a*", gen_repeat('a', 128), "Single a star (128)");
    BENCH("a*_256", "a*", gen_repeat('a', 256), "Single a star (256)");

    BENCH("a+_16", "a+", gen_repeat('a', 16), "Single a plus (16)");
    BENCH("a+_64", "a+", gen_repeat('a', 64), "Single a plus (64)");
    BENCH("a+_128", "a+", gen_repeat('a', 128), "Single a plus (128)");
    BENCH("a+_256", "a+", gen_repeat('a', 256), "Single a plus (256)");

    // ========================================================================
    // REAL-WORLD COMPLEX PATTERNS (from regex benchmark suite)
    // ========================================================================
    // These test CTRE's performance on patterns beyond simple character classes

    // Literal strings - baseline for literal matching
    BENCH("literal_Twain", "Twain", std::string("Twain"), "Literal: Twain");

    // Character class + literals - tests SIMD + literal combo
    BENCH("char_literal_32", "[a-z]shing", std::string("fishing"), "Char class + literal: [a-z]shing");

    // Alternation - tests branch prediction and multiple literal matching
    BENCH("alternation_4", "Tom|Sawyer|Huckleberry|Finn", std::string("Tom"), "Alternation: 4 names");

    // Complex pattern with negated class
    BENCH("negated_class", "[a-q][^u-z]{13}x", std::string("aabcdefghijklmx"), "Negated char class pattern");

    // Literal with suffix matching
    BENCH("suffix_ing", "[a-zA-Z]+ing", std::string("fishingfishingfishing"), "Suffix matching: *ing");

    // Complex alternation with character classes
    BENCH("complex_alt", "Huck[a-zA-Z]+|Saw[a-zA-Z]+", std::string("Huckleberry"), "Complex alternation");

    // Any character patterns (tests . wildcard)
    BENCH("any_char_group", ".{0,2}(Tom|Sawyer|Huckleberry|Finn)", std::string("xxTom"), "Any char + group");
    BENCH("any_char_range", ".{2,4}(Tom|Sawyer|Huckleberry|Finn)", std::string("xxxxTom"), "Any char range + group");

    // Long-range patterns (tests . with large counts)
    BENCH("long_range_1", "Tom.{10,25}river", std::string("Tom1234567890river"), "Long range: Tom...river");

    // Whitespace + character classes
    BENCH("whitespace_ing", "\\s[a-zA-Z]{0,12}ing\\s", std::string(" fishing "), "Whitespace + char class");

    // Complex groups with alternation
    BENCH("group_alt", "([A-Za-z]awyer|[A-Za-z]inn)\\s", std::string("Sawyer "), "Group alternation");

    // Quoted strings with character classes
    BENCH("quoted_str", "[\"'][^\"']{0,30}[?!\\.][\"']", std::string("\"Hello world!\""), "Quoted string pattern");

    #undef BENCH

    // Print results
    std::cout << std::left << std::setw(25) << "Pattern"
              << std::right << std::setw(15) << "Time (ns)" << "\n";
    std::cout << std::string(40, '-') << "\n";

    double total_time = 0.0;
    for (const auto& r : results) {
        std::cout << std::left << std::setw(25) << r.name
                  << std::right << std::setw(15) << std::fixed << std::setprecision(2)
                  << r.time_ns << "\n";
        total_time += r.time_ns;
    }

    std::cout << std::string(40, '-') << "\n";
    std::cout << "Total patterns tested: " << results.size() << "\n";
    std::cout << "Total time: " << std::fixed << std::setprecision(2) << total_time << " ns\n";
    std::cout << "Average time per pattern: " << std::fixed << std::setprecision(2)
              << (total_time / results.size()) << " ns\n";

    return 0;
}
