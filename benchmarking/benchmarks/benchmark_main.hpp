#pragma once
/**
 * Benchmark Main - CTRE Benchmark Template
 * 
 * This header provides the CTRE-specific benchmark function template.
 * The main benchmark file includes this and instantiates patterns.
 */

#include "benchmark_config.hpp"
#include "benchmark_utils.hpp"
#include "benchmark_engines.hpp"
#include "patterns.hpp"
#include <ctre.hpp>
#include <chrono>
#include <vector>
#include <string>

namespace bench {

using Clock = std::chrono::high_resolution_clock;

// ============================================================================
// CTRE BENCHMARK TEMPLATE
// ============================================================================

template <ctll::fixed_string Pattern>
engines::BenchmarkResult benchmark_ctre(
    const std::string& category,
    const std::string& name,
    const std::vector<std::string>& inputs
) {
    size_t matches = 0;
    
    // Warmup
    for (int i = 0; i < config::WARMUP_ITERS; i++) {
        for (const auto& s : inputs) {
            auto r = ctre::match<Pattern>(s);
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
            auto r = ctre::match<Pattern>(s);
            utils::do_not_optimize(r);
            if (r) matches++;
        }
    }
    utils::clobber_memory();
    auto end = Clock::now();
    
    double ns = std::chrono::duration<double, std::nano>(end - start).count()
              / (config::TIMING_ITERS * inputs.size());
    
    // CTRE runs first - set expected for validation
    engines::g_expected_matches = matches;
    
    return {category, name, config::CTRE_ENGINE, inputs[0].size(), ns, matches, 0};
}

// ============================================================================
// BENCHMARK PATTERN HELPER
// ============================================================================

template <ctll::fixed_string CTREPattern>
void benchmark_pattern(
    const std::string& category,
    const std::string& name,
    const std::string& runtime_pattern,
    bench::InputGenerator generator,
    const std::vector<size_t>& sizes,
    int input_count = config::INPUTS_DEFAULT,
    bool skip_std_regex = false
) {
    for (size_t size : sizes) {
        auto inputs = generator(size, input_count, 42);
        
        // CTRE first (sets expected matches)
        benchmark_ctre<CTREPattern>(category, name, inputs).print();
        
        // Other engines
        engines::RE2Engine::benchmark(category, name, runtime_pattern, inputs).print();
        engines::PCRE2Engine::benchmark(category, name, runtime_pattern, inputs).print();
        engines::HyperscanEngine::benchmark(category, name, runtime_pattern, inputs).print();
        
        if (!skip_std_regex) {
            engines::StdRegexEngine::benchmark(category, name, runtime_pattern, inputs).print();
        }
    }
}

} // namespace bench

