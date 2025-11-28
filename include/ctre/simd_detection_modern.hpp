#ifndef CTRE__SIMD_DETECTION__HPP
#define CTRE__SIMD_DETECTION__HPP

#include <immintrin.h>
#include <concepts>
#include <type_traits>

namespace ctre::simd {

// Compile-time flag to disable SIMD optimizations
#ifndef CTRE_DISABLE_SIMD
#define CTRE_SIMD_ENABLED 1
#else
#define CTRE_SIMD_ENABLED 0
#endif

// C++20: consteval for compile-time only evaluation
[[nodiscard]] consteval bool can_use_simd() noexcept {
    return CTRE_SIMD_ENABLED;
}

// SIMD capability levels (C++20: Use inline constexpr)
inline constexpr int SIMD_CAPABILITY_NONE = 0;
inline constexpr int SIMD_CAPABILITY_SSE42 = 1;
inline constexpr int SIMD_CAPABILITY_AVX2 = 2;
inline constexpr int SIMD_CAPABILITY_AVX512F = 3;

// C++20: Strongly-typed enum for better type safety (optional use)
enum class SimdLevel : int {
    None = SIMD_CAPABILITY_NONE,
    SSE42 = SIMD_CAPABILITY_SSE42,
    AVX2 = SIMD_CAPABILITY_AVX2,
    AVX512F = SIMD_CAPABILITY_AVX512F
};

// SIMD optimization thresholds
inline constexpr std::size_t SIMD_STRING_THRESHOLD = 16;
inline constexpr std::size_t SIMD_REPETITION_THRESHOLD = 32;

// C++20: Concept for iterator types
template<typename T>
concept CharIterator = requires(T it) {
    { *it } -> std::convertible_to<char>;
    { ++it } -> std::same_as<T&>;
    { it + 1 } -> std::convertible_to<T>;
};

// SIMD instruction set detection with caching
[[nodiscard]] inline bool has_avx2() noexcept {
    // Thread-safe initialization via static
    static const bool result = []() noexcept {
        unsigned int eax, ebx, ecx, edx;
        __asm__ __volatile__ ("cpuid" 
            : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx) 
            : "a" (7), "c" (0));
        return (ebx & (1 << 5)) != 0; // AVX2 bit
    }();
    return result;
}

[[nodiscard]] inline bool has_avx512f() noexcept {
    static const bool result = []() noexcept {
        unsigned int eax, ebx, ecx, edx;
        __asm__ __volatile__ ("cpuid" 
            : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx) 
            : "a" (7), "c" (0));
        return (ebx & (1 << 16)) != 0; // AVX-512F bit
    }();
    return result;
}

[[nodiscard]] inline bool has_sse42() noexcept {
    static const bool result = []() noexcept {
        unsigned int eax, ebx, ecx, edx;
        __asm__ __volatile__ ("cpuid" 
            : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx) 
            : "a" (1));
        return (ecx & (1 << 20)) != 0; // SSE4.2 bit
    }();
    return result;
}

// Choose the best available SIMD instruction set
// PERF: Cache the final result to avoid repeated function calls (saves ~25 cycles per call!)
// PERF: Skip AVX512 check - adds overhead and most systems don't have it
[[nodiscard]] inline int get_simd_capability() noexcept {
    if constexpr (CTRE_SIMD_ENABLED) {
        // Cache using immediate lambda initialization (C++17 feature, but cleaner)
        static const int cached_capability = []() noexcept -> int {
            // Cold path: detect once (skip AVX512 for less overhead)
            if (has_avx2()) return 2; // AVX2
            if (has_sse42()) return 1; // SSE4.2
            return 0; // No SIMD
        }();
        
        return cached_capability;
    }
    return 0; // No SIMD
}

// C++20: Strongly-typed version
[[nodiscard]] inline SimdLevel get_simd_level() noexcept {
    return static_cast<SimdLevel>(get_simd_capability());
}

} // namespace ctre::simd

#endif

