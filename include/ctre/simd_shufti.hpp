#ifndef CTRE__SIMD_SHUFTI__HPP
#define CTRE__SIMD_SHUFTI__HPP

#include "atoms.hpp"
#include "atoms_characters.hpp"
#include "flags_and_modes.hpp"
#include "simd_detection.hpp"
#include <array>
#include <cstring>
#ifdef CTRE_ARCH_X86
#include <immintrin.h>
#endif
#include <iterator>

namespace ctre {

struct zero_terminated_string_end_iterator;

namespace simd {

// Sentinel iterator detection
template <typename T>
struct is_sentinel_iterator : std::false_type {};
template <>
struct is_sentinel_iterator<ctre::zero_terminated_string_end_iterator> : std::true_type {};

// Null terminator scanning
#ifdef __AVX2__
inline const unsigned char* find_null_terminator_avx2(const unsigned char* p) {
    __m256i zero = _mm256_setzero_si256();
    const unsigned char* aligned = reinterpret_cast<const unsigned char*>((reinterpret_cast<uintptr_t>(p) + 31) & ~static_cast<uintptr_t>(31));
    for (const unsigned char* s = p; s < aligned && s < p + 32; ++s)
        if (*s == '\0')
            return s;
    for (const unsigned char* s = aligned;; s += 32) {
        int mask =
            _mm256_movemask_epi8(_mm256_cmpeq_epi8(_mm256_load_si256(reinterpret_cast<const __m256i*>(s)), zero));
        if (mask != 0)
            return s + static_cast<size_t>(__builtin_ctz(static_cast<unsigned>(mask)));
    }
}
#endif

inline const unsigned char* find_null_terminator_scalar(const unsigned char* p) {
    while (*p)
        ++p;
    return p;
}

template <typename Iterator, typename EndIterator>
inline const unsigned char* get_end_pointer(Iterator current, EndIterator last) {
    if constexpr (is_sentinel_iterator<std::remove_cvref_t<EndIterator>>::value) {
        const unsigned char* p = reinterpret_cast<const unsigned char*>(std::to_address(current));
#ifdef __AVX2__
        return find_null_terminator_avx2(p);
#else
        return find_null_terminator_scalar(p);
#endif
    } else {
        return reinterpret_cast<const unsigned char*>(std::to_address(last));
    }
}

// Sparse pattern detection
template <auto... Cs>
constexpr bool is_sparse_character_set() {
    if constexpr (sizeof...(Cs) < 5)
        return false;
    constexpr auto sorted = []() {
        std::array<int, sizeof...(Cs)> arr = {static_cast<int>(Cs)...};
        for (size_t i = 0; i < arr.size(); ++i)
            for (size_t j = i + 1; j < arr.size(); ++j)
                if (arr[i] > arr[j]) {
                    auto t = arr[i];
                    arr[i] = arr[j];
                    arr[j] = t;
                }
        return arr;
    }();
    size_t contiguous = 0;
    for (size_t i = 1; i < sorted.size(); ++i)
        if (sorted[i] == sorted[i - 1] + 1)
            ++contiguous;
    return contiguous < (sizeof...(Cs) / 2);
}

// Shufti pattern traits
template <typename PatternType>
struct shufti_pattern_trait {
    static constexpr bool is_shufti_optimizable = false;
    static constexpr bool should_use_shufti = false;
    static constexpr bool is_sparse = false;
    static constexpr bool is_negated = false;
};

template <typename... Content>
struct shufti_pattern_trait<set<Content...>> {
    static constexpr bool is_shufti_optimizable = true;
    static constexpr bool should_use_shufti = false;
    static constexpr bool is_sparse = false;
    static constexpr bool is_negated = false;
};

template <auto... Cs>
struct shufti_pattern_trait<set<character<Cs>...>> {
    static constexpr bool is_shufti_optimizable = true;
    static constexpr size_t num_chars = sizeof...(Cs);
    static constexpr bool is_sparse = is_sparse_character_set<Cs...>();
    static constexpr bool should_use_shufti = (num_chars >= 5 && num_chars <= 30) && is_sparse;
    static constexpr bool is_negated = false;
};

template <auto... Cs>
struct shufti_pattern_trait<negate<set<character<Cs>...>>> {
    static constexpr bool is_shufti_optimizable = true;
    static constexpr size_t num_chars = sizeof...(Cs);
    static constexpr bool is_sparse = is_sparse_character_set<Cs...>();
    static constexpr bool should_use_shufti = is_sparse;
    static constexpr bool is_negated = true;
};

template <auto... Cs>
struct shufti_pattern_trait<negative_set<character<Cs>...>> {
    static constexpr bool is_shufti_optimizable = true;
    static constexpr size_t num_chars = sizeof...(Cs);
    static constexpr bool is_sparse = is_sparse_character_set<Cs...>();
    static constexpr bool should_use_shufti = is_sparse;
    static constexpr bool is_negated = true;
};

template <auto... Cs>
struct shufti_pattern_trait<select<character<Cs>...>> {
    static constexpr size_t num_chars = sizeof...(Cs);
    static constexpr bool is_sparse = true;
    static constexpr bool should_use_shufti = (num_chars >= 5 && num_chars <= 30);
    static constexpr bool is_shufti_optimizable = should_use_shufti;
    static constexpr bool is_negated = false;
};

template <size_t Index, auto... Cs>
struct shufti_pattern_trait<capture<Index, select<character<Cs>...>>> {
    static constexpr size_t num_chars = sizeof...(Cs);
    static constexpr bool is_sparse = true;
    static constexpr bool should_use_shufti = (num_chars >= 5 && num_chars <= 30);
    static constexpr bool is_shufti_optimizable = should_use_shufti;
    static constexpr bool is_negated = false;
};

// Character class for Shufti
struct character_class {
    std::array<uint8_t, 16> upper_nibble_table{};
    std::array<uint8_t, 16> lower_nibble_table{};
    std::array<uint8_t, 16> upper_nibble_table2{};
    std::array<uint8_t, 16> lower_nibble_table2{};
    std::array<uint8_t, 256> exact_membership{};
    static constexpr uint8_t match_bit = 0x80;
    static constexpr uint8_t match_bit2 = 0x40;
    uint8_t density_estimate = 0;
    bool use_double_shufti = false;
    bool use_exact_range = false;

