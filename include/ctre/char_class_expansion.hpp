#ifndef CTRE__CHAR_CLASS_EXPANSION__HPP
#define CTRE__CHAR_CLASS_EXPANSION__HPP

#ifndef CTRE_IN_A_MODULE
#include <array>
#endif

namespace ctre {

// Forward declarations
template <auto V> struct character;
template <auto A, auto B> struct char_range;
template <typename... Content> struct set;
template <auto... Cs> struct enumeration;
template <typename... Content> struct negative_set;

inline constexpr size_t MAX_CHAR_CLASS_EXPANSION = 11; // Hyperscan paper: <=11 strings

template <size_t MaxChars>
struct char_class_expansion_result {
    std::array<char, MaxChars> chars{};
    size_t count = 0;
    bool is_expandable = false;
};

// Type trait: is this a character class?
template <typename T> struct is_char_class : std::false_type {};
template <auto V> struct is_char_class<character<V>> : std::true_type {};
template <auto A, auto B> struct is_char_class<char_range<A, B>> : std::true_type {};
template <typename... Content> struct is_char_class<set<Content...>> : std::true_type {};
template <auto... Cs> struct is_char_class<enumeration<Cs...>> : std::true_type {};
template <typename... Content> struct is_char_class<negative_set<Content...>> : std::false_type {}; // Don't expand negated
template <typename T> inline constexpr bool is_char_class_v = is_char_class<T>::value;

// Count characters in class
template <typename T> struct char_class_size;
template <auto V> struct char_class_size<character<V>> { static constexpr size_t value = 1; };
template <auto A, auto B> struct char_class_size<char_range<A, B>> {
    static_assert(B >= A, "Invalid character range");
    static constexpr size_t value = static_cast<size_t>(B - A) + 1;
};
template <auto... Cs> struct char_class_size<enumeration<Cs...>> { static constexpr size_t value = sizeof...(Cs); };
template <typename... Content> struct char_class_size<set<Content...>> {
    static constexpr size_t value = (char_class_size<Content>::value + ...);
};
template <typename... Content> struct char_class_size<negative_set<Content...>> {
    static constexpr size_t inner = (char_class_size<Content>::value + ...);
    static constexpr size_t value = (inner > 0) ? (256 - inner) : 256;
};
template <auto... Cs> struct char_class_size<select<character<Cs>...>> { static constexpr size_t value = sizeof...(Cs); };
template <size_t Index, auto... Cs> struct char_class_size<capture<Index, select<character<Cs>...>>> {
    static constexpr size_t value = sizeof...(Cs);
};
template <typename T> struct char_class_size { static constexpr size_t value = 0; };
template <typename T> inline constexpr size_t char_class_size_v = char_class_size<T>::value;

// Legacy function interface (for evaluation.hpp compatibility)
template <typename T>
[[nodiscard]] consteval size_t count_char_class_size(T*) noexcept { return char_class_size_v<T>; }
template <typename T>
[[nodiscard]] consteval size_t count_char_class_size() noexcept { return char_class_size_v<T>; }

// Expansion implementation
template <size_t MaxChars>
constexpr void add_char(char_class_expansion_result<MaxChars>& result, char c) noexcept {
    if (result.count < MaxChars) result.chars[result.count++] = c;
}

template <typename T, size_t MaxChars>
constexpr void expand_impl(char_class_expansion_result<MaxChars>& result) noexcept;

template <auto V, size_t MaxChars>
constexpr void expand_impl_char(character<V>*, char_class_expansion_result<MaxChars>& result) noexcept {
    add_char(result, static_cast<char>(V));
}

template <auto A, auto B, size_t MaxChars>
constexpr void expand_impl_range(char_range<A, B>*, char_class_expansion_result<MaxChars>& result) noexcept {
    for (auto c = A; c <= B && result.count < MaxChars; ++c)
        add_char(result, static_cast<char>(c));
}

template <auto... Cs, size_t MaxChars>
constexpr void expand_impl_enum(enumeration<Cs...>*, char_class_expansion_result<MaxChars>& result) noexcept {
    (add_char(result, static_cast<char>(Cs)), ...);
}

template <typename... Content, size_t MaxChars>
constexpr void expand_impl_set(set<Content...>*, char_class_expansion_result<MaxChars>& result) noexcept {
    (expand_impl<Content, MaxChars>(result), ...);
}

// Type traits for dispatch
template <typename T> struct is_character_type : std::false_type {};
template <auto V> struct is_character_type<character<V>> : std::true_type {};
template <typename T> struct is_char_range_type : std::false_type {};
template <auto A, auto B> struct is_char_range_type<char_range<A, B>> : std::true_type {};
template <typename T> struct is_enumeration_type : std::false_type {};
template <auto... Cs> struct is_enumeration_type<enumeration<Cs...>> : std::true_type {};
template <typename T> struct is_set_type : std::false_type {};
template <typename... Content> struct is_set_type<set<Content...>> : std::true_type {};

template <typename T, size_t MaxChars>
constexpr void expand_impl(char_class_expansion_result<MaxChars>& result) noexcept {
    if constexpr (is_character_type<T>::value)
        expand_impl_char(static_cast<T*>(nullptr), result);
    else if constexpr (is_char_range_type<T>::value)
        expand_impl_range(static_cast<T*>(nullptr), result);
    else if constexpr (is_enumeration_type<T>::value)
        expand_impl_enum(static_cast<T*>(nullptr), result);
    else if constexpr (is_set_type<T>::value)
        expand_impl_set(static_cast<T*>(nullptr), result);
}

// Main expansion function
template <typename T, size_t MaxChars = MAX_CHAR_CLASS_EXPANSION>
[[nodiscard]] constexpr auto expand_char_class() noexcept -> char_class_expansion_result<MaxChars> {
    char_class_expansion_result<MaxChars> result{};
    if constexpr (!is_char_class_v<T>) {
        result.is_expandable = false;
        return result;
    }
    constexpr size_t size = char_class_size_v<T>;
    if constexpr (size == 0 || size > MaxChars) {
        result.is_expandable = false;
        return result;
    }
    // Use pointer dispatch for expansion
    if constexpr (requires { expand_impl_char(static_cast<T*>(nullptr), result); })
        expand_impl_char(static_cast<T*>(nullptr), result);
    else if constexpr (requires { expand_impl_range(static_cast<T*>(nullptr), result); })
        expand_impl_range(static_cast<T*>(nullptr), result);
    else if constexpr (requires { expand_impl_enum(static_cast<T*>(nullptr), result); })
        expand_impl_enum(static_cast<T*>(nullptr), result);
    else if constexpr (requires { expand_impl_set(static_cast<T*>(nullptr), result); })
        expand_impl_set(static_cast<T*>(nullptr), result);

    result.is_expandable = (result.count > 0 && result.count <= MaxChars);
    return result;
}

template <typename T>
[[nodiscard]] consteval bool is_expandable_char_class() noexcept {
    if constexpr (!is_char_class_v<T>) return false;
    return char_class_size_v<T> > 0 && char_class_size_v<T> <= MAX_CHAR_CLASS_EXPANSION;
}

} // namespace ctre

#endif // CTRE__CHAR_CLASS_EXPANSION__HPP
