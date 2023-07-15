/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#pragma once
#include <bitset>
#include <optional>

namespace dcx {

/* Wrap std::bitset similar to how enumerated_array wraps std::array.
 * See d_array.h for a full description.
 */
template <std::size_t N, typename E>
class enumerated_bitset : std::bitset<N>
{
	using base_type = std::bitset<N>;
public:
	using base_type::base_type;
	using base_type::size;
	constexpr typename base_type::reference operator[](E position)
	{
		return this->base_type::operator[](static_cast<std::size_t>(position));
	}
	constexpr bool operator[](E position) const
	{
		return this->base_type::operator[](static_cast<std::size_t>(position));
	}
	enumerated_bitset &reset()
	{
		return static_cast<enumerated_bitset &>(this->base_type::reset());
	}
	enumerated_bitset &reset(E position)
	{
		return static_cast<enumerated_bitset &>(this->base_type::reset(static_cast<std::size_t>(position)));
	}
	enumerated_bitset &set(E position)
	{
		return static_cast<enumerated_bitset &>(this->base_type::set(static_cast<std::size_t>(position)));
	}
	[[nodiscard]]
	static constexpr std::optional<E> valid_index(const std::size_t s)
	{
		return s < N ? std::optional(static_cast<E>(s)) : std::nullopt;
	}
	[[nodiscard]]
	static constexpr bool valid_index(E e)
	{
		return static_cast<std::size_t>(e) < N;
	}
};

}
