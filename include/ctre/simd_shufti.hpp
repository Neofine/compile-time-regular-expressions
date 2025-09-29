#ifndef CTRE__SIMD_SHUFTI__HPP
#define CTRE__SIMD_SHUFTI__HPP

#include "atoms_characters.hpp"
#include "flags_and_modes.hpp"
#include "simd_detection.hpp"
#include <array>
#include <cstring>
#include <immintrin.h>
#include <iterator>

namespace ctre {
namespace simd {

// SHUFTI: SIMD character-class matching using nibble lookup tables
// This is the real SHUFTI algorithm for character classes like [A-Za-z0-9_], \s, etc.

// SHUFTI pattern traits for integration with existing SIMD system
template <typename PatternType>
struct shufti_pattern_trait {
    static constexpr bool is_shufti_optimizable = false;
    static constexpr bool should_use_shufti = false;
};

// Specialize for character sets - only use SHUFTI for complex patterns
template <typename... Content>
struct shufti_pattern_trait<set<Content...>> {
    static constexpr bool is_shufti_optimizable = true;
    // Only use SHUFTI for complex character sets, not simple ranges
    static constexpr bool should_use_shufti = false; // Let existing SIMD handle simple cases
};

// Specialize for individual character sets
template <auto... Cs>
struct shufti_pattern_trait<set<character<Cs>...>> {
    static constexpr bool is_shufti_optimizable = true;
    static constexpr size_t num_chars = sizeof...(Cs);
    // Use SHUFTI for character classes with many individual characters
    static constexpr bool should_use_shufti = num_chars > 10;
};

// Character class definitions with proper SHUFTI encoding
struct character_class {
    std::array<uint8_t, 16> upper_nibble_table;  // Lookup table for upper nibbles (c>>4)
    std::array<uint8_t, 16> lower_nibble_table;  // Lookup table for lower nibbles (c&0xF)
    std::array<uint8_t, 16> upper_nibble_table2; // Second LUT for double-SHUFTI
    std::array<uint8_t, 16> lower_nibble_table2; // Second LUT for double-SHUFTI
    std::array<uint8_t, 256> exact_membership;   // Exact membership table for verification
    static constexpr uint8_t match_bit = 0x80;   // MSB for movemask compatibility
    static constexpr uint8_t match_bit2 = 0x40;  // Second bit for double-SHUFTI

    // Heuristic data for optimization decisions
    uint8_t density_estimate; // Estimated percentage of bytes 0x00-0x7F that pass prefilter
    bool use_double_shufti;   // Whether double-SHUFTI is beneficial
    bool use_exact_range;     // Whether to use exact range comparisons instead of SHUFTI

    constexpr character_class()
        : upper_nibble_table{}, lower_nibble_table{}, upper_nibble_table2{}, lower_nibble_table2{}, exact_membership{},
          density_estimate(0), use_double_shufti(false), use_exact_range(false) {}

    // Helper function to check if a byte is alphanumeric
    static constexpr bool is_alnum_byte(int b) {
        return (b >= '0' && b <= '9') || (b >= 'A' && b <= 'Z') || (b == '_') || (b >= 'a' && b <= 'z');
    }

    // Calculate density estimate and decide on optimization strategy
    constexpr void calculate_heuristics() {
        // Count how many bytes in 0x00-0x7F would pass the first SHUFTI prefilter
        int prefilter_hits = 0;
        for (int b = 0; b < 128; ++b) {
            uint8_t upper_nibble = (b >> 4) & 0xF;
            uint8_t lower_nibble = b & 0xF;
            if ((upper_nibble_table[upper_nibble] & match_bit) && (lower_nibble_table[lower_nibble] & match_bit)) {
                prefilter_hits++;
            }
        }

        density_estimate = static_cast<uint8_t>((prefilter_hits * 100) / 128);

        // If >40% of bytes pass prefilter, use exact range instead of SHUFTI
        use_exact_range = (density_estimate > 40);

        // For double-SHUFTI, count candidates after second pass
        if (!use_exact_range) {
            int double_hits = 0;
            for (int b = 0; b < 128; ++b) {
                uint8_t upper_nibble = (b >> 4) & 0xF;
                uint8_t lower_nibble = b & 0xF;
                uint8_t lower_nibble2 = static_cast<uint8_t>((lower_nibble + 7) % 16);

                if ((upper_nibble_table[upper_nibble] & match_bit) && (lower_nibble_table[lower_nibble] & match_bit) &&
                    (upper_nibble_table2[upper_nibble] & match_bit2) &&
                    (lower_nibble_table2[lower_nibble2] & match_bit2)) {
                    double_hits++;
                }
            }

            // Only use double-SHUFTI if it reduces candidates by >80% (very conservative)
            // Double-SHUFTI usually costs more than it saves due to extra shuffles
            int reduction = prefilter_hits - double_hits;
            use_double_shufti = (reduction * 100 / prefilter_hits) > 80;
        }
    }

