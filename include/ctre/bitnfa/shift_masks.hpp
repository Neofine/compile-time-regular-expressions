#ifndef CTRE_BITNFA_SHIFT_MASKS_HPP
#define CTRE_BITNFA_SHIFT_MASKS_HPP

#include "state_mask.hpp"
#include <array>

// Phase 1b: Shift Masks
// Store masks for "typical" transitions - forward transitions with span ≤ 7
// From Hyperscan paper: "shift-k mask records all states with a forward transition of span k"
//
// Example: If state 5 has a transition to state 8 (span = 8-5 = 3):
//   - shift_masks[3] will have bit 5 set
//   - During matching: (current_states & shift_masks[3]) << 3 gives successor states

namespace ctre::bitnfa {

// Shift masks for typical transitions (span 0 through 7)
// Shift limit of 7 is from Hyperscan paper - balances performance vs coverage
template <size_t ShiftLimit = 7>
struct ShiftMasks {
    static constexpr size_t SHIFT_LIMIT = ShiftLimit;
    static_assert(ShiftLimit <= 16, "Shift limit too large (max 16)");

    // Array of masks, one per shift amount
    // masks[k] has bit i set if state i has a forward transition of span k
    std::array<StateMask128, ShiftLimit + 1> masks;

    // Constexpr constructor - all masks zero (for compile-time construction)
    constexpr ShiftMasks() : masks{} {}

    // Set bit in specific shift mask
    // Example: set_transition(5, 8) means state 5 → state 8, so set bit 5 in masks[3]
    constexpr void set_transition(size_t from_state, size_t to_state) {
        if (to_state <= from_state) {
            // Not a forward transition - will be handled as exception
            return;
        }

        size_t span = to_state - from_state;
        if (span <= ShiftLimit) {
            masks[span] = masks[span].set(from_state);
        }
        // If span > ShiftLimit, it's an exception (handled elsewhere)
    }

    // Check if state has a transition of specific span
    inline bool has_transition(size_t from_state, size_t span) const {
        if (span > ShiftLimit) return false;
        return masks[span].test(from_state);
    }

    // Get mask for specific shift amount
    inline const StateMask128& operator[](size_t k) const {
        return masks[k];
    }

    inline StateMask128& operator[](size_t k) {
        return masks[k];
    }

    // Calculate successor states from current states using all shift masks
    // This is the core operation from the Hyperscan paper (Algorithm 2)
    // Manually unrolled for maximum performance
    __attribute__((always_inline)) inline StateMask128 calculate_successors(const StateMask128& current_states) const {
        // Unroll loop for performance
        StateMask128 succ0 = current_states & masks[0];
        StateMask128 succ1 = (current_states & masks[1]) << 1;
        StateMask128 succ2 = (current_states & masks[2]) << 2;
        StateMask128 succ3 = (current_states & masks[3]) << 3;
        StateMask128 succ4 = (current_states & masks[4]) << 4;
        StateMask128 succ5 = (current_states & masks[5]) << 5;
        StateMask128 succ6 = (current_states & masks[6]) << 6;
        StateMask128 succ7 = (current_states & masks[7]) << 7;

        // Tree-reduce for better ILP (instruction-level parallelism)
        StateMask128 temp0 = succ0 | succ1;
        StateMask128 temp1 = succ2 | succ3;
        StateMask128 temp2 = succ4 | succ5;
        StateMask128 temp3 = succ6 | succ7;

        StateMask128 temp01 = temp0 | temp1;
        StateMask128 temp23 = temp2 | temp3;

        return temp01 | temp23;
    }

    // Count how many states have transitions of span k
    inline size_t count_span(size_t k) const {
        if (k > ShiftLimit) return 0;
        return masks[k].count();
    }

    // Check if any transitions exist for span k
    inline bool has_span(size_t k) const {
        if (k > ShiftLimit) return false;
        return masks[k].any();
    }
};

// Type alias for standard shift limit (7, from Hyperscan)
using ShiftMasks7 = ShiftMasks<7>;

} // namespace ctre::bitnfa

#endif // CTRE_BITNFA_SHIFT_MASKS_HPP
