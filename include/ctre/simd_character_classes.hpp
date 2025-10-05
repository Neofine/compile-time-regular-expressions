#ifndef CTRE__SIMD_CHARACTER_CLASSES__HPP
#define CTRE__SIMD_CHARACTER_CLASSES__HPP

#include "atoms_characters.hpp"
#include "flags_and_modes.hpp"
#include "simd_detection.hpp"
#include "simd_repetition.hpp"
#include "simd_shufti.hpp"
#include <array>
#include <ctll/list.hpp>
#include <immintrin.h>

namespace ctre {
namespace simd {

template <typename SetType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_class_repeat_avx2(Iterator current, const EndIterator& last, const flags& flags,
                                             size_t& count);

template <typename SetType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_class_repeat_sse42(Iterator current, const EndIterator& last, const flags& flags,
                                              size_t& count);

template <typename SetType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_class_repeat_scalar(Iterator current, const EndIterator& last, const flags& flags,
                                               size_t& count);

template <char TargetChar, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_single_char_repeat_avx2(Iterator current, const EndIterator& last, const flags& flags,
                                              size_t& count);

template <char TargetChar, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_single_char_repeat_sse42(Iterator current, const EndIterator& last, const flags& flags,
                                               size_t& count);

template <char TargetChar, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_single_char_repeat_scalar(Iterator current, const EndIterator& last, const flags& flags,
                                                size_t& count);

template <typename PatternType>
struct simd_pattern_trait {
    static constexpr bool is_simd_optimizable = false;
    static constexpr size_t min_simd_length = 0;
};

template <auto A, auto B>
struct simd_pattern_trait<char_range<A, B>> {
    static constexpr bool is_simd_optimizable = true;
    static constexpr size_t min_simd_length = 8;
    static constexpr auto min_char = A;
    static constexpr auto max_char = B;
    static constexpr bool is_ascii_range = (A >= 0 && B <= 127);
    static constexpr bool is_contiguous = true;
};

template <auto A, auto B>
struct simd_pattern_trait<set<char_range<A, B>>> {
    static constexpr bool is_simd_optimizable = true;
    static constexpr size_t min_simd_length = 8;
    static constexpr auto min_char = A;
    static constexpr auto max_char = B;
    static constexpr bool is_ascii_range = (A >= 0 && B <= 127);
    static constexpr bool is_contiguous = true;
};

template <auto C>
struct simd_pattern_trait<set<character<C>>> {
    static constexpr bool is_simd_optimizable = true;
    static constexpr size_t min_simd_length = 8;
    static constexpr auto min_char = C;
    static constexpr auto max_char = C;
    static constexpr bool is_ascii_range = (C >= 0 && C <= 127);
    static constexpr bool is_contiguous = true;
    static constexpr char single_char = C;
};

template <auto C>
struct simd_pattern_trait<character<C>> {
    static constexpr bool is_simd_optimizable = true;
    static constexpr size_t min_simd_length = 8;
    static constexpr auto min_char = C;
    static constexpr auto max_char = C;
    static constexpr bool is_ascii_range = (C >= 0 && C <= 127);
    static constexpr bool is_contiguous = true;
    static constexpr char single_char = C;
};

template <typename T>
struct is_char_range_set_trait : std::false_type {
    using type = std::false_type;
};

template <auto A, auto B>
struct is_char_range_set_trait<set<char_range<A, B>>> : std::true_type {
    using type = std::true_type;
};

template <auto C>
struct is_char_range_set_trait<set<character<C>>> : std::false_type {
    using type = std::false_type;
};

template <auto C>
struct is_char_range_set_trait<character<C>> : std::true_type {
    using type = std::true_type;
};

template <typename T>
constexpr bool is_char_range_set() {
    return is_char_range_set_trait<T>::value;
}

template <typename T>
using is_char_range_set_trait_t = is_char_range_set_trait<T>;

constexpr bool is_ascii_alpha(char char_val) {
    return (char_val >= 'a' && char_val <= 'z') || (char_val >= 'A' && char_val <= 'Z');
}

template <typename PatternType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_pattern_repeat_simd(Iterator current, const EndIterator last, const flags& flags) {
    if constexpr (requires { simd::shufti_pattern_trait<PatternType>::is_shufti_optimizable; }) {
        if constexpr (simd::shufti_pattern_trait<PatternType>::is_shufti_optimizable &&
                      simd::shufti_pattern_trait<PatternType>::should_use_shufti) {
            return simd::match_pattern_repeat_shufti<PatternType, MinCount, MaxCount>(current, last, flags);
        }
    }

    if constexpr (!is_char_range_set<PatternType>()) {
        Iterator start = current;
        size_t count = 0;
        while (current != last && (MaxCount == 0 || count < MaxCount)) {
            if constexpr (requires { PatternType::match_char(*current, flags); }) {
                if (PatternType::match_char(*current, flags)) {
                    ++current;
                    ++count;
                } else {
                    break;
                }
            } else {
                break;
            }
        }
        return (count >= MinCount) ? current : start;
    }

    Iterator start = current;
    size_t count = 0;

    if (can_use_simd()) {
        if constexpr (requires { simd_pattern_trait<PatternType>::single_char; }) {
            Iterator start = current;
            size_t count = 0;
            constexpr char target_char = simd_pattern_trait<PatternType>::single_char;

            while (current != last && (MaxCount == 0 || count < MaxCount)) {
                if (*current == target_char) {
                    ++current;
                    ++count;
                } else {
                    break;
                }
            }
            return (count >= MinCount) ? current : start;
        } else {
            if constexpr (requires {
                              simd_pattern_trait<PatternType>::min_char;
                              simd_pattern_trait<PatternType>::max_char;
                          }) {
                if (get_simd_capability() >= SIMD_CAPABILITY_AVX2) {
                    current =
                        match_char_class_repeat_avx2<PatternType, MinCount, MaxCount>(current, last, flags, count);
                } else if (get_simd_capability() >= SIMD_CAPABILITY_SSE42) {
                    current =
                        match_char_class_repeat_sse42<PatternType, MinCount, MaxCount>(current, last, flags, count);
                }
            } else {
                if (get_simd_capability() >= SIMD_CAPABILITY_AVX2) {
                    current =
                        match_char_class_repeat_avx2<PatternType, MinCount, MaxCount>(current, last, flags, count);
                } else if (get_simd_capability() >= SIMD_CAPABILITY_SSE42) {
                    current =
                        match_char_class_repeat_sse42<PatternType, MinCount, MaxCount>(current, last, flags, count);
                }
            }
        }
    }

    return (count >= MinCount || MinCount == 0) ? current : start;
}

template <typename Iterator, typename EndIterator>
inline Iterator match_small_range_direct_avx2(Iterator current, const EndIterator& last, size_t& count,
                                              const std::array<char, 3>& chars, size_t num_chars,
                                              bool case_insensitive) {
    __m256i char_vecs[3];
    if (num_chars >= 1) {
        char_vecs[0] = _mm256_set1_epi8(case_insensitive ? (chars[0] | 0x20) : chars[0]);
    }
    if (num_chars >= 2) {
        char_vecs[1] = _mm256_set1_epi8(case_insensitive ? (chars[1] | 0x20) : chars[1]);
    }
    if (num_chars >= 3) {
        char_vecs[2] = _mm256_set1_epi8(case_insensitive ? (chars[2] | 0x20) : chars[2]);
    }

    while (current != last) {
        __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*current));
        __m256i result = _mm256_setzero_si256();

