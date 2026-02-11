#ifndef CTRE__SIMD_PATTERN_ANALYSIS__HPP
#define CTRE__SIMD_PATTERN_ANALYSIS__HPP

#include "../../ctll/list.hpp"

namespace ctre::simd {

template <typename T, typename...>
struct first_type { using type = T; };

template <typename... Tail>
constexpr bool has_literal_next() {
    if constexpr (sizeof...(Tail) == 0) {
        return false;
    } else {
        using NextElement = typename first_type<Tail...>::type;
        if constexpr (requires { NextElement::value; }) return true;
        else if constexpr (requires { typename NextElement::head_type; } ||
                          requires { typename NextElement::tail_type; }) return true;
        return false;
    }
}

template <typename Content, typename... Tail>
struct pattern_suitability {
    static constexpr bool is_sequence = has_literal_next<Tail...>();
    static constexpr bool is_simd_suitable = !is_sequence;
};

} // namespace ctre::simd

#endif
