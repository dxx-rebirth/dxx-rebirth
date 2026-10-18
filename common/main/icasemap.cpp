/*
 * This file is part of the DXX-Rebirth project <https://github.com/dxx-rebirth/dxx-rebirth/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#include <algorithm>
#include "icasemap.h"

namespace dcx {

/* Copy from `in` to `out`.  Write up to the first null in `in`, or at most
 * std::min(`in.size()`, `out.size()`) bytes, whichever happens first.  Fold
 * uppercase ANSI letters to lowercase during the copy.
 *
 * No null is written to `out` by this function, even if `in` contains a null
 * byte.  The caller must value-initialize the entire output buffer before
 * calling this function.
 */
void transform_ansi_filename_to_lowercase(const std::span<std::byte> out, std::span<const char> in)
{
	auto output_iter{out.begin()};
	/* Clamp span `in` to effective size of output buffer, which may be smaller
	 * than the actual output buffer due to the allocated size being rounded up
	 * to the next word.
	 */
	if (const auto s{out.size()}; in.size() > s) [[unlikely]]
		in = in.first(s);
	for (const char c : in)
	{
		std::byte b;
		/* Transform uppercase to lowercase.  Inputs are from game data, so are
		 * normally 7-bit ANSI text.  Inputs are not chosen by the player, so
		 * no internationalization support is needed.
		 */
		if (c >= 'A' && c <= 'Z') [[unlikely]]
			b = static_cast<std::byte>(c + 'a' - 'A');
		/* On null, stop.  The buffer was value-constructed before the loop, so
		 * there is no need to write the null byte into the output buffer
		 * separately.
		 */
		else if (!c) [[unlikely]]
			break;
		else
			b = static_cast<std::byte>(c);
		*output_iter++ = b;
	}
}

template <std::size_t maximum_filename_bytes>
void icase_filename_map<maximum_filename_bytes>::emplace(const std::span<const char> key, const int value)
{
	m.emplace(icase_key_pair{key, {}}, value);
}

template <std::size_t maximum_filename_bytes>
std::optional<int> icase_filename_map<maximum_filename_bytes>::find(const std::span<const char> key) const
{
	/* Construct an `icase_key_storage` from `key` for use by `std::map::find`.
	 * This allows the search to perform the case folding operation once per
	 * `find`, rather than once per comparison.
	 *
	 * If `key` is longer than `maximum_filename_bytes`, then only the first
	 * `maximum_filename_bytes` characters in `key` are used.  This differs
	 * from Descent's historical behavior, which would use the full length of
	 * the search key, and then fail to find any match because elements in the
	 * hashtable had been truncated to their maximum allowed length.  No code
	 * should depend on failing to find entries that are too long to be valid,
	 * and allowing the truncation in the case that should not happen makes the
	 * code simpler for the common case.
	 */
	if (const auto i{m.find({icase_key_pair{key, {}}})}; i != m.end()) [[likely]]
		return i->second;
	return std::nullopt;
}

template struct icase_filename_map<14>;	// digi_sound_filename_map

}