        if (case_insensitive) {
            __m256i data_lower = _mm256_or_si256(data, _mm256_set1_epi8(0x20));
            if (num_chars >= 1) {
                result = _mm256_or_si256(result, _mm256_cmpeq_epi8(data_lower, char_vecs[0]));
            }
            if (num_chars >= 2) {
                result = _mm256_or_si256(result, _mm256_cmpeq_epi8(data_lower, char_vecs[1]));
            }
            if (num_chars >= 3) {
                result = _mm256_or_si256(result, _mm256_cmpeq_epi8(data_lower, char_vecs[2]));
            }
        } else {
            if (num_chars >= 1) {
                result = _mm256_or_si256(result, _mm256_cmpeq_epi8(data, char_vecs[0]));
            }
            if (num_chars >= 2) {
                result = _mm256_or_si256(result, _mm256_cmpeq_epi8(data, char_vecs[1]));
            }
            if (num_chars >= 3) {
                result = _mm256_or_si256(result, _mm256_cmpeq_epi8(data, char_vecs[2]));
            }
        }

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

    return current;
}

template <typename Iterator, typename EndIterator>
inline Iterator match_small_range_direct_sse42(Iterator current, const EndIterator& last, size_t& count,
                                               const std::array<char, 3>& chars, size_t num_chars,
                                               bool case_insensitive) {
    __m128i char_vecs[3];
    if (num_chars >= 1) {
        char_vecs[0] = _mm_set1_epi8(case_insensitive ? (chars[0] | 0x20) : chars[0]);
    }
    if (num_chars >= 2) {
        char_vecs[1] = _mm_set1_epi8(case_insensitive ? (chars[1] | 0x20) : chars[1]);
    }
    if (num_chars >= 3) {
        char_vecs[2] = _mm_set1_epi8(case_insensitive ? (chars[2] | 0x20) : chars[2]);
    }

    while (current != last) {

        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&*current));
        __m128i result = _mm_setzero_si128();

