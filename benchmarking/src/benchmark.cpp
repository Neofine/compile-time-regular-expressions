// Thesis Benchmark - Full String Matching
// All patterns anchored with ^pattern$

#include <chrono>
#include <ctre.hpp>
#include <iomanip>
#include <iostream>
#include <re2/re2.h>
#include <regex>
#include <string>
#include <vector>
#include <cstring>
#define PCRE2_CODE_UNIT_WIDTH 8
#include <hs/hs.h>
#include <pcre2.h>
#include "patterns.hpp"

template<typename T>
__attribute__((always_inline)) inline void do_not_optimize(T&& value) {
    asm volatile("" : : "r,m"(value) : "memory");
}

__attribute__((always_inline)) inline void clobber_memory() {
    asm volatile("" : : : "memory");
}

constexpr int WARMUP = 3;
constexpr int ITERS = 10;
constexpr int INPUTS = 1000;
constexpr int INPUTS_STD = 200;

#ifdef CTRE_ENGINE_NAME
#define CTRE_ENGINE CTRE_ENGINE_NAME
#elif defined(CTRE_DISABLE_SIMD)
#define CTRE_ENGINE "CTRE"
#else
#define CTRE_ENGINE "CTRE-SIMD"
#endif

std::vector<size_t> SIZES = {16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768};
std::vector<size_t> SMALL_SIZES = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
std::vector<size_t> LARGE_SIZES = {32768, 65536, 131072, 262144, 524288, 1048576, 2097152, 4194304, 8388608};

void print_result(const std::string& cat, const std::string& pat, const std::string& eng, 
                  size_t size, double ns, size_t matches) {
    std::cout << cat << "/" << pat << "," << eng << "," << size << "," 
              << std::fixed << std::setprecision(2) << ns << "," << matches << "\n";
}

// RE2 - FullMatch already anchored
void bench_re2(const std::string& cat, const std::string& name, const std::string& pattern,
               const std::vector<std::string>& inputs) {
    RE2 re(pattern);
    if (!re.ok()) return;

    size_t matches = 0;
    for (int i = 0; i < WARMUP; i++)
        for (const auto& s : inputs) { bool r = RE2::FullMatch(s, re); do_not_optimize(r); if (r) matches++; }
    
    clobber_memory();
    matches = 0;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ITERS; i++)
        for (const auto& s : inputs) { bool r = RE2::FullMatch(s, re); do_not_optimize(r); if (r) matches++; }
    clobber_memory();
    auto end = std::chrono::high_resolution_clock::now();
    
    print_result(cat, name, "RE2", inputs[0].size(), 
                 std::chrono::duration<double, std::nano>(end - start).count() / (ITERS * inputs.size()), matches);
}

// PCRE2 with JIT
void bench_pcre2(const std::string& cat, const std::string& name, const std::string& pattern,
                 const std::vector<std::string>& inputs) {
    std::string anchored = "^" + pattern + "$";
    int err; PCRE2_SIZE off;
    auto* re = pcre2_compile((PCRE2_SPTR)anchored.c_str(), PCRE2_ZERO_TERMINATED, 0, &err, &off, nullptr);
    if (!re) return;
    
    pcre2_jit_compile(re, PCRE2_JIT_COMPLETE);
    auto* md = pcre2_match_data_create_from_pattern(re, nullptr);
    auto* jit_stack = pcre2_jit_stack_create(32*1024, 512*1024, nullptr);
    auto* mctx = pcre2_match_context_create(nullptr);
    pcre2_jit_stack_assign(mctx, nullptr, jit_stack);

    size_t matches = 0;
    for (int i = 0; i < WARMUP; i++)
        for (const auto& s : inputs) { int r = pcre2_match(re, (PCRE2_SPTR)s.c_str(), s.size(), 0, 0, md, mctx); do_not_optimize(r); if (r >= 0) matches++; }
    
    clobber_memory();
    matches = 0;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ITERS; i++)
        for (const auto& s : inputs) { int r = pcre2_match(re, (PCRE2_SPTR)s.c_str(), s.size(), 0, 0, md, mctx); do_not_optimize(r); if (r >= 0) matches++; }
    clobber_memory();
    auto end = std::chrono::high_resolution_clock::now();

    print_result(cat, name, "PCRE2", inputs[0].size(),
                 std::chrono::duration<double, std::nano>(end - start).count() / (ITERS * inputs.size()), matches);

    pcre2_match_context_free(mctx);
    pcre2_jit_stack_free(jit_stack);
    pcre2_match_data_free(md);
    pcre2_code_free(re);
}

