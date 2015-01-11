/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once
#include <stdexcept>
#include <iterator>
#include <cstdio>
#include <string>
#include "dxxsconf.h"
#include "compiler-addressof.h"
#include "compiler-begin.h"
#include "compiler-type_traits.h"

template <typename I>
struct partial_range_t
{
	typedef I iterator;
	iterator m_begin, m_end;
	partial_range_t(iterator b, iterator e) :
		m_begin(b), m_end(e)
	{
	}
	iterator begin() const { return m_begin; }
	iterator end() const { return m_end; }
	bool empty() const __attribute_warn_unused_result
	{
		return m_begin == m_end;
	}
	std::reverse_iterator<iterator> rbegin() const __attribute_warn_unused_result { return std::reverse_iterator<iterator>{m_end}; }
	std::reverse_iterator<iterator> rend() const __attribute_warn_unused_result { return std::reverse_iterator<iterator>{m_begin}; }
	partial_range_t<std::reverse_iterator<iterator>> reversed() const __attribute_warn_unused_result
	{
		return {rbegin(), rend()};
	}
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
	template <std::size_t N>
		__attribute_cold
	static std::string prepare(const char *file, unsigned line, const char *estr, const char *desc, unsigned long expr, const void *t, unsigned long d)
	{
		char buf[(33 + (sizeof("18446744073709551615") * 2) + sizeof("0x0000000000000000") + N)];
		snprintf(buf, sizeof(buf), "%s:%u: %s %lu past %p end %lu \"%s\"", file, line, desc, expr, t, d, estr);
		return buf;
	}
	template <std::size_t NF, std::size_t NE, std::size_t ND>
		__attribute_cold
	static inline std::string prepare(const char (&file)[NF], unsigned line, const char (&estr)[NE], const char (&desc)[ND], unsigned long expr, const void *t, unsigned long d)
	{
		return prepare<((NF + NE + ND) | (sizeof(void *) - 1)) + 1>(file + 0, line, estr, desc, expr, t, d);
	}
};

template <typename T>
struct partial_range_error_t : base_partial_range_error_t
{
	DXX_INHERIT_CONSTRUCTORS(partial_range_error_t, base_partial_range_error_t);
	template <std::size_t NF, std::size_t NE, std::size_t ND>
		__attribute_cold
	static void report(const char (&file)[NF], unsigned line, const char (&estr)[NE], const char (&desc)[ND], unsigned long expr, const T &t, unsigned long d)
	{
		throw partial_range_error_t<T>(base_partial_range_error_t::prepare(file, line, estr, desc, expr, addressof(t), d));
	}
};

template <typename T, typename I, std::size_t NF, std::size_t NE>
static void check_partial_range(const char (&file)[NF], unsigned line, const char (&estr)[NE], T &t, I range_begin, I container_end, const std::size_t o, const std::size_t l)
{
	using std::distance;
#ifdef DXX_HAVE_BUILTIN_CONSTANT_P
#define PARTIAL_RANGE_COMPILE_CHECK_BOUND(EXPR,S)	\
	(__builtin_constant_p(EXPR) && __builtin_constant_p(d) && (DXX_ALWAYS_ERROR_FUNCTION(partial_range_will_always_throw, S " will always throw"), 0))
#else
#define PARTIAL_RANGE_COMPILE_CHECK_BOUND(EXPR,S)	0
#endif
#define PARTIAL_RANGE_CHECK_BOUND(EXPR,S)	\
	if (EXPR > d)	\
		((void)(PARTIAL_RANGE_COMPILE_CHECK_BOUND(EXPR,S))),	\
		partial_range_error_t<const T>::report(file, line, estr, S, EXPR, t, d)
	size_t d = distance(range_begin, container_end);
	PARTIAL_RANGE_CHECK_BOUND(o, "begin");
	PARTIAL_RANGE_CHECK_BOUND(l, "end");
#undef PARTIAL_RANGE_CHECK_BOUND
#undef PARTIAL_RANGE_COMPILE_CHECK_BOUND
}

template <typename I>
__attribute_warn_unused_result
static inline partial_range_t<I> unchecked_partial_range(I range_begin, const std::size_t o, const std::size_t l, tt::true_type)
{
#ifdef DXX_HAVE_BUILTIN_CONSTANT_P
	if (__builtin_constant_p(!(o < l)) && !(o < l))
		DXX_ALWAYS_ERROR_FUNCTION(partial_range_is_always_empty, "offset never less than length");
#endif
	auto range_end = range_begin;
	if (o <= l)
	{
		using std::advance;
		advance(range_begin, o);
		advance(range_end, l);
	}
	return {range_begin, range_end};
}

template <typename I>
static inline partial_range_t<I> unchecked_partial_range(I, std::size_t, std::size_t, tt::false_type) = delete;

template <typename I, typename UO, typename UL>
__attribute_warn_unused_result
static inline partial_range_t<I> unchecked_partial_range(I range_begin, const UO &o, const UL &l)
{
	/* Require unsigned length */
	typedef typename tt::conditional<tt::is_unsigned<UO>::value, tt::is_unsigned<UL>, tt::false_type>::type enable_type;
	return unchecked_partial_range<I>(range_begin, o, l, enable_type());
}

template <typename I, typename UL>
__attribute_warn_unused_result
static inline partial_range_t<I> unchecked_partial_range(I range_begin, const UL &l)
{
	return unchecked_partial_range<I, UL, UL>(range_begin, 0, l);
}

template <typename T, typename UO, typename UL, std::size_t NF, std::size_t NE, typename I = decltype(begin(*static_cast<T *>(nullptr)))>
__attribute_warn_unused_result
static inline partial_range_t<I> partial_range(const char (&file)[NF], unsigned line, const char (&estr)[NE], T &t, const UO &o, const UL &l)
{
	auto range_begin = begin(t);
	check_partial_range(file, line, estr, t, range_begin, end(t), o, l);
	return unchecked_partial_range<I, UO, UL>(range_begin, o, l);
}

template <typename T, typename UL, std::size_t NF, std::size_t NE, typename I = decltype(begin(*static_cast<T *>(nullptr)))>
__attribute_warn_unused_result
static inline partial_range_t<I> partial_range(const char (&file)[NF], unsigned line, const char (&estr)[NE], T &t, const UL &l)
{
	return partial_range<T, UL, UL, NF, NE, I>(file, line, estr, t, 0, l);
}

#define partial_range(T,...)	partial_range(__FILE__, __LINE__, #T, T, ##__VA_ARGS__)
