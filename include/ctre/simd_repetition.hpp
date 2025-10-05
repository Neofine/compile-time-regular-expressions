#ifndef CTRE__SIMD_REPETITION__HPP
#define CTRE__SIMD_REPETITION__HPP

#include "atoms_characters.hpp"
#include "flags_and_modes.hpp"
#include "simd_detection.hpp"
#include <immintrin.h>

namespace ctre {
namespace simd {

template <size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_character_repeat_simd_with_char(Iterator current, const EndIterator last, const flags& f,
                                                      char target_char);

template <typename CharacterType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_character_repeat_simd(Iterator current, const EndIterator last, const flags& f) {
    Iterator start = current;
    size_t count = 0;

    if (current != last) {
        char first_char = *current;

        if (CharacterType::match_char(first_char, f)) {
            return match_character_repeat_simd_with_char<MinCount, MaxCount>(current, last, f, first_char);
        }
    }

    while (current != last && (MaxCount == 0 || count < MaxCount)) {
        if (CharacterType::match_char(*current, f)) {
            ++current;
            ++count;
        } else {
            break;
        }
    }

    if (count >= MinCount) {
        return current;
    } else {
        return start; // No match
    }
}

template <size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_character_repeat_simd_with_char(Iterator current, const EndIterator last, const flags& f,
                                                      char target_char) {
    Iterator start = current;
    size_t count = 0;
    const bool case_insensitive = is_ascii_alpha(target_char) && is_case_insensitive(f);

    if constexpr (CTRE_SIMD_ENABLED) {
        if (get_simd_capability() >= SIMD_CAPABILITY_AVX2) {
            while (current != last && (MaxCount == 0 || count + 32 <= MaxCount)) {
                __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*current));

                if (case_insensitive) {
                    __m256i target_lower = _mm256_set1_epi8(target_char | 0x20);
                    __m256i data_lower = _mm256_or_si256(data, _mm256_set1_epi8(0x20));
                    __m256i result = _mm256_cmpeq_epi8(data_lower, target_lower);

                    int mask = _mm256_movemask_epi8(result);
                    if (static_cast<unsigned int>(mask) == 0xFFFFFFFFU) {
                        current += 32;
                        count += 32;
                    } else {
                        int first_mismatch = __builtin_ctz(~mask);
                        current += first_mismatch;
                        count += first_mismatch;
                        break;
                    }
                } else {
                    __m256i target_vec = _mm256_set1_epi8(target_char);
                    __m256i result = _mm256_cmpeq_epi8(data, target_vec);

                    int mask = _mm256_movemask_epi8(result);
                    if (static_cast<unsigned int>(mask) == 0xFFFFFFFFU) {
                        current += 32;
                        count += 32;
                    } else {
                        int first_mismatch = __builtin_ctz(~mask);
                        current += first_mismatch;
                        count += first_mismatch;
                        break;
                    }
                }
            }
        } else if (get_simd_capability() >= SIMD_CAPABILITY_SSE42) {
            while (current != last && (MaxCount == 0 || count + 16 <= MaxCount)) {
                __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&*current));

                if (case_insensitive) {
                    __m128i target_lower = _mm_set1_epi8(target_char | 0x20);
                    __m128i data_lower = _mm_or_si128(data, _mm_set1_epi8(0x20));
                    __m128i result = _mm_cmpeq_epi8(data_lower, target_lower);

                    int mask = _mm_movemask_epi8(result);
                    if (static_cast<unsigned int>(mask) == 0xFFFFU) {
                        current += 16;
                        count += 16;
                    } else {
                        int first_mismatch = __builtin_ctz(~mask);
                        current += first_mismatch;
                        count += first_mismatch;
                        break;
                    }
                } else {
                    __m128i target_vec = _mm_set1_epi8(target_char);
                    __m128i result = _mm_cmpeq_epi8(data, target_vec);

                    int mask = _mm_movemask_epi8(result);
                    if (static_cast<unsigned int>(mask) == 0xFFFFU) {
                        current += 16;
                        count += 16;
                    } else {
                        int first_mismatch = __builtin_ctz(~mask);
                        current += first_mismatch;
                        count += first_mismatch;
                        break;
                    }
                }
            }
        }
    }

    while (current != last && (MaxCount == 0 || count < MaxCount)) {
        if (case_insensitive) {
            if ((*current | 0x20) == (target_char | 0x20)) {
                ++current;
                ++count;
            } else {
                break;
            }
        } else {
            if (*current == target_char) {
                ++current;
                ++count;
            } else {
                break;
            }
        }
    }

    if (count >= MinCount) {
        return current;
    } else {
        return start;
    }
}

} // namespace simd
} // namespace ctre

#endif