    constexpr void calculate_heuristics() {
        int hits = 0;
        for (int b = 0; b < 128; ++b)
            if ((upper_nibble_table[(b >> 4) & 0xF] & match_bit) && (lower_nibble_table[b & 0xF] & match_bit))
                ++hits;
        density_estimate = static_cast<uint8_t>((hits * 100) / 128);
        use_exact_range = (density_estimate > 40);
        if (!use_exact_range) {
            int double_hits = 0;
            for (int b = 0; b < 128; ++b) {
                uint8_t un = static_cast<uint8_t>((b >> 4) & 0xF), ln = static_cast<uint8_t>(b & 0xF), ln2 = static_cast<uint8_t>((ln + 7) % 16);
                if ((upper_nibble_table[un] & match_bit) && (lower_nibble_table[ln] & match_bit) &&
                    (upper_nibble_table2[un] & match_bit2) && (lower_nibble_table2[ln2] & match_bit2))
                    ++double_hits;
            }
            use_double_shufti = (hits > 0 && ((hits - double_hits) * 100 / hits) > 80);
        }
    }

    constexpr void init_from_predicate(auto pred) {
        for (auto& t : upper_nibble_table)
            t = 0;
        for (auto& t : lower_nibble_table)
            t = 0;
        for (auto& t : upper_nibble_table2)
            t = 0;
        for (auto& t : lower_nibble_table2)
            t = 0;
        for (auto& m : exact_membership)
            m = 0;
        for (size_t b = 0; b < 256; ++b) {
            if (pred(static_cast<int>(b))) {
                size_t un = (b >> 4) & 0xF, ln = b & 0xF;
                upper_nibble_table[un] |= match_bit;
                lower_nibble_table[ln] |= match_bit;
                upper_nibble_table2[un] |= match_bit2;
                lower_nibble_table2[(ln + 7) % 16] |= match_bit2;
                exact_membership[b] = 0xFF;
            }
        }
        calculate_heuristics();
    }

