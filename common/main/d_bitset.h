/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#pragma once
#include <bitset>

namespace dcx {

/* Wrap std::bitset similar to how enumerated_array wraps std::array.
 * See d_array.h for a full description.
 */
template <std::size_t N, typename E>
struct enumerated_bitset : std::bitset<N>
{
	using base_type = std::bitset<N>;
	constexpr typename base_type::reference operator[](E position)
	{
		return this->base_type::operator[](static_cast<std::size_t>(position));
	}
	constexpr bool operator[](E position) const
	{
		return this->base_type::operator[](static_cast<std::size_t>(position));
	}
	__attribute_warn_unused_result
	static constexpr bool valid_index(std::size_t s)
	{
		return s < N;
	}
	__attribute_warn_unused_result
	static constexpr bool valid_index(E e)
	{
		return valid_index(static_cast<std::size_t>(e));
	}
};

}
