/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <cstddef>
#include <algorithm>
#include "dxxsconf.h"
#include <array>

template <std::size_t N>
class cstring_tie
{
public:
	static const std::size_t maximum_arity = N;
	using array_t = std::array<const char *, maximum_arity>;
	template <typename... Args>
		cstring_tie(Args&&... args) : p(array_t{{args...}}), m_count(sizeof...(Args))
	{
		static_assert(sizeof...(Args) <= maximum_arity, "too many arguments to cstring_tie");
	}
	unsigned count() const { return m_count; }
	const char *string(std::size_t i) const { return p[i]; }
private:
	array_t p;
	unsigned m_count;
};
