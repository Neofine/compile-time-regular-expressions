#ifndef CTRE__SIMD_SHIFT_OR__HPP
#define CTRE__SIMD_SHIFT_OR__HPP

#include "flags_and_modes.hpp"
#include "simd_detection.hpp"
#include <array>
#include <cstring>
#include <immintrin.h>
#include <iterator>

namespace ctre {
namespace simd {

// Threshold for using Shift-Or algorithm (shorter strings benefit more)
constexpr size_t SHIFT_OR_THRESHOLD = 32;

// Maximum pattern length for Shift-Or (limited by bit-vector size)
constexpr size_t MAX_SHIFT_OR_PATTERN_LENGTH = 64;

// Shift-Or state for exact string matching
template <size_t PatternLength>
struct shift_or_state {
    static_assert(PatternLength <= MAX_SHIFT_OR_PATTERN_LENGTH, "Pattern too long for Shift-Or");

    // Character masks - one bit per character position
    std::array<uint64_t, 256> char_masks;

    // Initialize character masks for exact string matching
    template <auto... Chars>
    constexpr void init_exact_pattern() {
        constexpr char pattern[] = {static_cast<char>(Chars)...};
        static_assert(sizeof...(Chars) == PatternLength, "Pattern length mismatch");

        // Initialize all masks to all 1s (no match)
        for (auto& mask : char_masks) {
            mask = ~uint64_t(0);
        }

        // Set 0 bits for matching characters at each position
        for (size_t i = 0; i < PatternLength; ++i) {
            char_masks[static_cast<unsigned char>(pattern[i])] &= ~(uint64_t(1) << i);
        }
    }

    // Initialize character masks for character class matching
    template <typename CharClass>
    constexpr void init_char_class_pattern() {
        // Initialize all masks to all 1s (no match)
        for (auto& mask : char_masks) {
            mask = ~uint64_t(0);
        }

        // Set 0 bits for characters that match the class at each position
        for (size_t i = 0; i < PatternLength; ++i) {
            for (int c = 0; c < 256; ++c) {
                if (CharClass::match_char(static_cast<char>(c), flags{})) {
                    char_masks[c] &= ~(uint64_t(1) << i);
                }
            }
        }
    }
};

// SIMD-optimized Shift-Or implementation using AVX2
template <size_t PatternLength, typename Iterator, typename EndIterator>
    requires std::is_same_v<std::remove_cv_t<std::remove_reference_t<decltype(*std::declval<Iterator>())>>, char>
inline bool match_shift_or_avx2(Iterator& current, const EndIterator last, const shift_or_state<PatternLength>& state) {
    if (current == last)
        return false;

    Iterator pos = current;
    uint64_t state_vector = ~uint64_t(0); // Start with all 1s (no match)

    // Process 32-byte chunks using AVX2
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

        // Process each byte in the chunk using array access
        uint8_t bytes[32];
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(bytes), input);

        for (int i = 0; i < 32; ++i) {
            // Shift state and OR with character mask
            state_vector = (state_vector << 1) | state.char_masks[bytes[i]];

            // Check if we have a match (bit at position PatternLength-1 is 0)
            if ((state_vector & (uint64_t(1) << (PatternLength - 1))) == 0) {
                std::advance(pos, i + 1);
                current = pos;
                return true;
            }
        }

        std::advance(pos, 32);
    }

    // Process remaining bytes
    while (pos != last) {
        uint8_t byte_val = static_cast<uint8_t>(*pos);
        state_vector = (state_vector << 1) | state.char_masks[byte_val];

        if ((state_vector & (uint64_t(1) << (PatternLength - 1))) == 0) {
            current = pos + 1;
            return true;
        }

        ++pos;
    }

    return false;
}

// SIMD-optimized Shift-Or implementation using SSE4.2
template <size_t PatternLength, typename Iterator, typename EndIterator>
    requires std::is_same_v<std::remove_cv_t<std::remove_reference_t<decltype(*std::declval<Iterator>())>>, char>
inline bool match_shift_or_sse42(Iterator& current, const EndIterator last,
                                 const shift_or_state<PatternLength>& state) {
    if (current == last)
        return false;

    Iterator pos = current;
    uint64_t state_vector = ~uint64_t(0); // Start with all 1s (no match)

    // Process 16-byte chunks using SSE4.2
    while (pos != last) {
        // Check if we have at least 16 bytes remaining
        auto temp_pos = pos;
        bool has_16_bytes = true;
        for (int i = 0; i < 16; ++i) {
            if (temp_pos == last) {
                has_16_bytes = false;
                break;
            }
            ++temp_pos;
        }

        if (!has_16_bytes)
            break;

        // Load 16 bytes of input
        const char* data_ptr = &*pos;
        __m128i input = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data_ptr));

        // Process each byte in the chunk using array access
        uint8_t bytes[16];
        _mm_storeu_si128(reinterpret_cast<__m128i*>(bytes), input);

        for (int i = 0; i < 16; ++i) {
            // Shift state and OR with character mask
            state_vector = (state_vector << 1) | state.char_masks[bytes[i]];

            // Check if we have a match (bit at position PatternLength-1 is 0)
            if ((state_vector & (uint64_t(1) << (PatternLength - 1))) == 0) {
                std::advance(pos, i + 1);
                current = pos;
                return true;
            }
        }

        std::advance(pos, 16);
    }

    // Process remaining bytes
    while (pos != last) {
        uint8_t byte_val = static_cast<uint8_t>(*pos);
        state_vector = (state_vector << 1) | state.char_masks[byte_val];

        if ((state_vector & (uint64_t(1) << (PatternLength - 1))) == 0) {
            current = pos + 1;
            return true;
        }

        ++pos;
    }

    return false;
}

