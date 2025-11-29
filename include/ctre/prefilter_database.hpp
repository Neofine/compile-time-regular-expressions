#ifndef CTRE_V2__CTRE__PREFILTER_DATABASE__HPP
#define CTRE_V2__CTRE__PREFILTER_DATABASE__HPP

// Prefilter "Database" - like Hyperscan's compiled pattern database
// Analysis machinery is NOT here, only results

namespace ctre {
namespace prefilter {

// Literal data (like Hyperscan's bytecode)
struct literal_data {
    bool has_literal;
    size_t length;
    const char* chars;
};

// Query the database (populated externally by analysis)
template <typename Pattern>
constexpr literal_data get_literal() {
    // This will be specialized by decomposition analysis
    return {false, 0, nullptr};
}

// Runtime helper: Check if literal exists (like hs_scan)
inline bool scan_for_literal(const char* begin, const char* end, const char* literal, size_t len) {
    if (len == 0) return true;
    
    // Simple memchr + memcmp approach (compiler will vectorize)
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
    return false;
}

} // namespace prefilter
} // namespace ctre

// Macro to populate database (called from analysis, NOT from wrapper)
#define CTRE_POPULATE_LITERAL_DB(Pattern, HasLit, Len, Chars) \
    namespace ctre { namespace prefilter { \
    template <> \
    constexpr literal_data get_literal<Pattern>() { \
        static constexpr char _chars[] = Chars; \
        return {HasLit, Len, _chars}; \
    } \
    }}

#endif