        if (case_insensitive) {
            __m128i data_lower = _mm_or_si128(data, _mm_set1_epi8(0x20));
            if (num_chars >= 1) {
                result = _mm_or_si128(result, _mm_cmpeq_epi8(data_lower, char_vecs[0]));
            }
            if (num_chars >= 2) {
                result = _mm_or_si128(result, _mm_cmpeq_epi8(data_lower, char_vecs[1]));
            }
            if (num_chars >= 3) {
                result = _mm_or_si128(result, _mm_cmpeq_epi8(data_lower, char_vecs[2]));
            }
        } else {
            if (num_chars >= 1) {
                result = _mm_or_si128(result, _mm_cmpeq_epi8(data, char_vecs[0]));
            }
            if (num_chars >= 2) {
                result = _mm_or_si128(result, _mm_cmpeq_epi8(data, char_vecs[1]));
            }
            if (num_chars >= 3) {
                result = _mm_or_si128(result, _mm_cmpeq_epi8(data, char_vecs[2]));
            }
        }

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

    return current;
}

template <typename SetType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_class_repeat_avx2(Iterator current, const EndIterator& last, const flags& flags,
                                             size_t& count) {
    if constexpr (!simd_pattern_trait<SetType>::is_simd_optimizable) {
        return match_char_class_repeat_scalar<SetType, MinCount, MaxCount>(current, last, flags, count);
    }

    if constexpr (requires {
                      simd_pattern_trait<SetType>::min_char;
                      simd_pattern_trait<SetType>::max_char;
                  }) {
        constexpr char min_char = simd_pattern_trait<SetType>::min_char;
        constexpr char max_char = simd_pattern_trait<SetType>::max_char;
        constexpr size_t range_size = max_char - min_char + 1;
        const bool case_insensitive =
            is_ascii_alpha(min_char) && is_ascii_alpha(max_char) && ctre::is_case_insensitive(flags);

        if constexpr (range_size == 1) {
            if (get_simd_capability() >= SIMD_CAPABILITY_AVX2) {
                return match_single_char_repeat_avx2<min_char, MinCount, MaxCount>(current, last, flags, count);
            } else if (get_simd_capability() >= SIMD_CAPABILITY_SSE42) {
                return match_single_char_repeat_sse42<min_char, MinCount, MaxCount>(current, last, flags, count);
            } else {
                return match_single_char_repeat_scalar<min_char, MinCount, MaxCount>(current, last, flags, count);
            }
        } else if constexpr (range_size <= 3) {
            return match_char_class_repeat_scalar<SetType, MinCount, MaxCount>(current, last, flags, count);
        } else {
            __m256i min_vec = _mm256_set1_epi8(min_char);
            __m256i max_vec = _mm256_set1_epi8(max_char);

            while (current != last && (MaxCount == 0 || count + 32 <= MaxCount)) {
                __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*current));
                __m256i result;

                if (case_insensitive) {
                    __m256i data_lower = _mm256_or_si256(data, _mm256_set1_epi8(0x20));
                    __m256i min_lower = _mm256_or_si256(min_vec, _mm256_set1_epi8(0x20));
                    __m256i max_lower = _mm256_or_si256(max_vec, _mm256_set1_epi8(0x20));

                    __m256i ge_min = _mm256_cmpgt_epi8(data_lower, _mm256_sub_epi8(min_lower, _mm256_set1_epi8(1)));
                    __m256i le_max = _mm256_cmpgt_epi8(_mm256_add_epi8(max_lower, _mm256_set1_epi8(1)), data_lower);
                    result = _mm256_and_si256(ge_min, le_max);
                } else {
                    __m256i ge_min = _mm256_cmpgt_epi8(data, _mm256_sub_epi8(min_vec, _mm256_set1_epi8(1)));
                    __m256i le_max = _mm256_cmpgt_epi8(_mm256_add_epi8(max_vec, _mm256_set1_epi8(1)), data);
                    result = _mm256_and_si256(ge_min, le_max);
                }

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

            const char min_char_processed = case_insensitive ? (min_char | 0x20) : min_char;
            const char max_char_processed = case_insensitive ? (max_char | 0x20) : max_char;

            while (current != last && (MaxCount == 0 || count < MaxCount)) {
                char char_val = *current;
                if (case_insensitive) {
                    char_val |= 0x20;
                }
                if (char_val < min_char_processed || char_val > max_char_processed) {
                    break;
                }
                ++current;
                ++count;
            }
        }

        return current;
    } else {
        return match_char_class_repeat_scalar<SetType, MinCount, MaxCount>(current, last, flags, count);
    }
}

