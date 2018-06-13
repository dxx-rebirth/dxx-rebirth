/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 *
 * Some simple physfs extensions
 *
 */

#include <cstdlib>
#if !defined(macintosh) && !defined(_MSC_VER)
#include <sys/param.h>
#endif
#if defined(__APPLE__) && defined(__MACH__)
#include <sys/mount.h>
#include <unistd.h>	// for chdir hack
#include <HIServices/Processes.h>
#endif

#include "args.h"
#include "newdemo.h"
#include "console.h"
#include "strutil.h"
#include "ignorecase.h"
#include "physfs_list.h"

#include "compiler-range_for.h"
#include "compiler-poison.h"

namespace dcx {

const array<file_extension_t, 1> archive_exts{{"dxa"}};

char *PHYSFSX_fgets_t::get(char *const buf, std::size_t n, PHYSFS_File *const fp)
{
	PHYSFS_sint64 r = PHYSFS_read(fp, buf, sizeof(*buf), n - 1);
	if (r <= 0)
		return DXX_POISON_MEMORY(buf, buf + n, 0xcc), nullptr;
	char *p = buf;
	const auto cleanup = [&]{
		return *p = 0, DXX_POISON_MEMORY(p + 1, buf + n, 0xcc), p;
	};
	char *const e = &buf[r];
	for (;;)
	{
		if (p == e)
		{
			return cleanup();
		}
		char c = *p;
		if (c == 0)
			break;
		if (c == '\n')
		{
			break;
		}
		else if (c == '\r')
		{
			*p = 0;
			if (++p != e && *p != '\n')
				--p;
			break;
		}
		++p;
	}
	PHYSFS_seek(fp, PHYSFS_tell(fp) + p - e + 1);
	return cleanup();
}

int PHYSFSX_checkMatchingExtension(const char *filename, const partial_range_t<const file_extension_t *> range)
{
	const char *ext = strrchr(filename, '.');
	if (!ext)
		return 0;
	++ext;
	// see if the file is of a type we want
	range_for (auto &k, range)
	{
		if (!d_stricmp(ext, k))
			return 1;
	}
	return 0;
}

}

