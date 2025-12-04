#ifndef CTRE__PATTERN_TRAITS__HPP
#define CTRE__PATTERN_TRAITS__HPP

// Consolidated type traits for CTRE pattern types
// Include this instead of defining traits locally

#include "atoms.hpp"
#include <type_traits>

namespace ctre::traits {

// String
template <typename T> struct is_string : std::false_type {};
template <auto... Cs> struct is_string<string<Cs...>> : std::true_type {
    static constexpr size_t length = sizeof...(Cs);
};
template <typename T> inline constexpr bool is_string_v = is_string<T>::value;

// Sequence
template <typename T> struct is_sequence : std::false_type {};
template <typename... Content> struct is_sequence<sequence<Content...>> : std::true_type {};
template <typename T> inline constexpr bool is_sequence_v = is_sequence<T>::value;

// Select (alternation)
template <typename T> struct is_select : std::false_type {};
template <typename... Options> struct is_select<select<Options...>> : std::true_type {};
template <typename T> inline constexpr bool is_select_v = is_select<T>::value;

// Character
template <typename T> struct is_character : std::false_type {};
template <auto C> struct is_character<character<C>> : std::true_type {};
template <typename T> inline constexpr bool is_character_v = is_character<T>::value;

// Capture
template <typename T> struct is_capture : std::false_type {};
template <size_t Index, typename... Content> struct is_capture<capture<Index, Content...>> : std::true_type {};
template <typename T> inline constexpr bool is_capture_v = is_capture<T>::value;

// Repeat variants
template <typename T> struct is_repeat : std::false_type {};
template <size_t A, size_t B, typename... Content> struct is_repeat<repeat<A, B, Content...>> : std::true_type {};
template <typename T> inline constexpr bool is_repeat_v = is_repeat<T>::value;

template <typename T> struct is_lazy_repeat : std::false_type {};
template <size_t A, size_t B, typename... Content> struct is_lazy_repeat<lazy_repeat<A, B, Content...>> : std::true_type {};
template <typename T> inline constexpr bool is_lazy_repeat_v = is_lazy_repeat<T>::value;

template <typename T> struct is_possessive_repeat : std::false_type {};
template <size_t A, size_t B, typename... Content> struct is_possessive_repeat<possessive_repeat<A, B, Content...>> : std::true_type {};
template <typename T> inline constexpr bool is_possessive_repeat_v = is_possessive_repeat<T>::value;

template <typename T> inline constexpr bool is_any_repeat_v = is_repeat_v<T> || is_lazy_repeat_v<T> || is_possessive_repeat_v<T>;

// Any (.)
template <typename T> struct is_any : std::false_type {};
template <> struct is_any<any> : std::true_type {};
template <typename T> inline constexpr bool is_any_v = is_any<T>::value;

// Empty
template <typename T> struct is_empty : std::false_type {};
template <> struct is_empty<empty> : std::true_type {};
template <typename T> inline constexpr bool is_empty_v = is_empty<T>::value;

// Concept: Has match_char method
template <typename T>
concept CharacterLike = requires { { T::match_char(char{}, flags{}) } -> std::same_as<bool>; };

} // namespace ctre::traits

#endif // CTRE__PATTERN_TRAITS__HPP
