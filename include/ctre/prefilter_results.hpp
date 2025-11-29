#ifndef CTRE_V2__CTRE__PREFILTER_RESULTS__HPP
#define CTRE_V2__CTRE__PREFILTER_RESULTS__HPP

// LIGHTWEIGHT: Just the results of analysis, no analysis machinery
// This is like Hyperscan's "database" - precomputed results

namespace ctre {
namespace prefilter {

// Storage for precomputed literal (like Hyperscan's scratch space)
template <typename Pattern>
struct precomputed_literal {
    static constexpr bool computed = false;
    static constexpr bool has_literal = false;
    static constexpr size_t length = 0;
    static constexpr char chars[64] = {};
};

// Macro to register precomputed results (called from decomposition analysis)
#define CTRE_REGISTER_LITERAL(Pattern, HasLit, Len, ...) \
    template <> \
    struct precomputed_literal<Pattern> { \
        static constexpr bool computed = true; \
        static constexpr bool has_literal = HasLit; \
        static constexpr size_t length = Len; \
        static constexpr char chars[64] = {__VA_ARGS__}; \
    };

} // namespace prefilter
} // namespace ctre

#endif