    // Initialize for alphanumeric characters [A-Za-z0-9_] with proper SHUFTI bit encoding
    constexpr void init_alnum() {
        // Clear tables first
        for (int i = 0; i < 16; ++i) {
            upper_nibble_table[i] = 0x00;
            lower_nibble_table[i] = 0x00;
            upper_nibble_table2[i] = 0x00;
            lower_nibble_table2[i] = 0x00;
        }
        for (int i = 0; i < 256; ++i) {
            exact_membership[i] = 0x00;
        }

        // Build tables by iterating through all possible bytes
        for (int b = 0; b < 256; ++b) {
            if (is_alnum_byte(b)) {
                uint8_t upper_nibble = (b >> 4) & 0xF;
                uint8_t lower_nibble = b & 0xF;

                // First LUT pair with match_bit (0x80)
                upper_nibble_table[upper_nibble] |= match_bit;
                lower_nibble_table[lower_nibble] |= match_bit;

                // Second LUT pair with different mapping and match_bit2 (0x40)
                // Use permutation: lower_nibble2 = (lower_nibble + 7) % 16
                uint8_t lower_nibble2 = static_cast<uint8_t>((lower_nibble + 7) % 16);
                upper_nibble_table2[upper_nibble] |= match_bit2;
                lower_nibble_table2[lower_nibble2] |= match_bit2;

                exact_membership[b] = 0xFF; // Mark as exact member
            }
        }

        // Calculate optimization heuristics
        calculate_heuristics();
    }

    // Helper function to check if a byte is whitespace
    static constexpr bool is_whitespace_byte(int b) {
        return b == '\t' || b == '\n' || b == '\v' || b == '\f' || b == '\r' || b == ' ';
    }

    // Initialize for whitespace characters \s
    constexpr void init_whitespace() {
        // Clear tables first
        for (int i = 0; i < 16; ++i) {
            upper_nibble_table[i] = 0x00;
            lower_nibble_table[i] = 0x00;
            upper_nibble_table2[i] = 0x00;
            lower_nibble_table2[i] = 0x00;
        }
        for (int i = 0; i < 256; ++i) {
            exact_membership[i] = 0x00;
        }

        // Build tables by iterating through all possible bytes
        for (int b = 0; b < 256; ++b) {
            if (is_whitespace_byte(b)) {
                uint8_t upper_nibble = (b >> 4) & 0xF;
                uint8_t lower_nibble = b & 0xF;

                // First LUT pair with match_bit (0x80)
                upper_nibble_table[upper_nibble] |= match_bit;
                lower_nibble_table[lower_nibble] |= match_bit;

                // Second LUT pair with different mapping and match_bit2 (0x40)
                uint8_t lower_nibble2 = static_cast<uint8_t>((lower_nibble + 7) % 16);
                upper_nibble_table2[upper_nibble] |= match_bit2;
                lower_nibble_table2[lower_nibble2] |= match_bit2;

                exact_membership[b] = 0xFF; // Mark as exact member
            }
        }

        // Calculate optimization heuristics
        calculate_heuristics();
    }

    // Helper function to check if a byte is a digit
    static constexpr bool is_digit_byte(int b) {
        return b >= '0' && b <= '9';
    }

    // Initialize for digits [0-9]
    constexpr void init_digits() {
        // Clear tables first
        for (int i = 0; i < 16; ++i) {
            upper_nibble_table[i] = 0x00;
            lower_nibble_table[i] = 0x00;
            upper_nibble_table2[i] = 0x00;
            lower_nibble_table2[i] = 0x00;
        }
        for (int i = 0; i < 256; ++i) {
            exact_membership[i] = 0x00;
        }

        // Build tables by iterating through all possible bytes
        for (int b = 0; b < 256; ++b) {
            if (is_digit_byte(b)) {
                uint8_t upper_nibble = (b >> 4) & 0xF;
                uint8_t lower_nibble = b & 0xF;

                // First LUT pair with match_bit (0x80)
                upper_nibble_table[upper_nibble] |= match_bit;
                lower_nibble_table[lower_nibble] |= match_bit;

                // Second LUT pair with different mapping and match_bit2 (0x40)
                uint8_t lower_nibble2 = static_cast<uint8_t>((lower_nibble + 7) % 16);
                upper_nibble_table2[upper_nibble] |= match_bit2;
                lower_nibble_table2[lower_nibble2] |= match_bit2;

                exact_membership[b] = 0xFF; // Mark as exact member
            }
        }

        // Calculate optimization heuristics
        calculate_heuristics();
    }

