#ifndef CTRE_TEDDY_FULL_HPP
#define CTRE_TEDDY_FULL_HPP

#include "literal_alternation_fast_path.hpp"
#include "simd_detection.hpp"
#include <immintrin.h>
#include <string_view>
#include <cstring>

// Full Teddy Implementation
// ==========================
// Based on Rust's regex-automata Teddy algorithm
// Uses pshufb shuffle for ultra-fast multi-pattern matching
//
// Key Ideas:
// 1. Build lookup masks for first 1-3 bytes of each literal
// 2. Use pshufb to do parallel lookups (16 at once!)
// 3. Combine masks with AND to find candidates
// 4. Verify only at candidate positions

namespace ctre {
namespace teddy_full {

// =============================================================================
// Teddy Masks: The Core Data Structure
// =============================================================================

// Teddy uses 8 buckets (3-bit IDs) for up to 8 literals
// For more literals, we use multiple Teddy instances ("Fat Teddy")
template <size_t MaxLiterals = 8>
struct teddy_bucket_masks {
    // Shuffle masks for low nibble (4 bits) of each byte
    // mask[nibble] = bitmask of which buckets could match
    alignas(16) uint8_t lo_mask[16];
    alignas(16) uint8_t hi_mask[16];

    // Bucket assignments: which literal is in which bucket (0-7)
    uint8_t bucket_map[MaxLiterals];
    size_t bucket_count;

    constexpr teddy_bucket_masks() : lo_mask{}, hi_mask{}, bucket_map{}, bucket_count(0) {}
};

// Build Teddy masks at compile-time from first bytes of literals
template <size_t MaxLiterals, size_t MaxLength>
constexpr auto build_full_teddy_masks(const literal_list<MaxLiterals, MaxLength>& literals) {
    teddy_bucket_masks<MaxLiterals> masks;

    // Assign each literal to a bucket (0-7)
    for (size_t i = 0; i < literals.count && i < 8; ++i) {
        if (literals.items[i].length > 0) {
            masks.bucket_map[i] = static_cast<uint8_t>(i);

            unsigned char first = static_cast<unsigned char>(literals.items[i].data[0]);
            uint8_t lo_nibble = first & 0x0F;
            uint8_t hi_nibble = (first >> 4) & 0x0F;

            // Set bit for this bucket in the appropriate mask entries
            uint8_t bucket_bit = uint8_t(1) << i;
            masks.lo_mask[lo_nibble] |= bucket_bit;
            masks.hi_mask[hi_nibble] |= bucket_bit;
        }
    }

    masks.bucket_count = literals.count < 8 ? literals.count : 8;
    return masks;
}

// =============================================================================
// Teddy Core: SSSE3 pshufb Shuffle-Based Scanning
// =============================================================================

// Scan 16 bytes using SSSE3 pshufb shuffle
template <size_t MaxLiterals, size_t MaxLength>
inline uint16_t teddy_scan_chunk_ssse3(
    const uint8_t* data,
    const teddy_bucket_masks<MaxLiterals>& masks) noexcept
{
    // Load 16 bytes
    __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data));

    // Extract nibbles
    __m128i lo_nibbles = _mm_and_si128(chunk, _mm_set1_epi8(0x0F));
    __m128i hi_nibbles = _mm_and_si128(_mm_srli_epi16(chunk, 4), _mm_set1_epi8(0x0F));

    // Load masks
    __m128i lo_mask_vec = _mm_load_si128(reinterpret_cast<const __m128i*>(masks.lo_mask));
    __m128i hi_mask_vec = _mm_load_si128(reinterpret_cast<const __m128i*>(masks.hi_mask));

    // Shuffle to get bucket masks for each byte
    __m128i lo_buckets = _mm_shuffle_epi8(lo_mask_vec, lo_nibbles);
    __m128i hi_buckets = _mm_shuffle_epi8(hi_mask_vec, hi_nibbles);

    // AND together: both nibbles must match
    __m128i candidates = _mm_and_si128(lo_buckets, hi_buckets);

    // Convert to bitmask
    return static_cast<uint16_t>(_mm_movemask_epi8(_mm_cmpgt_epi8(candidates, _mm_setzero_si128())));
}