    constexpr void init_alnum() {
        init_from_predicate([](int b) {
            return (b >= '0' && b <= '9') || (b >= 'A' && b <= 'Z') || b == '_' || (b >= 'a' && b <= 'z');
        });
    }
    constexpr void init_whitespace() {
        init_from_predicate(
            [](int b) { return b == '\t' || b == '\n' || b == '\v' || b == '\f' || b == '\r' || b == ' '; });
    }
    constexpr void init_digits() {
        init_from_predicate([](int b) { return b >= '0' && b <= '9'; });
    }
    constexpr void init_letters() {
        init_from_predicate([](int b) { return (b >= 'A' && b <= 'Z') || (b >= 'a' && b <= 'z'); });
    }
};

#ifdef CTRE_ARCH_X86
// Exact range SIMD finders
namespace exact_range {
inline bool find_alnum_avx2(const unsigned char* p, const unsigned char* end, const unsigned char*& out) {
    if (p >= end)
        return false;
    size_t rem = static_cast<size_t>(end - p);
    auto in_range = [](__m256i x, unsigned lo, unsigned hi) {
        __m256i L = _mm256_set1_epi8(char(lo ^ 0x80)), H = _mm256_set1_epi8(char(hi ^ 0x80));
        return _mm256_and_si256(_mm256_xor_si256(_mm256_cmpgt_epi8(L, x), _mm256_set1_epi8(char(0xFF))),
                                _mm256_xor_si256(_mm256_cmpgt_epi8(x, H), _mm256_set1_epi8(char(0xFF))));
    };
    while (rem >= 32) {
        __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
        __m256i x = _mm256_xor_si256(v, _mm256_set1_epi8(char(0x80)));
        __m256i ok =
            _mm256_or_si256(_mm256_or_si256(in_range(x, '0', '9'), in_range(x, 'A', 'Z')),
                            _mm256_or_si256(in_range(x, 'a', 'z'), _mm256_cmpeq_epi8(v, _mm256_set1_epi8('_'))));
        int mask = _mm256_movemask_epi8(ok);
        if (mask != 0) {
            out = p + static_cast<size_t>(__builtin_ctz(static_cast<unsigned>(mask))) + 1;
            return true;
        }
        p += 32;
        rem -= 32;
    }
    while (p < end) {
        unsigned char c = *p;
        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_') {
            out = p + 1;
            return true;
        }
        ++p;
    }
    return false;
}

inline bool find_digits_avx2(const unsigned char* p, const unsigned char* end, const unsigned char*& out) {
    if (p >= end)
        return false;
    size_t rem = static_cast<size_t>(end - p);
    while (rem >= 32) {
        __m256i x =
            _mm256_xor_si256(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(p)), _mm256_set1_epi8(char(0x80)));
        __m256i L = _mm256_set1_epi8(char('0' ^ 0x80)), H = _mm256_set1_epi8(char('9' ^ 0x80));
        __m256i ok = _mm256_and_si256(_mm256_xor_si256(_mm256_cmpgt_epi8(L, x), _mm256_set1_epi8(char(0xFF))),
                                      _mm256_xor_si256(_mm256_cmpgt_epi8(x, H), _mm256_set1_epi8(char(0xFF))));
        int mask = _mm256_movemask_epi8(ok);
        if (mask != 0) {
            out = p + static_cast<size_t>(__builtin_ctz(static_cast<unsigned>(mask))) + 1;
            return true;
        }
        p += 32;
        rem -= 32;
    }
    while (p < end) {
        if (*p >= '0' && *p <= '9') {
            out = p + 1;
            return true;
        }
        ++p;
    }
    return false;
}

inline bool find_letters_avx2(const unsigned char* p, const unsigned char* end, const unsigned char*& out) {
    if (p >= end)
        return false;
    size_t rem = static_cast<size_t>(end - p);
    auto in_range = [](__m256i x, unsigned lo, unsigned hi) {
        __m256i L = _mm256_set1_epi8(char(lo ^ 0x80)), H = _mm256_set1_epi8(char(hi ^ 0x80));
        return _mm256_and_si256(_mm256_xor_si256(_mm256_cmpgt_epi8(L, x), _mm256_set1_epi8(char(0xFF))),
                                _mm256_xor_si256(_mm256_cmpgt_epi8(x, H), _mm256_set1_epi8(char(0xFF))));
    };
    while (rem >= 32) {
        __m256i x =
            _mm256_xor_si256(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(p)), _mm256_set1_epi8(char(0x80)));
        __m256i ok = _mm256_or_si256(in_range(x, 'A', 'Z'), in_range(x, 'a', 'z'));
        int mask = _mm256_movemask_epi8(ok);
        if (mask != 0) {
            out = p + static_cast<size_t>(__builtin_ctz(static_cast<unsigned>(mask))) + 1;
            return true;
        }
        p += 32;
        rem -= 32;
    }
    while (p < end) {
        unsigned char c = *p;
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
            out = p + 1;
            return true;
        }
        ++p;
    }
    return false;
}

