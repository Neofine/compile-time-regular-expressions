#ifndef CTRE__SIMD_CHARACTER_CLASSES__HPP
#define CTRE__SIMD_CHARACTER_CLASSES__HPP

#include "atoms.hpp"
#include "atoms_characters.hpp"
#include "concepts.hpp"
#include "flags_and_modes.hpp"
#include "simd_detection.hpp"
#include "simd_repetition.hpp"
#include <array>
#ifdef CTRE_ARCH_X86
#include <immintrin.h>
#endif
#include <type_traits>

namespace ctre::simd {

// Forward declarations
template <typename SetType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_class_repeat_avx2(Iterator current, const EndIterator& last, const flags& f, size_t& count);

template <typename SetType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_class_repeat_sse42(Iterator current, const EndIterator& last, const flags& f, size_t& count);

template <typename SetType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_class_repeat_scalar(Iterator current, const EndIterator& last, const flags& f,
                                               size_t& count);

template <char TargetChar, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_single_char_repeat_avx2(Iterator current, const EndIterator& last, const flags& f, size_t& count);

template <char TargetChar, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_single_char_repeat_sse42(Iterator current, const EndIterator& last, const flags& f,
                                               size_t& count);

template <char TargetChar, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_single_char_repeat_scalar(Iterator current, const EndIterator& last, const flags& f,
                                                size_t& count);

// Bounds checking
template <typename Iterator, typename EndIterator>
    requires CharIterator<Iterator> && Subtractable<EndIterator>
[[nodiscard]] inline constexpr bool has_at_least_bytes(Iterator current, EndIterator last, std::size_t n) noexcept {
    if constexpr (PointerIterator<Iterator>) {
        return static_cast<std::size_t>(last - current) >= n;
    } else if constexpr (Subtractable<Iterator>) {
        const auto dist = last - current;
        return dist >= 0 && static_cast<std::size_t>(dist) >= n;
    } else {
        return false;
    }
}

// Pattern traits for SIMD optimization
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
    static constexpr char single_char = A;
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

// Multi-range sets - skip SIMD (gaps would be incorrectly matched)
template <auto A1, auto B1, auto A2, auto B2>
struct simd_pattern_trait<set<char_range<A1, B1>, char_range<A2, B2>>> {
    static constexpr bool is_simd_optimizable = false;
    static constexpr bool is_multi_range = true;
};

template <auto A1, auto B1, auto A2, auto B2, auto A3, auto B3>
struct simd_pattern_trait<set<char_range<A1, B1>, char_range<A2, B2>, char_range<A3, B3>>> {
    static constexpr bool is_simd_optimizable = false;
    static constexpr bool is_multi_range = true;
};

template <typename... Content>
    requires(sizeof...(Content) >= 2)
struct simd_pattern_trait<set<Content...>> {
    static constexpr bool is_simd_optimizable = false;
    static constexpr bool is_multi_range = true;
};

// Alternation helpers
template <auto First, auto... Rest>
constexpr auto constexpr_min() {
    if constexpr (sizeof...(Rest) == 0)
        return First;
    else {
        auto r = constexpr_min<Rest...>();
        return First < r ? First : r;
    }
}

template <auto First, auto... Rest>
constexpr auto constexpr_max() {
    if constexpr (sizeof...(Rest) == 0)
        return First;
    else {
        auto r = constexpr_max<Rest...>();
        return First > r ? First : r;
    }
}

template <auto... Cs>
struct simd_pattern_trait<select<character<Cs>...>> {
    static constexpr bool is_simd_optimizable = true;
    static constexpr size_t min_simd_length = 8;
    static constexpr auto min_char = constexpr_min<Cs...>();
    static constexpr auto max_char = constexpr_max<Cs...>();
    static constexpr bool is_ascii_range = (min_char >= 0 && max_char <= 127);
    static constexpr bool is_contiguous = ((max_char - min_char + 1) == sizeof...(Cs));
};

template <size_t Index, auto... Cs>
struct simd_pattern_trait<capture<Index, select<character<Cs>...>>> {
    static constexpr bool is_simd_optimizable = true;
    static constexpr size_t min_simd_length = 8;
    static constexpr auto min_char = constexpr_min<Cs...>();
    static constexpr auto max_char = constexpr_max<Cs...>();
    static constexpr bool is_ascii_range = (min_char >= 0 && max_char <= 127);
    static constexpr bool is_contiguous = ((max_char - min_char + 1) == sizeof...(Cs));
};

// Negated ranges
template <auto A, auto B>
struct simd_pattern_trait<negative_set<char_range<A, B>>> {
    static constexpr bool is_simd_optimizable = true;
    static constexpr bool is_negated = true;
    static constexpr char min_char = A;
    static constexpr char max_char = B;
    static constexpr size_t min_simd_length = 16;
};

// Type detection traits
template <typename T>
struct is_char_range_set_trait : std::false_type {};
template <auto A, auto B>
struct is_char_range_set_trait<set<char_range<A, B>>> : std::true_type {};
template <auto C>
struct is_char_range_set_trait<set<character<C>>> : std::true_type {};
template <auto C>
struct is_char_range_set_trait<character<C>> : std::true_type {};
template <char A, char B>
struct is_char_range_set_trait<negative_set<char_range<A, B>>> : std::true_type {};
template <auto... Cs>
struct is_char_range_set_trait<select<character<Cs>...>> : std::true_type {};
template <size_t I, auto... Cs>
struct is_char_range_set_trait<capture<I, select<character<Cs>...>>> : std::true_type {};

template <typename T>
constexpr bool is_char_range_set() {
    return is_char_range_set_trait<T>::value;
}
template <typename T>
using is_char_range_set_trait_t = is_char_range_set_trait<T>;

constexpr bool is_ascii_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

// Unified SIMD dispatcher
template <typename PatternType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_pattern_repeat_simd(Iterator current, const EndIterator last, const flags& f) {
    if constexpr (!is_char_range_set<PatternType>()) {
        Iterator start = current;
        size_t count = 0;
        while (current != last && (MaxCount == 0 || count < MaxCount)) {
            if constexpr (requires { PatternType::match_char(*current, f); }) {
                if (PatternType::match_char(*current, f)) {
                    ++current;
                    ++count;
                } else
                    break;
            } else
                break;
        }
        return (count >= MinCount) ? current : start;
    }

    Iterator start = current;
    size_t count = 0;

    if constexpr (can_use_simd()) {
        const auto remaining = last - current;
        if (remaining >= 32) {
#ifdef __AVX2__
            if (remaining >= 64)
                current = match_char_class_repeat_avx2<PatternType, MinCount, MaxCount>(current, last, f, count);
            else
                current = match_char_class_repeat_sse42<PatternType, MinCount, MaxCount>(current, last, f, count);
#elif defined(__SSE4_2__) || defined(__SSE2__)
            current = match_char_class_repeat_sse42<PatternType, MinCount, MaxCount>(current, last, f, count);
#endif
        }
    }
    return (count >= MinCount || MinCount == 0) ? current : start;
}

#ifdef CTRE_ARCH_X86
// AVX2 character class matching
template <typename SetType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_class_repeat_avx2(Iterator current, const EndIterator& last, const flags& f, size_t& count) {
    if constexpr (!simd_pattern_trait<SetType>::is_simd_optimizable)
        return match_char_class_repeat_scalar<SetType, MinCount, MaxCount>(current, last, f, count);

    if constexpr (requires {
                      simd_pattern_trait<SetType>::min_char;
                      simd_pattern_trait<SetType>::max_char;
                  }) {
        constexpr char min_c = simd_pattern_trait<SetType>::min_char;
        constexpr char max_c = simd_pattern_trait<SetType>::max_char;
        constexpr size_t range_size = static_cast<unsigned char>(max_c) - static_cast<unsigned char>(min_c) + 1;
        const bool case_insensitive = is_ascii_alpha(min_c) && is_ascii_alpha(max_c) && ctre::is_case_insensitive(f);

        constexpr bool is_negated = [] {
            if constexpr (requires { simd_pattern_trait<SetType>::is_negated; })
                return simd_pattern_trait<SetType>::is_negated;
            return false;
        }();

        if constexpr (range_size == 1 && requires { simd_pattern_trait<SetType>::single_char; }) {
            constexpr char target = simd_pattern_trait<SetType>::single_char;
            return match_single_char_repeat_avx2<target, MinCount, MaxCount>(current, last, f, count);
        }

        __m256i min_vec = _mm256_set1_epi8(min_c);
        __m256i max_vec = _mm256_set1_epi8(max_c);
        const __m256i all_ones = _mm256_set1_epi8(static_cast<char>(0xFF));

        // 16-byte fast path
        if (has_at_least_bytes(current, last, 16) && !has_at_least_bytes(current, last, 32)) {
            __m128i min_v = _mm_set1_epi8(min_c);
            __m128i max_v = _mm_set1_epi8(max_c);
            __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&*current));
            __m128i result;

            if (case_insensitive) {
                __m128i data_l = _mm_or_si128(data, _mm_set1_epi8(0x20));
                __m128i min_l = _mm_or_si128(min_v, _mm_set1_epi8(0x20));
                __m128i max_l = _mm_or_si128(max_v, _mm_set1_epi8(0x20));
                __m128i lt = _mm_cmpgt_epi8(min_l, data_l);
                __m128i gt = _mm_cmpgt_epi8(data_l, max_l);
                result = _mm_and_si128(_mm_xor_si128(lt, _mm_set1_epi8(static_cast<char>(0xFF))),
                                       _mm_xor_si128(gt, _mm_set1_epi8(static_cast<char>(0xFF))));
            } else {
                __m128i lt = _mm_cmpgt_epi8(min_v, data);
                __m128i gt = _mm_cmpgt_epi8(data, max_v);
                result = _mm_and_si128(_mm_xor_si128(lt, _mm_set1_epi8(static_cast<char>(0xFF))),
                                       _mm_xor_si128(gt, _mm_set1_epi8(static_cast<char>(0xFF))));
            }

            int mask = _mm_movemask_epi8(result);
            if (static_cast<unsigned>(mask) == 0xFFFFU) {
                current += 16;
                count += 16;
            } else {
                int m = __builtin_ctz(~mask);
                current += m;
                count += m;
                return current;
            }
            if (current >= last)
                return current;
        }