// Scan 32 bytes using AVX2
template <size_t MaxLiterals, size_t MaxLength>
inline uint32_t teddy_scan_chunk_avx2(
    const uint8_t* data,
    const teddy_bucket_masks<MaxLiterals>& masks) noexcept
{
    // Load 32 bytes
    __m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(data));

    // Extract nibbles
    __m256i lo_nibbles = _mm256_and_si256(chunk, _mm256_set1_epi8(0x0F));
    __m256i hi_nibbles = _mm256_and_si256(_mm256_srli_epi16(chunk, 4), _mm256_set1_epi8(0x0F));

    // Load masks (broadcast 16-byte mask to 32 bytes)
    __m128i lo_mask_128 = _mm_load_si128(reinterpret_cast<const __m128i*>(masks.lo_mask));
    __m128i hi_mask_128 = _mm_load_si128(reinterpret_cast<const __m128i*>(masks.hi_mask));
    __m256i lo_mask_vec = _mm256_broadcastsi128_si256(lo_mask_128);
    __m256i hi_mask_vec = _mm256_broadcastsi128_si256(hi_mask_128);

    // Shuffle to get bucket masks for each byte
    __m256i lo_buckets = _mm256_shuffle_epi8(lo_mask_vec, lo_nibbles);
    __m256i hi_buckets = _mm256_shuffle_epi8(hi_mask_vec, hi_nibbles);

    // AND together: both nibbles must match
    __m256i candidates = _mm256_and_si256(lo_buckets, hi_buckets);

    // Convert to bitmask
    return static_cast<uint32_t>(_mm256_movemask_epi8(_mm256_cmpgt_epi8(candidates, _mm256_setzero_si256())));
}

// =============================================================================
// Candidate Verification
// =============================================================================

// Check which literal (if any) matches at this position (for search - prefix match!)
template <size_t MaxLiterals, size_t MaxLength>
inline int verify_candidate(
    const char* pos,
    const char* end,
    const literal_list<MaxLiterals, MaxLength>& literals,
    size_t* out_length) noexcept
{
    std::string_view remaining(pos, end - pos);

    // Check each literal (prefix match for search!)
    for (size_t i = 0; i < literals.count; ++i) {
        if (literals.items[i].matches_prefix(remaining)) {  // Prefix match for search!
            if (out_length) {
                *out_length = literals.items[i].length;
            }
            return static_cast<int>(i);
        }
    }

    return -1;
}

// =============================================================================
// Teddy Match: Exact Match at Start
// =============================================================================

template <size_t MaxLiterals, size_t MaxLength>
inline size_t teddy_match(
    std::string_view input,
    const literal_list<MaxLiterals, MaxLength>& literals,
    const teddy_bucket_masks<MaxLiterals>& masks) noexcept
{
    if (input.empty()) return 0;

    // Fast path: check first byte
    unsigned char first = static_cast<unsigned char>(input[0]);
    uint8_t lo_nibble = first & 0x0F;
    uint8_t hi_nibble = (first >> 4) & 0x0F;

    // Quick reject: first byte doesn't match any literal
    if ((masks.lo_mask[lo_nibble] & masks.hi_mask[hi_nibble]) == 0) {
        return 0;
    }

    // Verify each literal (exact match for match operation!)
    for (size_t i = 0; i < literals.count; ++i) {
        if (literals.items[i].matches(input)) {  // Exact match!
            return literals.items[i].length;
        }
    }

    return 0;
}

// =============================================================================
// Teddy Search: Find First Occurrence (THE KILLER FEATURE!)
// =============================================================================

