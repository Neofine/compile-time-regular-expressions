// Glushkov NFA Construction - Phase 1 Testing
// This file is ISOLATED from main CTRE - safe to experiment!

#include <cstddef>
#include <array>
#include <utility>
#include <iostream>

// Include CTRE main header (read-only, we're just inspecting types)
#include "../include/ctre.hpp"

namespace ctre {
namespace glushkov {

// =============================================================================
// STEP 1: Type Traits - Identify CTRE Pattern Types
// =============================================================================

// Check if type is ctre::string<>
template <typename T>
struct is_string : std::false_type {};

template <auto... Cs>
struct is_string<string<Cs...>> : std::true_type {
    static constexpr size_t length = sizeof...(Cs);
};

// Check if type is ctre::sequence<>
template <typename T>
struct is_sequence : std::false_type {};

template <typename... Content>
struct is_sequence<sequence<Content...>> : std::true_type {};

// Check if type is ctre::select<>
template <typename T>
struct is_select : std::false_type {};

template <typename... Options>
struct is_select<select<Options...>> : std::true_type {};

// Check if type is ctre::repeat<>
template <typename T>
struct is_repeat : std::false_type {};

template <size_t A, size_t B, typename... Content>
struct is_repeat<repeat<A, B, Content...>> : std::true_type {};

// Check if type is ctre::lazy_repeat<>
template <typename T>
struct is_lazy_repeat : std::false_type {};

template <size_t A, size_t B, typename... Content>
struct is_lazy_repeat<lazy_repeat<A, B, Content...>> : std::true_type {};

// Check if type is ctre::possessive_repeat<>
template <typename T>
struct is_possessive_repeat : std::false_type {};

template <size_t A, size_t B, typename... Content>
struct is_possessive_repeat<possessive_repeat<A, B, Content...>> : std::true_type {};

// Check if type is any repeat type
template <typename T>
constexpr bool is_any_repeat_v = is_repeat<T>::value ||
                                  is_lazy_repeat<T>::value ||
                                  is_possessive_repeat<T>::value;

// Check if type is ctre::character<>
template <typename T>
struct is_character : std::false_type {};

template <auto C>
struct is_character<character<C>> : std::true_type {};

// Check if type is ctre::any
template <typename T>
struct is_any : std::false_type {};

template <>
struct is_any<any> : std::true_type {};

// Check if type is ctre::empty
template <typename T>
struct is_empty : std::false_type {};

template <>
struct is_empty<empty> : std::true_type {};

// Check if type is character-like (has match_char method)
template <typename T>
concept CharacterLike = requires {
    { T::match_char(char{}, flags{}) } -> std::same_as<bool>;
};

// =============================================================================
// STEP 2: Position Counting (Glushkov semantics)
// =============================================================================
// Rule: Count unique positions in the pattern
//   - Character: 1 position
//   - String "abc": 3 positions
//   - Sequence "ab.cd": 4 positions (sum)
//   - Select "ab|cd": 4 positions (sum)
//   - Repeat "a*": 1 position (doesn't multiply!)
//   - Empty: 0 positions
// =============================================================================

// Forward declaration
template <typename Pattern>
constexpr size_t count_positions();

// Helper: Count positions in variadic pack
template <typename... Patterns>
constexpr size_t count_positions_pack() {
    return (count_positions<Patterns>() + ... + 0);
}

// Main position counter
template <typename Pattern>
constexpr size_t count_positions() {
    // Empty pattern: 0 positions
    if constexpr (is_empty<Pattern>::value) {
        return 0;
    }
    // Character: 1 position
    else if constexpr (is_character<Pattern>::value) {
        return 1;
    }
    // Any (.): 1 position
    else if constexpr (is_any<Pattern>::value) {
        return 1;
    }
    // String: N positions (one per char)
    else if constexpr (is_string<Pattern>::value) {
        return is_string<Pattern>::length;
    }
    // Sequence: sum of all components
    else if constexpr (is_sequence<Pattern>::value) {
        return []<typename... Content>(sequence<Content...>*) {
            return count_positions_pack<Content...>();
        }(static_cast<Pattern*>(nullptr));
    }
    // Select (alternation): sum of all branches
    else if constexpr (is_select<Pattern>::value) {
        return []<typename... Options>(select<Options...>*) {
            return count_positions_pack<Options...>();
        }(static_cast<Pattern*>(nullptr));
    }
    // Repeat: same as inner content (positions don't multiply!)
    else if constexpr (is_any_repeat_v<Pattern>) {
        if constexpr (is_repeat<Pattern>::value) {
            return []<size_t A, size_t B, typename... Content>(repeat<A, B, Content...>*) {
                return count_positions_pack<Content...>();
            }(static_cast<Pattern*>(nullptr));
        }
        else if constexpr (is_lazy_repeat<Pattern>::value) {
            return []<size_t A, size_t B, typename... Content>(lazy_repeat<A, B, Content...>*) {
                return count_positions_pack<Content...>();
            }(static_cast<Pattern*>(nullptr));
        }
        else if constexpr (is_possessive_repeat<Pattern>::value) {
            return []<size_t A, size_t B, typename... Content>(possessive_repeat<A, B, Content...>*) {
                return count_positions_pack<Content...>();
            }(static_cast<Pattern*>(nullptr));
        }
    }
    // Character-like (set, range, etc.): 1 position
    else if constexpr (CharacterLike<Pattern>) {
        return 1;
    }
    // Unknown/unsupported: 0 positions
    else {
        return 0;
    }
}

// =============================================================================
// STEP 3: Nullable Detection (Glushkov 1961)
// =============================================================================
// Rule: Can pattern match empty string?
//   - Empty: true
//   - Character: false
//   - String "": true, String "a": false
//   - Sequence: ALL components must be nullable
//   - Select: ANY component nullable
//   - Repeat{min=0}: true (can repeat zero times)
//   - Repeat{min>0}: depends on content nullable
// =============================================================================

// Forward declaration
template <typename Pattern>
constexpr bool nullable();

// Helper: Check if all in pack are nullable
template <typename... Patterns>
constexpr bool all_nullable() {
    if constexpr (sizeof...(Patterns) == 0) {
        return true;  // Empty pack is nullable
    } else {
        return (nullable<Patterns>() && ...);
    }
}

// Helper: Check if any in pack are nullable
template <typename... Patterns>
constexpr bool any_nullable() {
    if constexpr (sizeof...(Patterns) == 0) {
        return false;
    } else {
        return (nullable<Patterns>() || ...);
    }
}

// Main nullable detector
template <typename Pattern>
constexpr bool nullable() {
    // Empty: always nullable
    if constexpr (is_empty<Pattern>::value) {
        return true;
    }
    // Character: never nullable
    else if constexpr (is_character<Pattern>::value) {
        return false;
    }
    // Any (.): never nullable
    else if constexpr (is_any<Pattern>::value) {
        return false;
    }
    // String: nullable only if empty
    else if constexpr (is_string<Pattern>::value) {
        return is_string<Pattern>::length == 0;
    }
    // Sequence: nullable if ALL components nullable
    else if constexpr (is_sequence<Pattern>::value) {
        return []<typename... Content>(sequence<Content...>*) {
            return all_nullable<Content...>();
        }(static_cast<Pattern*>(nullptr));
    }
    // Select: nullable if ANY branch nullable
    else if constexpr (is_select<Pattern>::value) {
        return []<typename... Options>(select<Options...>*) {
            return any_nullable<Options...>();
        }(static_cast<Pattern*>(nullptr));
    }
    // Repeat: check minimum count
    else if constexpr (is_any_repeat_v<Pattern>) {
        if constexpr (is_repeat<Pattern>::value) {
            return []<size_t A, size_t B, typename... Content>(repeat<A, B, Content...>*) {
                // Nullable if min count is 0, OR if min > 0 but all content is nullable
                if constexpr (A == 0) {
                    return true;
                } else {
                    return all_nullable<Content...>();
                }
            }(static_cast<Pattern*>(nullptr));
        }
        else if constexpr (is_lazy_repeat<Pattern>::value) {
            return []<size_t A, size_t B, typename... Content>(lazy_repeat<A, B, Content...>*) {
                if constexpr (A == 0) {
                    return true;
                } else {
                    return all_nullable<Content...>();
                }
            }(static_cast<Pattern*>(nullptr));
        }
        else if constexpr (is_possessive_repeat<Pattern>::value) {
            return []<size_t A, size_t B, typename... Content>(possessive_repeat<A, B, Content...>*) {
                if constexpr (A == 0) {
                    return true;
                } else {
                    return all_nullable<Content...>();
                }
            }(static_cast<Pattern*>(nullptr));
        }
    }
    // Character-like: never nullable
    else if constexpr (CharacterLike<Pattern>) {
        return false;
    }
    // Unknown: assume not nullable (safe default)
    else {
        return false;
    }
}

// =============================================================================
// STEP 4: First() Sets (Glushkov 1961)
// =============================================================================
// Rule: Which positions can START a match?
//   - Empty: {}
//   - Character at pos P: {P}
//   - String "abc" at pos P: {P} (first char only)
//   - Sequence e1.e2: first(e1) ∪ (first(e2) if nullable(e1))
//   - Select e1|e2: first(e1) ∪ first(e2)
//   - Repeat e*: first(e)
// =============================================================================

// Helper: Merge two position sets
template <size_t N1, size_t N2>
constexpr auto merge_position_sets(const std::array<size_t, N1>& set1, size_t count1,
                                   const std::array<size_t, N2>& set2, size_t count2) {
    constexpr size_t max_size = N1 + N2;
    std::array<size_t, max_size> result{};
    size_t result_count = 0;

    // Copy first set
    for (size_t i = 0; i < count1 && i < N1; ++i) {
        result[result_count++] = set1[i];
    }

    // Copy second set
    for (size_t i = 0; i < count2 && i < N2; ++i) {
        result[result_count++] = set2[i];
    }

    return std::pair{result, result_count};
}

// Forward declarations
template <typename Pattern>
constexpr auto first_positions(size_t offset);

template <typename... Content>
constexpr auto first_pack(size_t offset);

template <typename... Content>
constexpr auto first_sequence(size_t offset);

template <typename... Options>
constexpr auto first_select(size_t offset);

// Main first() function
template <typename Pattern>
constexpr auto first_positions(size_t offset) {
    // Empty: no starting positions
    if constexpr (is_empty<Pattern>::value) {
        return std::pair{std::array<size_t, 1>{}, size_t{0}};
    }
    // Character: position at offset+1
    else if constexpr (is_character<Pattern>::value) {
        return std::pair{std::array{offset + 1}, size_t{1}};
    }
    // Any (.): position at offset+1
    else if constexpr (is_any<Pattern>::value) {
        return std::pair{std::array{offset + 1}, size_t{1}};
    }
    // String: first character position
    else if constexpr (is_string<Pattern>::value) {
        if constexpr (is_string<Pattern>::length > 0) {
            return std::pair{std::array{offset + 1}, size_t{1}};
        } else {
            return std::pair{std::array<size_t, 1>{}, size_t{0}};
        }
    }
    // Sequence: first(e1) ∪ (first(e2) if nullable(e1))
    else if constexpr (is_sequence<Pattern>::value) {
        return []<typename... Content>(sequence<Content...>*, size_t off) {
            return first_sequence<Content...>(off);
        }(static_cast<Pattern*>(nullptr), offset);
    }
    // Select: union of all branches
    else if constexpr (is_select<Pattern>::value) {
        return []<typename... Options>(select<Options...>*, size_t off) {
            return first_select<Options...>(off);
        }(static_cast<Pattern*>(nullptr), offset);
    }
    // Repeat: first(content)
    else if constexpr (is_any_repeat_v<Pattern>) {
        if constexpr (is_repeat<Pattern>::value) {
            return []<size_t A, size_t B, typename... Content>(repeat<A, B, Content...>*, size_t off) {
                return first_pack<Content...>(off);
            }(static_cast<Pattern*>(nullptr), offset);
        }
        else if constexpr (is_lazy_repeat<Pattern>::value) {
            return []<size_t A, size_t B, typename... Content>(lazy_repeat<A, B, Content...>*, size_t off) {
                return first_pack<Content...>(off);
            }(static_cast<Pattern*>(nullptr), offset);
        }
        else if constexpr (is_possessive_repeat<Pattern>::value) {
            return []<size_t A, size_t B, typename... Content>(possessive_repeat<A, B, Content...>*, size_t off) {
                return first_pack<Content...>(off);
            }(static_cast<Pattern*>(nullptr), offset);
        }
    }
    // Character-like: position at offset+1
    else if constexpr (CharacterLike<Pattern>) {
        return std::pair{std::array{offset + 1}, size_t{1}};
    }
    // Unknown: empty set
    else {
        return std::pair{std::array<size_t, 1>{}, size_t{0}};
    }
}

// Helper: first() for variadic pack (used in repeat)
template <typename... Content>
constexpr auto first_pack(size_t offset) {
    if constexpr (sizeof...(Content) == 0) {
        return std::pair{std::array<size_t, 1>{}, size_t{0}};
    } else if constexpr (sizeof...(Content) == 1) {
        return []<typename T>(size_t off) {
            return first_positions<T>(off);
        }.template operator()<Content...>(offset);
    } else {
        // Multiple content items: treat as sequence
        return first_sequence<Content...>(offset);
    }
}

// Helper: first() for sequence
template <typename... Content>
constexpr auto first_sequence(size_t offset) {
    if constexpr (sizeof...(Content) == 0) {
        return std::pair{std::array<size_t, 1>{}, size_t{0}};
    } else if constexpr (sizeof...(Content) == 1) {
        return []<typename T>(size_t off) {
            return first_positions<T>(off);
        }.template operator()<Content...>(offset);
    } else {
        return []<typename Head, typename... Tail>(size_t off) {
            auto [head_first, head_count] = first_positions<Head>(off);

            // If head is nullable, we also need first of tail
            if constexpr (nullable<Head>()) {
                size_t tail_offset = off + count_positions<Head>();
                auto [tail_first, tail_count] = first_sequence<Tail...>(tail_offset);
                return merge_position_sets(head_first, head_count, tail_first, tail_count);
            } else {
                // Head not nullable, only its first set matters
                return std::pair{head_first, head_count};
            }
        }.template operator()<Content...>(offset);
    }
}

// Helper: first() for select (alternation)
template <typename... Options>
constexpr auto first_select(size_t offset) {
    if constexpr (sizeof...(Options) == 0) {
        return std::pair{std::array<size_t, 1>{}, size_t{0}};
    } else if constexpr (sizeof...(Options) == 1) {
        return []<typename T>(size_t off) {
            return first_positions<T>(off);
        }.template operator()<Options...>(offset);
    } else {
        return []<typename Head, typename... Tail>(size_t off) {
            auto [head_first, head_count] = first_positions<Head>(off);

            // All branches start at same offset (alternation doesn't advance)
            size_t tail_offset = off + count_positions<Head>();
            auto [tail_first, tail_count] = first_select<Tail...>(tail_offset);

            return merge_position_sets(head_first, head_count, tail_first, tail_count);
        }.template operator()<Options...>(offset);
    }
}

// =============================================================================
// STEP 5: Last() Sets (Glushkov 1961)
// =============================================================================
// Rule: Which positions can END a match?
//   - Empty: {}
//   - Character at pos P: {P}
//   - String "abc" at pos P: {P+2} (last char)
//   - Sequence e1.e2: last(e2) ∪ (last(e1) if nullable(e2))
//   - Select e1|e2: last(e1) ∪ last(e2)
//   - Repeat e*: last(e)
// =============================================================================

// Forward declarations
template <typename Pattern>
constexpr auto last_positions(size_t offset);

template <typename... Content>
constexpr auto last_pack(size_t offset);

template <typename... Content>
constexpr auto last_sequence(size_t offset);

template <typename Head, typename... Tail>
constexpr auto last_sequence_reverse(size_t offset);

template <typename... Options>
constexpr auto last_select(size_t offset);

// Main last() function
template <typename Pattern>
constexpr auto last_positions(size_t offset) {
    // Empty: no ending positions
    if constexpr (is_empty<Pattern>::value) {
        return std::pair{std::array<size_t, 1>{}, size_t{0}};
    }
    // Character: position at offset+1
    else if constexpr (is_character<Pattern>::value) {
        return std::pair{std::array{offset + 1}, size_t{1}};
    }
    // Any (.): position at offset+1
    else if constexpr (is_any<Pattern>::value) {
        return std::pair{std::array{offset + 1}, size_t{1}};
    }
    // String: last character position
    else if constexpr (is_string<Pattern>::value) {
        if constexpr (is_string<Pattern>::length > 0) {
            return std::pair{std::array{offset + is_string<Pattern>::length}, size_t{1}};
        } else {
            return std::pair{std::array<size_t, 1>{}, size_t{0}};
        }
    }
    // Sequence: last(e2) ∪ (last(e1) if nullable(e2))
    else if constexpr (is_sequence<Pattern>::value) {
        return []<typename... Content>(sequence<Content...>*, size_t off) {
            return last_sequence<Content...>(off);
        }(static_cast<Pattern*>(nullptr), offset);
    }
    // Select: union of all branches
    else if constexpr (is_select<Pattern>::value) {
        return []<typename... Options>(select<Options...>*, size_t off) {
            return last_select<Options...>(off);
        }(static_cast<Pattern*>(nullptr), offset);
    }
    // Repeat: last(content)
    else if constexpr (is_any_repeat_v<Pattern>) {
        if constexpr (is_repeat<Pattern>::value) {
            return []<size_t A, size_t B, typename... Content>(repeat<A, B, Content...>*, size_t off) {
                return last_pack<Content...>(off);
            }(static_cast<Pattern*>(nullptr), offset);
        }
        else if constexpr (is_lazy_repeat<Pattern>::value) {
            return []<size_t A, size_t B, typename... Content>(lazy_repeat<A, B, Content...>*, size_t off) {
                return last_pack<Content...>(off);
            }(static_cast<Pattern*>(nullptr), offset);
        }
        else if constexpr (is_possessive_repeat<Pattern>::value) {
            return []<size_t A, size_t B, typename... Content>(possessive_repeat<A, B, Content...>*, size_t off) {
                return last_pack<Content...>(off);
            }(static_cast<Pattern*>(nullptr), offset);
        }
    }
    // Character-like: position at offset+1
    else if constexpr (CharacterLike<Pattern>) {
        return std::pair{std::array{offset + 1}, size_t{1}};
    }
    // Unknown: empty set
    else {
        return std::pair{std::array<size_t, 1>{}, size_t{0}};
    }
}

// Helper: last() for variadic pack (used in repeat)
template <typename... Content>
constexpr auto last_pack(size_t offset) {
    if constexpr (sizeof...(Content) == 0) {
        return std::pair{std::array<size_t, 1>{}, size_t{0}};
    } else if constexpr (sizeof...(Content) == 1) {
        return []<typename T>(size_t off) {
            return last_positions<T>(off);
        }.template operator()<Content...>(offset);
    } else {
        // Multiple content items: treat as sequence
        return last_sequence<Content...>(offset);
    }
}

// Helper: last() for sequence (REVERSE of first)
template <typename... Content>
constexpr auto last_sequence(size_t offset) {
    if constexpr (sizeof...(Content) == 0) {
        return std::pair{std::array<size_t, 1>{}, size_t{0}};
    } else if constexpr (sizeof...(Content) == 1) {
        return []<typename T>(size_t off) {
            return last_positions<T>(off);
        }.template operator()<Content...>(offset);
    } else {
        // Process sequence in REVERSE: last element first
        return []<typename... All>(size_t off) {
            return last_sequence_reverse<All...>(off);
        }.template operator()<Content...>(offset);
    }
}

// Helper to reverse sequence processing for last()
template <typename Head, typename... Tail>
constexpr auto last_sequence_reverse(size_t offset) {
    if constexpr (sizeof...(Tail) == 0) {
        // Only one element: return its last
        return last_positions<Head>(offset);
    } else {
        // Process tail first (which are later in sequence)
        size_t head_size = count_positions<Head>();
        auto [tail_last, tail_count] = last_sequence_reverse<Tail...>(offset + head_size);

        // Get the last element type
        using LastType = typename std::tuple_element<sizeof...(Tail) - 1, std::tuple<Tail...>>::type;

        // If last element nullable, include last from previous elements
        if constexpr (nullable<LastType>()) {
            auto [head_last, head_count] = last_positions<Head>(offset);
            return merge_position_sets(tail_last, tail_count, head_last, head_count);
        } else {
            // Last element not nullable, only its last matters
            return std::pair{tail_last, tail_count};
        }
    }
}

// Helper: last() for select (alternation)
template <typename... Options>
constexpr auto last_select(size_t offset) {
    if constexpr (sizeof...(Options) == 0) {
        return std::pair{std::array<size_t, 1>{}, size_t{0}};
    } else if constexpr (sizeof...(Options) == 1) {
        return []<typename T>(size_t off) {
            return last_positions<T>(off);
        }.template operator()<Options...>(offset);
    } else {
        return []<typename Head, typename... Tail>(size_t off) {
            auto [head_last, head_count] = last_positions<Head>(off);

            // All branches start at same offset
            size_t tail_offset = off + count_positions<Head>();
            auto [tail_last, tail_count] = last_select<Tail...>(tail_offset);

            return merge_position_sets(head_last, head_count, tail_last, tail_count);
        }.template operator()<Options...>(offset);
    }
}

// =============================================================================
// STEP 6: Follow() Transitions (Glushkov 1961)
// =============================================================================
// Rule: Which positions can follow position P?
//   - String "abc": follow(1)={2}, follow(2)={3}, follow(3)={}
//   - Sequence e1.e2: last(e1) transitions to first(e2)
//   - Select e1|e2: union of follows from both branches
//   - Repeat e*: last(e) transitions back to first(e) (LOOP!)
//
// This builds the complete transition graph!
// =============================================================================

// Structure to hold follow sets for all positions
template <size_t MaxPositions>
struct follow_map {
    // For each position, store which positions can follow it
    std::array<std::array<size_t, 32>, MaxPositions> successors{};
    std::array<size_t, MaxPositions> successor_counts{};

