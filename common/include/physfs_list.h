/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#pragma once
#include <physfs.h>

typedef char file_extension_t[5];

#ifdef __cplusplus
#include <cstdint>
#include <memory>
#include "null_sentinel_iterator.h"

#include "dxxsconf.h"
#include "dsx-ns.h"
#include "fwd-partial_range.h"
#include <array>
#include "backports-ranges.h"

namespace dcx {

class PHYSFS_list_deleter
{
public:
	void operator()(char **list) const
	{
		PHYSFS_freeList(list);
	}
};

template <typename Deleter>
class PHYSFSX_uncounted_list_template : protected std::unique_ptr<char *[], Deleter>
{
	using base_type = std::unique_ptr<char *[], Deleter>;
public:
	using base_type::base_type;
	using base_type::get;
	using base_type::operator bool;
	using base_type::operator[];
	using base_type::release;
	using base_type::reset;
	null_sentinel_iterator<char *> begin() const
	{
		return this->get();
	}
	null_sentinel_sentinel<char *> end() const
	{
		return {};
	}
};

template <typename Deleter>
class PHYSFSX_counted_list_template : public PHYSFSX_uncounted_list_template<Deleter>
{
	using base_type = PHYSFSX_uncounted_list_template<Deleter>;
	std::size_t count = 0;
public:
	/* Inherit the zero-argument form of reset(), for clearing a list.
	 * Override and delete the one-argument form, so that the list cannot be
	 * given new contents, with a potentially different length, using
	 * `reset(other_list);`
	 */
	using base_type::reset;
	void reset(auto) = delete;
	constexpr PHYSFSX_counted_list_template() = default;
	constexpr PHYSFSX_counted_list_template(base_type &&base, std::size_t count) :
		base_type{std::move(base)}, count{count}
	{
	}
	std::size_t get_count() const
	{
		return count;
	}
};

typedef PHYSFSX_uncounted_list_template<PHYSFS_list_deleter> PHYSFSX_uncounted_list;
typedef PHYSFSX_counted_list_template<PHYSFS_list_deleter> PHYSFSX_counted_list;

[[nodiscard]]
__attribute_nonnull()
PHYSFSX_uncounted_list PHYSFSX_findFiles(const char *path, ranges::subrange<const file_extension_t *> exts);

[[nodiscard]]
__attribute_nonnull()
PHYSFSX_uncounted_list PHYSFSX_findabsoluteFiles(const char *path, const char *realpath, ranges::subrange<const file_extension_t *> exts);

}
#endif
