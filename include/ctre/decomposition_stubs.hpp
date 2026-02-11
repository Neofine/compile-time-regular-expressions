#ifndef CTRE__DECOMPOSITION_STUBS__HPP
#define CTRE__DECOMPOSITION_STUBS__HPP

#include <cstddef>

namespace ctre::decomposition {

template <typename RE> inline constexpr bool has_prefilter_literal = false;

template <typename RE>
struct prefilter_literal_type {
    static constexpr std::size_t length = 0;
};

template <typename RE> inline constexpr prefilter_literal_type<RE> prefilter_literal{};

} // namespace ctre::decomposition

#endif
