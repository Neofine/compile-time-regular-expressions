#ifndef CTRE_BITNFA_INTEGRATION_HPP
#define CTRE_BITNFA_INTEGRATION_HPP

#include "bitnfa_match.hpp"

namespace ctre::bitnfa {

// Pattern Complexity Analysis

template <typename T>
constexpr size_t count_alternations() {
    if constexpr (glushkov::is_select<T>::value) {
        return 1;
    } else {
        return 0;
    }
}

template <typename Pattern>
struct pattern_analysis {
    static constexpr size_t alternation_count = count_alternations<Pattern>();

    static constexpr size_t state_count = []() {
        if constexpr (requires { glushkov::glushkov_nfa<Pattern>().state_count; }) {
            return glushkov::glushkov_nfa<Pattern>().state_count;
        } else {
            return 0;
        }
    }();

    static constexpr bool use_bitnfa =
        (state_count > 16) || (alternation_count > 3);
};

// Force BitNFA engine (for benchmarking)
template <ctll::fixed_string Pattern>
struct bitnfa_engine {
    static auto match(std::string_view input) {
        return bitnfa::match<Pattern>(input);
    }

    static auto search(std::string_view input) {
        return bitnfa::search<Pattern>(input);
    }

    static auto find_all(std::string_view input) {
        return bitnfa::find_all<Pattern>(input);
    }
};

} // namespace ctre::bitnfa

#endif // CTRE_BITNFA_INTEGRATION_HPP
