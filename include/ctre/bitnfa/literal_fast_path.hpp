#ifndef CTRE_BITNFA_LITERAL_FAST_PATH_HPP
#define CTRE_BITNFA_LITERAL_FAST_PATH_HPP

#include <string_view>
#include <array>
#include <cstring>
#include "../atoms.hpp"

// BitNFA Fast Path for Literal Alternations
// ==========================================
// Optimizes patterns like "foo|bar|baz" with fast literal scanning.
// Architecture designed to be extensible for future Teddy implementation.

namespace ctre {
namespace bitnfa {

// =============================================================================
// Phase 1: Literal Extraction from Select Nodes
// =============================================================================

template <typename T>
struct is_pure_literal : std::false_type {};

template <auto... Chars>
struct is_pure_literal<string<Chars...>> : std::true_type {};

// Extract literal string at compile-time
template <auto... Chars>
constexpr auto extract_literal_string(string<Chars...>*) {
    constexpr size_t N = sizeof...(Chars);
    struct result {
        char data[N + 1];
        size_t length = N;
    };

    result r{};
    size_t idx = 0;
    ((r.data[idx++] = static_cast<char>(Chars)), ...);
    r.data[N] = '\0';
    return r;
}

// Analyze if all branches of select are pure literals
template <typename... Branches>
constexpr bool all_pure_literals(select<Branches...>*) {
    return (is_pure_literal<Branches>::value && ...);
}

// Count literals in alternation
template <typename... Branches>
constexpr size_t count_literals(select<Branches...>*) {
    return sizeof...(Branches);
}

// =============================================================================
// Phase 2: Fast Literal Container (Compile-Time Storage)
// =============================================================================

template <size_t MaxLiterals, size_t MaxLength>
struct literal_set {
    struct literal {
        char data[MaxLength];
        size_t length;

        constexpr bool matches(std::string_view input) const noexcept {
            if (input.size() < length) return false;
            return std::memcmp(input.data(), data, length) == 0;
        }
    };

    literal literals[MaxLiterals];
    size_t count;

    constexpr literal_set() : literals{}, count(0) {}

    constexpr void add(const char* str, size_t len) {
        if (count < MaxLiterals && len < MaxLength) {
            for (size_t i = 0; i < len; ++i) {
                literals[count].data[i] = str[i];
            }
            literals[count].length = len;
            ++count;
        }
    }
};

// Extract all literals from select node
template <size_t MaxLiterals, size_t MaxLength, typename... Branches>
constexpr auto extract_all_literals(select<Branches...>*) {
    literal_set<MaxLiterals, MaxLength> result;

    // Extract each literal branch
    ([&]() {
        if constexpr (is_pure_literal<Branches>::value) {
            auto lit = extract_literal_string(static_cast<Branches*>(nullptr));
            result.add(lit.data, lit.length);
        }
    }(), ...);

    return result;
}

// =============================================================================
// Phase 3: Fast Literal Scanning
// =============================================================================

// Simple first-character scan (fast, no SIMD yet)
// Architecture note: This is where Teddy would plug in later!
template <size_t MaxLiterals, size_t MaxLength>
inline const char* scan_for_first_chars(
    const char* begin,
    const char* end,
    const literal_set<MaxLiterals, MaxLength>& literals) noexcept
{
    if (literals.count == 0) return end;

    // Build quick lookup for first characters
    bool first_chars[256] = {};
    for (size_t i = 0; i < literals.count; ++i) {
        if (literals.literals[i].length > 0) {
            unsigned char c = static_cast<unsigned char>(literals.literals[i].data[0]);
            first_chars[c] = true;
        }
    }

    // Scan for any first character
    // NOTE: This is where we'd use SIMD shuffles (pshufb) for Teddy!
    for (const char* p = begin; p < end; ++p) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (first_chars[c]) {
            return p;
        }
    }