    constexpr void add_edge(size_t from, size_t to) {
        if (from < MaxPositions && successor_counts[from] < 32) {
            // Check if edge already exists
            for (size_t i = 0; i < successor_counts[from]; ++i) {
                if (successors[from][i] == to) {
                    return; // Already have this edge
                }
            }
            successors[from][successor_counts[from]++] = to;
        }
    }

    constexpr void add_edges(size_t from, const auto& to_positions, size_t to_count) {
        for (size_t i = 0; i < to_count; ++i) {
            add_edge(from, to_positions[i]);
        }
    }
};

// Forward declarations
template <typename Pattern, size_t MaxPositions>
constexpr void build_follow(follow_map<MaxPositions>& fmap, size_t offset);

template <typename... Content, size_t MaxPositions>
constexpr void build_follow_sequence(follow_map<MaxPositions>& fmap, size_t offset);

template <typename... Options, size_t MaxPositions>
constexpr void build_follow_select(follow_map<MaxPositions>& fmap, size_t offset);

template <typename... Content, size_t MaxPositions>
constexpr void build_follow_repeat(follow_map<MaxPositions>& fmap, size_t offset);

// Main follow builder
template <typename Pattern, size_t MaxPositions = 512>
constexpr follow_map<MaxPositions> compute_follow() {
    follow_map<MaxPositions> fmap{};
    build_follow<Pattern>(fmap, 0);
    return fmap;
}

// Build follow edges for a pattern
template <typename Pattern, size_t MaxPositions>
constexpr void build_follow(follow_map<MaxPositions>& fmap, size_t offset) {
    // Empty: no transitions
    if constexpr (is_empty<Pattern>::value) {
        // Nothing to do
    }
    // Character: no internal transitions (single position)
    else if constexpr (is_character<Pattern>::value) {
        // Single position, no successors (unless in larger context)
    }
    // Any: single position
    else if constexpr (is_any<Pattern>::value) {
        // Single position, no successors
    }
    // String: linear chain of transitions
    else if constexpr (is_string<Pattern>::value) {
        constexpr size_t len = is_string<Pattern>::length;
        for (size_t i = 1; i < len; ++i) {
            fmap.add_edge(offset + i, offset + i + 1);
        }
    }
    // Sequence: connect components + cross-boundary transitions
    else if constexpr (is_sequence<Pattern>::value) {
        []<typename... Content>(sequence<Content...>*, follow_map<MaxPositions>& fm, size_t off) {
            build_follow_sequence<Content...>(fm, off);
        }(static_cast<Pattern*>(nullptr), fmap, offset);
    }
    // Select: each branch independently
    else if constexpr (is_select<Pattern>::value) {
        []<typename... Options>(select<Options...>*, follow_map<MaxPositions>& fm, size_t off) {
            build_follow_select<Options...>(fm, off);
        }(static_cast<Pattern*>(nullptr), fmap, offset);
    }
    // Repeat: internal + loop back!
    else if constexpr (is_any_repeat_v<Pattern>) {
        if constexpr (is_repeat<Pattern>::value) {
            []<size_t A, size_t B, typename... Content>(repeat<A, B, Content...>*,
                                                         follow_map<MaxPositions>& fm, size_t off) {
                build_follow_repeat<Content...>(fm, off);
            }(static_cast<Pattern*>(nullptr), fmap, offset);
        }
        else if constexpr (is_lazy_repeat<Pattern>::value) {
            []<size_t A, size_t B, typename... Content>(lazy_repeat<A, B, Content...>*,
                                                         follow_map<MaxPositions>& fm, size_t off) {
                build_follow_repeat<Content...>(fm, off);
            }(static_cast<Pattern*>(nullptr), fmap, offset);
        }
        else if constexpr (is_possessive_repeat<Pattern>::value) {
            []<size_t A, size_t B, typename... Content>(possessive_repeat<A, B, Content...>*,
                                                         follow_map<MaxPositions>& fm, size_t off) {
                build_follow_repeat<Content...>(fm, off);
            }(static_cast<Pattern*>(nullptr), fmap, offset);
        }
    }
    // Character-like: single position
    else if constexpr (CharacterLike<Pattern>) {
        // Single position, no successors
    }
}

// Helper: build follow for sequence
template <typename... Content, size_t MaxPositions>
constexpr void build_follow_sequence(follow_map<MaxPositions>& fmap, size_t offset) {
    if constexpr (sizeof...(Content) == 0) {
        return;
    } else if constexpr (sizeof...(Content) == 1) {
        []<typename T>(follow_map<MaxPositions>& fm, size_t off) {
            build_follow<T>(fm, off);
        }.template operator()<Content...>(fmap, offset);
    } else {
        []<typename Head, typename... Tail>(follow_map<MaxPositions>& fm, size_t off) {
            // Build follow for head component
            build_follow<Head>(fm, off);

            // Get last positions of head
            auto [head_last, head_last_count] = last_positions<Head>(off);

            // Get first positions of tail (next component)
            size_t tail_offset = off + count_positions<Head>();
            auto [tail_first, tail_first_count] = first_positions<typename std::tuple_element<0, std::tuple<Tail...>>::type>(tail_offset);

            // Connect: last(head) → first(next)
            for (size_t i = 0; i < head_last_count; ++i) {
                fm.add_edges(head_last[i], tail_first, tail_first_count);
            }

            // Recursively process remaining tail
            build_follow_sequence<Tail...>(fm, tail_offset);
        }.template operator()<Content...>(fmap, offset);
    }
}

// Helper: build follow for select
template <typename... Options, size_t MaxPositions>
constexpr void build_follow_select(follow_map<MaxPositions>& fmap, size_t offset) {
    if constexpr (sizeof...(Options) == 0) {
        return;
    } else {
        []<typename Head, typename... Tail>(follow_map<MaxPositions>& fm, size_t off) {
            // Build follow for this branch
            build_follow<Head>(fm, off);

            // Process remaining branches
            if constexpr (sizeof...(Tail) > 0) {
                size_t next_offset = off + count_positions<Head>();
                build_follow_select<Tail...>(fm, next_offset);
            }
        }.template operator()<Options...>(fmap, offset);
    }
}

// Helper: build follow for repeat (CREATES LOOPS!)
template <typename... Content, size_t MaxPositions>
constexpr void build_follow_repeat(follow_map<MaxPositions>& fmap, size_t offset) {
    if constexpr (sizeof...(Content) == 0) {
        return;
    } else if constexpr (sizeof...(Content) == 1) {
        []<typename T>(follow_map<MaxPositions>& fm, size_t off) {
            // Build internal follows
            build_follow<T>(fm, off);

            // KEY: Add loop back! last(e) → first(e)
            auto [content_first, first_count] = first_positions<T>(off);
            auto [content_last, last_count] = last_positions<T>(off);

            for (size_t i = 0; i < last_count; ++i) {
                fm.add_edges(content_last[i], content_first, first_count);
            }
        }.template operator()<Content...>(fmap, offset);
    } else {
        // Multiple content items in repeat: treat as sequence, then loop
        []<typename... All>(follow_map<MaxPositions>& fm, size_t off) {
            // Build sequence follows
            build_follow_sequence<All...>(fm, off);

            // Add loop back
            auto [content_first, first_count] = first_pack<All...>(off);
            auto [content_last, last_count] = last_pack<All...>(off);

            for (size_t i = 0; i < last_count; ++i) {
                fm.add_edges(content_last[i], content_first, first_count);
            }
        }.template operator()<Content...>(fmap, offset);
    }
}

// =============================================================================
// STEP 7: Complete NFA Construction
// =============================================================================
// Assemble all Glushkov pieces into final NFA:
//   - Assign symbols to positions
//   - Build start state with transitions to first()
//   - Copy follow() transitions
//   - Mark accept states from last()
// =============================================================================

// Helper: Assign symbols to positions
template <typename Pattern>
constexpr void assign_symbols(std::array<char, 512>& symbols, size_t offset);

// Symbol assignment implementations
template <typename Pattern>
constexpr void assign_symbols(std::array<char, 512>& symbols, size_t offset) {
    if constexpr (is_empty<Pattern>::value) {
        // No symbols
    }
    else if constexpr (is_character<Pattern>::value) {
        []<auto C>(character<C>*, std::array<char, 512>& syms, size_t off) {
            syms[off + 1] = static_cast<char>(C);
        }(static_cast<Pattern*>(nullptr), symbols, offset);
    }
    else if constexpr (is_any<Pattern>::value) {
        symbols[offset + 1] = '.';  // Represent any as '.'
    }
    else if constexpr (is_string<Pattern>::value) {
        []<auto... Cs>(string<Cs...>*, std::array<char, 512>& syms, size_t off) {
            size_t pos = 1;
            ((syms[off + pos++] = static_cast<char>(Cs)), ...);
        }(static_cast<Pattern*>(nullptr), symbols, offset);
    }
    else if constexpr (is_sequence<Pattern>::value) {
        []<typename... Content>(sequence<Content...>*, std::array<char, 512>& syms, size_t off) {
            size_t current_offset = off;
            ((assign_symbols<Content>(syms, current_offset),
              current_offset += count_positions<Content>()), ...);
        }(static_cast<Pattern*>(nullptr), symbols, offset);
    }
    else if constexpr (is_select<Pattern>::value) {
        []<typename... Options>(select<Options...>*, std::array<char, 512>& syms, size_t off) {
            size_t current_offset = off;
            ((assign_symbols<Options>(syms, current_offset),
              current_offset += count_positions<Options>()), ...);
        }(static_cast<Pattern*>(nullptr), symbols, offset);
    }
    else if constexpr (is_any_repeat_v<Pattern>) {
        if constexpr (is_repeat<Pattern>::value) {
            []<size_t A, size_t B, typename... Content>(repeat<A, B, Content...>*,
                                                         std::array<char, 512>& syms, size_t off) {
                size_t current_offset = off;
                ((assign_symbols<Content>(syms, current_offset),
                  current_offset += count_positions<Content>()), ...);
            }(static_cast<Pattern*>(nullptr), symbols, offset);
        }
        else if constexpr (is_lazy_repeat<Pattern>::value) {
            []<size_t A, size_t B, typename... Content>(lazy_repeat<A, B, Content...>*,
                                                         std::array<char, 512>& syms, size_t off) {
                size_t current_offset = off;
                ((assign_symbols<Content>(syms, current_offset),
                  current_offset += count_positions<Content>()), ...);
            }(static_cast<Pattern*>(nullptr), symbols, offset);
        }
        else if constexpr (is_possessive_repeat<Pattern>::value) {
            []<size_t A, size_t B, typename... Content>(possessive_repeat<A, B, Content...>*,
                                                         std::array<char, 512>& syms, size_t off) {
                size_t current_offset = off;
                ((assign_symbols<Content>(syms, current_offset),
                  current_offset += count_positions<Content>()), ...);
            }(static_cast<Pattern*>(nullptr), symbols, offset);
        }
    }
    else if constexpr (CharacterLike<Pattern>) {
        symbols[offset + 1] = '?';  // Represent character class as '?'
    }
}

// Complete Glushkov NFA structure
template <typename Pattern, size_t MaxPositions = 512>
struct glushkov_nfa {
    static constexpr size_t MAX_POSITIONS = MaxPositions;
    static constexpr size_t MAX_SUCCESSORS = 32;

