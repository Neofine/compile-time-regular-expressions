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
#include <cstring>

// Prevent compiler from optimizing away results
template<typename T>
__attribute__((always_inline)) inline void do_not_optimize(T&& value) {
    asm volatile("" : : "r,m"(value) : "memory");
}

// Prevent compiler from reordering
__attribute__((always_inline)) inline void clobber_memory() {
    asm volatile("" : : : "memory");
}

// Configuration
constexpr int WARMUP = 3;
constexpr int ITERS = 10;
constexpr int INPUTS = 1000;          // Most engines: 1000 inputs
constexpr int INPUTS_STD_REGEX = 200; // std::regex: 200 inputs (slow)

#ifdef CTRE_ENGINE_NAME
#define CTRE_ENGINE CTRE_ENGINE_NAME
#elif defined(CTRE_DISABLE_SIMD)
#define CTRE_ENGINE "CTRE"
#else
#define CTRE_ENGINE "CTRE-SIMD"
#endif

// Sizes
std::vector<size_t> SIZES = {16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768};
std::vector<size_t> SMALL_SIZES = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
std::vector<size_t> LARGE_SIZES = {32768, 65536, 131072, 262144, 524288, 1048576, 2097152, 4194304, 8388608};

// Global for result validation
thread_local size_t g_expected_matches = 0;

void print_result(const std::string& cat, const std::string& pat, const std::string& eng, size_t size, double ns,
                  size_t matches, size_t expected = 0) {
    std::cout << cat << "/" << pat << "," << eng << "," << size << "," << std::fixed << std::setprecision(2) << ns
              << "," << matches << "\n";
    
    // Validation: warn if match count differs from expected (if set)
    if (expected > 0 && matches != expected) {
        double rate = 100.0 * matches / expected;
        std::cerr << "WARNING: " << eng << " " << pat << "@" << size 
                  << " matches=" << matches << " expected=" << expected 
                  << " (" << std::fixed << std::setprecision(1) << rate << "%)\n";
    }
}

// RE2 benchmark - full match
// Note: RE2::FullMatch already requires entire string to match (no ^$ needed)
void bench_re2(const std::string& cat, const std::string& name, const std::string& pattern,
               const std::vector<std::string>& inputs) {
    RE2 re(pattern);  // No anchors - FullMatch handles this
    if (!re.ok())
        return;

    size_t matches = 0;
    for (int i = 0; i < WARMUP; i++) {
        for (const auto& s : inputs) {
            bool r = RE2::FullMatch(s, re);
            do_not_optimize(r);
            if (r) matches++;
        }
    }
    clobber_memory();
    matches = 0;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ITERS; i++) {
        for (const auto& s : inputs) {
            bool r = RE2::FullMatch(s, re);
            do_not_optimize(r);
            if (r) matches++;
        }
    }
    clobber_memory();
    auto end = std::chrono::high_resolution_clock::now();
    double ns = std::chrono::duration<double, std::nano>(end - start).count() / (ITERS * inputs.size());
    print_result(cat, name, "RE2", inputs[0].size(), ns, matches, g_expected_matches);
}

