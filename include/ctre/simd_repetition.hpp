#ifndef CTRE__SIMD_REPETITION__HPP
#define CTRE__SIMD_REPETITION__HPP

#include "simd_detection.hpp"
#include "atoms_characters.hpp"
#include "flags_and_modes.hpp"
#include <immintrin.h>

namespace ctre::simd {

template <size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
[[nodiscard]] inline Iterator match_character_repeat_simd_with_char(Iterator current, EndIterator last,
                                                                     const flags& f, char target_char) noexcept;

template <typename CharacterType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
[[nodiscard]] inline Iterator match_character_repeat_simd(Iterator current, EndIterator last, const flags& f) noexcept {
    Iterator start = current;
    size_t count = 0;

    if (current != last) {
        char first_char = *current;
        if (CharacterType::match_char(first_char, f))
            return match_character_repeat_simd_with_char<MinCount, MaxCount>(current, last, f, first_char);
    }

    while (current != last && (MaxCount == 0 || count < MaxCount)) {
        if (CharacterType::match_char(*current, f)) { ++current; ++count; }
        else break;
    }
    return (count >= MinCount) ? current : start;
}

template <size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
[[nodiscard]] inline Iterator match_character_repeat_simd_with_char(Iterator current, EndIterator last,
                                                                     const flags& f, char target_char) noexcept {
    Iterator start = current;
    size_t count = 0;
    const bool ci = is_ascii_alpha(target_char) && is_case_insensitive(f);

    if constexpr (CTRE_SIMD_ENABLED) {
#ifdef __AVX2__
        while (count < 32 && (MaxCount == 0 || count < MaxCount)) {
            __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*current));
            __m256i result;
            if (ci) {
                __m256i t = _mm256_set1_epi8(target_char | 0x20);
                result = _mm256_cmpeq_epi8(_mm256_or_si256(data, _mm256_set1_epi8(0x20)), t);
            } else {
                result = _mm256_cmpeq_epi8(data, _mm256_set1_epi8(target_char));
            }
            int mask = _mm256_movemask_epi8(result);
            if (static_cast<unsigned>(mask) == 0xFFFFFFFFU) { current += 32; count += 32; }
            else { int m = __builtin_ctz(~mask); current += m; count += m; break; }
        }
#elif defined(__SSE4_2__) || defined(__SSE2__)
        while (count < 16 && (MaxCount == 0 || count < MaxCount)) {
            __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&*current));
            __m128i result;
            if (ci) {
                __m128i t = _mm_set1_epi8(target_char | 0x20);
                result = _mm_cmpeq_epi8(_mm_or_si128(data, _mm_set1_epi8(0x20)), t);
            } else {
                result = _mm_cmpeq_epi8(data, _mm_set1_epi8(target_char));
            }
            int mask = _mm_movemask_epi8(result);
            if (static_cast<unsigned>(mask) == 0xFFFFU) { current += 16; count += 16; }
            else { int m = __builtin_ctz(~mask); current += m; count += m; break; }
        }
#endif
    }

    while (current != last && (MaxCount == 0 || count < MaxCount)) {
        bool match = ci ? ((*current | 0x20) == (target_char | 0x20)) : (*current == target_char);
        if (match) { ++current; ++count; }
        else break;
    }
    return (count >= MinCount) ? current : start;
}

} // namespace ctre::simd

#endif // CTRE__SIMD_REPETITION__HPP
