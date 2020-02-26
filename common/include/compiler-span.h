/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <cstddef>

/* Partial implementation of std::span (C++20) for pre-C++20 compilers.
 * As of this writing, there are no released compilers with C++20
 * support, so this implementation is always used.
 *
 * This implementation covers only the minimal functionality used by
 * Rebirth.  It is not intended as a drop-in replacement for arbitrary
 * use of std::span.
 */

template <typename T>
class span
{
	std::size_t extent;
	T *ptr;
public:
	span(T *p, std::size_t l) :
		extent(l), ptr(p)
	{
	}
	T *begin() const
	{
		return ptr;
	}
	T *end() const
	{
		return ptr + extent;
	}
	std::size_t size() const
	{
		return extent;
	}
	T &operator[](std::size_t i) const
	{
		return *(ptr + i);
	}
};
