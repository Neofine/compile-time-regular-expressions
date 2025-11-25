#ifndef CTRE__WRAPPER__HPP
#define CTRE__WRAPPER__HPP


#include "evaluation.hpp"
#include "range.hpp"
#include "return_type.hpp"
#include "utf8.hpp"
#include "utility.hpp"
#include "decomposition.hpp"  // For automatic decomposition
#include "simd_shift_or.hpp"  // For SIMD literal search
#ifndef CTRE_IN_A_MODULE
#include <algorithm>
#include <string_view>
#endif

namespace ctre {

CTRE_EXPORT template <typename RE, typename Method = void, typename Modifier = singleline> struct regular_expression;

struct zero_terminated_string_end_iterator {
	// this is here only because I want to support std::make_reverse_iterator
	using self_type = zero_terminated_string_end_iterator;
	using value_type = char;
	using reference = char &;
	using pointer = const char *;
	using iterator_category = std::bidirectional_iterator_tag;
	using difference_type = int;

	// it's just sentinel it won't be ever called
	auto operator++() noexcept -> self_type &;
	auto operator++(int) noexcept -> self_type;
	auto operator--() noexcept -> self_type &;
	auto operator--(int) noexcept -> self_type;
	friend auto operator==(self_type, self_type) noexcept -> bool;
	auto operator*() noexcept -> reference;

	constexpr CTRE_FORCE_INLINE friend bool operator==(const char * ptr, zero_terminated_string_end_iterator) noexcept {
		return *ptr == '\0';
	}
	constexpr CTRE_FORCE_INLINE friend bool operator==(const wchar_t * ptr, zero_terminated_string_end_iterator) noexcept {
		return *ptr == 0;
	}
	constexpr CTRE_FORCE_INLINE friend bool operator!=(const char * ptr, zero_terminated_string_end_iterator) noexcept {
		return *ptr != '\0';
	}
	constexpr CTRE_FORCE_INLINE friend bool operator!=(const wchar_t * ptr, zero_terminated_string_end_iterator) noexcept {
		return *ptr != 0;
	}
	constexpr CTRE_FORCE_INLINE friend bool operator==(zero_terminated_string_end_iterator, const char * ptr) noexcept {
		return *ptr == '\0';
	}
	constexpr CTRE_FORCE_INLINE friend bool operator==(zero_terminated_string_end_iterator, const wchar_t * ptr) noexcept {
		return *ptr == 0;
	}
	constexpr CTRE_FORCE_INLINE friend bool operator!=(zero_terminated_string_end_iterator, const char * ptr) noexcept {
		return *ptr != '\0';
	}
	constexpr CTRE_FORCE_INLINE friend bool operator!=(zero_terminated_string_end_iterator, const wchar_t * ptr) noexcept {
		return *ptr != 0;
	}
};

template <typename T> class RangeLikeType {
	template <typename Y> static auto test(Y *) -> decltype(std::declval<const Y &>().begin(), std::declval<const Y &>().end(), std::true_type());
	template <typename> static auto test(...) -> std::false_type;

public:
	static constexpr bool value = decltype(test<std::remove_reference_t<std::remove_const_t<T>>>(nullptr))::value;
};

struct match_method {
	template <typename Modifier = singleline, typename ResultIterator = void, typename RE, typename IteratorBegin, typename IteratorEnd> constexpr CTRE_FORCE_INLINE static auto exec(IteratorBegin orig_begin, IteratorBegin begin, IteratorEnd end, RE) noexcept {
		using result_iterator = std::conditional_t<std::is_same_v<ResultIterator, void>, IteratorBegin, ResultIterator>;

		return evaluate(orig_begin, begin, end, Modifier{}, return_type<result_iterator, RE>{}, ctll::list<start_mark, RE, assert_subject_end, end_mark, accept>());
	}

