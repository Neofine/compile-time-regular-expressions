#ifndef CTRE__DOMINATOR_ANALYSIS__HPP
#define CTRE__DOMINATOR_ANALYSIS__HPP

// Dominator Analysis for Glushkov NFA
// Finds vertices that all paths from start to accept must traverse
// Used for extracting required literals for prefiltering

#include "glushkov_nfa.hpp"
#include "multi_literal.hpp"
#include <array>
#include <cstddef>

namespace ctre {
namespace dominators {

// Graph Reachability

// Simple reachability check: can we reach 'to' from 'from'?
template <typename NFA>
constexpr bool is_reachable(const NFA& nfa, size_t from, size_t to,
                            std::array<bool, 512>& visited) {
    if (from == to) return true;
    if (visited[from]) return false;

    visited[from] = true;

    // Try all successors
    for (size_t i = 0; i < nfa.states[from].successor_count; ++i) {
        size_t next = nfa.states[from].successors[i];
        if (is_reachable(nfa, next, to, visited)) {
            return true;
        }
    }

    return false;
}

// Wrapper for reachability (creates fresh visited array)
template <typename NFA>
constexpr bool is_reachable(const NFA& nfa, size_t from, size_t to) {
    std::array<bool, 512> visited{};
    return is_reachable(nfa, from, to, visited);
}

// Can we reach ANY accept state from 'from'?
template <typename NFA>
constexpr bool can_reach_accept(const NFA& nfa, size_t from) {
    for (size_t i = 0; i < nfa.accept_count; ++i) {
        if (is_reachable(nfa, from, nfa.accept_states[i])) {
            return true;
        }
    }
    return false;
}

// Dominator Detection

// Check if removing vertex 'v' disconnects start from accept states
template <typename NFA>
constexpr bool is_dominator(const NFA& nfa, size_t v) {
    // Can't remove start state
    if (v == nfa.start_state) return false;

    // Special case: if v itself is the ONLY accept state, it's a dominator
    // But we need to check if we can reach OTHER accept states without it
    bool v_is_accept = false;
    size_t other_accept_count = 0;
    for (size_t i = 0; i < nfa.accept_count; ++i) {
        if (v == nfa.accept_states[i]) {
            v_is_accept = true;
        } else {
            other_accept_count++;
        }
    }

    // If v is the only accept state, it's definitely a dominator
    if (v_is_accept && other_accept_count == 0) {
        return true;
    }

    // Try to reach ANY accept from start WITHOUT going through v
    std::array<bool, 512> visited{};
    visited[v] = true;  // Mark v as "visited" so we skip it

    // BFS from start, avoiding v
    std::array<size_t, 512> queue{};
    size_t queue_front = 0;
    size_t queue_back = 0;

    queue[queue_back++] = nfa.start_state;
    visited[nfa.start_state] = true;

    while (queue_front < queue_back) {
        size_t current = queue[queue_front++];

        // Check if current is an accept state (and not v itself)
        if (current != v) {
            for (size_t i = 0; i < nfa.accept_count; ++i) {
                if (current == nfa.accept_states[i]) {
                    return false;  // Found path to accept without v, so v is NOT a dominator
                }
            }
        }

        // Add successors to queue
        for (size_t i = 0; i < nfa.states[current].successor_count; ++i) {
            size_t next = nfa.states[current].successors[i];
            if (!visited[next]) {
                visited[next] = true;
                queue[queue_back++] = next;
            }
        }
    }

    // Couldn't reach any accept without v, so v IS a dominator!
    return true;
}

// Result structure for dominator set
template <size_t MaxDominators = 64>
struct dominator_set {
    std::array<size_t, MaxDominators> dominators{};
    size_t count = 0;

    constexpr void add(size_t dominator) {
        if (count < MaxDominators) {
            dominators[count++] = dominator;
        }
    }

    constexpr bool contains(size_t value) const {
        for (size_t i = 0; i < count; ++i) {
            if (dominators[i] == value) return true;
        }
        return false;
    }
};

// Find all dominators in the NFA
template <typename NFA>
constexpr auto find_dominators(const NFA& nfa) {
    dominator_set<64> result{};

    // Check each state (except start)
    for (size_t v = 1; v < nfa.state_count; ++v) {
        if (is_dominator(nfa, v)) {
            result.add(v);
        }
    }

    return result;
}

// Literal Extraction - uses ctre::literal_result from multi_literal.hpp
template <size_t MaxLength = 64>
using literal_result = ctre::literal_result<MaxLength>;

// Extract consecutive dominator positions as a literal string
template <typename NFA>
constexpr auto extract_literal_from_dominators(const NFA& nfa) {
    literal_result<64> result{};

    auto doms = find_dominators(nfa);

    if (doms.count == 0) {
        return result;
    }

    // Find longest consecutive sequence of concrete character dominators
    literal_result<64> best{};
    literal_result<64> current{};

    for (size_t i = 0; i < doms.count; ++i) {
        size_t pos = doms.dominators[i];
        char sym = nfa.states[pos].symbol;

        // Check if this state has a self-loop (variable repeat like +, *)
        bool has_self_loop = false;
        for (size_t j = 0; j < nfa.states[pos].successor_count; ++j) {
            if (nfa.states[pos].successors[j] == pos) {
                has_self_loop = true;
                break;
            }
        }

        // Check if it's a concrete character (not '.', '?', etc.) and NOT a variable repeat
        if (sym != '\0' && sym != '.' && sym != '?' && !has_self_loop) {
            // Check if this position is consecutive with current sequence
            bool is_consecutive = (current.length == 0) ||
                                   (pos == current.start_position + current.length);

            if (is_consecutive) {
                // Start or extend current sequence
                if (current.length == 0) {
                    current.start_position = pos;
                }
                current.add_char(sym);
            } else {
                // Gap in positions - break sequence
                if (current.length > best.length) {
                    best = current;
                }
                // Start new sequence
                current = literal_result<64>{};
                current.start_position = pos;
                current.add_char(sym);
            }
        } else {
            // Non-concrete character or variable repeat - break in sequence
            if (current.length > best.length) {
                best = current;
            }
            current = literal_result<64>{};
        }
    }

    // Final check
    if (current.length > best.length) {
        best = current;
    }

    return best;
}

// Convenience function: check if pattern has extractable literal (path only)
template <typename Pattern>
constexpr bool has_extractable_literal() {
    constexpr auto nfa = glushkov::glushkov_nfa<Pattern>();
    constexpr auto path_literal = extract_literal_from_dominators(nfa);
    return path_literal.has_literal;
    // Note: Region analysis fallback is in extract_literal() instead
}

// Convenience function: extract literal from pattern
template <typename Pattern>
constexpr auto extract_literal() {
    constexpr auto nfa = glushkov::glushkov_nfa<Pattern>();
    return extract_literal_from_dominators(nfa);
}

// With region fallback (defined in decomposition.hpp to avoid circular dependency)
template <typename Pattern>
inline auto extract_literal_with_fallback() {
    constexpr auto nfa = glushkov::glushkov_nfa<Pattern>();

    // Step 1: Try dominant path analysis
    constexpr auto path_result = extract_literal_from_dominators(nfa);
    if constexpr (path_result.has_literal) {
        return path_result;  // Path analysis succeeded!
    }

    // Step 2: Region fallback handled in decomposition.hpp (avoids circular dependency)
    return literal_result<64>{};
}

} // namespace dominators
} // namespace ctre

#endif // CTRE__DOMINATOR_ANALYSIS__HPP
