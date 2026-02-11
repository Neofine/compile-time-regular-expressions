#ifndef CTRE__SIMD_SHIFT_OR__HPP
#define CTRE__SIMD_SHIFT_OR__HPP

#include "flags_and_modes.hpp"
#include "simd_detection.hpp"
#include <array>
#include <cstring>
#ifdef CTRE_ARCH_X86
#include <immintrin.h>
#endif
#include <iterator>

#if defined(__GNUC__) || defined(__clang__)
#define HOT_ALWAYS_INLINE [[gnu::always_inline]] inline
#else
#define HOT_ALWAYS_INLINE __forceinline
#endif

namespace ctre::simd {

// Forward declaration for verify_equal (used in prefilter functions)
template <size_t N>
HOT_ALWAYS_INLINE bool verify_equal(const unsigned char* s, const char* pat);

template <typename It>
HOT_ALWAYS_INLINE It uchar_to_iter(const unsigned char* p) {
    // Need const_cast because the internal pointer is const but iterators may need non-const
    using char_type = typename std::iterator_traits<It>::value_type;
    return It(const_cast<char_type*>(reinterpret_cast<const char_type*>(p)));
}

constexpr size_t SHIFT_OR_THRESHOLD = 32;

constexpr size_t MAX_SHIFT_OR_PATTERN_LENGTH = 64;

template <size_t N>
using mask_t =
    std::conditional_t<(N <= 8), uint8_t,
                       std::conditional_t<(N <= 16), uint16_t, std::conditional_t<(N <= 32), uint32_t, uint64_t>>>;

template <size_t PatternLength>
struct alignas(64) shift_or_state {
    static_assert(PatternLength > 0, "Pattern length must be positive");
    static_assert(PatternLength <= MAX_SHIFT_OR_PATTERN_LENGTH, "Pattern too long for Shift-Or");

    using M = mask_t<PatternLength>;

    alignas(32) std::array<M, 256> char_masks;

    template <auto... Chars>
    constexpr void init_exact_pattern() {
        constexpr char pattern[] = {static_cast<char>(Chars)...};
        static_assert(sizeof...(Chars) == PatternLength, "Pattern length mismatch");

        for (auto& mask : char_masks) {
            mask = ~M(0);
        }

        // Set 0 bits for matching characters at each position (Shift-Or uses 0=good, 1=bad)
        for (size_t i = 0; i < PatternLength; ++i) {
            char_masks[static_cast<unsigned char>(pattern[i])] &= static_cast<M>(~(M(1) << i));
        }
    }