	template <typename Modifier = singleline, typename ResultIterator = void, typename RE, typename IteratorBegin, typename IteratorEnd> constexpr CTRE_FORCE_INLINE static auto exec(IteratorBegin begin, IteratorEnd end, RE) noexcept {
		return exec<Modifier, ResultIterator>(begin, begin, end, RE{});
	}
};

// Helpers for detecting leading greedy repeats (.*) which cause backtracking issues
template <typename T>
struct sequence_first;

template <typename First, typename... Rest>
struct sequence_first<ctre::sequence<First, Rest...>> {
	using type = First;
};

template <typename T>
struct is_greedy_any_repeat : std::false_type {};

// Note: repeat takes Content... as a variadic pack, so we need to match repeat<0, 0, any>
// where any is the single element in the Content pack
template <>
struct is_greedy_any_repeat<ctre::repeat<0, 0, ctre::any>> : std::true_type {}; // .*

template <>
struct is_greedy_any_repeat<ctre::repeat<1, 0, ctre::any>> : std::true_type {}; // .+

// BUG FIX #12: Check if pattern contains ANY greedy .* or .+ (not just leading)
// These cause stack overflow/segfault with our lookback mechanism

// Use a struct so we can do partial specialization
template <typename T>
struct greedy_any_repeat_checker {
	static constexpr bool value = is_greedy_any_repeat<T>::value;
};

// Specialization for sequence
template <typename... Content>
struct greedy_any_repeat_checker<ctre::sequence<Content...>> {
	static constexpr bool value = (greedy_any_repeat_checker<Content>::value || ...);
};

// Specialization for select
template <typename A, typename B>
struct greedy_any_repeat_checker<ctre::select<A, B>> {
	static constexpr bool value = greedy_any_repeat_checker<A>::value || greedy_any_repeat_checker<B>::value;
};

// Specialization for repeat - check if it's .* or .+ first, then recurse into content
template <size_t N, size_t M, typename... Content>
struct greedy_any_repeat_checker<ctre::repeat<N, M, Content...>> {
	// Check if this repeat itself is .* or .+ (greedy repeat of any)
	static constexpr bool is_greedy_dot = is_greedy_any_repeat<ctre::repeat<N, M, Content...>>::value;
	// Also recursively check the content
	static constexpr bool has_greedy_in_content = (greedy_any_repeat_checker<Content>::value || ...);
	static constexpr bool value = is_greedy_dot || has_greedy_in_content;
};

template <typename T>
constexpr bool contains_greedy_any_repeat() {
	return greedy_any_repeat_checker<T>::value;
}

template <typename T>
constexpr bool has_leading_greedy_repeat() {
	// Check if T is a sequence<repeat<0, 0, any>, ...>
	if constexpr (requires { typename sequence_first<T>::type; }) {
		using first = typename sequence_first<T>::type;
		return is_greedy_any_repeat<first>::value;
	}
	// Check if T itself is repeat<0, 0, any>
	return is_greedy_any_repeat<T>::value;
}

struct search_method {
	// Helper: SIMD finder generator (same as fast_search.hpp)
	template <auto Literal, size_t... Is>
	static constexpr auto make_simd_finder(std::index_sequence<Is...>) noexcept {
		return []<typename Iterator, typename EndIterator>(Iterator& it, EndIterator end) -> bool {
			return simd::match_string_shift_or<Literal.chars[Is]...>(it, end, flags{});
		};
	}

	// Helper: Naive literal search fallback
	template <typename Iterator, typename EndIterator, size_t LitLength>
	static constexpr bool find_literal_naive(Iterator& it, EndIterator end, const char (&literal)[LitLength]) noexcept {
		if (it == end) return false;
		const size_t len = LitLength - 1;
		for (auto search_it = it; search_it != end; ++search_it) {
			bool match = true;
			auto check_it = search_it;
			for (size_t i = 0; i < len && check_it != end; ++i, ++check_it) {
				if (*check_it != literal[i]) {
					match = false;
					break;
				}
			}
			if (match && std::distance(search_it, check_it) == static_cast<std::ptrdiff_t>(len)) {
				it = search_it;
				return true;
			}
		}
		return false;
	}

