#pragma once
// ============================================================================
// BENCHMARK RUNNER - Common infrastructure for all engines
// ============================================================================

#include "patterns.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <random>
#include <regex>
#include <string>
#include <string_view>
#include <vector>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <hs/hs.h>
#include <pcre2.h>
#include <re2/re2.h>

namespace bench {

using namespace std::chrono;

// ============================================================================
// DoNotOptimize - Prevents compiler from optimizing away benchmark code
// From Google Benchmark
// ============================================================================

template <class Tp>
inline void DoNotOptimize(Tp const& value) {
    asm volatile("" : : "r,m"(value) : "memory");
}

template <class Tp>
inline void DoNotOptimize(Tp& value) {
#if defined(__clang__)
    asm volatile("" : "+r,m"(value) : : "memory");
#else
    asm volatile("" : "+m,r"(value) : : "memory");
#endif
}

// ============================================================================
// CONFIGURATION
// ============================================================================

struct BenchConfig {
    size_t buffer_size_mb = 1024;  // 1GB buffer for no-cache benchmark
    std::vector<int> input_sizes = {16, 64, 256, 1024, 4096, 16384};
    bool verbose = true;
};

// ============================================================================
// UTILITIES
// ============================================================================

inline unsigned int get_runtime_seed() {
    unsigned int seed;
    std::ifstream urandom("/dev/urandom", std::ios::in | std::ios::binary);
    if (urandom.read(reinterpret_cast<char*>(&seed), sizeof(seed))) {
        return seed;
    }
    // Fallback
    return static_cast<unsigned int>(
        high_resolution_clock::now().time_since_epoch().count());
}

// ============================================================================
// BENCHMARK RESULT
// ============================================================================

struct BenchResult {
    std::string pattern;
    std::string engine;
    int input_size;
    double time_ns;
    int match_count;
    int expected_matches;

    double throughput_mbs() const {
        return input_size / (time_ns * 1e-9) / (1024.0 * 1024.0);
    }

    bool matches_correct() const { return match_count == expected_matches; }
};

// ============================================================================
// ENGINE BENCHMARKS
// ============================================================================

// RE2 benchmark
inline BenchResult bench_re2(const std::string& pattern_name, const std::string& re2_pattern,
                             char* buffer, size_t str_len, const std::vector<size_t>& order) {
    RE2 re("^" + re2_pattern + "$");
    int mc = 0;
    size_t num_strings = order.size();

    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < num_strings; i++) {
        char* entry = buffer + order[i] * str_len;
        DoNotOptimize(entry);
        bool result = RE2::FullMatch(std::string_view(entry, str_len), re);
        DoNotOptimize(result);
        if (result) mc++;
    }
    auto end = high_resolution_clock::now();

    return {pattern_name, "RE2", static_cast<int>(str_len),
            duration_cast<nanoseconds>(end - start).count() / (double)num_strings, mc,
            static_cast<int>(num_strings / 2)};
}

// PCRE2 benchmark
inline BenchResult bench_pcre2(const std::string& pattern_name, const std::string& pcre2_pattern,
                               char* buffer, size_t str_len, const std::vector<size_t>& order) {
    std::string p = "^" + pcre2_pattern + "$";
    int err;
    PCRE2_SIZE errofs;
    pcre2_code* re = pcre2_compile((PCRE2_SPTR)p.c_str(), PCRE2_ZERO_TERMINATED, 0, &err, &errofs, nullptr);
    pcre2_match_data* md = pcre2_match_data_create_from_pattern(re, nullptr);

    int mc = 0;
    size_t num_strings = order.size();

    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < num_strings; i++) {
        char* entry = buffer + order[i] * str_len;
        DoNotOptimize(entry);
        int result = pcre2_match(re, (PCRE2_SPTR)entry, str_len, 0, 0, md, nullptr);
        DoNotOptimize(result);
        if (result >= 0) mc++;
    }
    auto end = high_resolution_clock::now();

    pcre2_match_data_free(md);
    pcre2_code_free(re);

    return {pattern_name, "PCRE2", static_cast<int>(str_len),
            duration_cast<nanoseconds>(end - start).count() / (double)num_strings, mc,
            static_cast<int>(num_strings / 2)};
}

// Hyperscan benchmark
inline BenchResult bench_hyperscan(const std::string& pattern_name, const std::string& hs_pattern,
                                   char* buffer, size_t str_len, const std::vector<size_t>& order) {
    std::string p = "^" + hs_pattern + "$";
    hs_database_t* db = nullptr;
    hs_compile_error_t* ce;

    if (hs_compile(p.c_str(), HS_FLAG_DOTALL, HS_MODE_BLOCK, nullptr, &db, &ce) != HS_SUCCESS) {
        if (ce) {
            std::cerr << "Hyperscan compile failed for " << pattern_name << ": " << ce->message << "\n";
            hs_free_compile_error(ce);
        }
        return {pattern_name, "Hyperscan", static_cast<int>(str_len), -1, -1, -1};
    }

    hs_scratch_t* sc = nullptr;
    if (hs_alloc_scratch(db, &sc) != HS_SUCCESS) {
        hs_free_database(db);
        return {pattern_name, "Hyperscan", static_cast<int>(str_len), -1, -1, -1};
    }

    auto cb = [](unsigned, unsigned long long, unsigned long long, unsigned, void* c) -> int {
        *(int*)c = 1;
        return 0;
    };

    int mc = 0;
    size_t num_strings = order.size();

    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < num_strings; i++) {
        char* entry = buffer + order[i] * str_len;
        DoNotOptimize(entry);
        int matched = 0;
        hs_scan(db, entry, str_len, 0, sc, cb, &matched);
        DoNotOptimize(matched);
        if (matched) mc++;
    }
    auto end = high_resolution_clock::now();

    hs_free_scratch(sc);
    hs_free_database(db);

    return {pattern_name, "Hyperscan", static_cast<int>(str_len),
            duration_cast<nanoseconds>(end - start).count() / (double)num_strings, mc,
            static_cast<int>(num_strings / 2)};
}

// std::regex benchmark
inline BenchResult bench_std_regex(const std::string& pattern_name, const std::string& std_pattern,
                                   char* buffer, size_t str_len, const std::vector<size_t>& order) {
    std::regex re("^" + std_pattern + "$", std::regex::optimize);
    int mc = 0;
    size_t num_strings = order.size();

    auto start = high_resolution_clock::now();
    for (size_t i = 0; i < num_strings; i++) {
        char* entry = buffer + order[i] * str_len;
        DoNotOptimize(entry);
        bool result = std::regex_match(entry, entry + str_len, re);
        DoNotOptimize(result);
        if (result) mc++;
    }
    auto end = high_resolution_clock::now();

    return {pattern_name, "std::regex", static_cast<int>(str_len),
            duration_cast<nanoseconds>(end - start).count() / (double)num_strings, mc,
            static_cast<int>(num_strings / 2)};
}

// ============================================================================
// OUTPUT UTILITIES
// ============================================================================

inline void print_csv_header(std::ostream& out) { out << "Pattern,Engine,Input_Size,Time_ns\n"; }

inline void print_result_csv(std::ostream& out, const BenchResult& r) {
    out << r.pattern << "," << r.engine << "," << r.input_size << "," << r.time_ns << "\n";
}

inline void print_result_human(std::ostream& out, const BenchResult& r) {
    out << "  " << r.engine << ": " << r.time_ns << " ns (" << r.throughput_mbs() << " MB/s)";
    if (!r.matches_correct()) {
        out << " [WARN: " << r.match_count << "/" << r.expected_matches << " matches]";
    }
    out << "\n";
}

} // namespace bench
