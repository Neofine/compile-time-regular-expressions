#ifndef CTRE__BITNFA_STUBS__HPP
#define CTRE__BITNFA_STUBS__HPP

#include <cstddef>
#include <string_view>

namespace ctre::bitnfa {

struct match_result {
    bool matched = false;
    std::size_t length = 0;
};

template <typename RE>
[[nodiscard]] match_result match_from_ast(std::string_view) noexcept {
    return {};
}

template <typename RE>
[[nodiscard]] match_result search_from_ast(std::string_view) noexcept {
    return {};
}

} // namespace ctre::bitnfa

#endif
