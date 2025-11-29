#ifndef CTRE_V2__CTRE__REGISTER_PREFILTERS__HPP
#define CTRE_V2__CTRE__REGISTER_PREFILTERS__HPP

// This header DOES pull in graph analysis - but it's ONLY included in ONE place
// to register all the prefilter information

#include "prefilter_traits.hpp"
#include "decomposition.hpp"

namespace ctre {
namespace prefilter {

// Macro to register a prefilter for a pattern
// This populates the lightweight traits that wrapper.hpp can access
#define REGISTER_PREFILTER(Pattern) \
    template <> \
    struct literal_info<Pattern> { \
        static constexpr bool has_literal = decomposition::has_prefilter_literal<Pattern>; \
        static constexpr auto _literal = []() { \
            if constexpr (has_literal) { \
                return decomposition::prefilter_literal<Pattern>; \
            } else { \
                return decomposition::literal_t<0>{}; \
            } \
        }(); \
        static constexpr const char* chars = _literal.chars; \
        static constexpr size_t length = has_literal ? _literal.length : 0; \
    };

// Helper to auto-register prefilter for any pattern
template <typename Pattern>
struct auto_register {
    static constexpr bool has_literal = decomposition::has_prefilter_literal<Pattern>;

    static constexpr auto get_chars() {
        if constexpr (has_literal) {
            return decomposition::prefilter_literal<Pattern>.chars;
        } else {
            return nullptr;
        }
    }

    static constexpr size_t get_length() {
        if constexpr (has_literal) {
            return decomposition::prefilter_literal<Pattern>.length;
        } else {
            return 0;
        }
    }
};

} // namespace prefilter
} // namespace ctre

#endif