inline bool find_whitespace_avx2(const unsigned char* p, const unsigned char* end, const unsigned char*& out) {
    if (p >= end)
        return false;
    size_t rem = static_cast<size_t>(end - p);
    while (rem >= 32) {
        __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
        __m256i ok = _mm256_or_si256(
            _mm256_or_si256(_mm256_cmpeq_epi8(v, _mm256_set1_epi8('\t')), _mm256_cmpeq_epi8(v, _mm256_set1_epi8('\n'))),
            _mm256_or_si256(_mm256_or_si256(_mm256_cmpeq_epi8(v, _mm256_set1_epi8('\v')),
                                            _mm256_cmpeq_epi8(v, _mm256_set1_epi8('\f'))),
                            _mm256_or_si256(_mm256_cmpeq_epi8(v, _mm256_set1_epi8('\r')),
                                            _mm256_cmpeq_epi8(v, _mm256_set1_epi8(' ')))));
        int mask = _mm256_movemask_epi8(ok);
        if (mask != 0) {
            out = p + static_cast<size_t>(__builtin_ctz(static_cast<unsigned>(mask))) + 1;
            return true;
        }
        p += 32;
        rem -= 32;
    }
    while (p < end) {
        unsigned char c = *p;
        if (c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r' || c == ' ') {
            out = p + 1;
            return true;
        }
        ++p;
    }
    return false;
}
} // namespace exact_range

// Shufti find implementations
inline bool shufti_find_avx2_single(const unsigned char* p, const unsigned char* end, const character_class& cc,
                                    const unsigned char*& out) {
    if (p >= end)
        return false;
    size_t rem = static_cast<size_t>(end - p);
    const __m256i upper_lut =
        _mm256_broadcastsi128_si256(_mm_loadu_si128(reinterpret_cast<const __m128i*>(cc.upper_nibble_table.data())));
    const __m256i lower_lut =
        _mm256_broadcastsi128_si256(_mm_loadu_si128(reinterpret_cast<const __m128i*>(cc.lower_nibble_table.data())));
    const __m256i nibble_mask = _mm256_set1_epi8(0x0F);
    while (rem >= 32) {
        __m256i input = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
        __m256i un = _mm256_and_si256(_mm256_srli_epi16(input, 4), nibble_mask);
        __m256i ln = _mm256_and_si256(input, nibble_mask);
        int mask = _mm256_movemask_epi8(
            _mm256_and_si256(_mm256_shuffle_epi8(upper_lut, un), _mm256_shuffle_epi8(lower_lut, ln)));
        if (mask != 0) {
            while (mask != 0) {
                size_t i = static_cast<size_t>(__builtin_ctz(static_cast<unsigned>(mask)));
                if (cc.exact_membership[p[i]]) {
                    out = p + i + 1;
                    return true;
                }
                mask &= mask - 1;
            }
        }
        p += 32;
        rem -= 32;
    }
#if defined(__SSSE3__) || defined(__SSE4_2__)
    if (rem >= 16) {
        __m128i input = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
        __m128i un = _mm_and_si128(_mm_srli_epi16(input, 4), _mm_set1_epi8(0x0F));
        __m128i ln = _mm_and_si128(input, _mm_set1_epi8(0x0F));
        int mask = _mm_movemask_epi8(_mm_and_si128(
            _mm_shuffle_epi8(_mm_loadu_si128(reinterpret_cast<const __m128i*>(cc.upper_nibble_table.data())), un),
            _mm_shuffle_epi8(_mm_loadu_si128(reinterpret_cast<const __m128i*>(cc.lower_nibble_table.data())), ln)));
        if (mask != 0) {
            while (mask != 0) {
                size_t i = static_cast<size_t>(__builtin_ctz(static_cast<unsigned>(mask)));
                if (cc.exact_membership[p[i]]) {
                    out = p + i + 1;
                    return true;
                }
                mask &= mask - 1;
            }
        }
        p += 16;
        rem -= 16;
    }
#endif
    while (p < end) {
        if (cc.exact_membership[*p]) {
            out = p + 1;
            return true;
        }
        ++p;
    }
    return false;
}

