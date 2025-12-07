#pragma once
/**
 * Benchmark Engine Wrappers
 * 
 * Provides a uniform interface for benchmarking different regex engines.
 * Each engine implements the same timing methodology for fair comparison.
 */

#include "benchmark_config.hpp"
#include "benchmark_utils.hpp"
#include <chrono>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <regex>

// External library headers
#include <re2/re2.h>
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include <hs/hs.h>

namespace bench::engines {

using Clock = std::chrono::high_resolution_clock;

// ============================================================================
// RESULT OUTPUT
// ============================================================================

struct BenchmarkResult {
    std::string category;
    std::string pattern;
    std::string engine;
    size_t input_size;
    double time_ns;
    size_t matches;
    size_t expected_matches;
    
    void print() const {
        std::cout << category << "/" << pattern << ","
                  << engine << ","
                  << input_size << ","
                  << std::fixed << std::setprecision(config::PRECISION) << time_ns << ","
                  << matches << "\n";
        
        // Validation warning
        if (expected_matches > 0 && matches != expected_matches) {
            double rate = 100.0 * static_cast<double>(matches) / static_cast<double>(expected_matches);
            std::cerr << "WARNING: " << engine << " " << pattern << "@" << input_size 
                      << " matches=" << matches << " expected=" << expected_matches 
                      << " (" << std::fixed << std::setprecision(1) << rate << "%)\n";
        }
    }
};

// Global expected matches (set by CTRE, validated by others)
inline thread_local size_t g_expected_matches = 0;

// ============================================================================
// RE2 ENGINE
// ============================================================================

class RE2Engine {
public:
    static constexpr const char* NAME = "RE2";
    
    static BenchmarkResult benchmark(
        const std::string& category,
        const std::string& name,
        const std::string& pattern,
        const std::vector<std::string>& inputs
    ) {
        // RE2::FullMatch already anchors - no ^$ needed
        RE2 re(pattern);
        if (!re.ok()) {
            return {category, name, NAME, inputs[0].size(), -1.0, 0, 0};
        }
        
        size_t matches = 0;
        
        // Warmup
        for (int i = 0; i < config::WARMUP_ITERS; i++) {
            for (const auto& s : inputs) {
                bool r = RE2::FullMatch(s, re);
                utils::do_not_optimize(r);
                if (r) matches++;
            }
        }
        utils::clobber_memory();
        
        // Timed run
        matches = 0;
        auto start = Clock::now();
        for (int i = 0; i < config::TIMING_ITERS; i++) {
            for (const auto& s : inputs) {
                bool r = RE2::FullMatch(s, re);
                utils::do_not_optimize(r);
                if (r) matches++;
            }
        }
        utils::clobber_memory();
        auto end = Clock::now();
        
        double ns = std::chrono::duration<double, std::nano>(end - start).count() 
                  / (config::TIMING_ITERS * inputs.size());
        
        return {category, name, NAME, inputs[0].size(), ns, matches, g_expected_matches};
    }
};

// ============================================================================
// PCRE2 ENGINE
// ============================================================================

class PCRE2Engine {
public:
    static constexpr const char* NAME = "PCRE2";
    
    static BenchmarkResult benchmark(
        const std::string& category,
        const std::string& name,
        const std::string& pattern,
        const std::vector<std::string>& inputs
    ) {
        std::string anchored = "^" + pattern + "$";
        int err;
        PCRE2_SIZE off;
        auto* re = pcre2_compile(
            reinterpret_cast<PCRE2_SPTR>(anchored.c_str()),
            PCRE2_ZERO_TERMINATED, 0, &err, &off, nullptr
        );
        if (!re) {
            return {category, name, NAME, inputs[0].size(), -1.0, 0, 0};
        }
        
        // Enable JIT for fair comparison
        pcre2_jit_compile(re, PCRE2_JIT_COMPLETE);
        
        auto* md = pcre2_match_data_create_from_pattern(re, nullptr);
        auto* jit_stack = pcre2_jit_stack_create(32*1024, 512*1024, nullptr);
        auto* mctx = pcre2_match_context_create(nullptr);
        pcre2_jit_stack_assign(mctx, nullptr, jit_stack);
        
        size_t matches = 0;
        
        // Warmup
        for (int i = 0; i < config::WARMUP_ITERS; i++) {
            for (const auto& s : inputs) {
                int r = pcre2_match(re, reinterpret_cast<PCRE2_SPTR>(s.c_str()), 
                                    s.size(), 0, 0, md, mctx);
                utils::do_not_optimize(r);
                if (r >= 0) matches++;
            }
        }
        utils::clobber_memory();
        
        // Timed run
        matches = 0;
        auto start = Clock::now();
        for (int i = 0; i < config::TIMING_ITERS; i++) {
            for (const auto& s : inputs) {
                int r = pcre2_match(re, reinterpret_cast<PCRE2_SPTR>(s.c_str()),
                                    s.size(), 0, 0, md, mctx);
                utils::do_not_optimize(r);
                if (r >= 0) matches++;
            }
        }
        utils::clobber_memory();
        auto end = Clock::now();
        
        double ns = std::chrono::duration<double, std::nano>(end - start).count()
                  / (config::TIMING_ITERS * inputs.size());
        
        pcre2_match_context_free(mctx);
        pcre2_jit_stack_free(jit_stack);
        pcre2_match_data_free(md);
        pcre2_code_free(re);
        
        return {category, name, NAME, inputs[0].size(), ns, matches, g_expected_matches};
    }
};

// ============================================================================
// HYPERSCAN ENGINE
// ============================================================================

class HyperscanEngine {
public:
    static constexpr const char* NAME = "Hyperscan";
    
