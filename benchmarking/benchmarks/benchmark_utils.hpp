#pragma once
/**
 * Benchmark Utilities
 * 
 * Common utilities for preventing compiler optimizations during benchmarking.
 */

#include <string>
#include <algorithm>
#include <cctype>

namespace bench::utils {

// ============================================================================
// COMPILER BARRIERS
// ============================================================================

// Prevent compiler from optimizing away a value
template<typename T>
__attribute__((always_inline)) inline void do_not_optimize(T&& value) {
    asm volatile("" : : "r,m"(value) : "memory");
}

// Prevent compiler from reordering memory operations
__attribute__((always_inline)) inline void clobber_memory() {
    asm volatile("" : : : "memory");
}

// ============================================================================
// STRING UTILITIES
// ============================================================================

inline std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

inline bool iequals(const std::string& a, const std::string& b) {
    return to_lower(a) == to_lower(b);
}

// ============================================================================
// CATEGORY FILTERING
// ============================================================================

inline bool should_run_category(const std::string& category, const std::string& filter) {
    if (filter.empty() || filter == "all") {
        return true;
    }
    return iequals(category, filter);
}

} // namespace bench::utils


