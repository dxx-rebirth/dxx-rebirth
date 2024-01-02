/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include <cstddef>
#include "dxxsconf.h"
#include <array>

template <std::size_t N>
class cstring_tie : std::array<const char *, N>
{
public:
	static const std::size_t maximum_arity = N;
	using array_t = std::array<const char *, maximum_arity>;
	template <typename... Args>
		requires(sizeof...(Args) <= maximum_arity)
		cstring_tie(Args&&... args) :
			array_t{{args...}}, m_count{sizeof...(Args)}
	{
	}
	unsigned count() const { return m_count; }
	const char *string(std::size_t i) const { return this->operator[](i); }
	using array_t::begin;
	auto end() const
	{
		return std::next(begin(), m_count);
	}
private:
	unsigned m_count;
};