    static BenchmarkResult benchmark(
        const std::string& category,
        const std::string& name,
        const std::string& pattern,
        const std::vector<std::string>& inputs
    ) {
        // Full-string matching with anchors
        std::string anchored = "^" + pattern + "$";
        hs_database_t* db = nullptr;
        hs_compile_error_t* error = nullptr;
        
        if (hs_compile(anchored.c_str(), HS_FLAG_SINGLEMATCH, HS_MODE_BLOCK,
                       nullptr, &db, &error) != HS_SUCCESS) {
            if (error) hs_free_compile_error(error);
            return {category, name, NAME, inputs[0].size(), -1.0, 0, 0};
        }
        
        hs_scratch_t* scratch = nullptr;
        hs_alloc_scratch(db, &scratch);
        
        size_t matches = 0;
        
        // Warmup
        for (int i = 0; i < config::WARMUP_ITERS; i++) {
            for (const auto& s : inputs) {
                size_t m = 0;
                hs_error_t r = hs_scan(db, s.c_str(), static_cast<unsigned>(s.size()),
                                       0, scratch, hs_callback, &m);
                utils::do_not_optimize(r);
                utils::do_not_optimize(m);
                matches += m;
            }
        }
        utils::clobber_memory();
        
        // Timed run
        matches = 0;
        auto start = Clock::now();
        for (int i = 0; i < config::TIMING_ITERS; i++) {
            for (const auto& s : inputs) {
                size_t m = 0;
                hs_error_t r = hs_scan(db, s.c_str(), static_cast<unsigned>(s.size()),
                                       0, scratch, hs_callback, &m);
                utils::do_not_optimize(r);
                utils::do_not_optimize(m);
                matches += m;
            }
        }
        utils::clobber_memory();
        auto end = Clock::now();
        
        double ns = std::chrono::duration<double, std::nano>(end - start).count()
                  / (config::TIMING_ITERS * inputs.size());
        
        hs_free_scratch(scratch);
        hs_free_database(db);
        
        return {category, name, NAME, inputs[0].size(), ns, matches, g_expected_matches};
    }
    
private:
    static int hs_callback(unsigned int, unsigned long long, unsigned long long,
                           unsigned int, void* ctx) {
        (*static_cast<size_t*>(ctx))++;
        return 0;
    }
};

// ============================================================================
// STD::REGEX ENGINE
// ============================================================================

class StdRegexEngine {
public:
    static constexpr const char* NAME = "std::regex";
    
    static BenchmarkResult benchmark(
        const std::string& category,
        const std::string& name,
        const std::string& pattern,
        const std::vector<std::string>& inputs
    ) {
        // Skip large inputs (stack overflow risk)
        if (!inputs.empty() && inputs[0].size() > config::STD_REGEX_MAX_SIZE) {
            return {category, name, NAME, inputs[0].size(), -1.0, 0, 0};
        }
        
        std::regex re;
        try {
            // Use optimize flag for fair comparison
            re = std::regex("^" + pattern + "$", std::regex::optimize);
        } catch (...) {
            return {category, name, NAME, inputs[0].size(), -1.0, 0, 0};
        }
        
        // Use fewer inputs (std::regex is slow)
        size_t num_inputs = std::min(inputs.size(), static_cast<size_t>(config::INPUTS_STD_REGEX));
        
        size_t matches = 0;
        
        // Warmup
        for (int i = 0; i < config::WARMUP_ITERS; i++) {
            for (size_t j = 0; j < num_inputs; j++) {
                bool r = std::regex_match(inputs[j], re);
                utils::do_not_optimize(r);
                if (r) matches++;
            }
        }
        utils::clobber_memory();
        
        // Timed run
        matches = 0;
        auto start = Clock::now();
        for (int i = 0; i < config::TIMING_ITERS; i++) {
            for (size_t j = 0; j < num_inputs; j++) {
                bool r = std::regex_match(inputs[j], re);
                utils::do_not_optimize(r);
                if (r) matches++;
            }
        }
        utils::clobber_memory();
        auto end = Clock::now();
        
        double ns = std::chrono::duration<double, std::nano>(end - start).count()
                  / (config::TIMING_ITERS * num_inputs);
        
        // Scale expected matches for fewer inputs
        size_t expected = g_expected_matches * num_inputs / inputs.size();
        
        return {category, name, NAME, inputs[0].size(), ns, matches, expected};
    }
};

// ============================================================================
// RUN ALL ENGINES
// ============================================================================

inline void run_all_engines(
    const std::string& category,
    const std::string& name,
    const std::string& pattern,
    const std::vector<std::string>& inputs
) {
    // Note: CTRE runs first (in main benchmark file) and sets g_expected_matches
    RE2Engine::benchmark(category, name, pattern, inputs).print();
    PCRE2Engine::benchmark(category, name, pattern, inputs).print();
    HyperscanEngine::benchmark(category, name, pattern, inputs).print();
    StdRegexEngine::benchmark(category, name, pattern, inputs).print();
}

} // namespace bench::engines