// PCRE2 benchmark - full match with anchors and JIT
void bench_pcre2(const std::string& cat, const std::string& name, const std::string& pattern,
                 const std::vector<std::string>& inputs) {
    std::string anchored = "^" + pattern + "$";
    int err;
    PCRE2_SIZE off;
    auto* re = pcre2_compile((PCRE2_SPTR)anchored.c_str(), PCRE2_ZERO_TERMINATED, 0, &err, &off, nullptr);
    if (!re)
        return;
    
    // Enable JIT compilation for fair comparison
    pcre2_jit_compile(re, PCRE2_JIT_COMPLETE);
    
    auto* md = pcre2_match_data_create_from_pattern(re, nullptr);
    auto* jit_stack = pcre2_jit_stack_create(32*1024, 512*1024, nullptr);

    // Create match context for JIT
    auto* mctx = pcre2_match_context_create(nullptr);
    pcre2_jit_stack_assign(mctx, nullptr, jit_stack);
    
    size_t matches = 0;
    for (int i = 0; i < WARMUP; i++) {
        for (const auto& s : inputs) {
            int r = pcre2_match(re, (PCRE2_SPTR)s.c_str(), s.size(), 0, 0, md, mctx);
            do_not_optimize(r);
            if (r >= 0) matches++;
        }
    }
    clobber_memory();
    matches = 0;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ITERS; i++) {
        for (const auto& s : inputs) {
            int r = pcre2_match(re, (PCRE2_SPTR)s.c_str(), s.size(), 0, 0, md, mctx);
            do_not_optimize(r);
            if (r >= 0) matches++;
        }
    }
    clobber_memory();
    auto end = std::chrono::high_resolution_clock::now();
    double ns = std::chrono::duration<double, std::nano>(end - start).count() / (ITERS * inputs.size());
    print_result(cat, name, "PCRE2", inputs[0].size(), ns, matches, g_expected_matches);

    pcre2_match_context_free(mctx);
    pcre2_jit_stack_free(jit_stack);
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
    // Full-string matching with ^pattern$ - same semantics as CTRE match()
    // Note: Hyperscan is optimized for streaming/search, not full-string matching
    // The $ anchor adds significant overhead (~20x slower than unanchored)
    std::string anchored = "^" + pattern + "$";
    hs_database_t* db = nullptr;
    hs_compile_error_t* error = nullptr;
    if (hs_compile(anchored.c_str(), HS_FLAG_SINGLEMATCH, HS_MODE_BLOCK, nullptr, &db, &error) !=
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
            hs_error_t r = hs_scan(db, s.c_str(), s.size(), 0, scratch, hs_cb, &m);
            do_not_optimize(r);
            do_not_optimize(m);
            matches += m;
        }
    }
    clobber_memory();
    matches = 0;
    size_t m = 0;  // Moved outside timing loop
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ITERS; i++) {
        for (const auto& s : inputs) {
            m = 0;
            hs_error_t r = hs_scan(db, s.c_str(), s.size(), 0, scratch, hs_cb, &m);
            do_not_optimize(r);
            do_not_optimize(m);
            matches += m;
        }
    }
    clobber_memory();
    auto end = std::chrono::high_resolution_clock::now();
    double ns = std::chrono::duration<double, std::nano>(end - start).count() / (ITERS * inputs.size());
    print_result(cat, name, "Hyperscan", inputs[0].size(), ns, matches, g_expected_matches);

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
        // Use optimize flag for fair comparison
        re = std::regex("^" + pattern + "$", std::regex::optimize);
    } catch (...) {
        return;
    }

    // Use only first INPUTS_STD_REGEX inputs (std::regex is slow)
    size_t num_inputs = std::min(inputs.size(), static_cast<size_t>(INPUTS_STD_REGEX));

    size_t matches = 0;
    for (int i = 0; i < WARMUP; i++) {
        for (size_t j = 0; j < num_inputs; j++) {
            bool r = std::regex_match(inputs[j], re);
            do_not_optimize(r);
            if (r) matches++;
        }
    }
    clobber_memory();
    matches = 0;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ITERS; i++) {
        for (size_t j = 0; j < num_inputs; j++) {
            bool r = std::regex_match(inputs[j], re);
            do_not_optimize(r);
            if (r) matches++;
        }
    }
    clobber_memory();
    auto end = std::chrono::high_resolution_clock::now();
    double ns = std::chrono::duration<double, std::nano>(end - start).count() / (ITERS * num_inputs);
    // std::regex uses fewer inputs, so scale expected matches
    size_t expected_std = g_expected_matches * num_inputs / inputs.size();
    print_result(cat, name, "std::regex", inputs[0].size(), ns, matches, expected_std);
}