    template <typename CharClass>
    constexpr void init_char_class_pattern() {
        for (auto& mask : char_masks) {
            mask = ~M(0);
        }

        for (size_t i = 0; i < PatternLength; ++i) {
            for (int c = 0; c < 256; ++c) {
                if (CharClass::match_char(static_cast<char>(c), flags{})) {
                    char_masks[c] &= ~(M(1) << i);
                }
            }
        }
    }
};

template <size_t PatternLength, typename It, typename EndIt>
    requires std::contiguous_iterator<It> && std::same_as<std::remove_cvref_t<decltype(*std::declval<It>())>, char>
inline bool match_shift_or_unrolled16(It& cur, const EndIt last, const shift_or_state<PatternLength>& st) {
    if (cur == last)
        return false;

    using M = typename shift_or_state<PatternLength>::M;
    const unsigned char* base = reinterpret_cast<const unsigned char*>(std::to_address(cur));
    const unsigned char* p = base;
    const unsigned char* end = reinterpret_cast<const unsigned char*>(std::to_address(last));
    const M* __restrict cm = st.char_masks.data();

    uint64_t D = ~0ull;
    const uint64_t MSB = 1ull << (PatternLength - 1);

    while (size_t(end - p) >= 16) {
        uint32_t hits = 0;
#define STEP(j)                                                                                                        \
    do {                                                                                                               \
        uint64_t T = (D << 1) | (uint64_t)cm[p[(j)]];                                                                  \
        hits |= ((~T >> (PatternLength - 1)) & 1u) << (j);                                                             \
        D = T;                                                                                                         \
    } while (0)

        STEP(0);
        STEP(1);
        STEP(2);
        STEP(3);
        if (__builtin_expect(hits != 0, 0)) {
            cur = uchar_to_iter<It>(p + __builtin_ctz(hits) + 1);
            return true;
        }
        STEP(4);
        STEP(5);
        STEP(6);
        STEP(7);
        if (__builtin_expect(hits != 0, 0)) {
            cur = uchar_to_iter<It>(p + __builtin_ctz(hits) + 1);
            return true;
        }
        STEP(8);
        STEP(9);
        STEP(10);
        STEP(11);
        if (__builtin_expect(hits != 0, 0)) {
            cur = uchar_to_iter<It>(p + __builtin_ctz(hits) + 1);
            return true;
        }
        STEP(12);
        STEP(13);
        STEP(14);
        STEP(15);
        if (__builtin_expect(hits != 0, 0)) {
            cur = uchar_to_iter<It>(p + __builtin_ctz(hits) + 1);
            return true;
        }

#undef STEP
        p += 16;
    }

    while (p < end) {
        D = (D << 1) | (uint64_t)cm[*p++];
        if (__builtin_expect(!(D & MSB), 0)) {
            cur = uchar_to_iter<It>(p);
            return true;
        }
    }
    return false;
}

template <size_t PatternLength, typename It, typename EndIt>
    requires std::contiguous_iterator<It> && std::same_as<std::remove_cvref_t<decltype(*std::declval<It>())>, char>
inline bool match_shift_or_unrolled8(It& cur, const EndIt last, const shift_or_state<PatternLength>& st) {
    if (cur == last)
        return false;

    using M = typename shift_or_state<PatternLength>::M;
    const unsigned char* base = reinterpret_cast<const unsigned char*>(std::to_address(cur));
    const unsigned char* p = base;
    const unsigned char* end = reinterpret_cast<const unsigned char*>(std::to_address(last));
    const M* __restrict cm = st.char_masks.data();

    uint64_t D = ~0ull;
    const uint64_t MSB = 1ull << (PatternLength - 1);

    while (size_t(end - p) >= 8) {
        uint32_t hits = 0;
#define STEP(j)                                                                                                        \
    do {                                                                                                               \
        uint64_t T = (D << 1) | (uint64_t)cm[p[(j)]];                                                                  \
        hits |= ((~T >> (PatternLength - 1)) & 1u) << (j);                                                             \
        D = T;                                                                                                         \
    } while (0)

        STEP(0);
        STEP(1);
        STEP(2);
        STEP(3);
        if (__builtin_expect(hits != 0, 0)) {
            cur = uchar_to_iter<It>(p + __builtin_ctz(hits) + 1);
            return true;
        }
        STEP(4);
        STEP(5);
        STEP(6);
        STEP(7);
        if (__builtin_expect(hits != 0, 0)) {
            cur = uchar_to_iter<It>(p + __builtin_ctz(hits) + 1);
            return true;
        }

#undef STEP
        p += 8;
    }

    while (p < end) {
        D = (D << 1) | (uint64_t)cm[*p++];
        if (__builtin_expect(!(D & MSB), 0)) {
            cur = uchar_to_iter<It>(p);
            return true;
        }
    }
    return false;
}

template <size_t PatternLength, typename It, typename EndIt>
    requires std::contiguous_iterator<It> && std::same_as<std::remove_cvref_t<decltype(*std::declval<It>())>, char>
inline bool match_shift_or_scalar(It& cur, const EndIt last, const shift_or_state<PatternLength>& st) {
    if (cur == last)
        return false;

    using M = typename shift_or_state<PatternLength>::M;
    const unsigned char* base = reinterpret_cast<const unsigned char*>(std::to_address(cur));
    const unsigned char* p = base;
    const unsigned char* end = reinterpret_cast<const unsigned char*>(std::to_address(last));
    const M* __restrict cm = st.char_masks.data();

    uint64_t D = ~0ull;
    const uint64_t MSB = 1ull << (PatternLength - 1);

    while (size_t(end - p) >= 4) {
        D = (D << 1) | (uint64_t)cm[p[0]];
        if (__builtin_expect(!(D & MSB), 0)) {
            cur = uchar_to_iter<It>(p + 1);
            return true;
        }
        D = (D << 1) | (uint64_t)cm[p[1]];
        if (__builtin_expect(!(D & MSB), 0)) {
            std::advance(cur, (p + 2) - base);
            return true;
        }
        D = (D << 1) | (uint64_t)cm[p[2]];
        if (__builtin_expect(!(D & MSB), 0)) {
            std::advance(cur, (p + 3) - base);
            return true;
        }
        D = (D << 1) | (uint64_t)cm[p[3]];
        if (__builtin_expect(!(D & MSB), 0)) {
            std::advance(cur, (p + 4) - base);
            return true;
        }
        p += 4;
    }

    while (p < end) {
        D = (D << 1) | (uint64_t)cm[*p++];
        if (__builtin_expect(!(D & MSB), 0)) {
            cur = uchar_to_iter<It>(p);
            return true;
        }
    }
    return false;
}

template <size_t PatternLength, typename It, typename EndIt>
    requires std::contiguous_iterator<It> && std::same_as<std::remove_cvref_t<decltype(*std::declval<It>())>, char>
inline bool match_shift_or(It& current, const EndIt last, const shift_or_state<PatternLength>& state) {
    const size_t haystack_size = std::to_address(last) - std::to_address(current);

    if constexpr (PatternLength <= 16) {
        if (haystack_size > SHIFT_OR_THRESHOLD * 1024) {
        }
    }

    if constexpr (CTRE_SIMD_ENABLED) {
        if (get_simd_capability() >= SIMD_CAPABILITY_AVX2) {
            return match_shift_or_unrolled16<PatternLength>(current, last, state);
        } else if (get_simd_capability() >= SIMD_CAPABILITY_SSE42) {
            return match_shift_or_unrolled8<PatternLength>(current, last, state);
        }
    }

    return match_shift_or_scalar<PatternLength>(current, last, state);
}

#ifdef CTRE_ARCH_X86
template <size_t N, typename It, typename EndIt>
    requires(N > 1 && N <= 32) && std::contiguous_iterator<It> &&
            std::same_as<std::remove_cvref_t<decltype(*std::declval<It>())>, char>
inline bool match_string_prefilter_2bytes_sse2(It& cur, EndIt last, const char* pat) {
    if (cur == last)
        return false;
    const unsigned char* base = reinterpret_cast<const unsigned char*>(std::to_address(cur));
    const unsigned char* p = base;
    const unsigned char* end = reinterpret_cast<const unsigned char*>(std::to_address(last));
    const __m128i f = _mm_set1_epi8(static_cast<char>(pat[0]));
    const __m128i l = _mm_set1_epi8(static_cast<char>(pat[N - 1]));

    while (p + 16 + (N - 1) <= end) {
        __m128i v0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
        __m128i v1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p + (N - 1)));
        unsigned m0 = static_cast<unsigned>(_mm_movemask_epi8(_mm_cmpeq_epi8(v0, f)));
        unsigned m1 = static_cast<unsigned>(_mm_movemask_epi8(_mm_cmpeq_epi8(v1, l)));
        unsigned cand = m0 & (m1 << (N - 1));
        cand &= ~0u >> (N - 1);

