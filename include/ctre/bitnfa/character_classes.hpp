#ifndef CTRE_BITNFA_CHARACTER_CLASSES_HPP
#define CTRE_BITNFA_CHARACTER_CLASSES_HPP

#include "bitnfa_types.hpp"
#include "../atoms_characters.hpp"

namespace ctre::bitnfa {

template <auto V>
constexpr void expand_character(ReachabilityTable& table, size_t state) {
    table.set_reachable_mut(static_cast<char>(V), state);
}

template <auto A, auto B>
constexpr void expand_char_range(ReachabilityTable& table, size_t state) {
    table.set_reachable_range(static_cast<char>(A), static_cast<char>(B), state);
}

template <typename... Content>
constexpr void expand_set(ReachabilityTable& table, size_t state) {
    (expand_char_class_element<Content>(table, state), ...);
}

template <typename T>
constexpr void expand_char_class_element(ReachabilityTable& table, size_t state);

template <typename T>
constexpr void expand_char_class_element(ReachabilityTable& table, size_t state) {
    if constexpr (requires { T::template match_char<char>; }) {
        for (int c = 0; c < 256; ++c) {
            if (T::match_char(static_cast<char>(c), ctre::flags{}))
                table.set_reachable_mut(static_cast<char>(c), state);
        }
    }
}

template <typename T>
constexpr bool is_char_class_type() {
    if constexpr (requires { typename T::is_char_class; }) return true;
    else if constexpr (requires { T::template match_char<char>; }) return true;
    else return false;
}

template <typename T>
constexpr void expand_any_char_class(ReachabilityTable& table, size_t state) {
    for (int c = 0; c < 256; ++c) {
        if (T::match_char(static_cast<char>(c), ctre::flags{}))
            table.set_reachable_mut(static_cast<char>(c), state);
    }
}

} // namespace ctre::bitnfa

#endif // CTRE_BITNFA_CHARACTER_CLASSES_HPP
