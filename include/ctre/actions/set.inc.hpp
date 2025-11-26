#ifndef CTRE__ACTIONS__SET__HPP
#define CTRE__ACTIONS__SET__HPP

// UTILITY
// add into set if not exists
template <template <typename...> typename SetType, typename T, typename... As, bool Exists = (std::is_same_v<T, As> || ... || false)> static constexpr auto push_back_into_set(T, SetType<As...>) -> ctll::conditional<Exists, SetType<As...>, SetType<As...,T>> { return {}; }

// END OF UTILITY

// set_start
template <auto V, typename A,typename... Ts, typename Parameters> static constexpr auto apply(pcre::set_start, ctll::term<V>, pcre_context<ctll::list<A,Ts...>, Parameters> subject) {
	return pcre_context{ctll::push_front(set<A>(), ctll::list<Ts...>()), subject.parameters};
}

// set_empty
template <auto V, typename... Ts, typename Parameters> static constexpr auto apply(pcre::set_empty, ctll::term<V>, pcre_context<ctll::list<Ts...>, Parameters> subject) {
	return pcre_context{ctll::push_front(set<>(), ctll::list<Ts...>()), subject.parameters};
}

// set_make
template <auto V, typename... Content, typename... Ts, typename Parameters> static constexpr auto apply(pcre::set_make, ctll::term<V>, pcre_context<ctll::list<set<Content...>, Ts...>, Parameters> subject) {
	return pcre_context{ctll::push_front(set<Content...>(), ctll::list<Ts...>()), subject.parameters};
}
// set_make_negative
template <auto V, typename... Content, typename... Ts, typename Parameters> static constexpr auto apply(pcre::set_make_negative, ctll::term<V>, pcre_context<ctll::list<set<Content...>, Ts...>, Parameters> subject) {
	return pcre_context{ctll::push_front(negative_set<Content...>(), ctll::list<Ts...>()), subject.parameters};
}
// set{A...} + B = set{A,B}
template <auto V, typename A, typename... Content, typename... Ts, typename Parameters> static constexpr auto apply(pcre::set_combine, ctll::term<V>, pcre_context<ctll::list<A,set<Content...>,Ts...>, Parameters> subject) {
	auto new_set = push_back_into_set<set>(A(), set<Content...>());
	return pcre_context{ctll::push_front(new_set, ctll::list<Ts...>()), subject.parameters};
}

// negative_set{A...} + B = negative_set{A,B}
template <auto V, typename A, typename... Content, typename... Ts, typename Parameters> static constexpr auto apply(pcre::set_combine, ctll::term<V>, pcre_context<ctll::list<A,negative_set<Content...>,Ts...>, Parameters> subject) {
	auto new_set = push_back_into_set<set>(A(), set<Content...>());
	return pcre_context{ctll::push_front(new_set, ctll::list<Ts...>()), subject.parameters};
}

// negate_class_named: [[^:digit:]] = [^[:digit:]]
template <auto V, typename A, typename... Ts, typename Parameters> static constexpr auto apply(pcre::negate_class_named, ctll::term<V>, pcre_context<ctll::list<A, Ts...>, Parameters> subject) {
	return pcre_context{ctll::push_front(negate<A>(), ctll::list<Ts...>()), subject.parameters};
}

// add range to set
template <auto V, auto B, auto A, typename... Ts, typename Parameters> static constexpr auto apply(pcre::make_range, ctll::term<V>, pcre_context<ctll::list<character<B>,character<A>, Ts...>, Parameters> subject) {
	return pcre_context{ctll::push_front(char_range<A,B>(), ctll::list<Ts...>()), subject.parameters};
}

#endif