static int hs_cb(unsigned int, unsigned long long, unsigned long long, unsigned int, void* ctx) {
    (*static_cast<size_t*>(ctx))++;
    return 0;
}

// Hyperscan
void bench_hyperscan(const std::string& cat, const std::string& name, const std::string& pattern,
                     const std::vector<std::string>& inputs) {
    std::string anchored = "^" + pattern + "$";
    hs_database_t* db = nullptr;
    hs_compile_error_t* error = nullptr;
    if (hs_compile(anchored.c_str(), HS_FLAG_SINGLEMATCH, HS_MODE_BLOCK, nullptr, &db, &error) != HS_SUCCESS) {
        if (error) hs_free_compile_error(error);
        return;
    }
    hs_scratch_t* scratch = nullptr;
    hs_alloc_scratch(db, &scratch);

    size_t matches = 0;
    for (int i = 0; i < WARMUP; i++)
        for (const auto& s : inputs) { size_t m = 0; hs_scan(db, s.c_str(), s.size(), 0, scratch, hs_cb, &m); do_not_optimize(m); matches += m; }
    
    clobber_memory();
    matches = 0;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ITERS; i++)
        for (const auto& s : inputs) { size_t m = 0; hs_scan(db, s.c_str(), s.size(), 0, scratch, hs_cb, &m); do_not_optimize(m); matches += m; }
    clobber_memory();
    auto end = std::chrono::high_resolution_clock::now();

    print_result(cat, name, "Hyperscan", inputs[0].size(),
                 std::chrono::duration<double, std::nano>(end - start).count() / (ITERS * inputs.size()), matches);

    hs_free_scratch(scratch);
    hs_free_database(db);
}

// std::regex (slower, uses fewer inputs)
void bench_std_regex(const std::string& cat, const std::string& name, const std::string& pattern,
                     const std::vector<std::string>& inputs) {
    if (!inputs.empty() && inputs[0].size() > 8192) return;

    std::regex re;
    try { re = std::regex("^" + pattern + "$", std::regex::optimize); }
    catch (...) { return; }

    size_t num = std::min(inputs.size(), static_cast<size_t>(INPUTS_STD));
    size_t matches = 0;
    for (int i = 0; i < WARMUP; i++)
        for (size_t j = 0; j < num; j++) { bool r = std::regex_match(inputs[j], re); do_not_optimize(r); if (r) matches++; }
    
    clobber_memory();
    matches = 0;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ITERS; i++)
        for (size_t j = 0; j < num; j++) { bool r = std::regex_match(inputs[j], re); do_not_optimize(r); if (r) matches++; }
    clobber_memory();
    auto end = std::chrono::high_resolution_clock::now();

    print_result(cat, name, "std::regex", inputs[0].size(),
                 std::chrono::duration<double, std::nano>(end - start).count() / (ITERS * num), matches);
}

template <ctll::fixed_string Pattern>
void bench_ctre(const std::string& cat, const std::string& name, const std::vector<std::string>& inputs) {
    size_t matches = 0;
    for (int i = 0; i < WARMUP; i++)
        for (const auto& s : inputs) { auto r = ctre::match<Pattern>(s); do_not_optimize(r); if (r) matches++; }
    
    clobber_memory();
    matches = 0;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ITERS; i++)
        for (const auto& s : inputs) { auto r = ctre::match<Pattern>(s); do_not_optimize(r); if (r) matches++; }
    clobber_memory();
    auto end = std::chrono::high_resolution_clock::now();
    
    print_result(cat, name, CTRE_ENGINE, inputs[0].size(),
                 std::chrono::duration<double, std::nano>(end - start).count() / (ITERS * inputs.size()), matches);
}

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

