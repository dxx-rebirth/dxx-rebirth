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
	DXX_INHERIT_CONSTRUCTORS(PHYSFSX_uncounted_list_template, std::unique_ptr<char *[], D>);
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
	typedef PHYSFSX_uncounted_list_template<D> base_ptr;
	using typename base_ptr::pointer;
	uint_fast32_t count = 0;
public:
	using base_ptr::base_ptr;
	uint_fast32_t get_count() const
	{
		return count;
	}
	void set_count(uint_fast32_t c)
	{
		count = c;
	}
	void reset()
	{
		base_ptr::reset();
	}
	void reset(pointer p)
	{
		count = 0;
		base_ptr::reset(p);
	}
};

typedef PHYSFSX_uncounted_list_template<PHYSFS_list_deleter> PHYSFSX_uncounted_list;
typedef PHYSFSX_counted_list_template<PHYSFS_list_deleter> PHYSFSX_counted_list;

[[nodiscard]]
__attribute_nonnull()
PHYSFSX_uncounted_list PHYSFSX_findFiles(const char *path, partial_range_t<const file_extension_t *> exts);

[[nodiscard]]
__attribute_nonnull()
PHYSFSX_uncounted_list PHYSFSX_findabsoluteFiles(const char *path, const char *realpath, const partial_range_t<const file_extension_t *> exts);

}
#endif
