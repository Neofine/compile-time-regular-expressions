#ifndef CTRE__SIMD_LITERAL_SEARCH__HPP
#define CTRE__SIMD_LITERAL_SEARCH__HPP

// SIMD-accelerated literal substring search for prefiltering
// Searches for a compile-time known literal in runtime input

#include "simd_detection.hpp"
#include <cstring>

#if CTRE_SIMD_ENABLED
#include <immintrin.h>
#endif

namespace ctre {
namespace simd {

// =============================================================================
// SIMD Literal Search (like fast strstr with compile-time literal)
// =============================================================================

#if CTRE_SIMD_ENABLED && defined(__AVX2__)
// AVX2 implementation: search for literal in input
template <size_t LiteralLen>
inline bool search_literal_avx2(const char* begin, const char* end, const char (&literal)[LiteralLen]) {
	const size_t len = LiteralLen - 1; // Exclude null terminator
	if (len == 0) return true;  // Empty literal always matches

	const char* search_end = end - len + 1;
	if (begin >= search_end) return false;  // Input too short

	// Broadcast first character to all 32 positions
	__m256i first_char = _mm256_set1_epi8(literal[0]);

	const char* ptr = begin;

	// Process 32 bytes at a time
	while (ptr + 32 <= search_end) {
		__m256i chunk = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(ptr));
		__m256i cmp = _mm256_cmpeq_epi8(chunk, first_char);
		int mask = _mm256_movemask_epi8(cmp);

		// Check each potential match position
		while (mask != 0) {
			int pos = __builtin_ctz(mask);

			// Verify full literal match
			if (std::memcmp(ptr + pos, literal, len) == 0) {
				return true;  // Found!
			}

			// Clear this bit and continue
			mask &= mask - 1;
		}

		ptr += 32;
	}

	// Scalar tail for remaining bytes
	while (ptr < search_end) {
		if (*ptr == literal[0] && std::memcmp(ptr, literal, len) == 0) {
			return true;
		}
		++ptr;
	}

	return false;
}
#endif

#if CTRE_SIMD_ENABLED && defined(__SSE4_2__)
// Forward declaration for scalar
template <size_t LiteralLen>
inline bool search_literal_scalar(const char* begin, const char* end, const char (&literal)[LiteralLen]);

// SSE4.2 implementation: use hardware string instructions
template <size_t LiteralLen>
inline bool search_literal_sse42(const char* begin, const char* end, const char (&literal)[LiteralLen]) {
	const size_t len = LiteralLen - 1;
	if (len == 0) return true;

	const char* search_end = end - len + 1;
	if (begin >= search_end) return false;

	// For very short literals, just use scalar
	if (len > 16) {
		// Fallback to scalar for literals > 16 bytes
		// (SSE4.2 string instructions limited to 16 bytes)
		return search_literal_scalar(begin, end, literal);
	}

	// Load literal into XMM register
	__m128i pattern = _mm_setzero_si128();
	std::memcpy(&pattern, literal, len);

	const char* ptr = begin;

	// Use pcmpistri to search for the literal
	// This is a hardware string search instruction!
	while (ptr + 16 <= search_end) {
		__m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr));

		// _mm_cmpistri: compare implicit-length strings
		// Returns index of first match, or 16 if no match
		// Mode 0x0C: EQUAL_ORDERED (substring search)
		int idx = _mm_cmpistri(pattern, chunk, 0x0C);

		if (idx < 16) {
			// Found potential match, verify full literal
			if (ptr + idx + len <= end) {
				if (std::memcmp(ptr + idx, literal, len) == 0) {
					return true;
				}
			}
		}

		ptr += 16;
	}

	// Scalar tail
	while (ptr < search_end) {
		if (std::memcmp(ptr, literal, len) == 0) {
			return true;
		}
		++ptr;
	}

	return false;
}
#endif

// Forward declaration
template <size_t LiteralLen>
inline bool search_literal_scalar(const char* begin, const char* end, const char (&literal)[LiteralLen]);

// Scalar fallback (always available)
template <size_t LiteralLen>
inline bool search_literal_scalar(const char* begin, const char* end, const char (&literal)[LiteralLen]) {
	const size_t len = LiteralLen - 1;
	if (len == 0) return true;

	const char* search_end = end - len + 1;
	if (begin >= search_end) return false;

	const char first = literal[0];

	for (const char* ptr = begin; ptr < search_end; ++ptr) {
		if (*ptr == first) {
			bool match = true;
			for (size_t i = 1; i < len; ++i) {
				if (ptr[i] != literal[i]) {
					match = false;
					break;
				}
			}
			if (match) return true;
		}
	}

	return false;
}

// Dispatcher: selects best implementation based on capabilities
template <size_t LiteralLen>
inline bool search_literal(const char* begin, const char* end, const char (&literal)[LiteralLen]) {
	const size_t input_size = end - begin;

#if CTRE_SIMD_ENABLED
	// Use AVX2 for inputs >= 32 bytes
	#if defined(__AVX2__)
	if (input_size >= 32 && get_simd_capability() >= SIMD_CAPABILITY_AVX2) {
		return search_literal_avx2(begin, end, literal);
	}
	#endif

	// Use SSE4.2 for inputs >= 16 bytes and literals <= 16 bytes
	#if defined(__SSE4_2__)
	if (input_size >= 16 && LiteralLen <= 17 && get_simd_capability() >= SIMD_CAPABILITY_SSE42) {
		return search_literal_sse42(begin, end, literal);
	}
	#endif
#endif

	// Fallback to scalar
	return search_literal_scalar(begin, end, literal);
}

// =============================================================================
// Template-based interface (compile-time literal)
// =============================================================================

template <char... Chars>
inline bool search_literal_ct(const char* begin, const char* end) {
	constexpr char literal[] = {Chars..., '\0'};
	return search_literal(begin, end, literal);
}

} // namespace simd
} // namespace ctre

#endif // CTRE__SIMD_LITERAL_SEARCH__HPP
