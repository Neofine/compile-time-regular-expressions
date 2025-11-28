#ifndef CTRE__SIMD_CHARACTER_CLASSES__HPP
#define CTRE__SIMD_CHARACTER_CLASSES__HPP

#include "atoms_characters.hpp"
#include "flags_and_modes.hpp"
#include "simd_detection.hpp"
#include "simd_repetition.hpp"
#include <array>
#include <immintrin.h>
#include <type_traits>

namespace ctre {
namespace simd {

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

template <typename SetType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_class_repeat_avx2(Iterator current, const EndIterator& last, const flags& flags,
                                             size_t& count);

template <typename SetType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_class_repeat_sse42(Iterator current, const EndIterator& last, const flags& flags,
                                              size_t& count);

template <typename SetType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_char_class_repeat_scalar(Iterator current, const EndIterator& last, const flags& flags,
                                               size_t& count);

// Forward declare with template parameters
template <char TargetChar, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_single_char_repeat_avx2(Iterator current, const EndIterator& last, const flags& flags,
                                              size_t& count);

template <char TargetChar, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_single_char_repeat_sse42(Iterator current, const EndIterator& last, const flags& flags,
                                               size_t& count);

template <char TargetChar, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_single_char_repeat_scalar(Iterator current, const EndIterator& last, const flags& flags,
                                                size_t& count);

// ============================================================================
// SIMD BOUNDS CHECKING HELPERS
// ============================================================================

// Safe check if we have at least N bytes available
// Handles different iterator types gracefully
// C++20: [[nodiscard]] warns if result is ignored
template <typename Iterator, typename EndIterator>
[[nodiscard]] inline constexpr bool has_at_least_bytes(Iterator current, EndIterator last, size_t n) noexcept {
    // For pointers, we can directly compute distance
    if constexpr (std::is_pointer_v<Iterator>) {
        return static_cast<size_t>(last - current) >= n;
    }
    // For iterators that support operator-, use it
    else if constexpr (requires {
                           { last - current } -> std::convertible_to<std::ptrdiff_t>;
                       }) {
        auto dist = last - current;
        return dist >= 0 && static_cast<size_t>(dist) >= n;
    }
    // Conservative fallback: assume we don't have enough
    else {
        return false;
    }
}

// ============================================================================
// SIMD PATTERN TRAITS
// ============================================================================

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

// Multi-range sets like [a-zA-Z] or [0-9a-fA-F] - DON'T use range-based SIMD!
// These have gaps that would be incorrectly matched by >= min && <= max
template <auto A1, auto B1, auto A2, auto B2>
struct simd_pattern_trait<set<char_range<A1, B1>, char_range<A2, B2>>> {
    static constexpr bool is_simd_optimizable = false; // Skip SIMD, use recursive evaluator
    static constexpr bool is_multi_range = true;
};

// Three ranges (e.g., [0-9a-fA-F])
template <auto A1, auto B1, auto A2, auto B2, auto A3, auto B3>
struct simd_pattern_trait<set<char_range<A1, B1>, char_range<A2, B2>, char_range<A3, B3>>> {
    static constexpr bool is_simd_optimizable = false; // Skip SIMD, use recursive evaluator
    static constexpr bool is_multi_range = true;
};

// Generic multi-element sets - catch all complex patterns
template <typename... Content>
    requires(sizeof...(Content) >= 2)
struct simd_pattern_trait<set<Content...>> {
    static constexpr bool is_simd_optimizable = false; // Skip SIMD for multi-element sets
    static constexpr bool is_multi_range = true;
};

// ============================================================================
// NEGATED RANGE TRAITS (for [^...] patterns)
// ============================================================================

// Negated single range: [^a-z] becomes: char < 'a' OR char > 'z'
template <auto A, auto B>
struct simd_pattern_trait<negative_set<char_range<A, B>>> {
    static constexpr bool is_simd_optimizable = true;
    static constexpr bool is_negated = true;
    static constexpr char min_char = A;
    static constexpr char max_char = B;
    static constexpr size_t min_simd_length = 16;
};

// ============================================================================
// PATTERN DETECTION TRAITS
// ============================================================================

template <typename T>
struct is_char_range_set_trait : std::false_type {
    using type = std::false_type;
};

template <auto A, auto B>
struct is_char_range_set_trait<set<char_range<A, B>>> : std::true_type {
    using type = std::true_type;
};

template <auto C>
struct is_char_range_set_trait<set<character<C>>> : std::true_type {
    using type = std::true_type;
};

template <auto C>
struct is_char_range_set_trait<character<C>> : std::true_type {
    using type = std::true_type;
};

// CRITICAL: Add support for negated ranges to enable SIMD!
template <char A, char B>
struct is_char_range_set_trait<negative_set<char_range<A, B>>> : std::true_type {
    using type = std::true_type;
};

template <typename T>
constexpr bool is_char_range_set() {
    return is_char_range_set_trait<T>::value;
}

// Add type alias for evaluation.hpp compatibility
template <typename T>
using is_char_range_set_trait_t = is_char_range_set_trait<T>;

// Helper function to check if character is ASCII alpha
constexpr bool is_ascii_alpha(char char_val) {
    return (char_val >= 'a' && char_val <= 'z') || (char_val >= 'A' && char_val <= 'Z');
}

// ============================================================================
// UNIFIED SIMD MATCHING FUNCTIONS
// ============================================================================

template <typename PatternType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_pattern_repeat_simd(Iterator current, const EndIterator last, const flags& flags) {
    if constexpr (!is_char_range_set<PatternType>()) {
        // Fallback to scalar for non-SIMD patterns
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
                // For patterns without match_char, fall back to general evaluation
                break;
            }
        }
        return (count >= MinCount) ? current : start;
    }

