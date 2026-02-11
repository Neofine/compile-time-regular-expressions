#ifndef CTRE__SIMD_LITERAL_SEARCH__HPP
#define CTRE__SIMD_LITERAL_SEARCH__HPP

#include "detection.hpp"
#include <cstring>

#if CTRE_SIMD_ENABLED && defined(CTRE_ARCH_X86)
#include <immintrin.h>
#endif

namespace ctre::simd {

template <size_t LiteralLen>
[[nodiscard]] inline bool search_literal_scalar(const char* begin, const char* end,
                                                 const char (&literal)[LiteralLen]) noexcept {
    const size_t len = LiteralLen - 1;
    if (len == 0) return true;
    const char* search_end = end - len + 1;
    if (begin >= search_end) return false;
    const char first = literal[0];
    for (const char* ptr = begin; ptr < search_end; ++ptr) {
        if (*ptr == first) {
            bool match = true;
            for (size_t i = 1; i < len; ++i)
                if (ptr[i] != literal[i]) { match = false; break; }
            if (match) return true;
        }
    }
    return false;
}

#if CTRE_SIMD_ENABLED && defined(__AVX2__)
template <size_t LiteralLen>
[[nodiscard]] inline bool search_literal_avx2(const char* begin, const char* end,
                                               const char (&literal)[LiteralLen]) noexcept {
    const size_t len = LiteralLen - 1;
    if (len == 0) return true;
    const char* search_end = end - len + 1;
    if (begin >= search_end) return false;

    __m256i first_char = _mm256_set1_epi8(literal[0]);
    const char* ptr = begin;

    while (ptr + 32 <= search_end) {
        __m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr));
        int mask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(chunk, first_char));
        while (mask != 0) {
            int pos = __builtin_ctz(mask);
            if (std::memcmp(ptr + pos, literal, len) == 0) return true;
            mask &= mask - 1;
        }
        ptr += 32;
    }

    while (ptr < search_end) {
        if (*ptr == literal[0] && std::memcmp(ptr, literal, len) == 0) return true;
        ++ptr;
    }
    return false;
}
#endif

#if CTRE_SIMD_ENABLED && defined(__SSE4_2__)
template <size_t LiteralLen>
[[nodiscard]] inline bool search_literal_sse42(const char* begin, const char* end,
                                                const char (&literal)[LiteralLen]) noexcept {
    const size_t len = LiteralLen - 1;
    if (len == 0) return true;
    const char* search_end = end - len + 1;
    if (begin >= search_end) return false;
    if (len > 16) return search_literal_scalar(begin, end, literal);

    __m128i pattern = _mm_setzero_si128();
    std::memcpy(&pattern, literal, len);
    const char* ptr = begin;

    while (ptr + 16 <= search_end) {
        __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr));
        int idx = _mm_cmpistri(pattern, chunk, 0x0C);
        if (idx < 16 && ptr + idx + len <= end && std::memcmp(ptr + idx, literal, len) == 0)
            return true;
        ptr += 16;
    }

    while (ptr < search_end) {
        if (std::memcmp(ptr, literal, len) == 0) return true;
        ++ptr;
    }
    return false;
}
#endif

template <size_t LiteralLen>
[[nodiscard]] inline bool search_literal(const char* begin, const char* end,
                                          const char (&literal)[LiteralLen]) noexcept {
#if CTRE_SIMD_ENABLED
    [[maybe_unused]] const size_t input_size = end - begin;
#ifdef __AVX2__
    if (input_size >= 32 && get_simd_capability() >= SIMD_CAPABILITY_AVX2)
        return search_literal_avx2(begin, end, literal);
#endif
#ifdef __SSE4_2__
    if (input_size >= 16 && LiteralLen <= 17 && get_simd_capability() >= SIMD_CAPABILITY_SSE42)
        return search_literal_sse42(begin, end, literal);
#endif
#endif
    return search_literal_scalar(begin, end, literal);
}

template <char... Chars>
[[nodiscard]] inline bool search_literal_ct(const char* begin, const char* end) noexcept {
    constexpr char literal[] = {Chars..., '\0'};
    return search_literal(begin, end, literal);
}

} // namespace ctre::simd

#endif // CTRE__SIMD_LITERAL_SEARCH__HPP
