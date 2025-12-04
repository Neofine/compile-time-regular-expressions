/**
 * Thesis Benchmark - Full String Matching with ^pattern$
 *
 * All patterns are anchored to match the entire input string.
 */

#include <chrono>
#include <ctre.hpp>
#include <iomanip>
#include <iostream>
#include <re2/re2.h>
#include <regex>
#include <string>
#include <vector>
#define PCRE2_CODE_UNIT_WIDTH 8
#include <hs/hs.h>
#include <pcre2.h>

#include "patterns.hpp"

// Configuration
constexpr int WARMUP = 3;
constexpr int ITERS = 10;
constexpr int INPUTS = 1000;          // Most engines: 1000 inputs
constexpr int INPUTS_STD_REGEX = 200; // std::regex: 200 inputs (slow)

#ifdef CTRE_DISABLE_SIMD
#define CTRE_ENGINE "CTRE"
#else
#define CTRE_ENGINE "CTRE-SIMD"
#endif

// Sizes
std::vector<size_t> SIZES = {16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768};
std::vector<size_t> SMALL_SIZES = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
std::vector<size_t> LARGE_SIZES = {32768, 65536, 131072, 262144, 524288, 1048576, 2097152, 4194304, 8388608};

void print_result(const std::string& cat, const std::string& pat, const std::string& eng, size_t size, double ns,
                  size_t matches) {
    std::cout << cat << "/" << pat << "," << eng << "," << size << "," << std::fixed << std::setprecision(2) << ns
              << "," << matches << "\n";
}

