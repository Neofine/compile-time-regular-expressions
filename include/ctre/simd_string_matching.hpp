#ifndef CTRE__SIMD_STRING_MATCHING__HPP
#define CTRE__SIMD_STRING_MATCHING__HPP

#include "atoms_characters.hpp"
#include "flags_and_modes.hpp"
#include "simd_detection.hpp"
#include <immintrin.h>
#include <nmmintrin.h>
#include <iterator>

namespace ctre::simd {

template <typename Iterator, typename EndIterator>
[[nodiscard]] inline bool match_string_avx2_impl(Iterator& current, EndIterator last,
                                                  [[maybe_unused]] const flags& f,
                                                  const char* string_chars, size_t string_length) noexcept;
template <typename Iterator, typename EndIterator>
[[nodiscard]] inline bool match_string_sse42_impl(Iterator& current, EndIterator last,
                                                   [[maybe_unused]] const flags& f,
                                                   const char* string_chars, size_t string_length) noexcept;
template <typename Iterator, typename EndIterator>
[[nodiscard]] inline bool match_string_scalar_impl(Iterator& current, EndIterator last,
                                                    [[maybe_unused]] const flags& f,
                                                    const char* string_chars, size_t string_length) noexcept;

template <auto... String, typename Iterator, typename EndIterator>
[[nodiscard]] inline bool match_string_simd(Iterator& current, EndIterator last, const flags& f) noexcept {
    constexpr size_t string_length = sizeof...(String);
    constexpr char string_chars[] = {static_cast<char>(String)...};

    if constexpr (CTRE_SIMD_ENABLED) {
        if (get_simd_capability() >= SIMD_CAPABILITY_AVX2 && string_length >= 32)
            return match_string_avx2_impl(current, last, f, string_chars, string_length);
        else if (get_simd_capability() >= SIMD_CAPABILITY_SSE42 && string_length >= 16)
            return match_string_sse42_impl(current, last, f, string_chars, string_length);
    }
    return match_string_scalar_impl(current, last, f, string_chars, string_length);
}

template <typename Iterator, typename EndIterator>
[[nodiscard]] inline bool match_string_avx2_impl(Iterator& current, [[maybe_unused]] EndIterator last,
                                                  [[maybe_unused]] const flags& f,
                                                  const char* string_chars, size_t string_length) noexcept {
    Iterator pos = current;
    size_t processed = 0;

    while (processed + 64 <= string_length) {
        if (processed + 128 <= string_length) {
            __builtin_prefetch(&*(pos + 64), 0, 3);
            __builtin_prefetch(&string_chars[processed + 64], 0, 3);
        }
        __m256i data1 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*pos));
        __m256i data2 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*(pos + 32)));
        __m256i pattern1 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&string_chars[processed]));
        __m256i pattern2 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&string_chars[processed + 32]));
        __m256i result1 = _mm256_cmpeq_epi8(data1, pattern1);
        __m256i result2 = _mm256_cmpeq_epi8(data2, pattern2);
        if (static_cast<unsigned>(_mm256_movemask_epi8(result1)) != 0xFFFFFFFFU ||
            static_cast<unsigned>(_mm256_movemask_epi8(result2)) != 0xFFFFFFFFU)
            return false;
        pos += 64;
        processed += 64;
    }

    if (processed + 32 <= string_length) {
        __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*pos));
        __m256i pattern = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&string_chars[processed]));
        if (static_cast<unsigned>(_mm256_movemask_epi8(_mm256_cmpeq_epi8(data, pattern))) != 0xFFFFFFFFU)
            return false;
        pos += 32;
        processed += 32;
    }

    if (processed + 16 <= string_length) {
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&*pos));
        __m128i pattern = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&string_chars[processed]));
        if (static_cast<unsigned>(_mm_movemask_epi8(_mm_cmpeq_epi8(data, pattern))) != 0xFFFFU)
            return false;
        pos += 16;
        processed += 16;
    }

    for (size_t i = processed; i < string_length; ++i) {
        if (*pos != string_chars[i]) return false;
        ++pos;
    }
    current = pos;
    return true;
}

template <typename Iterator, typename EndIterator>
[[nodiscard]] inline bool match_string_sse42_impl(Iterator& current, [[maybe_unused]] EndIterator last,
                                                   [[maybe_unused]] const flags& f,
                                                   const char* string_chars, size_t string_length) noexcept {
    Iterator pos = current;

    if (string_length <= 16) {
        __m128i pattern = _mm_setzero_si128();
        if (string_length == 16) {
            pattern = _mm_loadu_si128(reinterpret_cast<const __m128i*>(string_chars));
        } else {
            char padded[16] = {0};
            for (size_t i = 0; i < string_length; ++i) padded[i] = string_chars[i];
            pattern = _mm_loadu_si128(reinterpret_cast<const __m128i*>(padded));
        }
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&*pos));
        if (_mm_cmpistri(pattern, data, 0x18) < static_cast<int>(string_length)) return false;
        current = pos + string_length;
        return true;
    }

    size_t processed = 0;
    while (processed + 16 <= string_length) {
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&*pos));
        __m128i pattern = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&string_chars[processed]));
        if (static_cast<unsigned>(_mm_movemask_epi8(_mm_cmpeq_epi8(data, pattern))) != 0xFFFFU)
            return false;
        pos += 16;
        processed += 16;
    }

    for (size_t i = processed; i < string_length; ++i) {
        if (*pos != string_chars[i]) return false;
        ++pos;
    }
    current = pos;
    return true;
}

template <typename Iterator, typename EndIterator>
[[nodiscard]] inline bool match_string_scalar_impl(Iterator& current, EndIterator last,
                                                    [[maybe_unused]] const flags& f,
                                                    const char* string_chars, size_t string_length) noexcept {
    for (size_t i = 0; i < string_length; ++i) {
        if (current == last || *current != string_chars[i]) return false;
        ++current;
    }
    return true;
}

} // namespace ctre::simd

#endif // CTRE__SIMD_STRING_MATCHING__HPP
