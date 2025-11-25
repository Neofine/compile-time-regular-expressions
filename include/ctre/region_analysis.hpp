#ifndef CTRE__REGION_ANALYSIS__HPP
#define CTRE__REGION_ANALYSIS__HPP

// Dominant Region Analysis
// Fallback when dominant path analysis fails (covers remaining 2-3% of patterns)
// Based on Hyperscan paper Section 3.3 and ng_region.cpp implementation

#include "glushkov_nfa.hpp"
#include "dominator_analysis.hpp"  // For literal_result
#include <array>
#include <cstddef>

namespace ctre {
namespace region {

// =============================================================================
// Constants
// =============================================================================

constexpr size_t MAX_STATES = 512;       // Max NFA states
constexpr size_t MAX_REGIONS = 64;       // Max regions in a graph
constexpr size_t MAX_REGION_SIZE = 128;  // Max vertices per region

// =============================================================================
// Phase 6.1: Acyclic Graph Construction
// =============================================================================

// Acyclic graph: Original NFA with back edges marked
// Back edge = edge that goes backwards in topological order
// We identify back edges by checking if target position <= source position
template <typename NFA>
struct acyclic_graph {
    const NFA& nfa;

    // Mark which edges are back edges (loops)
    // Index: [state_id * MAX_STATES + successor_index]
    std::array<bool, MAX_STATES * MAX_STATES> is_back_edge;

    constexpr acyclic_graph(const NFA& n) : nfa(n), is_back_edge{} {
        // Identify back edges
        // In Glushkov NFA, states are numbered by position in regex
        // A back edge is one that goes from a later position to an earlier position

        for (size_t i = 0; i < nfa.state_count; i++) {
            for (size_t j = 0; j < nfa.states[i].successor_count; j++) {
                size_t succ = nfa.states[i].successors[j];

                // Back edge if:
                // 1. Successor position <= current position (goes backwards)
                // 2. Not the final accept state (state_count - 1 is special)
                if (succ <= i && i < nfa.state_count - 1) {
                    is_back_edge[i * MAX_STATES + j] = true;
                }
            }
        }
    }

    // Check if an edge is a back edge
    constexpr bool is_back_edge_between(size_t from, size_t to) const {
        for (size_t j = 0; j < nfa.states[from].successor_count; j++) {
            if (nfa.states[from].successors[j] == to) {
                return is_back_edge[from * MAX_STATES + j];
            }
        }
        return false;
    }

    // Get successors excluding back edges (for DAG traversal)
    constexpr auto get_forward_successors(size_t state_id) const {
        struct result_t {
            std::array<size_t, MAX_STATES> successors;
            size_t count;
        };

        result_t result{};
        result.count = 0;

        for (size_t j = 0; j < nfa.states[state_id].successor_count; j++) {
            if (!is_back_edge[state_id * MAX_STATES + j]) {
                result.successors[result.count++] = nfa.states[state_id].successors[j];
            }
        }

        return result;
    }

    // Get predecessors (reverse edges)
    constexpr auto get_predecessors(size_t state_id) const {
        struct result_t {
            std::array<size_t, MAX_STATES> predecessors;
            size_t count;
        };

        result_t result{};
        result.count = 0;

        // Search all states for edges pointing to state_id
        for (size_t i = 0; i < nfa.state_count; i++) {
            for (size_t j = 0; j < nfa.states[i].successor_count; j++) {
                if (nfa.states[i].successors[j] == state_id) {
                    result.predecessors[result.count++] = i;
                    break;  // Don't double-count same predecessor
                }
            }
        }

        return result;
    }