        while (cand) {
            int i = __builtin_ctz(cand);
            if (verify_equal<N>(p + i, pat)) {
                cur = uchar_to_iter<It>(p + i + N);
                return true;
            }
            cand &= cand - 1;
        }
        p += 16 - (N - 1);
    }

    while (p < end) {
        auto* hit = static_cast<const unsigned char*>(std::memchr(p, static_cast<unsigned char>(pat[0]), end - p));
        if (!hit)
            break;
        if (hit + N <= end && verify_equal<N>(hit, pat)) {
            cur = uchar_to_iter<It>(hit + N);
            return true;
        }
        p = hit + 1;
    }
    return false;
}

template <size_t N>
HOT_ALWAYS_INLINE bool verify_equal(const unsigned char* s, const char* pat) {
    if constexpr (N <= 16) {
        __m128i a = _mm_loadu_si128(reinterpret_cast<const __m128i*>(s));
        __m128i b = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pat));
        alignas(16) static constexpr uint8_t maskBytes[16] = {
            (uint8_t) - (0 < N),  (uint8_t) - (1 < N),  (uint8_t) - (2 < N),  (uint8_t) - (3 < N),
            (uint8_t) - (4 < N),  (uint8_t) - (5 < N),  (uint8_t) - (6 < N),  (uint8_t) - (7 < N),
            (uint8_t) - (8 < N),  (uint8_t) - (9 < N),  (uint8_t) - (10 < N), (uint8_t) - (11 < N),
            (uint8_t) - (12 < N), (uint8_t) - (13 < N), (uint8_t) - (14 < N), (uint8_t) - (15 < N)};
        __m128i m = _mm_load_si128(reinterpret_cast<const __m128i*>(maskBytes));
        __m128i d = _mm_xor_si128(a, b);
        return _mm_testz_si128(_mm_and_si128(d, m), m);
    } else if constexpr (N <= 32) {
        return verify_equal<16>(s, pat) && verify_equal<N - 16>(s + 16, pat + 16);
    } else if constexpr (N <= 48) {
        return verify_equal<32>(s, pat) && verify_equal<N - 32>(s + 32, pat + 32);
    } else if constexpr (N <= 64) {
        return verify_equal<48>(s, pat) && verify_equal<N - 48>(s + 48, pat + 48);
    } else {
        return std::memcmp(s, pat, N) == 0;
    }
}