    struct State {
        size_t id = 0;
        char symbol = '\0';
        std::array<size_t, MAX_SUCCESSORS> successors{};
        size_t successor_count = 0;
    };

    std::array<State, MAX_POSITIONS> states{};
    size_t state_count = 0;
    size_t start_state = 0;
    std::array<size_t, MAX_SUCCESSORS> accept_states{};
    size_t accept_count = 0;

    constexpr glushkov_nfa() {
        // 1. Count positions (state 0 is start, positions are states 1..N)
        constexpr size_t num_positions = count_positions<Pattern>();
        state_count = num_positions + 1;  // +1 for start state

        static_assert(num_positions + 1 <= MAX_POSITIONS, "Pattern too large");

        // 2. Initialize states
        for (size_t i = 0; i < state_count; ++i) {
            states[i].id = i;
            states[i].symbol = '\0';
            states[i].successor_count = 0;
        }

        // 3. Assign symbols to positions
        std::array<char, 512> symbols{};
        assign_symbols<Pattern>(symbols, 0);
        for (size_t i = 1; i < state_count; ++i) {
            states[i].symbol = symbols[i];
        }

        // 4. Add start state transitions (0 → first())
        auto [first_set, first_count] = first_positions<Pattern>(0);
        for (size_t i = 0; i < first_count; ++i) {
            if (states[0].successor_count < MAX_SUCCESSORS) {
                states[0].successors[states[0].successor_count++] = first_set[i];
            }
        }

        // 5. Copy follow transitions
        auto follow_map = compute_follow<Pattern>();
        for (size_t i = 1; i < state_count; ++i) {
            for (size_t j = 0; j < follow_map.successor_counts[i]; ++j) {
                size_t succ = follow_map.successors[i][j];
                if (states[i].successor_count < MAX_SUCCESSORS) {
                    states[i].successors[states[i].successor_count++] = succ;
                }
            }
        }

        // 6. Mark accept states (from last())
        auto [last_set, last_count] = last_positions<Pattern>(0);
        accept_count = last_count;
        for (size_t i = 0; i < last_count; ++i) {
            accept_states[i] = last_set[i];
        }

        // 7. If pattern is nullable, start is also accept
        if constexpr (nullable<Pattern>()) {
            if (accept_count < MAX_SUCCESSORS) {
                accept_states[accept_count++] = 0;
            }
        }
    }
};

} // namespace glushkov
} // namespace ctre

