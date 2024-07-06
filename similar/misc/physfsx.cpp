/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
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
#if !defined(_MSC_VER)
#include <sys/param.h>
#endif
#if defined(__APPLE__) && defined(__MACH__)
#include <sys/mount.h>
#include <unistd.h>	// for chdir hack
#include <ApplicationServices/ApplicationServices.h>
#endif

#include "args.h"
#include "newdemo.h"
#include "console.h"
#include "strutil.h"
#include "ignorecase.h"
#include "physfs_list.h"

#include "compiler-range_for.h"
#include "compiler-poison.h"
#include "d_enumerate.h"
#include "partial_range.h"

#ifndef DXX_ENABLE_ENVIRONMENT_VARIABLE_DXX_REBIRTH_HOME
#if defined(__unix__)
#define DXX_ENABLE_ENVIRONMENT_VARIABLE_DXX_REBIRTH_HOME	1
#else
#define DXX_ENABLE_ENVIRONMENT_VARIABLE_DXX_REBIRTH_HOME	0
#endif
#endif

namespace dcx {

namespace {

constexpr std::array<file_extension_t, 1> archive_exts{{"dxa"}};

static void add_data_directory_to_search_path()
{
	std::array<char, PATH_MAX> pathname;
	static char relname[]{"data"};
	PHYSFSX_addRelToSearchPath(relname, pathname, physfs_search_path::append);	// 'Data' subdirectory
}

}

}

