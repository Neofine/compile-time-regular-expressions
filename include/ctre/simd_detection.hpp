#ifndef CTRE__SIMD_DETECTION__HPP
#define CTRE__SIMD_DETECTION__HPP

#include <immintrin.h>
#include <type_traits>

namespace ctre {
namespace simd {

// Compile-time flag to disable SIMD optimizations
// Define CTRE_DISABLE_SIMD to disable all SIMD optimizations
#ifndef CTRE_DISABLE_SIMD
#define CTRE_SIMD_ENABLED 1
#else
#define CTRE_SIMD_ENABLED 0
#endif

// Detect if we can use SIMD at runtime
// Note: This will be called from evaluation.hpp with proper constexpr checks
constexpr bool can_use_simd() {
    return CTRE_SIMD_ENABLED;
}

// SIMD instruction set detection
inline bool has_avx2() {
    static bool detected = false;
    static bool result = false;
    
    if (!detected) {
        // Check CPUID for AVX2 support
        unsigned int eax, ebx, ecx, edx;
        __asm__ __volatile__ ("cpuid" : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx) : "a" (7), "c" (0));
        result = (ebx & (1 << 5)) != 0; // AVX2 bit
        detected = true;
    }
    
    return result;
}

inline bool has_avx512f() {
    static bool detected = false;
    static bool result = false;
    
    if (!detected) {
        // Check CPUID for AVX-512F support
        unsigned int eax, ebx, ecx, edx;
        __asm__ __volatile__ ("cpuid" : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx) : "a" (7), "c" (0));
        result = (ebx & (1 << 16)) != 0; // AVX-512F bit
        detected = true;
    }
    
    return result;
}

inline bool has_sse42() {
    static bool detected = false;
    static bool result = false;
    
    if (!detected) {
        // Check CPUID for SSE4.2 support
        unsigned int eax, ebx, ecx, edx;
        __asm__ __volatile__ ("cpuid" : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx) : "a" (1));
        result = (ecx & (1 << 20)) != 0; // SSE4.2 bit
        detected = true;
    }
    
    return result;
}

// Choose the best available SIMD instruction set
inline int get_simd_capability() {
    if constexpr (CTRE_SIMD_ENABLED) {
        if (has_avx512f()) return 3; // AVX-512F
        if (has_avx2()) return 2; // AVX2
        if (has_sse42()) return 1; // SSE4.2
    }
    return 0; // No SIMD
}

// SIMD capability levels
constexpr int SIMD_CAPABILITY_NONE = 0;
constexpr int SIMD_CAPABILITY_SSE42 = 1;
constexpr int SIMD_CAPABILITY_AVX2 = 2;
constexpr int SIMD_CAPABILITY_AVX512F = 3;

// SIMD optimization thresholds
constexpr size_t SIMD_STRING_THRESHOLD = 16;
constexpr size_t SIMD_REPETITION_THRESHOLD = 32;

} // namespace simd
} // namespace ctre

#endif