// =============================================================================
// COMPILE-TIME TESTS (Safe - no runtime, no integration)
// =============================================================================

// TEST SUITE 1: Position Counting
namespace test_position_counting {
    using namespace ctre::glushkov;

    // Test 1.1: Empty pattern
    using Pat_Empty = ctre::empty;
    static_assert(count_positions<Pat_Empty>() == 0, "Empty should have 0 positions");

    // Test 1.2: Single character
    using Pat_Char = ctre::character<'a'>;
    static_assert(count_positions<Pat_Char>() == 1, "Character should have 1 position");

    // Test 1.3: Any character (.)
    using Pat_Any = ctre::any;
    static_assert(count_positions<Pat_Any>() == 1, "Any should have 1 position");

    // Test 1.4: String "foo" (3 chars)
    using Pat_String = ctre::string<'f', 'o', 'o'>;
    static_assert(count_positions<Pat_String>() == 3, "String 'foo' should have 3 positions");

    // Test 1.5: String "a" (1 char)
    using Pat_String1 = ctre::string<'a'>;
    static_assert(count_positions<Pat_String1>() == 1, "String 'a' should have 1 position");

    // Test 1.6: Sequence "ab" . "cd" (4 positions)
    using Pat_Seq = ctre::sequence<ctre::string<'a', 'b'>, ctre::string<'c', 'd'>>;
    static_assert(count_positions<Pat_Seq>() == 4, "Sequence 'ab.cd' should have 4 positions");

    // Test 1.7: Select "ab" | "cd" (4 positions)
    using Pat_Select = ctre::select<ctre::string<'a', 'b'>, ctre::string<'c', 'd'>>;
    static_assert(count_positions<Pat_Select>() == 4, "Select 'ab|cd' should have 4 positions");

    // Test 1.8: Select "a" | "b" | "c" (3 positions)
    using Pat_Select3 = ctre::select<ctre::character<'a'>, ctre::character<'b'>, ctre::character<'c'>>;
    static_assert(count_positions<Pat_Select3>() == 3, "Select 'a|b|c' should have 3 positions");

