#ifndef CTRE__SIMD_ROSE__HPP
#define CTRE__SIMD_ROSE__HPP

#include "../utility.hpp"
#include "../flags_and_modes.hpp"
#include "detection.hpp"
#ifdef CTRE_ARCH_X86
#include <immintrin.h>
#endif

namespace ctre::simd {

// Rose: Fast literal suffix search (Hyperscan technique)
template <char C1, char C2, char C3, typename Iterator, typename EndIterator>
[[nodiscard]] inline Iterator rose_search_literal_3(Iterator current, const EndIterator& last) noexcept {
    const auto remaining = last - current;
    if (remaining < 3) return last;

#ifdef CTRE_ARCH_X86
    if (get_simd_capability() >= SIMD_CAPABILITY_SSE42) {
        __m128i c1_vec = _mm_set1_epi8(C1);
        __m128i c2_vec = _mm_set1_epi8(C2);
        __m128i c3_vec = _mm_set1_epi8(C3);
        Iterator pos = current;
        Iterator search_end = last - 2;

        while (pos + 16 <= search_end) {
            __m128i data1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&*pos));
            __m128i data2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&*(pos + 1)));
            __m128i data3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&*(pos + 2)));
            __m128i match = _mm_and_si128(_mm_and_si128(_mm_cmpeq_epi8(data1, c1_vec),
                                                        _mm_cmpeq_epi8(data2, c2_vec)),
                                          _mm_cmpeq_epi8(data3, c3_vec));
            int mask = _mm_movemask_epi8(match);
            if (mask) return pos + __builtin_ctz(mask);
            pos += 16;
        }
        while (pos <= search_end) {
            if (*pos == C1 && *(pos + 1) == C2 && *(pos + 2) == C3) return pos;
            ++pos;
        }
    } else
#endif
    {
        Iterator pos = current;
        Iterator search_end = last - 2;
        while (pos <= search_end) {
            if (*pos == C1 && *(pos + 1) == C2 && *(pos + 2) == C3) return pos;
            ++pos;
        }
    }
    return last;
}

// Rose for [a-zA-Z]+ing pattern
template <typename Iterator, typename EndIterator>
[[nodiscard]] inline Iterator rose_alpha_suffix_ing(Iterator begin, const EndIterator& last,
                                                     const flags& f) noexcept {
    Iterator current = begin;
    while (true) {
        Iterator ing_pos = rose_search_literal_3<'i', 'n', 'g'>(current, last);
        if (ing_pos == last) return last;
        if (ing_pos == begin) { current = ing_pos + 3; continue; }

        Iterator prefix_start = ing_pos - 1;
        const bool ci = is_case_insensitive(f);
        while (prefix_start >= begin) {
            unsigned char c = static_cast<unsigned char>(*prefix_start);
            bool is_alpha = ci ? ((c | 0x20) >= 'a' && (c | 0x20) <= 'z')
                               : ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
            if (is_alpha) --prefix_start;
            else break;
        }
        ++prefix_start;
        if (prefix_start <= ing_pos - 1) return ing_pos + 3;
        current = ing_pos + 3;
    }
}

template <typename Pattern> struct is_rose_alpha_suffix_ing : std::false_type {};

} // namespace ctre::simd

#endif // CTRE__SIMD_ROSE__HPP
