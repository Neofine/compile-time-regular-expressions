#ifndef CTRE__BITNFA__STATE_MASK__HPP
#define CTRE__BITNFA__STATE_MASK__HPP

#include <immintrin.h> // For __m128i intrinsics
#include <cstddef>     // For size_t
#include <cstdint>     // For uint64_t

namespace ctre {
namespace bitnfa {

// A wrapper around __m128i to represent a set of up to 128 NFA states.
// Each bit corresponds to an NFA state.
//
// Design:
// - Construction: constexpr-friendly (no SIMD intrinsics)
// - Matching: SIMD-accelerated (runtime intrinsics)
struct StateMask128 {
    __m128i bits;

    // CONSTRUCTION (constexpr-friendly)

    // Default constructor - all zeros
    constexpr StateMask128() : bits{} {}

    // Constructor from raw __m128i (for SIMD operations)
    constexpr StateMask128(__m128i val) : bits(val) {}

    // Constexpr constructor from two 64-bit values
    constexpr StateMask128(uint64_t low, uint64_t high) : bits{} {
        // Use GCC/Clang vector extension for constexpr
        #if defined(__GNUC__) || defined(__clang__)
        bits = __extension__ (__m128i)(__v2di){(long long)low, (long long)high};
        #endif
    }

    // CONSTEXPR ACCESSORS (for compile-time construction)

    constexpr uint64_t get_low() const {
        #if defined(__GNUC__) || defined(__clang__)
        return ((__v2di)bits)[0];
        #else
        return 0; // Fallback
        #endif
    }

    constexpr uint64_t get_high() const {
        #if defined(__GNUC__) || defined(__clang__)
        return ((__v2di)bits)[1];
        #else
        return 0; // Fallback
        #endif
    }

    // Set a bit - CONSTEXPR (returns new mask)
    constexpr StateMask128 set(size_t bit_pos) const {
        if (bit_pos >= 128) return *this;

        uint64_t low = get_low();
        uint64_t high = get_high();

        if (bit_pos < 64) {
            low |= (1ULL << bit_pos);
        } else {
            high |= (1ULL << (bit_pos - 64));
        }

        return StateMask128(low, high);
    }

    // Clear a bit - CONSTEXPR (returns new mask)
    constexpr StateMask128 clear(size_t bit_pos) const {
        if (bit_pos >= 128) return *this;

        uint64_t low = get_low();
        uint64_t high = get_high();

        if (bit_pos < 64) {
            low &= ~(1ULL << bit_pos);
        } else {
            high &= ~(1ULL << (bit_pos - 64));
        }

        return StateMask128(low, high);
    }

    // Test a bit - CONSTEXPR
    constexpr bool test(size_t bit_pos) const {
        if (bit_pos >= 128) return false;

        if (bit_pos < 64) {
            return (get_low() & (1ULL << bit_pos)) != 0;
        } else {
            return (get_high() & (1ULL << (bit_pos - 64))) != 0;
        }
    }

    // RUNTIME SIMD OPERATIONS (for matching)

    // Check if any bit is set (SIMD)
    inline bool any() const {
        return !_mm_testz_si128(bits, bits);
    }

    // Check if all bits are zero (SIMD)
    inline bool none() const {
        return _mm_testz_si128(bits, bits);
    }

    // Bitwise AND (SIMD)
    inline StateMask128 operator&(const StateMask128& other) const {
        return StateMask128(_mm_and_si128(bits, other.bits));
    }

    // Bitwise OR (SIMD)
    inline StateMask128 operator|(const StateMask128& other) const {
        return StateMask128(_mm_or_si128(bits, other.bits));
    }

    // Bitwise XOR (SIMD)
    inline StateMask128 operator^(const StateMask128& other) const {
        return StateMask128(_mm_xor_si128(bits, other.bits));
    }

    // Left shift (constexpr for compile-time, optimized for runtime)
    constexpr StateMask128 operator<<(size_t shift_amount) const {
        if (shift_amount >= 128) return StateMask128();
        if (shift_amount == 0) return *this;

        uint64_t low = get_low();
        uint64_t high = get_high();

        if (shift_amount >= 64) {
            // Shift >= 64: low bits move to high, low becomes 0
            uint64_t new_high = low << (shift_amount - 64);
            return StateMask128(0, new_high);
        } else {
            // Shift < 64: bits shift within and across boundaries
            uint64_t new_low = low << shift_amount;
            uint64_t new_high = (high << shift_amount) | (low >> (64 - shift_amount));
            return StateMask128(new_low, new_high);
        }
    }

    // Fast runtime shift using SIMD (non-constexpr)
    [[nodiscard]] inline StateMask128 shift_runtime(size_t shift_amount) const noexcept {
        if (shift_amount == 0) return *this;
        if (shift_amount >= 128) return StateMask128{};

        const int shift = static_cast<int>(shift_amount);
        if (shift_amount < 64) {
            __m128i shifted = _mm_slli_epi64(bits, shift);
            uint64_t low = _mm_extract_epi64(bits, 0);
            uint64_t carry = low >> (64 - shift_amount);
            uint64_t high = _mm_extract_epi64(shifted, 1);
            shifted = _mm_insert_epi64(shifted, high | carry, 1);
            return StateMask128(shifted);
        } else {
            uint64_t low = _mm_extract_epi64(bits, 0);
            return StateMask128(_mm_insert_epi64(_mm_setzero_si128(), low << (shift_amount - 64), 1));
        }
    }

    // Equality (constexpr-friendly)
    constexpr bool operator==(const StateMask128& other) const {
        return get_low() == other.get_low() && get_high() == other.get_high();
    }

    // Inequality
    constexpr bool operator!=(const StateMask128& other) const {
        return !(*this == other);
    }

    // Count set bits (constexpr-friendly using builtin)
    constexpr size_t count() const {
        return __builtin_popcountll(get_low()) + __builtin_popcountll(get_high());
    }
};

} // namespace bitnfa
} // namespace ctre

#endif // CTRE__BITNFA__STATE_MASK__HPP