    // Helper function to check if a byte is a letter
    static constexpr bool is_letter_byte(int b) {
        return (b >= 'A' && b <= 'Z') || (b >= 'a' && b <= 'z');
    }

    // Initialize for letters [A-Za-z]
    constexpr void init_letters() {
        // Clear tables first
        for (int i = 0; i < 16; ++i) {
            upper_nibble_table[i] = 0x00;
            lower_nibble_table[i] = 0x00;
            upper_nibble_table2[i] = 0x00;
            lower_nibble_table2[i] = 0x00;
        }
        for (int i = 0; i < 256; ++i) {
            exact_membership[i] = 0x00;
        }

        // Build tables by iterating through all possible bytes
        for (int b = 0; b < 256; ++b) {
            if (is_letter_byte(b)) {
                uint8_t upper_nibble = (b >> 4) & 0xF;
                uint8_t lower_nibble = b & 0xF;

                // First LUT pair with match_bit (0x80)
                upper_nibble_table[upper_nibble] |= match_bit;
                lower_nibble_table[lower_nibble] |= match_bit;

                // Second LUT pair with different mapping and match_bit2 (0x40)
                uint8_t lower_nibble2 = static_cast<uint8_t>((lower_nibble + 7) % 16);
                upper_nibble_table2[upper_nibble] |= match_bit2;
                lower_nibble_table2[lower_nibble2] |= match_bit2;

                exact_membership[b] = 0xFF; // Mark as exact member
            }
        }

        // Calculate optimization heuristics
        calculate_heuristics();
    }
};

// Exact range comparison functions for hot character classes (no SHUFTI needed)
namespace exact_range {

// AVX2 exact alnum (ASCII + '_'), branch-free
inline bool find_alnum_avx2(const unsigned char* p, const unsigned char* end, const unsigned char*& out) {
    if (p >= end)
        return false;

    size_t remaining = end - p;

    while (remaining >= 32) {
        __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
        __m256i x = _mm256_xor_si256(v, _mm256_set1_epi8(char(0x80))); // to signed domain

        auto in_range = [](const __m256i& x, unsigned lo, unsigned hi) {
            __m256i L = _mm256_set1_epi8(char((int)lo ^ 0x80));
            __m256i H = _mm256_set1_epi8(char((int)hi ^ 0x80));
            __m256i ge = _mm256_cmpgt_epi8(_mm256_add_epi8(x, _mm256_set1_epi8(1)), L); // x >= L
            __m256i le = _mm256_cmpgt_epi8(_mm256_add_epi8(H, _mm256_set1_epi8(1)), x); // x <= H
            return _mm256_and_si256(ge, le);
        };

        __m256i is_digit = in_range(x, '0', '9');
        __m256i is_upper = in_range(x, 'A', 'Z');
        __m256i is_lower = in_range(x, 'a', 'z');
        __m256i is_us = _mm256_cmpeq_epi8(v, _mm256_set1_epi8('_'));

        __m256i ok = _mm256_or_si256(_mm256_or_si256(is_digit, is_upper), _mm256_or_si256(is_lower, is_us));
        int mask = _mm256_movemask_epi8(ok);

        if (mask != 0) {
            int i = __builtin_ctz(mask);
            out = p + i + 1;
            return true;
        }

        p += 32;
        remaining -= 32;
    }

    // Handle remaining bytes
    while (p < end) {
        unsigned char c = *p;
        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c == '_')) {
            out = p + 1;
            return true;
        }
        ++p;
    }

