#ifndef CTRE__BITNFA__STATE_MASK__HPP
#define CTRE__BITNFA__STATE_MASK__HPP

#include "../simd_detection.hpp"
#ifdef CTRE_ARCH_X86
#include <immintrin.h> // For __m128i intrinsics
#endif
#include <cstddef>     // For size_t
#include <cstdint>     // For uint64_t

namespace ctre::bitnfa {

// A wrapper to represent a set of up to 128 NFA states.
// Each bit corresponds to an NFA state.
//
// Design:
// - Construction: constexpr-friendly (no SIMD intrinsics)
// - Matching: SIMD-accelerated on x86 (runtime intrinsics)
struct StateMask128 {
#ifdef CTRE_ARCH_X86
    __m128i bits;
#else
    uint64_t low_bits;
    uint64_t high_bits;
#endif

    // CONSTRUCTION (constexpr-friendly)

    // Default constructor - all zeros
#ifdef CTRE_ARCH_X86
    constexpr StateMask128() : bits{} {}
#else
    constexpr StateMask128() : low_bits(0), high_bits(0) {}
#endif

#ifdef CTRE_ARCH_X86
    // Constructor from raw __m128i (for SIMD operations)
    constexpr StateMask128(__m128i val) : bits(val) {}
#endif

    // Constexpr constructor from two 64-bit values
#ifdef CTRE_ARCH_X86
    constexpr StateMask128(uint64_t low, uint64_t high) : bits{} {
        // Use GCC/Clang vector extension for constexpr
        #if defined(__GNUC__) || defined(__clang__)
        bits = __extension__ (__m128i)(__v2di){(long long)low, (long long)high};
        #endif
    }
#else
    constexpr StateMask128(uint64_t low, uint64_t high) : low_bits(low), high_bits(high) {}
#endif

    constexpr uint64_t get_low() const {
#ifdef CTRE_ARCH_X86
        #if defined(__GNUC__) || defined(__clang__)
        return static_cast<uint64_t>(((__v2di)bits)[0]);
        #else
        return 0; // Fallback
        #endif
#else
        return low_bits;
#endif
    }

    constexpr uint64_t get_high() const {
#ifdef CTRE_ARCH_X86
        #if defined(__GNUC__) || defined(__clang__)
        return static_cast<uint64_t>(((__v2di)bits)[1]);
        #else
        return 0; // Fallback
        #endif
#else
        return high_bits;
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

    // RUNTIME OPERATIONS

    // Check if any bit is set
    inline bool any() const {
#ifdef CTRE_ARCH_X86
        return !_mm_testz_si128(bits, bits);
#else
        return (low_bits | high_bits) != 0;
#endif
    }

    // Check if all bits are zero
    inline bool none() const {
#ifdef CTRE_ARCH_X86
        return _mm_testz_si128(bits, bits);
#else
        return (low_bits | high_bits) == 0;
#endif
    }

    // Bitwise AND
    inline StateMask128 operator&(const StateMask128& other) const {
#ifdef CTRE_ARCH_X86
        return StateMask128(_mm_and_si128(bits, other.bits));
#else
        return StateMask128(low_bits & other.low_bits, high_bits & other.high_bits);
#endif
    }

    // Bitwise OR
    inline StateMask128 operator|(const StateMask128& other) const {
#ifdef CTRE_ARCH_X86
        return StateMask128(_mm_or_si128(bits, other.bits));
#else
        return StateMask128(low_bits | other.low_bits, high_bits | other.high_bits);
#endif
    }

    // Bitwise XOR
    inline StateMask128 operator^(const StateMask128& other) const {
#ifdef CTRE_ARCH_X86
        return StateMask128(_mm_xor_si128(bits, other.bits));
#else
        return StateMask128(low_bits ^ other.low_bits, high_bits ^ other.high_bits);
#endif
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

    // Fast runtime shift
    [[nodiscard]] inline StateMask128 shift_runtime(size_t shift_amount) const noexcept {
        if (shift_amount == 0) return *this;
        if (shift_amount >= 128) return StateMask128{};

#ifdef CTRE_ARCH_X86
        const int shift = static_cast<int>(shift_amount);
        if (shift_amount < 64) {
            __m128i shifted = _mm_slli_epi64(bits, shift);
            uint64_t low = static_cast<uint64_t>(_mm_extract_epi64(bits, 0));
            uint64_t carry = low >> (64 - shift_amount);
            uint64_t high = static_cast<uint64_t>(_mm_extract_epi64(shifted, 1));
            shifted = _mm_insert_epi64(shifted, static_cast<long long>(high | carry), 1);
            return StateMask128(shifted);
        } else {
            uint64_t low = static_cast<uint64_t>(_mm_extract_epi64(bits, 0));
            return StateMask128(_mm_insert_epi64(_mm_setzero_si128(), static_cast<long long>(low << (shift_amount - 64)), 1));
        }
#else
        // Use the constexpr implementation
        return *this << shift_amount;
#endif
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
        return static_cast<size_t>(__builtin_popcountll(get_low())) + static_cast<size_t>(__builtin_popcountll(get_high()));
    }
};

} // namespace ctre::bitnfa

#endif // CTRE__BITNFA__STATE_MASK__HPP
