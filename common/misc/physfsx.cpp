/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#include "dxxsconf.h"
#include "compiler-poison.h"
#include "physfsx.h"

namespace dcx {

PHYSFSX_fgets_t::result PHYSFSX_fgets_t::get(const std::span<char> buf, PHYSFS_File *const fp)
{
	/* Tell PHYSFS not to use the last byte of the buffer, so that this
	 * function can always write to the byte after the last byte read from the
	 * file.
	 */
	const PHYSFS_sint64 r{PHYSFSX_readBytes(fp, buf.data(), buf.size() - 1)};
	if (r <= 0)
	{
		DXX_POISON_MEMORY(buf, 0xcc);
		return {};
	}
	const auto bb{buf.begin()};
	auto p{bb};
	std::ptrdiff_t buffer_end_adjustment{0};
	for (const auto e{std::next(bb, r)}; p != e; ++p)
	{
		if (const char c{*p}; c == 0 || c == '\n')
		{
			/* Do nothing - and skip the `continue` below that handles most
			 * characters.
			 */
		}
		else if (c == '\r')
		{
			/* If a Carriage Return is found, stop here.  If the next byte is a
			 * newline, then update `p` to consider the newline included in the
			 * string, so that the seek does not cause the newline to replay on
			 * the next call.  Otherwise, leave `p` such that the seek will
			 * cause that next byte to replay on the next call.
			 */
			*p = 0;
			if (const auto p2{std::next(p)}; p2 != e && *p2 == '\n')
			{
				p = p2;
				/* Increment `buffer_end_adjustment` so that the returned span does
				 * not include the null that was previously a Carriage Return.
				 */
				++ buffer_end_adjustment;
			}
		}
		else
			continue;
		PHYSFS_seek(fp, PHYSFS_tell(fp) + p - e + 1);
		break;
	}
	*p = 0;
	DXX_POISON_MEMORY(buf.subspan((p + 1) - bb), 0xcc);
	return {bb, p - buffer_end_adjustment};
}

}