    return false;
}

// AVX2 exact digits [0-9], branch-free
inline bool find_digits_avx2(const unsigned char* p, const unsigned char* end, const unsigned char*& out) {
    if (p >= end)
        return false;

    size_t remaining = end - p;

    while (remaining >= 32) {
        __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
        __m256i x = _mm256_xor_si256(v, _mm256_set1_epi8(char(0x80))); // to signed domain

        __m256i L = _mm256_set1_epi8(char('0' ^ 0x80));
        __m256i H = _mm256_set1_epi8(char('9' ^ 0x80));
        __m256i ge = _mm256_cmpgt_epi8(_mm256_add_epi8(x, _mm256_set1_epi8(1)), L); // x >= '0'
        __m256i le = _mm256_cmpgt_epi8(_mm256_add_epi8(H, _mm256_set1_epi8(1)), x); // x <= '9'
        __m256i ok = _mm256_and_si256(ge, le);

        int mask = _mm256_movemask_epi8(ok);
        if (mask != 0) {
            int i = __builtin_ctz(mask);
            out = p + i + 1;
            return true;
        }

        p += 32;
        remaining -= 32;
    }

    // Handle remaining bytes
    while (p < end) {
        if (*p >= '0' && *p <= '9') {
            out = p + 1;
            return true;
        }
        ++p;
    }

    return false;
}

// AVX2 exact letters [A-Za-z], branch-free
inline bool find_letters_avx2(const unsigned char* p, const unsigned char* end, const unsigned char*& out) {
    if (p >= end)
        return false;

    size_t remaining = end - p;

    while (remaining >= 32) {
        __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));
        __m256i x = _mm256_xor_si256(v, _mm256_set1_epi8(char(0x80))); // to signed domain

        auto in_range = [](const __m256i& x, unsigned lo, unsigned hi) {
            __m256i L = _mm256_set1_epi8(char((int)lo ^ 0x80));
            __m256i H = _mm256_set1_epi8(char((int)hi ^ 0x80));
            __m256i ge = _mm256_cmpgt_epi8(_mm256_add_epi8(x, _mm256_set1_epi8(1)), L); // x >= L
            __m256i le = _mm256_cmpgt_epi8(_mm256_add_epi8(H, _mm256_set1_epi8(1)), x); // x <= H
            return _mm256_and_si256(ge, le);
        };

        __m256i is_upper = in_range(x, 'A', 'Z');
        __m256i is_lower = in_range(x, 'a', 'z');
        __m256i ok = _mm256_or_si256(is_upper, is_lower);

        int mask = _mm256_movemask_epi8(ok);
        if (mask != 0) {
            int i = __builtin_ctz(mask);
            out = p + i + 1;
            return true;
        }

        p += 32;
        remaining -= 32;
    }

    // Handle remaining bytes
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

// AVX2 exact whitespace, branch-free
inline bool find_whitespace_avx2(const unsigned char* p, const unsigned char* end, const unsigned char*& out) {
    if (p >= end)
        return false;

    size_t remaining = end - p;

    while (remaining >= 32) {
        __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));

        __m256i is_tab = _mm256_cmpeq_epi8(v, _mm256_set1_epi8('\t'));
        __m256i is_nl = _mm256_cmpeq_epi8(v, _mm256_set1_epi8('\n'));
        __m256i is_vt = _mm256_cmpeq_epi8(v, _mm256_set1_epi8('\v'));
        __m256i is_ff = _mm256_cmpeq_epi8(v, _mm256_set1_epi8('\f'));
        __m256i is_cr = _mm256_cmpeq_epi8(v, _mm256_set1_epi8('\r'));
        __m256i is_sp = _mm256_cmpeq_epi8(v, _mm256_set1_epi8(' '));

        __m256i ok = _mm256_or_si256(_mm256_or_si256(is_tab, is_nl),
                                     _mm256_or_si256(_mm256_or_si256(is_vt, is_ff), _mm256_or_si256(is_cr, is_sp)));

        int mask = _mm256_movemask_epi8(ok);
        if (mask != 0) {
            int i = __builtin_ctz(mask);
            out = p + i + 1;
            return true;
        }

        p += 32;
        remaining -= 32;
    }

    // Handle remaining bytes
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

// Forward declarations
inline bool shufti_find_avx2_single(const unsigned char* p, const unsigned char* end, const character_class& char_class,
                                    const unsigned char*& out);
inline bool shufti_find_avx2_double(const unsigned char* p, const unsigned char* end, const character_class& char_class,
                                    const unsigned char*& out);

// Pointer-based fast-path for AVX2 (no iterator overhead)
inline bool shufti_find_avx2(const unsigned char* p, const unsigned char* end, const character_class& char_class,
                             const unsigned char*& out) {
    if (p >= end)
        return false;

    // Use exact range comparisons for hot classes if heuristics suggest it
    if (char_class.use_exact_range) {
        // For dense classes, use exact range comparisons instead of SHUFTI
        // This is much faster for common classes like [A-Za-z0-9_], [0-9], \s, etc.
        return exact_range::find_alnum_avx2(p, end, out);
    }

    // Use single or double SHUFTI based on heuristics
    if (char_class.use_double_shufti) {
        return shufti_find_avx2_double(p, end, char_class, out);
    } else {
        return shufti_find_avx2_single(p, end, char_class, out);
    }
}