    Iterator start = current;
    size_t count = 0;

    // PERF TEST: Try enabling SIMD for ALL range sizes, including 2-8 chars
    // Theory: Even small ranges benefit from processing 32 bytes at once
    // But we need to check if we have enough data to make SIMD worth it

    // PERF FIX: Single characters NOW use specialized SIMD dispatch!
    // We added match_single_char_repeat_avx2() which uses simple cmpeq
    // This should be faster than range comparison for single chars

    if (can_use_simd()) {
        // Check if this pattern has min_char and max_char traits (character ranges)
        if constexpr (requires {
                          simd_pattern_trait<PatternType>::min_char;
                          simd_pattern_trait<PatternType>::max_char;
                      }) {
            if (get_simd_capability() >= SIMD_CAPABILITY_AVX2) {
                current = match_char_class_repeat_avx2<PatternType, MinCount, MaxCount>(current, last, flags, count);
            } else if (get_simd_capability() >= SIMD_CAPABILITY_SSE42) {
                current = match_char_class_repeat_sse42<PatternType, MinCount, MaxCount>(current, last, flags, count);
            }
        } else {
            // Fallback to general character class patterns
            if (get_simd_capability() >= SIMD_CAPABILITY_AVX2) {
                current = match_char_class_repeat_avx2<PatternType, MinCount, MaxCount>(current, last, flags, count);
            } else if (get_simd_capability() >= SIMD_CAPABILITY_SSE42) {
                current = match_char_class_repeat_sse42<PatternType, MinCount, MaxCount>(current, last, flags, count);
            }
        }
    }

    return (count >= MinCount || MinCount == 0) ? current : start;
}

// ============================================================================
// SMALL RANGE OPTIMIZATIONS (≤6 characters) - EXPANDED FOR 14x PUSH!
// ============================================================================

