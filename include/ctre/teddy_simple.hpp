#ifndef CTRE_TEDDY_SIMPLE_HPP
#define CTRE_TEDDY_SIMPLE_HPP

#include "literal_alternation_fast_path.hpp"
#include "simd_detection.hpp"
#include <immintrin.h>
#include <string_view>
#include <cstring>

// Simple Teddy Implementation for CTRE
// =====================================
// Uses SIMD shuffles (pshufb) to scan for first characters of literals.
// This is a simplified version focusing on clarity and performance.

namespace ctre {
namespace teddy {

// =============================================================================
// Teddy Masks: Compile-Time Lookup Table
// =============================================================================

// Build a 16-byte lookup table for first characters
// Each byte index represents a character, value is a bitmask of which literals start with it
template <size_t MaxLiterals>
struct teddy_mask {
    uint8_t lookup[16];  // 16-byte shuffle mask
    uint8_t literal_ids[256]; // Which literal(s) start with each character

    constexpr teddy_mask() : lookup{}, literal_ids{} {
        for (int i = 0; i < 16; ++i) lookup[i] = 0;
        for (int i = 0; i < 256; ++i) literal_ids[i] = 0xFF;
    }

    // Add a literal's first character
    constexpr void add_first_char(unsigned char c, uint8_t literal_id) {
        // Map character to 4-bit index for pshufb (low nibble)
        uint8_t nibble = c & 0x0F;
        lookup[nibble] |= (1 << (literal_id & 0x07));
        literal_ids[c] = literal_id;
    }
};

// Build teddy mask at compile-time
template <size_t MaxLiterals, size_t MaxLength>
constexpr auto build_teddy_mask(const literal_list<MaxLiterals, MaxLength>& literals) {
    teddy_mask<MaxLiterals> mask;

    for (size_t i = 0; i < literals.count && i < 8; ++i) {
        if (literals.items[i].length > 0) {
            unsigned char first = static_cast<unsigned char>(literals.items[i].data[0]);
            mask.add_first_char(first, static_cast<uint8_t>(i));
        }
    }

    return mask;
}

// =============================================================================
// Teddy Scan: SIMD First-Character Search
// =============================================================================

// Scan for candidate positions using SSE4.2 (16 bytes at a time)
template <size_t MaxLiterals, size_t MaxLength>
inline const char* teddy_scan_sse42(
    const char* begin,
    const char* end,
    const literal_list<MaxLiterals, MaxLength>& literals,
    const teddy_mask<MaxLiterals>& mask) noexcept
{
    // Build the first character set for quick check
    __m128i first_chars[MaxLiterals];
    for (size_t i = 0; i < literals.count && i < MaxLiterals; ++i) {
        if (literals.items[i].length > 0) {
            first_chars[i] = _mm_set1_epi8(literals.items[i].data[0]);
        }
    }

    const char* pos = begin;

    // SIMD loop: process 16 bytes at a time
    while (pos + 16 <= end) {
        __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pos));

        // Check for any first character match
        __m128i matches = _mm_setzero_si128();
        for (size_t i = 0; i < literals.count && i < MaxLiterals; ++i) {
            __m128i cmp = _mm_cmpeq_epi8(chunk, first_chars[i]);
            matches = _mm_or_si128(matches, cmp);
        }

        // Get bitmask of matches
        int match_mask = _mm_movemask_epi8(matches);

        if (match_mask != 0) {
            // Found a candidate! Return first match position
            int offset = __builtin_ctz(match_mask);
            return pos + offset;
        }

        pos += 16;
    }

    // Scalar tail
    while (pos < end) {
        unsigned char c = static_cast<unsigned char>(*pos);
        if (mask.literal_ids[c] != 0xFF) {
            return pos;
        }
        ++pos;
    }

    return end;
}