// Single-pass SHUFTI (original approach)
inline bool shufti_find_avx2_single(const unsigned char* p, const unsigned char* end, const character_class& char_class,
                                    const unsigned char*& out) {
    if (p >= end)
        return false;

    size_t remaining = end - p;

    // Create SIMD lookup tables - broadcast 16-byte tables to 32 bytes (hoisted outside loop)
    const __m256i upper_lut = _mm256_broadcastsi128_si256(
        _mm_loadu_si128(reinterpret_cast<const __m128i*>(char_class.upper_nibble_table.data())));
    const __m256i lower_lut = _mm256_broadcastsi128_si256(
        _mm_loadu_si128(reinterpret_cast<const __m128i*>(char_class.lower_nibble_table.data())));

    // Process 32-byte chunks using AVX2
    while (remaining >= 32) {
        // Load 32 bytes of input
        __m256i input = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));

        // Extract upper nibbles (c >> 4) for all 32 bytes
        __m256i upper_nibbles = _mm256_srli_epi16(input, 4);
        upper_nibbles = _mm256_and_si256(upper_nibbles, _mm256_set1_epi8(0x0F));

        // Extract lower nibbles (c & 0x0F) for all 32 bytes
        __m256i lower_nibbles = _mm256_and_si256(input, _mm256_set1_epi8(0x0F));

        // Single SHUFTI pass with match_bit (0x80)
        __m256i upper_matches = _mm256_shuffle_epi8(upper_lut, upper_nibbles);
        __m256i lower_matches = _mm256_shuffle_epi8(lower_lut, lower_nibbles);
        __m256i combined = _mm256_and_si256(upper_matches, lower_matches);

        // Check if any byte has the match bit set (prefilter)
        int mask = _mm256_movemask_epi8(combined);
        if (mask != 0) {
            // Verify candidates with exact membership table
            while (mask != 0) {
                int i = __builtin_ctz(mask);
                unsigned char c = p[i];
                if (char_class.exact_membership[c]) {
                    out = p + i + 1; // Advance by 1 past the match
                    return true;
                }
                mask &= mask - 1; // Clear lowest set bit, check next candidate
            }
        }

        p += 32;
        remaining -= 32;
    }

    // Try one 16-byte SSSE3 block before falling to scalar
    if (remaining >= 16 && get_simd_capability() >= SIMD_CAPABILITY_SSSE3) {
        __m128i input = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));

        // Extract upper nibbles (c >> 4) for all 16 bytes
        __m128i upper_nibbles = _mm_srli_epi16(input, 4);
        upper_nibbles = _mm_and_si128(upper_nibbles, _mm_set1_epi8(0x0F));

        // Extract lower nibbles (c & 0x0F) for all 16 bytes
        __m128i lower_nibbles = _mm_and_si128(input, _mm_set1_epi8(0x0F));

        // Create 128-bit LUTs for SSE tail (SINGLE PASS ONLY)
        __m128i upper_lut_128 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(char_class.upper_nibble_table.data()));
        __m128i lower_lut_128 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(char_class.lower_nibble_table.data()));

        // Single SHUFTI pass with match_bit (0x80)
        __m128i upper_matches = _mm_shuffle_epi8(upper_lut_128, upper_nibbles);
        __m128i lower_matches = _mm_shuffle_epi8(lower_lut_128, lower_nibbles);
        __m128i combined = _mm_and_si128(upper_matches, lower_matches);

        // Check if any byte has the match bit set (single prefilter)
        int mask = _mm_movemask_epi8(combined);
        if (mask != 0) {
            // Verify candidates with exact membership table
            while (mask != 0) {
                int i = __builtin_ctz(mask);
                unsigned char c = p[i];
                if (char_class.exact_membership[c]) {
                    out = p + i + 1; // Advance by 1 past the match
                    return true;
                }
                mask &= mask - 1; // Clear lowest set bit, check next candidate
            }
        }

        p += 16;
        remaining -= 16;
    }

    // Process remaining bytes
    while (p < end) {
        if (char_class.exact_membership[*p]) {
            out = p + 1; // Advance by 1 past the match
            return true;
        }
        ++p;
    }

    return false;
}

