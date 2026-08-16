/*
 * This file is part of the DXX-Rebirth project <https://github.com/dxx-rebirth/dxx-rebirth/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#pragma once
#include "dxxsconf.h"
#include <array>
#include <optional>
#include "fwd-d_array.h"

namespace dcx {

template <std::size_t N, typename E>
struct enumerated_array_length_operations
{
protected:
	using index_type = E;
	[[nodiscard]]
	static constexpr std::optional<index_type> valid_index(const std::size_t s)
	{
		/* static_cast<index_type> is necessary here, since
		 * `sizeof(index_type) < sizeof(std::size_t)` is supported and commonly
		 * used.
		 *
		 * The narrowing rule is stateless, and therefore does not observe that
		 * the cast is on a path where `s < N` ensures no narrowing can occur.
		 */
		if (s < N) [[likely]]
			return std::optional{static_cast<index_type>(s)};
		return std::nullopt;
	}
	[[nodiscard]]
	static constexpr std::optional<index_type> valid_index(const index_type e)
	{
		if (static_cast<std::size_t>(e) < N) [[likely]]
			return std::optional{e};
		return std::nullopt;
	}
};

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
struct enumerated_array :
	std::array<T, N>,
	enumerated_array_length_operations<N, E>
{
	using base_type = std::array<T, N>;
	using operations_type = enumerated_array_length_operations<N, E>;
	using typename operations_type::index_type;
	using typename base_type::reference;
	using typename base_type::const_reference;
	constexpr reference at(index_type position) = delete;
	constexpr const_reference at(index_type position) const = delete;
	[[nodiscard]]
	constexpr decltype(auto) operator[](this auto &self, E position)
	{
		return self.base_type::operator[](static_cast<std::size_t>(position));
	}
	/* Reject implicit conversions from integer types to `E`.  Some
	 * instantiations use an `E` that cannot be implicitly constructed from an
	 * integer.  Others use one that can.  Ensure that no implicit conversions
	 * are done, by providing an explicitly deleted overload that matches
	 * integers.
	 */
	const_reference operator[](std::integral auto) const = delete;
	using operations_type::valid_index;
};

}
