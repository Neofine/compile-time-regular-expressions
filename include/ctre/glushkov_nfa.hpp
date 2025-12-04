#ifndef CTRE__GLUSHKOV_NFA__HPP
#define CTRE__GLUSHKOV_NFA__HPP

// Glushkov NFA Construction - converts CTRE AST to position-based NFA
// Used for literal extraction and pattern decomposition

#include "atoms.hpp"
#include "atoms_characters.hpp"
#include "pattern_traits.hpp"
#include "ctll.hpp"
#include <array>
#include <cstddef>
#include <utility>

namespace ctre::glushkov {

// Import traits from consolidated header
using namespace ctre::traits;

// Position counting (Glushkov semantics)
template <typename Pattern> [[nodiscard]] consteval size_t count_positions() noexcept;
template <typename... Patterns> [[nodiscard]] consteval size_t count_positions_pack() noexcept {
    return (count_positions<Patterns>() + ... + 0);
}

template <typename Pattern>
[[nodiscard]] consteval size_t count_positions() noexcept {
    if constexpr (is_empty_v<Pattern>) return 0;
    else if constexpr (is_character_v<Pattern>) return 1;
    else if constexpr (is_any_v<Pattern>) return 1;
    else if constexpr (is_string_v<Pattern>) return is_string<Pattern>::length;
    else if constexpr (is_sequence_v<Pattern>) {
        return []<typename... Content>(sequence<Content...>*) {
            return count_positions_pack<Content...>();
        }(static_cast<Pattern*>(nullptr));
    }
    else if constexpr (is_select_v<Pattern>) {
        return []<typename... Options>(select<Options...>*) {
            return count_positions_pack<Options...>();
        }(static_cast<Pattern*>(nullptr));
    }
    else if constexpr (is_capture_v<Pattern>) {
        return []<size_t Index, typename... Content>(capture<Index, Content...>*) {
            return count_positions_pack<Content...>();
        }(static_cast<Pattern*>(nullptr));
    }
    else if constexpr (is_any_repeat_v<Pattern>) {
        if constexpr (is_repeat_v<Pattern>) {
            return []<size_t A, size_t B, typename... Content>(repeat<A, B, Content...>*) {
                return count_positions_pack<Content...>();
            }(static_cast<Pattern*>(nullptr));
        } else if constexpr (is_lazy_repeat_v<Pattern>) {
            return []<size_t A, size_t B, typename... Content>(lazy_repeat<A, B, Content...>*) {
                return count_positions_pack<Content...>();
            }(static_cast<Pattern*>(nullptr));
        } else if constexpr (is_possessive_repeat_v<Pattern>) {
            return []<size_t A, size_t B, typename... Content>(possessive_repeat<A, B, Content...>*) {
                return count_positions_pack<Content...>();
            }(static_cast<Pattern*>(nullptr));
        }
    }
    else if constexpr (CharacterLike<Pattern>) return 1;
    else return 0;
}

// Nullable detection - can pattern match empty string?
template <typename Pattern> [[nodiscard]] consteval bool nullable() noexcept;
template <typename... Patterns> [[nodiscard]] consteval bool all_nullable() noexcept {
    if constexpr (sizeof...(Patterns) == 0) return true;
    else return (nullable<Patterns>() && ...);
}
template <typename... Patterns> [[nodiscard]] consteval bool any_nullable() noexcept {
    if constexpr (sizeof...(Patterns) == 0) return false;
    else return (nullable<Patterns>() || ...);
}

template <typename Pattern>
[[nodiscard]] consteval bool nullable() noexcept {
    if constexpr (is_empty_v<Pattern>) return true;
    else if constexpr (is_character_v<Pattern>) return false;
    else if constexpr (is_any_v<Pattern>) return false;
    else if constexpr (is_string_v<Pattern>) return is_string<Pattern>::length == 0;
    else if constexpr (is_sequence_v<Pattern>) {
        return []<typename... Content>(sequence<Content...>*) {
            return all_nullable<Content...>();
        }(static_cast<Pattern*>(nullptr));
    }
    else if constexpr (is_select_v<Pattern>) {
        return []<typename... Options>(select<Options...>*) {
            return any_nullable<Options...>();
        }(static_cast<Pattern*>(nullptr));
    }
    else if constexpr (is_capture_v<Pattern>) {
        return []<size_t Index, typename... Content>(capture<Index, Content...>*) {
            return all_nullable<Content...>();
        }(static_cast<Pattern*>(nullptr));
    }
    else if constexpr (is_any_repeat_v<Pattern>) {
        if constexpr (is_repeat_v<Pattern>) {
            return []<size_t A, size_t B, typename... Content>(repeat<A, B, Content...>*) {
                return (A == 0) || all_nullable<Content...>();
            }(static_cast<Pattern*>(nullptr));
        } else if constexpr (is_lazy_repeat_v<Pattern>) {
            return []<size_t A, size_t B, typename... Content>(lazy_repeat<A, B, Content...>*) {
                return (A == 0) || all_nullable<Content...>();
            }(static_cast<Pattern*>(nullptr));
        } else if constexpr (is_possessive_repeat_v<Pattern>) {
            return []<size_t A, size_t B, typename... Content>(possessive_repeat<A, B, Content...>*) {
                return (A == 0) || all_nullable<Content...>();
            }(static_cast<Pattern*>(nullptr));
        }
    }
    else if constexpr (CharacterLike<Pattern>) return false;
    else return false;
}

// Position set merging helper
template <size_t N1, size_t N2>
[[nodiscard]] constexpr auto merge_position_sets(const std::array<size_t, N1>& set1, size_t count1,
                                                 const std::array<size_t, N2>& set2, size_t count2) {
    constexpr size_t max_size = N1 + N2;
    std::array<size_t, max_size> result{};
    size_t result_count = 0;
    for (size_t i = 0; i < count1 && i < N1; ++i) result[result_count++] = set1[i];
    for (size_t i = 0; i < count2 && i < N2; ++i) result[result_count++] = set2[i];
    return std::pair{result, result_count};
}

// First() sets - which positions can START a match?
template <typename Pattern> [[nodiscard]] constexpr auto first_positions(size_t offset);
template <typename... Content> [[nodiscard]] constexpr auto first_pack(size_t offset);
template <typename... Content> [[nodiscard]] constexpr auto first_sequence(size_t offset);
template <typename... Options> [[nodiscard]] constexpr auto first_select(size_t offset);

template <typename Pattern>
[[nodiscard]] constexpr auto first_positions(size_t offset) {
    if constexpr (is_empty_v<Pattern>) return std::pair{std::array<size_t, 1>{}, size_t{0}};
    else if constexpr (is_character_v<Pattern>) return std::pair{std::array{offset + 1}, size_t{1}};
    else if constexpr (is_any_v<Pattern>) return std::pair{std::array{offset + 1}, size_t{1}};
    else if constexpr (is_string_v<Pattern>) {
        if constexpr (is_string<Pattern>::length > 0) return std::pair{std::array{offset + 1}, size_t{1}};
        else return std::pair{std::array<size_t, 1>{}, size_t{0}};
    }
    else if constexpr (is_sequence_v<Pattern>) {
        return []<typename... Content>(sequence<Content...>*, size_t off) { return first_sequence<Content...>(off); }(
            static_cast<Pattern*>(nullptr), offset);
    }
    else if constexpr (is_select_v<Pattern>) {
        return []<typename... Options>(select<Options...>*, size_t off) { return first_select<Options...>(off); }(
            static_cast<Pattern*>(nullptr), offset);
    }
    else if constexpr (is_capture_v<Pattern>) {
        return []<size_t Index, typename... Content>(capture<Index, Content...>*, size_t off) {
            return first_pack<Content...>(off);
        }(static_cast<Pattern*>(nullptr), offset);
    }
    else if constexpr (is_any_repeat_v<Pattern>) {
        if constexpr (is_repeat_v<Pattern>) {
            return []<size_t A, size_t B, typename... Content>(repeat<A, B, Content...>*, size_t off) {
                return first_pack<Content...>(off);
            }(static_cast<Pattern*>(nullptr), offset);
        } else if constexpr (is_lazy_repeat_v<Pattern>) {
            return []<size_t A, size_t B, typename... Content>(lazy_repeat<A, B, Content...>*, size_t off) {
                return first_pack<Content...>(off);
            }(static_cast<Pattern*>(nullptr), offset);
        } else if constexpr (is_possessive_repeat_v<Pattern>) {
            return []<size_t A, size_t B, typename... Content>(possessive_repeat<A, B, Content...>*, size_t off) {
                return first_pack<Content...>(off);
            }(static_cast<Pattern*>(nullptr), offset);
        }
    }
    else if constexpr (CharacterLike<Pattern>) return std::pair{std::array{offset + 1}, size_t{1}};
    else return std::pair{std::array<size_t, 1>{}, size_t{0}};
}

template <typename... Content>
[[nodiscard]] constexpr auto first_pack(size_t offset) {
    if constexpr (sizeof...(Content) == 0) return std::pair{std::array<size_t, 1>{}, size_t{0}};
    else if constexpr (sizeof...(Content) == 1)
        return []<typename T>(size_t off) { return first_positions<T>(off); }.template operator()<Content...>(offset);
    else return first_sequence<Content...>(offset);
}

template <typename... Content>
constexpr auto first_sequence(size_t offset) {
    if constexpr (sizeof...(Content) == 0) return std::pair{std::array<size_t, 1>{}, size_t{0}};
    else if constexpr (sizeof...(Content) == 1)
        return []<typename T>(size_t off) { return first_positions<T>(off); }.template operator()<Content...>(offset);
    else {
        return []<typename Head, typename... Tail>(size_t off) {
            auto [head_first, head_count] = first_positions<Head>(off);
            if constexpr (nullable<Head>()) {
                size_t tail_offset = off + count_positions<Head>();
                auto [tail_first, tail_count] = first_sequence<Tail...>(tail_offset);
                return merge_position_sets(head_first, head_count, tail_first, tail_count);
            } else {
                return std::pair{head_first, head_count};
            }
        }.template operator()<Content...>(offset);
    }
}

template <typename... Options>
constexpr auto first_select(size_t offset) {
    if constexpr (sizeof...(Options) == 0) return std::pair{std::array<size_t, 1>{}, size_t{0}};
    else if constexpr (sizeof...(Options) == 1)
        return []<typename T>(size_t off) { return first_positions<T>(off); }.template operator()<Options...>(offset);
    else {
        return []<typename Head, typename... Tail>(size_t off) {
            auto [head_first, head_count] = first_positions<Head>(off);
            size_t tail_offset = off + count_positions<Head>();
            auto [tail_first, tail_count] = first_select<Tail...>(tail_offset);
            return merge_position_sets(head_first, head_count, tail_first, tail_count);
        }.template operator()<Options...>(offset);
    }
}

// Last() sets - which positions can END a match?
template <typename Pattern> [[nodiscard]] constexpr auto last_positions(size_t offset);
template <typename... Content> [[nodiscard]] constexpr auto last_pack(size_t offset);
template <typename... Content> [[nodiscard]] constexpr auto last_sequence(size_t offset);
template <typename Head, typename... Tail> [[nodiscard]] constexpr auto last_sequence_reverse(size_t offset);
template <typename... Options> [[nodiscard]] constexpr auto last_select(size_t offset);

template <typename Pattern>
[[nodiscard]] constexpr auto last_positions(size_t offset) {
    if constexpr (is_empty_v<Pattern>) return std::pair{std::array<size_t, 1>{}, size_t{0}};
    else if constexpr (is_character_v<Pattern>) return std::pair{std::array{offset + 1}, size_t{1}};
    else if constexpr (is_any_v<Pattern>) return std::pair{std::array{offset + 1}, size_t{1}};
    else if constexpr (is_string_v<Pattern>) {
        if constexpr (is_string<Pattern>::length > 0)
            return std::pair{std::array{offset + is_string<Pattern>::length}, size_t{1}};
        else return std::pair{std::array<size_t, 1>{}, size_t{0}};
    }
    else if constexpr (is_sequence_v<Pattern>) {
        return []<typename... Content>(sequence<Content...>*, size_t off) { return last_sequence<Content...>(off); }(
            static_cast<Pattern*>(nullptr), offset);
    }
    else if constexpr (is_select_v<Pattern>) {
        return []<typename... Options>(select<Options...>*, size_t off) { return last_select<Options...>(off); }(
            static_cast<Pattern*>(nullptr), offset);
    }
    else if constexpr (is_capture_v<Pattern>) {
        return []<size_t Index, typename... Content>(capture<Index, Content...>*, size_t off) {
            return last_pack<Content...>(off);
        }(static_cast<Pattern*>(nullptr), offset);
    }
    else if constexpr (is_any_repeat_v<Pattern>) {
        if constexpr (is_repeat_v<Pattern>) {
            return []<size_t A, size_t B, typename... Content>(repeat<A, B, Content...>*, size_t off) {
                return last_pack<Content...>(off);
            }(static_cast<Pattern*>(nullptr), offset);
        } else if constexpr (is_lazy_repeat_v<Pattern>) {
            return []<size_t A, size_t B, typename... Content>(lazy_repeat<A, B, Content...>*, size_t off) {
                return last_pack<Content...>(off);
            }(static_cast<Pattern*>(nullptr), offset);
        } else if constexpr (is_possessive_repeat_v<Pattern>) {
            return []<size_t A, size_t B, typename... Content>(possessive_repeat<A, B, Content...>*, size_t off) {
                return last_pack<Content...>(off);
            }(static_cast<Pattern*>(nullptr), offset);
        }
    }
    else if constexpr (CharacterLike<Pattern>) return std::pair{std::array{offset + 1}, size_t{1}};
    else return std::pair{std::array<size_t, 1>{}, size_t{0}};
}

template <typename... Content>
[[nodiscard]] constexpr auto last_pack(size_t offset) {
    if constexpr (sizeof...(Content) == 0) return std::pair{std::array<size_t, 1>{}, size_t{0}};
    else if constexpr (sizeof...(Content) == 1)
        return []<typename T>(size_t off) { return last_positions<T>(off); }.template operator()<Content...>(offset);
    else return last_sequence<Content...>(offset);
}

template <typename... Content>
constexpr auto last_sequence(size_t offset) {
    if constexpr (sizeof...(Content) == 0) return std::pair{std::array<size_t, 1>{}, size_t{0}};
    else if constexpr (sizeof...(Content) == 1)
        return []<typename T>(size_t off) { return last_positions<T>(off); }.template operator()<Content...>(offset);
    else {
        return []<typename... All>(size_t off) {
            return last_sequence_reverse<All...>(off);
        }.template operator()<Content...>(offset);
    }
}

template <typename Head, typename... Tail>
constexpr auto last_sequence_reverse(size_t offset) {
    if constexpr (sizeof...(Tail) == 0) return last_positions<Head>(offset);
    else {
        size_t head_size = count_positions<Head>();
        auto [tail_last, tail_count] = last_sequence_reverse<Tail...>(offset + head_size);
        using LastType = typename std::tuple_element<sizeof...(Tail) - 1, std::tuple<Tail...>>::type;
        if constexpr (nullable<LastType>()) {
            auto [head_last, head_count] = last_positions<Head>(offset);
            return merge_position_sets(tail_last, tail_count, head_last, head_count);
        } else {
            return std::pair{tail_last, tail_count};
        }
    }
}

template <typename... Options>
constexpr auto last_select(size_t offset) {
    if constexpr (sizeof...(Options) == 0) return std::pair{std::array<size_t, 1>{}, size_t{0}};
    else if constexpr (sizeof...(Options) == 1)
        return []<typename T>(size_t off) { return last_positions<T>(off); }.template operator()<Options...>(offset);
    else {
        return []<typename Head, typename... Tail>(size_t off) {
            auto [head_last, head_count] = last_positions<Head>(off);
            size_t tail_offset = off + count_positions<Head>();
            auto [tail_last, tail_count] = last_select<Tail...>(tail_offset);
            return merge_position_sets(head_last, head_count, tail_last, tail_count);
        }.template operator()<Options...>(offset);
    }
}

// Follow() transitions - builds the complete transition graph
template <size_t MaxPositions>
struct follow_map {
    std::array<std::array<size_t, 32>, MaxPositions> successors{};
    std::array<size_t, MaxPositions> successor_counts{};

