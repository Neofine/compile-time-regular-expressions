// Simplified exec() function for search_method
// Extracts common patterns and reduces duplication

// This is a proposed replacement for lines 182-313 in wrapper.hpp

template <typename Modifier = singleline, typename ResultIterator = void, typename RE, typename IteratorBegin, typename IteratorEnd>
constexpr CTRE_FORCE_INLINE static auto exec(IteratorBegin orig_begin, IteratorBegin begin, IteratorEnd end, RE) noexcept {
	using result_iterator = std::conditional_t<std::is_same_v<ResultIterator, void>, IteratorBegin, ResultIterator>;

#ifndef CTRE_DISABLE_DECOMPOSITION
	using RawAST = decomposition::unwrap_regex_t<RE>;

	// Safety checks
	constexpr bool safe = !ctre::contains_greedy_any_repeat<RawAST>();
	constexpr bool real_iters = !std::is_same_v<IteratorEnd, zero_terminated_string_end_iterator>;

	if constexpr (safe && real_iters) {
		// Lookback helper: tries regex match with up to 64 positions lookback for leftmost match
		auto try_with_lookback = [&](auto lit_pos) {
			constexpr size_t max_lookback = 64;
			auto start = (lit_pos > begin + max_lookback) ? (lit_pos - max_lookback) : begin;
			for (auto pos = start; pos <= lit_pos; ++pos) {
				if (auto out = evaluate(orig_begin, pos, end, Modifier{}, return_type<result_iterator, RE>{}, ctll::list<start_mark, RE, end_mark, accept>())) {
					return out;
				}
			}
			return decltype(evaluate(orig_begin, begin, end, Modifier{}, return_type<result_iterator, RE>{}, ctll::list<start_mark, RE, end_mark, accept>())){};
		};

		// Path 1: Compile-time literal (dominant path analysis + SIMD)
		constexpr auto lit = dominators::extract_literal<RawAST>();
		if constexpr (lit.has_literal && lit.length >= 2) {
			constexpr auto finder = make_simd_finder<lit>(std::make_index_sequence<lit.length>{});

			for (auto it = begin; it != end; ++it) {
				bool found;
				if (std::is_constant_evaluated()) {
					char arr[lit.length + 1];
					for (size_t i = 0; i < lit.length; ++i) arr[i] = lit.chars[i];
					arr[lit.length] = '\0';
					found = find_literal_naive(it, end, arr);
				} else {
					found = finder(it, end);
					if (found) it = it - lit.length;
				}

				if (!found) break;
				if (auto out = try_with_lookback(it)) return out;
			}
		}
		// Path 2: Runtime literal (region analysis)
		else if (!std::is_constant_evaluated()) {
			constexpr auto nfa = glushkov::glushkov_nfa<RawAST>();
			auto region_lit = region::extract_literal_from_regions(nfa);

			if (region_lit.has_literal && region_lit.length >= 2) {
				for (auto it = begin; it != end; ++it) {
					// Check if literal matches at current position
					bool match = true;
					auto check = it;
					for (size_t i = 0; i < region_lit.length && check != end; ++i, ++check) {
						if (*check != region_lit.chars[i]) { match = false; break; }
					}

					if (match && check - it == static_cast<std::ptrdiff_t>(region_lit.length)) {
						if (auto out = try_with_lookback(it)) return out;
					}
				}
			}
		}
	}
#endif

	// Fallback: Standard search (try each position)
	constexpr bool fixed = starts_with_anchor(Modifier{}, ctll::list<RE>{});
	for (auto it = begin; end != it && !fixed; ++it) {
		if (auto out = evaluate(orig_begin, it, end, Modifier{}, return_type<result_iterator, RE>{}, ctll::list<start_mark, RE, end_mark, accept>())) {
			return out;
		}
	}
	return evaluate(orig_begin, end, end, Modifier{}, return_type<result_iterator, RE>{}, ctll::list<start_mark, RE, end_mark, accept>());
}
