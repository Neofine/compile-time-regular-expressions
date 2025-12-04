#ifndef CTRE__SIMD_DETECTION__HPP
#define CTRE__SIMD_DETECTION__HPP

#include <cstddef>
#include <immintrin.h>

namespace ctre::simd {

// Compile-time SIMD control
#ifndef CTRE_DISABLE_SIMD
#define CTRE_SIMD_ENABLED 1
#else
#define CTRE_SIMD_ENABLED 0
#endif

[[nodiscard]] consteval bool can_use_simd() noexcept { return CTRE_SIMD_ENABLED; }

// Runtime CPU feature detection (cached)
[[nodiscard]] inline bool has_avx2() noexcept {
    static bool detected = false, result = false;
    if (!detected) {
        unsigned int eax, ebx, ecx, edx;
        __asm__ __volatile__("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
        result = (ebx & (1 << 5)) != 0;
        detected = true;
    }
    return result;
}

[[nodiscard]] inline bool has_avx512f() noexcept {
    static bool detected = false, result = false;
    if (!detected) {
        unsigned int eax, ebx, ecx, edx;
        __asm__ __volatile__("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
        result = (ebx & (1 << 16)) != 0;
        detected = true;
    }
    return result;
}

[[nodiscard]] inline bool has_sse42() noexcept {
    static bool detected = false, result = false;
    if (!detected) {
        unsigned int eax, ebx, ecx, edx;
        __asm__ __volatile__("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
        result = (ecx & (1 << 20)) != 0;
        detected = true;
    }
    return result;
}

// Capability levels
inline constexpr int SIMD_CAPABILITY_NONE = 0;
inline constexpr int SIMD_CAPABILITY_SSE42 = 1;
inline constexpr int SIMD_CAPABILITY_AVX2 = 2;
inline constexpr int SIMD_CAPABILITY_AVX512F = 3;

[[nodiscard]] inline int get_simd_capability() noexcept {
    if constexpr (CTRE_SIMD_ENABLED) {
        static int cached = -1;
        if (cached == -1) [[unlikely]] {
            cached = has_avx2() ? 2 : has_sse42() ? 1 : 0;
        }
        return cached;
    }
    return 0;
}

// Optimization thresholds (bytes)
inline constexpr std::size_t SIMD_STRING_THRESHOLD = 16;
inline constexpr std::size_t SIMD_REPETITION_THRESHOLD = 32;
inline constexpr std::size_t SIMD_SHUFTI_THRESHOLD = 16;
inline constexpr std::size_t SIMD_SEQUENCE_THRESHOLD = 48;

} // namespace ctre::simd

#endif