template <ctll::fixed_string CTREPat>
void benchmark_large(const std::string& cat, const std::string& name, const std::string& re2pat,
                     bench::InputGenerator gen, const std::vector<size_t>& sizes) {
    for (size_t size : sizes) {
        auto inputs = gen(size, 50, 42);
        bench_ctre<CTREPat>(cat, name, inputs);
        bench_re2(cat, name, re2pat, inputs);
        bench_pcre2(cat, name, re2pat, inputs);
        bench_hyperscan(cat, name, re2pat, inputs);
    }
}

bool should_run(const std::string& cat, const std::string& filter) {
    if (filter.empty() || filter == "all") return true;
    std::string c = cat, f = filter;
    for (auto& x : c) x = std::tolower(x);
    for (auto& x : f) x = std::tolower(x);
    return c == f;
}

int main(int argc, char* argv[]) {
    std::string filter = argc > 1 ? argv[1] : "";
    if (!filter.empty()) std::cerr << "Running: " << filter << "\n";
    std::cout << "Pattern,Engine,Input_Size,Time_ns,Matches\n";

    // Simple patterns
    if (should_run("Simple", filter)) {
        benchmark_pattern<"[0-9]+">("Simple", "digits", "[0-9]+", bench::gen_digits, SIZES);
        benchmark_pattern<"[a-z]+">("Simple", "lowercase", "[a-z]+", bench::gen_letters, SIZES);
        benchmark_pattern<"[A-Z]+">("Simple", "uppercase", "[A-Z]+", bench::gen_upper, SIZES);
        benchmark_pattern<"[aeiou]+">("Simple", "vowels", "[aeiou]+", bench::gen_vowels, SIZES);
        benchmark_pattern<"[a-zA-Z0-9]+">("Simple", "alphanumeric", "[a-zA-Z0-9]+", bench::gen_alnum, SIZES);
    }

    // Complex patterns
    if (should_run("Complex", filter)) {
        benchmark_pattern<"[0-9]+\\.[0-9]+">("Complex", "decimal", "[0-9]+\\.[0-9]+", bench::gen_decimal, SIZES);
        benchmark_pattern<"[0-9a-fA-F]+">("Complex", "hex", "[0-9a-fA-F]+", bench::gen_hex, SIZES);
        benchmark_pattern<"[a-zA-Z_][a-zA-Z0-9_]*">("Complex", "identifier", "[a-zA-Z_][a-zA-Z0-9_]*", bench::gen_json_key, SIZES);
        benchmark_pattern<"http://[a-z]+">("Complex", "url", "http://[a-z]+", bench::gen_url, SIZES);
        benchmark_pattern<"[a-z]+=[0-9]+">("Complex", "key_value", "[a-z]+=[0-9]+", bench::gen_key_value, SIZES);
        benchmark_pattern<"(GET|POST)/[a-z]+">("Complex", "http_method", "(GET|POST)/[a-z]+", bench::gen_http_method, SIZES);
        benchmark_pattern<"[a-z]+[0-9]+">("Complex", "letters_digits", "[a-z]+[0-9]+", bench::gen_letters_then_digits, SIZES);
        benchmark_pattern<"[A-Za-z\\-]+: [a-zA-Z0-9 ]+">("Complex", "http_header", "[A-Za-z\\-]+: [a-zA-Z0-9 ]+", bench::gen_http_header_full, SIZES);
        benchmark_pattern<"[0-9]+:[0-9]+:[0-9]+">("Complex", "log_time", "[0-9]+:[0-9]+:[0-9]+", bench::gen_log_time_full, SIZES);
    }

    // Scaling
    if (should_run("Scaling", filter)) {
        benchmark_pattern<"(a|b)+">("Scaling", "alt_2", "(a|b)+", bench::gen_ab, SIZES);
        benchmark_pattern<"(a|b|c|d)+">("Scaling", "alt_4", "(a|b|c|d)+", bench::gen_abcd, SIZES);
        benchmark_pattern<"[ab]+">("Scaling", "class_2", "[ab]+", bench::gen_ab, SIZES);
        benchmark_pattern<"[abcd]+">("Scaling", "class_4", "[abcd]+", bench::gen_abcd, SIZES);
        benchmark_pattern<"[a-z]+">("Scaling", "class_26", "[a-z]+", bench::gen_letters, SIZES);
    }

    // Real-world
    if (should_run("RealWorld", filter)) {
        benchmark_pattern<"[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+">("RealWorld", "ipv4", "[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+", bench::gen_ipv4_full, SIZES);
        benchmark_pattern<"[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+">("RealWorld", "uuid", "[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+-[0-9a-f]+", bench::gen_uuid_full, SIZES);
        benchmark_pattern<"[a-z]+@[a-z]+\\.[a-z]+">("RealWorld", "email", "[a-z]+@[a-z]+\\.[a-z]+", bench::gen_email_full, SIZES);
        benchmark_pattern<"[0-9]+-[0-9]+-[0-9]+">("RealWorld", "date", "[0-9]+-[0-9]+-[0-9]+", bench::gen_date_full, SIZES);
    }

    // Non-matching
    if (should_run("NonMatch", filter)) {
        benchmark_pattern<"[0-9]+">("NonMatch", "digits_on_letters", "[0-9]+", bench::gen_pure_letters, SIZES);
        benchmark_pattern<"[a-z]+">("NonMatch", "letters_on_digits", "[a-z]+", bench::gen_pure_digits, SIZES);
        benchmark_pattern<"http://[a-z]+">("NonMatch", "url_on_digits", "http://[a-z]+", bench::gen_pure_digits, SIZES);
        benchmark_pattern<"[a-z]+test">("NonMatch", "dom_suffix", "[a-z]+test", bench::gen_no_test_literal, SIZES);
        benchmark_pattern<"test[a-z]+">("NonMatch", "dom_prefix", "test[a-z]+", bench::gen_no_test_literal, SIZES);
        benchmark_pattern<"[a-z]+test[0-9]+">("NonMatch", "dom_middle", "[a-z]+test[0-9]+", bench::gen_no_test_literal, SIZES);
        benchmark_pattern<"(foo|bar)test">("NonMatch", "dom_alt", "(foo|bar)test", bench::gen_no_test_literal, SIZES);
        benchmark_pattern<"(runn|jump|walk)ing">("NonMatch", "region_suffix", "(runn|jump|walk)ing", bench::gen_no_ing_suffix, SIZES);
        benchmark_pattern<"http://[a-z]+\\.[a-z]+">("NonMatch", "dom_url", "http://[a-z]+\\.[a-z]+", bench::gen_no_http_literal, SIZES);
    }

    // Small inputs
    if (should_run("Small", filter)) {
        benchmark_pattern<"[0-9]+">("Small", "digits", "[0-9]+", bench::gen_digits, SMALL_SIZES);
    }

    // Large inputs
    if (should_run("Large", filter)) {
        benchmark_large<"[0-9]+">("Large", "digits", "[0-9]+", bench::gen_digits, LARGE_SIZES);
    }

    // Fallback patterns (SIMD ineligible)
    if (should_run("Fallback", filter)) {
        benchmark_pattern<"(.)\\1+">("Fallback", "backref_repeat", "(.)\\1+", bench::gen_repeated_char, SIZES);
        benchmark_pattern<"((.)\\2)+">("Fallback", "nested_backref", "((.)\\2)+", bench::gen_repeated_char, SIZES);
        benchmark_pattern<"[a-z]*?x">("Fallback", "lazy_star", "[a-z]*?x", bench::gen_lazy_match, SIZES);
        benchmark_pattern<"[a-z]+?x">("Fallback", "lazy_plus", "[a-z]+?x", bench::gen_lazy_match, SIZES);
        benchmark_pattern<"[a-z](?=[0-9])">("Fallback", "lookahead_pos", "[a-z](?=[0-9])", bench::gen_lookahead, SIZES);
        benchmark_pattern<"[a-z](?![0-9])">("Fallback", "lookahead_neg", "[a-z](?![0-9])", bench::gen_letters, SIZES);
        benchmark_pattern<"(abc)+">("Fallback", "group_repeat", "(abc)+", bench::gen_repeated_group, SIZES);
    }

    // Adversarial patterns (SIMD unfavorable)
    if (should_run("Adversarial", filter)) {
        // Short literals
        benchmark_pattern<"test">("Adversarial", "literal_4", "test", bench::gen_literal_test, SIZES);
        benchmark_pattern<"hello world">("Adversarial", "literal_11", "hello world", bench::gen_literal_hello_world, SIZES);
        benchmark_pattern<"a">("Adversarial", "single_char", "a", bench::gen_single_a_only, SIZES);
        
        // Bounded/optional
        benchmark_pattern<"[a-z]{2,4}">("Adversarial", "bounded_short", "[a-z]{2,4}", bench::gen_bounded_short, SIZES);
        benchmark_pattern<"a?">("Adversarial", "optional_single", "a?", bench::gen_optional_a, SIZES);
        benchmark_pattern<"a?b?">("Adversarial", "optional_2", "a?b?", bench::gen_optional_ab, SIZES);
        benchmark_pattern<"a?b?c?d?">("Adversarial", "optional_4", "a?b?c?d?", bench::gen_optional_4, SIZES);
        
        // Alternations
        benchmark_pattern<"(cat|dog|bird|fish)">("Adversarial", "alt_words", "(cat|dog|bird|fish)", bench::gen_word_choice, SIZES);
        benchmark_pattern<"(alpha|beta|gamma|delta|epsilon|zeta|eta|theta|iota|kappa)">("Adversarial", "alt_10", "(alpha|...|kappa)", bench::gen_greek_word, SIZES);
        benchmark_pattern<"a|b">("Adversarial", "alt_2_char", "a|b", bench::gen_a_or_b, SIZES);
        benchmark_pattern<"ab|cd">("Adversarial", "alt_2x2", "ab|cd", bench::gen_ab_or_cd, SIZES);
        benchmark_pattern<"(a|b)(c|d)">("Adversarial", "nested_alt", "(a|b)(c|d)", bench::gen_ac_ad_bc_bd, SIZES);
        
        // Mixed/complex
        benchmark_pattern<"id:[0-9]+">("Adversarial", "prefix_range", "id:[0-9]+", bench::gen_prefix_digits, SIZES);
        benchmark_pattern<"(www\\.)?example">("Adversarial", "optional_prefix", "(www\\.)?example", bench::gen_optional_www, SIZES);
        benchmark_pattern<".*middle.*">("Adversarial", "dot_star", ".*middle.*", bench::gen_contains_middle, SIZES);
        benchmark_pattern<"a.b.c.d.e.f.g.h">("Adversarial", "interleaved", "a.b.c.d.e.f.g.h", bench::gen_interleaved, SIZES);
        
        // Nested/captures
        benchmark_pattern<"(((a)))">("Adversarial", "nested_3", "(((a)))", bench::gen_single_a_only, SIZES);
        benchmark_pattern<"((((a))))">("Adversarial", "nested_4", "((((a))))", bench::gen_single_a_only, SIZES);
        benchmark_pattern<"(a)(b)(c)">("Adversarial", "capture_3", "(a)(b)(c)", bench::gen_abc_only, SIZES);
        benchmark_pattern<"(a)(b)(c)(d)(e)">("Adversarial", "capture_5", "(a)(b)(c)(d)(e)", bench::gen_abcde, SIZES);
        
        // Counted/bounded
        benchmark_pattern<"a{20}">("Adversarial", "counted_20", "a{20}", bench::gen_a_20, SIZES);
        benchmark_pattern<"a{2}">("Adversarial", "repeat_2", "a{2}", bench::gen_aa_only, SIZES);
        benchmark_pattern<"a{1,2}">("Adversarial", "bound_1_2", "a{1,2}", bench::gen_a_or_aa, SIZES);
        benchmark_pattern<"[a-z]{1}">("Adversarial", "bounded_1", "[a-z]{1}", bench::gen_single_letter, SIZES);
        
        // Edge cases
        benchmark_pattern<"x*">("Adversarial", "star_empty", "x*", bench::gen_empty_or_x, SIZES);
        benchmark_pattern<"[a]">("Adversarial", "class_single", "[a]", bench::gen_single_a_only, SIZES);
        benchmark_pattern<"\\.">("Adversarial", "escaped_dot", "\\.", bench::gen_dot_only, SIZES);
        benchmark_pattern<".">("Adversarial", "any_char", ".", bench::gen_any_single, SIZES);
        benchmark_pattern<"..">("Adversarial", "any_2", "..", bench::gen_any_two, SIZES);
        benchmark_pattern<"ab?c">("Adversarial", "req_opt_req", "ab?c", bench::gen_ac_or_abc, SIZES);
        benchmark_pattern<"a.b.c">("Adversarial", "dot_sep", "a.b.c", bench::gen_axbxc, SIZES);
        benchmark_pattern<".*a">("Adversarial", "dotstar_a", ".*a", bench::gen_ends_a, SIZES);
        
        // Literals of various lengths
        benchmark_pattern<"ab">("Adversarial", "literal_2", "ab", bench::gen_ab_only, SIZES);
        benchmark_pattern<"abc">("Adversarial", "literal_3", "abc", bench::gen_abc_only, SIZES);
        benchmark_pattern<"hello">("Adversarial", "literal_5", "hello", bench::gen_hello, SIZES);
        benchmark_pattern<"foobar">("Adversarial", "literal_6", "foobar", bench::gen_foobar, SIZES);
        benchmark_pattern<"abcdefgh">("Adversarial", "literal_8", "abcdefgh", bench::gen_8char, SIZES);
        benchmark_pattern<"abcdefghijklmno">("Adversarial", "literal_15", "abcdefghijklmno", bench::gen_15char, SIZES);
        benchmark_pattern<"abcdefghijklmnop">("Adversarial", "literal_16", "abcdefghijklmnop", bench::gen_16char, SIZES);
        benchmark_pattern<"abcdefghijklmnopqrstuvwxyz012345">("Adversarial", "literal_32", "32-char", bench::gen_literal_32, SIZES);
        benchmark_pattern<"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789ab">("Adversarial", "literal_64", "64-char", bench::gen_64char, SIZES);
        benchmark_pattern<"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcd">("Adversarial", "literal_128", "128-char", bench::gen_128char, SIZES);
        benchmark_pattern<"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcd">("Adversarial", "literal_256", "256-char", bench::gen_256char, SIZES);
        
        // 4-char literal investigation
        benchmark_pattern<"abcd">("Adversarial", "lit4_abcd", "abcd", bench::gen_abcd_only, SIZES);
        benchmark_pattern<"aaaa">("Adversarial", "lit4_aaaa", "aaaa", bench::gen_aaaa, SIZES);
        
        // Long alternations
        benchmark_pattern<"(alpha|beta|gamma|delta|epsilon|zeta|eta|theta)">("Adversarial", "alt_8_words", "8-word alt", bench::gen_greek, SIZES);
        benchmark_pattern<"(one|two|three|four|five|six|seven|eight|nine|ten|eleven|twelve)">("Adversarial", "alt_12_words", "12-word alt", bench::gen_numbers, SIZES);
        benchmark_pattern<"(a|b|c|d|e|f|g|h|i|j|k|l|m|n|o|p)">("Adversarial", "alt_16_single", "16-char alt", bench::gen_alt_16, SIZES);
        
        // Complex mixed
        benchmark_pattern<"foo[a-z]{3}bar">("Adversarial", "lit_range_lit", "foo[a-z]{3}bar", bench::gen_foo_xxx_bar, SIZES);
        benchmark_pattern<"start[a-z]{5}end">("Adversarial", "lit_range_lit2", "start[a-z]{5}end", bench::gen_start_range_end, SIZES);
        benchmark_pattern<"prefix_[a-z]{30}_suffix">("Adversarial", "lit_range_lit_big", "lit+range30+lit", bench::gen_lit_range_lit_big, SIZES);
        benchmark_pattern<"[a-z][0-9][a-z][0-9][a-z]">("Adversarial", "multi_class_5", "[a-z][0-9]x5", bench::gen_a1a1a, SIZES);
        benchmark_pattern<"id_[0-9]+_name_[a-z]+">("Adversarial", "complex_id", "id_[0-9]+_name_[a-z]+", bench::gen_id_name, SIZES);
        benchmark_pattern<"[a-z]{5}[0-9]{5}">("Adversarial", "class_5_5", "[a-z]5[0-9]5", bench::gen_class_55, SIZES);
        
        // Long ranges (SIMD favorable - for comparison)
        benchmark_pattern<"[a-z]{64}">("Adversarial", "range_64", "[a-z]{64}", bench::gen_lower64, SIZES);
        benchmark_pattern<"[a-z]{256}">("Adversarial", "range_256", "[a-z]{256}", bench::gen_lower256, SIZES);
        benchmark_pattern<"[0-9]{100}">("Adversarial", "digits_100", "[0-9]{100}", bench::gen_digits100, SIZES);
        benchmark_pattern<"[0-9]{500}">("Adversarial", "digits_500", "[0-9]{500}", bench::gen_digits500, SIZES);
        benchmark_pattern<"[a-zA-Z]{1000}">("Adversarial", "alpha_1000", "[a-zA-Z]{1000}", bench::gen_alpha1000, SIZES);
        
        // More complex
        benchmark_pattern<"([a-z]{10}|[0-9]{10}|[A-Z]{10})">("Adversarial", "alt_types_10", "3-type alt", bench::gen_alt_types, SIZES);
        benchmark_pattern<"[a-z]{20}[0-9]{20}[A-Z]{20}">("Adversarial", "triple_range_20", "3x20 ranges", bench::gen_triple_range, SIZES);
        benchmark_pattern<"(abc|def|ghi|jkl|mno|pqr|stu|vwx|yz0|123|456|789)">("Adversarial", "alt_12x3", "12x3-char alt", bench::gen_12x3, SIZES);
        benchmark_pattern<"[a-zA-Z0-9]{50}">("Adversarial", "alnum_50", "[alnum]{50}", bench::gen_alnum50, SIZES);
        
        // Sequences
        benchmark_pattern<"a.b.c.d.e.f">("Adversarial", "dot_sep_6", "a.b.c.d.e.f", bench::gen_dot_sep_6, SIZES);
        benchmark_pattern<"a.b.c.d.e.f.g.h.i.j">("Adversarial", "dot_sep_10", "a.b...j", bench::gen_dot_sep_10, SIZES);
        benchmark_pattern<"(a|b)(c|d)(e|f)(g|h)">("Adversarial", "alt_seq_4", "(a|b)x4", bench::gen_alt_seq_4, SIZES);
        benchmark_pattern<"(a)(b)(c)(d)(e)(f)(g)(h)">("Adversarial", "capture_8", "8 captures", bench::gen_abcdefgh, SIZES);
        
        // More alternations
        benchmark_pattern<"(aa|bb|cc|dd|ee|ff)">("Adversarial", "alt_6_pairs", "6 pairs", bench::gen_pairs, SIZES);
        benchmark_pattern<"(aa|bb|cc|dd|ee|ff|gg|hh|ii|jj)">("Adversarial", "alt_10_pairs", "10 pairs", bench::gen_10_pairs, SIZES);
        benchmark_pattern<"(foo|bar|baz|qux)">("Adversarial", "alt_4x3", "4x3-char", bench::gen_4x3, SIZES);
        benchmark_pattern<"(the|quick|brown|fox|jumps)">("Adversarial", "alt_5_words", "5 words", bench::gen_5_words, SIZES);
        benchmark_pattern<"(a|bb|ccc|dddd|eeeee)">("Adversarial", "alt_varied", "varied len", bench::gen_varied_alt, SIZES);
        
        // Counted
        benchmark_pattern<"a{10}">("Adversarial", "repeat_10", "a{10}", bench::gen_a10, SIZES);
        benchmark_pattern<"a{50}">("Adversarial", "repeat_50", "a{50}", bench::gen_a50, SIZES);
        benchmark_pattern<"a{100}">("Adversarial", "repeat_100", "a{100}", bench::gen_a100, SIZES);
    }

    // Instantiation time
    if (should_run("Instantiation", filter)) {
        constexpr int INST_ITERS = 10000;
        const std::vector<std::pair<std::string, std::string>> patterns = {
            {"simple", "[0-9]+"},
            {"identifier", "[a-zA-Z_][a-zA-Z0-9_]*"},
            {"hex", "[0-9a-fA-F]+"},
            {"url", "https?://[a-zA-Z0-9.-]+(/[a-zA-Z0-9._~:/?#@!$&'()*+,;=-]*)?"},
            {"email", "[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}"},
            {"ipv4", "[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}"},
            {"uuid", "[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}"},
            {"log_line", "\\[[0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}\\] \\[(DEBUG|INFO|WARN|ERROR)\\] .*"},
        };

        for (const auto& [name, pattern] : patterns) {
            std::string anchored = "^" + pattern + "$";

            // RE2
            auto t0 = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < INST_ITERS; i++) { RE2 re(anchored); do_not_optimize(re.ok()); }
            auto t1 = std::chrono::high_resolution_clock::now();
            std::cout << "Instantiation/" << name << ",RE2,0," << std::fixed << std::setprecision(2) 
                      << std::chrono::duration<double, std::nano>(t1 - t0).count() / INST_ITERS << ",0\n";

            // PCRE2
            t0 = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < INST_ITERS; i++) {
                int err; PCRE2_SIZE off;
                auto* re = pcre2_compile((PCRE2_SPTR)anchored.c_str(), PCRE2_ZERO_TERMINATED, 0, &err, &off, nullptr);
                do_not_optimize(re); if (re) pcre2_code_free(re);
            }
            t1 = std::chrono::high_resolution_clock::now();
            std::cout << "Instantiation/" << name << ",PCRE2,0," << std::fixed << std::setprecision(2)
                      << std::chrono::duration<double, std::nano>(t1 - t0).count() / INST_ITERS << ",0\n";

            // Hyperscan
            t0 = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < INST_ITERS; i++) {
                hs_database_t* db = nullptr; hs_compile_error_t* error = nullptr;
                hs_error_t ret = hs_compile(anchored.c_str(), HS_FLAG_SINGLEMATCH, HS_MODE_BLOCK, nullptr, &db, &error);
                do_not_optimize(ret);
                if (ret == HS_SUCCESS) hs_free_database(db); else if (error) hs_free_compile_error(error);
            }
            t1 = std::chrono::high_resolution_clock::now();
            std::cout << "Instantiation/" << name << ",Hyperscan,0," << std::fixed << std::setprecision(2)
                      << std::chrono::duration<double, std::nano>(t1 - t0).count() / INST_ITERS << ",0\n";

            // std::regex
            t0 = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < INST_ITERS; i++) { try { std::regex re(anchored); do_not_optimize(&re); } catch (...) {} }
            t1 = std::chrono::high_resolution_clock::now();
            std::cout << "Instantiation/" << name << ",std::regex,0," << std::fixed << std::setprecision(2)
                      << std::chrono::duration<double, std::nano>(t1 - t0).count() / INST_ITERS << ",0\n";

            // CTRE - compile time
            std::cout << "Instantiation/" << name << ",CTRE-SIMD,0,0.00,0\n";
        }
    }

    return 0;
}
