#ifndef CTRE__WRAPPER_TEST__HPP  
#define CTRE__WRAPPER_TEST__HPP

#include "evaluation.hpp"
#include "return_type.hpp"
#include "prefilter_database.hpp"  // ONLY the database, NO analysis

namespace ctre {

struct match_method_optimized {
	template <typename Modifier = singleline, typename ResultIterator = void, typename RE, typename IteratorBegin, typename IteratorEnd> 
	constexpr CTRE_FORCE_INLINE static auto exec(IteratorBegin orig_begin, IteratorBegin begin, IteratorEnd end, RE) noexcept {
		using result_iterator = std::conditional_t<std::is_same_v<ResultIterator, void>, IteratorBegin, ResultIterator>;

		// Query prefilter database (like calling hs_database_info)
		constexpr auto lit_info = prefilter::get_literal<RE>();
		constexpr bool can_prefilter = lit_info.has_literal && lit_info.length >= 2 && 
		                                std::is_pointer_v<IteratorBegin> && std::is_same_v<IteratorEnd, const char*>;
		
		if constexpr (can_prefilter) {
			// Runtime scan (like hs_scan)
			if (!std::is_constant_evaluated()) {
				if (!prefilter::scan_for_literal(begin, end, lit_info.chars, lit_info.length)) {
					// Fast fail
					auto out = evaluate(orig_begin, end, end, Modifier{}, return_type<result_iterator, RE>{}, 
					                   ctll::list<start_mark, RE, assert_subject_end, end_mark, accept>());
					return out;
				}
			}
		}

		return evaluate(orig_begin, begin, end, Modifier{}, return_type<result_iterator, RE>{}, 
		               ctll::list<start_mark, RE, assert_subject_end, end_mark, accept>());
	}
};

} // namespace ctre

#endif