namespace dsx {

// Initialise PhysicsFS, set up basic search paths and add arguments from .ini file.
// The .ini file can be in either the user directory or the same directory as the program.
// The user directory is searched first.
bool PHYSFSX_init(int argc, char *argv[])
{
#if defined(__unix__) || defined(__APPLE__) || defined(__MACH__)
	char fullPath[PATH_MAX + 5];
#endif
#ifdef macintosh	// Mac OS 9
	char base_dir[PATH_MAX];
	int bundle = 0;
#else
#define base_dir PHYSFS_getBaseDir()
#endif
	
	PHYSFS_init(argv[0]);
	PHYSFS_permitSymbolicLinks(1);
	
#ifdef macintosh
	strcpy(base_dir, PHYSFS_getBaseDir());
	if (strstr(base_dir, ".app:Contents:MacOSClassic"))	// the Mac OS 9 program is still in the .app bundle
	{
		char *p;
		
		bundle = 1;
		if (base_dir[strlen(base_dir) - 1] == ':')
			base_dir[strlen(base_dir) - 1] = '\0';
		p = strrchr(base_dir, ':'); *p = '\0';	// path to 'Contents'
		p = strrchr(base_dir, ':'); *p = '\0';	// path to bundle
		p = strrchr(base_dir, ':'); *p = '\0';	// path to directory containing bundle
	}
#endif
	
#if (defined(__APPLE__) && defined(__MACH__))	// others?
	chdir(base_dir);	// make sure relative hogdir paths work
#endif
	
	const char *writedir;
#if defined(__unix__)
#if defined(DXX_BUILD_DESCENT_I)
#define DESCENT_PATH_NUMBER	"1"
#elif defined(DXX_BUILD_DESCENT_II)
#define DESCENT_PATH_NUMBER	"2"
#endif
	const auto &home_environ_var = "D" DESCENT_PATH_NUMBER "X_REBIRTH_HOME";
	const char *path = getenv(home_environ_var);
	if (!path)
	{
		path = getenv(&home_environ_var[4]);
		if (!path)
# if !(defined(__APPLE__) && defined(__MACH__))
	path = "~/.d" DESCENT_PATH_NUMBER "x-rebirth/";
# else
	path = "~/Library/Preferences/D" DESCENT_PATH_NUMBER "X Rebirth/";
# endif
	}
	
	if (path[0] == '~') // yes, this tilde can be put before non-unix paths.
	{
		const char *home = PHYSFS_getUserDir();
		
		strcpy(fullPath, home); // prepend home to the path
		path++;
		if (*path == *PHYSFS_getDirSeparator())
			path++;
		strncat(fullPath, path, PATH_MAX + 5 - strlen(home));
	}
	else
		strncpy(fullPath, path, PATH_MAX + 5);
	
	PHYSFS_setWriteDir(fullPath);
	if (!(writedir = PHYSFS_getWriteDir()))
	{                                               // need to make it
		char *p;
		char ancestor[PATH_MAX + 5];    // the directory which actually exists
		char child[PATH_MAX + 5];               // the directory relative to the above we're trying to make
		
		strcpy(ancestor, fullPath);
		const auto separator = *PHYSFS_getDirSeparator();
		while (!PHYSFS_getWriteDir() && (p = strrchr(ancestor, separator)))
		{
			if (p[1] == 0)
			{                                       // separator at the end (intended here, for safety)
				*p = 0;                 // kill this separator
				if (!(p = strrchr(ancestor, separator)))
					break;          // give up, this is (usually) the root directory
			}
			
			p[1] = 0;                       // go to parent
			PHYSFS_setWriteDir(ancestor);
		}
		
		strcpy(child, fullPath + strlen(ancestor));
		if (separator != '/')
			for (p = child; (p = strchr(p, separator)); p++)
				*p = '/';
		PHYSFS_mkdir(child);
		PHYSFS_setWriteDir(fullPath);
		writedir = PHYSFS_getWriteDir();
	}
	con_printf(CON_DEBUG, "PHYSFS: append write directory \"%s\" to search path", writedir);
	PHYSFS_addToSearchPath(writedir, 1);
#endif
	con_printf(CON_DEBUG, "PHYSFS: temporarily append base directory \"%s\" to search path", base_dir);
	PHYSFS_addToSearchPath(base_dir, 1);
	if (!InitArgs( argc,argv ))
		return false;
	PHYSFS_removeFromSearchPath(base_dir);
	
	if (!PHYSFS_getWriteDir())
	{
		PHYSFS_setWriteDir(base_dir);
		if (!(writedir = PHYSFS_getWriteDir()))
			Error("can't set write dir: %s\n", PHYSFS_getLastError());
		PHYSFS_addToSearchPath(writedir, 0);
	}
	
	//tell PHYSFS where hogdir is
	if (!CGameArg.SysHogDir.empty())
	{
		const auto p = CGameArg.SysHogDir.c_str();
		con_printf(CON_DEBUG, "PHYSFS: append argument hog directory \"%s\" to search path", p);
		PHYSFS_addToSearchPath(p, 1);
	}
#ifdef SHAREPATH
	else if (!CGameArg.SysNoHogDir)
	{
		con_puts(CON_DEBUG, "PHYSFS: append sharepath directory \"" SHAREPATH "\" to search path");
		PHYSFS_addToSearchPath(SHAREPATH, 1);
	}
#endif
	
	PHYSFSX_addRelToSearchPath("data", 1);	// 'Data' subdirectory
	
	// For Macintosh, add the 'Resources' directory in the .app bundle to the searchpaths
#if defined(__APPLE__) && defined(__MACH__)
	{
		ProcessSerialNumber psn = { 0, kCurrentProcess };
		FSRef fsref;
		OSStatus err;
		
		err = GetProcessBundleLocation(&psn, &fsref);
		if (err == noErr)
			err = FSRefMakePath(&fsref, reinterpret_cast<uint8_t *>(fullPath), PATH_MAX);
		
		if (err == noErr)
		{
			strncat(fullPath, "/Contents/Resources/", PATH_MAX + 4 - strlen(fullPath));
			fullPath[PATH_MAX + 4] = '\0';
			con_printf(CON_DEBUG, "PHYSFS: append resources directory \"%s\" to search path", fullPath);
			PHYSFS_addToSearchPath(fullPath, 1);
		}
	}
#elif defined(macintosh)
	if (bundle)
	{
		base_dir[strlen(base_dir)] = ':';	// go back in the bundle
		base_dir[strlen(base_dir)] = ':';	// go back in 'Contents'
		strncat(base_dir, ":Resources:", PATH_MAX - 1 - strlen(base_dir));
		base_dir[PATH_MAX - 1] = '\0';
		con_printf(CON_DEBUG, "PHYSFS: append bundle directory \"%s\" to search path", base_dir);
		PHYSFS_addToSearchPath(base_dir, 1);
	}
#endif
	return true;
}

}

