/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#pragma once
#include "dxxsconf.h"
#include <array>
#include "fwd-d_array.h"

namespace dcx {

/* Wrap a std::array and override the normal indexing operations to take
 * an instance of type `E`.  This is intended for use where `E` is an
 * instance of `enum class`.  When `E` is an `enum class`, instances of
 * `E` cannot implicitly convert to `std::array::size_type`, and so
 * would require a cast in the caller if this wrapper were not used.
 * Using this wrapper moves the casts into the wrapper, after the
 * compiler has checked that the argument is the right type of `enum
 * class`.  This prevents accidentally using an incorrect `enum class`.
 *
 * Other types for E are not likely to be useful, but are not blocked.
 */
template <typename T, std::size_t N, typename E>
struct enumerated_array : std::array<T, N>
{
	using base_type = std::array<T, N>;
	using typename base_type::reference;
	using typename base_type::const_reference;
	constexpr reference at(E position)
	{
		return this->base_type::at(static_cast<std::size_t>(position));
	}
	constexpr const_reference at(E position) const
	{
		return this->base_type::at(static_cast<std::size_t>(position));
	}
	constexpr reference operator[](E position)
	{
		return this->base_type::operator[](static_cast<std::size_t>(position));
	}
	constexpr const_reference operator[](E position) const
	{
		return this->base_type::operator[](static_cast<std::size_t>(position));
	}
	[[nodiscard]]
	static constexpr bool valid_index(std::size_t s)
	{
		return s < N;
	}
	[[nodiscard]]
	static constexpr bool valid_index(E e)
	{
		return valid_index(static_cast<std::size_t>(e));
	}
};

}
