#ifndef CTRE__SIMD_SEQUENCE_FUSION__HPP
#define CTRE__SIMD_SEQUENCE_FUSION__HPP

#include "atoms.hpp"
#include "atoms_characters.hpp"
#include "simd_detection.hpp"
#include <array>
#include <cstdint>
#include <immintrin.h>

namespace ctre::simd {

#if !defined(CTRE_DISABLE_SIMD) && (defined(__SSE2__) || defined(__AVX2__))

struct CharRange {
    char lo = 0, hi = 0;
    constexpr CharRange() = default;
    constexpr CharRange(char l, char h) : lo(l), hi(h) {}
};

// Range extraction
template <typename T> struct to_char_range;
template <auto Lo, auto Hi> struct to_char_range<char_range<Lo, Hi>> {
    static constexpr CharRange value{static_cast<char>(Lo), static_cast<char>(Hi)};
};
template <auto C> struct to_char_range<character<C>> {
    static constexpr CharRange value{static_cast<char>(C), static_cast<char>(C)};
};

template <typename T> struct range_extractor {
    static constexpr size_t num_ranges = 0;
    static constexpr std::array<CharRange, 1> ranges = {CharRange()};
};

template <typename... Elements> requires(sizeof...(Elements) > 0 && sizeof...(Elements) <= 8)
struct range_extractor<set<Elements...>> {
    static constexpr size_t num_ranges = sizeof...(Elements);
    static constexpr auto build_ranges() {
        std::array<CharRange, num_ranges> result{};
        size_t idx = 0;
        ((result[idx++] = to_char_range<Elements>::value), ...);
        return result;
    }
    static constexpr auto ranges = build_ranges();
};

// Segment info
template <typename T> struct segment_info {
    static constexpr bool is_literal = false;
    static constexpr bool is_char_class = false;
    static constexpr char literal_char = '\0';
    static constexpr size_t min_len = 0, max_len = 0;
    static constexpr bool is_unbounded = false;
    static constexpr size_t num_ranges = 0;
    static constexpr std::array<CharRange, 1> ranges = {CharRange()};
};

template <auto C> struct segment_info<character<C>> {
    static constexpr bool is_literal = true;
    static constexpr bool is_char_class = false;
    static constexpr char literal_char = static_cast<char>(C);
    static constexpr size_t min_len = 1, max_len = 1;
    static constexpr bool is_unbounded = false;
    static constexpr size_t num_ranges = 0;
    static constexpr std::array<CharRange, 1> ranges = {CharRange()};
};

template <size_t A, size_t B, typename... Elements> requires(sizeof...(Elements) > 0 && sizeof...(Elements) <= 8)
struct segment_info<repeat<A, B, set<Elements...>>> {
    static constexpr bool is_literal = false;
    static constexpr bool is_char_class = true;
    static constexpr char literal_char = '\0';
    static constexpr size_t min_len = A;
    static constexpr size_t max_len = (B == 0) ? 16 : B;
    static constexpr bool is_unbounded = (B == 0);
    using Extractor = range_extractor<set<Elements...>>;
    static constexpr size_t num_ranges = Extractor::num_ranges;
    static constexpr auto ranges = Extractor::ranges;
};

[[gnu::always_inline]] inline bool check_positions_with_ranges(const char* data, uint32_t mask,
                                                                       const CharRange* ranges, size_t num_ranges) {
    if (num_ranges == 0 || num_ranges > 8 || mask == 0) return true;
    __m128i input = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data));
    __m128i result = _mm_setzero_si128();
    for (size_t i = 0; i < num_ranges; ++i) {
        __m128i lo = _mm_set1_epi8(ranges[i].lo);
        __m128i hi = _mm_set1_epi8(ranges[i].hi);
        __m128i ge = _mm_cmpgt_epi8(input, _mm_sub_epi8(lo, _mm_set1_epi8(1)));
        __m128i le = _mm_cmpgt_epi8(_mm_add_epi8(hi, _mm_set1_epi8(1)), input);
        result = _mm_or_si128(result, _mm_and_si128(ge, le));
    }
    return (_mm_movemask_epi8(result) & mask) == mask;
}

[[gnu::always_inline]] inline bool check_literals_simd(const char* data, uint32_t mask, const char* expected) {
    if (mask == 0) return true;
    __m128i input = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data));
    __m128i exp = _mm_loadu_si128(reinterpret_cast<const __m128i*>(expected));
    return (_mm_movemask_epi8(_mm_cmpeq_epi8(input, exp)) & mask) == mask;
}

// Variant structures
struct SegmentRangeInfo {
    CharRange ranges[8]{};
    size_t num_ranges = 0;
    uint32_t position_mask = 0;
};

struct Variant {
    size_t len = 0;
    uint32_t lit_mask = 0;
    char lit_vals[32]{};
    SegmentRangeInfo segment_ranges[8]{};
    size_t num_range_sets = 0;
};

