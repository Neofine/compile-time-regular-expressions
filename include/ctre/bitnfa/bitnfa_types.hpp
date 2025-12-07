#ifndef CTRE_BITNFA_TYPES_HPP
#define CTRE_BITNFA_TYPES_HPP

#include "state_mask.hpp"
#include "shift_masks.hpp"
#include "reachability.hpp"

namespace ctre::bitnfa {

// Bit-based NFA structure for Algorithm 2 (Hyperscan paper)
template <size_t MaxStates = 128>
struct BitNFA {
    static constexpr size_t MAX_STATES = MaxStates;
    static_assert(MaxStates <= 128, "Current implementation supports up to 128 states");

    size_t state_count = 0;
    ShiftMasks7 shift_masks;
    ReachabilityTable reachability;
    StateMask128 accept_mask;
    StateMask128 exception_mask;
    std::array<StateMask128, MaxStates> exception_successors;

    constexpr BitNFA()
        : state_count(0)
        , shift_masks()
        , reachability()
        , accept_mask()
        , exception_mask()
        , exception_successors{}
    {}

    constexpr void set_accept(size_t state) { accept_mask = accept_mask.set(state); }
    constexpr BitNFA with_accept(size_t state) const {
        BitNFA result = *this;
        result.accept_mask = accept_mask.set(state);
        return result;
    }
    [[nodiscard]] bool is_accept(size_t state) const { return accept_mask.test(state); }
    [[nodiscard]] bool has_accept(const StateMask128& active_states) const {
        return (active_states & accept_mask).any();
    }

    constexpr void set_exception(size_t state) { exception_mask = exception_mask.set(state); }
    constexpr BitNFA with_exception(size_t state) const {
        BitNFA result = *this;
        result.exception_mask = exception_mask.set(state);
        return result;
    }
    [[nodiscard]] bool is_exception(size_t state) const { return exception_mask.test(state); }

    constexpr void add_exception_successor(size_t from, size_t to) {
        exception_successors[from] = exception_successors[from].set(to);
    }
    constexpr BitNFA with_exception_successor(size_t from, size_t to) const {
        BitNFA result = *this;
        result.exception_successors[from] = exception_successors[from].set(to);
        return result;
    }

    [[gnu::always_inline, gnu::hot]] StateMask128 calculate_successors(const StateMask128& current_states, char c) const {
        StateMask128 typical_succ = shift_masks.calculate_successors(current_states);
        StateMask128 all_succ = typical_succ;

        if (__builtin_expect((current_states & exception_mask).any(), 0)) {
            StateMask128 exception_states = current_states & exception_mask;
            uint64_t low = exception_states.get_low();
            uint64_t high = exception_states.get_high();
            StateMask128 exception_succ;

            while (low) {
                exception_succ = exception_succ | exception_successors[static_cast<size_t>(__builtin_ctzll(low))];
                low &= (low - 1);
            }
            while (high) {
                exception_succ = exception_succ | exception_successors[64 + static_cast<size_t>(__builtin_ctzll(high))];
                high &= (high - 1);
            }
            all_succ = typical_succ | exception_succ;
        }

        return reachability.filter_by_char(all_succ, c);
    }

    [[nodiscard]] StateMask128 get_initial_state() const {
        StateMask128 initial;
        return initial.set(0);
    }
    [[nodiscard]] bool has_exceptions() const { return exception_mask.any(); }
    [[nodiscard]] size_t count_exceptions() const { return exception_mask.count(); }
};

using BitNFA128 = BitNFA<128>;

} // namespace ctre::bitnfa

#endif // CTRE_BITNFA_TYPES_HPP