namespace dcx {

// Add a searchpath, but that searchpath is relative to an existing searchpath
// It will add the first one it finds and return 1, if it doesn't find any it returns 0
int PHYSFSX_addRelToSearchPath(const char *relname, int add_to_end)
{
	char relname2[PATH_MAX];

	snprintf(relname2, sizeof(relname2), "%s", relname);
	PHYSFSEXT_locateCorrectCase(relname2);

	array<char, PATH_MAX> pathname;
	if (!PHYSFSX_getRealPath(relname2, pathname))
	{
		con_printf(CON_DEBUG, "PHYSFS: ignoring map request: no canonical path for relative name \"%s\"", relname2);
		return 0;
	}

	auto r = PHYSFS_addToSearchPath(pathname.data(), add_to_end);
	const auto action = add_to_end ? "append" : "insert";
	if (r)
		con_printf(CON_DEBUG, "PHYSFS: %s canonical directory \"%s\" to search path from relative name \"%s\"", action, pathname.data(), relname);
	else
		con_printf(CON_VERBOSE, "PHYSFS: failed to %s canonical directory \"%s\" to search path from relative name \"%s\": \"%s\"", action, pathname.data(), relname, PHYSFS_getLastError());
	return r;
}

int PHYSFSX_removeRelFromSearchPath(const char *relname)
{
	char relname2[PATH_MAX];

	snprintf(relname2, sizeof(relname2), "%s", relname);
	PHYSFSEXT_locateCorrectCase(relname2);

	array<char, PATH_MAX> pathname;
	if (!PHYSFSX_getRealPath(relname2, pathname))
	{
		con_printf(CON_DEBUG, "PHYSFS: ignoring unmap request: no canonical path for relative name \"%s\"", relname2);
		return 0;
	}
	auto r = PHYSFS_removeFromSearchPath(pathname.data());
	if (r)
		con_printf(CON_DEBUG, "PHYSFS: unmap canonical directory \"%s\" (relative name \"%s\")", pathname.data(), relname);
	else
		con_printf(CON_VERBOSE, "PHYSFS: failed to unmap canonical directory \"%s\" (relative name \"%s\"): \"%s\"", pathname.data(), relname, PHYSFS_getLastError());
	return r;
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
	con_puts(CON_DEBUG, "PHYSFS: Listing contents of Search Path.");
	PHYSFSX_uncounted_list list{PHYSFS_getSearchPath()};
	range_for (const auto i, list)
		con_printf(CON_DEBUG, "PHYSFS: [%s] is in the Search Path.", i);
	list.reset();
	list.reset(PHYSFS_enumerateFiles(""));
	range_for (const auto i, list)
		con_printf(CON_DEBUG, "PHYSFS: * We've got [%s].", i);
}

}