        // 64-byte chunks
        while (current != last && (MaxCount == 0 || count + 64 <= MaxCount)) {
            if (!has_at_least_bytes(current, last, 64))
                break;

            __m256i d1 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*current));
            __m256i d2 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*(current + 32)));
            __m256i r1, r2;

            if (case_insensitive) {
                __m256i d1_l = _mm256_or_si256(d1, _mm256_set1_epi8(0x20));
                __m256i d2_l = _mm256_or_si256(d2, _mm256_set1_epi8(0x20));
                __m256i min_l = _mm256_or_si256(min_vec, _mm256_set1_epi8(0x20));
                __m256i max_l = _mm256_or_si256(max_vec, _mm256_set1_epi8(0x20));
                __m256i lt1 = _mm256_cmpgt_epi8(min_l, d1_l), gt1 = _mm256_cmpgt_epi8(d1_l, max_l);
                __m256i lt2 = _mm256_cmpgt_epi8(min_l, d2_l), gt2 = _mm256_cmpgt_epi8(d2_l, max_l);
                if constexpr (is_negated) {
                    r1 = _mm256_or_si256(lt1, gt1);
                    r2 = _mm256_or_si256(lt2, gt2);
                } else {
                    r1 = _mm256_and_si256(_mm256_xor_si256(lt1, all_ones), _mm256_xor_si256(gt1, all_ones));
                    r2 = _mm256_and_si256(_mm256_xor_si256(lt2, all_ones), _mm256_xor_si256(gt2, all_ones));
                }
            } else {
                __m256i lt1 = _mm256_cmpgt_epi8(min_vec, d1), gt1 = _mm256_cmpgt_epi8(d1, max_vec);
                __m256i lt2 = _mm256_cmpgt_epi8(min_vec, d2), gt2 = _mm256_cmpgt_epi8(d2, max_vec);
                if constexpr (is_negated) {
                    r1 = _mm256_or_si256(lt1, gt1);
                    r2 = _mm256_or_si256(lt2, gt2);
                } else {
                    r1 = _mm256_and_si256(_mm256_xor_si256(lt1, all_ones), _mm256_xor_si256(gt1, all_ones));
                    r2 = _mm256_and_si256(_mm256_xor_si256(lt2, all_ones), _mm256_xor_si256(gt2, all_ones));
                }
            }

            __m256i combined = _mm256_and_si256(r1, r2);
            if (_mm256_testc_si256(combined, all_ones)) {
                current += 64;
                count += 64;
            } else {
                if (_mm256_testc_si256(r1, all_ones)) {
                    int m = __builtin_ctz(~_mm256_movemask_epi8(r2));
                    current += 32 + m;
                    count += 32 + m;
                } else {
                    int m = __builtin_ctz(~_mm256_movemask_epi8(r1));
                    current += m;
                    count += m;
                }
                break;
            }
        }

        // 32-byte chunks
        while (current != last && (MaxCount == 0 || count + 32 <= MaxCount)) {
            if (!has_at_least_bytes(current, last, 32))
                break;

            __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*current));
            __m256i result;

            if (case_insensitive) {
                __m256i data_l = _mm256_or_si256(data, _mm256_set1_epi8(0x20));
                __m256i min_l = _mm256_or_si256(min_vec, _mm256_set1_epi8(0x20));
                __m256i max_l = _mm256_or_si256(max_vec, _mm256_set1_epi8(0x20));
                __m256i lt = _mm256_cmpgt_epi8(min_l, data_l), gt = _mm256_cmpgt_epi8(data_l, max_l);
                if constexpr (is_negated)
                    result = _mm256_or_si256(lt, gt);
                else
                    result = _mm256_and_si256(_mm256_xor_si256(lt, all_ones), _mm256_xor_si256(gt, all_ones));
            } else {
                __m256i lt = _mm256_cmpgt_epi8(min_vec, data), gt = _mm256_cmpgt_epi8(data, max_vec);
                if constexpr (is_negated)
                    result = _mm256_or_si256(lt, gt);
                else
                    result = _mm256_and_si256(_mm256_xor_si256(lt, all_ones), _mm256_xor_si256(gt, all_ones));
            }

            int mask = _mm256_movemask_epi8(result);
            if (static_cast<unsigned>(mask) == 0xFFFFFFFFU) {
                current += 32;
                count += 32;
            } else {
                int m = __builtin_ctz(~mask);
                current += m;
                count += m;
                break;
            }
        }

        // Scalar tail
        unsigned char min_l = case_insensitive ? (min_c | 0x20) : static_cast<unsigned char>(min_c);
        unsigned char max_l = case_insensitive ? (max_c | 0x20) : static_cast<unsigned char>(max_c);
        for (; current != last && (MaxCount == 0 || count < MaxCount); ++current, ++count) {
            unsigned char c = static_cast<unsigned char>(*current);
            if (case_insensitive)
                c |= 0x20;
            bool in_range = (c >= min_l && c <= max_l);
            if constexpr (is_negated) {
                if (in_range)
                    break;
            } else {
                if (!in_range)
                    break;
            }
        }
        return current;
    } else {
        return match_char_class_repeat_scalar<SetType, MinCount, MaxCount>(current, last, f, count);
    }
}

