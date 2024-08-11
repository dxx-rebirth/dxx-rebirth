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
#include "newdemo.h"
#include "physfs_list.h"
#include "physfsx.h"
#include "strutil.h"

namespace dcx {

namespace {

constexpr std::array<file_extension_t, 1> archive_exts{{"dxa"}};

std::pair<RAIINamedPHYSFS_File, PHYSFS_ErrorCode> PHYSFSX_openReadBuffered_internal(char *const filename, const char *const reported_filename)
{
	PHYSFS_uint64 bufSize;
	if (PHYSFSEXT_locateCorrectCase(filename) != PHYSFSX_case_search_result::success)
		return {RAIINamedPHYSFS_File{}, PHYSFS_ERR_NOT_FOUND};
	/* Use the specified filename in any error messages.  This may be the
	 * modified filename or may be the original, depending on whether the
	 * modified filename is a temporary.  Even if the original filename is
	 * used, it should be close enough that the user should be able to identify
	 * the bad file, and avoids the need to allocate and return storage for the
	 * filename.  In almost all cases, the filename will never be seen, since
	 * it is only shown if an error occurs when reading the file.
	 */
	RAIINamedPHYSFS_File fp{PHYSFS_openRead(filename), reported_filename};
	if (!fp)
		return {std::move(fp), PHYSFS_getLastErrorCode()};
	bufSize = PHYSFS_fileLength(fp);
	while (!PHYSFS_setBuffer(fp, bufSize) && bufSize)
		bufSize /= 2;	// even if the error isn't memory full, for a 20MB file it'll only do this 8 times
	return {std::move(fp), PHYSFS_ERR_OK};
}

}

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
	if (PHYSFSEXT_locateCorrectCase(relname2) != PHYSFSX_case_search_result::success ||
		!PHYSFSX_getRealPath(relname2, pathname))
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

void PHYSFSX_removeRelFromSearchPath(char *const relname)
{
	std::array<char, PATH_MAX> pathname;
	if (PHYSFSEXT_locateCorrectCase(relname) != PHYSFSX_case_search_result::success ||
		!PHYSFSX_getRealPath(relname, pathname))
	{
		con_printf(CON_DEBUG, "PHYSFS: ignoring unmap request: no canonical path for relative name \"%s\"", relname);
		return;
	}
	auto r = PHYSFS_unmount(pathname.data());
	if (r)
		con_printf(CON_DEBUG, "PHYSFS: unmap canonical directory \"%s\" (relative name \"%s\")", pathname.data(), relname);
	else
		con_printf(CON_VERBOSE, "PHYSFS: failed to unmap canonical directory \"%s\" (relative name \"%s\"): \"%s\"", pathname.data(), relname, PHYSFS_getLastError());
}

int PHYSFSX_fsize(char *const filename)
{
	if (PHYSFSEXT_locateCorrectCase(filename) != PHYSFSX_case_search_result::success)
	{
		/* Fall through to the final return. */
	}
	else if (RAIIPHYSFS_File fp{PHYSFS_openRead(filename)})
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

int PHYSFSX_rename(const char *oldpath, const char *newpath)
{
	std::array<char, PATH_MAX> old, n;
	if (!PHYSFSX_getRealPath(oldpath, old) || !PHYSFSX_getRealPath(newpath, n))
		return -1;
	return (rename(old.data(), n.data()) == 0);
}

static PHYSFSX_uncounted_list trim_physfs_list(PHYSFSX_uncounted_list list, char **const iter_first_unused)
{
	*iter_first_unused = nullptr;
	const auto old_begin = list.get();
	const auto r = reinterpret_cast<char **>(realloc(old_begin, (iter_first_unused - old_begin + 1) * sizeof(char *)));	// save a bit of memory (or a lot?)
	/* iter_first_unused, old_begin are now invalid since realloc may have moved the block */
	return r ? PHYSFSX_uncounted_list{(list.release(), r)} : std::move(list);
}

static PHYSFSX_uncounted_list PHYSFSX_findPredicateFiles(const char *path, auto &&predicate)
{
	PHYSFSX_uncounted_list list{PHYSFS_enumerateFiles(path)};
	if (!list)
		return nullptr;	// out of memory: not so good
	char **j = list.get();
	for (const auto i : list)
	{
		if (predicate(i))
			*j++ = i;
		else
			free(i);
	}
	return trim_physfs_list(std::move(list), j);
}

// Find files at path that have an extension listed in exts
// The extension list exts must be NULL-terminated, with each ext beginning with a '.'
PHYSFSX_uncounted_list PHYSFSX_findFiles(const char *path, const std::ranges::subrange<const file_extension_t *> exts)
{
	const auto predicate = [&](const char *i) {
		return PHYSFSX_checkMatchingExtension(i, exts);
	};
	return PHYSFSX_findPredicateFiles(path, predicate);
}

// Same function as above but takes a real directory as second argument, only adding files originating from this directory.
// This can be used to further seperate files in search path but it must be made sure realpath is properly formatted.
PHYSFSX_uncounted_list PHYSFSX_findabsoluteFiles(const char *path, const char *realpath, const std::ranges::subrange<const file_extension_t *> exts)
{
	const auto predicate = [&](const char *i) {
		return PHYSFSX_checkMatchingExtension(i, exts) && (!strcmp(PHYSFS_getRealDir(i), realpath));
	};
	return PHYSFSX_findPredicateFiles(path, predicate);
}

int PHYSFSX_exists_ignorecase(char *filename)
{
	return PHYSFSEXT_locateCorrectCase(filename) == PHYSFSX_case_search_result::success;
}

//Open a file for reading, set up a buffer
std::pair<RAIINamedPHYSFS_File, PHYSFS_ErrorCode> PHYSFSX_openReadBuffered_updateCase(char *const filename)
{
	/* The caller provided the filename buffer, so use that buffer for the
	 * reported filename.
	 */
	return PHYSFSX_openReadBuffered_internal(filename, filename);
}

//Open a file for reading, set up a buffer
std::pair<RAIINamedPHYSFS_File, PHYSFS_ErrorCode> PHYSFSX_openReadBuffered(const char *filename)
{
	char filename2[PATH_MAX];
	snprintf(filename2, sizeof(filename2), "%s", filename);
	/* Use the original filename in any error messages.  This is close enough
	 * that the user should be able to identify the bad file, and avoids the
	 * need to allocate and return storage for the filename.  In almost all
	 * cases, the filename will never be seen, since it is only shown if an
	 * error occurs when reading the file.
	 */
	return PHYSFSX_openReadBuffered_internal(filename2, filename);
}

//Open a file for writing, set up a buffer
std::pair<RAIIPHYSFS_File, PHYSFS_ErrorCode> PHYSFSX_openWriteBuffered(const char *filename)
{
	PHYSFS_uint64 bufSize = 1024*1024;	// hmm, seems like an OK size.
	RAIIPHYSFS_File fp{PHYSFS_openWrite(filename)};
	if (!fp)
		return {nullptr, PHYSFS_getLastErrorCode()};
	while (!PHYSFS_setBuffer(fp, bufSize) && bufSize)
		bufSize /= 2;
	return {std::move(fp), PHYSFS_ERR_OK};
}

static uint8_t add_archives_to_search_path()
{
	uint8_t content_updated{};
	// find files in Searchpath ...
	// if found, add them...
	const auto s{PHYSFSX_findFiles("", archive_exts)};
	if (!s)
		return content_updated;
	for (const auto i : s)
	{
		std::array<char, PATH_MAX> realfile;
		if (PHYSFSX_getRealPath(i, realfile) && PHYSFS_mount(realfile.data(), nullptr, 0))
		{
			con_printf(CON_DEBUG, "PHYSFS: Added %s to Search Path",realfile.data());
			content_updated = 1;
		}
	}
	return content_updated;
}

static uint8_t add_demo_files_to_search_path(uint8_t content_updated)
{
	// find files in DEMO_DIR ...
	// if found, add them...
	for (const auto i : PHYSFSX_findFiles(DEMO_DIR, archive_exts))
	{
		char demofile[PATH_MAX];
		snprintf(demofile, sizeof(demofile), DEMO_DIR "%s", i);
		std::array<char, PATH_MAX> realfile;
		if (PHYSFSX_getRealPath(demofile, realfile) && PHYSFS_mount(realfile.data(), DEMO_DIR, 0))
		{
			con_printf(CON_DEBUG, "PHYSFS: Added %s to " DEMO_DIR, realfile.data());
			content_updated = 1;
		}
	}
	return content_updated;
}

/*
 * Add archives to the game.
 * 1) archives from Sharepath/Data to extend/replace builtin game content
 * 2) archived demos
 */
void PHYSFSX_addArchiveContent()
{
	con_puts(CON_DEBUG, "PHYSFS: Adding archives to the game.");
	auto content_updated{add_archives_to_search_path()};
	content_updated = add_demo_files_to_search_path(content_updated);
	if (content_updated)
	{
		con_puts(CON_DEBUG, "Game content updated!");
		PHYSFSX_listSearchPathContent();
	}
}

// Removes content added above when quitting game
void PHYSFSX_removeArchiveContent()
{
	// find files in Searchpath ...
	// if found, remove them...
	for (const auto i : PHYSFSX_findFiles("", archive_exts))
	{
		std::array<char, PATH_MAX> realfile;
		if (PHYSFSX_getRealPath(i, realfile))
			PHYSFS_unmount(realfile.data());
	}
	// find files in DEMO_DIR ...
	// if found, remove them...
	for (const auto i : PHYSFSX_findFiles(DEMO_DIR, archive_exts))
	{
		char demofile[PATH_MAX];
		snprintf(demofile, sizeof(demofile), DEMO_DIR "%s", i);
		std::array<char, PATH_MAX> realfile;
		if (PHYSFSX_getRealPath(demofile, realfile))
			PHYSFS_unmount(realfile.data());
	}
}

void PHYSFSX_read_helper_report_error(const char *const filename, const unsigned line, const char *const func, const NamedPHYSFS_File file)
{
	(Error)(filename, line, func, "reading file %s at %lu", file.filename, static_cast<unsigned long>((PHYSFS_tell)(file.fp)));
}

RAIIPHYSFS_ComputedPathMount make_PHYSFSX_ComputedPathMount(char *const name, physfs_search_path position)
{
	auto pathname = std::make_unique<std::array<char, PATH_MAX>>();
	if (PHYSFSX_addRelToSearchPath(name, *pathname.get(), position) == PHYSFS_ERR_OK)
		return RAIIPHYSFS_ComputedPathMount(std::move(pathname));
	return nullptr;
}

}