// Optimized SIMD for small sparse sets (2-6 characters) using direct character comparison
// PERF: Direct comparison is ~21x faster than Shufti for small sets
template <typename Iterator, typename EndIterator, size_t N>
inline Iterator match_small_range_direct_avx2(Iterator current, const EndIterator& last, size_t& count,
                                              const std::array<char, N>& chars, size_t num_chars,
                                              bool case_insensitive) {
    // Create vectors for each character (unrolled for performance)
    __m256i char_vecs[N];
    for (size_t i = 0; i < num_chars; ++i) {
        char_vecs[i] = _mm256_set1_epi8(case_insensitive ? (chars[i] | 0x20) : chars[i]);
    }

    // Process full 32-byte chunks
    while (current != last) {
        if (!has_at_least_bytes(current, last, 32))
            break;

        __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*current));
        __m256i result = _mm256_setzero_si256();

        if (case_insensitive) {
            __m256i data_lower = _mm256_or_si256(data, _mm256_set1_epi8(0x20));
            // Unroll comparisons for each character
            if (num_chars >= 1)
                result = _mm256_or_si256(result, _mm256_cmpeq_epi8(data_lower, char_vecs[0]));
            if (num_chars >= 2)
                result = _mm256_or_si256(result, _mm256_cmpeq_epi8(data_lower, char_vecs[1]));
            if (num_chars >= 3)
                result = _mm256_or_si256(result, _mm256_cmpeq_epi8(data_lower, char_vecs[2]));
            if (num_chars >= 4)
                result = _mm256_or_si256(result, _mm256_cmpeq_epi8(data_lower, char_vecs[3]));
            if (num_chars >= 5)
                result = _mm256_or_si256(result, _mm256_cmpeq_epi8(data_lower, char_vecs[4]));
            if (num_chars >= 6)
                result = _mm256_or_si256(result, _mm256_cmpeq_epi8(data_lower, char_vecs[5]));
        } else {
            if (num_chars >= 1)
                result = _mm256_or_si256(result, _mm256_cmpeq_epi8(data, char_vecs[0]));
            if (num_chars >= 2)
                result = _mm256_or_si256(result, _mm256_cmpeq_epi8(data, char_vecs[1]));
            if (num_chars >= 3)
                result = _mm256_or_si256(result, _mm256_cmpeq_epi8(data, char_vecs[2]));
            if (num_chars >= 4)
                result = _mm256_or_si256(result, _mm256_cmpeq_epi8(data, char_vecs[3]));
            if (num_chars >= 5)
                result = _mm256_or_si256(result, _mm256_cmpeq_epi8(data, char_vecs[4]));
            if (num_chars >= 6)
                result = _mm256_or_si256(result, _mm256_cmpeq_epi8(data, char_vecs[5]));
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

// Optimized SIMD for small sparse sets (2-6 characters) using direct character comparison - SSE4.2
template <typename Iterator, typename EndIterator, size_t N>
inline Iterator match_small_range_direct_sse42(Iterator current, const EndIterator& last, size_t& count,
                                               const std::array<char, N>& chars, size_t num_chars,
                                               bool case_insensitive) {
    // Create vectors for each character (unrolled for performance)
    __m128i char_vecs[N];
    for (size_t i = 0; i < num_chars; ++i) {
        char_vecs[i] = _mm_set1_epi8(case_insensitive ? (chars[i] | 0x20) : chars[i]);
    }

    // Process full 16-byte chunks
    while (current != last) {
        if (!has_at_least_bytes(current, last, 16))
            break;

        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&*current));
        __m128i result = _mm_setzero_si128();

        if (case_insensitive) {
            __m128i data_lower = _mm_or_si128(data, _mm_set1_epi8(0x20));
            if (num_chars >= 1)
                result = _mm_or_si128(result, _mm_cmpeq_epi8(data_lower, char_vecs[0]));
            if (num_chars >= 2)
                result = _mm_or_si128(result, _mm_cmpeq_epi8(data_lower, char_vecs[1]));
            if (num_chars >= 3)
                result = _mm_or_si128(result, _mm_cmpeq_epi8(data_lower, char_vecs[2]));
            if (num_chars >= 4)
                result = _mm_or_si128(result, _mm_cmpeq_epi8(data_lower, char_vecs[3]));
            if (num_chars >= 5)
                result = _mm_or_si128(result, _mm_cmpeq_epi8(data_lower, char_vecs[4]));
            if (num_chars >= 6)
                result = _mm_or_si128(result, _mm_cmpeq_epi8(data_lower, char_vecs[5]));
        } else {
            if (num_chars >= 1)
                result = _mm_or_si128(result, _mm_cmpeq_epi8(data, char_vecs[0]));
            if (num_chars >= 2)
                result = _mm_or_si128(result, _mm_cmpeq_epi8(data, char_vecs[1]));
            if (num_chars >= 3)
                result = _mm_or_si128(result, _mm_cmpeq_epi8(data, char_vecs[2]));
            if (num_chars >= 4)
                result = _mm_or_si128(result, _mm_cmpeq_epi8(data, char_vecs[3]));
            if (num_chars >= 5)
                result = _mm_or_si128(result, _mm_cmpeq_epi8(data, char_vecs[4]));
            if (num_chars >= 6)
                result = _mm_or_si128(result, _mm_cmpeq_epi8(data, char_vecs[5]));
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

// ============================================================================
// GENERIC CHARACTER CLASS SIMD IMPLEMENTATIONS
// ============================================================================

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
        // FIX: Cast to unsigned char to avoid signed overflow in range calculation
        constexpr size_t range_size = static_cast<unsigned char>(max_char) - static_cast<unsigned char>(min_char) + 1;
        const bool case_insensitive =
            is_ascii_alpha(min_char) && is_ascii_alpha(max_char) && ctre::is_case_insensitive(flags);

        // Check if this is a negated range [^a-z]
        constexpr bool is_negated = [] {
            if constexpr (requires { simd_pattern_trait<SetType>::is_negated; }) {
                return simd_pattern_trait<SetType>::is_negated;
            } else {
                return false;
            }
        }();

        // PERF: Dispatch to specialized single-char function (uses simple cmpeq)
        if constexpr (range_size == 1 && requires { simd_pattern_trait<SetType>::single_char; }) {
            constexpr char target = simd_pattern_trait<SetType>::single_char;
            return match_single_char_repeat_avx2<target, MinCount, MaxCount>(current, last, flags, count);
        }

        // Use range comparison for multi-character ranges
        // Range comparison: 2 cmpgt + 1 and = 3 ops with short dependency chain
        // Direct comparison: N cmpeq + N or = 2N ops with longer dependency chain (for N>1)
        __m256i min_vec = _mm256_set1_epi8(min_char);
        __m256i max_vec = _mm256_set1_epi8(max_char);

        // PERF: 16-byte fast path for inputs between 16-31 bytes
        if (has_at_least_bytes(current, last, 16) && !has_at_least_bytes(current, last, 32)) {
            __m128i min_vec_128 = _mm_set1_epi8(min_char);
            __m128i max_vec_128 = _mm_set1_epi8(max_char);
            __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&*current));
            __m128i result;

            if (case_insensitive) {
                __m128i data_lower = _mm_or_si128(data, _mm_set1_epi8(0x20));
                __m128i min_lower = _mm_or_si128(min_vec_128, _mm_set1_epi8(0x20));
                __m128i max_lower = _mm_or_si128(max_vec_128, _mm_set1_epi8(0x20));

                __m128i lt_min = _mm_cmpgt_epi8(min_lower, data_lower);
                __m128i ge_min = _mm_xor_si128(lt_min, _mm_set1_epi8(static_cast<char>(0xFF)));

                __m128i gt_max = _mm_cmpgt_epi8(data_lower, max_lower);
                __m128i le_max = _mm_xor_si128(gt_max, _mm_set1_epi8(static_cast<char>(0xFF)));

                result = _mm_and_si128(ge_min, le_max);
            } else {
                __m128i lt_min = _mm_cmpgt_epi8(min_vec_128, data);
                __m128i ge_min = _mm_xor_si128(lt_min, _mm_set1_epi8(static_cast<char>(0xFF)));

                __m128i gt_max = _mm_cmpgt_epi8(data, max_vec_128);
                __m128i le_max = _mm_xor_si128(gt_max, _mm_set1_epi8(static_cast<char>(0xFF)));

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
                return current; // Early return: found mismatch
            }

            // Early return if no more data (exact 16-byte input)
            // C++20: [[likely]] attribute (cleaner than __builtin_expect)
            if (current >= last) [[likely]] {
                return current;
            }
        }

        // PERF: Process 64-byte chunks with interleaved scheduling (same as single-char)
        const __m256i all_ones = _mm256_set1_epi8(0xFF);

        while (current != last && (MaxCount == 0 || count + 64 <= MaxCount)) {
            // C++20: [[unlikely]] for rare branch (cleaner syntax)
            if (!has_at_least_bytes(current, last, 64)) [[unlikely]] {
                break; // Unlikely path for small inputs
            }

            __m256i data1 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*current));
            __m256i data2 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*(current + 32)));
            __m256i result1, result2;

            // Range comparison (optimized for both normal and negated ranges)
            if (case_insensitive) {
                __m256i data1_lower = _mm256_or_si256(data1, _mm256_set1_epi8(0x20));
                __m256i data2_lower = _mm256_or_si256(data2, _mm256_set1_epi8(0x20));
                __m256i min_lower = _mm256_or_si256(min_vec, _mm256_set1_epi8(0x20));
                __m256i max_lower = _mm256_or_si256(max_vec, _mm256_set1_epi8(0x20));

                __m256i lt_min1 = _mm256_cmpgt_epi8(min_lower, data1_lower);
                __m256i gt_max1 = _mm256_cmpgt_epi8(data1_lower, max_lower);
                __m256i lt_min2 = _mm256_cmpgt_epi8(min_lower, data2_lower);
                __m256i gt_max2 = _mm256_cmpgt_epi8(data2_lower, max_lower);

                if constexpr (is_negated) {
                    result1 = _mm256_or_si256(lt_min1, gt_max1);
                    result2 = _mm256_or_si256(lt_min2, gt_max2);
                } else {
                    __m256i ge_min1 = _mm256_xor_si256(lt_min1, all_ones);
                    __m256i le_max1 = _mm256_xor_si256(gt_max1, all_ones);
                    __m256i ge_min2 = _mm256_xor_si256(lt_min2, all_ones);
                    __m256i le_max2 = _mm256_xor_si256(gt_max2, all_ones);
                    result1 = _mm256_and_si256(ge_min1, le_max1);
                    result2 = _mm256_and_si256(ge_min2, le_max2);
                }
            } else {
                __m256i lt_min1 = _mm256_cmpgt_epi8(min_vec, data1);
                __m256i gt_max1 = _mm256_cmpgt_epi8(data1, max_vec);
                __m256i lt_min2 = _mm256_cmpgt_epi8(min_vec, data2);
                __m256i gt_max2 = _mm256_cmpgt_epi8(data2, max_vec);

                if constexpr (is_negated) {
                    result1 = _mm256_or_si256(lt_min1, gt_max1);
                    result2 = _mm256_or_si256(lt_min2, gt_max2);
                } else {
                    __m256i ge_min1 = _mm256_xor_si256(lt_min1, all_ones);
                    __m256i le_max1 = _mm256_xor_si256(gt_max1, all_ones);
                    __m256i ge_min2 = _mm256_xor_si256(lt_min2, all_ones);
                    __m256i le_max2 = _mm256_xor_si256(gt_max2, all_ones);
                    result1 = _mm256_and_si256(ge_min1, le_max1);
                    result2 = _mm256_and_si256(ge_min2, le_max2);
                }
            }

            // PERF: Combine results first, then check (better ILP + fewer testc calls)
            __m256i combined = _mm256_and_si256(result1, result2);

        // C++20: [[likely]] for hot path
        if (_mm256_testc_si256(combined, all_ones)) [[likely]] {
            // Hot path: both chunks match (single testc check!)
            current += 64;
            count += 64;
        } else {
                // Slow path: Determine which chunk failed
                if (_mm256_testc_si256(result1, all_ones)) {
                    // First chunk matches, mismatch in second
                    int mask2 = _mm256_movemask_epi8(result2);
                    int first_mismatch = __builtin_ctz(~mask2);
                    current += 32 + first_mismatch;
                    count += 32 + first_mismatch;
                } else {
                    // Mismatch in first chunk
                    int mask1 = _mm256_movemask_epi8(result1);
                    int first_mismatch = __builtin_ctz(~mask1);
                    current += first_mismatch;
                    count += first_mismatch;
                }
                break;
            }
        }

        // Process remaining 32-byte chunks with bounds checking
        while (current != last && (MaxCount == 0 || count + 32 <= MaxCount)) {
            // SAFETY: Check if we have at least 32 bytes available
            if (!has_at_least_bytes(current, last, 32)) {
                break; // Not enough data, fall back to scalar tail
            }

            __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*current));
            __m256i result;

            // Range comparison (optimized for both normal and negated ranges)
            if (case_insensitive) {
                __m256i data_lower = _mm256_or_si256(data, _mm256_set1_epi8(0x20));
                __m256i min_lower = _mm256_or_si256(min_vec, _mm256_set1_epi8(0x20));
                __m256i max_lower = _mm256_or_si256(max_vec, _mm256_set1_epi8(0x20));

                __m256i lt_min = _mm256_cmpgt_epi8(min_lower, data_lower);
                __m256i gt_max = _mm256_cmpgt_epi8(data_lower, max_lower);

                if constexpr (is_negated) {
                    // For negated: match if (< min OR > max)
                    result = _mm256_or_si256(lt_min, gt_max);
                } else {
                    // For normal: match if (>= min AND <= max)
                    __m256i ge_min = _mm256_xor_si256(lt_min, all_ones);
                    __m256i le_max = _mm256_xor_si256(gt_max, all_ones);
                    result = _mm256_and_si256(ge_min, le_max);
                }
            } else {
                __m256i lt_min = _mm256_cmpgt_epi8(min_vec, data);
                __m256i gt_max = _mm256_cmpgt_epi8(data, max_vec);

                if constexpr (is_negated) {
                    // For negated: match if (< min OR > max)
                    result = _mm256_or_si256(lt_min, gt_max);
                } else {
                    // For normal: match if (>= min AND <= max)
                    __m256i ge_min = _mm256_xor_si256(lt_min, all_ones);
                    __m256i le_max = _mm256_xor_si256(gt_max, all_ones);
                    result = _mm256_and_si256(ge_min, le_max);
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

        // Process remaining characters with scalar (much faster for small amounts)
        // FIX: Use unsigned char to avoid signed comparison issues with high-bit characters
        unsigned char min_char_lower = case_insensitive ? (min_char | 0x20) : static_cast<unsigned char>(min_char);
        unsigned char max_char_lower = case_insensitive ? (max_char | 0x20) : static_cast<unsigned char>(max_char);

        for (; current != last && (MaxCount == 0 || count < MaxCount); ++current, ++count) {
            unsigned char char_val = static_cast<unsigned char>(*current);
            if (case_insensitive) {
                char_val |= 0x20; // Convert to lowercase
            }
            bool in_range = (char_val >= min_char_lower && char_val <= max_char_lower);
            if constexpr (is_negated) {
                if (in_range)
                    break; // For negated, stop if char IS in range
            } else {
                if (!in_range)
                    break; // For normal, stop if char NOT in range
            }
        }

        return current;
    } else {
        // Fallback for patterns without min_char/max_char traits
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
        // FIX: Cast to unsigned char to avoid signed overflow in range calculation
        constexpr size_t range_size = static_cast<unsigned char>(max_char) - static_cast<unsigned char>(min_char) + 1;
        const bool case_insensitive =
            is_ascii_alpha(min_char) && is_ascii_alpha(max_char) && ctre::is_case_insensitive(flags);

        // Check if this is a negated range [^a-z]
        constexpr bool is_negated = [] {
            if constexpr (requires { simd_pattern_trait<SetType>::is_negated; }) {
                return simd_pattern_trait<SetType>::is_negated;
            } else {
                return false;
            }
        }();

        // PERF: Dispatch to specialized single-char function (uses simple cmpeq)
        if constexpr (range_size == 1 && requires { simd_pattern_trait<SetType>::single_char; }) {
            constexpr char target = simd_pattern_trait<SetType>::single_char;
            return match_single_char_repeat_sse42<target, MinCount, MaxCount>(current, last, flags, count);
        }

        // Use range comparison for multi-character ranges
        __m128i min_vec = _mm_set1_epi8(min_char);
        __m128i max_vec = _mm_set1_epi8(max_char);

        // Process full 16-byte chunks with bounds checking
        while (current != last && (MaxCount == 0 || count + 16 <= MaxCount)) {
            // SAFETY: Check if we have at least 16 bytes available
            if (!has_at_least_bytes(current, last, 16)) {
                break; // Not enough data, fall back to scalar tail
            }

            __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&*current));
            __m128i result;

            if (case_insensitive) {
                __m128i data_lower = _mm_or_si128(data, _mm_set1_epi8(0x20));
                __m128i min_lower = _mm_or_si128(min_vec, _mm_set1_epi8(0x20));
                __m128i max_lower = _mm_or_si128(max_vec, _mm_set1_epi8(0x20));

                __m128i lt_min = _mm_cmpgt_epi8(min_lower, data_lower);
                __m128i ge_min = _mm_xor_si128(lt_min, _mm_set1_epi8(static_cast<char>(0xFF)));

                __m128i gt_max = _mm_cmpgt_epi8(data_lower, max_lower);
                __m128i le_max = _mm_xor_si128(gt_max, _mm_set1_epi8(static_cast<char>(0xFF)));

                result = _mm_and_si128(ge_min, le_max);
            } else {
                __m128i lt_min = _mm_cmpgt_epi8(min_vec, data);
                __m128i ge_min = _mm_xor_si128(lt_min, _mm_set1_epi8(static_cast<char>(0xFF)));

                __m128i gt_max = _mm_cmpgt_epi8(data, max_vec);
                __m128i le_max = _mm_xor_si128(gt_max, _mm_set1_epi8(static_cast<char>(0xFF)));

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

        // Process remaining characters with scalar (much faster for small amounts)
        // FIX: Use unsigned char to avoid signed comparison issues with high-bit characters
        unsigned char min_char_lower = case_insensitive ? (min_char | 0x20) : static_cast<unsigned char>(min_char);
        unsigned char max_char_lower = case_insensitive ? (max_char | 0x20) : static_cast<unsigned char>(max_char);

        for (; current != last && (MaxCount == 0 || count < MaxCount); ++current, ++count) {
            unsigned char char_val = static_cast<unsigned char>(*current);
            if (case_insensitive) {
                char_val |= 0x20; // Convert to lowercase
            }
            bool in_range = (char_val >= min_char_lower && char_val <= max_char_lower);
            if constexpr (is_negated) {
                if (in_range)
                    break; // For negated, stop if char IS in range
            } else {
                if (!in_range)
                    break; // For normal, stop if char NOT in range
            }
        }

        return current;
    } else {
        // Fallback for patterns without min_char/max_char traits
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
            // For patterns without match_char, fall back to general evaluation
            break;
        }
    }
    return current;
}