inline bool shufti_find_avx2_double(const unsigned char* p, const unsigned char* end, const character_class& cc,
                                    const unsigned char*& out) {
    if (p >= end)
        return false;
    size_t rem = static_cast<size_t>(end - p);
    const __m256i upper_lut =
        _mm256_broadcastsi128_si256(_mm_loadu_si128(reinterpret_cast<const __m128i*>(cc.upper_nibble_table.data())));
    const __m256i lower_lut =
        _mm256_broadcastsi128_si256(_mm_loadu_si128(reinterpret_cast<const __m128i*>(cc.lower_nibble_table.data())));
    const __m256i upper_lut2 =
        _mm256_broadcastsi128_si256(_mm_loadu_si128(reinterpret_cast<const __m128i*>(cc.upper_nibble_table2.data())));
    const __m256i lower_lut2 =
        _mm256_broadcastsi128_si256(_mm_loadu_si128(reinterpret_cast<const __m128i*>(cc.lower_nibble_table2.data())));
    const __m256i nibble_mask = _mm256_set1_epi8(0x0F);
    while (rem >= 32) {
        __m256i input = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
        __m256i un = _mm256_and_si256(_mm256_srli_epi16(input, 4), nibble_mask);
        __m256i ln = _mm256_and_si256(input, nibble_mask);
        __m256i c1 = _mm256_and_si256(_mm256_shuffle_epi8(upper_lut, un), _mm256_shuffle_epi8(lower_lut, ln));
        __m256i c2 = _mm256_and_si256(_mm256_shuffle_epi8(upper_lut2, un), _mm256_shuffle_epi8(lower_lut2, ln));
        int mask = _mm256_movemask_epi8(_mm256_and_si256(c1, c2));
        if (mask != 0) {
            while (mask != 0) {
                size_t i = static_cast<size_t>(__builtin_ctz(static_cast<unsigned>(mask)));
                if (cc.exact_membership[p[i]]) {
                    out = p + i + 1;
                    return true;
                }
                mask &= mask - 1;
            }
        }
        p += 32;
        rem -= 32;
    }
    while (p < end) {
        if (cc.exact_membership[*p]) {
            out = p + 1;
            return true;
        }
        ++p;
    }
    return false;
}

inline bool shufti_find_avx2(const unsigned char* p, const unsigned char* end, const character_class& cc,
                             const unsigned char*& out) {
    if (p >= end)
        return false;
    if (cc.use_exact_range)
        return exact_range::find_alnum_avx2(p, end, out);
    return cc.use_double_shufti ? shufti_find_avx2_double(p, end, cc, out) : shufti_find_avx2_single(p, end, cc, out);
}

inline bool shufti_find_ssse3(const unsigned char* p, const unsigned char* end, const character_class& cc,
                              const unsigned char*& out) {
    if (p >= end)
        return false;
    size_t rem = static_cast<size_t>(end - p);
    const __m128i upper_lut = _mm_loadu_si128(reinterpret_cast<const __m128i*>(cc.upper_nibble_table.data()));
    const __m128i lower_lut = _mm_loadu_si128(reinterpret_cast<const __m128i*>(cc.lower_nibble_table.data()));
    const __m128i upper_lut2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(cc.upper_nibble_table2.data()));
    const __m128i lower_lut2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(cc.lower_nibble_table2.data()));
    while (rem >= 16) {
        __m128i input = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
        __m128i un = _mm_and_si128(_mm_srli_epi16(input, 4), _mm_set1_epi8(0x0F));
        __m128i ln = _mm_and_si128(input, _mm_set1_epi8(0x0F));
        __m128i c1 = _mm_and_si128(_mm_shuffle_epi8(upper_lut, un), _mm_shuffle_epi8(lower_lut, ln));
        __m128i c2 = _mm_and_si128(_mm_shuffle_epi8(upper_lut2, un), _mm_shuffle_epi8(lower_lut2, ln));
        int mask = _mm_movemask_epi8(_mm_and_si128(c1, c2));
        if (mask != 0) {
            while (mask != 0) {
                size_t i = static_cast<size_t>(__builtin_ctz(static_cast<unsigned>(mask)));
                if (cc.exact_membership[p[i]]) {
                    out = p + i + 1;
                    return true;
                }
                mask &= mask - 1;
            }
        }
        p += 16;
        rem -= 16;
    }
    while (p < end) {
        if (cc.exact_membership[*p]) {
            out = p + 1;
            return true;
        }
        ++p;
    }
    return false;
}

