/*
 * This file is part of the DXX-Rebirth project <https://github.com/dxx-rebirth/dxx-rebirth/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <span>

namespace dcx {

void transform_ansi_filename_to_lowercase(std::span<std::byte> out, std::span<const char> in);

template <std::size_t maximum_filename_words>
class icase_key_storage
{
	/* Store the key in `filename`.  Compare instances of `icase_key_storage`
	 * by comparing their `padded_storage`, so that the compiler can perform
	 * comparisons in blocks, rather than one `std::byte` at a time.
	 */
	union {
		std::array<std::byte, maximum_filename_words * sizeof(std::uintptr_t)> filename;
		std::array<std::uintptr_t, maximum_filename_words> padded_storage{};
	};
public:
	/* Copy up to std::min(`in.first.size()`, `maximum_filename_bytes`) bytes
	 * into `filename`.
	 *
	 * `maximum_filename_bytes` is required to be at most `filename.size()`,
	 * but callers could pass a smaller value.
	 */
	template <std::size_t maximum_filename_bytes>
		icase_key_storage(const std::pair<std::span<const char>, std::integral_constant<std::size_t, maximum_filename_bytes>> in)
		{
			transform_ansi_filename_to_lowercase(std::span(filename).template first<maximum_filename_bytes>(), in.first);
		}
	constexpr auto operator<=>(const icase_key_storage &rhs) const
	{
		return padded_storage <=> rhs.padded_storage;
	}
};

template <std::size_t maximum_filename_bytes>
struct icase_filename_map
{
	std::map<icase_key_storage<(maximum_filename_bytes + sizeof(std::uintptr_t) - 1) / sizeof(std::uintptr_t)>, int> m;
	/* Define a convenience type alias that can construct `icase_key_storage`
	 * with the correct `maximum_filename_bytes`.
	 */
	using icase_key_pair = std::pair<std::span<const char>, std::integral_constant<std::size_t, maximum_filename_bytes>>;
	void emplace(std::span<const char> key, int value);
	std::optional<int> find(std::span<const char> key) const;
};

}