// Double-pass SHUFTI (when heuristics suggest it's beneficial)
inline bool shufti_find_avx2_double(const unsigned char* p, const unsigned char* end, const character_class& char_class,
                                    const unsigned char*& out) {
    if (p >= end)
        return false;

    size_t remaining = end - p;

    // Create SIMD lookup tables - broadcast 16-byte tables to 32 bytes (hoisted outside loop)
    const __m256i upper_lut = _mm256_broadcastsi128_si256(
        _mm_loadu_si128(reinterpret_cast<const __m128i*>(char_class.upper_nibble_table.data())));
    const __m256i lower_lut = _mm256_broadcastsi128_si256(
        _mm_loadu_si128(reinterpret_cast<const __m128i*>(char_class.lower_nibble_table.data())));
    const __m256i upper_lut2 = _mm256_broadcastsi128_si256(
        _mm_loadu_si128(reinterpret_cast<const __m128i*>(char_class.upper_nibble_table2.data())));
    const __m256i lower_lut2 = _mm256_broadcastsi128_si256(
        _mm_loadu_si128(reinterpret_cast<const __m128i*>(char_class.lower_nibble_table2.data())));

    // Process 32-byte chunks using AVX2
    while (remaining >= 32) {
        // Load 32 bytes of input
        __m256i input = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(p));

        // Extract upper nibbles (c >> 4) for all 32 bytes
        __m256i upper_nibbles = _mm256_srli_epi16(input, 4);
        upper_nibbles = _mm256_and_si256(upper_nibbles, _mm256_set1_epi8(0x0F));

        // Extract lower nibbles (c & 0x0F) for all 32 bytes
        __m256i lower_nibbles = _mm256_and_si256(input, _mm256_set1_epi8(0x0F));

        // First SHUFTI pass with match_bit (0x80)
        __m256i upper_matches1 = _mm256_shuffle_epi8(upper_lut, upper_nibbles);
        __m256i lower_matches1 = _mm256_shuffle_epi8(lower_lut, lower_nibbles);
        __m256i combined1 = _mm256_and_si256(upper_matches1, lower_matches1);

        // Second SHUFTI pass with match_bit2 (0x40) and different mapping
        __m256i upper_matches2 = _mm256_shuffle_epi8(upper_lut2, upper_nibbles);
        __m256i lower_matches2 = _mm256_shuffle_epi8(lower_lut2, lower_nibbles);
        __m256i combined2 = _mm256_and_si256(upper_matches2, lower_matches2);

        // AND both results - only bytes that pass both filters are candidates
        __m256i candidates = _mm256_and_si256(combined1, combined2);

        // Check if any byte has both match bits set (double prefilter)
        int mask = _mm256_movemask_epi8(candidates);
        if (mask != 0) {
            // Verify candidates with exact membership table
            while (mask != 0) {
                int i = __builtin_ctz(mask);
                unsigned char c = p[i];
                if (char_class.exact_membership[c]) {
                    out = p + i + 1; // Advance by 1 past the match
                    return true;
                }
                mask &= mask - 1; // Clear lowest set bit, check next candidate
            }
        }

        p += 32;
        remaining -= 32;
    }

    // Process remaining bytes
    while (p < end) {
        if (char_class.exact_membership[*p]) {
            out = p + 1; // Advance by 1 past the match
            return true;
        }
        ++p;
    }

    return false;
}

// Pointer-based fast-path for SSSE3 (no iterator overhead)
inline bool shufti_find_ssse3(const unsigned char* p, const unsigned char* end, const character_class& char_class,
                              const unsigned char*& out) {
    if (p >= end)
        return false;

    size_t remaining = end - p;

    // Create SIMD lookup tables (hoisted outside loop)
    const __m128i upper_lut = _mm_loadu_si128(reinterpret_cast<const __m128i*>(char_class.upper_nibble_table.data()));
    const __m128i lower_lut = _mm_loadu_si128(reinterpret_cast<const __m128i*>(char_class.lower_nibble_table.data()));
    const __m128i upper_lut2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(char_class.upper_nibble_table2.data()));
    const __m128i lower_lut2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(char_class.lower_nibble_table2.data()));

    // Process 16-byte chunks using SSSE3
    while (remaining >= 16) {
        // Load 16 bytes of input
        __m128i input = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));

        // Extract upper nibbles (c >> 4) for all 16 bytes
        __m128i upper_nibbles = _mm_srli_epi16(input, 4);
        upper_nibbles = _mm_and_si128(upper_nibbles, _mm_set1_epi8(0x0F));

        // Extract lower nibbles (c & 0x0F) for all 16 bytes
        __m128i lower_nibbles = _mm_and_si128(input, _mm_set1_epi8(0x0F));

        // First SHUFTI pass with match_bit (0x80)
        __m128i upper_matches1 = _mm_shuffle_epi8(upper_lut, upper_nibbles);
        __m128i lower_matches1 = _mm_shuffle_epi8(lower_lut, lower_nibbles);
        __m128i combined1 = _mm_and_si128(upper_matches1, lower_matches1);

        // Second SHUFTI pass with match_bit2 (0x40) and different mapping
        __m128i upper_matches2 = _mm_shuffle_epi8(upper_lut2, upper_nibbles);
        __m128i lower_matches2 = _mm_shuffle_epi8(lower_lut2, lower_nibbles);
        __m128i combined2 = _mm_and_si128(upper_matches2, lower_matches2);

        // AND both results - only bytes that pass both filters are candidates
        __m128i candidates = _mm_and_si128(combined1, combined2);

        // Check if any byte has both match bits set (double prefilter)
        int mask = _mm_movemask_epi8(candidates);
        if (mask != 0) {
            // Verify candidates with exact membership table
            while (mask != 0) {
                int i = __builtin_ctz(mask);
                unsigned char c = p[i];
                if (char_class.exact_membership[c]) {
                    out = p + i + 1; // Advance by 1 past the match
                    return true;
                }
                mask &= mask - 1; // Clear lowest set bit, check next candidate
            }
        }

        p += 16;
        remaining -= 16;
    }

    // Process remaining bytes
    while (p < end) {
        if (char_class.exact_membership[*p]) {
            out = p + 1; // Advance by 1 past the match
            return true;
        }
        ++p;
    }

    return false;
}