// Template matchers
template <typename Iterator, typename EndIterator>
    requires std::contiguous_iterator<Iterator> &&
             std::is_same_v<std::remove_cvref_t<decltype(*std::declval<Iterator>())>, char>
inline bool match_char_class_shufti_avx2(Iterator& current, EndIterator last, const character_class& cc) {
    if (current == last)
        return false;
    const unsigned char* p = reinterpret_cast<const unsigned char*>(std::to_address(current));
    const unsigned char* end = get_end_pointer(current, last);
    const unsigned char* out;
    if (shufti_find_avx2(p, end, cc, out)) {
        using char_type = typename std::iterator_traits<Iterator>::value_type;
        current = Iterator(reinterpret_cast<const char_type*>(out));
        return true;
    }
    return false;
}

template <typename Iterator, typename EndIterator>
    requires std::contiguous_iterator<Iterator> &&
             std::is_same_v<std::remove_cvref_t<decltype(*std::declval<Iterator>())>, char>
inline bool match_char_class_shufti_ssse3(Iterator& current, EndIterator last, const character_class& cc) {
    if (current == last)
        return false;
    const unsigned char* p = reinterpret_cast<const unsigned char*>(std::to_address(current));
    const unsigned char* end = get_end_pointer(current, last);
    const unsigned char* out;
    if (shufti_find_ssse3(p, end, cc, out)) {
        using char_type = typename std::iterator_traits<Iterator>::value_type;
        current = Iterator(reinterpret_cast<const char_type*>(out));
        return true;
    }
    return false;
}

#else // !CTRE_ARCH_X86

// Fallback implementations for non-x86
template <typename Iterator, typename EndIterator>
inline bool match_char_class_shufti_avx2(Iterator& current, EndIterator last, const character_class& cc) {
    return false;
}
template <typename Iterator, typename EndIterator>
inline bool match_char_class_shufti_ssse3(Iterator& current, EndIterator last, const character_class& cc) {
    return false;
}

#endif // CTRE_ARCH_X86

template <typename Iterator, typename EndIterator>
    requires std::is_same_v<std::remove_cvref_t<decltype(*std::declval<Iterator>())>, char>
inline bool match_char_class_shufti_scalar(Iterator& current, EndIterator last, const character_class& cc) {
    if (current == last)
        return false;
    for (Iterator pos = current; pos != last; ++pos) {
        if (cc.exact_membership[static_cast<uint8_t>(*pos)]) {
            current = ++pos;
            return true;
        }
    }
    return false;
}

template <typename Iterator, typename EndIterator>
    requires std::is_same_v<std::remove_cvref_t<decltype(*std::declval<Iterator>())>, char>
inline bool match_char_class_shufti(Iterator& current, EndIterator last, const character_class& cc) {
    if constexpr (is_sentinel_iterator<std::remove_cvref_t<EndIterator>>::value)
        return false;
    else {
        if constexpr (CTRE_SIMD_ENABLED) {
#ifdef __AVX2__
            return match_char_class_shufti_avx2(current, last, cc);
#elif defined(__SSSE3__) || defined(__SSE4_2__)
            return match_char_class_shufti_ssse3(current, last, cc);
#else
            return match_char_class_shufti_scalar(current, last, cc);
#endif
        }
        return match_char_class_shufti_scalar(current, last, cc);
    }
}

// Character class initialization from pattern
template <auto... Chars>
constexpr character_class init_from_chars() {
    character_class c;
    for (auto& t : c.upper_nibble_table)
        t = 0;
    for (auto& t : c.lower_nibble_table)
        t = 0;
    for (auto& t : c.upper_nibble_table2)
        t = 0;
    for (auto& t : c.lower_nibble_table2)
        t = 0;
    for (auto& m : c.exact_membership)
        m = 0;
    (([&]() {
         constexpr uint8_t b = static_cast<uint8_t>(Chars);
         constexpr uint8_t un = (b >> 4) & 0xF, ln = b & 0xF;
         c.upper_nibble_table[un] |= c.match_bit;
         c.lower_nibble_table[ln] |= c.match_bit;
         c.upper_nibble_table2[un] |= c.match_bit2;
         c.lower_nibble_table2[(ln + 7) % 16] |= c.match_bit2;
         c.exact_membership[b] = 0xFF;
     }()),
     ...);
    c.calculate_heuristics();
    return c;
}

