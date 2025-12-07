#ifndef CTRE_BITNFA_SIMD_ACCELERATION_HPP
#define CTRE_BITNFA_SIMD_ACCELERATION_HPP

#include "../simd_detection.hpp"
#include "../simd_character_classes.hpp"
#include "../atoms.hpp"
#include <cstddef>
#include <cstring>
#include <string_view>
#ifdef CTRE_ARCH_X86
#include <immintrin.h>
#endif

namespace ctre::bitnfa {

// Find next position where a character class STOPS matching
template <typename CharClassType>
[[nodiscard]] inline const char* simd_find_char_class_end(const char* begin, const char* end) noexcept {
    if constexpr (requires { simd::simd_pattern_trait<CharClassType>::min_char;
                             simd::simd_pattern_trait<CharClassType>::max_char; }) {
        constexpr char min_char = simd::simd_pattern_trait<CharClassType>::min_char;
        constexpr char max_char = simd::simd_pattern_trait<CharClassType>::max_char;
        const char* current = begin;

#ifdef CTRE_ARCH_X86
        if (simd::can_use_simd() && simd::get_simd_capability() >= simd::SIMD_CAPABILITY_AVX2) {
            const char* scan_end = end - 32;
            __m256i min_vec = _mm256_set1_epi8(min_char);
            __m256i max_vec = _mm256_set1_epi8(max_char);

            while (current <= scan_end) {
                __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(current));
                __m256i lt_min = _mm256_cmpgt_epi8(min_vec, data);
                __m256i ge_min = _mm256_xor_si256(lt_min, _mm256_set1_epi8(static_cast<char>(0xFF)));
                __m256i gt_max = _mm256_cmpgt_epi8(data, max_vec);
                __m256i le_max = _mm256_xor_si256(gt_max, _mm256_set1_epi8(static_cast<char>(0xFF)));
                __m256i result = _mm256_and_si256(ge_min, le_max);

                int mask = _mm256_movemask_epi8(result);
                if (static_cast<unsigned>(mask) != 0xFFFFFFFFU) {
                    return current + __builtin_ctz(~mask);
                }
                current += 32;
            }
        }
#endif

        while (current < end &&
               static_cast<unsigned char>(*current) >= static_cast<unsigned char>(min_char) &&
               static_cast<unsigned char>(*current) <= static_cast<unsigned char>(max_char)) {
            ++current;
        }
        return current;
    } else {
        const char* current = begin;
        while (current < end && CharClassType::match_char(*current, flags{})) ++current;
        return current;
    }
}