	template <typename Modifier = singleline, typename ResultIterator = void, typename RE, typename IteratorBegin, typename IteratorEnd>
	constexpr CTRE_FORCE_INLINE static auto exec(IteratorBegin orig_begin, IteratorBegin begin, IteratorEnd end, RE) noexcept {
		using result_iterator = std::conditional_t<std::is_same_v<ResultIterator, void>, IteratorBegin, ResultIterator>;

	// PHASE 5: Automatic decomposition optimization
#ifndef CTRE_DISABLE_DECOMPOSITION
	// Check if pattern has a useful literal for prefiltering
	using RawAST = decomposition::unwrap_regex_t<RE>;

#ifndef CTRE_CHAR_EXPANSION_DISABLED
	// Use expansion-aware extraction (tries AST-based expansion first)
	constexpr auto path_literal = decomposition::extract_literal_with_expansion_and_fallback<RE>();
#else
	// Use original NFA-based extraction only
	constexpr auto path_literal = dominators::extract_literal<RawAST>();
#endif

	// BUG FIX #12: Disable decomposition for patterns with ANY greedy .* or .+
	// These cause stack overflow/segfault with our lookback approach (e.g., hello.*world)
	constexpr bool safe_for_decomposition = !ctre::contains_greedy_any_repeat<RawAST>();

	// BUG FIX #10: Disable decomposition when end iterator is a sentinel (e.g., zero_terminated_string_end_iterator)
	// The decomposition path assumes real iterators that can be converted to pointers
	constexpr bool has_real_iterators = !std::is_same_v<IteratorEnd, zero_terminated_string_end_iterator>;

	// BUG FIX #17: Require minimum literal length to avoid overhead on short patterns
	// For patterns like x[0-1]y (3 chars total), SIMD prefiltering adds overhead vs direct regex
	// Require literal >= 4 chars to ensure prefiltering is worthwhile
	constexpr bool literal_long_enough = path_literal.has_literal && path_literal.length >= 4;

	// BUG FIX #21: Disable decomposition for alternations where expansion picks a non-dominant literal
	// Example: (foo|bar)suffix → expansion picks "foosuffix" but "barsuffix" inputs won't match
	// Solution: extract_literal_with_expansion_and_fallback() now stores the NFA dominator length
	// in the result, so we can compare without recomputing (avoids duplicate NFA construction)
	constexpr bool literal_is_truly_dominant = !path_literal.has_literal ||  // No literal, safe to proceed
	                                            (path_literal.nfa_dominator_length > 0 &&
	                                             path_literal.length == path_literal.nfa_dominator_length);  // Lengths match, truly dominant

	// Try compile-time path analysis first
	if constexpr (safe_for_decomposition && has_real_iterators && literal_long_enough && literal_is_truly_dominant) {
			constexpr auto literal = path_literal;

		// Check if pattern is EXACTLY the literal (no prefix/suffix)
		constexpr auto nfa = glushkov::glushkov_nfa<RawAST>();
		constexpr bool pattern_is_literal = (nfa.state_count == literal.length + 1);

		// BUG FIX #16: Don't use decomposition for pure literals!
		// The original CTRE engine is faster (2-5ns) than decomposition overhead (15-20ns)
		// Just skip this entire decomposition block and fall through to standard path
		if constexpr (!pattern_is_literal) {
		// Use compile-time extracted literal
		{
				// === FAST PATH: Use SIMD prefiltering ===
				auto it = begin;
				constexpr auto simd_finder = make_simd_finder<literal>(std::make_index_sequence<literal.length>{});

				while (it != end) {
					bool found;
					if (std::is_constant_evaluated()) {
						char lit_array[literal.length + 1];
						for (size_t i = 0; i < literal.length; ++i) {
							lit_array[i] = literal.chars[i];
						}
						lit_array[literal.length] = '\0';
						found = find_literal_naive(it, end, lit_array);
					} else {
						found = simd_finder(it, end);
						if (found && literal.length > 0) {
							it = it - literal.length;
						}
					}

				if (!found) break;

				// Pattern has prefix/suffix - need lookback
				constexpr size_t max_lookback = 64;
				auto search_start = (it > begin + max_lookback) ? (it - max_lookback) : begin;

				// Search forward from search_start to find the LEFTMOST match
				for (auto try_pos = search_start; try_pos <= it; ++try_pos) {
					if (auto out = evaluate(orig_begin, try_pos, end, Modifier{}, return_type<result_iterator, RE>{}, ctll::list<start_mark, RE, end_mark, accept>())) {
						return out;  // This is the leftmost match
					}
				}

					++it;
				}

			auto out = evaluate(orig_begin, end, end, Modifier{}, return_type<result_iterator, RE>{}, ctll::list<start_mark, RE, end_mark, accept>());
			if (!out) out.set_end_mark(end);
			return out;
		}
		} // End of !pattern_is_literal block
} else {
		// Fallback: Try region analysis at runtime (can't be constexpr)
		if constexpr (safe_for_decomposition && has_real_iterators && literal_is_truly_dominant) {
				if (!std::is_constant_evaluated()) {
					constexpr auto nfa = glushkov::glushkov_nfa<RawAST>();
					auto region_literal = region::extract_literal_from_regions(nfa);

				// BUG FIX #17: Require minimum literal length for region analysis too
				if (region_literal.has_literal && region_literal.length >= 4) {
				// === FAST PATH: Use manual search for runtime literal ===
				auto it = begin;
				const size_t needle_length = region_literal.length;

				// Check if pattern might be just the literal (heuristic: state_count close to literal length)
				bool likely_literal_only = (nfa.state_count <= needle_length + 2);

				while (it != end) {
					// Try to match the literal at current position
					bool match = true;
					auto check_it = it;
					for (size_t i = 0; i < needle_length && check_it != end; ++i, ++check_it) {
						if (*check_it != region_literal.chars[i]) {
							match = false;
							break;
						}
					}

					if (match && check_it - it == static_cast<std::ptrdiff_t>(needle_length)) {
						// Found literal at 'it' - try matching
						if (likely_literal_only) {
							// Pattern is likely just the literal - try exact position only
							if (auto out = evaluate(orig_begin, it, end, Modifier{}, return_type<result_iterator, RE>{}, ctll::list<start_mark, RE, end_mark, accept>())) {
								return out;
							}
						} else {
							// Pattern has more than literal - use lookback
							constexpr size_t max_lookback = 64;
							auto search_start = (it > begin + max_lookback) ? (it - max_lookback) : begin;

							for (auto try_pos = search_start; try_pos <= it; ++try_pos) {
								if (auto out = evaluate(orig_begin, try_pos, end, Modifier{}, return_type<result_iterator, RE>{}, ctll::list<start_mark, RE, end_mark, accept>())) {
									return out;
								}
							}
						}
					}

					++it;
					if (it == end) break;
				}

				auto out = evaluate(orig_begin, end, end, Modifier{}, return_type<result_iterator, RE>{}, ctll::list<start_mark, RE, end_mark, accept>());
				if (!out) out.set_end_mark(end);
				return out;
				}
				}
			}
		}
#endif // CTRE_DISABLE_DECOMPOSITION

		// === STANDARD PATH: Original search implementation ===
		constexpr bool fixed = starts_with_anchor(Modifier{}, ctll::list<RE>{});
		auto it = begin;

		for (; end != it && !fixed; ++it) {
			if (auto out = evaluate(orig_begin, it, end, Modifier{}, return_type<result_iterator, RE>{}, ctll::list<start_mark, RE, end_mark, accept>())) {
				return out;
			}
		}

		auto out = evaluate(orig_begin, it, end, Modifier{}, return_type<result_iterator, RE>{}, ctll::list<start_mark, RE, end_mark, accept>());
		if (!out) out.set_end_mark(it);
		return out;
	}