// SSE4.2 character class matching
template <typename SetType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_class_repeat_sse42(Iterator current, const EndIterator& last, const flags& f,
                                              size_t& count) {
    if constexpr (!simd_pattern_trait<SetType>::is_simd_optimizable)
        return match_char_class_repeat_scalar<SetType, MinCount, MaxCount>(current, last, f, count);

    if constexpr (requires {
                      simd_pattern_trait<SetType>::min_char;
                      simd_pattern_trait<SetType>::max_char;
                  }) {
        constexpr char min_c = simd_pattern_trait<SetType>::min_char;
        constexpr char max_c = simd_pattern_trait<SetType>::max_char;
        constexpr size_t range_size = static_cast<unsigned char>(max_c) - static_cast<unsigned char>(min_c) + 1;
        const bool case_insensitive = is_ascii_alpha(min_c) && is_ascii_alpha(max_c) && ctre::is_case_insensitive(f);

        constexpr bool is_negated = [] {
            if constexpr (requires { simd_pattern_trait<SetType>::is_negated; })
                return simd_pattern_trait<SetType>::is_negated;
            return false;
        }();

        if constexpr (range_size == 1 && requires { simd_pattern_trait<SetType>::single_char; }) {
            constexpr char target = simd_pattern_trait<SetType>::single_char;
            return match_single_char_repeat_sse42<target, MinCount, MaxCount>(current, last, f, count);
        }

        __m128i min_vec = _mm_set1_epi8(min_c);
        __m128i max_vec = _mm_set1_epi8(max_c);

        while (current != last && (MaxCount == 0 || count + 16 <= MaxCount)) {
            if (!has_at_least_bytes(current, last, 16))
                break;

            __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&*current));
            __m128i result;

            if (case_insensitive) {
                __m128i data_l = _mm_or_si128(data, _mm_set1_epi8(0x20));
                __m128i min_l = _mm_or_si128(min_vec, _mm_set1_epi8(0x20));
                __m128i max_l = _mm_or_si128(max_vec, _mm_set1_epi8(0x20));
                __m128i lt = _mm_cmpgt_epi8(min_l, data_l), gt = _mm_cmpgt_epi8(data_l, max_l);
                result = _mm_and_si128(_mm_xor_si128(lt, _mm_set1_epi8(static_cast<char>(0xFF))),
                                       _mm_xor_si128(gt, _mm_set1_epi8(static_cast<char>(0xFF))));
            } else {
                __m128i lt = _mm_cmpgt_epi8(min_vec, data), gt = _mm_cmpgt_epi8(data, max_vec);
                result = _mm_and_si128(_mm_xor_si128(lt, _mm_set1_epi8(static_cast<char>(0xFF))),
                                       _mm_xor_si128(gt, _mm_set1_epi8(static_cast<char>(0xFF))));
            }

            int mask = _mm_movemask_epi8(result);
            if (static_cast<unsigned>(mask) == 0xFFFFU) {
                current += 16;
                count += 16;
            } else {
                int m = __builtin_ctz(~mask);
                current += m;
                count += m;
                break;
            }
        }

        unsigned char min_l = case_insensitive ? (min_c | 0x20) : static_cast<unsigned char>(min_c);
        unsigned char max_l = case_insensitive ? (max_c | 0x20) : static_cast<unsigned char>(max_c);
        for (; current != last && (MaxCount == 0 || count < MaxCount); ++current, ++count) {
            unsigned char c = static_cast<unsigned char>(*current);
            if (case_insensitive)
                c |= 0x20;
            bool in_range = (c >= min_l && c <= max_l);
            if constexpr (is_negated) {
                if (in_range)
                    break;
            } else {
                if (!in_range)
                    break;
            }
        }
        return current;
    } else {
        return match_char_class_repeat_scalar<SetType, MinCount, MaxCount>(current, last, f, count);
    }
}