namespace dsx {

// checks which archives are supported by PhysFS. Return 0 if some essential (HOG) is not supported
int PHYSFSX_checkSupportedArchiveTypes()
{
	int hog_sup = 0;
#ifdef DXX_BUILD_DESCENT_II
	int mvl_sup = 0;
#endif

	con_puts(CON_DEBUG, "PHYSFS: Checking supported archive types.");
	range_for (const auto i, make_null_sentinel_array(PHYSFS_supportedArchiveTypes()))
	{
		const auto iextension = i->extension;
		con_printf(CON_DEBUG, "PHYSFS: Supported archive: [%s], which is [%s].", iextension, i->description);
		if (!d_stricmp(iextension, "HOG"))
			hog_sup = 1;
#ifdef DXX_BUILD_DESCENT_II
		else if (!d_stricmp(iextension, "MVL"))
			mvl_sup = 1;
#endif
	}

	if (!hog_sup)
		con_puts(CON_CRITICAL, "PHYSFS: HOG not supported. The game will not work without!");
#ifdef DXX_BUILD_DESCENT_II
	if (!mvl_sup)
		con_puts(CON_URGENT, "PHYSFS: MVL not supported. Won't be able to play movies!");
#endif

	return hog_sup;
}

}

namespace dcx {

int PHYSFSX_getRealPath(const char *stdPath, char *realPath, const std::size_t outSize)
{
	DXX_POISON_MEMORY(realPath, outSize, 0xdd);
	const char *realDir = PHYSFS_getRealDir(stdPath);
	if (!realDir)
	{
		realDir = PHYSFS_getWriteDir();
		if (!realDir)
			return 0;
	}
	const auto realDirSize = strlen(realDir);
	if (realDirSize >= outSize)
		return 0;
	auto mountpoint = PHYSFS_getMountPoint(realDir);
	if (!mountpoint)
		return 0;
	std::copy_n(realDir, realDirSize + 1, realPath);
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
		const auto p = realPath + realDirSize - sepSize;
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
	if (realPathUsed + stdPathLen >= outSize)
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

// checks if path is already added to Searchpath. Returns 0 if yes, 1 if not.
int PHYSFSX_isNewPath(const char *path)
{
	int is_new_path = 1;
	PHYSFSX_uncounted_list list{PHYSFS_getSearchPath()};
	range_for (const auto i, list)
	{
		if (!strcmp(path, i))
		{
			is_new_path = 0;
			break;
		}
	}
	return is_new_path;
}

int PHYSFSX_rename(const char *oldpath, const char *newpath)
{
	array<char, PATH_MAX> old, n;
	PHYSFSX_getRealPath(oldpath, old);
	PHYSFSX_getRealPath(newpath, n);
	return (rename(old.data(), n.data()) == 0);
}

template <typename F>
static inline PHYSFSX_uncounted_list PHYSFSX_findPredicateFiles(const char *path, F f)
{
	PHYSFSX_uncounted_list list{PHYSFS_enumerateFiles(path)};
	if (!list)
		return nullptr;	// out of memory: not so good
	char **j = list.get();
	range_for (const auto i, list)
	{
		if (f(i))
			*j++ = i;
		else
			free(i);
	}
	*j = NULL;
	char **r = reinterpret_cast<char **>(realloc(list.get(), (j - list.get() + 1)*sizeof(char *)));	// save a bit of memory (or a lot?)
	if (r)
	{
		list.release();
		list.reset(r);
	}
	return list;
}

// Find files at path that have an extension listed in exts
// The extension list exts must be NULL-terminated, with each ext beginning with a '.'
PHYSFSX_uncounted_list PHYSFSX_findFiles(const char *path, const partial_range_t<const file_extension_t *> exts)
{
	const auto predicate = [&](const char *i) {
		return PHYSFSX_checkMatchingExtension(i, exts);
	};
	return PHYSFSX_findPredicateFiles(path, predicate);
}

// Same function as above but takes a real directory as second argument, only adding files originating from this directory.
// This can be used to further seperate files in search path but it must be made sure realpath is properly formatted.
PHYSFSX_uncounted_list PHYSFSX_findabsoluteFiles(const char *path, const char *realpath, const partial_range_t<const file_extension_t *> exts)
{
	const auto predicate = [&](const char *i) {
		return PHYSFSX_checkMatchingExtension(i, exts) && (!strcmp(PHYSFS_getRealDir(i), realpath));
	};
	return PHYSFSX_findPredicateFiles(path, predicate);
}

int PHYSFSX_exists_ignorecase(const char *filename)
{
	char filename2[PATH_MAX];
	snprintf(filename2, sizeof(filename2), "%s", filename);
	return !PHYSFSEXT_locateCorrectCase(filename2);
}

//Open a file for reading, set up a buffer
RAIIPHYSFS_File PHYSFSX_openReadBuffered(const char *filename)
{
	PHYSFS_uint64 bufSize;
	char filename2[PATH_MAX];
	
	if (filename[0] == '\x01')
	{
		//FIXME: don't look in dir, only in hogfile
		filename++;
	}
	
	snprintf(filename2, sizeof(filename2), "%s", filename);
	PHYSFSEXT_locateCorrectCase(filename2);
	
	RAIIPHYSFS_File fp{PHYSFS_openRead(filename2)};
	if (!fp)
		return nullptr;
	
	bufSize = PHYSFS_fileLength(fp);
	while (!PHYSFS_setBuffer(fp, bufSize) && bufSize)
		bufSize /= 2;	// even if the error isn't memory full, for a 20MB file it'll only do this 8 times
	return fp;
}

//Open a file for writing, set up a buffer
RAIIPHYSFS_File PHYSFSX_openWriteBuffered(const char *filename)
{
	PHYSFS_uint64 bufSize = 1024*1024;	// hmm, seems like an OK size.
	
	RAIIPHYSFS_File fp{PHYSFS_openWrite(filename)};
	if (!fp)
		return nullptr;
	while (!PHYSFS_setBuffer(fp, bufSize) && bufSize)
		bufSize /= 2;
	return fp;
}

/* 
 * Add archives to the game.
 * 1) archives from Sharepath/Data to extend/replace builtin game content
 * 2) archived demos
 */
void PHYSFSX_addArchiveContent()
{
	int content_updated = 0;

	con_puts(CON_DEBUG, "PHYSFS: Adding archives to the game.");
	// find files in Searchpath ...
	auto list = PHYSFSX_findFiles("", archive_exts);
	// if found, add them...
	range_for (const auto i, list)
	{
		array<char, PATH_MAX> realfile;
		PHYSFSX_getRealPath(i,realfile);
		if (PHYSFS_addToSearchPath(realfile.data(), 0))
		{
			con_printf(CON_DEBUG, "PHYSFS: Added %s to Search Path",realfile.data());
			content_updated = 1;
		}
	}
#if PHYSFS_VER_MAJOR >= 2
	list.reset();
	// find files in DEMO_DIR ...
	list = PHYSFSX_findFiles(DEMO_DIR, archive_exts);
	// if found, add them...
	range_for (const auto i, list)
	{
		char demofile[PATH_MAX];
		snprintf(demofile, sizeof(demofile), DEMO_DIR "%s", i);
		array<char, PATH_MAX> realfile;
		PHYSFSX_getRealPath(demofile,realfile);
		if (PHYSFS_mount(realfile.data(), DEMO_DIR, 0))
		{
			con_printf(CON_DEBUG, "PHYSFS: Added %s to " DEMO_DIR, realfile.data());
			content_updated = 1;
		}
	}
#endif
	list.reset();

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
	auto list = PHYSFSX_findFiles("", archive_exts);
	// if found, remove them...
	range_for (const auto i, list)
	{
		array<char, PATH_MAX> realfile;
		PHYSFSX_getRealPath(i, realfile);
		PHYSFS_removeFromSearchPath(realfile.data());
	}
	list.reset();
	// find files in DEMO_DIR ...
	list = PHYSFSX_findFiles(DEMO_DIR, archive_exts);
	// if found, remove them...
	range_for (const auto i, list)
	{
		char demofile[PATH_MAX];
		snprintf(demofile, sizeof(demofile), DEMO_DIR "%s", i);
		array<char, PATH_MAX> realfile;
		PHYSFSX_getRealPath(demofile,realfile);
		PHYSFS_removeFromSearchPath(realfile.data());
	}
}

void PHYSFSX_read_helper_report_error(const char *const filename, const unsigned line, const char *const func, PHYSFS_File *const file)
{
	(Error)(filename, line, func, "reading at %lu", static_cast<unsigned long>((PHYSFS_tell)(file)));
}

}
