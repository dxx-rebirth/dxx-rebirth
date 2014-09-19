/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once
#include <stdexcept>
#include <cstdio>
#include "dxxsconf.h"
#include "compiler-addressof.h"
#include "compiler-begin.h"
#include "compiler-type_traits.h"

template <typename iterator>
struct partial_range_t
{
	iterator m_begin, m_end;
	partial_range_t(iterator b, iterator e) :
		m_begin(b), m_end(e)
	{
	}
	iterator begin() const { return m_begin; }
	iterator end() const { return m_end; }
};

#ifdef DXX_HAVE_BOOST_FOREACH
#include <boost/range/const_iterator.hpp>
#include <boost/range/mutable_iterator.hpp>

namespace boost
{
	template <typename iterator>
		struct range_mutable_iterator< partial_range_t<iterator> >
		{
			typedef iterator type;
		};

	template <typename iterator>
		struct range_const_iterator< partial_range_t<iterator> >
		{
			typedef iterator type;
		};
}
#endif

struct base_partial_range_error_t : std::out_of_range
{
	DXX_INHERIT_CONSTRUCTORS(base_partial_range_error_t, std::out_of_range);
	static std::string prepare(const char *file, unsigned line, const char *estr, const char *desc, unsigned long expr, const void *t, unsigned long d)
	{
		char buf[256];
		snprintf(buf, sizeof(buf), "%s:%u: %s %lu past %p end %lu \"%s\"", file, line, desc, expr, t, d, estr);
		return buf;
	}
};

template <typename T>
struct partial_range_error_t : base_partial_range_error_t
{
	DXX_INHERIT_CONSTRUCTORS(partial_range_error_t, base_partial_range_error_t);
	static void report(const char *file, unsigned line, const char *estr, const char *desc, unsigned long expr, const T &t, unsigned long d)
	{
		throw partial_range_error_t<T>(base_partial_range_error_t::prepare(file, line, estr, desc, expr, addressof(t), d));
	}
};

template <typename T, typename U>
typename tt::enable_if<!tt::is_unsigned<U>::value, T &>::type partial_range(const char *, unsigned, const char *, T &, U, U = U()) = delete;

template <typename T, typename U, typename I = decltype(begin(*(T *)0))>
static inline typename tt::enable_if<tt::is_unsigned<U>::value, partial_range_t<I>>::type partial_range(const char *, unsigned, const char *, T &t, const U &o, const U &l) __attribute_warn_unused_result;

template <typename T, typename U, typename I>
static inline typename tt::enable_if<tt::is_unsigned<U>::value, partial_range_t<I>>::type partial_range(const char *file, unsigned line, const char *estr, T &t, const U &uo, const U &ul)
{
	using std::advance;
	using std::distance;
	auto range_begin = begin(t);
	auto range_end = range_begin;
	size_t o = uo, l = ul;
#ifdef DXX_HAVE_BUILTIN_CONSTANT_P
#define PARTIAL_RANGE_COMPILE_CHECK_BOUND(EXPR,S)	\
	(__builtin_constant_p(EXPR) && __builtin_constant_p(d) && (DXX_ALWAYS_ERROR_FUNCTION(partial_range_will_always_throw, S " will always throw"), 0))
	if (__builtin_constant_p(!(o < l)) && !(o < l))
		DXX_ALWAYS_ERROR_FUNCTION(partial_range_is_always_empty, "offset never less than length");
#else
#define PARTIAL_RANGE_COMPILE_CHECK_BOUND(EXPR,S)	0
#endif
#define PARTIAL_RANGE_CHECK_BOUND(EXPR,S)	\
	if (EXPR > d)	\
		((void)(PARTIAL_RANGE_COMPILE_CHECK_BOUND(EXPR,S))),	\
		partial_range_error_t<const T>::report(file, line, estr, S, EXPR, t, d)
	size_t d = distance(range_begin, end(t));
	PARTIAL_RANGE_CHECK_BOUND(o, "begin");
	PARTIAL_RANGE_CHECK_BOUND(l, "end");
#undef PARTIAL_RANGE_CHECK_BOUND
	if (o <= l)
	{
		advance(range_begin, o);
		advance(range_end, l);
	}
	return {range_begin, range_end};
}

template <typename T, typename U, typename I = decltype(begin(*(T *)0))>
static inline typename tt::enable_if<tt::is_unsigned<U>::value, partial_range_t<I>>::type partial_range(const char *, unsigned, const char *, T &t, const U &l) __attribute_warn_unused_result;

template <typename T, typename U, typename I>
static inline typename tt::enable_if<tt::is_unsigned<U>::value, partial_range_t<I>>::type partial_range(const char *file, unsigned line, const char *estr, T &t, const U &l)
{
	return partial_range(file, line, estr, t, static_cast<U>(0), l);
}

#define partial_range(T,...)	partial_range(__FILE__, __LINE__, #T, T, ##__VA_ARGS__)