// Scalar Shift-Or implementation
template <size_t PatternLength, typename Iterator, typename EndIterator>
    requires std::is_same_v<std::remove_cv_t<std::remove_reference_t<decltype(*std::declval<Iterator>())>>, char>
inline bool match_shift_or_scalar(Iterator& current, const EndIterator last,
                                  const shift_or_state<PatternLength>& state) {
    if (current == last)
        return false;

    Iterator pos = current;
    uint64_t state_vector = ~uint64_t(0); // Start with all 1s (no match)

    // Process each character
    while (pos != last) {
        uint8_t byte_val = static_cast<uint8_t>(*pos);

        // Shift state and OR with character mask
        state_vector = (state_vector << 1) | state.char_masks[byte_val];

        // Check if we have a match (bit at position PatternLength-1 is 0)
        if ((state_vector & (uint64_t(1) << (PatternLength - 1))) == 0) {
            current = pos + 1;
            return true;
        }

        ++pos;
    }

    return false;
}

// Main Shift-Or matching function with automatic SIMD selection
template <size_t PatternLength, typename Iterator, typename EndIterator>
    requires std::is_same_v<std::remove_cv_t<std::remove_reference_t<decltype(*std::declval<Iterator>())>>, char>
inline bool match_shift_or(Iterator& current, const EndIterator last, const shift_or_state<PatternLength>& state) {
    if constexpr (CTRE_SIMD_ENABLED) {
        if (get_simd_capability() >= SIMD_CAPABILITY_AVX2) {
            return match_shift_or_avx2<PatternLength>(current, last, state);
        } else if (get_simd_capability() >= SIMD_CAPABILITY_SSE42) {
            return match_shift_or_sse42<PatternLength>(current, last, state);
        }
    }

    return match_shift_or_scalar<PatternLength>(current, last, state);
}

// ============================================================================
// CTRE INTEGRATION FUNCTIONS
// ============================================================================

// Shift-Or string matching for exact patterns
template <auto... String, typename Iterator, typename EndIterator>
    requires std::is_same_v<std::remove_cv_t<std::remove_reference_t<decltype(*std::declval<Iterator>())>>, char>
inline bool match_string_shift_or(Iterator& current, const EndIterator last, const flags& f) {
    (void)f; // Suppress unused parameter warning
    constexpr size_t string_length = sizeof...(String);
    static_assert(string_length <= MAX_SHIFT_OR_PATTERN_LENGTH, "Pattern too long for Shift-Or");

    // Create and initialize Shift-Or state
    static constexpr shift_or_state<string_length> state = []() {
        shift_or_state<string_length> s;
        s.template init_exact_pattern<String...>();
        return s;
    }();

    // Use Shift-Or algorithm
    return match_shift_or<string_length>(current, last, state);
}

// Multi-pattern Shift-Or state for searching multiple patterns simultaneously
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
        // Initialize character masks for this pattern
        for (auto& mask : state.char_masks) {
            mask = ~uint64_t(0);
        }

        // Set 0 bits for matching characters at each position
        for (size_t i = 0; i < PatternLength; ++i) {
            state.char_masks[static_cast<unsigned char>(pattern[i])] &= ~(uint64_t(1) << i);
        }
    }
};

// Multi-pattern Shift-Or matching function
template <size_t NumPatterns, size_t MaxPatternLength, typename Iterator, typename EndIterator>
    requires std::is_same_v<std::remove_cv_t<std::remove_reference_t<decltype(*std::declval<Iterator>())>>, char>
inline bool match_multi_pattern_shift_or(Iterator& current, const EndIterator last,
                                         const multi_pattern_shift_or_state<NumPatterns, MaxPatternLength>& state) {
    if (current == last)
        return false;

    Iterator pos = current;

    // Try each pattern individually (simplified approach)
    for (size_t i = 0; i < NumPatterns; ++i) {
        Iterator test_pos = pos;
        if (match_shift_or<MaxPatternLength>(test_pos, last, state.pattern_states[i])) {
            current = test_pos;
            return true;
        }
    }

    return false;
}

// Multi-pattern Shift-Or for common use cases
template <typename Iterator, typename EndIterator>
    requires std::is_same_v<std::remove_cv_t<std::remove_reference_t<decltype(*std::declval<Iterator>())>>, char>
inline bool match_keywords_shift_or(Iterator& current, const EndIterator last, const flags& f) {
    (void)f;

    // Search for common keywords: CTRE, REGX, SCAN, FIND
    static constexpr multi_pattern_shift_or_state<4, 4> state = []() {
        multi_pattern_shift_or_state<4, 4> s;
        s.init_multi_pattern(std::array<char, 4>{'C', 'T', 'R', 'E'}, std::array<char, 4>{'R', 'E', 'G', 'X'},
                             std::array<char, 4>{'S', 'C', 'A', 'N'}, std::array<char, 4>{'F', 'I', 'N', 'D'});
        return s;
    }();

    return match_multi_pattern_shift_or<4, 4>(current, last, state);
}

// Shift-Or character class matching
template <typename CharClass, size_t Count, typename Iterator, typename EndIterator>
inline bool match_char_class_shift_or(Iterator& current, const EndIterator last, const flags& f) {
    static_assert(Count <= MAX_SHIFT_OR_PATTERN_LENGTH, "Pattern too long for Shift-Or");

    // Create and initialize Shift-Or state
    static constexpr shift_or_state<Count> state = []() {
        shift_or_state<Count> s;
        s.template init_char_class_pattern<CharClass>();
        return s;
    }();

    // Use Shift-Or algorithm
    return match_shift_or<Count>(current, last, state);
}

} // namespace simd
} // namespace ctre

#endif