    // Get predecessors excluding back edges (for DAG traversal)
    constexpr auto get_forward_predecessors(size_t state_id) const {
        struct result_t {
            std::array<size_t, MAX_STATES> predecessors;
            size_t count;
        };

        result_t result{};
        result.count = 0;

        // Search all states for forward edges pointing to state_id
        for (size_t i = 0; i < nfa.state_count; i++) {
            for (size_t j = 0; j < nfa.states[i].successor_count; j++) {
                if (!is_back_edge[i * MAX_STATES + j] &&
                    nfa.states[i].successors[j] == state_id) {
                    result.predecessors[result.count++] = i;
                    break;  // Don't double-count same predecessor
                }
            }
        }

        return result;
    }
};

// =============================================================================
// Phase 6.2: Topological Sort
// =============================================================================

// Topological sort result
struct topo_sort_result {
    std::array<size_t, MAX_STATES> order;  // States in reverse topological order
    size_t count;
};

// DFS-based topological sort on the acyclic graph
// Returns states in REVERSE order (accept states first, start state last)
template <typename NFA>
constexpr topo_sort_result topological_sort(const acyclic_graph<NFA>& dag) {
    topo_sort_result result{};
    result.count = 0;

    std::array<bool, MAX_STATES> visited{};

    // DFS helper (recursive via lambda)
    auto dfs = [&](auto& self, size_t v) -> void {
        if (visited[v]) return;
        visited[v] = true;

        // Visit all forward successors first
        auto succs = dag.get_forward_successors(v);
        for (size_t i = 0; i < succs.count; i++) {
            self(self, succs.successors[i]);
        }

        // Add this vertex AFTER visiting successors (post-order)
        result.order[result.count++] = v;
    };

    // Start DFS from accept states
    // Accept states are the ones that complete a match
    for (size_t i = 0; i < dag.nfa.accept_count; i++) {
        size_t accept_state = dag.nfa.accept_states[i];
        dfs(dfs, accept_state);
    }

    // Also visit any unreached states (e.g., start state if not reached yet)
    for (size_t i = 0; i < dag.nfa.state_count; i++) {
        if (!visited[i]) {
            dfs(dfs, i);
        }
    }

    return result;
}

// =============================================================================
// Phase 6.3: Region Finding
// =============================================================================

// Region: A set of vertices that form a valid cut in the graph
// Valid cut means all entry edges share same predecessors, all exit edges share same successors
struct region_info {
    std::array<size_t, MAX_REGION_SIZE> vertices;
    size_t vertex_count = 0;

    // Entry vertices: vertices in region with incoming edges from outside
    std::array<size_t, MAX_STATES> entries;
    size_t entry_count = 0;

    // Exit vertices: vertices in region with outgoing edges to outside
    std::array<size_t, MAX_STATES> exits;
    size_t exit_count = 0;

    // Predecessors: vertices outside region that point to entries
    std::array<size_t, MAX_STATES> preds;
    size_t pred_count = 0;