// Scalar fallback
template <typename SetType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_class_repeat_scalar(Iterator current, const EndIterator& last, const flags& f,
                                               size_t& count) {
    while (current != last && (MaxCount == 0 || count < MaxCount)) {
        if constexpr (requires { SetType::match_char(*current, f); }) {
            if (SetType::match_char(*current, f)) {
                ++current;
                ++count;
            } else
                break;
        } else
            break;
    }
    return current;
}

// Single character AVX2
template <char TargetChar, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_single_char_repeat_avx2(Iterator current, const EndIterator& last, const flags& f,
                                              size_t& count) {
    const bool ci = is_ascii_alpha(TargetChar) && ctre::is_case_insensitive(f);
    const __m256i target = _mm256_set1_epi8(TargetChar);
    const __m256i target_l = ci ? _mm256_set1_epi8(TargetChar | 0x20) : target;
    const __m256i all_ones = _mm256_set1_epi8(static_cast<char>(0xFF));

    // 16-byte path
    if (has_at_least_bytes(current, last, 16) && !has_at_least_bytes(current, last, 32)) {
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&*current));
        __m128i t = _mm_set1_epi8(TargetChar);
        __m128i result = ci ? _mm_cmpeq_epi8(_mm_or_si128(data, _mm_set1_epi8(0x20)), _mm_set1_epi8(TargetChar | 0x20))
                            : _mm_cmpeq_epi8(data, t);
        if (_mm_test_all_ones(result)) {
            current += 16;
            count += 16;
        } else {
            int m = __builtin_ctz(~_mm_movemask_epi8(result));
            current += m;
            count += m;
            return current;
        }
        if (current >= last)
            return current;
    }

    // 32-byte path
    if (has_at_least_bytes(current, last, 32) && !has_at_least_bytes(current, last, 64)) {
        __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*current));
        __m256i result = ci ? _mm256_cmpeq_epi8(_mm256_or_si256(data, _mm256_set1_epi8(0x20)), target_l)
                            : _mm256_cmpeq_epi8(data, target);
        if (_mm256_testc_si256(result, all_ones)) {
            current += 32;
            count += 32;
        } else {
            int m = __builtin_ctz(~_mm256_movemask_epi8(result));
            current += m;
            count += m;
            return current;
        }
        if (current >= last)
            return current;
    }

    // 64-byte chunks
    while (current != last && (MaxCount == 0 || count + 64 <= MaxCount)) {
        if (!has_at_least_bytes(current, last, 64))
            break;
        __m256i d1 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*current));
        __m256i d2 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*(current + 32)));
        __m256i r1 = ci ? _mm256_cmpeq_epi8(_mm256_or_si256(d1, _mm256_set1_epi8(0x20)), target_l)
                        : _mm256_cmpeq_epi8(d1, target);
        __m256i r2 = ci ? _mm256_cmpeq_epi8(_mm256_or_si256(d2, _mm256_set1_epi8(0x20)), target_l)
                        : _mm256_cmpeq_epi8(d2, target);
        __m256i combined = _mm256_and_si256(r1, r2);
        if (_mm256_testc_si256(combined, all_ones)) {
            current += 64;
            count += 64;
        } else {
            if (_mm256_testc_si256(r1, all_ones)) {
                int m = __builtin_ctz(~_mm256_movemask_epi8(r2));
                current += 32 + m;
                count += 32 + m;
            } else {
                int m = __builtin_ctz(~_mm256_movemask_epi8(r1));
                current += m;
                count += m;
            }
            break;
        }
    }

    // 32-byte chunks
    while (current != last && (MaxCount == 0 || count + 32 <= MaxCount)) {
        if (!has_at_least_bytes(current, last, 32))
            break;
        __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*current));
        __m256i result = ci ? _mm256_cmpeq_epi8(_mm256_or_si256(data, _mm256_set1_epi8(0x20)), target_l)
                            : _mm256_cmpeq_epi8(data, target);
        if (_mm256_testc_si256(result, all_ones)) {
            current += 32;
            count += 32;
        } else {
            int m = __builtin_ctz(~_mm256_movemask_epi8(result));
            current += m;
            count += m;
            break;
        }
    }

    // Scalar tail
    unsigned char tc = ci ? (TargetChar | 0x20) : static_cast<unsigned char>(TargetChar);
    for (; current != last && (MaxCount == 0 || count < MaxCount); ++current, ++count) {
        unsigned char c = static_cast<unsigned char>(*current);
        if (ci)
            c |= 0x20;
        if (c != tc)
            break;
    }
    return current;
}