// Find next position where a character class matches
template <typename CharClassType>
[[nodiscard]] inline const char* simd_find_char_class(const char* begin, const char* end) noexcept {
#ifndef CTRE_ARCH_X86
    // Scalar fallback for non-x86
    for (const char* p = begin; p != end; ++p)
        if (CharClassType::match_char(*p, flags{})) return p;
    return end;
#else
    if (!simd::can_use_simd()) {
        for (const char* p = begin; p != end; ++p)
            if (CharClassType::match_char(*p, flags{})) return p;
        return end;
    }

    if constexpr (requires { simd::simd_pattern_trait<CharClassType>::min_char;
                             simd::simd_pattern_trait<CharClassType>::max_char; }) {
        constexpr char min_char = simd::simd_pattern_trait<CharClassType>::min_char;
        constexpr char max_char = simd::simd_pattern_trait<CharClassType>::max_char;

        if (simd::get_simd_capability() >= simd::SIMD_CAPABILITY_AVX2) {
            const char* current = begin;
            const char* scan_end = end - 32;
            __m256i min_vec = _mm256_set1_epi8(min_char);
            __m256i max_vec = _mm256_set1_epi8(max_char);

            while (current <= scan_end) {
                __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(current));
                __m256i lt_min = _mm256_cmpgt_epi8(min_vec, data);
                __m256i ge_min = _mm256_xor_si256(lt_min, _mm256_set1_epi8(static_cast<char>(0xFF)));
                __m256i gt_max = _mm256_cmpgt_epi8(data, max_vec);
                __m256i le_max = _mm256_xor_si256(gt_max, _mm256_set1_epi8(static_cast<char>(0xFF)));
                __m256i result = _mm256_and_si256(ge_min, le_max);

                int mask = _mm256_movemask_epi8(result);
                if (mask != 0) return current + __builtin_ctz(mask);
                current += 32;
            }

            for (; current != end; ++current) {
                if (static_cast<unsigned char>(*current) >= static_cast<unsigned char>(min_char) &&
                    static_cast<unsigned char>(*current) <= static_cast<unsigned char>(max_char))
                    return current;
            }
            return end;
        } else if (simd::get_simd_capability() >= simd::SIMD_CAPABILITY_SSE42) {
            const char* current = begin;
            const char* scan_end = end - 16;
            __m128i min_vec = _mm_set1_epi8(min_char);
            __m128i max_vec = _mm_set1_epi8(max_char);

            while (current <= scan_end) {
                __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(current));
                __m128i lt_min = _mm_cmpgt_epi8(min_vec, data);
                __m128i ge_min = _mm_xor_si128(lt_min, _mm_set1_epi8(static_cast<char>(0xFF)));
                __m128i gt_max = _mm_cmpgt_epi8(data, max_vec);
                __m128i le_max = _mm_xor_si128(gt_max, _mm_set1_epi8(static_cast<char>(0xFF)));
                __m128i result = _mm_and_si128(ge_min, le_max);

                int mask = _mm_movemask_epi8(result);
                if (mask != 0) return current + __builtin_ctz(mask);
                current += 16;
            }

            for (; current != end; ++current) {
                if (static_cast<unsigned char>(*current) >= static_cast<unsigned char>(min_char) &&
                    static_cast<unsigned char>(*current) <= static_cast<unsigned char>(max_char))
                    return current;
            }
            return end;
        }
    }

    for (const char* p = begin; p != end; ++p)
        if (CharClassType::match_char(*p, flags{})) return p;
    return end;
#endif // CTRE_ARCH_X86
}

// Find next single character
[[nodiscard]] inline const char* simd_find_char(const char* begin, const char* end, char target) noexcept {
#ifndef CTRE_ARCH_X86
    const char* r = static_cast<const char*>(memchr(begin, target, static_cast<size_t>(end - begin)));
    return r ? r : end;
#else
    if (!simd::can_use_simd() || end - begin < 16) {
        const char* r = static_cast<const char*>(memchr(begin, target, static_cast<size_t>(end - begin)));
        return r ? r : end;
    }

    if (simd::get_simd_capability() >= simd::SIMD_CAPABILITY_AVX2) {
        const char* current = begin;
        const char* scan_end = end - 32;
        __m256i target_vec = _mm256_set1_epi8(target);

        while (current <= scan_end) {
            __m256i data = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(current));
            int mask = _mm256_movemask_epi8(_mm256_cmpeq_epi8(data, target_vec));
            if (mask != 0) return current + static_cast<size_t>(__builtin_ctz(static_cast<unsigned>(mask)));
            current += 32;
        }

        for (; current != end; ++current)
            if (*current == target) return current;
        return end;
    }

    const char* r = static_cast<const char*>(memchr(begin, target, static_cast<size_t>(end - begin)));
    return r ? r : end;
#endif // CTRE_ARCH_X86
}

// Pattern Analysis for Acceleration
template <typename AST> struct can_accelerate : std::false_type {};

template <size_t A, size_t B, typename Content>
struct can_accelerate<ctre::repeat<A, B, Content>> {
    static constexpr bool value = requires {
        { simd::simd_pattern_trait<Content>::min_char } -> std::convertible_to<char>;
        { simd::simd_pattern_trait<Content>::max_char } -> std::convertible_to<char>;
    };
};

template <typename T> inline constexpr bool can_accelerate_v = can_accelerate<T>::value;

template <typename T> struct extract_repeat_content;
template <size_t A, size_t B, typename Content>
struct extract_repeat_content<ctre::repeat<A, B, Content>> { using type = Content; };
template <typename T> using extract_repeat_content_t = typename extract_repeat_content<T>::type;

} // namespace ctre::bitnfa

#endif // CTRE_BITNFA_SIMD_ACCELERATION_HPP
