#ifndef CTRE_V2__CTRE__PREFILTER_DATABASE_V2__HPP
#define CTRE_V2__CTRE__PREFILTER_DATABASE_V2__HPP

// Prefilter Database - constexpr data only, NO analysis machinery

namespace ctre {
namespace prefilter {

// Literal database entry
template <typename Pattern>
struct db_entry {
    static constexpr bool analyzed = false;
    static constexpr bool has_literal = false;
    static constexpr size_t length = 0;
};

// Runtime scan helper
inline bool contains_literal(const char* begin, const char* end, const char* literal, size_t len) {
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
    return false;
}

} // namespace prefilter
} // namespace ctre

#endif