// Optimized SHUFTI implementation using AVX2
template <typename Iterator, typename EndIterator>
    requires std::contiguous_iterator<Iterator> &&
             std::is_same_v<std::remove_cv_t<std::remove_reference_t<decltype(*std::declval<Iterator>())>>, char>
inline bool match_char_class_shufti_avx2(Iterator& current, const EndIterator last, const character_class& char_class) {
    if (current == last)
        return false;

    // Use pointer fast-path for better performance (avoid UB with past-the-end iterator)
    const unsigned char* p = std::to_address(current);
    const unsigned char* end = std::to_address(last);
    const unsigned char* out;

    if (shufti_find_avx2(p, end, char_class, out)) {
        current = Iterator(out);
        return true;
    }

    return false;
}

// Proper SHUFTI implementation using SSSE3 with actual SIMD shuffles
template <typename Iterator, typename EndIterator>
    requires std::contiguous_iterator<Iterator> &&
             std::is_same_v<std::remove_cv_t<std::remove_reference_t<decltype(*std::declval<Iterator>())>>, char>
inline bool match_char_class_shufti_ssse3(Iterator& current, const EndIterator last,
                                          const character_class& char_class) {
    if (current == last)
        return false;

    // Use pointer fast-path for better performance (avoid UB with past-the-end iterator)
    const unsigned char* p = std::to_address(current);
    const unsigned char* end = std::to_address(last);
    const unsigned char* out;

    if (shufti_find_ssse3(p, end, char_class, out)) {
        current = Iterator(out);
        return true;
    }

    return false;
}

// Scalar SHUFTI implementation
template <typename Iterator, typename EndIterator>
    requires std::is_same_v<std::remove_cv_t<std::remove_reference_t<decltype(*std::declval<Iterator>())>>, char>
inline bool match_char_class_shufti_scalar(Iterator& current, const EndIterator last,
                                           const character_class& char_class) {
    if (current == last)
        return false;

    Iterator pos = current;

    while (pos != last) {
        uint8_t byte_val = static_cast<uint8_t>(*pos);

        if (char_class.exact_membership[byte_val]) {
            current = ++pos;
            return true;
        }
        ++pos;
    }

    return false;
}

// Main SHUFTI matching function with automatic SIMD selection
template <typename Iterator, typename EndIterator>
    requires std::is_same_v<std::remove_cv_t<std::remove_reference_t<decltype(*std::declval<Iterator>())>>, char>
inline bool match_char_class_shufti(Iterator& current, const EndIterator last, const character_class& char_class) {
    if constexpr (CTRE_SIMD_ENABLED) {
        if (get_simd_capability() >= SIMD_CAPABILITY_AVX2) {
            return match_char_class_shufti_avx2(current, last, char_class);
        } else if (get_simd_capability() >= SIMD_CAPABILITY_SSSE3) {
            return match_char_class_shufti_ssse3(current, last, char_class);
        }
    }
    return match_char_class_shufti_scalar(current, last, char_class);
}

// ============================================================================
// CTRE INTEGRATION FUNCTIONS
// ============================================================================