template <size_t N, typename It, typename EndIt>
    requires(N > 1 && N <= 32) && std::contiguous_iterator<It> &&
            std::same_as<std::remove_cvref_t<decltype(*std::declval<It>())>, char>
inline bool match_string_prefilter_2bytes(It& cur, EndIt last, const char* pat) {
    if (cur == last)
        return false;

    constexpr int sh = int(N) - 1;
    const unsigned char* base = reinterpret_cast<const unsigned char*>(std::to_address(cur));
    const unsigned char* p = base;
    const unsigned char* end = reinterpret_cast<const unsigned char*>(std::to_address(last));

    const __m256i f = _mm256_set1_epi8(pat[0]);
    const __m256i l = _mm256_set1_epi8(pat[sh]);

    uint32_t m1_next = 0;
    if (p + 32 + sh > end)
        goto tail;

    m1_next =
        _mm256_movemask_epi8(_mm256_cmpeq_epi8(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(p + 32 + sh)), l));

    for (;;) {
        __m256i v0 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
        __m256i v1_cur = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p + sh));

        uint32_t m0 = _mm256_movemask_epi8(_mm256_cmpeq_epi8(v0, f));
        uint32_t m1_cur = _mm256_movemask_epi8(_mm256_cmpeq_epi8(v1_cur, l));

        // Carry-based alignment: combine current and next block masks to avoid overlap
        uint32_t m1_aligned = (m1_cur << sh) | (m1_next >> (32 - sh));
        uint32_t cand = m0 & m1_aligned;

        while (cand) {
            int i = __builtin_ctz(cand);
            const unsigned char* s = p + i;
            if (verify_equal<N>(s, pat)) {
                cur = uchar_to_iter<It>(s + N);
                return true;
            }
            cand &= cand - 1;
        }

        p += 32;
        if (p + 32 + sh > end)
            break;

        m1_next = _mm256_movemask_epi8(
            _mm256_cmpeq_epi8(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(p + 32 + sh)), l));
    }