namespace dsx {

#if DXX_ENABLE_ENVIRONMENT_VARIABLE_DXX_REBIRTH_HOME
static void check_add_environment_rebirth_home(const char *const home_environ_var, const char *const fallback_home_path)
{
	char fullPath[PATH_MAX + 5];
	const char *path = getenv(home_environ_var);
	if (!path)
	{
		path = fallback_home_path;
	}
	if (path[0] == '~') // yes, this tilde can be put before non-unix paths.
	{
		const char *home = PHYSFS_getUserDir();
		path++;
		// prepend home to the path
		if (*path == *PHYSFS_getDirSeparator())
			path++;
		snprintf(fullPath, sizeof(fullPath), "%s%s", home, path);
	}
	else
	{
		fullPath[sizeof(fullPath) - 1] = 0;
		strncpy(fullPath, path, sizeof(fullPath) - 1);
	}
	PHYSFS_setWriteDir(fullPath);
	auto writedir{PHYSFS_getWriteDir()};
	if (!writedir)
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
	PHYSFS_mount(writedir, nullptr, 1);
}
#endif

static void setup_hogdir_path()
{
	//tell PHYSFS where hogdir is
	if (!CGameArg.SysHogDir.empty())
	{
		const auto p{CGameArg.SysHogDir.c_str()};
		con_printf(CON_DEBUG, "PHYSFS: append argument hog directory \"%s\" to search path", p);
		PHYSFS_mount(p, nullptr, 1);
	}
#if DXX_USE_SHAREPATH
	else if (!CGameArg.SysNoHogDir)
	{
		con_puts(CON_DEBUG, "PHYSFS: append built-in sharepath directory \"" DXX_SHAREPATH "\" to search path");
		PHYSFS_mount(DXX_SHAREPATH, nullptr, 1);
	}
	else
	{
		con_puts(CON_DEBUG, "PHYSFS: skipping built-in sharepath \"" DXX_SHAREPATH "\"");
	}
#else
	else
	{
		con_puts(CON_DEBUG, "PHYSFS: no built-in sharepath");
	}
#endif
}

static void setup_final_fallback_write_directory(const char *const base_dir)
{
	if (!PHYSFS_getWriteDir())
	{
		PHYSFS_setWriteDir(base_dir);
		auto writedir{PHYSFS_getWriteDir()};
		if (!writedir)
			Error("can't set write dir: %s\n", PHYSFS_getLastError());
		PHYSFS_mount(writedir, nullptr, 0);
	}
}

#if defined(__APPLE__) && defined(__MACH__)
static void setup_osx_resource_path()
{
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	if (mainBundle)
	{
		CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
		if (resourcesURL)
		{
			char fullPath[PATH_MAX + 5];
			if (CFURLGetFileSystemRepresentation(resourcesURL, TRUE, reinterpret_cast<uint8_t *>(fullPath), sizeof(fullPath)))
			{
				con_printf(CON_DEBUG, "PHYSFS: append resources directory \"%s\" to search path", fullPath);
				PHYSFS_mount(fullPath, nullptr, 1);
			}
			CFRelease(resourcesURL);
		}
	}
}
#endif

// Initialise PhysicsFS, set up basic search paths and add arguments from .ini file.
// The .ini file can be in either the user directory or the same directory as the program.
// The user directory is searched first.
bool PHYSFSX_init(int argc, char *argv[])
{
	if (!PHYSFS_init(argv[0]))
		Error("Failed to init PhysFS: %s", PHYSFS_getLastError());
	PHYSFS_permitSymbolicLinks(1);
	const auto base_dir{PHYSFS_getBaseDir()};
#if (defined(__APPLE__) && defined(__MACH__))	// others?
	chdir(base_dir);	// make sure relative hogdir paths work
#endif

#if DXX_ENABLE_ENVIRONMENT_VARIABLE_DXX_REBIRTH_HOME
#if defined(DXX_BUILD_DESCENT_I)
#define DESCENT_PATH_NUMBER	"1"
#elif defined(DXX_BUILD_DESCENT_II)
#define DESCENT_PATH_NUMBER	"2"
#endif
#if !(defined(__APPLE__) && defined(__MACH__))
#define DXX_FALLBACK_HOME_PATH	"~/.d" DESCENT_PATH_NUMBER "x-rebirth/"
#else
#define DXX_FALLBACK_HOME_PATH	"~/Library/Preferences/D" DESCENT_PATH_NUMBER "X Rebirth/"
#endif
#define DXX_HOME_ENVIRONMENT_VARIABLE	"D" DESCENT_PATH_NUMBER "X_REBIRTH_HOME"
	check_add_environment_rebirth_home(DXX_HOME_ENVIRONMENT_VARIABLE, DXX_FALLBACK_HOME_PATH);
#undef DXX_FALLBACK_HOME_PATH
#undef DXX_HOME_ENVIRONMENT_VARIABLE
#endif
	con_printf(CON_DEBUG, "PHYSFS: temporarily append base directory \"%s\" to search path", base_dir);
	PHYSFS_mount(base_dir, nullptr, 1);
	if (!InitArgs(std::span(argv, argc).template subspan<1>()))
		return false;
	PHYSFS_unmount(base_dir);

	setup_final_fallback_write_directory(base_dir);
	setup_hogdir_path();
	
	add_data_directory_to_search_path();
	
	// For Macintosh, add the 'Resources' directory in the .app bundle to the searchpaths
#if defined(__APPLE__) && defined(__MACH__)
	setup_osx_resource_path();
#endif
	return true;
}

#if defined(DXX_BUILD_DESCENT_II)
RAIIPHYSFS_ComputedPathMount make_PHYSFSX_ComputedPathMount(char *const name1, char *const name2, physfs_search_path position)
{
	auto pathname{std::make_unique<std::array<char, PATH_MAX>>()};
	if (PHYSFSX_addRelToSearchPath(name1, *pathname.get(), position) == PHYSFS_ERR_OK ||
		PHYSFSX_addRelToSearchPath(name2, *pathname.get(), position) == PHYSFS_ERR_OK)
		return RAIIPHYSFS_ComputedPathMount{std::move(pathname)};
	return nullptr;
}
#endif

// checks which archives are supported by PhysFS. Return 0 if some essential (HOG) is not supported
int PHYSFSX_checkSupportedArchiveTypes()
{
	int hog_sup = 0;
#ifdef DXX_BUILD_DESCENT_II
	int mvl_sup = 0;
#endif

	con_puts(CON_DEBUG, "PHYSFS: Checking supported archive types.");
	range_for (const auto i, null_sentinel_array(PHYSFS_supportedArchiveTypes()))
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

//Open a file for reading, set up a buffer
std::pair<RAIINamedPHYSFS_File, PHYSFS_ErrorCode> PHYSFSX_openReadBuffered(const char *filename)
{
	PHYSFS_uint64 bufSize;
	char filename2[PATH_MAX];
#if 0
	if (filename[0] == '\x01')
	{
		//FIXME: don't look in dir, only in hogfile
		filename++;
	}
#endif
	snprintf(filename2, sizeof(filename2), "%s", filename);
	PHYSFSEXT_locateCorrectCase(filename2);

	/* Use the original filename in any error messages.  This is close enough
	 * that the user should be able to identify the bad file, and avoids the
	 * need to allocate and return storage for the filename.  In almost all
	 * cases, the filename will never be seen, since it is only shown if an
	 * error occurs when reading the file.
	 */
	RAIINamedPHYSFS_File fp{PHYSFS_openRead(filename2), filename};
	if (!fp)
		return {std::move(fp), PHYSFS_getLastErrorCode()};
	
	bufSize = PHYSFS_fileLength(fp);
	while (!PHYSFS_setBuffer(fp, bufSize) && bufSize)
		bufSize /= 2;	// even if the error isn't memory full, for a 20MB file it'll only do this 8 times
	return {std::move(fp), PHYSFS_ERR_OK};
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
