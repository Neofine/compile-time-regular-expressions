#pragma once
/**
 * Benchmark Configuration
 * 
 * Centralized configuration for benchmark parameters.
 * Modify these to adjust benchmark behavior without touching benchmark code.
 */

#include <cstddef>
#include <vector>
#include <string>

namespace bench::config {

// ============================================================================
// ITERATION COUNTS
// ============================================================================

inline constexpr int WARMUP_ITERS = 3;          // Warmup iterations (discarded)
inline constexpr int TIMING_ITERS = 10;         // Timed iterations
inline constexpr int INPUTS_DEFAULT = 1000;     // Default input count
inline constexpr int INPUTS_STD_REGEX = 200;    // std::regex is slow
inline constexpr int INPUTS_LARGE = 50;         // Large inputs (memory)
inline constexpr int INSTANTIATION_ITERS = 10000; // Regex compilation timing

// ============================================================================
// INPUT SIZE PRESETS
// ============================================================================

inline std::vector<size_t> sizes_standard() {
    return {16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768};
}

inline std::vector<size_t> sizes_small() {
    return {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
}

inline std::vector<size_t> sizes_large() {
    return {32768, 65536, 131072, 262144, 524288, 1048576, 2097152, 4194304, 8388608};
}

// ============================================================================
// ENGINE CONFIGURATION
// ============================================================================

// Maximum input size for std::regex (avoids stack overflow)
inline constexpr size_t STD_REGEX_MAX_SIZE = 8192;

// CTRE engine name (set by build system)
#ifdef CTRE_ENGINE_NAME
inline constexpr const char* CTRE_ENGINE = CTRE_ENGINE_NAME;
#elif defined(CTRE_DISABLE_SIMD)
inline constexpr const char* CTRE_ENGINE = "CTRE";
#else
inline constexpr const char* CTRE_ENGINE = "CTRE-SIMD";
#endif

// ============================================================================
// OUTPUT FORMAT
// ============================================================================

inline constexpr const char* CSV_HEADER = "Pattern,Engine,Input_Size,Time_ns,Matches";
inline constexpr int PRECISION = 2;  // Decimal places for timing

} // namespace bench::config