// CTRE benchmark template - full match with anchors
// Uses SAME methodology as other engines: iterate over ALL inputs
// Sets g_expected_matches for validation by other engines
template <ctll::fixed_string Pattern>
void bench_ctre(const std::string& cat, const std::string& name, const std::vector<std::string>& inputs) {
    size_t matches = 0;
    
    // Warmup - same as other engines
    for (int i = 0; i < WARMUP; i++) {
        for (const auto& s : inputs) {
            auto r = ctre::match<Pattern>(s);
            do_not_optimize(r);
            if (r) matches++;
        }
    }
    clobber_memory();
    
    matches = 0;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ITERS; i++) {
        for (const auto& s : inputs) {
            auto r = ctre::match<Pattern>(s);
            do_not_optimize(r);
            if (r) matches++;
        }
    }
    clobber_memory();
    auto end = std::chrono::high_resolution_clock::now();
    double ns = std::chrono::duration<double, std::nano>(end - start).count() / (ITERS * inputs.size());
    
    // CTRE runs first - set expected matches for validation
    g_expected_matches = matches;
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
    // ADVERSARIAL PATTERNS - Patterns that are NOT favorable for SIMD
    // Tests patterns where SIMD provides minimal or no benefit to show
    // that the benchmark is not cherry-picking SIMD-friendly patterns
    // ========================================================================
    if (should_run("Adversarial", category_filter)) {
        // Short fixed literal: "test" - only 4 chars, too short for SIMD benefit
        benchmark_pattern<"test">("Adversarial", "literal_4", "test", bench::gen_literal_test, SIZES);

        // Medium fixed literal: "hello world" - 11 chars, still below SIMD threshold
        benchmark_pattern<"hello world">("Adversarial", "literal_11", "hello world", bench::gen_literal_hello_world, SIZES);

        // Single character: just "a" - trivial, no vectorization benefit
        benchmark_pattern<"a">("Adversarial", "single_char", "a", bench::gen_single_a_only, SIZES);

        // Bounded short quantifier: [a-z]{2,4} - small bounds, minimal SIMD benefit
        benchmark_pattern<"[a-z]{2,4}">("Adversarial", "bounded_short", "[a-z]{2,4}", bench::gen_bounded_short, SIZES);

        // Mixed literal + range: "id:[0-9]+" - literal prefix limits SIMD window
        benchmark_pattern<"id:[0-9]+">("Adversarial", "prefix_range", "id:[0-9]+", bench::gen_prefix_digits, SIZES);

        // Long alternation without repetition: (cat|dog|bird|fish)
        benchmark_pattern<"(cat|dog|bird|fish)">("Adversarial", "alt_words", "(cat|dog|bird|fish)", bench::gen_word_choice, SIZES);

        // Optional prefix: (www\\.)?example - branching reduces SIMD effectiveness
        benchmark_pattern<"(www\\.)?example">("Adversarial", "optional_prefix", "(www\\.)?example", bench::gen_optional_www, SIZES);

        // Dot-star sandwich: .*middle.* - greedy matching with any char
        benchmark_pattern<".*middle.*">("Adversarial", "dot_star", ".*middle.*", bench::gen_contains_middle, SIZES);

        // === LONGER ADVERSARIAL PATTERNS ===

        // Long fixed literal: 32 chars - still no SIMD benefit for literal matching
        benchmark_pattern<"abcdefghijklmnopqrstuvwxyz012345">("Adversarial", "literal_32", 
            "abcdefghijklmnopqrstuvwxyz012345", bench::gen_literal_32, SIZES);

        // Interleaved pattern: a.b.c.d - dots interrupt character class runs
        benchmark_pattern<"a.b.c.d.e.f.g.h">("Adversarial", "interleaved", 
            "a.b.c.d.e.f.g.h", bench::gen_interleaved, SIZES);

        // Many alternations: (w1|w2|...|w10) - too many branches for SIMD
        benchmark_pattern<"(alpha|beta|gamma|delta|epsilon|zeta|eta|theta|iota|kappa)">(
            "Adversarial", "alt_10", "(alpha|beta|gamma|delta|epsilon|zeta|eta|theta|iota|kappa)",
            bench::gen_greek_word, SIZES);

        // Counted repetition: a{20} - fixed count, processed sequentially
        benchmark_pattern<"a{20}">("Adversarial", "counted_20", "a{20}", bench::gen_a_20, SIZES);

        // Nested optional groups: (a(b(c)?)?)?d - complex branching
        benchmark_pattern<"(a(b(c)?)?)?d">("Adversarial", "nested_opt", "(a(b(c)?)?)?d", 
            bench::gen_nested_optional, SIZES);

        // Long alternation suffix: prefix(s1|s2|s3|s4|s5) - many branches after literal
        benchmark_pattern<"data_(one|two|three|four|five)">("Adversarial", "prefix_alt", 
            "data_(one|two|three|four|five)", bench::gen_data_suffix, SIZES);

        // === WORST CASE PATTERNS - Hunting for maximum SIMD overhead ===

        // Single optional: a? - trivial pattern
        benchmark_pattern<"a?">("Adversarial", "optional_single", "a?", bench::gen_optional_a, SIZES);

        // Many optionals: a?b?c?d? - 4 optional chars (simpler)
        benchmark_pattern<"a?b?c?d?">("Adversarial", "optional_4", "a?b?c?d?", 
            bench::gen_optional_4, SIZES);

        // Deeply nested groups: (((a))) - 3 levels
        benchmark_pattern<"(((a)))">("Adversarial", "nested_3", "(((a)))", 
            bench::gen_single_a_only, SIZES);

        // Many captures: (a)(b)(c) - 3 capture groups
        benchmark_pattern<"(a)(b)(c)">("Adversarial", "capture_3", "(a)(b)(c)", 
            bench::gen_abc_only, SIZES);

        // Tiny bounded: [a-z]{1} - single char range
        benchmark_pattern<"[a-z]{1}">("Adversarial", "bounded_1", "[a-z]{1}", 
            bench::gen_single_letter, SIZES);

        // Two chars: ab - minimal literal
        benchmark_pattern<"ab">("Adversarial", "literal_2", "ab", bench::gen_ab_only, SIZES);

        // Three chars: abc - still tiny
        benchmark_pattern<"abc">("Adversarial", "literal_3", "abc", bench::gen_abc_only, SIZES);

        // Empty or single: a* matching "" - zero length match
        benchmark_pattern<"x*">("Adversarial", "star_empty", "x*", bench::gen_empty_or_x, SIZES);

        // === EVEN MORE EXTREME PATTERNS ===

        // Two-way alternation: a|b - simplest alternation
        benchmark_pattern<"a|b">("Adversarial", "alt_2_char", "a|b", bench::gen_a_or_b, SIZES);

        // Character class single: [a] - should be same as 'a'
        benchmark_pattern<"[a]">("Adversarial", "class_single", "[a]", bench::gen_single_a_only, SIZES);

        // Escaped dot: \\. - literal dot
        benchmark_pattern<"\\.">("Adversarial", "escaped_dot", "\\.", bench::gen_dot_only, SIZES);

        // Two optionals: a?b? 
        benchmark_pattern<"a?b?">("Adversarial", "optional_2", "a?b?", bench::gen_optional_ab, SIZES);

        // Plus on single: a+ - but very short input
        benchmark_pattern<"a+">("Adversarial", "plus_single", "a+", bench::gen_short_a, SIZES);

        // Literal with escape: a\\.b - a.b
        benchmark_pattern<"a\\.b">("Adversarial", "literal_dot", "a\\.b", bench::gen_a_dot_b, SIZES);

        // Question mark: a? on "a" or ""
        benchmark_pattern<"ab?">("Adversarial", "optional_b", "ab?", bench::gen_a_or_ab, SIZES);

        // Group no capture: (?:ab) - should be same as ab
        benchmark_pattern<"(?:ab)">("Adversarial", "noncap_ab", "(?:ab)", bench::gen_ab_only, SIZES);

        // Empty alternation branch: (a|) - a or empty
        benchmark_pattern<"a|">("Adversarial", "alt_empty", "a|", bench::gen_a_or_empty, SIZES);

        // === ROUND 3: EVEN MORE EDGE CASES ===

        // Just a dot: . - any single char
        benchmark_pattern<".">("Adversarial", "any_char", ".", bench::gen_any_single, SIZES);

        // Two dots: .. - any two chars  
        benchmark_pattern<"..">("Adversarial", "any_2", "..", bench::gen_any_two, SIZES);

        // Sequence of literals: abcd - 4 chars as sequence
        benchmark_pattern<"abcd">("Adversarial", "seq_4", "abcd", bench::gen_abcd_only, SIZES);

        // Very short bounded repeat: a{2}
        benchmark_pattern<"a{2}">("Adversarial", "repeat_2", "a{2}", bench::gen_aa_only, SIZES);

        // Bounded 1-2: a{1,2}
        benchmark_pattern<"a{1,2}">("Adversarial", "bound_1_2", "a{1,2}", bench::gen_a_or_aa, SIZES);

        // Required-optional-required: ab?c
        benchmark_pattern<"ab?c">("Adversarial", "req_opt_req", "ab?c", bench::gen_ac_or_abc, SIZES);

        // Three dots: a.b.c - dots between literals
        benchmark_pattern<"a.b.c">("Adversarial", "dot_sep", "a.b.c", bench::gen_axbxc, SIZES);

        // Leading dot star: .*a - greedy then literal
        benchmark_pattern<".*a">("Adversarial", "dotstar_a", ".*a", bench::gen_ends_a, SIZES);

        // Alternation of 2-char: ab|cd
        benchmark_pattern<"ab|cd">("Adversarial", "alt_2x2", "ab|cd", bench::gen_ab_or_cd, SIZES);

        // Nested alternation: (a|b)(c|d)
        benchmark_pattern<"(a|b)(c|d)">("Adversarial", "nested_alt", "(a|b)(c|d)", bench::gen_ac_ad_bc_bd, SIZES);

        // Five char literal
        benchmark_pattern<"hello">("Adversarial", "literal_5", "hello", bench::gen_hello, SIZES);

        // === ROUND 4: HUNT FOR THE ABSOLUTE WORST ===

        // 3-char literal: "abc"
        benchmark_pattern<"abc">("Adversarial", "literal_3b", "abc", bench::gen_abc_only, SIZES);

        // 6-char literal
        benchmark_pattern<"foobar">("Adversarial", "literal_6", "foobar", bench::gen_foobar, SIZES);

        // 7-char literal  
        benchmark_pattern<"testing">("Adversarial", "literal_7", "testing", bench::gen_testing, SIZES);

        // 8-char literal
        benchmark_pattern<"abcdefgh">("Adversarial", "literal_8", "abcdefgh", bench::gen_8char, SIZES);

        // 15-char literal (just under 16 SIMD threshold)
        benchmark_pattern<"abcdefghijklmno">("Adversarial", "literal_15", "abcdefghijklmno", bench::gen_15char, SIZES);

        // Bounded repeat with literal: test{2}
        benchmark_pattern<"a{2}b">("Adversarial", "repeat_lit", "a{2}b", bench::gen_aab, SIZES);

        // Simple concat: ab
        benchmark_pattern<"ab">("Adversarial", "concat_2", "ab", bench::gen_ab_only, SIZES);

        // Trivial: empty match possible with x*y
        benchmark_pattern<"x*y">("Adversarial", "star_then_lit", "x*y", bench::gen_xy, SIZES);

        // === ROUND 5: BIGGER PATTERNS ===

        // 16-char literal (exactly at SIMD threshold)
        benchmark_pattern<"abcdefghijklmnop">("Adversarial", "literal_16", "abcdefghijklmnop", bench::gen_16char, SIZES);

        // 20-char literal
        benchmark_pattern<"abcdefghijklmnopqrst">("Adversarial", "literal_20", "abcdefghijklmnopqrst", bench::gen_20char, SIZES);

        // 24-char literal  
        benchmark_pattern<"abcdefghijklmnopqrstuvwx">("Adversarial", "literal_24", "abcdefghijklmnopqrstuvwx", bench::gen_24char, SIZES);

        // Long alternation: 8 words
        benchmark_pattern<"(alpha|beta|gamma|delta|epsilon|zeta|eta|theta)">("Adversarial", "alt_8_words", "(alpha|beta|gamma|delta|epsilon|zeta|eta|theta)", bench::gen_greek, SIZES);

        // Multiple optionals in sequence
        benchmark_pattern<"a?b?c?d?e?f?">("Adversarial", "optional_6", "a?b?c?d?e?f?", bench::gen_optional_6, SIZES);

        // Long bounded repeat
        benchmark_pattern<"a{10}">("Adversarial", "repeat_10", "a{10}", bench::gen_a10, SIZES);

        // Sequence of 8 chars
        benchmark_pattern<"abcdefgh">("Adversarial", "seq_8", "abcdefgh", bench::gen_8char, SIZES);

        // Multiple capture groups
        benchmark_pattern<"(a)(b)(c)(d)(e)">("Adversarial", "capture_5", "(a)(b)(c)(d)(e)", bench::gen_abcde, SIZES);

        // Alternation of 2-char words
        benchmark_pattern<"(aa|bb|cc|dd|ee|ff)">("Adversarial", "alt_6_pairs", "(aa|bb|cc|dd|ee|ff)", bench::gen_pairs, SIZES);

        // Nested groups deep
        benchmark_pattern<"((((a))))">("Adversarial", "nested_4", "((((a))))", bench::gen_single_a_only, SIZES);

        // Complex: literal + bounded + literal
        benchmark_pattern<"foo[a-z]{3}bar">("Adversarial", "lit_range_lit", "foo[a-z]{3}bar", bench::gen_foo_xxx_bar, SIZES);

        // Multiple dots: longer
        benchmark_pattern<"a.b.c.d.e.f">("Adversarial", "dot_sep_6", "a.b.c.d.e.f", bench::gen_dot_sep_6, SIZES);

        // Alternation of 3-char 
        benchmark_pattern<"(foo|bar|baz|qux)">("Adversarial", "alt_4x3", "(foo|bar|baz|qux)", bench::gen_4x3, SIZES);

        // Long counted
        benchmark_pattern<"a{50}">("Adversarial", "repeat_50", "a{50}", bench::gen_a50, SIZES);

        // Very long literal (28 chars)
        benchmark_pattern<"abcdefghijklmnopqrstuvwxyzab">("Adversarial", "literal_28", "abcdefghijklmnopqrstuvwxyzab", bench::gen_28char, SIZES);

        // === ROUND 6: BIG PATTERNS (simplified for compiler) ===

        // 64-char literal
        benchmark_pattern<"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789ab">("Adversarial", "literal_64", "64-char literal", bench::gen_64char, SIZES);

        // Long alternation: 12 words
        benchmark_pattern<"(one|two|three|four|five|six|seven|eight|nine|ten|eleven|twelve)">("Adversarial", "alt_12_words", "alt 12 words", bench::gen_numbers, SIZES);

        // Multiple character classes in sequence
        benchmark_pattern<"[a-z][0-9][a-z][0-9][a-z]">("Adversarial", "multi_class_5", "[a-z][0-9][a-z][0-9][a-z]", bench::gen_a1a1a, SIZES);

        // Complex: multiple ranges with literals
        benchmark_pattern<"id_[0-9]+_name_[a-z]+">("Adversarial", "complex_id", "id_[0-9]+_name_[a-z]+", bench::gen_id_name, SIZES);

        // Very long alternation of pairs
        benchmark_pattern<"(aa|bb|cc|dd|ee|ff|gg|hh|ii|jj)">("Adversarial", "alt_10_pairs", "10 pairs alt", bench::gen_10_pairs, SIZES);

        // Nested groups 5 deep
        benchmark_pattern<"(((((a)))))">("Adversarial", "nested_5", "(((((a)))))", bench::gen_single_a_only, SIZES);

        // Long sequence of dots
        benchmark_pattern<"a.b.c.d.e.f.g.h.i.j">("Adversarial", "dot_sep_10", "a.b.c.d.e.f.g.h.i.j", bench::gen_dot_sep_10, SIZES);

        // Mixed: literal + range + literal
        benchmark_pattern<"start[a-z]{5}end">("Adversarial", "lit_range_lit2", "start[a-z]{5}end", bench::gen_start_range_end, SIZES);

        // Huge alternation of singles
        benchmark_pattern<"(a|b|c|d|e|f|g|h|i|j|k|l|m|n|o|p)">("Adversarial", "alt_16_single", "16 single alt", bench::gen_alt_16, SIZES);

        // Character class sequence: [a-z]{5}[0-9]{5}
        benchmark_pattern<"[a-z]{5}[0-9]{5}">("Adversarial", "class_5_5", "[a-z]5[0-9]5", bench::gen_class_55, SIZES);

        // 100-char bounded repeat
        benchmark_pattern<"a{100}">("Adversarial", "repeat_100", "a{100}", bench::gen_a100, SIZES);

        // === ROUND 7: BIG NON-SIMD-ABLE PATTERNS ===
        // These are long/complex but have NO character class repetitions

        // Long sequence of literals (no repeats, just abcdefghij...)
        benchmark_pattern<"abcdefghijklmno">("Adversarial", "seq_15", "abcdefghijklmno", bench::gen_15char, SIZES);

        // Many small alternations in sequence: (a|b)(c|d)(e|f)(g|h)
        benchmark_pattern<"(a|b)(c|d)(e|f)(g|h)">("Adversarial", "alt_seq_4", "(a|b)(c|d)(e|f)(g|h)", bench::gen_alt_seq_4, SIZES);

        // Long chain of optionals with literals between
        benchmark_pattern<"a.b.c.d.e.f.g.h.i.j.k.l">("Adversarial", "dot_chain_12", "a.b.c...l", bench::gen_dot_chain_12, SIZES);

        // Multiple captures without repeats
        benchmark_pattern<"(a)(b)(c)(d)(e)(f)(g)(h)">("Adversarial", "capture_8", "(a)(b)...(h)", bench::gen_abcdefgh, SIZES);

        // Alternation of different-length words (no common structure)
        benchmark_pattern<"(a|bb|ccc|dddd|eeeee)">("Adversarial", "alt_varied", "varied length alt", bench::gen_varied_alt, SIZES);

        // Nested alternations: ((a|b)|(c|d))
        benchmark_pattern<"((a|b)|(c|d))">("Adversarial", "nested_alt_2", "((a|b)|(c|d))", bench::gen_ac_ad_bc_bd, SIZES);

        // Sequence of bounded singles: a{1}b{1}c{1}d{1}...
        benchmark_pattern<"a{1}b{1}c{1}d{1}e{1}f{1}">("Adversarial", "bounded_seq_6", "a{1}b{1}...f{1}", bench::gen_abcdef, SIZES);

        // Mixed short literals and dots: ab.cd.ef.gh
        benchmark_pattern<"ab.cd.ef.gh">("Adversarial", "lit_dot_lit", "ab.cd.ef.gh", bench::gen_lit_dot_lit, SIZES);

        // Long word alternation (all different structure)
        benchmark_pattern<"(the|quick|brown|fox|jumps)">("Adversarial", "alt_5_words", "5 english words", bench::gen_5_words, SIZES);

        // Complex nested: (a(b(c(d))))
        benchmark_pattern<"(a(b(c(d))))">("Adversarial", "nested_capture_4", "(a(b(c(d))))", bench::gen_abcd_only, SIZES);

        // === INVESTIGATING 4-CHAR ANOMALY ===
        // Multiple 4-char literals to see if it's all of them or just "test"
        benchmark_pattern<"abcd">("Adversarial", "lit4_abcd", "abcd", bench::gen_abcd_only, SIZES);
        benchmark_pattern<"wxyz">("Adversarial", "lit4_wxyz", "wxyz", bench::gen_wxyz, SIZES);
        benchmark_pattern<"1234">("Adversarial", "lit4_1234", "1234", bench::gen_1234, SIZES);
        benchmark_pattern<"best">("Adversarial", "lit4_best", "best", bench::gen_best, SIZES);
        benchmark_pattern<"fest">("Adversarial", "lit4_fest", "fest", bench::gen_fest, SIZES);
        benchmark_pattern<"rest">("Adversarial", "lit4_rest", "rest", bench::gen_rest, SIZES);
        // Repeated char tests
        benchmark_pattern<"abab">("Adversarial", "lit4_abab", "abab", bench::gen_abab, SIZES);
        benchmark_pattern<"aaaa">("Adversarial", "lit4_aaaa", "aaaa", bench::gen_aaaa, SIZES);
        benchmark_pattern<"aabb">("Adversarial", "lit4_aabb", "aabb", bench::gen_aabb, SIZES);

        // === MONSTER PATTERNS ===
        
        // 128-char literal
        benchmark_pattern<"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcd">("Adversarial", "literal_128", "128-char", bench::gen_128char, SIZES);

        // Long bounded repeat
        benchmark_pattern<"[a-z]{64}">("Adversarial", "range_64", "[a-z]{64}", bench::gen_lower64, SIZES);

        // Long digit sequence
        benchmark_pattern<"[0-9]{100}">("Adversarial", "digits_100", "[0-9]{100}", bench::gen_digits100, SIZES);

        // Complex alternation of different types
        benchmark_pattern<"([a-z]{10}|[0-9]{10}|[A-Z]{10})">("Adversarial", "alt_types_10", "3-type alt x10", bench::gen_alt_types, SIZES);

        // Multiple ranges in sequence
        benchmark_pattern<"[a-z]{20}[0-9]{20}[A-Z]{20}">("Adversarial", "triple_range_20", "3 ranges x20", bench::gen_triple_range, SIZES);

        // Very long alternation
        benchmark_pattern<"(abc|def|ghi|jkl|mno|pqr|stu|vwx|yz0|123|456|789)">("Adversarial", "alt_12x3", "12 x 3-char alt", bench::gen_12x3, SIZES);

        // Deep character class repetition
        benchmark_pattern<"[a-zA-Z0-9]{50}">("Adversarial", "alnum_50", "[alnum]{50}", bench::gen_alnum50, SIZES);

        // Mixed: long literal + long range
        benchmark_pattern<"prefix_[a-z]{30}_suffix">("Adversarial", "lit_range_lit_big", "lit+range30+lit", bench::gen_lit_range_lit_big, SIZES);

        // === TRULY MASSIVE PATTERNS (256+ chars) ===
        
        // 256-char literal
        benchmark_pattern<"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcd">("Adversarial", "literal_256", "256-char", bench::gen_256char, SIZES);

        // 256-char range
        benchmark_pattern<"[a-z]{256}">("Adversarial", "range_256", "[a-z]{256}", bench::gen_lower256, SIZES);

        // 500-char range  
        benchmark_pattern<"[0-9]{500}">("Adversarial", "digits_500", "[0-9]{500}", bench::gen_digits500, SIZES);

        // 1000-char range
        benchmark_pattern<"[a-zA-Z]{1000}">("Adversarial", "alpha_1000", "[a-zA-Z]{1000}", bench::gen_alpha1000, SIZES);

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
                    do_not_optimize(re.ok());
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
                    do_not_optimize(re);
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
                    hs_error_t ret = hs_compile(anchored.c_str(), HS_FLAG_SINGLEMATCH, HS_MODE_BLOCK, nullptr, &db, &error);
                    do_not_optimize(ret);
                    if (ret == HS_SUCCESS) {
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
                        do_not_optimize(&re);
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
