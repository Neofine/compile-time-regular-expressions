#ifndef CTRE_BITNFA_TYPES_HPP
#define CTRE_BITNFA_TYPES_HPP

#include "state_mask.hpp"
#include "shift_masks.hpp"
#include "reachability.hpp"

// Phase 1d: Complete BitNFA Structure
// Combine all data structures into a complete bit-based NFA
// This represents everything needed for runtime matching

namespace ctre::bitnfa {

// Complete bit-based NFA structure
// Contains all data needed for Algorithm 2 from Hyperscan paper
template <size_t MaxStates = 128>
struct BitNFA {
    static constexpr size_t MAX_STATES = MaxStates;
    static_assert(MaxStates <= 128, "Current implementation supports up to 128 states");

    // Number of states in the NFA
    size_t state_count = 0;

    // Shift masks for typical transitions (forward, span ≤ 7)
    ShiftMasks7 shift_masks;

    // Reachability table (which states can be entered with each character)
    ReachabilityTable reachability;

    // Accept states (states where we report a match)
    StateMask128 accept_mask;

    // Exception mask (states with atypical transitions - backward or span > 7)
    StateMask128 exception_mask;

    // Exception successor masks (for each exception state, which states it can reach)
    // exception_successors[i] = states reachable from state i via exception transitions
    std::array<StateMask128, MaxStates> exception_successors;

    // Default constructor (constexpr-friendly)
    constexpr BitNFA()
        : state_count(0)
        , shift_masks()
        , reachability()
        , accept_mask()
        , exception_mask()
        , exception_successors{}
    {}

    // Mark state as accept state (mutable)
    constexpr void set_accept(size_t state) {
        accept_mask = accept_mask.set(state);
    }

    // Mark state as accept state (constexpr, returns new BitNFA)
    constexpr BitNFA with_accept(size_t state) const {
        BitNFA result = *this;
        result.accept_mask = accept_mask.set(state);
        return result;
    }

    // Check if state is an accept state
    inline bool is_accept(size_t state) const {
        return accept_mask.test(state);
    }

    // Check if any active states are accept states
    inline bool has_accept(const StateMask128& active_states) const {
        return (active_states & accept_mask).any();
    }

    // Mark state as having exception transitions (mutable)
    constexpr void set_exception(size_t state) {
        exception_mask = exception_mask.set(state);
    }

    // Mark state as having exception transitions (constexpr)
    constexpr BitNFA with_exception(size_t state) const {
        BitNFA result = *this;
        result.exception_mask = exception_mask.set(state);
        return result;
    }

    // Check if state has exception transitions
    inline bool is_exception(size_t state) const {
        return exception_mask.test(state);
    }

    // Add exception successor (state 'from' can reach state 'to' via exception) - mutable
    constexpr void add_exception_successor(size_t from, size_t to) {
        exception_successors[from] = exception_successors[from].set(to);
    }

    // Add exception successor (constexpr)
    constexpr BitNFA with_exception_successor(size_t from, size_t to) const {
        BitNFA result = *this;
        result.exception_successors[from] = exception_successors[from].set(to);
        return result;
    }

    // Calculate successor states (core Algorithm 2)
    // OPTIMIZED: Reduced branching, fast path for no exceptions
    // This combines shift masks, exception handling, and reachability filtering
    __attribute__((always_inline, hot)) inline StateMask128 calculate_successors(const StateMask128& current_states, char c) const {
        // Step 1: Calculate typical successors using shift masks
        StateMask128 typical_succ = shift_masks.calculate_successors(current_states);

        // Step 2: Calculate exception successors (optimized)
        // OPTIMIZATION: Skip exception handling if no exceptions exist (compile-time check possible)
        StateMask128 all_succ = typical_succ;

        if (__builtin_expect((current_states & exception_mask).any(), 0)) {
            // We have some exception states active (rare path)
            StateMask128 exception_states = current_states & exception_mask;

            // OPTIMIZATION: Use __builtin_ctz to find set bits instead of testing all states
            uint64_t low = exception_states.get_low();
            uint64_t high = exception_states.get_high();

            StateMask128 exception_succ;

            // Process low 64 bits
            while (low) {
                int state = __builtin_ctzll(low);
                exception_succ = exception_succ | exception_successors[state];
                low &= (low - 1); // Clear lowest set bit
            }

            // Process high 64 bits
            while (high) {
                int state = 64 + __builtin_ctzll(high);
                exception_succ = exception_succ | exception_successors[state];
                high &= (high - 1); // Clear lowest set bit
            }

            all_succ = typical_succ | exception_succ;
        }

        // Step 3: Filter by reachability with character c
        StateMask128 filtered = reachability.filter_by_char(all_succ, c);

        return filtered;
    }

    // Get initial state (state 0)
    inline StateMask128 get_initial_state() const {
        StateMask128 initial;
        return initial.set(0);
    }

    // Check if NFA has any exception states
    inline bool has_exceptions() const {
        return exception_mask.any();
    }

    // Get count of exception states
    inline size_t count_exceptions() const {
        return exception_mask.count();
    }
};

// Type alias for standard 128-state bit NFA
using BitNFA128 = BitNFA<128>;

} // namespace ctre::bitnfa

#endif // CTRE_BITNFA_TYPES_HPP
