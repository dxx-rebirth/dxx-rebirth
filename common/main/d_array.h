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
/* `std::size_t` is `unsigned` on i686-w64-mingw32, and `unsigned long` on
 * x86_64-pc-linux-gnu.
 *
 * As a result, if `E` is `unsigned`, then i686-w64-mingw32 breaks due to
 * `operator[](E)` and `operator[](std::size_t)` being the same signature.  If
 * `E` is `unsigned long`, then x86_64-pc-linux-gnu breaks for the same reason.
 * Disallow both `unsigned` and `unsigned long`, so that any attempt to use
 * either for `E` will break everywhere, rather than breaking only some
 * platforms.
 */
requires(!std::is_same<unsigned, E>::value && !std::is_same<unsigned long, E>::value)
struct enumerated_array : std::array<T, N>
{
	using base_type = std::array<T, N>;
	using index_type = E;
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
	template <typename I>
		requires(std::is_integral_v<I>)
		const_reference operator[](I) const = delete;
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
