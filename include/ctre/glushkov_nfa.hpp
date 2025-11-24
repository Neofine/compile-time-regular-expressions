#ifndef CTRE__GLUSHKOV_NFA__HPP
#define CTRE__GLUSHKOV_NFA__HPP

// Glushkov NFA Construction
// Converts CTRE's type-based regex AST into position-based NFA
// Used for literal extraction and pattern decomposition

#include "ctll.hpp"
#include "atoms.hpp"
#include "atoms_characters.hpp"
#include <cstddef>
#include <array>
#include <utility>

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

#endif // CTRE__GLUSHKOV_NFA__HPP