    // Test 1.9: Repeat "a*" (1 position - doesn't multiply!)
    using Pat_Star = ctre::star<ctre::character<'a'>>;
    static_assert(count_positions<Pat_Star>() == 1, "Repeat 'a*' should have 1 position");

    // Test 1.10: Repeat "a+" (1 position)
    using Pat_Plus = ctre::plus<ctre::character<'a'>>;
    static_assert(count_positions<Pat_Plus>() == 1, "Repeat 'a+' should have 1 position");

    // Test 1.11: Repeat "a?" (1 position)
    using Pat_Optional = ctre::optional<ctre::character<'a'>>;
    static_assert(count_positions<Pat_Optional>() == 1, "Repeat 'a?' should have 1 position");

    // Test 1.12: Repeat "a{3,5}" (1 position - repeat doesn't multiply)
    using Pat_Range = ctre::repeat<3, 5, ctre::character<'a'>>;
    static_assert(count_positions<Pat_Range>() == 1, "Repeat 'a{3,5}' should have 1 position");

    // Test 1.13: Nested repeat "(ab)*" (2 positions for 'ab')
    using Pat_RepeatSeq = ctre::star<ctre::string<'a', 'b'>>;
    static_assert(count_positions<Pat_RepeatSeq>() == 2, "Repeat '(ab)*' should have 2 positions");

    // Test 1.14: Complex pattern "(abc|def)" (6 positions)
    using Pat_Complex1 = ctre::select<ctre::string<'a', 'b', 'c'>, ctre::string<'d', 'e', 'f'>>;
    static_assert(count_positions<Pat_Complex1>() == 6, "Select '(abc|def)' should have 6 positions");

    // Test 1.15: Complex pattern "(abc|def).*ghi" from Hyperscan paper
    // Should be: 3 (abc) + 3 (def) + 1 (.) + 3 (ghi) = 10 positions
    using Pat_Hyperscan = ctre::sequence<
        ctre::select<ctre::string<'a','b','c'>, ctre::string<'d','e','f'>>,
        ctre::star<ctre::any>,
        ctre::string<'g','h','i'>
    >;
    static_assert(count_positions<Pat_Hyperscan>() == 10,
                  "Pattern '(abc|def).*ghi' should have 10 positions (Hyperscan paper Figure 1)");

    // All tests pass!
    constexpr bool all_tests_pass = true;
}

// TEST SUITE 2: Nullable Detection
namespace test_nullable {
    using namespace ctre::glushkov;

    // Test 2.1: Empty - nullable
    using Pat_Empty = ctre::empty;
    static_assert(nullable<Pat_Empty>(), "Empty should be nullable");

    // Test 2.2: Character - not nullable
    using Pat_Char = ctre::character<'a'>;
    static_assert(!nullable<Pat_Char>(), "Character should NOT be nullable");

    // Test 2.3: Any (.) - not nullable
    using Pat_Any = ctre::any;
    static_assert(!nullable<Pat_Any>(), "Any should NOT be nullable");

    // Test 2.4: String "foo" - not nullable
    using Pat_String = ctre::string<'f', 'o', 'o'>;
    static_assert(!nullable<Pat_String>(), "String 'foo' should NOT be nullable");

    // Test 2.5: Sequence "ab.cd" - not nullable (neither component nullable)
    using Pat_Seq = ctre::sequence<ctre::string<'a', 'b'>, ctre::string<'c', 'd'>>;
    static_assert(!nullable<Pat_Seq>(), "Sequence 'ab.cd' should NOT be nullable");

    // Test 2.6: Select "ab|cd" - not nullable (neither branch nullable)
    using Pat_Select = ctre::select<ctre::string<'a', 'b'>, ctre::string<'c', 'd'>>;
    static_assert(!nullable<Pat_Select>(), "Select 'ab|cd' should NOT be nullable");

    // Test 2.7: Repeat "a*" - nullable (min=0)
    using Pat_Star = ctre::star<ctre::character<'a'>>;
    static_assert(nullable<Pat_Star>(), "Repeat 'a*' SHOULD be nullable");

    // Test 2.8: Repeat "a+" - not nullable (min=1)
    using Pat_Plus = ctre::plus<ctre::character<'a'>>;
    static_assert(!nullable<Pat_Plus>(), "Repeat 'a+' should NOT be nullable");

    // Test 2.9: Repeat "a?" - nullable (min=0)
    using Pat_Optional = ctre::optional<ctre::character<'a'>>;
    static_assert(nullable<Pat_Optional>(), "Repeat 'a?' SHOULD be nullable");

    // Test 2.10: Repeat "a{3,5}" - not nullable (min=3)
    using Pat_Range = ctre::repeat<3, 5, ctre::character<'a'>>;
    static_assert(!nullable<Pat_Range>(), "Repeat 'a{3,5}' should NOT be nullable");

    // Test 2.11: Repeat "a{0,5}" - nullable (min=0)
    using Pat_Range0 = ctre::repeat<0, 5, ctre::character<'a'>>;
    static_assert(nullable<Pat_Range0>(), "Repeat 'a{0,5}' SHOULD be nullable");

    // Test 2.12: Nested "(a*)" - nullable
    using Pat_NestedStar = ctre::star<ctre::star<ctre::character<'a'>>>;
    static_assert(nullable<Pat_NestedStar>(), "Nested '(a*)' SHOULD be nullable");

    // Test 2.13: Sequence with nullable component "a*.b" - not nullable (b required)
    using Pat_SeqWithStar = ctre::sequence<ctre::star<ctre::character<'a'>>, ctre::character<'b'>>;
    static_assert(!nullable<Pat_SeqWithStar>(), "Sequence 'a*.b' should NOT be nullable");

    // Test 2.14: Sequence all nullable "a*.b*" - nullable (both optional)
    using Pat_SeqAllNull = ctre::sequence<ctre::star<ctre::character<'a'>>, ctre::star<ctre::character<'b'>>>;
    static_assert(nullable<Pat_SeqAllNull>(), "Sequence 'a*.b*' SHOULD be nullable");

    // Test 2.15: Select with one nullable "ab|c*" - nullable (c* branch)
    using Pat_SelectNull = ctre::select<ctre::string<'a','b'>, ctre::star<ctre::character<'c'>>>;
    static_assert(nullable<Pat_SelectNull>(), "Select 'ab|c*' SHOULD be nullable");

    // Test 2.16: Complex "(abc|def)*" - nullable
    using Pat_ComplexStar = ctre::star<ctre::select<ctre::string<'a','b','c'>, ctre::string<'d','e','f'>>>;
    static_assert(nullable<Pat_ComplexStar>(), "Complex '(abc|def)*' SHOULD be nullable");

    // Test 2.17: Hyperscan pattern "(abc|def).*ghi" - not nullable (ghi required)
    using Pat_Hyperscan = ctre::sequence<
        ctre::select<ctre::string<'a','b','c'>, ctre::string<'d','e','f'>>,
        ctre::star<ctre::any>,
        ctre::string<'g','h','i'>
    >;
    static_assert(!nullable<Pat_Hyperscan>(), "Hyperscan '(abc|def).*ghi' should NOT be nullable");

    // All tests pass!
    constexpr bool all_tests_pass = true;
}

// TEST SUITE 3: First() Sets
namespace test_first {
    using namespace ctre::glushkov;

    // Test 3.1: Character 'a' at position 0 → first = {1}
    using Pat_Char = ctre::character<'a'>;
    constexpr auto first1 = first_positions<Pat_Char>(0);
    static_assert(first1.second == 1 && first1.first[0] == 1, "Character 'a' first = {1}");

    // Test 3.2: String "abc" → first = {1}
    using Pat_String = ctre::string<'a', 'b', 'c'>;
    constexpr auto first2 = first_positions<Pat_String>(0);
    static_assert(first2.second == 1 && first2.first[0] == 1, "String 'abc' first = {1}");

    // Test 3.3: Select "ab|cd" → first = {1, 3}
    // Positions: a=1, b=2, c=3, d=4
    using Pat_Select = ctre::select<ctre::string<'a', 'b'>, ctre::string<'c', 'd'>>;
    constexpr auto first3 = first_positions<Pat_Select>(0);
    static_assert(first3.second == 2, "Select 'ab|cd' has 2 starting positions");
    static_assert(first3.first[0] == 1 && first3.first[1] == 3, "Select 'ab|cd' first = {1, 3}");

    // Test 3.4: Sequence "ab.cd" → first = {1}
    // Not nullable, so only first of "ab" matters
    using Pat_Seq = ctre::sequence<ctre::string<'a', 'b'>, ctre::string<'c', 'd'>>;
    constexpr auto first4 = first_positions<Pat_Seq>(0);
    static_assert(first4.second == 1 && first4.first[0] == 1, "Sequence 'ab.cd' first = {1}");

    // Test 3.5: Sequence with nullable "a*.b" → first = {1, 2}
    // a* is nullable, so b can also start
    // Positions: a=1, b=2
    using Pat_SeqNull = ctre::sequence<ctre::star<ctre::character<'a'>>, ctre::character<'b'>>;
    constexpr auto first5 = first_positions<Pat_SeqNull>(0);
    static_assert(first5.second == 2, "Sequence 'a*.b' has 2 starting positions");
    static_assert(first5.first[0] == 1 && first5.first[1] == 2, "Sequence 'a*.b' first = {1, 2}");

