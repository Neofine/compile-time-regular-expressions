#ifndef CTRE_BITNFA_SHIFT_MASKS_HPP
#define CTRE_BITNFA_SHIFT_MASKS_HPP

#include "state_mask.hpp"
#include <array>

// Shift masks for transitions with span ≤ 7 (Hyperscan Algorithm 2)

namespace ctre::bitnfa {

template <size_t ShiftLimit = 7>
struct ShiftMasks {
    static constexpr size_t SHIFT_LIMIT = ShiftLimit;
    static_assert(ShiftLimit <= 16, "Shift limit too large (max 16)");

    std::array<StateMask128, ShiftLimit + 1> masks;

    constexpr ShiftMasks() : masks{} {}

    constexpr void set_transition(size_t from_state, size_t to_state) {
        if (to_state <= from_state) return;
        size_t span = to_state - from_state;
        if (span <= ShiftLimit)
            masks[span] = masks[span].set(from_state);
    }

    [[nodiscard]] bool has_transition(size_t from_state, size_t span) const {
        return span <= ShiftLimit && masks[span].test(from_state);
    }

    [[nodiscard]] const StateMask128& operator[](size_t k) const { return masks[k]; }
    StateMask128& operator[](size_t k) { return masks[k]; }

    [[gnu::always_inline]] StateMask128 calculate_successors(const StateMask128& current_states) const {
        StateMask128 succ0 = current_states & masks[0];
        StateMask128 succ1 = (current_states & masks[1]) << 1;
        StateMask128 succ2 = (current_states & masks[2]) << 2;
        StateMask128 succ3 = (current_states & masks[3]) << 3;
        StateMask128 succ4 = (current_states & masks[4]) << 4;
        StateMask128 succ5 = (current_states & masks[5]) << 5;
        StateMask128 succ6 = (current_states & masks[6]) << 6;
        StateMask128 succ7 = (current_states & masks[7]) << 7;

        return (succ0 | succ1 | succ2 | succ3) | (succ4 | succ5 | succ6 | succ7);
    }

    [[nodiscard]] size_t count_span(size_t k) const { return k <= ShiftLimit ? masks[k].count() : 0; }
    [[nodiscard]] bool has_span(size_t k) const { return k <= ShiftLimit && masks[k].any(); }
};

using ShiftMasks7 = ShiftMasks<7>;

} // namespace ctre::bitnfa

#endif // CTRE_BITNFA_SHIFT_MASKS_HPP