tail:
    if (p + 32 + sh <= end) {
        __m256i v0 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
        __m256i v1_cur = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p + sh));
        uint32_t m0 = _mm256_movemask_epi8(_mm256_cmpeq_epi8(v0, f));
        uint32_t m1_cur = _mm256_movemask_epi8(_mm256_cmpeq_epi8(v1_cur, l));
        uint32_t m1_aligned = (m1_cur << sh); // no next block available
        uint32_t cand = m0 & m1_aligned;
        while (cand) {
            int i = __builtin_ctz(cand);
            const unsigned char* s = p + i;
            if (verify_equal<N>(s, pat)) {
                cur = uchar_to_iter<It>(s + N);
                return true;
            }
            cand &= cand - 1;
        }
        p += 32;
    }

    while (p < end) {
        auto* hit = static_cast<const unsigned char*>(std::memchr(p, static_cast<unsigned char>(pat[0]), end - p));
        if (!hit)
            break;
        if (hit + N <= end && verify_equal<N>(hit, pat)) {
            cur = uchar_to_iter<It>(hit + N);
            return true;
        }
        p = hit + 1;
    }
    return false;
}

template <size_t N, typename It, typename EndIt>
    requires(N == 1) && std::contiguous_iterator<It> &&
            std::same_as<std::remove_cvref_t<decltype(*std::declval<It>())>, char>
inline bool match_string_prefilter_2bytes(It& cur, EndIt last, const char* pat) {
    if (cur == last)
        return false;

    const unsigned char* base = reinterpret_cast<const unsigned char*>(std::to_address(cur));
    const unsigned char* p = base;
    const unsigned char* end = reinterpret_cast<const unsigned char*>(std::to_address(last));
    const unsigned char target = static_cast<unsigned char>(pat[0]);

    while (p + 32 <= end) {
        __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
        __m256i eq0 = _mm256_cmpeq_epi8(v, _mm256_set1_epi8(target));
        int mask = _mm256_movemask_epi8(eq0);

        if (mask) {
            int i = __builtin_ctz(mask);
            cur = uchar_to_iter<It>(p + i + 1);
            return true;
        }
        p += 32;
    }

    p = static_cast<const unsigned char*>(std::memchr(p, target, end - p));
    if (p) {
        cur = uchar_to_iter<It>(p + 1);
        return true;
    }

    return false;
}

template <size_t PatternLength, typename It, typename EndIt>
    requires std::contiguous_iterator<It> && std::same_as<std::remove_cvref_t<decltype(*std::declval<It>())>, char>
inline bool match_string_vector_prefilter(It& cur, const EndIt last, const char* pattern) {
#if CTRE_SIMD_ENABLED
    // Use AVX2 version if available
    if (get_simd_capability() >= SIMD_CAPABILITY_AVX2) {
        return match_string_prefilter_2bytes<PatternLength>(cur, last, pattern);
    }
    // Fall back to SSE2 version
    else if (get_simd_capability() >= SIMD_CAPABILITY_SSE42) {
        return match_string_prefilter_2bytes_sse2<PatternLength>(cur, last, pattern);
    }
#endif
    return false;
}

template <size_t K, typename It, typename EndIt>
    requires std::contiguous_iterator<It> && std::same_as<std::remove_cvref_t<decltype(*std::declval<It>())>, char>
inline bool match_class_run_shufti(It& cur, EndIt last, const auto& cc) {
    if (cur == last)
        return false;
    const unsigned char* p = reinterpret_cast<const unsigned char*>(std::to_address(cur));
    const unsigned char* end = reinterpret_cast<const unsigned char*>(std::to_address(last));

    const __m256i upper_lut =
        _mm256_broadcastsi128_si256(_mm_loadu_si128(reinterpret_cast<const __m128i*>(cc.upper_nibble_table.data())));
    const __m256i lower_lut =
        _mm256_broadcastsi128_si256(_mm_loadu_si128(reinterpret_cast<const __m128i*>(cc.lower_nibble_table.data())));

    while (p + 32 <= end) {
        __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
        __m256i up = _mm256_and_si256(_mm256_srli_epi16(v, 4), _mm256_set1_epi8(0x0F));
        __m256i low = _mm256_and_si256(v, _mm256_set1_epi8(0x0F));
        __m256i mU = _mm256_shuffle_epi8(upper_lut, up);
        __m256i mL = _mm256_shuffle_epi8(lower_lut, low);
        __m256i m = _mm256_and_si256(mU, mL);
        uint32_t M = _mm256_movemask_epi8(m); // 1 bit per byte: class match?

        uint32_t R = M;
        for (int i = 1; i < static_cast<int>(K); ++i) {
            R &= (M >> i);
        }
        if (R) {
            int i = __builtin_ctz(R);
            cur = uchar_to_iter<It>(p + i + K);
            return true;
        }
        p += 32 - (K - 1);
    }

    while (p + K <= end) {
        bool ok = true;
        for (size_t i = 0; i < K; ++i)
            ok &= (cc.exact_membership[p[i]] != 0);
        if (ok) {
            cur = uchar_to_iter<It>(p + K);
            return true;
        }
        ++p;
    }
    return false;
}

