#ifndef CTRE__WRAPPER_WITH_PREFILTER__HPP
#define CTRE__WRAPPER_WITH_PREFILTER__HPP

// Copy of wrapper.hpp but with prefilter integrated

#include "evaluation.hpp"
#include "range.hpp"
#include "return_type.hpp"
#include "utf8.hpp"
#include "utility.hpp"
#include "glushkov_nfa.hpp"
#include "bitnfa/bitnfa_match.hpp"
#include "prefilter_traits.hpp"

namespace ctre {

struct match_method_with_prefilter {
	template <typename Modifier = singleline, typename ResultIterator = void, typename RE, typename IteratorBegin, typename IteratorEnd> 
	constexpr CTRE_FORCE_INLINE static auto exec(IteratorBegin orig_begin, IteratorBegin begin, IteratorEnd end, RE) noexcept {
		using result_iterator = std::conditional_t<std::is_same_v<ResultIterator, void>, IteratorBegin, ResultIterator>;

		// Prefilter check
		constexpr bool has_literal = prefilter::literal_info<RE>::has_literal;
		constexpr size_t lit_len = prefilter::literal_info<RE>::get_length();
		constexpr bool can_prefilter = has_literal && lit_len >= 2 && std::is_pointer_v<IteratorBegin> && std::is_same_v<IteratorEnd, const char*>;
		
		if constexpr (can_prefilter) {
			if (!std::is_constant_evaluated()) {
				bool found = [&]<size_t... Is>(std::index_sequence<Is...>) {
					constexpr auto chars = prefilter::literal_info<RE>::get_chars();
					return prefilter::contains_literal_simd<chars[Is]...>(begin, end);
				}(std::make_index_sequence<lit_len>{});
				
				if (!found) {
					// Return not_matched
					auto out = evaluate(orig_begin, end, end, Modifier{}, return_type<result_iterator, RE>{}, ctll::list<start_mark, RE, assert_subject_end, end_mark, accept>());
					return out;
				}
			}
		}

		return evaluate(orig_begin, begin, end, Modifier{}, return_type<result_iterator, RE>{}, ctll::list<start_mark, RE, assert_subject_end, end_mark, accept>());
	}

	template <typename Modifier = singleline, typename ResultIterator = void, typename RE, typename IteratorBegin, typename IteratorEnd> 
	constexpr CTRE_FORCE_INLINE static auto exec(IteratorBegin begin, IteratorEnd end, RE) noexcept {
		return exec<Modifier, ResultIterator>(begin, begin, end, RE{});
	}
};

// Test API
template <CTRE_REGEX_INPUT_TYPE input, typename... Modifiers>
constexpr auto match_with_prefilter = regular_expression<typename regex_builder<input>::type, match_method_with_prefilter, ctll::list<singleline, Modifiers...>>();

} // namespace ctre

#endif