// Scan for candidate positions using AVX2 (32 bytes at a time)
template <size_t MaxLiterals, size_t MaxLength>
inline const char* teddy_scan_avx2(
    const char* begin,
    const char* end,
    const literal_list<MaxLiterals, MaxLength>& literals,
    const teddy_mask<MaxLiterals>& mask) noexcept
{
    // Build the first character set for quick check
    __m256i first_chars[MaxLiterals];
    for (size_t i = 0; i < literals.count && i < MaxLiterals; ++i) {
        if (literals.items[i].length > 0) {
            first_chars[i] = _mm256_set1_epi8(literals.items[i].data[0]);
        }
    }

    const char* pos = begin;

    // SIMD loop: process 32 bytes at a time
    while (pos + 32 <= end) {
        __m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pos));

        // Check for any first character match
        __m256i matches = _mm256_setzero_si256();
        for (size_t i = 0; i < literals.count && i < MaxLiterals; ++i) {
            __m256i cmp = _mm256_cmpeq_epi8(chunk, first_chars[i]);
            matches = _mm256_or_si256(matches, cmp);
        }

        // Get bitmask of matches
        int match_mask = _mm256_movemask_epi8(matches);

        if (match_mask != 0) {
            // Found a candidate! Return first match position
            int offset = __builtin_ctz(match_mask);
            return pos + offset;
        }

        pos += 32;
    }

    // SSE4.2 for remaining 16+ bytes
    if (pos + 16 <= end) {
        return teddy_scan_sse42(pos, end, literals, mask);
    }

    // Scalar tail
    while (pos < end) {
        unsigned char c = static_cast<unsigned char>(*pos);
        if (mask.literal_ids[c] != 0xFF) {
            return pos;
        }
        ++pos;
    }

    return end;
}

// =============================================================================
// Teddy Match: Fast Literal Alternation Matching
// =============================================================================

// Match using Teddy SIMD scan
template <size_t MaxLiterals, size_t MaxLength>
inline size_t teddy_match(
    std::string_view input,
    const literal_list<MaxLiterals, MaxLength>& literals,
    const teddy_mask<MaxLiterals>& mask) noexcept
{
    const char* begin = input.data();
    const char* end = input.data() + input.size();

    // Quick check: does input start with any first character?
    if (input.empty()) return 0;

    unsigned char first = static_cast<unsigned char>(input[0]);
    if (mask.literal_ids[first] == 0xFF) {
        return 0; // No literal starts with this character
    }

// Check all literals that start with this character (exact match for match operation)
    for (size_t i = 0; i < literals.count; ++i) {
        if (literals.items[i].length > 0 &&
            literals.items[i].data[0] == input[0] &&
            literals.items[i].matches(input)) {  // Exact match!
            return literals.items[i].length;
        }
    }

    return 0;
}

// Search using Teddy SIMD scan
template <size_t MaxLiterals, size_t MaxLength>
inline const char* teddy_search(
    std::string_view input,
    const literal_list<MaxLiterals, MaxLength>& literals,
    const teddy_mask<MaxLiterals>& mask,
    size_t* out_length = nullptr) noexcept
{
    const char* begin = input.data();
    const char* end = input.data() + input.size();
    const char* pos = begin;

    // Use SIMD to find candidate positions
    while (pos < end) {
        // Scan for next candidate (SIMD accelerated!)
        const char* candidate = (simd::get_simd_capability() >= 2) ?
            teddy_scan_avx2(pos, end, literals, mask) :
            teddy_scan_sse42(pos, end, literals, mask);

        if (candidate == end) {
            return nullptr; // No more candidates
        }

        // Verify each literal at this position (prefix match for search!)
        std::string_view remaining(candidate, end - candidate);
        for (size_t i = 0; i < literals.count; ++i) {
            if (literals.items[i].matches_prefix(remaining)) {  // Prefix match for search!
                if (out_length) {
                    *out_length = literals.items[i].length;
                }
                return candidate;
            }
        }

        // No match at this position, continue
        pos = candidate + 1;
    }

    return nullptr;
}

// =============================================================================
// High-Level API
// =============================================================================

// Match with Teddy (compile-time mask building)
template <typename Pattern>
inline size_t teddy_fast_match(std::string_view input) {
    if constexpr (is_literal_alt<Pattern>::value) {
        constexpr auto literals = get_literal_list<Pattern>();
        constexpr auto mask = build_teddy_mask(literals);

        // Use Teddy for match
        return teddy_match(input, literals, mask);
    } else {
        // Not a literal alternation
        return 0;
    }
}

// Search with Teddy
template <typename Pattern>
inline const char* teddy_fast_search(std::string_view input, size_t* out_length = nullptr) {
    if constexpr (is_literal_alt<Pattern>::value) {
        constexpr auto literals = get_literal_list<Pattern>();
        constexpr auto mask = build_teddy_mask(literals);

        // Use Teddy for search
        return teddy_search(input, literals, mask, out_length);
    } else {
        // Not a literal alternation
        return nullptr;
    }
}

} // namespace teddy
} // namespace ctre

#endif // CTRE_TEDDY_SIMPLE_HPP