// Single character SSE4.2
template <char TargetChar, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_single_char_repeat_sse42(Iterator current, const EndIterator& last, const flags& f,
                                               size_t& count) {
    const bool ci = is_ascii_alpha(TargetChar) && ctre::is_case_insensitive(f);
    const __m128i target = _mm_set1_epi8(TargetChar);
    const __m128i target_l = ci ? _mm_set1_epi8(TargetChar | 0x20) : target;

    while (current != last && (MaxCount == 0 || count + 16 <= MaxCount)) {
        if (!has_at_least_bytes(current, last, 16))
            break;
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&*current));
        __m128i result =
            ci ? _mm_cmpeq_epi8(_mm_or_si128(data, _mm_set1_epi8(0x20)), target_l) : _mm_cmpeq_epi8(data, target);
        int mask = _mm_movemask_epi8(result);
        if (static_cast<unsigned>(mask) == 0xFFFFU) {
            current += 16;
            count += 16;
        } else {
            int m = __builtin_ctz(~mask);
            current += m;
            count += m;
            break;
        }
    }

    char tc = ci ? (TargetChar | 0x20) : TargetChar;
    for (; current != last && (MaxCount == 0 || count < MaxCount); ++current, ++count) {
        char c = *current;
        if (ci)
            c |= 0x20;
        if (c != tc)
            break;
    }
    return current;
}

