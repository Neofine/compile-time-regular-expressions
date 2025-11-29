#ifndef CTRE_V2__CTRE__POPULATE_PREFILTER_DB__HPP
#define CTRE_V2__CTRE__POPULATE_PREFILTER_DB__HPP

// This header DOES the heavy analysis and populates the database
// Only include this where you want analysis to run (NOT in wrapper.hpp)

#include "prefilter_database.hpp"
#include "decomposition.hpp"

namespace ctre {
namespace prefilter {

// Template function to analyze pattern and populate database
template <typename Pattern>
constexpr void populate_database_for_pattern() {
    constexpr bool has_lit = decomposition::has_prefilter_literal<Pattern>;
    if constexpr (has_lit) {
        constexpr auto lit = decomposition::prefilter_literal<Pattern>;
        // Populate happens via template specialization below
    }
}

// Auto-populate for any pattern that gets queried
template <typename Pattern>
constexpr literal_data get_literal_with_analysis() {
    constexpr bool has_lit = decomposition::has_prefilter_literal<Pattern>;
    if constexpr (has_lit) {
        constexpr auto lit = decomposition::prefilter_literal<Pattern>;
        return {true, lit.length, lit.chars};
    } else {
        return {false, 0, nullptr};
    }
}

} // namespace prefilter
} // namespace ctre

// Redefine get_literal to do analysis when needed
namespace ctre {
namespace prefilter {
    template <typename Pattern>
    constexpr literal_data get_literal() {
        return get_literal_with_analysis<Pattern>();
    }
}
}

#endif