template <typename... Elements>
constexpr Variant generate_variant_at_lengths(const std::array<size_t, sizeof...(Elements)>& lengths) {
    Variant v{};
    size_t pos = 0, idx = 0;
    (([&]() {
        using Info = segment_info<Elements>;
        size_t len = lengths[idx++];
        if constexpr (Info::is_char_class) {
            size_t range_set_idx = v.num_range_sets;
            bool found = false;
            for (size_t i = 0; i < v.num_range_sets; ++i) {
                if (v.segment_ranges[i].num_ranges == Info::num_ranges) {
                    bool same = true;
                    for (size_t r = 0; r < Info::num_ranges; ++r) {
                        if (v.segment_ranges[i].ranges[r].lo != Info::ranges[r].lo ||
                            v.segment_ranges[i].ranges[r].hi != Info::ranges[r].hi) { same = false; break; }
                    }
                    if (same) { range_set_idx = i; found = true; break; }
                }
            }
            if (!found && v.num_range_sets < 8) {
                range_set_idx = v.num_range_sets;
                v.segment_ranges[range_set_idx].num_ranges = Info::num_ranges;
                for (size_t r = 0; r < Info::num_ranges && r < 8; ++r)
                    v.segment_ranges[range_set_idx].ranges[r] = Info::ranges[r];
                v.num_range_sets++;
            }
            for (size_t i = 0; i < len && (pos + i) < 32; ++i)
                v.segment_ranges[range_set_idx].position_mask |= (1u << (pos + i));
        } else if constexpr (Info::is_literal) {
            v.lit_mask |= (1u << pos);
            v.lit_vals[pos] = Info::literal_char;
        }
        pos += len;
    }()), ...);
    v.len = pos;
    return v;
}

template <typename... Elements>
struct variant_generator {
    static constexpr size_t num_elements = sizeof...(Elements);
    static constexpr std::array<size_t, num_elements> min_lengths = {segment_info<Elements>::min_len...};
    static constexpr std::array<size_t, num_elements> max_lengths = {segment_info<Elements>::max_len...};

    static constexpr auto generate_variants() {
        constexpr size_t total_min = (... + segment_info<Elements>::min_len);
        constexpr size_t total_max = (... + segment_info<Elements>::max_len);
        constexpr size_t range = total_max - total_min;

        if constexpr (total_max > 16 || range > 16) return std::array<Variant, 1>{};
        else if constexpr (range <= 8) {
            std::array<Variant, 4> variants{};
            variants[0] = generate_variant_at_lengths<Elements...>(max_lengths);
            variants[1] = generate_variant_at_lengths<Elements...>(min_lengths);
            std::array<size_t, num_elements> mid1 = min_lengths;
            for (size_t i = 0; i < num_elements; ++i)
                if (max_lengths[i] > min_lengths[i]) mid1[i] = (min_lengths[i] + max_lengths[i]) / 2;
            variants[2] = generate_variant_at_lengths<Elements...>(mid1);
            std::array<size_t, num_elements> mid2 = min_lengths;
            for (size_t i = 0; i < num_elements / 2; ++i) mid2[i] = max_lengths[i];
            variants[3] = generate_variant_at_lengths<Elements...>(mid2);
            return variants;
        } else {
            std::array<Variant, 2> variants{};
            variants[0] = generate_variant_at_lengths<Elements...>(max_lengths);
            variants[1] = generate_variant_at_lengths<Elements...>(min_lengths);
            return variants;
        }
    }
    static constexpr auto variants = generate_variants();
};

template <typename... Elements, typename Iterator, typename EndIterator>
inline Iterator match_sequence_generic(Iterator begin, EndIterator end) {
    using VarGen = variant_generator<Elements...>;
    constexpr size_t max_total = (... + segment_info<Elements>::max_len);
    constexpr size_t min_total = (... + segment_info<Elements>::min_len);
    constexpr bool has_unbounded = (... || segment_info<Elements>::is_unbounded);

    if constexpr (max_total > 16 || (max_total - min_total) > 16 || VarGen::variants.size() == 0 || has_unbounded)
        return begin;

    size_t remaining;
    if constexpr (std::is_pointer_v<Iterator> && std::is_pointer_v<EndIterator>) remaining = end - begin;
    else { remaining = 0; auto it = begin; while (it != end && remaining < 64) { ++it; ++remaining; } }

    if (remaining < SIMD_SEQUENCE_THRESHOLD) return begin;

    for (const auto& v : VarGen::variants) {
        if (remaining >= v.len && v.len > 0) {
            if (v.lit_mask != 0 && __builtin_popcount(v.lit_mask) < 4)
                if (!check_literals_simd(begin, v.lit_mask, v.lit_vals)) continue;
            bool match = true;
            for (size_t i = 0; i < v.num_range_sets && match; ++i) {
                const auto& rs = v.segment_ranges[i];
                if (!check_positions_with_ranges(begin, rs.position_mask, rs.ranges, rs.num_ranges)) match = false;
            }
            if (match && v.lit_mask != 0 && __builtin_popcount(v.lit_mask) >= 4)
                match = check_literals_simd(begin, v.lit_mask, v.lit_vals);
            if (match) return begin + v.len;
        }
    }
    return begin;
}

template <typename... Elements, typename Iterator, typename EndIterator>
inline Iterator match_sequence_fused(sequence<Elements...>*, Iterator begin, EndIterator end) {
    return match_sequence_generic<Elements...>(begin, end);
}

#else

template <typename SequenceType, typename Iterator, typename EndIterator>
inline Iterator match_sequence_fused(SequenceType*, Iterator begin, EndIterator) { return begin; }

#endif

} // namespace ctre::simd

#endif