// ============================================================================
// SINGLE CHARACTER SIMD IMPLEMENTATIONS
// ============================================================================

// AVX2 version for single character repetition (optimal for patterns like 'a*', 'b+')
template <char TargetChar, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_single_char_repeat_avx2(Iterator current, const EndIterator& last, const flags& flags,
                                              size_t& count) {
    const bool case_insensitive = is_ascii_alpha(TargetChar) && ctre::is_case_insensitive(flags);
    const __m256i target_vec = _mm256_set1_epi8(TargetChar);
    const __m256i target_lower_vec = case_insensitive ? _mm256_set1_epi8(TargetChar | 0x20) : target_vec;
    const __m256i all_ones = _mm256_set1_epi8(0xFF); // PERF: Hoist out of loops

    // PERF: 16-byte fast path for inputs between 16-31 bytes
    if (has_at_least_bytes(current, last, 16) && !has_at_least_bytes(current, last, 32)) {
        __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&*current));
        __m128i target_vec_128 = _mm_set1_epi8(TargetChar);
        __m128i result;

        if (case_insensitive) {
            __m128i data_lower = _mm_or_si128(data, _mm_set1_epi8(0x20));
            __m128i target_lower = _mm_set1_epi8(TargetChar | 0x20);
            result = _mm_cmpeq_epi8(data_lower, target_lower);
        } else {
            result = _mm_cmpeq_epi8(data, target_vec_128);
        }

        // PERF: Use testz for faster all-match check (1 instruction vs movemask+cmp+ctz)
        if (_mm_test_all_ones(result)) {
            current += 16;
            count += 16;
        } else {
            int mask = _mm_movemask_epi8(result);
            int first_mismatch = __builtin_ctz(~mask);
            current += first_mismatch;
            count += first_mismatch;
            return current; // Early return: found mismatch
        }

        // Early return if no more data (exact 16-byte input)
        // C++20: [[likely]] attribute
        if (current >= last) [[likely]] {
            return current;
        }
    }

    // PERF: 32-byte fast path for inputs between 32-63 bytes
    if (has_at_least_bytes(current, last, 32) && !has_at_least_bytes(current, last, 64)) {
        __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*current));
        __m256i result;

        if (case_insensitive) {
            __m256i data_lower = _mm256_or_si256(data, _mm256_set1_epi8(0x20));
            result = _mm256_cmpeq_epi8(data_lower, target_lower_vec);
        } else {
            result = _mm256_cmpeq_epi8(data, target_vec);
        }

        // PERF: Use testc for faster all-match check
        if (_mm256_testc_si256(result, all_ones)) {
            current += 32;
            count += 32;
        } else {
            int mask = _mm256_movemask_epi8(result);
            int first_mismatch = __builtin_ctz(~mask);
            current += first_mismatch;
            count += first_mismatch;
            return current; // Early return: found mismatch
        }

        // Early return if no more data (exact 32-byte input)
        // C++20: [[likely]] for common case
        if (current >= last) [[likely]] {
            return current;
        }
    }

    // PERF: Process 64-byte chunks when possible (2x unroll reduces loop overhead)
    // C++20: Use [[unlikely]] for better branch prediction
    while (current != last && (MaxCount == 0 || count + 64 <= MaxCount)) {
        if (!has_at_least_bytes(current, last, 64)) [[unlikely]] {
            break; // Unlikely path for small inputs
        }

        __m256i data1 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*current));
        __m256i data2 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*(current + 32)));
        __m256i result1, result2;

        if (case_insensitive) {
            __m256i data1_lower = _mm256_or_si256(data1, _mm256_set1_epi8(0x20));
            __m256i data2_lower = _mm256_or_si256(data2, _mm256_set1_epi8(0x20));
            result1 = _mm256_cmpeq_epi8(data1_lower, target_lower_vec);
            result2 = _mm256_cmpeq_epi8(data2_lower, target_lower_vec);
        } else {
            result1 = _mm256_cmpeq_epi8(data1, target_vec);
            result2 = _mm256_cmpeq_epi8(data2, target_vec);
        }

        // PERF: Combine results first, then check (better ILP + fewer testc calls)
        // This saves one testc instruction in the hot path (both match)
        __m256i combined = _mm256_and_si256(result1, result2);

        // C++20: [[likely]] for hot path optimization
        if (_mm256_testc_si256(combined, all_ones)) [[likely]] {
            // Hot path: both chunks match (single testc check!)
            current += 64;
            count += 64;
        } else {
            // Slow path: Determine which chunk failed
            if (_mm256_testc_si256(result1, all_ones)) {
                // First chunk matches, mismatch in second
                int mask2 = _mm256_movemask_epi8(result2);
                int first_mismatch = __builtin_ctz(~mask2);
                current += 32 + first_mismatch;
                count += 32 + first_mismatch;
            } else {
                // Mismatch in first chunk
                int mask1 = _mm256_movemask_epi8(result1);
                int first_mismatch = __builtin_ctz(~mask1);
                current += first_mismatch;
                count += first_mismatch;
            }
            break;
        }
    }

    // Process remaining 32-byte chunks
    while (current != last && (MaxCount == 0 || count + 32 <= MaxCount)) {
        // C++20: [[unlikely]] for uncommon branch
        if (!has_at_least_bytes(current, last, 32)) [[unlikely]] {
            break; // Unlikely for most inputs
        }

        __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&*current));
        __m256i result;

        if (case_insensitive) {
            __m256i data_lower = _mm256_or_si256(data, _mm256_set1_epi8(0x20));
            result = _mm256_cmpeq_epi8(data_lower, target_lower_vec);
        } else {
            result = _mm256_cmpeq_epi8(data, target_vec);
        }

        // PERF: Hint that full matches are common (hot path)
        // C++20: [[likely]] for expected case
        if (_mm256_testc_si256(result, all_ones)) [[likely]] {
            current += 32;
            count += 32;
        } else {
            int mask = _mm256_movemask_epi8(result);
            int first_mismatch = __builtin_ctz(~mask);
            current += first_mismatch;
            count += first_mismatch;
            break;
        }
    }

    // Process remaining characters with scalar (much faster for small amounts)
    // FIX: Use unsigned char for consistency
    unsigned char target_char = case_insensitive ? (TargetChar | 0x20) : static_cast<unsigned char>(TargetChar);

    for (; current != last && (MaxCount == 0 || count < MaxCount); ++current, ++count) {
        unsigned char char_val = static_cast<unsigned char>(*current);
        if (case_insensitive) {
            char_val |= 0x20; // Convert to lowercase
        }
        if (char_val != target_char) {
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

    // Process full 16-byte chunks with bounds checking
    while (current != last && (MaxCount == 0 || count + 16 <= MaxCount)) {
        // SAFETY: Check if we have at least 16 bytes available
        if (!has_at_least_bytes(current, last, 16)) {
            break; // Not enough data, fall back to scalar tail
        }

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

    // Process remaining characters with scalar (much faster for small amounts)
    char target_char = case_insensitive ? (TargetChar | 0x20) : TargetChar;

    for (; current != last && (MaxCount == 0 || count < MaxCount); ++current, ++count) {
        char char_val = *current;
        if (case_insensitive) {
            char_val |= 0x20; // Convert to lowercase
        }
        if (char_val != target_char) {
            break;
        }
    }

    return current;
}

template <char TargetChar, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_single_char_repeat_scalar(Iterator current, const EndIterator& last, const flags& flags,
                                                size_t& count) {
    const bool case_insensitive = is_ascii_alpha(TargetChar) && ctre::is_case_insensitive(flags);

    char target_char = case_insensitive ? (TargetChar | 0x20) : TargetChar;

    for (; current != last && (MaxCount == 0 || count < MaxCount); ++current, ++count) {
        char char_val = *current;
        if (case_insensitive) {
            char_val |= 0x20; // Convert to lowercase
        }
        if (char_val != target_char) {
            break;
        }
    }

    return current;
}

} // namespace simd
} // namespace ctre

#endif // CTRE__SIMD_CHARACTER_CLASSES__HPP
