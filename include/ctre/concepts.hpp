#ifndef CTRE__CONCEPTS__HPP
#define CTRE__CONCEPTS__HPP

#include <concepts>
#include <iterator>
#include <type_traits>

namespace ctre {

// C++20 CONCEPTS FOR TYPE SAFETY

/// Concept: Iterator that points to character data
template <typename T>
concept CharIterator = requires(T it) {
    // Must be dereferenceable to a char-like type
    { *it } -> std::convertible_to<char>;
    // Must be incrementable
    { ++it } -> std::same_as<T&>;
    // Must support iterator operations
    requires std::input_or_output_iterator<T>;
};

/// Concept: Sentinel for CharIterator (can be different type)
template <typename S, typename I>
concept CharSentinel = requires(S s, I i) {
    // Must be comparable with iterator
    { i != s } -> std::convertible_to<bool>;
    { i == s } -> std::convertible_to<bool>;
};

/// Concept: Random access iterator for efficient SIMD processing
template <typename T>
concept RandomAccessCharIterator = CharIterator<T> && requires(T it, std::ptrdiff_t n) {
    // Must support pointer arithmetic
    { it + n } -> std::convertible_to<T>;
    { it - n } -> std::convertible_to<T>;
    { it += n } -> std::same_as<T&>;
    // Must support distance calculation
    requires std::random_access_iterator<T>;
};

/// Concept: Contiguous iterator (pointers) - best for SIMD
template <typename T>
concept ContiguousCharIterator = RandomAccessCharIterator<T> && std::contiguous_iterator<T>;

/// Concept: String-like type
template <typename T>
concept StringLike = requires(T str) {
    // Must have begin() and end() that return char iterators
    { str.begin() } -> CharIterator;
    { str.end() } -> CharIterator;
    // Must have size() or length()
    { str.size() } -> std::convertible_to<std::size_t>;
};

/// Concept: String view-like type (non-owning)
template <typename T>
concept StringViewLike = StringLike<T> && std::is_trivially_copyable_v<T>;

/// Concept: Character type (for pattern matching)
template <typename T>
concept Character = std::same_as<std::remove_cv_t<T>, char> || std::same_as<std::remove_cv_t<T>, signed char> ||
                    std::same_as<std::remove_cv_t<T>, unsigned char> || std::same_as<std::remove_cv_t<T>, char8_t>;

/// Concept: Iterator pair that supports SIMD operations
template <typename I, typename E>
concept SimdCompatibleRange = CharIterator<I> && CharSentinel<E, I> && requires(I begin, E end) {
    // Must be able to compute distance
    { end - begin } -> std::convertible_to<std::ptrdiff_t>;
};

/// Concept: Type that can be used as a regex pattern
template <typename T>
concept RegexPattern = requires {
    typename T::value_type;
};

// HELPER TYPE TRAITS (as concepts)

/// Check if iterator is a raw pointer (best case for SIMD)
template <typename T>
concept PointerIterator = std::is_pointer_v<T>;

/// Check if type supports subtraction (for distance calculation)
template <typename T>
concept Subtractable = requires(T a, T b) {
    { a - b } -> std::convertible_to<std::ptrdiff_t>;
};

/// Check if type is nothrow dereferenceable
template <typename T>
concept NothrowDereferenceable = requires(T t) {
    { *t } noexcept;
};

} // namespace ctre

#endif // CTRE__CONCEPTS__HPP
