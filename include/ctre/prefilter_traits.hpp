#ifndef CTRE_V2__CTRE__PREFILTER_TRAITS__HPP
#define CTRE_V2__CTRE__PREFILTER_TRAITS__HPP

#include "decomposition.hpp"

namespace ctre {
namespace prefilter {

// Check if literal exists using SIMD
template <char... Chars>
inline bool contains_literal_simd(const char* begin, const char* end) noexcept {
    if (sizeof...(Chars) == 0) return true;
    
    constexpr char literal[] = {Chars...};
    constexpr size_t len = sizeof...(Chars);
    
    // Simple scan (compiler will vectorize this)
    for (const char* it = begin; it + len <= end; ++it) {
        bool match = true;
        for (size_t i = 0; i < len; ++i) {
            if (it[i] != literal[i]) {
                match = false;
                break;
            }
        }
        if (match) return true;
    }
    return false;
}

// Helper to get literal info at compile-time
template <typename Pattern>
struct literal_info {
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
