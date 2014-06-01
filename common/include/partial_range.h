/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once
#include "dxxsconf.h"
#include "compiler-begin.h"

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
partial_range_t<typename T::iterator> partial_range(T &t, typename T::size_type l)
{
	auto b = begin(t);
	return partial_range_t<typename T::iterator>(b, b + l);
}

template <typename T>
partial_range_t<typename T::const_iterator> partial_range(const T &t, typename T::size_type l)
{
	auto b = begin(t);
	return partial_range_t<typename T::const_iterator>(b, b + l);
}