// SHUFTI integration function for use in match_pattern_repeat_simd
template <typename PatternType, size_t MinCount, size_t MaxCount, typename Iterator, typename EndIterator>
inline Iterator match_pattern_repeat_shufti(Iterator current, const EndIterator last, const flags& f) {
    if constexpr (!shufti_pattern_trait<PatternType>::is_shufti_optimizable) {
        // Not a SHUFTI-optimizable pattern, fall back to existing SIMD
        return current; // Signal to use existing SIMD
    }

    if constexpr (!shufti_pattern_trait<PatternType>::should_use_shufti) {
        // Pattern doesn't benefit from SHUFTI, fall back to existing SIMD
        return current; // Signal to use existing SIMD
    }

    Iterator start = current;
    size_t count = 0;

    // Use SHUFTI for character class matching
    while (current != last && (MaxCount == 0 || count < MaxCount)) {
        Iterator pos = current;

        // Try to match one character using SHUFTI
        // Use a smart character class based on the pattern type
        static constexpr character_class smart_char_class = []() {
            character_class c;
            // For now, use alphanumeric as default - this could be made smarter
            c.init_alnum();
            return c;
        }();
        if (match_char_class_shufti(pos, last, smart_char_class)) {
            current = pos;
            ++count;
        } else {
            break;
        }
    }

    // Check if we met the minimum requirement
    if (count >= MinCount) {
        return current;
    } else {
        return start; // No match
    }
}

// Helper function to get character class for a pattern type
template <typename PatternType>
constexpr character_class get_character_class_for_pattern() {
    // This would need to be specialized for different pattern types
    // For now, return a default alphanumeric class
    character_class c;
    c.init_alnum();
    return c;
}

// Forward declaration
template <typename PatternType>
constexpr character_class get_character_class_for_pattern();

// Optimized SHUFTI character class matching for alphanumeric [A-Za-z0-9_]
template <typename Iterator, typename EndIterator>
    requires std::is_same_v<std::remove_cv_t<std::remove_reference_t<decltype(*std::declval<Iterator>())>>, char>
inline bool match_alnum_shufti(Iterator& current, const EndIterator last, const flags& f) {
    (void)f;

    if (current == last)
        return false;

    Iterator pos = current;

    // Process 32-byte chunks using AVX2 for alphanumeric matching
    while (pos != last) {
        // Check if we have at least 32 bytes remaining
        auto temp_pos = pos;
        bool has_32_bytes = true;
        for (int i = 0; i < 32; ++i) {
            if (temp_pos == last) {
                has_32_bytes = false;
                break;
            }
            ++temp_pos;
        }

        if (!has_32_bytes)
            break;

        // Load 32 bytes of input
        const char* data_ptr = &*pos;
        __m256i input = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(data_ptr));

        // Optimized alphanumeric matching using SIMD
        // Check each byte for alphanumeric characters: A-Z, a-z, 0-9, _
        for (int i = 0; i < 32; ++i) {
            unsigned char c = static_cast<unsigned char>(data_ptr[i]);

            // Fast alphanumeric check: (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c
            // == '_'
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_') {
                std::advance(pos, i);
                current = pos;
                return true;
            }
        }

        std::advance(pos, 32);
    }

    // Process remaining bytes
    while (pos != last) {
        unsigned char c = static_cast<unsigned char>(*pos);
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_') {
            current = ++pos;
            return true;
        }
        ++pos;
    }

    return false;
}

// SHUFTI character class matching for whitespace \s
template <typename Iterator, typename EndIterator>
    requires std::is_same_v<std::remove_cv_t<std::remove_reference_t<decltype(*std::declval<Iterator>())>>, char>
inline bool match_whitespace_shufti(Iterator& current, const EndIterator last, const flags& f) {
    (void)f;

    static constexpr character_class whitespace_class = []() {
        character_class c;
        c.init_whitespace();
        return c;
    }();

    return match_char_class_shufti(current, last, whitespace_class);
}

// SHUFTI character class matching for digits [0-9]
template <typename Iterator, typename EndIterator>
    requires std::is_same_v<std::remove_reference_t<decltype(*std::declval<Iterator>())>, char>
inline bool match_digits_shufti(Iterator& current, const EndIterator last, const flags& f) {
    (void)f;

    static constexpr character_class digits_class = []() {
        character_class c;
        c.init_digits();
        return c;
    }();

    return match_char_class_shufti(current, last, digits_class);
}

// SHUFTI character class matching for letters [A-Za-z]
template <typename Iterator, typename EndIterator>
    requires std::is_same_v<std::remove_cv_t<std::remove_reference_t<decltype(*std::declval<Iterator>())>>, char>
inline bool match_letters_shufti(Iterator& current, const EndIterator last, const flags& f) {
    (void)f;

    static constexpr character_class letters_class = []() {
        character_class c;
        c.init_letters();
        return c;
    }();

    return match_char_class_shufti(current, last, letters_class);
}

} // namespace simd
} // namespace ctre

#endif