#else // !CTRE_ARCH_X86 - Fallback to scalar implementations

template <typename SetType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_class_repeat_avx2(Iterator current, const EndIterator& last, const flags& f, size_t& count) {
    return match_char_class_repeat_scalar<SetType, MinCount, MaxCount>(current, last, f, count);
}

template <typename SetType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_class_repeat_sse42(Iterator current, const EndIterator& last, const flags& f, size_t& count) {
    return match_char_class_repeat_scalar<SetType, MinCount, MaxCount>(current, last, f, count);
}

template <char TargetChar, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_single_char_repeat_avx2(Iterator current, const EndIterator& last, const flags& f, size_t& count) {
    return match_single_char_repeat_scalar<TargetChar, MinCount, MaxCount>(current, last, f, count);
}

template <char TargetChar, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_single_char_repeat_sse42(Iterator current, const EndIterator& last, const flags& f, size_t& count) {
    return match_single_char_repeat_scalar<TargetChar, MinCount, MaxCount>(current, last, f, count);
}

#endif // CTRE_ARCH_X86

// Single character scalar
template <char TargetChar, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_single_char_repeat_scalar(Iterator current, const EndIterator& last, const flags& f,
                                                size_t& count) {
    const bool ci = is_ascii_alpha(TargetChar) && ctre::is_case_insensitive(f);
    char tc = ci ? (TargetChar | 0x20) : TargetChar;
    for (; current != last && (MaxCount == 0 || count < MaxCount); ++current, ++count) {
        char c = *current;
        if (ci)
            c |= 0x20;
        if (c != tc)
            break;
    }
    return current;
}

} // namespace ctre::simd

#endif