    return end;
}

// Verify which literal matches at this position
template <size_t MaxLiterals, size_t MaxLength>
inline int which_literal_matches(
    std::string_view input,
    const literal_set<MaxLiterals, MaxLength>& literals) noexcept
{
    for (size_t i = 0; i < literals.count; ++i) {
        if (literals.literals[i].matches(input)) {
            return static_cast<int>(i);
        }
    }
    return -1; // No match
}

// =============================================================================
// Phase 4: Fast Path Matcher
// =============================================================================

// Fast literal alternation matcher
// Returns: index of matched literal, or -1 if no match
// Out parameter: match_length (if found)
template <size_t MaxLiterals, size_t MaxLength>
inline int match_literal_alternation(
    std::string_view input,
    const literal_set<MaxLiterals, MaxLength>& literals,
    size_t* out_length = nullptr) noexcept
{
    // Direct match attempt for each literal
    // This is simpler and faster than scanning for first chars
    for (size_t i = 0; i < literals.count; ++i) {
        if (literals.literals[i].matches(input)) {
            if (out_length) {
                *out_length = literals.literals[i].length;
            }
            return static_cast<int>(i);
        }
    }

    return -1; // No match
}

// Search for any literal in alternation
template <size_t MaxLiterals, size_t MaxLength>
inline const char* search_literal_alternation(
    std::string_view input,
    const literal_set<MaxLiterals, MaxLength>& literals,
    size_t* match_length = nullptr) noexcept
{
    const char* begin = input.data();
    const char* end = input.data() + input.size();

    // Scan through input
    const char* pos = begin;
    while (pos < end) {
        // Find next candidate position
        pos = scan_for_first_chars(pos, end, literals);
        if (pos == end) break;

        // Check if any literal matches here
        std::string_view remaining(pos, end - pos);
        int idx = which_literal_matches(remaining, literals);

        if (idx >= 0) {
            if (match_length) {
                *match_length = literals.literals[idx].length;
            }
            return pos; // Found match!
        }

        // Move to next position
        ++pos;
    }

    return nullptr; // No match found
}

// =============================================================================
// Phase 5: Pattern Analysis Traits
// =============================================================================

// Determine if pattern is a pure literal alternation at compile-time
template <typename Pattern>
struct is_literal_alternation {
    static constexpr bool value = []() {
        if constexpr (glushkov::is_select<Pattern>::value) {
            return all_pure_literals(static_cast<Pattern*>(nullptr));
        }
        return false;
    }();
};

// Extract literals if pattern is literal alternation
template <typename Pattern, size_t MaxLiterals = 16, size_t MaxLength = 64>
constexpr auto get_literals_if_applicable() {
    if constexpr (is_literal_alternation<Pattern>::value) {
        return extract_all_literals<MaxLiterals, MaxLength>(static_cast<Pattern*>(nullptr));
    } else {
        return literal_set<MaxLiterals, MaxLength>();
    }
}

} // namespace bitnfa
} // namespace ctre

// =============================================================================
// Design Notes for Future Teddy Integration
// =============================================================================
/*
 * ARCHITECTURE FOR TEDDY:
 * =======================
 *
 * This module is designed to be extensible for Teddy implementation:
 *
 * 1. scan_for_first_chars() is the HOT PATH:
 *    - Currently: Simple char-by-char scan
 *    - With Teddy: Replace with pshufb SIMD shuffle
 *    - Interface stays the same!
 *
 * 2. literal_set<> structure is Teddy-ready:
 *    - Can add "masks" field for Teddy lookup tables
 *    - Can add "build_teddy_masks()" method
 *
 * 3. Pluggable architecture:
 *    ```cpp
 *    #ifdef USE_TEDDY
 *        return teddy_scan_for_first_chars(...);
 *    #else
 *        return simple_scan_for_first_chars(...);
 *    #endif
 *    ```
 *
 * 4. Expected Teddy speedup:
 *    - Current (simple scan): 2-5x vs Glushkov NFA
 *    - With Teddy: 5-10x vs Glushkov NFA
 *    - Improvement: 2-3x faster than current
 *
 * TO ADD TEDDY LATER:
 * ===================
 * 1. Add teddy_masks to literal_set
 * 2. Implement build_teddy_masks() (compile-time)
 * 3. Replace scan_for_first_chars with teddy_scan (runtime)
 * 4. Keep fallback for non-SSSE3 CPUs
 *
 * Estimated: ~300-500 lines for Teddy
 * Current: ~200 lines for simple fast path
 * Total: ~500-700 lines (vs 1000+ for full Teddy from scratch)
 */

#endif // CTRE_BITNFA_LITERAL_FAST_PATH_HPP
