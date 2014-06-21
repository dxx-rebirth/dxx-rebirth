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

template <typename T>
struct partial_range_error_t : public std::out_of_range
{
	partial_range_error_t(const std::string& s) :
		std::out_of_range(s)
	{
	}
	static void report(const char *desc, unsigned long expr, const T &t, unsigned long d)
	{
		char buf[84];
		snprintf(buf, sizeof(buf), "%s %lu past %p end %lu", desc, expr, addressof(t), d);
		throw partial_range_error_t<T>(buf);
	}
};

template <typename T, typename U>
typename tt::enable_if<!tt::is_unsigned<U>::value, T &>::type partial_range(T &, U, U = U()) DXX_CXX11_EXPLICIT_DELETE;

template <typename T, typename U>
static inline typename tt::enable_if<tt::is_unsigned<U>::value, partial_range_t<typename tt::conditional<tt::is_const<T>::value, typename T::const_iterator, typename T::iterator>::type>>::type partial_range(T &t, const U o, const U l) __attribute_warn_unused_result;

template <typename T, typename U>
static inline typename tt::enable_if<tt::is_unsigned<U>::value, partial_range_t<typename tt::conditional<tt::is_const<T>::value, typename T::const_iterator, typename T::iterator>::type>>::type partial_range(T &t, const U o, const U l)
{
	using std::advance;
	using std::distance;
	auto range_begin = begin(t);
	auto range_end = range_begin;
#define PARTIAL_RANGE_CHECK_BOUND(EXPR,S)	\
	if (EXPR > d)	\
		partial_range_error_t<const T>::report(S, EXPR, t, d)
	size_t d = distance(range_begin, end(t));
	PARTIAL_RANGE_CHECK_BOUND(o, "begin");
	PARTIAL_RANGE_CHECK_BOUND(l, "end");
#undef PARTIAL_RANGE_CHECK_BOUND
	if (o <= l)
	{
		advance(range_begin, o);
		advance(range_end, l);
	}
	return partial_range_t<typename tt::conditional<tt::is_const<T>::value, typename T::const_iterator, typename T::iterator>::type>(range_begin, range_end);
}

template <typename T, typename U>
static inline typename tt::enable_if<tt::is_unsigned<U>::value, partial_range_t<typename tt::conditional<tt::is_const<T>::value, typename T::const_iterator, typename T::iterator>::type>>::type partial_range(T &t, const U l) __attribute_warn_unused_result;

template <typename T, typename U>
static inline typename tt::enable_if<tt::is_unsigned<U>::value, partial_range_t<typename tt::conditional<tt::is_const<T>::value, typename T::const_iterator, typename T::iterator>::type>>::type partial_range(T &t, const U l)
{
	return partial_range(t, static_cast<U>(0), l);
}
