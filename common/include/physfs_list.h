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

template <typename D>
class PHYSFSX_uncounted_list_template : public std::unique_ptr<char *[], D>
{
public:
	typedef null_sentinel_iterator<char *> const_iterator;
	using std::unique_ptr<char *[], D>::unique_ptr;
	const_iterator begin() const
	{
		return this->get();
	}
	const_iterator end() const
	{
		return {};
	}
};

template <typename D>
class PHYSFSX_counted_list_template : public PHYSFSX_uncounted_list_template<D>
{
	using base_type = PHYSFSX_uncounted_list_template<D>;
	std::size_t count = 0;
public:
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
