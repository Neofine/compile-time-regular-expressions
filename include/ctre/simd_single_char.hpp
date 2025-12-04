#ifndef CTRE__SIMD_SINGLE_CHAR__HPP
#define CTRE__SIMD_SINGLE_CHAR__HPP

#include "simd_detection.hpp"
#include <cstddef>
#include <immintrin.h>

namespace ctre::simd {

template <char C, size_t MaxCount = 0>
[[nodiscard]] inline size_t match_single_char_avx2(const char* data, size_t length) noexcept {
    const char* p = data;
    size_t remaining = length;
    size_t count = 0;
    __m256i target = _mm256_set1_epi8(C);

    while (remaining >= 32 && (MaxCount == 0 || count + 32 <= MaxCount)) {
        __m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
        uint32_t mask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(chunk, target));
        if (mask == 0xFFFFFFFF) { p += 32; count += 32; remaining -= 32; }
        else if (mask == 0) break;
        else { int m = __builtin_ctz(~mask); count += m; break; }
    }

    while (remaining > 0 && (MaxCount == 0 || count < MaxCount)) {
        if (*p == C) { ++p; ++count; --remaining; }
        else break;
    }
    return count;
}

template <char C, size_t MaxCount = 0>
[[nodiscard]] inline size_t match_single_char_sse42(const char* data, size_t length) noexcept {
    const char* p = data;
    size_t remaining = length;
    size_t count = 0;
    __m128i target = _mm_set1_epi8(C);

    while (remaining >= 16 && (MaxCount == 0 || count + 16 <= MaxCount)) {
        __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
        uint16_t mask = _mm_movemask_epi8(_mm_cmpeq_epi8(chunk, target));
        if (mask == 0xFFFF) { p += 16; count += 16; remaining -= 16; }
        else if (mask == 0) break;
        else { int m = __builtin_ctz(~static_cast<uint32_t>(mask)); count += m; break; }
    }

    while (remaining > 0 && (MaxCount == 0 || count < MaxCount)) {
        if (*p == C) { ++p; ++count; --remaining; }
        else break;
    }
    return count;
}

template <char C, size_t MaxCount = 0>
[[nodiscard]] inline size_t match_single_char_scalar(const char* data, size_t length) noexcept {
    size_t count = 0;
    for (size_t i = 0; i < length && (MaxCount == 0 || count < MaxCount); ++i) {
        if (data[i] == C) ++count;
        else break;
    }
    return count;
}

template <char C, size_t MaxCount = 0>
[[nodiscard]] inline size_t match_single_char_repeat(const char* data, size_t length) noexcept {
#ifdef __AVX2__
    if (length >= 16 && length < 32) return match_single_char_sse42<C, MaxCount>(data, length);
    return match_single_char_avx2<C, MaxCount>(data, length);
#elif defined(__SSE4_2__) || defined(__SSE2__)
    return match_single_char_sse42<C, MaxCount>(data, length);
#else
    return match_single_char_scalar<C, MaxCount>(data, length);
#endif
}

} // namespace ctre::simd

#endif // CTRE__SIMD_SINGLE_CHAR__HPP