template <size_t MaxLiterals, size_t MaxLength>
inline const char* teddy_search_ssse3(
    const char* begin,
    const char* end,
    const literal_list<MaxLiterals, MaxLength>& literals,
    const teddy_bucket_masks<MaxLiterals>& masks,
    size_t* out_length) noexcept
{
    const uint8_t* pos = reinterpret_cast<const uint8_t*>(begin);
    const uint8_t* last = reinterpret_cast<const uint8_t*>(end);

    // SIMD loop: process 16 bytes at a time
    while (pos + 16 <= last) {
        uint16_t candidate_mask = teddy_scan_chunk_ssse3<MaxLiterals, MaxLength>(pos, masks);

        if (candidate_mask != 0) {
            // Found candidates! Check each bit
            while (candidate_mask != 0) {
                int offset = __builtin_ctz(candidate_mask);
                candidate_mask &= candidate_mask - 1; // Clear lowest bit

                // Verify this candidate
                const char* candidate_pos = reinterpret_cast<const char*>(pos + offset);
                if (verify_candidate(candidate_pos, end, literals, out_length) >= 0) {
                    return candidate_pos;
                }
            }
        }

        pos += 16;
    }

    // Scalar tail
    while (pos < last) {
        const char* candidate_pos = reinterpret_cast<const char*>(pos);
        if (verify_candidate(candidate_pos, end, literals, out_length) >= 0) {
            return candidate_pos;
        }
        ++pos;
    }

    return nullptr;
}

template <size_t MaxLiterals, size_t MaxLength>
inline const char* teddy_search_avx2(
    const char* begin,
    const char* end,
    const literal_list<MaxLiterals, MaxLength>& literals,
    const teddy_bucket_masks<MaxLiterals>& masks,
    size_t* out_length) noexcept
{
    const uint8_t* pos = reinterpret_cast<const uint8_t*>(begin);
    const uint8_t* last = reinterpret_cast<const uint8_t*>(end);

    // AVX2 loop: process 32 bytes at a time
    while (pos + 32 <= last) {
        uint32_t candidate_mask = teddy_scan_chunk_avx2<MaxLiterals, MaxLength>(pos, masks);

        if (candidate_mask != 0) {
            // Found candidates! Check each bit
            while (candidate_mask != 0) {
                int offset = __builtin_ctz(candidate_mask);
                candidate_mask &= candidate_mask - 1; // Clear lowest bit

                // Verify this candidate
                const char* candidate_pos = reinterpret_cast<const char*>(pos + offset);
                if (verify_candidate(candidate_pos, end, literals, out_length) >= 0) {
                    return candidate_pos;
                }
            }
        }

        pos += 32;
    }

    // SSSE3 for remaining 16+ bytes
    if (pos + 16 <= last) {
        return teddy_search_ssse3(reinterpret_cast<const char*>(pos), end, literals, masks, out_length);
    }

    // Scalar tail
    while (pos < last) {
        const char* candidate_pos = reinterpret_cast<const char*>(pos);
        if (verify_candidate(candidate_pos, end, literals, out_length) >= 0) {
            return candidate_pos;
        }
        ++pos;
    }

    return nullptr;
}

// Dispatch to best SIMD variant
template <size_t MaxLiterals, size_t MaxLength>
inline const char* teddy_search(
    std::string_view input,
    const literal_list<MaxLiterals, MaxLength>& literals,
    const teddy_bucket_masks<MaxLiterals>& masks,
    size_t* out_length = nullptr) noexcept
{
    const char* begin = input.data();
    const char* end = input.data() + input.size();

    // Use best available SIMD
    if (simd::get_simd_capability() >= 2) {
        // AVX2 available
        return teddy_search_avx2(begin, end, literals, masks, out_length);
    } else {
        // Fall back to SSSE3
        return teddy_search_ssse3(begin, end, literals, masks, out_length);
    }
}

// =============================================================================
// High-Level API
// =============================================================================

// Match with full Teddy
template <typename Pattern>
inline size_t match(std::string_view input) {
    if constexpr (is_literal_alt<Pattern>::value) {
        constexpr auto literals = get_literal_list<Pattern>();
        constexpr auto masks = build_full_teddy_masks(literals);
        return teddy_match(input, literals, masks);
    } else {
        return 0;
    }
}

// Search with full Teddy
template <typename Pattern>
inline const char* search(std::string_view input, size_t* out_length = nullptr) {
    if constexpr (is_literal_alt<Pattern>::value) {
        constexpr auto literals = get_literal_list<Pattern>();
        constexpr auto masks = build_full_teddy_masks(literals);
        return teddy_search(input, literals, masks, out_length);
    } else {
        return nullptr;
    }
}

} // namespace teddy_full
} // namespace ctre

#endif // CTRE_TEDDY_FULL_HPP