template <auto... String, typename Iterator, typename EndIterator>
    requires std::is_same_v<std::remove_cv_t<std::remove_reference_t<decltype(*std::declval<Iterator>())>>, char>
inline bool match_string_shift_or(Iterator& current, const EndIterator last, const flags& f) {
    (void)f; // Suppress unused parameter warning
    constexpr size_t string_length = sizeof...(String);
    static_assert(string_length <= MAX_SHIFT_OR_PATTERN_LENGTH, "Pattern too long for Shift-Or");

    constexpr char pattern[] = {static_cast<char>(String)...};

    static constexpr shift_or_state<string_length> state = []() {
        shift_or_state<string_length> s;
        s.template init_exact_pattern<String...>();
        return s;
    }();

    return match_shift_or<string_length>(current, last, state);
}

template <size_t NumPatterns, size_t MaxPatternLength>
struct multi_pattern_shift_or_state {
    static_assert(NumPatterns <= 4, "Too many patterns for multi-pattern Shift-Or");
    static_assert(MaxPatternLength <= MAX_SHIFT_OR_PATTERN_LENGTH, "Pattern too long for Shift-Or");

    std::array<shift_or_state<MaxPatternLength>, NumPatterns> pattern_states;
    std::array<size_t, NumPatterns> pattern_lengths;

    template <typename... PatternArrays>
    constexpr void init_multi_pattern(const PatternArrays&... patterns) {
        static_assert(sizeof...(patterns) == NumPatterns, "Number of patterns must match NumPatterns");

        size_t i = 0;
        ((pattern_lengths[i] = patterns.size(), init_pattern_state(pattern_states[i], patterns), ++i), ...);
    }

    template <size_t PatternLength>
    constexpr void init_pattern_state(shift_or_state<MaxPatternLength>& state,
                                      const std::array<char, PatternLength>& pattern) {
        using M = typename shift_or_state<MaxPatternLength>::M;

        for (auto& mask : state.char_masks) {
            mask = ~M(0);
        }

        for (size_t i = 0; i < PatternLength; ++i) {
            state.char_masks[static_cast<unsigned char>(pattern[i])] &= ~(M(1) << i);
        }
    }
};

template <size_t NumPatterns, size_t MaxPatternLength, typename It, typename EndIt>
    requires std::contiguous_iterator<It> && std::same_as<std::remove_cvref_t<decltype(*std::declval<It>())>, char>
