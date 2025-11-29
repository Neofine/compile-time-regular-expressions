#ifndef CTRE_V2__CTRE__DO_ANALYSIS_SIMPLE__HPP
#define CTRE_V2__CTRE__DO_ANALYSIS_SIMPLE__HPP

#include "decomposition.hpp"

namespace ctre {
namespace prefilter {

// Simple: just query decomposition and return the data
template <typename Pattern>
constexpr bool pattern_has_literal() {
    return decomposition::has_prefilter_literal<Pattern>;
}

template <typename Pattern>
constexpr size_t pattern_literal_length() {
    if constexpr (decomposition::has_prefilter_literal<Pattern>) {
        return decomposition::prefilter_literal<Pattern>.length;
    }
    return 0;
}

template <typename Pattern, size_t... Is>
constexpr bool scan_for_pattern_literal(const char* begin, const char* end, std::index_sequence<Is...>) {
    if constexpr (decomposition::has_prefilter_literal<Pattern>) {
        constexpr auto lit = decomposition::prefilter_literal<Pattern>;
        constexpr char literal[] = {lit.chars[Is]..., '\0'};
        constexpr size_t len = sizeof...(Is);
        
        if (len == 0) return true;
        
        const char first = literal[0];
        for (const char* it = begin; it + len <= end; ++it) {
            if (*it == first) {
                bool match = true;
                for (size_t i = 1; i < len; ++i) {
                    if (it[i] != literal[i]) {
                        match = false;
                        break;
                    }
                }
                if (match) return true;
            }
        }
    }
    return false;
}

} // namespace prefilter
} // namespace ctre

#endif