// RE2 benchmark - full match
void bench_re2(const std::string& cat, const std::string& name, const std::string& pattern,
               const std::vector<std::string>& inputs) {
    RE2 re("^" + pattern + "$");
    if (!re.ok())
        return;

    size_t matches = 0;
    for (int i = 0; i < WARMUP; i++) {
        for (const auto& s : inputs) {
            if (RE2::FullMatch(s, re))
                matches++;
        }
    }
    matches = 0;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ITERS; i++) {
        for (const auto& s : inputs) {
            if (RE2::FullMatch(s, re))
                matches++;
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    double ns = std::chrono::duration<double, std::nano>(end - start).count() / (ITERS * inputs.size());
    print_result(cat, name, "RE2", inputs[0].size(), ns, matches);
}

// PCRE2 benchmark - full match with anchors
void bench_pcre2(const std::string& cat, const std::string& name, const std::string& pattern,
                 const std::vector<std::string>& inputs) {
    std::string anchored = "^" + pattern + "$";
    int err;
    PCRE2_SIZE off;
    auto* re = pcre2_compile((PCRE2_SPTR)anchored.c_str(), PCRE2_ZERO_TERMINATED, 0, &err, &off, nullptr);
    if (!re)
        return;
    auto* md = pcre2_match_data_create_from_pattern(re, nullptr);

    size_t matches = 0;
    for (int i = 0; i < WARMUP; i++) {
        for (const auto& s : inputs) {
            if (pcre2_match(re, (PCRE2_SPTR)s.c_str(), s.size(), 0, 0, md, nullptr) >= 0)
                matches++;
        }
    }
    matches = 0;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ITERS; i++) {
        for (const auto& s : inputs) {
            if (pcre2_match(re, (PCRE2_SPTR)s.c_str(), s.size(), 0, 0, md, nullptr) >= 0)
                matches++;
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    double ns = std::chrono::duration<double, std::nano>(end - start).count() / (ITERS * inputs.size());
    print_result(cat, name, "PCRE2", inputs[0].size(), ns, matches);

    pcre2_match_data_free(md);
    pcre2_code_free(re);
}

// Hyperscan callback
static int hs_cb(unsigned int, unsigned long long, unsigned long long, unsigned int, void* ctx) {
    (*static_cast<size_t*>(ctx))++;
    return 0;
}

void bench_hyperscan(const std::string& cat, const std::string& name, const std::string& pattern,
                     const std::vector<std::string>& inputs) {
    // Use anchored pattern with ^ and $ for full-string matching
    std::string anchored = "^" + pattern + "$";
    hs_database_t* db = nullptr;
    hs_compile_error_t* error = nullptr;
    // HS_FLAG_SINGLEMATCH: stop after first match (we only need yes/no)
    // HS_FLAG_DOTALL: . matches newlines (consistent with other engines)
    if (hs_compile(anchored.c_str(), HS_FLAG_SINGLEMATCH | HS_FLAG_DOTALL, HS_MODE_BLOCK, nullptr, &db, &error) !=
        HS_SUCCESS) {
        if (error)
            hs_free_compile_error(error);
        return;
    }
    hs_scratch_t* scratch = nullptr;
    hs_alloc_scratch(db, &scratch);

    size_t matches = 0;
    for (int i = 0; i < WARMUP; i++) {
        for (const auto& s : inputs) {
            size_t m = 0;
            hs_scan(db, s.c_str(), s.size(), 0, scratch, hs_cb, &m);
            matches += m;
        }
    }
    matches = 0;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ITERS; i++) {
        for (const auto& s : inputs) {
            size_t m = 0;
            hs_scan(db, s.c_str(), s.size(), 0, scratch, hs_cb, &m);
            matches += m;
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    double ns = std::chrono::duration<double, std::nano>(end - start).count() / (ITERS * inputs.size());
    print_result(cat, name, "Hyperscan", inputs[0].size(), ns, matches);

    hs_free_scratch(scratch);
    hs_free_database(db);
}

// std::regex benchmark - full match (uses fewer inputs due to slowness)
void bench_std_regex(const std::string& cat, const std::string& name, const std::string& pattern,
                     const std::vector<std::string>& inputs) {
    // std::regex uses recursive backtracking -> stack overflow on very large inputs
    // Limit to 8192 bytes (2^13) - larger inputs cause excessive slowdown
    if (!inputs.empty() && inputs[0].size() > 8192)
        return;

    std::regex re;
    try {
        re = std::regex("^" + pattern + "$");
    } catch (...) {
        return;
    }

    // Use only first INPUTS_STD_REGEX inputs (std::regex is slow)
    size_t num_inputs = std::min(inputs.size(), static_cast<size_t>(INPUTS_STD_REGEX));

    size_t matches = 0;
    for (int i = 0; i < WARMUP; i++) {
        for (size_t j = 0; j < num_inputs; j++) {
            if (std::regex_match(inputs[j], re))
                matches++;
        }
    }
    matches = 0;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ITERS; i++) {
        for (size_t j = 0; j < num_inputs; j++) {
            if (std::regex_match(inputs[j], re))
                matches++;
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    double ns = std::chrono::duration<double, std::nano>(end - start).count() / (ITERS * num_inputs);
    print_result(cat, name, "std::regex", inputs[0].size(), ns, matches);
}

// CTRE benchmark template - full match with anchors
template <ctll::fixed_string Pattern>
void bench_ctre(const std::string& cat, const std::string& name, const std::vector<std::string>& inputs) {
    size_t matches = 0;
    for (int i = 0; i < WARMUP; i++) {
        for (const auto& s : inputs) {
            if (ctre::match<Pattern>(s))
                matches++;
        }
    }
    matches = 0;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ITERS; i++) {
        for (const auto& s : inputs) {
            if (ctre::match<Pattern>(s))
                matches++;
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    double ns = std::chrono::duration<double, std::nano>(end - start).count() / (ITERS * inputs.size());
    print_result(cat, name, CTRE_ENGINE, inputs[0].size(), ns, matches);
}

// Benchmark all engines for a pattern
template <ctll::fixed_string CTREPat>
void benchmark_pattern(const std::string& cat, const std::string& name, const std::string& re2pat,
                       bench::InputGenerator gen, const std::vector<size_t>& sizes) {
    for (size_t size : sizes) {
        auto inputs = gen(size, INPUTS, 42);
        bench_ctre<CTREPat>(cat, name, inputs);
        bench_re2(cat, name, re2pat, inputs);
        bench_pcre2(cat, name, re2pat, inputs);
        bench_hyperscan(cat, name, re2pat, inputs);
        bench_std_regex(cat, name, re2pat, inputs);
    }
}

// Benchmark for large inputs (fewer inputs to avoid OOM, skip std::regex)
template <ctll::fixed_string CTREPat>
void benchmark_pattern_large(const std::string& cat, const std::string& name, const std::string& re2pat,
                             bench::InputGenerator gen, const std::vector<size_t>& sizes) {
    constexpr int LARGE_INPUTS = 50; // Reduced for memory
    for (size_t size : sizes) {
        auto inputs = gen(size, LARGE_INPUTS, 42);
        bench_ctre<CTREPat>(cat, name, inputs);
        bench_re2(cat, name, re2pat, inputs);
        bench_pcre2(cat, name, re2pat, inputs);
        bench_hyperscan(cat, name, re2pat, inputs);
        // Skip std::regex - crashes on large inputs
    }
}

// Check if category should be run
bool should_run(const std::string& cat, const std::string& filter) {
    if (filter.empty() || filter == "all")
        return true;
    // Case-insensitive compare
    std::string cat_lower = cat, filter_lower = filter;
    for (auto& c : cat_lower)
        c = std::tolower(c);
    for (auto& c : filter_lower)
        c = std::tolower(c);
    return cat_lower == filter_lower;
}

int main(int argc, char* argv[]) {
    std::string category_filter = "";
    if (argc > 1) {
        category_filter = argv[1];
        std::cerr << "Running category: " << category_filter << std::endl;
    }

    std::cout << "Pattern,Engine,Input_Size,Time_ns,Matches\n";

    // Simple patterns - full string match (input is entirely digits, letters, etc.)
    if (should_run("Simple", category_filter)) {
        benchmark_pattern<"[0-9]+">("Simple", "digits", "[0-9]+", bench::gen_digits, SIZES);
        benchmark_pattern<"[a-z]+">("Simple", "lowercase", "[a-z]+", bench::gen_letters, SIZES);
        benchmark_pattern<"[A-Z]+">("Simple", "uppercase", "[A-Z]+", bench::gen_upper, SIZES);
        benchmark_pattern<"[aeiou]+">("Simple", "vowels", "[aeiou]+", bench::gen_vowels, SIZES);
        benchmark_pattern<"[a-zA-Z0-9]+">("Simple", "alphanumeric", "[a-zA-Z0-9]+", bench::gen_alnum, SIZES);
    }

    // Complex patterns
    if (should_run("Complex", category_filter)) {
        benchmark_pattern<"[0-9]+\\.[0-9]+">("Complex", "decimal", "[0-9]+\\.[0-9]+", bench::gen_decimal, SIZES);
        benchmark_pattern<"[0-9a-fA-F]+">("Complex", "hex", "[0-9a-fA-F]+", bench::gen_hex, SIZES);
        benchmark_pattern<"[a-zA-Z_][a-zA-Z0-9_]*">("Complex", "identifier", "[a-zA-Z_][a-zA-Z0-9_]*",
                                                    bench::gen_json_key, SIZES);
        benchmark_pattern<"http://[a-z]+">("Complex", "url", "http://[a-z]+", bench::gen_url, SIZES);
        benchmark_pattern<"[a-z]+=[0-9]+">("Complex", "key_value", "[a-z]+=[0-9]+", bench::gen_key_value, SIZES);
        benchmark_pattern<"(GET|POST)/[a-z]+">("Complex", "http_method", "(GET|POST)/[a-z]+", bench::gen_http_method,
                                               SIZES);
        benchmark_pattern<"[a-z]+[0-9]+">("Complex", "letters_digits", "[a-z]+[0-9]+", bench::gen_letters_then_digits,
                                          SIZES);
        benchmark_pattern<"[A-Za-z\\-]+: [a-zA-Z0-9 ]+">("Complex", "http_header", "[A-Za-z\\-]+: [a-zA-Z0-9 ]+",
                                                         bench::gen_http_header_full, SIZES);
        benchmark_pattern<"[0-9]+:[0-9]+:[0-9]+">("Complex", "log_time", "[0-9]+:[0-9]+:[0-9]+",
                                                  bench::gen_log_time_full, SIZES);
    }

    // Scaling - alternation
    if (should_run("Scaling", category_filter)) {
        benchmark_pattern<"(a|b)+">("Scaling", "alt_2", "(a|b)+", bench::gen_ab, SIZES);
        benchmark_pattern<"(a|b|c|d)+">("Scaling", "alt_4", "(a|b|c|d)+", bench::gen_abcd, SIZES);
        benchmark_pattern<"[ab]+">("Scaling", "class_2", "[ab]+", bench::gen_ab, SIZES);
        benchmark_pattern<"[abcd]+">("Scaling", "class_4", "[abcd]+", bench::gen_abcd, SIZES);
        benchmark_pattern<"[a-z]+">("Scaling", "class_26", "[a-z]+", bench::gen_letters, SIZES);
    }

    // Real-world patterns
    if (should_run("RealWorld", category_filter)) {
        benchmark_pattern<"[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+">("RealWorld", "ipv4", "[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+",
                                                               bench::gen_ipv4_full, SIZES);
        benchmark_pattern<"[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+">(
            "RealWorld", "uuid", "[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+", bench::gen_uuid_full, SIZES);
        benchmark_pattern<"[a-z]+@[a-z]+\\.[a-z]+">("RealWorld", "email", "[a-z]+@[a-z]+\\.[a-z]+",
                                                    bench::gen_email_full, SIZES);
        benchmark_pattern<"[0-9]+-[0-9]+-[0-9]+">("RealWorld", "date", "[0-9]+-[0-9]+-[0-9]+", bench::gen_date_full,
                                                  SIZES);
    }

    // Non-matching inputs (pattern won't match the input at all)
    if (should_run("NonMatch", category_filter)) {
        // Use gen_pure_letters (100% letters, no digits) for true non-match
        benchmark_pattern<"[0-9]+">("NonMatch", "digits_on_letters", "[0-9]+", bench::gen_pure_letters, SIZES);
        benchmark_pattern<"[a-z]+">("NonMatch", "letters_on_digits", "[a-z]+", bench::gen_pure_digits, SIZES);
        benchmark_pattern<"http://[a-z]+">("NonMatch", "url_on_digits", "http://[a-z]+", bench::gen_pure_digits, SIZES);

        // DOMINATOR PREFILTER: Patterns with extractable literal that dominates all paths
        // Input lacks the literal "test" → prefilter fast-fails without full regex eval
        benchmark_pattern<"[a-z]+test">("NonMatch", "dom_suffix", "[a-z]+test", bench::gen_no_test_literal, SIZES);
        benchmark_pattern<"test[a-z]+">("NonMatch", "dom_prefix", "test[a-z]+", bench::gen_no_test_literal, SIZES);
        benchmark_pattern<"[a-z]+test[0-9]+">("NonMatch", "dom_middle", "[a-z]+test[0-9]+", bench::gen_no_test_literal,
                                              SIZES);

        // Alternation with dominator: "test" dominates both foo|bar branches
        benchmark_pattern<"(foo|bar)test">("NonMatch", "dom_alt", "(foo|bar)test", bench::gen_no_test_literal, SIZES);

        // REGION PREFILTER: Common suffix across alternation branches
        // "ing" suffix extracted from (running|jumping|walking)
        benchmark_pattern<"(runn|jump|walk)ing">("NonMatch", "region_suffix", "(runn|jump|walk)ing",
                                                 bench::gen_no_ing_suffix, SIZES);

        // URL pattern with "http" literal for prefiltering
        benchmark_pattern<"http://[a-z]+\\.[a-z]+">("NonMatch", "dom_url", "http://[a-z]+\\.[a-z]+",
                                                    bench::gen_no_http_literal, SIZES);
    }

    // Small inputs
    if (should_run("Small", category_filter)) {
        benchmark_pattern<"[0-9]+">("Small", "digits", "[0-9]+", bench::gen_digits, SMALL_SIZES);
    }

    // Large inputs (reduced input count for memory, skip std::regex)
    if (should_run("Large", category_filter)) {
        benchmark_pattern_large<"[0-9]+">("Large", "digits", "[0-9]+", bench::gen_digits, LARGE_SIZES);
    }

    // ========================================================================
    // FALLBACK PATTERNS - Patterns that might seem SIMD-ineligible
    // Demonstrates: single-char backreferences CAN be SIMD-optimized (broadcast+compare)
    // while lazy quantifiers, lookaheads, and group repetitions truly cannot
    // ========================================================================
    if (should_run("Fallback", category_filter)) {
        // Backreference: (.)\1+ matches repeated character (e.g., "aaaa")
        // CAN use SIMD: broadcasts captured char, compares 32 bytes at once
        benchmark_pattern<"(.)\\1+">("Fallback", "backref_repeat", "(.)\\1+", bench::gen_repeated_char, SIZES);

        // Nested backreference: ((.)\\2)+ - single-char backref inside group
        // CAN use SIMD: same broadcast+compare optimization
        benchmark_pattern<"((.)\\2)+">("Fallback", "nested_backref", "((.)\\2)+", bench::gen_repeated_char, SIZES);

        // Lazy quantifier: [a-z]*?x matches shortest string ending in x
        // CANNOT use SIMD: requires backtracking semantics
        benchmark_pattern<"[a-z]*?x">("Fallback", "lazy_star", "[a-z]*?x", bench::gen_lazy_match, SIZES);

        // Lazy plus: [a-z]+?x
        // CANNOT use SIMD: requires backtracking semantics
        benchmark_pattern<"[a-z]+?x">("Fallback", "lazy_plus", "[a-z]+?x", bench::gen_lazy_match, SIZES);

        // Lookahead positive: [a-z](?=[0-9]) letter followed by digit
        // CANNOT use SIMD: position-dependent context evaluation
        benchmark_pattern<"[a-z](?=[0-9])">("Fallback", "lookahead_pos", "[a-z](?=[0-9])", bench::gen_lookahead, SIZES);

        // Lookahead negative: [a-z](?![0-9]) letter NOT followed by digit
        // CANNOT use SIMD: position-dependent context evaluation
        benchmark_pattern<"[a-z](?![0-9])">("Fallback", "lookahead_neg", "[a-z](?![0-9])", bench::gen_letters, SIZES);

        // Capture group repetition: (abc)+ repeated literal group
        // CANNOT use SIMD: must track capture boundaries
        benchmark_pattern<"(abc)+">("Fallback", "group_repeat", "(abc)+", bench::gen_repeated_group, SIZES);
    }

    // ========================================================================
    // INSTANTIATION TIME - How long to compile/create the regex at runtime
    // CTRE does this at compile time, so we show runtime engines only
    // ========================================================================
    if (should_run("Instantiation", category_filter)) {
        constexpr int INST_ITERS = 10000; // Many iterations for accurate timing

        // Patterns of increasing complexity
        const std::vector<std::pair<std::string, std::string>> patterns = {
            {"simple", "[0-9]+"},
            {"identifier", "[a-zA-Z_][a-zA-Z0-9_]*"},
            {"hex", "[0-9a-fA-F]+"},
            {"url", "https?://[a-zA-Z0-9.-]+(/[a-zA-Z0-9._~:/?#@!$&'()*+,;=-]*)?"},
            {"email", "[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}"},
            {"ipv4", "[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}"},
            {"uuid", "[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}"},
            {"log_line",
             "\\[[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[(DEBUG|INFO|WARN|ERROR)\\] .*"},
        };

        for (const auto& [name, pattern] : patterns) {
            std::string anchored = "^" + pattern + "$";

            // RE2
            {
                auto start = std::chrono::high_resolution_clock::now();
                for (int i = 0; i < INST_ITERS; i++) {
                    RE2 re(anchored);
                    if (!re.ok())
                        break;
                }
                auto end = std::chrono::high_resolution_clock::now();
                double ns = std::chrono::duration<double, std::nano>(end - start).count() / INST_ITERS;
                std::cout << "Instantiation/" << name << ",RE2,0," << std::fixed << std::setprecision(2) << ns
                          << ",0\n";
            }

            // PCRE2
            {
                auto start = std::chrono::high_resolution_clock::now();
                for (int i = 0; i < INST_ITERS; i++) {
                    int err;
                    PCRE2_SIZE off;
                    auto* re =
                        pcre2_compile((PCRE2_SPTR)anchored.c_str(), PCRE2_ZERO_TERMINATED, 0, &err, &off, nullptr);
                    if (re)
                        pcre2_code_free(re);
                }
                auto end = std::chrono::high_resolution_clock::now();
                double ns = std::chrono::duration<double, std::nano>(end - start).count() / INST_ITERS;
                std::cout << "Instantiation/" << name << ",PCRE2,0," << std::fixed << std::setprecision(2) << ns
                          << ",0\n";
            }

            // Hyperscan
            {
                auto start = std::chrono::high_resolution_clock::now();
                for (int i = 0; i < INST_ITERS; i++) {
                    hs_database_t* db = nullptr;
                    hs_compile_error_t* error = nullptr;
                    if (hs_compile(anchored.c_str(), HS_FLAG_SINGLEMATCH, HS_MODE_BLOCK, nullptr, &db, &error) ==
                        HS_SUCCESS) {
                        hs_free_database(db);
                    } else if (error) {
                        hs_free_compile_error(error);
                    }
                }
                auto end = std::chrono::high_resolution_clock::now();
                double ns = std::chrono::duration<double, std::nano>(end - start).count() / INST_ITERS;
                std::cout << "Instantiation/" << name << ",Hyperscan,0," << std::fixed << std::setprecision(2) << ns
                          << ",0\n";
            }

            // std::regex
            {
                auto start = std::chrono::high_resolution_clock::now();
                for (int i = 0; i < INST_ITERS; i++) {
                    try {
                        std::regex re(anchored);
                    } catch (...) {
                    }
                }
                auto end = std::chrono::high_resolution_clock::now();
                double ns = std::chrono::duration<double, std::nano>(end - start).count() / INST_ITERS;
                std::cout << "Instantiation/" << name << ",std::regex,0," << std::fixed << std::setprecision(2) << ns
                          << ",0\n";
            }

            // CTRE-SIMD - compile time, so 0ns at runtime
            std::cout << "Instantiation/" << name << ",CTRE-SIMD,0,0.00,0\n";
        }
    }

    return 0;
}
