#ifndef CTRE__SIMD_DETECTION__HPP
#define CTRE__SIMD_DETECTION__HPP

#include <immintrin.h>
#include <type_traits>

namespace ctre {
namespace simd {

#ifndef CTRE_DISABLE_SIMD
#define CTRE_SIMD_ENABLED 1
#else
#define CTRE_SIMD_ENABLED 0
#endif

constexpr bool can_use_simd() {
    return CTRE_SIMD_ENABLED;
}

inline bool has_avx2() {
    static bool detected = false;
    static bool result = false;

    if (!detected) {
        unsigned int eax, ebx, ecx, edx;
        __asm__ __volatile__("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
        result = (ebx & (1 << 5)) != 0; // AVX2 bit
        detected = true;
    }

    return result;
}

inline bool has_avx512f() {
    static bool detected = false;
    static bool result = false;

    if (!detected) {
        unsigned int eax, ebx, ecx, edx;
        __asm__ __volatile__("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(7), "c"(0));
        result = (ebx & (1 << 16)) != 0; // AVX-512F bit
        detected = true;
    }

    return result;
}

inline bool has_sse42() {
    static bool detected = false;
    static bool result = false;

    if (!detected) {
        unsigned int eax, ebx, ecx, edx;
        __asm__ __volatile__("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
        result = (ecx & (1 << 20)) != 0; // SSE4.2 bit
        detected = true;
    }

    return result;
}

constexpr int SIMD_CAPABILITY_NONE = 0;
constexpr int SIMD_CAPABILITY_SSSE3 = 1;
constexpr int SIMD_CAPABILITY_SSE42 = 2;
constexpr int SIMD_CAPABILITY_AVX2 = 3;
constexpr int SIMD_CAPABILITY_AVX512F = 4;

inline bool has_ssse3() {
    static bool result = false;
    static bool detected = false;

    if (!detected) {
        unsigned int eax, ebx, ecx, edx;
        __asm__ __volatile__("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
        result = (ecx & (1 << 9)) != 0; // SSSE3 bit
        detected = true;
    }

    return result;
}

inline int get_simd_capability() {
    if constexpr (CTRE_SIMD_ENABLED) {
        if (has_avx512f())
            return SIMD_CAPABILITY_AVX512F;
        if (has_avx2())
            return SIMD_CAPABILITY_AVX2;
        if (has_sse42())
            return SIMD_CAPABILITY_SSE42;
        if (has_ssse3())
            return SIMD_CAPABILITY_SSSE3;
    }
    return SIMD_CAPABILITY_NONE;
}

constexpr size_t SIMD_STRING_THRESHOLD = 16;
constexpr size_t SIMD_REPETITION_THRESHOLD = 32;

} // namespace simd
} // namespace ctre

#endif
