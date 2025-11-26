#ifndef CTRE_BITNFA_REACHABILITY_HPP
#define CTRE_BITNFA_REACHABILITY_HPP

#include "state_mask.hpp"
#include <array>

// Phase 1c: Reachability Table
// Store which states can be entered with each character
// From Hyperscan paper: "reachable states by 'c'" - states that can be transitioned to with character c

namespace ctre::bitnfa {

// Reachability table: 256 state masks (one per ASCII character)
struct ReachabilityTable {
    // Array of masks, one per character (0-255)
    // reachable[c] has bit i set if state i can be entered with character c
    std::array<StateMask128, 256> reachable;

    // Constexpr constructor - all masks zero (for compile-time construction)
    constexpr ReachabilityTable() : reachable{} {}

    // Mutable version for compile-time construction
    constexpr void set_reachable_mut(char c, size_t state) {
        reachable[static_cast<unsigned char>(c)] = reachable[static_cast<unsigned char>(c)].set(state);
    }

    // Convenience: same as set_reachable_mut (for compatibility)
    constexpr void set_reachable(char c, size_t state) {
        set_reachable_mut(c, state);
    }

    // Check if state is reachable with character c
    inline bool is_reachable(char c, size_t state) const {
        return reachable[static_cast<unsigned char>(c)].test(state);
    }

    // Get mask of all states reachable with character c
    inline const StateMask128& operator[](char c) const {
        return reachable[static_cast<unsigned char>(c)];
    }

    inline StateMask128& operator[](char c) {
        return reachable[static_cast<unsigned char>(c)];
    }

    // Get mask by unsigned char (for iteration)
    inline const StateMask128& get(unsigned char c) const {
        return reachable[c];
    }

    inline StateMask128& get(unsigned char c) {
        return reachable[c];
    }

    // Mark state as reachable with any character in a range [from, to]
    inline void set_reachable_range(char from, char to, size_t state) {
        unsigned char start = static_cast<unsigned char>(from);
        unsigned char end = static_cast<unsigned char>(to);
        for (unsigned char c = start; c <= end; ++c) {
            reachable[c] = reachable[c].set(state);
            if (c == end) break; // Avoid wrap-around for c=255
        }
    }

    // Mark state as reachable with any character (dot '.')
    constexpr void set_reachable_any(size_t state) {
        for (size_t c = 0; c < 256; ++c) {
            reachable[c] = reachable[c].set(state);
        }
    }

    // Count how many states are reachable with character c
    inline size_t count_reachable(char c) const {
        return reachable[static_cast<unsigned char>(c)].count();
    }

    // Check if any states are reachable with character c
    inline bool has_reachable(char c) const {
        return reachable[static_cast<unsigned char>(c)].any();
    }

    // Filter successor states by reachability with character c
    // This is the core operation from Algorithm 2: current_states = succ & reachable[c]
    inline StateMask128 filter_by_char(const StateMask128& successors, char c) const {
        return successors & reachable[static_cast<unsigned char>(c)];
    }
};

} // namespace ctre::bitnfa

#endif // CTRE_BITNFA_REACHABILITY_HPP
