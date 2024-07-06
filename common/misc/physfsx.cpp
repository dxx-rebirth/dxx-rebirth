/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#include "dxxsconf.h"
#include "args.h"
#include "compiler-poison.h"
#include "console.h"
#include "d_enumerate.h"
#include "ignorecase.h"
#include "physfs_list.h"
#include "physfsx.h"
#include "strutil.h"

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

const file_extension_t *PHYSFSX_checkMatchingExtension(const char *filename, const std::ranges::subrange<const file_extension_t *> range)
{
	const char *ext{strrchr(filename, '.')};
	if (!ext)
		return nullptr;
	++ext;
	// see if the file is of a type we want
	for (auto &k : range)
	{
		if (!d_stricmp(ext, k))
			return &k;
	}
	return nullptr;
}

// Add a searchpath, but that searchpath is relative to an existing searchpath
// It will add the first one it finds and return a PhysFS error code, if it doesn't find any it returns PHYSFS_ERR_OK
PHYSFS_ErrorCode PHYSFSX_addRelToSearchPath(char *const relname2, std::array<char, PATH_MAX> &pathname, physfs_search_path add_to_end)
{
	PHYSFSEXT_locateCorrectCase(relname2);

	if (!PHYSFSX_getRealPath(relname2, pathname))
	{
		/* This failure is not reported as an error, because callers
		 * probe for files that users may not have, and do not need.
		 */
		con_printf(CON_DEBUG, "PHYSFS: ignoring map request: no canonical path for relative name \"%s\"", relname2);
		return PHYSFS_ERR_OK;
	}

	auto r = PHYSFS_mount(pathname.data(), nullptr, static_cast<int>(add_to_end));
	const auto action = add_to_end != physfs_search_path::prepend ? "append" : "insert";
	if (r)
	{
		con_printf(CON_DEBUG, "PHYSFS: %s canonical directory \"%s\" to search path from relative name \"%s\"", action, pathname.data(), relname2);
		return PHYSFS_ERR_OK;
	}
	else
	{
		const auto err = PHYSFS_getLastErrorCode();
		con_printf(CON_VERBOSE, "PHYSFS: failed to %s canonical directory \"%s\" to search path from relative name \"%s\": \"%s\"", action, pathname.data(), relname2, PHYSFS_getErrorByCode(err));
		return err;
	}
}

void PHYSFSX_removeRelFromSearchPath(const char *relname)
{
	char relname2[PATH_MAX];

	snprintf(relname2, sizeof(relname2), "%s", relname);
	PHYSFSEXT_locateCorrectCase(relname2);

	std::array<char, PATH_MAX> pathname;
	if (!PHYSFSX_getRealPath(relname2, pathname))
	{
		con_printf(CON_DEBUG, "PHYSFS: ignoring unmap request: no canonical path for relative name \"%s\"", relname2);
		return;
	}
	auto r = PHYSFS_unmount(pathname.data());
	if (r)
		con_printf(CON_DEBUG, "PHYSFS: unmap canonical directory \"%s\" (relative name \"%s\")", pathname.data(), relname);
	else
		con_printf(CON_VERBOSE, "PHYSFS: failed to unmap canonical directory \"%s\" (relative name \"%s\"): \"%s\"", pathname.data(), relname, PHYSFS_getLastError());
}

int PHYSFSX_fsize(const char *hogname)
{
	char hogname2[PATH_MAX];

	snprintf(hogname2, sizeof(hogname2), "%s", hogname);
	PHYSFSEXT_locateCorrectCase(hogname2);

	if (RAIIPHYSFS_File fp{PHYSFS_openRead(hogname2)})
		return PHYSFS_fileLength(fp);
	return -1;
}

void PHYSFSX_listSearchPathContent()
{
	if (!(CON_DEBUG <= CGameArg.DbgVerbose))
		/* Skip retrieving the path contents if the verbosity level will
		 * prevent showing the results.
		 */
		return;
	con_puts(CON_DEBUG, "PHYSFS: Listing contents of Search Path.");
	if (const auto s{PHYSFSX_uncounted_list{PHYSFS_getSearchPath()}})
		for (const auto &&[idx, entry] : enumerate(s))
			con_printf(CON_DEBUG, "PHYSFS: search path entry #%" DXX_PRI_size_type ": %s", idx, entry);
	if (const auto s{PHYSFSX_uncounted_list{PHYSFS_enumerateFiles("")}})
		for (const auto &&[idx, entry] : enumerate(s))
			con_printf(CON_DEBUG, "PHYSFS: found file #%" DXX_PRI_size_type ": %s", idx, entry);
}

int PHYSFSX_getRealPath(const char *stdPath, std::array<char, PATH_MAX> &realPath)
{
	DXX_POISON_MEMORY(std::span(realPath), 0xdd);
	const char *realDir = PHYSFS_getRealDir(stdPath);
	if (!realDir)
	{
		realDir = PHYSFS_getWriteDir();
		if (!realDir)
			return 0;
	}
	const auto realDirSize = strlen(realDir);
	if (realDirSize >= realPath.size())
		return 0;
	auto mountpoint = PHYSFS_getMountPoint(realDir);
	if (!mountpoint)
		return 0;
	std::copy_n(realDir, realDirSize + 1, realPath.begin());
#ifdef _unix__
	auto &sep = "/";
	assert(!strcmp(PHYSFS_getDirSeparator(), sep));
#else
	const auto sep = PHYSFS_getDirSeparator();
#endif
	const auto sepSize = strlen(sep);
	auto realPathUsed = realDirSize;
	if (realDirSize >= sepSize)
	{
		const auto p = std::next(realPath.begin(), realDirSize - sepSize);
		if (strcmp(p, sep)) // no sep at end of realPath
		{
			realPathUsed += sepSize;
			std::copy_n(sep, sepSize, &realPath[realDirSize]);
		}
	}
	if (*mountpoint == '/')
		++mountpoint;
	if (*stdPath == '/')
		++stdPath;
	const auto ml = strlen(mountpoint);
	if (!strncmp(mountpoint, stdPath, ml))
		stdPath += ml;
	else
	{
		/* Virtual path is not under the virtual mount point that
		 * provides the path.
		 */
		assert(false);
	}
	const auto stdPathLen = strlen(stdPath) + 1;
	if (realPathUsed + stdPathLen >= realPath.size())
		return 0;
#ifdef __unix__
	/* Separator is "/" and physfs internal separator is "/".  Copy
	 * through.
	 */
	std::copy_n(stdPath, stdPathLen, &realPath[realPathUsed]);
#else
	/* Separator might be / on non-unix, but the fallback path works
	 * regardless of whether separator is "/".
	 */
	const auto csep = *sep;
	const auto a = [csep](char c) {
		return c == '/' ? csep : c;
	};
	std::transform(stdPath, &stdPath[stdPathLen], &realPath[realPathUsed], a);
#endif
	return 1;
}

}