    // Test 3.6: Repeat "a*" → first = {1}
    using Pat_Star = ctre::star<ctre::character<'a'>>;
    constexpr auto first6 = first_positions<Pat_Star>(0);
    static_assert(first6.second == 1 && first6.first[0] == 1, "Repeat 'a*' first = {1}");

    // Test 3.7: Empty → first = {}
    using Pat_Empty = ctre::empty;
    constexpr auto first7 = first_positions<Pat_Empty>(0);
    static_assert(first7.second == 0, "Empty first = {}");

    // Test 3.8: Select with 3 branches "a|b|c" → first = {1, 2, 3}
    using Pat_Select3 = ctre::select<ctre::character<'a'>, ctre::character<'b'>, ctre::character<'c'>>;
    constexpr auto first8 = first_positions<Pat_Select3>(0);
    static_assert(first8.second == 3, "Select 'a|b|c' has 3 starting positions");
    static_assert(first8.first[0] == 1 && first8.first[1] == 2 && first8.first[2] == 3,
                  "Select 'a|b|c' first = {1, 2, 3}");

    // Test 3.9: Hyperscan paper "(abc|def).*ghi" → first = {1, 4}
    // Positions: a=1, b=2, c=3, d=4, e=5, f=6, .=7, g=8, h=9, i=10
    // Select branches: abc starts at 1, def starts at 4
    using Pat_Hyperscan = ctre::sequence<
        ctre::select<ctre::string<'a','b','c'>, ctre::string<'d','e','f'>>,
        ctre::star<ctre::any>,
        ctre::string<'g','h','i'>
    >;
    constexpr auto first9 = first_positions<Pat_Hyperscan>(0);
    static_assert(first9.second == 2, "Hyperscan pattern has 2 starting positions");
    static_assert(first9.first[0] == 1 && first9.first[1] == 4,
                  "Hyperscan '(abc|def).*ghi' first = {1, 4}");

    // Test 3.10: Sequence all nullable "a*.b*.c" → first = {1, 2, 3}
    // All nullable, so all can start
    using Pat_AllNull = ctre::sequence<
        ctre::star<ctre::character<'a'>>,
        ctre::star<ctre::character<'b'>>,
        ctre::character<'c'>
    >;
    constexpr auto first10 = first_positions<Pat_AllNull>(0);
    static_assert(first10.second == 3, "Sequence 'a*.b*.c' has 3 starting positions");
    static_assert(first10.first[0] == 1 && first10.first[1] == 2 && first10.first[2] == 3,
                  "Sequence 'a*.b*.c' first = {1, 2, 3}");

    // All tests pass!
    constexpr bool all_tests_pass = true;
}

// TEST SUITE 4: Last() Sets
namespace test_last {
    using namespace ctre::glushkov;

    // Test 4.1: Character 'a' at position 0 → last = {1}
    using Pat_Char = ctre::character<'a'>;
    constexpr auto last1 = last_positions<Pat_Char>(0);
    static_assert(last1.second == 1 && last1.first[0] == 1, "Character 'a' last = {1}");

    // Test 4.2: String "abc" → last = {3}
    using Pat_String = ctre::string<'a', 'b', 'c'>;
    constexpr auto last2 = last_positions<Pat_String>(0);
    static_assert(last2.second == 1 && last2.first[0] == 3, "String 'abc' last = {3}");

    // Test 4.3: Select "ab|cd" → last = {2, 4}
    // Positions: a=1, b=2, c=3, d=4
    using Pat_Select = ctre::select<ctre::string<'a', 'b'>, ctre::string<'c', 'd'>>;
    constexpr auto last3 = last_positions<Pat_Select>(0);
    static_assert(last3.second == 2, "Select 'ab|cd' has 2 ending positions");
    static_assert(last3.first[0] == 2 && last3.first[1] == 4, "Select 'ab|cd' last = {2, 4}");

    // Test 4.4: Sequence "ab.cd" → last = {4}
    // Not nullable, so only last of "cd" matters
    using Pat_Seq = ctre::sequence<ctre::string<'a', 'b'>, ctre::string<'c', 'd'>>;
    constexpr auto last4 = last_positions<Pat_Seq>(0);
    static_assert(last4.second == 1 && last4.first[0] == 4, "Sequence 'ab.cd' last = {4}");

    // Test 4.5: Sequence with nullable "a.b*" → last = {1, 2}
    // b* is nullable, so a can also end
    // Positions: a=1, b=2
    using Pat_SeqNull = ctre::sequence<ctre::character<'a'>, ctre::star<ctre::character<'b'>>>;
    constexpr auto last5 = last_positions<Pat_SeqNull>(0);
    static_assert(last5.second == 2, "Sequence 'a.b*' has 2 ending positions");
    static_assert(last5.first[0] == 2 && last5.first[1] == 1, "Sequence 'a.b*' last = {2, 1}");

    // Test 4.6: Repeat "a*" → last = {1}
    using Pat_Star = ctre::star<ctre::character<'a'>>;
    constexpr auto last6 = last_positions<Pat_Star>(0);
    static_assert(last6.second == 1 && last6.first[0] == 1, "Repeat 'a*' last = {1}");

    // Test 4.7: Empty → last = {}
    using Pat_Empty = ctre::empty;
    constexpr auto last7 = last_positions<Pat_Empty>(0);
    static_assert(last7.second == 0, "Empty last = {}");

    // Test 4.8: Select with 3 branches "a|b|c" → last = {1, 2, 3}
    using Pat_Select3 = ctre::select<ctre::character<'a'>, ctre::character<'b'>, ctre::character<'c'>>;
    constexpr auto last8 = last_positions<Pat_Select3>(0);
    static_assert(last8.second == 3, "Select 'a|b|c' has 3 ending positions");
    static_assert(last8.first[0] == 1 && last8.first[1] == 2 && last8.first[2] == 3,
                  "Select 'a|b|c' last = {1, 2, 3}");

    // Test 4.9: Hyperscan paper "(abc|def).*ghi" → last = {10}
    // Positions: a=1, b=2, c=3, d=4, e=5, f=6, .=7, g=8, h=9, i=10
    // Only 'i' can end
    using Pat_Hyperscan = ctre::sequence<
        ctre::select<ctre::string<'a','b','c'>, ctre::string<'d','e','f'>>,
        ctre::star<ctre::any>,
        ctre::string<'g','h','i'>
    >;
    constexpr auto last9 = last_positions<Pat_Hyperscan>(0);
    static_assert(last9.second == 1, "Hyperscan pattern has 1 ending position");
    static_assert(last9.first[0] == 10,
                  "Hyperscan '(abc|def).*ghi' last = {10}");

    // Test 4.10: Sequence all nullable "a*.b*.c*" → last = {3, 2, 1}
    // All nullable, so all can end (working backwards)
    using Pat_AllNull = ctre::sequence<
        ctre::star<ctre::character<'a'>>,
        ctre::star<ctre::character<'b'>>,
        ctre::star<ctre::character<'c'>>
    >;
    constexpr auto last10 = last_positions<Pat_AllNull>(0);
    static_assert(last10.second == 3, "Sequence 'a*.b*.c*' has 3 ending positions");
    // Order: c*=3 (last), then b*=2, then a*=1
    static_assert(last10.first[0] == 3 && last10.first[1] == 2 && last10.first[2] == 1,
                  "Sequence 'a*.b*.c*' last = {3, 2, 1}");

    // All tests pass!
    constexpr bool all_tests_pass = true;
}

// TEST SUITE 5: Follow() Transitions
namespace test_follow {
    using namespace ctre::glushkov;

    // Test 5.1: String "abc" → follow(1)={2}, follow(2)={3}, follow(3)={}
    using Pat_String = ctre::string<'a', 'b', 'c'>;
    constexpr auto fmap1 = compute_follow<Pat_String>();
    static_assert(fmap1.successor_counts[1] == 1 && fmap1.successors[1][0] == 2, "follow(1) = {2}");
    static_assert(fmap1.successor_counts[2] == 1 && fmap1.successors[2][0] == 3, "follow(2) = {3}");
    static_assert(fmap1.successor_counts[3] == 0, "follow(3) = {}");

    // Test 5.2: Sequence "ab.cd" → follow(2)={3} (crosses boundary)
    using Pat_Seq = ctre::sequence<ctre::string<'a', 'b'>, ctre::string<'c', 'd'>>;
    constexpr auto fmap2 = compute_follow<Pat_Seq>();
    static_assert(fmap2.successor_counts[1] == 1 && fmap2.successors[1][0] == 2, "follow(1) = {2}");
    static_assert(fmap2.successor_counts[2] == 1 && fmap2.successors[2][0] == 3, "follow(2) = {3} (boundary)");
    static_assert(fmap2.successor_counts[3] == 1 && fmap2.successors[3][0] == 4, "follow(3) = {4}");
    static_assert(fmap2.successor_counts[4] == 0, "follow(4) = {}");

    // Test 5.3: Repeat "a*" → follow(1)={1} (SELF-LOOP!)
    using Pat_Star = ctre::star<ctre::character<'a'>>;
    constexpr auto fmap3 = compute_follow<Pat_Star>();
    static_assert(fmap3.successor_counts[1] == 1 && fmap3.successors[1][0] == 1, "follow(1) = {1} (loop!)");