template <typename SetType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_class_repeat_sse42(Iterator current, const EndIterator& last, const flags& flags,
                                              size_t& count) {
    if constexpr (!simd_pattern_trait<SetType>::is_simd_optimizable) {
        return match_char_class_repeat_scalar<SetType, MinCount, MaxCount>(current, last, flags, count);
    }

    if constexpr (requires {
                      simd_pattern_trait<SetType>::min_char;
                      simd_pattern_trait<SetType>::max_char;
                  }) {
        constexpr char min_char = simd_pattern_trait<SetType>::min_char;
        constexpr char max_char = simd_pattern_trait<SetType>::max_char;
        constexpr size_t range_size = max_char - min_char + 1;
        const bool case_insensitive =
            is_ascii_alpha(min_char) && is_ascii_alpha(max_char) && ctre::is_case_insensitive(flags);

        if constexpr (range_size == 1) {
            if (get_simd_capability() >= SIMD_CAPABILITY_AVX2) {
                return match_single_char_repeat_avx2<min_char, MinCount, MaxCount>(current, last, flags, count);
            } else if (get_simd_capability() >= SIMD_CAPABILITY_SSE42) {
                return match_single_char_repeat_sse42<min_char, MinCount, MaxCount>(current, last, flags, count);
            } else {
                return match_single_char_repeat_scalar<min_char, MinCount, MaxCount>(current, last, flags, count);
            }
        } else if constexpr (range_size <= 3) {
            return match_char_class_repeat_scalar<SetType, MinCount, MaxCount>(current, last, flags, count);
        } else {
            __m128i min_vec = _mm_set1_epi8(min_char);
            __m128i max_vec = _mm_set1_epi8(max_char);

            while (current != last && (MaxCount == 0 || count + 16 <= MaxCount)) {
                __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&*current));
                __m128i result;

                if (case_insensitive) {
                    __m128i data_lower = _mm_or_si128(data, _mm_set1_epi8(0x20));
                    __m128i min_lower = _mm_or_si128(min_vec, _mm_set1_epi8(0x20));
                    __m128i max_lower = _mm_or_si128(max_vec, _mm_set1_epi8(0x20));

                    __m128i ge_min = _mm_cmpgt_epi8(data_lower, _mm_sub_epi8(min_lower, _mm_set1_epi8(1)));
                    __m128i le_max = _mm_cmpgt_epi8(_mm_add_epi8(max_lower, _mm_set1_epi8(1)), data_lower);
                    result = _mm_and_si128(ge_min, le_max);
                } else {
                    __m128i ge_min = _mm_cmpgt_epi8(data, _mm_sub_epi8(min_vec, _mm_set1_epi8(1)));
                    __m128i le_max = _mm_cmpgt_epi8(_mm_add_epi8(max_vec, _mm_set1_epi8(1)), data);
                    result = _mm_and_si128(ge_min, le_max);
                }

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

            const char min_char_processed = case_insensitive ? (min_char | 0x20) : min_char;
            const char max_char_processed = case_insensitive ? (max_char | 0x20) : max_char;

            while (current != last && (MaxCount == 0 || count < MaxCount)) {
                char char_val = *current;
                if (case_insensitive) {
                    char_val |= 0x20;
                }
                if (char_val < min_char_processed || char_val > max_char_processed) {
                    break;
                }
                ++current;
                ++count;
            }

            return current;
        }
    } else {
        return match_char_class_repeat_scalar<SetType, MinCount, MaxCount>(current, last, flags, count);
    }
}