inline bool match_multi_pattern_shift_or(It& current, const EndIt last,
                                         const multi_pattern_shift_or_state<NumPatterns, MaxPatternLength>& state) {
    if (current == last)
        return false;

    using M = typename shift_or_state<MaxPatternLength>::M;
    const unsigned char* p = reinterpret_cast<const unsigned char*>(std::to_address(current));
    const unsigned char* end = reinterpret_cast<const unsigned char*>(std::to_address(last));

    uint64_t D0 = ~0ull, D1 = ~0ull, D2 = ~0ull, D3 = ~0ull;
    const M* m0 = state.pattern_states[0].char_masks.data();
    const M* m1 = NumPatterns > 1 ? state.pattern_states[1].char_masks.data() : nullptr;
    const M* m2 = NumPatterns > 2 ? state.pattern_states[2].char_masks.data() : nullptr;
    const M* m3 = NumPatterns > 3 ? state.pattern_states[3].char_masks.data() : nullptr;

    const uint64_t MSB0 = 1ull << (state.pattern_lengths[0] - 1);
    const uint64_t MSB1 = NumPatterns > 1 ? (1ull << (state.pattern_lengths[1] - 1)) : 0;
    const uint64_t MSB2 = NumPatterns > 2 ? (1ull << (state.pattern_lengths[2] - 1)) : 0;
    const uint64_t MSB3 = NumPatterns > 3 ? (1ull << (state.pattern_lengths[3] - 1)) : 0;

    while (p < end) {
        const unsigned c = *p++;

        D0 = (D0 << 1) | (uint64_t)m0[c];
        if (NumPatterns > 1)
            D1 = (D1 << 1) | (uint64_t)m1[c];
        if (NumPatterns > 2)
            D2 = (D2 << 1) | (uint64_t)m2[c];
        if (NumPatterns > 3)
            D3 = (D3 << 1) | (uint64_t)m3[c];

        const bool hit0 = !(D0 & MSB0);
        const bool hit1 = NumPatterns > 1 && !(D1 & MSB1);
        const bool hit2 = NumPatterns > 2 && !(D2 & MSB2);
        const bool hit3 = NumPatterns > 3 && !(D3 & MSB3);

        if (hit0 || hit1 || hit2 || hit3) {
            current = uchar_to_iter<It>(p);
            return true;
        }
    }
    return false;
}

template <typename Iterator, typename EndIterator>
    requires std::is_same_v<std::remove_cv_t<std::remove_reference_t<decltype(*std::declval<Iterator>())>>, char>
inline bool match_keywords_shift_or(Iterator& current, const EndIterator last, const flags& f) {
    (void)f;

    static constexpr multi_pattern_shift_or_state<4, 4> state = []() {
        multi_pattern_shift_or_state<4, 4> s;
        s.init_multi_pattern(std::array<char, 4>{'C', 'T', 'R', 'E'}, std::array<char, 4>{'R', 'E', 'G', 'X'},
                             std::array<char, 4>{'S', 'C', 'A', 'N'}, std::array<char, 4>{'F', 'I', 'N', 'D'});
        return s;
    }();

    return match_multi_pattern_shift_or<4, 4>(current, last, state);
}

template <typename CharClass, size_t Count, typename Iterator, typename EndIterator>
inline bool match_char_class_shift_or(Iterator& current, const EndIterator last, const flags& f) {
    static_assert(Count <= MAX_SHIFT_OR_PATTERN_LENGTH, "Pattern too long for Shift-Or");

    static constexpr shift_or_state<Count> state = []() {
        shift_or_state<Count> s;
        s.template init_char_class_pattern<CharClass>();
        return s;
    }();

    return match_shift_or<Count>(current, last, state);
}

#else // !CTRE_ARCH_X86 - provide stub implementations

template <size_t N>
HOT_ALWAYS_INLINE bool verify_equal(const unsigned char* s, const char* pat) {
    return std::memcmp(s, pat, N) == 0;
}

template <size_t PatternLength, typename It, typename EndIt>
inline bool match_string_vector_prefilter(It&, const EndIt, const char*) {
    return false;
}

template <auto... String, typename Iterator, typename EndIterator>
inline bool match_string_shift_or(Iterator&, const EndIterator, const flags&) {
    return false;
}

template <typename Iterator, typename EndIterator>
inline bool match_keywords_shift_or(Iterator&, const EndIterator, const flags&) {
    return false;
}

template <typename CharClass, size_t Count, typename Iterator, typename EndIterator>
inline bool match_char_class_shift_or(Iterator& current, const EndIterator last, const flags& f) {
    return match_shift_or<Count>(current, last, shift_or_state<Count>{});
}

#endif // CTRE_ARCH_X86

} // namespace ctre::simd

#endif
