#ifndef CTRE_BITNFA_REACHABILITY_HPP
#define CTRE_BITNFA_REACHABILITY_HPP

#include "state_mask.hpp"
#include <array>

// Reachability table: 256 state masks (one per ASCII character)
// From Hyperscan paper: "reachable states by 'c'"

namespace ctre::bitnfa {

struct ReachabilityTable {
    std::array<StateMask128, 256> reachable;

    constexpr ReachabilityTable() : reachable{} {}

    constexpr void set_reachable_mut(char c, size_t state) {
        reachable[static_cast<unsigned char>(c)] = reachable[static_cast<unsigned char>(c)].set(state);
    }

    constexpr void set_reachable(char c, size_t state) { set_reachable_mut(c, state); }

    [[nodiscard]] bool is_reachable(char c, size_t state) const {
        return reachable[static_cast<unsigned char>(c)].test(state);
    }

    [[nodiscard]] const StateMask128& operator[](char c) const {
        return reachable[static_cast<unsigned char>(c)];
    }
    StateMask128& operator[](char c) { return reachable[static_cast<unsigned char>(c)]; }

    [[nodiscard]] const StateMask128& get(unsigned char c) const { return reachable[c]; }
    StateMask128& get(unsigned char c) { return reachable[c]; }

    void set_reachable_range(char from, char to, size_t state) {
        unsigned char start = static_cast<unsigned char>(from);
        unsigned char end = static_cast<unsigned char>(to);
        for (unsigned char c = start; c <= end; ++c) {
            reachable[c] = reachable[c].set(state);
            if (c == end) break;
        }
    }

    constexpr void set_reachable_any(size_t state) {
        for (size_t c = 0; c < 256; ++c)
            reachable[c] = reachable[c].set(state);
    }

    [[nodiscard]] size_t count_reachable(char c) const {
        return reachable[static_cast<unsigned char>(c)].count();
    }

    [[nodiscard]] bool has_reachable(char c) const {
        return reachable[static_cast<unsigned char>(c)].any();
    }

    [[nodiscard]] StateMask128 filter_by_char(const StateMask128& successors, char c) const {
        return successors & reachable[static_cast<unsigned char>(c)];
    }
};

} // namespace ctre::bitnfa

#endif // CTRE_BITNFA_REACHABILITY_HPP
