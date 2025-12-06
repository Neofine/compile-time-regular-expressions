#ifndef CTRE__SIMD_DETECTION__HPP
#define CTRE__SIMD_DETECTION__HPP

#include <cstddef>

// Architecture detection
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #define CTRE_ARCH_X86 1
    #include <immintrin.h>
#elif defined(__aarch64__) || defined(_M_ARM64) || defined(__arm__) || defined(_M_ARM)
    #define CTRE_ARCH_ARM 1
    #if defined(__ARM_NEON) || defined(__ARM_NEON__)
        #include <arm_neon.h>
        #define CTRE_ARM_NEON 1
    #endif
#endif

namespace ctre::simd {

// Compile-time SIMD control
#ifndef CTRE_DISABLE_SIMD
#define CTRE_SIMD_ENABLED 1
#else
#define CTRE_SIMD_ENABLED 0
#endif

[[nodiscard]] consteval bool can_use_simd() noexcept { return CTRE_SIMD_ENABLED; }

// Runtime CPU feature detection (cached)
#ifdef CTRE_ARCH_X86
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
#else
// ARM or other architectures - no x86 SIMD
[[nodiscard]] inline bool has_avx2() noexcept { return false; }
[[nodiscard]] inline bool has_avx512f() noexcept { return false; }
[[nodiscard]] inline bool has_sse42() noexcept { return false; }
#endif

// ARM NEON detection
#ifdef CTRE_ARM_NEON
[[nodiscard]] inline bool has_neon() noexcept { return true; }
#else
[[nodiscard]] inline bool has_neon() noexcept { return false; }
#endif

// Capability levels
inline constexpr int SIMD_CAPABILITY_NONE = 0;
inline constexpr int SIMD_CAPABILITY_SSE42 = 1;
inline constexpr int SIMD_CAPABILITY_AVX2 = 2;
inline constexpr int SIMD_CAPABILITY_AVX512F = 3;
inline constexpr int SIMD_CAPABILITY_NEON = 4;  // ARM NEON

[[nodiscard]] inline int get_simd_capability() noexcept {
    if constexpr (CTRE_SIMD_ENABLED) {
        static int cached = -1;
        if (cached == -1) [[unlikely]] {
#ifdef CTRE_ARCH_X86
            cached = has_avx2() ? SIMD_CAPABILITY_AVX2 : has_sse42() ? SIMD_CAPABILITY_SSE42 : SIMD_CAPABILITY_NONE;
#elif defined(CTRE_ARM_NEON)
            cached = SIMD_CAPABILITY_NEON;
#else
            cached = SIMD_CAPABILITY_NONE;
#endif
        }
        return cached;
    }
    return SIMD_CAPABILITY_NONE;
}

// Optimization thresholds (bytes)
inline constexpr std::size_t SIMD_STRING_THRESHOLD = 16;
inline constexpr std::size_t SIMD_REPETITION_THRESHOLD = 32;
inline constexpr std::size_t SIMD_SHUFTI_THRESHOLD = 16;
inline constexpr std::size_t SIMD_SEQUENCE_THRESHOLD = 48;

} // namespace ctre::simd

#endif