    // Test 5.4: Select "ab|cd" → separate chains
    using Pat_Select = ctre::select<ctre::string<'a', 'b'>, ctre::string<'c', 'd'>>;
    constexpr auto fmap4 = compute_follow<Pat_Select>();
    static_assert(fmap4.successor_counts[1] == 1 && fmap4.successors[1][0] == 2, "follow(1) = {2}");
    static_assert(fmap4.successor_counts[2] == 0, "follow(2) = {} (end of branch 1)");
    static_assert(fmap4.successor_counts[3] == 1 && fmap4.successors[3][0] == 4, "follow(3) = {4}");
    static_assert(fmap4.successor_counts[4] == 0, "follow(4) = {} (end of branch 2)");

    // Test 5.5: Repeat sequence "(ab)*" → follow(2)={1} (loops back!)
    using Pat_RepeatSeq = ctre::star<ctre::string<'a', 'b'>>;
    constexpr auto fmap5 = compute_follow<Pat_RepeatSeq>();
    static_assert(fmap5.successor_counts[1] == 1 && fmap5.successors[1][0] == 2, "follow(1) = {2}");
    static_assert(fmap5.successor_counts[2] == 1 && fmap5.successors[2][0] == 1, "follow(2) = {1} (loop!)");

    // Test 5.6: Sequence with nullable "a*.b" → follow(1) includes b
    using Pat_SeqNull = ctre::sequence<ctre::star<ctre::character<'a'>>, ctre::character<'b'>>;
    constexpr auto fmap6 = compute_follow<Pat_SeqNull>();
    static_assert(fmap6.successor_counts[1] == 2, "follow(1) has 2 successors");
    // Position 1 (a) can loop to itself OR go to position 2 (b)
    static_assert((fmap6.successors[1][0] == 1 && fmap6.successors[1][1] == 2) ||
                  (fmap6.successors[1][0] == 2 && fmap6.successors[1][1] == 1),
                  "follow(1) = {1, 2}");
    static_assert(fmap6.successor_counts[2] == 0, "follow(2) = {}");

    // Test 5.7: Complex nested "((a)*b)*"
    using Pat_Nested = ctre::star<ctre::sequence<ctre::star<ctre::character<'a'>>, ctre::character<'b'>>>;
    constexpr auto fmap7 = compute_follow<Pat_Nested>();
    // a (pos 1) can loop to itself AND go to b
    // b (pos 2) can loop back to a (outer loop)
    static_assert(fmap7.successor_counts[1] == 2, "follow(1) has multiple successors");
    static_assert(fmap7.successor_counts[2] >= 1, "follow(2) has at least outer loop");
    // Verify 2 → 1 edge exists (outer loop) - check first successor
    static_assert(fmap7.successors[2][0] == 1 ||
                  (fmap7.successor_counts[2] > 1 && fmap7.successors[2][1] == 1),
                  "follow(2) must include edge to 1");

    // Test 5.8: Hyperscan paper "(abc|def).*ghi"
    // Expected transitions (from paper Figure 1):
    // Positions: a=1, b=2, c=3, d=4, e=5, f=6, .=7, g=8, h=9, i=10
    // follow(2)={3}, follow(3)={7}, follow(5)={6}, follow(6)={7}
    // follow(7)={7,8} (. loops + goes to g), follow(8)={9}, follow(9)={10}
    using Pat_Hyperscan = ctre::sequence<
        ctre::select<ctre::string<'a','b','c'>, ctre::string<'d','e','f'>>,
        ctre::star<ctre::any>,
        ctre::string<'g','h','i'>
    >;
    constexpr auto fmap8 = compute_follow<Pat_Hyperscan>();

    // Branch 1: abc chain
    static_assert(fmap8.successor_counts[1] == 1 && fmap8.successors[1][0] == 2, "follow(1) = {2}");
    static_assert(fmap8.successor_counts[2] == 1 && fmap8.successors[2][0] == 3, "follow(2) = {3}");
    static_assert(fmap8.successor_counts[3] == 1 && fmap8.successors[3][0] == 7, "follow(3) = {7} (to .)");

    // Branch 2: def chain
    static_assert(fmap8.successor_counts[4] == 1 && fmap8.successors[4][0] == 5, "follow(4) = {5}");
    static_assert(fmap8.successor_counts[5] == 1 && fmap8.successors[5][0] == 6, "follow(5) = {6}");
    static_assert(fmap8.successor_counts[6] == 1 && fmap8.successors[6][0] == 7, "follow(6) = {7} (to .)");

    // .* (position 7): loops to self AND goes to g
    static_assert(fmap8.successor_counts[7] == 2, "follow(7) has 2 successors");
    static_assert((fmap8.successors[7][0] == 7 && fmap8.successors[7][1] == 8) ||
                  (fmap8.successors[7][0] == 8 && fmap8.successors[7][1] == 7),
                  "follow(7) = {7, 8} (loop + to g)");

    // ghi chain
    static_assert(fmap8.successor_counts[8] == 1 && fmap8.successors[8][0] == 9, "follow(8) = {9}");
    static_assert(fmap8.successor_counts[9] == 1 && fmap8.successors[9][0] == 10, "follow(9) = {10}");
    static_assert(fmap8.successor_counts[10] == 0, "follow(10) = {} (accept state)");

    // All tests pass!
    constexpr bool all_tests_pass = true;
}

// TEST SUITE 6: Complete NFA Construction
namespace test_nfa {
    using namespace ctre::glushkov;

    // Test 6.1: String "abc"
    using Pat_String = ctre::string<'a', 'b', 'c'>;
    constexpr auto nfa1 = glushkov_nfa<Pat_String>();
    static_assert(nfa1.state_count == 4, "NFA has 4 states (0 + 3 positions)");
    static_assert(nfa1.states[1].symbol == 'a', "Position 1 = 'a'");
    static_assert(nfa1.states[2].symbol == 'b', "Position 2 = 'b'");
    static_assert(nfa1.states[3].symbol == 'c', "Position 3 = 'c'");
    static_assert(nfa1.accept_count == 1 && nfa1.accept_states[0] == 3, "Accept state = {3}");

    // Test 6.2: Repeat "a*"
    using Pat_Star = ctre::star<ctre::character<'a'>>;
    constexpr auto nfa2 = glushkov_nfa<Pat_Star>();
    static_assert(nfa2.state_count == 2, "NFA has 2 states");
    static_assert(nfa2.states[1].symbol == 'a', "Position 1 = 'a'");
    // Start should accept (nullable)
    static_assert(nfa2.accept_count == 2, "Two accept states (1 + start)");

    // Test 6.3: Sequence "ab.cd"
    using Pat_Seq = ctre::sequence<ctre::string<'a', 'b'>, ctre::string<'c', 'd'>>;
    constexpr auto nfa3 = glushkov_nfa<Pat_Seq>();
    static_assert(nfa3.state_count == 5, "NFA has 5 states");
    static_assert(nfa3.states[1].symbol == 'a', "Position 1 = 'a'");
    static_assert(nfa3.states[2].symbol == 'b', "Position 2 = 'b'");
    static_assert(nfa3.states[3].symbol == 'c', "Position 3 = 'c'");
    static_assert(nfa3.states[4].symbol == 'd', "Position 4 = 'd'");

    // Test 6.4: Select "ab|cd"
    using Pat_Select = ctre::select<ctre::string<'a', 'b'>, ctre::string<'c', 'd'>>;
    constexpr auto nfa4 = glushkov_nfa<Pat_Select>();
    static_assert(nfa4.state_count == 5, "NFA has 5 states");
    // Start transitions should go to {1, 3}
    static_assert(nfa4.states[0].successor_count == 2, "Start has 2 successors");

    // Test 6.5: Hyperscan paper "(abc|def).*ghi"
    // This is the BIG test - verify complete structure!
    using Pat_Hyperscan = ctre::sequence<
        ctre::select<ctre::string<'a','b','c'>, ctre::string<'d','e','f'>>,
        ctre::star<ctre::any>,
        ctre::string<'g','h','i'>
    >;
    constexpr auto nfa5 = glushkov_nfa<Pat_Hyperscan>();

    // Verify state count: 0 (start) + 10 positions = 11 states
    static_assert(nfa5.state_count == 11, "NFA has 11 states");

    // Verify symbols match expected positions
    static_assert(nfa5.states[1].symbol == 'a', "Position 1 = 'a'");
    static_assert(nfa5.states[2].symbol == 'b', "Position 2 = 'b'");
    static_assert(nfa5.states[3].symbol == 'c', "Position 3 = 'c'");
    static_assert(nfa5.states[4].symbol == 'd', "Position 4 = 'd'");
    static_assert(nfa5.states[5].symbol == 'e', "Position 5 = 'e'");
    static_assert(nfa5.states[6].symbol == 'f', "Position 6 = 'f'");
    static_assert(nfa5.states[7].symbol == '.', "Position 7 = '.' (any)");
    static_assert(nfa5.states[8].symbol == 'g', "Position 8 = 'g'");
    static_assert(nfa5.states[9].symbol == 'h', "Position 9 = 'h'");
    static_assert(nfa5.states[10].symbol == 'i', "Position 10 = 'i'");

    // Verify start transitions: 0 → {1, 4} (first of select)
    static_assert(nfa5.states[0].successor_count == 2, "Start has 2 successors");
    static_assert((nfa5.states[0].successors[0] == 1 && nfa5.states[0].successors[1] == 4) ||
                  (nfa5.states[0].successors[0] == 4 && nfa5.states[0].successors[1] == 1),
                  "Start → {1, 4}");

    // Verify key transitions from paper
    // Position 3 (c) → 7 (.)
    static_assert(nfa5.states[3].successor_count == 1 && nfa5.states[3].successors[0] == 7,
                  "follow(3) = {7}");

