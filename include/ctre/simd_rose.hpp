#ifndef CTRE__SIMD_ROSE__HPP
#define CTRE__SIMD_ROSE__HPP

#include <immintrin.h>
#include "utility.hpp"
#include "flags_and_modes.hpp"
#include "simd_detection.hpp"

namespace ctre {
namespace simd {

// Rose: Fast literal suffix search (Hyperscan technique)
// Strategy: Search for fixed literal, then verify prefix pattern

// SIMD search for 3-byte literal (like "ing")
template<char C1, char C2, char C3, typename Iterator, typename EndIterator>
inline Iterator rose_search_literal_3(Iterator current, const EndIterator& last) {
    const auto remaining = last - current;
    if (remaining < 3) return last;

    if (get_simd_capability() >= SIMD_CAPABILITY_SSE42) {
        __m128i c1_vec = _mm_set1_epi8(C1);
        __m128i c2_vec = _mm_set1_epi8(C2);
        __m128i c3_vec = _mm_set1_epi8(C3);

        Iterator pos = current;
        Iterator search_end = last - 2; // Need 3 bytes

        // SIMD search
        while (pos + 16 <= search_end) {
            __m128i data1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&*pos));
            __m128i data2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&*(pos + 1)));
            __m128i data3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&*(pos + 2)));

            __m128i cmp1 = _mm_cmpeq_epi8(data1, c1_vec);
            __m128i cmp2 = _mm_cmpeq_epi8(data2, c2_vec);
            __m128i cmp3 = _mm_cmpeq_epi8(data3, c3_vec);

            __m128i match_12 = _mm_and_si128(cmp1, cmp2);
            __m128i match_123 = _mm_and_si128(match_12, cmp3);

            int mask = _mm_movemask_epi8(match_123);

            if (mask) {
                int offset = __builtin_ctz(mask);
                return pos + offset;
            }

            pos += 16;
        }

        // Scalar tail
        while (pos <= search_end) {
            if (*pos == C1 && *(pos + 1) == C2 && *(pos + 2) == C3) {
                return pos;
            }
            pos++;
        }
    } else {
        // Scalar fallback
        Iterator pos = current;
        Iterator search_end = last - 2;

        while (pos <= search_end) {
            if (*pos == C1 && *(pos + 1) == C2 && *(pos + 2) == C3) {
                return pos;
            }
            pos++;
        }
    }

    return last;
}

// Rose optimization for pattern: [a-zA-Z]+ing
// Strategy: Find "ing", verify [a-zA-Z]+ backward
template<typename Iterator, typename EndIterator>
inline Iterator rose_alpha_suffix_ing(Iterator begin, const EndIterator& last, const flags& f) {
    Iterator current = begin;

    while (true) {
        // Step 1: Fast SIMD search for "ing"
        Iterator ing_pos = rose_search_literal_3<'i', 'n', 'g'>(current, last);

        if (ing_pos == last) {
            return last; // No match
        }

        // Step 2: Verify [a-zA-Z]+ before "ing"
        if (ing_pos == begin) {
            // "ing" at start, no prefix
            current = ing_pos + 3;
            continue;
        }

        Iterator prefix_end = ing_pos - 1;
        Iterator prefix_start = prefix_end;

        // Scan backward for [a-zA-Z]
        const bool case_insensitive = is_case_insensitive(f);

        while (prefix_start >= begin) {
            unsigned char c = static_cast<unsigned char>(*prefix_start);
            bool is_alpha;

            if (case_insensitive) {
                c |= 0x20; // Convert to lowercase
                is_alpha = (c >= 'a' && c <= 'z');
            } else {
                is_alpha = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
            }

            if (is_alpha) {
                prefix_start--;
            } else {
                break;
            }
        }
        prefix_start++; // Move back to first alpha char

        // Check if we have at least one alpha char
        if (prefix_start <= prefix_end) {
            // Found match! Pattern matches from prefix_start to ing_pos+2
            return ing_pos + 3; // Return position after "ing"
        }

        // No match, continue searching after this "ing"
        current = ing_pos + 3;
    }
}

// Check if pattern is [a-zA-Z]+ing (candidate for Rose optimization)
template<typename Pattern>
struct is_rose_alpha_suffix_ing : std::false_type {};

// Helper to detect the pattern at compile time
// This is a simplified check - in practice, you'd need more sophisticated pattern analysis
// For now, we can detect it manually for specific test patterns

} // namespace simd
} // namespace ctre

#endif