template <typename SetType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_class_repeat_scalar(Iterator current, const EndIterator& last, const flags& flags,
                                               size_t& count) {
    while (current != last && (MaxCount == 0 || count < MaxCount)) {
        if constexpr (requires { SetType::match_char(*current, flags); }) {
            if (SetType::match_char(*current, flags)) {
                ++current;
                ++count;
            } else {
                break;
            }
        } else {
            break;
        }
    }
    return current;
}

template <char TargetChar, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_single_char_repeat_sse42(Iterator current, const EndIterator& last, const flags& flags,
                                               size_t& count) {
    const bool case_insensitive = is_ascii_alpha(TargetChar) && ctre::is_case_insensitive(flags);
    const __m128i target_vec = _mm_set1_epi8(TargetChar);
    const __m128i target_lower_vec = case_insensitive ? _mm_set1_epi8(TargetChar | 0x20) : target_vec;

    while (current != last && (MaxCount == 0 || count + 16 <= MaxCount)) {
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&*current));
        __m128i result;

        if (case_insensitive) {
            __m128i data_lower = _mm_or_si128(data, _mm_set1_epi8(0x20));
            result = _mm_cmpeq_epi8(data_lower, target_lower_vec);
        } else {
            result = _mm_cmpeq_epi8(data, target_vec);
        }

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

    const char target_char_processed = case_insensitive ? (TargetChar | 0x20) : TargetChar;

    while (current != last && (MaxCount == 0 || count < MaxCount)) {
        char char_val = *current;
        if (case_insensitive) {
            char_val |= 0x20;
        }
        if (char_val != target_char_processed) {
            break;
        }
        ++current;
        ++count;
    }

    return current;
}

template <char TargetChar, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_single_char_repeat_scalar(Iterator current, const EndIterator& last, const flags& flags,
                                                size_t& count) {
    const bool case_insensitive = is_ascii_alpha(TargetChar) && ctre::is_case_insensitive(flags);

    const char target_char_processed = case_insensitive ? (TargetChar | 0x20) : TargetChar;

    while (current != last && (MaxCount == 0 || count < MaxCount)) {
        char char_val = *current;
        if (case_insensitive) {
            char_val |= 0x20;
        }
        if (char_val != target_char_processed) {
            break;
        }
        ++current;
        ++count;
    }

    return current;
}

} // namespace simd
} // namespace ctre

#endif // CTRE__SIMD_CHARACTER_CLASSES__HPP
