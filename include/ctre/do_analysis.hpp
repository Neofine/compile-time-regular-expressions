#ifndef CTRE_V2__CTRE__DO_ANALYSIS__HPP
#define CTRE_V2__CTRE__DO_ANALYSIS__HPP

#include "prefilter_database_v2.hpp"
#include "decomposition.hpp"

namespace ctre {
namespace prefilter {

// Analyze and store results
template <typename Pattern>
struct analyzed_entry {
    static constexpr bool has_lit = decomposition::has_prefilter_literal<Pattern>;
    
    static constexpr auto get_chars() {
        if constexpr (has_lit) {
            constexpr auto lit = decomposition::prefilter_literal<Pattern>;
            return lit.chars;
        } else {
            static constexpr char empty[1] = {0};
            return empty;
        }
    }
    
    static constexpr size_t get_length() {
        if constexpr (has_lit) {
            constexpr auto lit = decomposition::prefilter_literal<Pattern>;
            return lit.length;
        } else {
            return 0;
        }
    }
    
    static constexpr bool analyzed = true;
    static constexpr bool has_literal = has_lit;
    static constexpr size_t length = get_length();
    static constexpr auto chars = get_chars();
};

// Helper
template <typename Pattern>
constexpr auto get_analyzed() {
    return analyzed_entry<Pattern>{};
}

} // namespace prefilter
} // namespace ctre

#endif