template <typename PatternType>
struct get_character_class_impl;
template <auto... Cs>
struct get_character_class_impl<set<character<Cs>...>> {
    static constexpr character_class value = init_from_chars<Cs...>();
};
template <auto... Cs>
struct get_character_class_impl<negate<set<character<Cs>...>>> {
    static constexpr character_class value = init_from_chars<Cs...>();
};
template <auto... Cs>
struct get_character_class_impl<negative_set<character<Cs>...>> {
    static constexpr character_class value = init_from_chars<Cs...>();
};
template <auto... Cs>
struct get_character_class_impl<select<character<Cs>...>> {
    static constexpr character_class value = init_from_chars<Cs...>();
};
template <size_t I, auto... Cs>
struct get_character_class_impl<capture<I, select<character<Cs>...>>> {
    static constexpr character_class value = init_from_chars<Cs...>();
};

template <typename PatternType>
constexpr character_class get_character_class_for_pattern() {
    if constexpr (shufti_pattern_trait<PatternType>::should_use_shufti)
        return get_character_class_impl<PatternType>::value;
    else {
        character_class c;
        c.init_alnum();
        return c;
    }
}

// Shufti repetition matching
template <typename PatternType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_pattern_repeat_shufti(Iterator current, EndIterator last, const flags&) {
    if constexpr (!shufti_pattern_trait<PatternType>::should_use_shufti)
        return current;
    Iterator start = current;
    if constexpr (is_sentinel_iterator<std::remove_cvref_t<EndIterator>>::value)
        return start;

    static constexpr character_class cc = get_character_class_for_pattern<PatternType>();
    if constexpr (std::contiguous_iterator<Iterator>) {
        const unsigned char* p = reinterpret_cast<const unsigned char*>(std::to_address(current));
        const unsigned char* end_ptr = get_end_pointer(current, last);
        size_t count = 0, rem = static_cast<size_t>(end_ptr - p);

#if defined(CTRE_ARCH_X86) && (defined(__SSE4_2__) || defined(__SSSE3__) || defined(__SSE2__))
        if (rem >= 16 && rem < 32) {
            const __m128i upper_lut = _mm_loadu_si128(reinterpret_cast<const __m128i*>(cc.upper_nibble_table.data()));
            const __m128i lower_lut = _mm_loadu_si128(reinterpret_cast<const __m128i*>(cc.lower_nibble_table.data()));
            const __m128i nibble_mask = _mm_set1_epi8(0x0F);
            while (rem >= 16 && (MaxCount == 0 || count + 16 <= MaxCount)) {
                __m128i input = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
                __m128i un = _mm_and_si128(_mm_srli_epi16(input, 4), nibble_mask);
                __m128i ln = _mm_and_si128(input, nibble_mask);
                int mask =
                    _mm_movemask_epi8(_mm_and_si128(_mm_shuffle_epi8(upper_lut, un), _mm_shuffle_epi8(lower_lut, ln)));
                if constexpr (shufti_pattern_trait<PatternType>::is_negated) {
                    if (mask == 0) {
                        count += 16;
                        p += 16;
                        rem -= 16;
                        continue;
                    }
                    int fp = __builtin_ctz(mask);
                    count += fp;
                    p += fp;
                    rem -= fp;
                    if (rem > 0 && cc.exact_membership[*p])
                        break;
                    ++count;
                    ++p;
                    --rem;
                } else {
                    if (mask == 0)
                        break;
                    for (int i = 0; i < 16 && rem > 0; ++i) {
                        if (!cc.exact_membership[p[i]]) {
                            p += i;
                            goto done_sse;
                        }
                        ++count;
                    }
                    p += 16;
                    rem -= 16;
                }
            }
        done_sse:;
        }
#endif

#if defined(CTRE_ARCH_X86) && defined(__AVX2__)
        if (rem >= 32) {
            const __m256i upper_lut = _mm256_broadcastsi128_si256(
                _mm_loadu_si128(reinterpret_cast<const __m128i*>(cc.upper_nibble_table.data())));
            const __m256i lower_lut = _mm256_broadcastsi128_si256(
                _mm_loadu_si128(reinterpret_cast<const __m128i*>(cc.lower_nibble_table.data())));
            const __m256i nibble_mask = _mm256_set1_epi8(0x0F);
            while (rem >= 32 && (MaxCount == 0 || count + 32 <= MaxCount)) {
                __m256i input = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
                __m256i un = _mm256_and_si256(_mm256_srli_epi16(input, 4), nibble_mask);
                __m256i ln = _mm256_and_si256(input, nibble_mask);
                int mask = _mm256_movemask_epi8(
                    _mm256_and_si256(_mm256_shuffle_epi8(upper_lut, un), _mm256_shuffle_epi8(lower_lut, ln)));
                if constexpr (shufti_pattern_trait<PatternType>::is_negated) {
                    if (mask == 0) {
                        count += 32;
                        p += 32;
                        rem -= 32;
                        continue;
                    }
                    int fp = __builtin_ctz(mask);
                    count += fp;
                    p += fp;
                    rem -= fp;
                    if (rem > 0 && cc.exact_membership[*p])
                        break;
                    ++count;
                    ++p;
                    --rem;
                } else {
                    if (mask == 0)
                        break;
                    for (int i = 0; i < 32 && rem > 0; ++i) {
                        if (!cc.exact_membership[p[i]]) {
                            p += i;
                            goto done_avx2;
                        }
                        ++count;
                    }
                    p += 32;
                    rem -= 32;
                }
            }
        done_avx2:;
        }
#endif

        while (rem > 0 && (MaxCount == 0 || count < MaxCount)) {
            bool matches =
                shufti_pattern_trait<PatternType>::is_negated ? !cc.exact_membership[*p] : cc.exact_membership[*p];
            if (matches) {
                ++p;
                ++count;
                --rem;
            } else
                break;
        }

        if (count >= MinCount) {
            return std::next(start, static_cast<typename std::iterator_traits<Iterator>::difference_type>(count));
        }
    }
    return start;
}

