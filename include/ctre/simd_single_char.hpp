#ifndef CTRE__SIMD_SINGLE_CHAR__HPP
#define CTRE__SIMD_SINGLE_CHAR__HPP

#include <immintrin.h>
#include <cstddef>
#include "simd_detection.hpp"

namespace ctre {
namespace simd {

// Fast path for single-character repetitions (e.g., a+, 9*, [x]+)
// Much faster than Shufti for single chars
template <char C, size_t MaxCount = 0>
inline size_t match_single_char_avx2(const char* data, size_t length) {
    const char* p = data;
    size_t remaining = length;
    size_t count = 0;
    
    // AVX2 path: process 32 bytes at a time
    __m256i target = _mm256_set1_epi8(C);
    
    while (remaining >= 32 && (MaxCount == 0 || count + 32 <= MaxCount)) {
        __m256i chunk = _mm256_loadu_si256((__m256i*)p);
        __m256i matches = _mm256_cmpeq_epi8(chunk, target);
        uint32_t mask = _mm256_movemask_epi8(matches);
        
        if (mask == 0xFFFFFFFF) {
            // All 32 bytes match
            p += 32;
            count += 32;
            remaining -= 32;
        } else if (mask == 0) {
            // No matches - done
            goto done_avx2;
        } else {
            // Find first non-match
            int first_nonmatch = __builtin_ctz(~mask);
            count += first_nonmatch;
            p += first_nonmatch;
            goto done_avx2;
        }
    }
    
done_avx2:
    // Scalar fallback for remaining bytes
    while (remaining > 0 && (MaxCount == 0 || count < MaxCount)) {
        if (*p == C) {
            ++p;
            ++count;
            --remaining;
        } else {
            break;
        }
    }
    
    return count;
}

// SSE4.2 fallback (16 bytes at a time)
template <char C, size_t MaxCount = 0>
inline size_t match_single_char_sse42(const char* data, size_t length) {
    const char* p = data;
    size_t remaining = length;
    size_t count = 0;
    
    __m128i target = _mm_set1_epi8(C);
    
    while (remaining >= 16 && (MaxCount == 0 || count + 16 <= MaxCount)) {
        __m128i chunk = _mm_loadu_si128((__m128i*)p);
        __m128i matches = _mm_cmpeq_epi8(chunk, target);
        uint16_t mask = _mm_movemask_epi8(matches);
        
        if (mask == 0xFFFF) {
            p += 16;
            count += 16;
            remaining -= 16;
        } else if (mask == 0) {
            goto done_sse;
        } else {
            int first_nonmatch = __builtin_ctz(~(uint32_t)mask);
            count += first_nonmatch;
            p += first_nonmatch;
            goto done_sse;
        }
    }
    
done_sse:
    // Scalar fallback
    while (remaining > 0 && (MaxCount == 0 || count < MaxCount)) {
        if (*p == C) {
            ++p;
            ++count;
            --remaining;
        } else {
            break;
        }
    }
    
    return count;
}

// Scalar fallback
template <char C, size_t MaxCount = 0>
inline size_t match_single_char_scalar(const char* data, size_t length) {
    size_t count = 0;
    for (size_t i = 0; i < length && (MaxCount == 0 || count < MaxCount); ++i) {
        if (data[i] == C) {
            ++count;
        } else {
            break;
        }
    }
    return count;
}

// Main dispatcher
template <char C, size_t MaxCount = 0>
inline size_t match_single_char_repeat(const char* data, size_t length) {
    int capability = get_simd_capability();
    
    if (capability >= SIMD_CAPABILITY_AVX2) {
        return match_single_char_avx2<C, MaxCount>(data, length);
    } else if (capability >= SIMD_CAPABILITY_SSE42) {
        return match_single_char_sse42<C, MaxCount>(data, length);
    } else {
        return match_single_char_scalar<C, MaxCount>(data, length);
    }
}

} // namespace simd
} // namespace ctre

#endif