    // Successors: vertices outside region that exits point to
    std::array<size_t, MAX_STATES> succs;
    size_t succ_count = 0;
};

// Helper: Check if vertex is in set
constexpr bool contains(const std::array<size_t, MAX_REGION_SIZE>& arr, size_t count, size_t value) {
    for (size_t i = 0; i < count; i++) {
        if (arr[i] == value) return true;
    }
    return false;
}

constexpr bool contains_state(const std::array<size_t, MAX_STATES>& arr, size_t count, size_t value) {
    for (size_t i = 0; i < count; i++) {
        if (arr[i] == value) return true;
    }
    return false;
}

// Helper: Add unique vertex to array
constexpr void add_unique(std::array<size_t, MAX_STATES>& arr, size_t& count, size_t value) {
    if (!contains_state(arr, count, value) && count < MAX_STATES) {
        arr[count++] = value;
    }
}

// Update region boundaries (entries, exits, preds, succs)
template <typename NFA>
constexpr void update_region_boundaries(const acyclic_graph<NFA>& dag, region_info& region) {
    region.entry_count = 0;
    region.exit_count = 0;
    region.pred_count = 0;
    region.succ_count = 0;

    // Find entries and their predecessors
    for (size_t i = 0; i < region.vertex_count; i++) {
        size_t v = region.vertices[i];

        // Check incoming edges (from outside region)
        auto preds = dag.get_forward_predecessors(v);
        bool has_external_pred = false;

        for (size_t j = 0; j < preds.count; j++) {
            size_t pred = preds.predecessors[j];
            if (!contains(region.vertices, region.vertex_count, pred)) {
                // External predecessor
                has_external_pred = true;
                add_unique(region.preds, region.pred_count, pred);
            }
        }

        if (has_external_pred) {
            add_unique(region.entries, region.entry_count, v);
        }

        // Check outgoing edges (to outside region)
        auto succs = dag.get_forward_successors(v);
        bool has_external_succ = false;

        for (size_t j = 0; j < succs.count; j++) {
            size_t succ = succs.successors[j];
            if (!contains(region.vertices, region.vertex_count, succ)) {
                // External successor
                has_external_succ = true;
                add_unique(region.succs, region.succ_count, succ);
            }
        }

        if (has_external_succ) {
            add_unique(region.exits, region.exit_count, v);
        }
    }
}

// Check if region forms a valid cut
// Valid cut: All entries have same predecessors, all exits have same successors
template <typename NFA>
constexpr bool is_valid_cut(const acyclic_graph<NFA>& dag, const region_info& region) {
    if (region.entry_count == 0 || region.exit_count == 0) {
        return true;  // Trivial region
    }

    // Check: All entries must have the SAME set of predecessors
    for (size_t i = 0; i < region.entry_count; i++) {
        size_t entry = region.entries[i];

        // Count how many of the region's predecessors point to this entry
        size_t matching_preds = 0;
        for (size_t j = 0; j < region.pred_count; j++) {
            size_t pred = region.preds[j];

            // Check if pred -> entry edge exists
            auto succs = dag.get_forward_successors(pred);
            for (size_t k = 0; k < succs.count; k++) {
                if (succs.successors[k] == entry) {
                    matching_preds++;
                    break;
                }
            }
        }

        // This entry must have edges from ALL region predecessors
        if (matching_preds != region.pred_count) {
            return false;
        }
    }

    // Check: All exits must have the SAME set of successors
    for (size_t i = 0; i < region.exit_count; i++) {
        size_t exit_v = region.exits[i];

        // Count how many of the region's successors are reachable from this exit
        size_t matching_succs = 0;
        auto succs = dag.get_forward_successors(exit_v);
        for (size_t j = 0; j < region.succ_count; j++) {
            size_t succ = region.succs[j];

            // Check if exit_v -> succ edge exists
            for (size_t k = 0; k < succs.count; k++) {
                if (succs.successors[k] == succ) {
                    matching_succs++;
                    break;
                }
            }
        }

        // This exit must have edges to ALL region successors
        if (matching_succs != region.succ_count) {
            return false;
        }
    }

    return true;
}

// Find all regions in the graph using incremental cut-set analysis
template <typename NFA>
constexpr auto find_regions(const acyclic_graph<NFA>& dag) {
    struct result_t {
        std::array<region_info, MAX_REGIONS> regions;
        size_t count = 0;
    };

    result_t result{};
    auto topo = topological_sort(dag);

    if (topo.count < 2) {
        return result;  // Too small to partition
    }

    region_info candidate{};
    size_t topo_idx = 0;

    // Start with first two vertices
    candidate.vertices[candidate.vertex_count++] = topo.order[topo_idx++];
    candidate.vertices[candidate.vertex_count++] = topo.order[topo_idx++];
    update_region_boundaries(dag, candidate);

    while (topo_idx < topo.count) {
        if (is_valid_cut(dag, candidate)) {
            // Save this region
            if (result.count < MAX_REGIONS) {
                result.regions[result.count++] = candidate;
            }

            // Start new candidate with next vertex
            candidate = {};
            if (topo_idx < topo.count) {
                candidate.vertices[candidate.vertex_count++] = topo.order[topo_idx++];
                update_region_boundaries(dag, candidate);
            }
        } else {
            // Add next vertex to candidate
            if (topo_idx < topo.count && candidate.vertex_count < MAX_REGION_SIZE) {
                candidate.vertices[candidate.vertex_count++] = topo.order[topo_idx++];
                update_region_boundaries(dag, candidate);
            } else {
                // Can't grow region further, save it anyway
                if (result.count < MAX_REGIONS) {
                    result.regions[result.count++] = candidate;
                }
                candidate = {};
                if (topo_idx < topo.count) {
                    candidate.vertices[candidate.vertex_count++] = topo.order[topo_idx++];
                    update_region_boundaries(dag, candidate);
                }
            }
        }
    }

    // Save final region
    if (candidate.vertex_count > 0 && result.count < MAX_REGIONS) {
        result.regions[result.count++] = candidate;
    }

    return result;
}

// =============================================================================
// Phase 6.4: String Extraction
// =============================================================================

// Check if a state is "simple" (literal or small character set)
template <typename NFA>
constexpr bool is_simple_state(const NFA& nfa, size_t state_id) {
    if (state_id >= nfa.state_count) return false;

    const auto& state = nfa.states[state_id];

    // Start state is not extractable
    if (state_id == nfa.start_state) return false;

    // Accept states are not extractable (they're the end)
    for (size_t i = 0; i < nfa.accept_count; i++) {
        if (nfa.accept_states[i] == state_id) return false;
    }

    // State must have a symbol (literal character)
    if (state.symbol == '\0') return false;

    // TODO: Add character-set support (for now, only literals)
    // For now, we only accept simple literal characters

    return true;
}

// Extract string from a linear path through region
template <typename NFA>
constexpr auto extract_string_from_path(const NFA& nfa,
                                       const std::array<size_t, MAX_REGION_SIZE>& path,
                                       size_t path_length) {
    dominators::literal_result<64> result{};

    if (path_length == 0) return result;

    // Check all states in path are simple
    for (size_t i = 0; i < path_length; i++) {
        if (!is_simple_state(nfa, path[i])) {
            return result;  // Not extractable
        }
    }

    // Extract characters from path
    for (size_t i = 0; i < path_length && result.length < 64; i++) {
        result.chars[result.length++] = nfa.states[path[i]].symbol;
    }

    // Only accept strings of reasonable length (≥2 chars)
    if (result.length >= 2) {
        result.has_literal = true;
    } else {
        result.has_literal = false;
        result.length = 0;
    }

    return result;
}

// Extract string by traversing backward from accept states
template <typename NFA>
constexpr auto extract_string_backward(const NFA& nfa, size_t accept_state) {
    dominators::literal_result<64> result{};

    // Traverse backwards from accept state, collecting characters
    std::array<char, 64> chars_reversed{};
    size_t char_count = 0;

    size_t current = accept_state;
    std::array<bool, MAX_STATES> visited{};

    // Walk backwards following predecessors
    while (char_count < 64 && !visited[current]) {
        visited[current] = true;

        // Get the symbol for this state
        char sym = nfa.states[current].symbol;
        if (sym != '\0' && current != nfa.start_state) {
            chars_reversed[char_count++] = sym;
        }

        // Find predecessor (state that points to current)
        bool found_pred = false;
        for (size_t i = 0; i < nfa.state_count && !found_pred; i++) {
            for (size_t j = 0; j < nfa.states[i].successor_count; j++) {
                if (nfa.states[i].successors[j] == current) {
                    current = i;
                    found_pred = true;
                    break;
                }
            }
        }

        if (!found_pred || current == nfa.start_state) break;
    }

    // Reverse the string (we collected it backwards)
    for (size_t i = 0; i < char_count; i++) {
        result.chars[i] = chars_reversed[char_count - 1 - i];
    }
    result.length = char_count;

    // Only accept strings of reasonable length (≥2 chars)
    if (result.length >= 2) {
        result.has_literal = true;
    }

    return result;
}

// Extract strings from a region
// Strategy: Look for accept states in region and extract backwards
template <typename NFA>
constexpr auto extract_strings_from_region(const NFA& nfa, const region_info& region) {
    dominators::literal_result<64> result{};

    if (region.vertex_count == 0) return result;

    // Check if region contains accept states
    for (size_t i = 0; i < region.vertex_count; i++) {
        size_t state_id = region.vertices[i];

        // Check if this is an accept state
        for (size_t j = 0; j < nfa.accept_count; j++) {
            if (nfa.accept_states[j] == state_id) {
                // Found accept state - extract string backwards
                auto str = extract_string_backward(nfa, state_id);
                if (str.has_literal) {
                    return str;  // Return first valid string
                }
            }
        }
    }

    return result;
}

// Main function: Extract literal from regions
template <typename NFA>
constexpr auto extract_literal_from_regions(const NFA& nfa) {
    dominators::literal_result<64> result{};

    // Build acyclic graph
    auto dag = acyclic_graph(nfa);

    // Find regions
    auto regions = find_regions(dag);

    // Merge back-edge-connected regions
    merge_back_edge_regions(dag, regions);

    // Try to extract strings from each region
    for (size_t i = 0; i < regions.count; i++) {
        auto str = extract_strings_from_region(nfa, regions.regions[i]);
        if (str.has_literal) {
            return str;  // Return first extractable string
        }
    }

    return result;  // No extractable literal
}

// =============================================================================
// Phase 6.5: Back Edge Merging
// =============================================================================

// Merge regions connected by back edges
// Back edges can connect regions that seemed independent in the DAG
// Must merge to maintain correctness!
template <typename NFA, typename Regions>
constexpr void merge_back_edge_regions(const acyclic_graph<NFA>& dag,
                                      Regions& regions) {
    if (regions.count == 0) return;

    // Find which region each state belongs to
    std::array<size_t, MAX_STATES> state_to_region{};
    for (size_t i = 0; i < MAX_STATES; i++) {
        state_to_region[i] = MAX_REGIONS;  // Not in any region
    }

    for (size_t r = 0; r < regions.count; r++) {
        for (size_t v = 0; v < regions.regions[r].vertex_count; v++) {
            size_t state = regions.regions[r].vertices[v];
            state_to_region[state] = r;
        }
    }

    // For each back edge, check if it connects different regions
    std::array<bool, MAX_REGIONS> merged_into_previous{};  // Track merged regions

    for (size_t u = 0; u < dag.nfa.state_count; u++) {
        for (size_t j = 0; j < dag.nfa.states[u].successor_count; j++) {
            if (!dag.is_back_edge[u * MAX_STATES + j]) continue;

            size_t v = dag.nfa.states[u].successors[j];

            size_t region_u = state_to_region[u];
            size_t region_v = state_to_region[v];

            // If different regions, merge them
            if (region_u != region_v &&
                region_u < MAX_REGIONS &&
                region_v < MAX_REGIONS &&
                !merged_into_previous[region_u]) {

                // Merge region_u into region_v (and all intervening regions)
                size_t min_region = (region_u < region_v) ? region_u : region_v;
                size_t max_region = (region_u < region_v) ? region_v : region_u;

                // Merge all regions from min to max into min
                for (size_t r = min_region + 1; r <= max_region && r < regions.count; r++) {
                    // Copy vertices from region r to region min_region
                    auto& target = regions.regions[min_region];
                    auto& source = regions.regions[r];

                    for (size_t v = 0; v < source.vertex_count &&
                         target.vertex_count < MAX_REGION_SIZE; v++) {
                        // Check if vertex already in target
                        bool already_present = false;
                        for (size_t t = 0; t < target.vertex_count; t++) {
                            if (target.vertices[t] == source.vertices[v]) {
                                already_present = true;
                                break;
                            }
                        }
                        if (!already_present) {
                            target.vertices[target.vertex_count++] = source.vertices[v];
                        }
                    }

                    merged_into_previous[r] = true;
                }
            }
        }
    }

    // Compact regions (remove merged ones)
    size_t write_idx = 0;
    for (size_t r = 0; r < regions.count; r++) {
        if (!merged_into_previous[r]) {
            if (write_idx != r) {
                regions.regions[write_idx] = regions.regions[r];
            }
            write_idx++;
        }
    }
    regions.count = write_idx;
}

// =============================================================================
// Phase 6.6: Integration (TODO)
// =============================================================================

// TODO: Integrate region analysis into decomposition::extract_literal()

} // namespace region
} // namespace ctre

#endif // CTRE__REGION_ANALYSIS__HPP