// Helper matchers
template <typename Iterator, typename EndIterator>
    requires std::is_same_v<std::remove_cvref_t<decltype(*std::declval<Iterator>())>, char>
inline bool match_alnum_shufti(Iterator& current, EndIterator last, const flags&) {
    if (current == last)
        return false;
    for (Iterator pos = current; pos != last;) {
        auto temp = pos;
        bool has_32 = true;
        for (int i = 0; i < 32; ++i) {
            if (temp == last) {
                has_32 = false;
                break;
            }
            ++temp;
        }
        if (!has_32)
            break;
        const char* data = &*pos;
        for (int i = 0; i < 32; ++i) {
            unsigned char c = static_cast<unsigned char>(data[i]);
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_') {
                std::advance(pos, i);
                current = pos;
                return true;
            }
        }
        std::advance(pos, 32);
    }
    for (Iterator pos = current; pos != last; ++pos) {
        unsigned char c = static_cast<unsigned char>(*pos);
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_') {
            current = ++pos;
            return true;
        }
    }
    return false;
}

template <typename Iterator, typename EndIterator>
    requires std::is_same_v<std::remove_cvref_t<decltype(*std::declval<Iterator>())>, char>
inline bool match_whitespace_shufti(Iterator& current, EndIterator last, const flags&) {
    static constexpr character_class ws = []() {
        character_class c;
        c.init_whitespace();
        return c;
    }();
    return match_char_class_shufti(current, last, ws);
}

template <typename Iterator, typename EndIterator>
    requires std::is_same_v<std::remove_reference_t<decltype(*std::declval<Iterator>())>, char>
inline bool match_digits_shufti(Iterator& current, EndIterator last, const flags&) {
    static constexpr character_class d = []() {
        character_class c;
        c.init_digits();
        return c;
    }();
    return match_char_class_shufti(current, last, d);
}

template <typename Iterator, typename EndIterator>
    requires std::is_same_v<std::remove_cvref_t<decltype(*std::declval<Iterator>())>, char>
inline bool match_letters_shufti(Iterator& current, EndIterator last, const flags&) {
    static constexpr character_class l = []() {
        character_class c;
        c.init_letters();
        return c;
    }();
    return match_char_class_shufti(current, last, l);
}

} // namespace simd
} // namespace ctre

#endif