    constexpr void add_edge(size_t from, size_t to) {
        if (from < MaxPositions && successor_counts[from] < 32) {
            for (size_t i = 0; i < successor_counts[from]; ++i)
                if (successors[from][i] == to) return;
            successors[from][successor_counts[from]++] = to;
        }
    }

    constexpr void add_edges(size_t from, const auto& to_positions, size_t to_count) {
        for (size_t i = 0; i < to_count; ++i) add_edge(from, to_positions[i]);
    }
};

template <typename Pattern, size_t MaxPositions> constexpr void build_follow(follow_map<MaxPositions>& fmap, size_t offset);
template <typename... Content, size_t MaxPositions> constexpr void build_follow_sequence(follow_map<MaxPositions>& fmap, size_t offset);
template <typename... Options, size_t MaxPositions> constexpr void build_follow_select(follow_map<MaxPositions>& fmap, size_t offset);
template <size_t A, size_t B, typename... Content, size_t MaxPositions> constexpr void build_follow_repeat(follow_map<MaxPositions>& fmap, size_t offset);

template <typename Pattern, size_t MaxPositions = 512>
constexpr follow_map<MaxPositions> compute_follow() {
    follow_map<MaxPositions> fmap{};
    build_follow<Pattern>(fmap, 0);
    return fmap;
}

template <typename Pattern, size_t MaxPositions>
constexpr void build_follow(follow_map<MaxPositions>& fmap, size_t offset) {
    if constexpr (is_empty<Pattern>::value || is_character<Pattern>::value || is_any<Pattern>::value) { }
    else if constexpr (is_string<Pattern>::value) {
        constexpr size_t len = is_string<Pattern>::length;
        for (size_t i = 1; i < len; ++i) fmap.add_edge(offset + i, offset + i + 1);
    }
    else if constexpr (is_sequence<Pattern>::value) {
        []<typename... Content>(sequence<Content...>*, follow_map<MaxPositions>& fm, size_t off) {
            build_follow_sequence<Content...>(fm, off);
        }(static_cast<Pattern*>(nullptr), fmap, offset);
    }
    else if constexpr (is_select<Pattern>::value) {
        []<typename... Options>(select<Options...>*, follow_map<MaxPositions>& fm, size_t off) {
            build_follow_select<Options...>(fm, off);
        }(static_cast<Pattern*>(nullptr), fmap, offset);
    }
    else if constexpr (is_capture<Pattern>::value) {
        []<size_t Index, typename... Content>(capture<Index, Content...>*, follow_map<MaxPositions>& fm, size_t off) {
            if constexpr (sizeof...(Content) == 1) (build_follow<Content>(fm, off), ...);
            else build_follow_sequence<Content...>(fm, off);
        }(static_cast<Pattern*>(nullptr), fmap, offset);
    }
    else if constexpr (is_any_repeat_v<Pattern>) {
        if constexpr (is_repeat<Pattern>::value) {
            []<size_t A, size_t B, typename... Content>(repeat<A, B, Content...>*, follow_map<MaxPositions>& fm, size_t off) {
                build_follow_repeat<A, B, Content...>(fm, off);
            }(static_cast<Pattern*>(nullptr), fmap, offset);
        } else if constexpr (is_lazy_repeat<Pattern>::value) {
            []<size_t A, size_t B, typename... Content>(lazy_repeat<A, B, Content...>*, follow_map<MaxPositions>& fm, size_t off) {
                build_follow_repeat<A, B, Content...>(fm, off);
            }(static_cast<Pattern*>(nullptr), fmap, offset);
        } else if constexpr (is_possessive_repeat<Pattern>::value) {
            []<size_t A, size_t B, typename... Content>(possessive_repeat<A, B, Content...>*, follow_map<MaxPositions>& fm, size_t off) {
                build_follow_repeat<A, B, Content...>(fm, off);
            }(static_cast<Pattern*>(nullptr), fmap, offset);
        }
    }
    else if constexpr (CharacterLike<Pattern>) { }
}

template <typename... Content, size_t MaxPositions>
constexpr void build_follow_sequence(follow_map<MaxPositions>& fmap, size_t offset) {
    if constexpr (sizeof...(Content) == 0) return;
    else if constexpr (sizeof...(Content) == 1) {
        []<typename T>(follow_map<MaxPositions>& fm, size_t off) { build_follow<T>(fm, off); }
            .template operator()<Content...>(fmap, offset);
    } else {
        []<typename Head, typename... Tail>(follow_map<MaxPositions>& fm, size_t off) {
            build_follow<Head>(fm, off);
            auto [head_last, head_last_count] = last_positions<Head>(off);
            size_t tail_offset = off + count_positions<Head>();
            auto [tail_first, tail_first_count] = first_pack<Tail...>(tail_offset);
            for (size_t i = 0; i < head_last_count; ++i)
                fm.add_edges(head_last[i], tail_first, tail_first_count);
            build_follow_sequence<Tail...>(fm, tail_offset);
        }.template operator()<Content...>(fmap, offset);
    }
}

template <typename... Options, size_t MaxPositions>
constexpr void build_follow_select(follow_map<MaxPositions>& fmap, size_t offset) {
    if constexpr (sizeof...(Options) == 0) return;
    else {
        []<typename Head, typename... Tail>(follow_map<MaxPositions>& fm, size_t off) {
            build_follow<Head>(fm, off);
            if constexpr (sizeof...(Tail) > 0) {
                size_t next_offset = off + count_positions<Head>();
                build_follow_select<Tail...>(fm, next_offset);
            }
        }.template operator()<Options...>(fmap, offset);
    }
}

template <size_t A, size_t B, typename... Content, size_t MaxPositions>
constexpr void build_follow_repeat(follow_map<MaxPositions>& fmap, size_t offset) {
    if constexpr (sizeof...(Content) == 0) return;
    else if constexpr (sizeof...(Content) == 1) {
        []<typename T>(follow_map<MaxPositions>& fm, size_t off) {
            build_follow<T>(fm, off);
            if constexpr (B != 1) { // Add loop back (not for '?' quantifier)
                auto [content_first, first_count] = first_positions<T>(off);
                auto [content_last, last_count] = last_positions<T>(off);
                for (size_t i = 0; i < last_count; ++i)
                    fm.add_edges(content_last[i], content_first, first_count);
            }
        }.template operator()<Content...>(fmap, offset);
    } else {
        []<typename... All>(follow_map<MaxPositions>& fm, size_t off) {
            build_follow_sequence<All...>(fm, off);
            if constexpr (B != 1) {
                auto [content_first, first_count] = first_pack<All...>(off);
                auto [content_last, last_count] = last_pack<All...>(off);
                for (size_t i = 0; i < last_count; ++i)
                    fm.add_edges(content_last[i], content_first, first_count);
            }
        }.template operator()<Content...>(fmap, offset);
    }
}

// Symbol assignment to positions
template <typename Pattern>
constexpr void assign_symbols(std::array<char, 512>& symbols, size_t offset) {
    if constexpr (is_empty<Pattern>::value) { }
    else if constexpr (is_character<Pattern>::value) {
        []<auto C>(character<C>*, std::array<char, 512>& syms, size_t off) {
            syms[off + 1] = static_cast<char>(C);
        }(static_cast<Pattern*>(nullptr), symbols, offset);
    }
    else if constexpr (is_any<Pattern>::value) symbols[offset + 1] = '.';
    else if constexpr (is_string<Pattern>::value) {
        []<auto... Cs>(string<Cs...>*, std::array<char, 512>& syms, size_t off) {
            size_t pos = 1;
            ((syms[off + pos++] = static_cast<char>(Cs)), ...);
        }(static_cast<Pattern*>(nullptr), symbols, offset);
    }
    else if constexpr (is_sequence<Pattern>::value) {
        []<typename... Content>(sequence<Content...>*, std::array<char, 512>& syms, size_t off) {
            size_t current_offset = off;
            ((assign_symbols<Content>(syms, current_offset), current_offset += count_positions<Content>()), ...);
        }(static_cast<Pattern*>(nullptr), symbols, offset);
    }
    else if constexpr (is_select<Pattern>::value) {
        []<typename... Options>(select<Options...>*, std::array<char, 512>& syms, size_t off) {
            size_t current_offset = off;
            ((assign_symbols<Options>(syms, current_offset), current_offset += count_positions<Options>()), ...);
        }(static_cast<Pattern*>(nullptr), symbols, offset);
    }
    else if constexpr (is_capture<Pattern>::value) {
        []<size_t Index, typename... Content>(capture<Index, Content...>*, std::array<char, 512>& syms, size_t off) {
            size_t current_offset = off;
            ((assign_symbols<Content>(syms, current_offset), current_offset += count_positions<Content>()), ...);
        }(static_cast<Pattern*>(nullptr), symbols, offset);
    }
    else if constexpr (is_any_repeat_v<Pattern>) {
        if constexpr (is_repeat<Pattern>::value) {
            []<size_t A, size_t B, typename... Content>(repeat<A, B, Content...>*, std::array<char, 512>& syms, size_t off) {
                size_t current_offset = off;
                ((assign_symbols<Content>(syms, current_offset), current_offset += count_positions<Content>()), ...);
            }(static_cast<Pattern*>(nullptr), symbols, offset);
        } else if constexpr (is_lazy_repeat<Pattern>::value) {
            []<size_t A, size_t B, typename... Content>(lazy_repeat<A, B, Content...>*, std::array<char, 512>& syms, size_t off) {
                size_t current_offset = off;
                ((assign_symbols<Content>(syms, current_offset), current_offset += count_positions<Content>()), ...);
            }(static_cast<Pattern*>(nullptr), symbols, offset);
        } else if constexpr (is_possessive_repeat<Pattern>::value) {
            []<size_t A, size_t B, typename... Content>(possessive_repeat<A, B, Content...>*, std::array<char, 512>& syms, size_t off) {
                size_t current_offset = off;
                ((assign_symbols<Content>(syms, current_offset), current_offset += count_positions<Content>()), ...);
            }(static_cast<Pattern*>(nullptr), symbols, offset);
        }
    }
    else if constexpr (CharacterLike<Pattern>) symbols[offset + 1] = '?';
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
        constexpr size_t num_positions = count_positions<Pattern>();
        state_count = num_positions + 1;
        static_assert(num_positions + 1 <= MAX_POSITIONS, "Pattern too large");

        for (size_t i = 0; i < state_count; ++i) {
            states[i].id = i;
            states[i].symbol = '\0';
            states[i].successor_count = 0;
        }

        std::array<char, 512> symbols{};
        assign_symbols<Pattern>(symbols, 0);
        for (size_t i = 1; i < state_count; ++i) states[i].symbol = symbols[i];

        auto [first_set, first_count] = first_positions<Pattern>(0);
        for (size_t i = 0; i < first_count; ++i) {
            if (states[0].successor_count < MAX_SUCCESSORS)
                states[0].successors[states[0].successor_count++] = first_set[i];
        }

        auto follow_map = compute_follow<Pattern>();
        for (size_t i = 1; i < state_count; ++i) {
            for (size_t j = 0; j < follow_map.successor_counts[i]; ++j) {
                size_t succ = follow_map.successors[i][j];
                if (states[i].successor_count < MAX_SUCCESSORS)
                    states[i].successors[states[i].successor_count++] = succ;
            }
        }

        auto [last_set, last_count] = last_positions<Pattern>(0);
        accept_count = last_count;
        for (size_t i = 0; i < last_count; ++i) accept_states[i] = last_set[i];

        if constexpr (nullable<Pattern>()) {
            if (accept_count < MAX_SUCCESSORS) accept_states[accept_count++] = 0;
        }
    }
};

} // namespace ctre::glushkov

#endif // CTRE__GLUSHKOV_NFA__HPP
