/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
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
#include "compiler-array.h"

class PHYSFS_list_deleter
{
public:
	void operator()(char **list) const
	{
		PHYSFS_freeList(list);
	}
};

template <typename D>
class PHYSFS_list_template : public std::unique_ptr<char *[], D>
{
	typedef std::unique_ptr<char *[], D> base_ptr;
public:
	typedef null_sentinel_iterator<char *> const_iterator;
	DXX_INHERIT_CONSTRUCTORS(PHYSFS_list_template, base_ptr);
	const_iterator begin() const
	{
		return this->get();
	}
	const_iterator end() const
	{
		return {};
	}
};

typedef PHYSFS_list_template<PHYSFS_list_deleter> PHYSFS_list_t;

__attribute_nonnull()
__attribute_warn_unused_result
PHYSFS_list_t PHYSFSX_findFiles(const char *path, const file_extension_t *exts, uint_fast32_t count);

template <std::size_t count>
__attribute_nonnull()
__attribute_warn_unused_result
static inline PHYSFS_list_t PHYSFSX_findFiles(const char *path, const array<file_extension_t, count> &exts)
{
	return PHYSFSX_findFiles(path, exts.data(), count);
}

__attribute_nonnull()
__attribute_warn_unused_result
PHYSFS_list_t PHYSFSX_findabsoluteFiles(const char *path, const char *realpath, const file_extension_t *exts, uint_fast32_t count);

template <std::size_t count>
__attribute_nonnull()
__attribute_warn_unused_result
static inline PHYSFS_list_t PHYSFSX_findabsoluteFiles(const char *path, const char *realpath, const array<file_extension_t, count> &exts)
{
	return PHYSFSX_findabsoluteFiles(path, realpath, exts.data(), count);
}
#endif