    // Position 6 (f) → 7 (.)
    static_assert(nfa5.states[6].successor_count == 1 && nfa5.states[6].successors[0] == 7,
                  "follow(6) = {7}");

    // Position 7 (.) → {7, 8} (self-loop + to g)
    static_assert(nfa5.states[7].successor_count == 2, "follow(7) has 2 successors");
    static_assert((nfa5.states[7].successors[0] == 7 && nfa5.states[7].successors[1] == 8) ||
                  (nfa5.states[7].successors[0] == 8 && nfa5.states[7].successors[1] == 7),
                  "follow(7) = {7, 8}");

    // Verify accept state: only position 10 (i)
    static_assert(nfa5.accept_count == 1, "One accept state");
    static_assert(nfa5.accept_states[0] == 10, "Accept = {10}");

    // Test 6.6: Nullable pattern "a*"
    // Should have start as accept state too
    using Pat_Nullable = ctre::star<ctre::character<'a'>>;
    constexpr auto nfa6 = glushkov_nfa<Pat_Nullable>();
    static_assert(nfa6.accept_count == 2, "Nullable: 2 accept states");
    // One should be position 1, one should be start (0)
    static_assert((nfa6.accept_states[0] == 1 && nfa6.accept_states[1] == 0) ||
                  (nfa6.accept_states[0] == 0 && nfa6.accept_states[1] == 1),
                  "Accept = {0, 1} for nullable");

    // All tests pass!
    constexpr bool all_tests_pass = true;
}

int main() {
    std::cout << "=== Glushkov NFA Construction - Phase 1 Tests ===\n\n";

    // TEST 1: Position counting
    {
        std::cout << "TEST 1: Position counting ✅\n";
        std::cout << "  ✓ Empty: 0 positions\n";
        std::cout << "  ✓ Character 'a': 1 position\n";
        std::cout << "  ✓ Any '.': 1 position\n";
        std::cout << "  ✓ String 'foo': 3 positions\n";
        std::cout << "  ✓ Sequence 'ab.cd': 4 positions\n";
        std::cout << "  ✓ Select 'ab|cd': 4 positions\n";
        std::cout << "  ✓ Repeat 'a*': 1 position\n";
        std::cout << "  ✓ Repeat 'a+': 1 position\n";
        std::cout << "  ✓ Repeat 'a?': 1 position\n";
        std::cout << "  ✓ Repeat '(ab)*': 2 positions\n";
        std::cout << "  ✓ Hyperscan paper '(abc|def).*ghi': 10 positions ✨\n";
        std::cout << "  All " << 15 << " tests PASSED at compile-time!\n\n";
    }

    // TEST 2: Nullable detection
    {
        std::cout << "TEST 2: Nullable detection ✅\n";
        std::cout << "  ✓ Empty: nullable\n";
        std::cout << "  ✓ Character 'a': NOT nullable\n";
        std::cout << "  ✓ Any '.': NOT nullable\n";
        std::cout << "  ✓ String 'foo': NOT nullable\n";
        std::cout << "  ✓ Sequence 'ab.cd': NOT nullable\n";
        std::cout << "  ✓ Repeat 'a*': nullable\n";
        std::cout << "  ✓ Repeat 'a+': NOT nullable\n";
        std::cout << "  ✓ Repeat 'a?': nullable\n";
        std::cout << "  ✓ Sequence 'a*.b*': nullable\n";
        std::cout << "  ✓ Select 'ab|c*': nullable\n";
        std::cout << "  ✓ Hyperscan '(abc|def).*ghi': NOT nullable ✨\n";
        std::cout << "  All " << 17 << " tests PASSED at compile-time!\n\n";
    }

    // TEST 3: First() sets
    {
        std::cout << "TEST 3: First() sets ✅\n";
        std::cout << "  ✓ Character 'a': first = {1}\n";
        std::cout << "  ✓ String 'abc': first = {1}\n";
        std::cout << "  ✓ Select 'ab|cd': first = {1, 3}\n";
        std::cout << "  ✓ Sequence 'ab.cd': first = {1}\n";
        std::cout << "  ✓ Sequence 'a*.b': first = {1, 2}\n";
        std::cout << "  ✓ Repeat 'a*': first = {1}\n";
        std::cout << "  ✓ Empty: first = {}\n";
        std::cout << "  ✓ Select 'a|b|c': first = {1, 2, 3}\n";
        std::cout << "  ✓ Hyperscan '(abc|def).*ghi': first = {1, 4} ✨\n";
        std::cout << "  ✓ Sequence 'a*.b*.c': first = {1, 2, 3}\n";
        std::cout << "  All " << 10 << " tests PASSED at compile-time!\n\n";
    }

    // TEST 4: Last() sets
    {
        std::cout << "TEST 4: Last() sets ✅\n";
        std::cout << "  ✓ Character 'a': last = {1}\n";
        std::cout << "  ✓ String 'abc': last = {3}\n";
        std::cout << "  ✓ Select 'ab|cd': last = {2, 4}\n";
        std::cout << "  ✓ Sequence 'ab.cd': last = {4}\n";
        std::cout << "  ✓ Sequence 'a.b*': last = {1, 2}\n";
        std::cout << "  ✓ Repeat 'a*': last = {1}\n";
        std::cout << "  ✓ Empty: last = {}\n";
        std::cout << "  ✓ Select 'a|b|c': last = {1, 2, 3}\n";
        std::cout << "  ✓ Hyperscan '(abc|def).*ghi': last = {10} ✨\n";
        std::cout << "  ✓ Sequence 'a*.b*.c*': last = {3, 2, 1}\n";
        std::cout << "  All " << 10 << " tests PASSED at compile-time!\n\n";
    }

    // TEST 5: Follow transitions
    {
        std::cout << "TEST 5: Follow() transitions ✅\n";
        std::cout << "  ✓ String 'abc': follow(1)={2}, follow(2)={3}, follow(3)={}\n";
        std::cout << "  ✓ Sequence 'ab.cd': follow(2)={3} (crosses boundary)\n";
        std::cout << "  ✓ Repeat 'a*': follow(1)={1} (SELF-LOOP!)\n";
        std::cout << "  ✓ Select 'ab|cd': separate chains\n";
        std::cout << "  ✓ Repeat '(ab)*': follow(2)={1} (loops back!)\n";
        std::cout << "  ✓ Sequence 'a*.b': follow(1)={1,2}\n";
        std::cout << "  ✓ Nested '((a)*b)*': complex loops\n";
        std::cout << "  ✓ Hyperscan '(abc|def).*ghi': ALL transitions verified! ✨\n";
        std::cout << "     - follow(3)={7}, follow(6)={7}\n";
        std::cout << "     - follow(7)={7,8} (. loops + goes to g)\n";
        std::cout << "     - follow(8)={9}, follow(9)={10}\n";
        std::cout << "  All " << 8 << " tests PASSED at compile-time!\n\n";
    }

    // TEST 6: Complete NFA construction
    {
        std::cout << "TEST 6: Complete NFA Construction ✅\n";
        std::cout << "  ✓ String 'abc': 4 states, symbols assigned\n";
        std::cout << "  ✓ Repeat 'a*': nullable, start is accept\n";
        std::cout << "  ✓ Sequence 'ab.cd': boundary transitions\n";
        std::cout << "  ✓ Select 'ab|cd': start → {1, 3}\n";
        std::cout << "  ✓ Hyperscan '(abc|def).*ghi': COMPLETE NFA! ✨\n";
        std::cout << "     - 11 states (0 + 10 positions)\n";
        std::cout << "     - All symbols correct (a,b,c,d,e,f,.,g,h,i)\n";
        std::cout << "     - Start → {1, 4}\n";
        std::cout << "     - Accept = {10}\n";
        std::cout << "     - All transitions verified\n";
        std::cout << "  ✓ Nullable pattern: start in accept set\n";
        std::cout << "  All " << 6 << " NFA tests PASSED at compile-time!\n\n";
    }

    std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║      🎉 PHASE 1 COMPLETE - GLUSHKOV NFA SUCCESS! 🎉     ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n\n";

    std::cout << "✅ Position counting:     15 tests\n";
    std::cout << "✅ Nullable detection:    17 tests\n";
    std::cout << "✅ First() sets:          10 tests\n";
    std::cout << "✅ Last() sets:           10 tests\n";
    std::cout << "✅ Follow() transitions:   8 tests\n";
    std::cout << "✅ Complete NFA:           6 tests\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "✨ TOTAL: 66 tests, ALL PASSING! ✨\n\n";

    std::cout << "🎯 Hyperscan Paper Pattern VERIFIED:\n";
    std::cout << "   Pattern: (abc|def).*ghi\n";
    std::cout << "   ✓ 10 positions\n";
    std::cout << "   ✓ NOT nullable\n";
    std::cout << "   ✓ First = {1, 4}\n";
    std::cout << "   ✓ Last = {10}\n";
    std::cout << "   ✓ All follow() transitions\n";
    std::cout << "   ✓ Complete NFA structure\n";
    std::cout << "   ✨ MATCHES PAPER FIGURE 1 EXACTLY! ✨\n\n";

    std::cout << "📊 Implementation Stats:\n";
    std::cout << "   - Lines of code: ~1,100\n";
    std::cout << "   - Compile-time tests: 66\n";
    std::cout << "   - Runtime overhead: ZERO\n";
    std::cout << "   - Integration risk: ZERO (isolated)\n\n";

    std::cout << "🚀 Next Phase: Dominator Analysis\n";
    std::cout << "   Use this NFA to find required literals!\n\n";

    return 0;
}
