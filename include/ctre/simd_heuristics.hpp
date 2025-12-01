#ifndef CTRE__SIMD_HEURISTICS__HPP
#define CTRE__SIMD_HEURISTICS__HPP

#include <cstddef>

namespace ctre {
namespace simd {

// ============================================================================
// COMPILE-TIME SIMD SUITABILITY ANALYSIS
// ============================================================================

/// Analyze if a pattern is suitable for SIMD optimization
/// Returns false if SIMD will likely cause regression
template <typename Pattern>
struct simd_suitability {
    // Default: assume SIMD is beneficial
    static constexpr bool is_suitable = true;
    static constexpr size_t min_beneficial_size = 32;
};

// Detect patterns with multiple short segments (like IPv4: [0-9]+\.[0-9]+\.[0-9]+\.[0-9]+)
// These have high overhead from repeatedly entering/exiting SIMD
template <typename... Content>
constexpr size_t count_segments(sequence<Content...>*) {
    return sizeof...(Content);
}

template <typename Pattern>
constexpr size_t pattern_segment_count() {
    if constexpr (requires { count_segments((Pattern*)nullptr); }) {
        return count_segments((Pattern*)nullptr);
    }
    return 1;
}

// For patterns with many segments, require larger input to amortize overhead
template <typename Pattern>
constexpr size_t get_min_beneficial_size() {
    constexpr size_t segments = pattern_segment_count<Pattern>();
    
    // More segments = need larger input to benefit
    if constexpr (segments >= 5) {
        return 128;  // IPv4-like: 4+ segments
    } else if constexpr (segments >= 3) {
        return 64;   // MAC-like: 3-4 segments
    } else {
        return 32;   // Simple repetitions
    }
}

} // namespace simd
} // namespace ctre

#endif