	template <typename Modifier = singleline, typename ResultIterator = void, typename RE, typename IteratorBegin, typename IteratorEnd>
	constexpr CTRE_FORCE_INLINE static auto exec(IteratorBegin begin, IteratorEnd end, RE) noexcept {
		return exec<Modifier, ResultIterator>(begin, begin, end, RE{});
	}
};

struct starts_with_method {
	template <typename Modifier = singleline, typename ResultIterator = void, typename RE, typename IteratorBegin, typename IteratorEnd> constexpr CTRE_FORCE_INLINE static auto exec(IteratorBegin orig_begin, IteratorBegin begin, IteratorEnd end, RE) noexcept {
		using result_iterator = std::conditional_t<std::is_same_v<ResultIterator, void>, IteratorBegin, ResultIterator>;
		return evaluate(orig_begin, begin, end, Modifier{}, return_type<result_iterator, RE>{}, ctll::list<start_mark, RE, end_mark, accept>());
	}

	template <typename Modifier = singleline, typename ResultIterator = void, typename RE, typename IteratorBegin, typename IteratorEnd> constexpr CTRE_FORCE_INLINE static auto exec(IteratorBegin begin, IteratorEnd end, RE) noexcept {
		return exec<Modifier, ResultIterator>(begin, begin, end, RE{});
	}
};

// wrapper which calls search on input
struct range_method {
	template <typename Modifier = singleline, typename ResultIterator = void, typename RE, typename IteratorBegin, typename IteratorEnd> constexpr CTRE_FORCE_INLINE static auto exec(IteratorBegin begin, IteratorEnd end, RE) noexcept {
		using result_iterator = std::conditional_t<std::is_same_v<ResultIterator, void>, IteratorBegin, ResultIterator>;
		using wrapped_regex = regular_expression<RE, search_method, Modifier>;

		return regex_range<IteratorBegin, IteratorEnd, wrapped_regex, result_iterator>(begin, end);
	}
};

struct tokenize_method {
	template <typename Modifier = singleline, typename ResultIterator = void, typename RE, typename IteratorBegin, typename IteratorEnd> constexpr CTRE_FORCE_INLINE static auto exec(IteratorBegin begin, IteratorEnd end, RE) noexcept {
		using result_iterator = std::conditional_t<std::is_same_v<ResultIterator, void>, IteratorBegin, ResultIterator>;
		using wrapped_regex = regular_expression<RE, starts_with_method, Modifier>;

		return regex_range<IteratorBegin, IteratorEnd, wrapped_regex, result_iterator>(begin, end);
	}
};

struct split_method {
	template <typename Modifier = singleline, typename ResultIterator = void, typename RE, typename IteratorBegin, typename IteratorEnd> constexpr CTRE_FORCE_INLINE static auto exec(IteratorBegin begin, IteratorEnd end, RE) noexcept {
		using result_iterator = std::conditional_t<std::is_same_v<ResultIterator, void>, IteratorBegin, ResultIterator>;
		using wrapped_regex = regular_expression<RE, search_method, Modifier>;

		return regex_split_range<IteratorBegin, IteratorEnd, wrapped_regex, result_iterator>(begin, end);
	}
};

struct iterator_method {
	template <typename Modifier = singleline, typename ResultIterator = void, typename RE, typename IteratorBegin, typename IteratorEnd> constexpr CTRE_FORCE_INLINE static auto exec(IteratorBegin begin, IteratorEnd end, RE) noexcept {
		using result_iterator = std::conditional_t<std::is_same_v<ResultIterator, void>, IteratorBegin, ResultIterator>;
		using wrapped_regex = regular_expression<RE, search_method, Modifier>;

		return regex_iterator<IteratorBegin, IteratorEnd, wrapped_regex, result_iterator>(begin, end);
	}
	constexpr CTRE_FORCE_INLINE static auto exec() noexcept {
		return regex_end_iterator{};
	}
};

CTRE_EXPORT template <typename RE, typename Method, typename Modifier> struct regular_expression {
	constexpr CTRE_FORCE_INLINE regular_expression() noexcept { }
	constexpr CTRE_FORCE_INLINE regular_expression(RE) noexcept { }

	template <typename ResultIterator, typename IteratorBegin, typename IteratorEnd> constexpr CTRE_FORCE_INLINE static auto exec_with_result_iterator(IteratorBegin orig_begin, IteratorBegin begin, IteratorEnd end) noexcept {
		return Method::template exec<Modifier, ResultIterator>(orig_begin, begin, end, RE{});
	}
	template <typename ResultIterator, typename IteratorBegin, typename IteratorEnd> constexpr CTRE_FORCE_INLINE static auto exec_with_result_iterator(IteratorBegin begin, IteratorEnd end) noexcept {
		return Method::template exec<Modifier, ResultIterator>(begin, end, RE{});
	}
	template <typename Range> constexpr CTRE_FORCE_INLINE static auto multi_exec(Range && range) noexcept {
		return multi_subject_range<Range, regular_expression>{std::forward<Range>(range)};
	}
	constexpr CTRE_FORCE_INLINE static auto exec() noexcept {
		return Method::exec();
	}
	template <typename IteratorBegin, typename IteratorEnd> constexpr CTRE_FORCE_INLINE static auto exec(IteratorBegin begin, IteratorEnd end) noexcept {
		return Method::template exec<Modifier>(begin, end, RE{});
	}
	static constexpr CTRE_FORCE_INLINE auto exec(const char * s) noexcept {
		return Method::template exec<Modifier>(s, zero_terminated_string_end_iterator(), RE{});
	}
	static constexpr CTRE_FORCE_INLINE auto exec(const wchar_t * s) noexcept {
		return Method::template exec<Modifier>(s, zero_terminated_string_end_iterator(), RE{});
	}
	static constexpr CTRE_FORCE_INLINE auto exec(std::string_view sv) noexcept {
		return exec(sv.begin(), sv.end());
	}
	static constexpr CTRE_FORCE_INLINE auto exec(std::wstring_view sv) noexcept {
		return exec(sv.begin(), sv.end());
	}
#ifdef CTRE_ENABLE_UTF8_RANGE
	static constexpr CTRE_FORCE_INLINE auto exec(std::u8string_view sv) noexcept {
		return exec_with_result_iterator<const char8_t *>(utf8_range(sv).begin(), utf8_range(sv).end());
	}
#endif
	static constexpr CTRE_FORCE_INLINE auto exec(std::u16string_view sv) noexcept {
		return exec(sv.begin(), sv.end());
	}
	static constexpr CTRE_FORCE_INLINE auto exec(std::u32string_view sv) noexcept {
		return exec(sv.begin(), sv.end());
	}
	template <typename Range, typename = typename std::enable_if<RangeLikeType<Range>::value>::type> static constexpr CTRE_FORCE_INLINE auto exec(Range && range) noexcept {
		return exec(std::begin(range), std::end(range));
	}

	// another api
	template <typename... Args> CTRE_FORCE_INLINE constexpr auto operator()(Args &&... args) const noexcept {
		return exec(std::forward<Args>(args)...);
	}
	// api for pattern matching
	template <typename... Args> CTRE_FORCE_INLINE constexpr auto try_extract(Args &&... args) const noexcept {
		return exec(std::forward<Args>(args)...);
	}

	// for compatibility with _ctre literal
	template <typename... Args> static constexpr CTRE_FORCE_INLINE auto match(Args &&... args) noexcept {
		return regular_expression<RE, match_method, singleline>::exec(std::forward<Args>(args)...);
	}
	template <typename... Args> static constexpr CTRE_FORCE_INLINE auto search(Args &&... args) noexcept {
		return regular_expression<RE, search_method, singleline>::exec(std::forward<Args>(args)...);
	}
	template <typename... Args> static constexpr CTRE_FORCE_INLINE auto starts_with(Args &&... args) noexcept {
		return regular_expression<RE, starts_with_method, singleline>::exec(std::forward<Args>(args)...);
	}
	template <typename... Args> static constexpr CTRE_FORCE_INLINE auto range(Args &&... args) noexcept {
		return regular_expression<RE, range_method, singleline>::exec(std::forward<Args>(args)...);
	}
	template <typename... Args> static constexpr CTRE_FORCE_INLINE auto split(Args &&... args) noexcept {
		return regular_expression<RE, split_method, singleline>::exec(std::forward<Args>(args)...);
	}
	template <typename... Args> static constexpr CTRE_FORCE_INLINE auto tokenize(Args &&... args) noexcept {
		return regular_expression<RE, tokenize_method, singleline>::exec(std::forward<Args>(args)...);
	}
	template <typename... Args> static constexpr CTRE_FORCE_INLINE auto iterator(Args &&... args) noexcept {
		return regular_expression<RE, iterator_method, singleline>::exec(std::forward<Args>(args)...);
	}

	template <typename... Args> static constexpr CTRE_FORCE_INLINE auto multiline_match(Args &&... args) noexcept {
		return regular_expression<RE, match_method, multiline>::exec(std::forward<Args>(args)...);
	}
	template <typename... Args> static constexpr CTRE_FORCE_INLINE auto multiline_search(Args &&... args) noexcept {
		return regular_expression<RE, search_method, multiline>::exec(std::forward<Args>(args)...);
	}
	template <typename... Args> static constexpr CTRE_FORCE_INLINE auto multiline_starts_with(Args &&... args) noexcept {
		return regular_expression<RE, starts_with_method, multiline>::exec(std::forward<Args>(args)...);
	}
	template <typename... Args> static constexpr CTRE_FORCE_INLINE auto multiline_range(Args &&... args) noexcept {
		return regular_expression<RE, range_method, multiline>::exec(std::forward<Args>(args)...);
	}
	template <typename... Args> static constexpr CTRE_FORCE_INLINE auto multiline_split(Args &&... args) noexcept {
		return regular_expression<RE, split_method, multiline>::exec(std::forward<Args>(args)...);
	}
	template <typename... Args> static constexpr CTRE_FORCE_INLINE auto multiline_tokenize(Args &&... args) noexcept {
		return regular_expression<RE, tokenize_method, multiline>::exec(std::forward<Args>(args)...);
	}
	template <typename... Args> static constexpr CTRE_FORCE_INLINE auto multiline_iterator(Args &&... args) noexcept {
		return regular_expression<RE, iterator_method, multiline>::exec(std::forward<Args>(args)...);
	}
};

// range style API support for tokenize/range/split operations
template <typename Range, typename RE, typename Modifier> constexpr auto operator|(Range && range, regular_expression<RE, range_method, Modifier> re) noexcept {
	return re.exec(std::forward<Range>(range));
}

template <typename Range, typename RE, typename Modifier> constexpr auto operator|(Range && range, regular_expression<RE, tokenize_method, Modifier> re) noexcept {
	return re.exec(std::forward<Range>(range));
}

template <typename Range, typename RE, typename Modifier> constexpr auto operator|(Range && range, regular_expression<RE, split_method, Modifier> re) noexcept {
	return re.exec(std::forward<Range>(range));
}

template <typename Range, typename RE, typename Modifier> constexpr auto operator|(Range && range, regular_expression<RE, iterator_method, Modifier> re) noexcept = delete;

template <typename Range, typename RE, typename Method, typename Modifier> constexpr auto operator|(Range && range, regular_expression<RE, Method, Modifier> re) noexcept {
	return re.multi_exec(std::forward<Range>(range));
}

// error reporting of problematic position in a regex
template <size_t> struct problem_at_position; // do not define!

template <> struct problem_at_position<~static_cast<size_t>(0)> {
	constexpr operator bool() const noexcept {
		return true;
	}
};

#if CTRE_CNTTP_COMPILER_CHECK
#define CTRE_REGEX_INPUT_TYPE ctll::fixed_string
#define CTRE_REGEX_TEMPLATE_COPY_TYPE auto
#else
#define CTRE_REGEX_INPUT_TYPE const auto &
#define CTRE_REGEX_TEMPLATE_COPY_TYPE const auto &
#endif

template <CTRE_REGEX_TEMPLATE_COPY_TYPE input> struct regex_builder {
	static constexpr auto _input = input;
	using result = typename ctll::parser<ctre::pcre, _input, ctre::pcre_actions>::template output<pcre_context<>>;

	static constexpr auto n = result::is_correct ? ~static_cast<size_t>(0) : result::position;

	static_assert(result::is_correct && problem_at_position<n>{}, "Regular Expression contains syntax error.");

	using type = ctll::conditional<result::is_correct, decltype(ctll::front(typename result::output_type::stack_type())), ctll::list<reject>>;
};

// case-sensitive

CTRE_EXPORT template <CTRE_REGEX_INPUT_TYPE input, typename... Modifiers> constexpr auto match = regular_expression<typename regex_builder<input>::type, match_method, ctll::list<singleline, Modifiers...>>();

CTRE_EXPORT template <CTRE_REGEX_INPUT_TYPE input, typename... Modifiers> constexpr auto search = regular_expression<typename regex_builder<input>::type, search_method, ctll::list<singleline, Modifiers...>>();

CTRE_EXPORT template <CTRE_REGEX_INPUT_TYPE input, typename... Modifiers> constexpr auto starts_with = regular_expression<typename regex_builder<input>::type, starts_with_method, ctll::list<singleline, Modifiers...>>();

CTRE_EXPORT template <CTRE_REGEX_INPUT_TYPE input, typename... Modifiers> constexpr auto search_all = regular_expression<typename regex_builder<input>::type, range_method, ctll::list<singleline, Modifiers...>>();

CTRE_EXPORT template <CTRE_REGEX_INPUT_TYPE input, typename... Modifiers> [[deprecated("use search_all")]] constexpr auto range = regular_expression<typename regex_builder<input>::type, range_method, ctll::list<singleline, Modifiers...>>();

CTRE_EXPORT template <CTRE_REGEX_INPUT_TYPE input, typename... Modifiers> constexpr auto split = regular_expression<typename regex_builder<input>::type, split_method, ctll::list<singleline, Modifiers...>>();

CTRE_EXPORT template <CTRE_REGEX_INPUT_TYPE input, typename... Modifiers> constexpr auto tokenize = regular_expression<typename regex_builder<input>::type, tokenize_method, ctll::list<singleline, Modifiers...>>();

CTRE_EXPORT template <CTRE_REGEX_INPUT_TYPE input, typename... Modifiers> constexpr auto iterator = regular_expression<typename regex_builder<input>::type, iterator_method, ctll::list<singleline, Modifiers...>>();

CTRE_EXPORT constexpr auto sentinel = regex_end_iterator();

// multiline

CTRE_EXPORT template <CTRE_REGEX_INPUT_TYPE input, typename... Modifiers> constexpr auto multiline_match = regular_expression<typename regex_builder<input>::type, match_method, ctll::list<multiline, Modifiers...>>();

CTRE_EXPORT template <CTRE_REGEX_INPUT_TYPE input, typename... Modifiers> constexpr auto multiline_search = regular_expression<typename regex_builder<input>::type, search_method, ctll::list<multiline, Modifiers...>>();

CTRE_EXPORT template <CTRE_REGEX_INPUT_TYPE input, typename... Modifiers> constexpr auto multiline_starts_with = regular_expression<typename regex_builder<input>::type, starts_with_method, ctll::list<multiline, Modifiers...>>();

CTRE_EXPORT template <CTRE_REGEX_INPUT_TYPE input, typename... Modifiers> constexpr auto multiline_search_all = regular_expression<typename regex_builder<input>::type, range_method, ctll::list<multiline, Modifiers...>>();

CTRE_EXPORT template <CTRE_REGEX_INPUT_TYPE input, typename... Modifiers> [[deprecated("use multiline_search_all")]] constexpr auto multiline_range = regular_expression<typename regex_builder<input>::type, range_method, ctll::list<multiline, Modifiers...>>();

CTRE_EXPORT template <CTRE_REGEX_INPUT_TYPE input, typename... Modifiers> constexpr auto multiline_split = regular_expression<typename regex_builder<input>::type, split_method, ctll::list<multiline, Modifiers...>>();

CTRE_EXPORT template <CTRE_REGEX_INPUT_TYPE input, typename... Modifiers> constexpr auto multiline_tokenize = regular_expression<typename regex_builder<input>::type, tokenize_method, ctll::list<multiline, Modifiers...>>();

CTRE_EXPORT template <CTRE_REGEX_INPUT_TYPE input, typename... Modifiers> constexpr auto multiline_iterator = regular_expression<typename regex_builder<input>::type, iterator_method, ctll::list<multiline, Modifiers...>>();

CTRE_EXPORT constexpr auto multiline_sentinel = regex_end_iterator();

} // namespace ctre


#endif
