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
#include "poison.h"

const array<file_extension_t, 1> archive_exts{{"dxa"}};

char *PHYSFSX_fgets_t::get(char *const buf, std::size_t n, PHYSFS_file *const fp)
{
	PHYSFS_sint64 t = PHYSFS_tell(fp);
	PHYSFS_sint64 r = PHYSFS_read(fp, buf, sizeof(*buf), n - 1);
	if (r <= 0)
		return DXX_POISON_MEMORY(buf, buf + n, 0xcc), nullptr;
	char *p = buf;
	const auto cleanup = [&]{
		return *p = 0, DXX_POISON_MEMORY(p + 1, buf + n, 0xcc), p;
	};
	for (char *e = buf + r;;)
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
	PHYSFS_seek(fp, t + (p - buf) + 1);
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

// Initialise PhysicsFS, set up basic search paths and add arguments from .ini file.
// The .ini file can be in either the user directory or the same directory as the program.
// The user directory is searched first.
bool PHYSFSX_init(int argc, char *argv[])
{
#if defined(__unix__) || defined(__APPLE__) || defined(__MACH__)
	char fullPath[PATH_MAX + 5];
#endif
#if defined(__unix__)
	const char *path = NULL;
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
	
#if defined(__unix__)
#if defined(DXX_BUILD_DESCENT_I)
#define DESCENT_PATH_NUMBER	"1"
#elif defined(DXX_BUILD_DESCENT_II)
#define DESCENT_PATH_NUMBER	"2"
#endif
# if !(defined(__APPLE__) && defined(__MACH__))
	path = "~/.d" DESCENT_PATH_NUMBER "x-rebirth/";
# else
	path = "~/Library/Preferences/D" DESCENT_PATH_NUMBER "X Rebirth/";
# endif
	
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
	if (!PHYSFS_getWriteDir())
	{                                               // need to make it
		char *p;
		char ancestor[PATH_MAX + 5];    // the directory which actually exists
		char child[PATH_MAX + 5];               // the directory relative to the above we're trying to make
		
		strcpy(ancestor, fullPath);
		while (!PHYSFS_getWriteDir() && ((p = strrchr(ancestor, *PHYSFS_getDirSeparator()))))
		{
			if (p[1] == 0)
			{                                       // separator at the end (intended here, for safety)
				*p = 0;                 // kill this separator
				if (!((p = strrchr(ancestor, *PHYSFS_getDirSeparator()))))
					break;          // give up, this is (usually) the root directory
			}
			
			p[1] = 0;                       // go to parent
			PHYSFS_setWriteDir(ancestor);
		}
		
		strcpy(child, fullPath + strlen(ancestor));
		for (p = child; (p = strchr(p, *PHYSFS_getDirSeparator())); p++)
			*p = '/';
		PHYSFS_mkdir(child);
		PHYSFS_setWriteDir(fullPath);
	}
	const char *writedir = PHYSFS_getWriteDir();
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
		if (!PHYSFS_getWriteDir())
			Error("can't set write dir: %s\n", PHYSFS_getLastError());
		else
			PHYSFS_addToSearchPath(PHYSFS_getWriteDir(), 0);
	}
	
	//tell PHYSFS where hogdir is
	if (!GameArg.SysHogDir.empty())
	{
		con_printf(CON_DEBUG, "PHYSFS: append argument hog directory \"%s\" to search path", GameArg.SysHogDir.c_str());
		PHYSFS_addToSearchPath(GameArg.SysHogDir.c_str(),1);
	}
	if (GameArg.SysFullMissionDir != "")
	{
		con_printf(CON_DEBUG, "PHYSFS: append argument missions directory \"%s\" to search path", GameArg.SysFullMissionDir.c_str());
		PHYSFS_addToSearchPath(GameArg.SysFullMissionDir.c_str(),1);
	}
#if defined(__unix__)
	else if (!GameArg.SysNoHogDir)
	{
		con_printf(CON_DEBUG, "PHYSFS: append sharepath directory \"" SHAREPATH "\" to search path");
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
			err = FSRefMakePath(&fsref, (ubyte *)fullPath, PATH_MAX);
		
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

// Add a searchpath, but that searchpath is relative to an existing searchpath
// It will add the first one it finds and return 1, if it doesn't find any it returns 0
int PHYSFSX_addRelToSearchPath(const char *relname, int add_to_end)
{
	char relname2[PATH_MAX], pathname[PATH_MAX];

	snprintf(relname2, sizeof(relname2), "%s", relname);
	PHYSFSEXT_locateCorrectCase(relname2);

	if (!PHYSFSX_getRealPath(relname2, pathname))
		return 0;

	con_printf(CON_DEBUG, "PHYSFS: %s canonical directory \"%s\" to search path from relative name \"%s\"", add_to_end ? "append" : "set", pathname, relname);
	return PHYSFS_addToSearchPath(pathname, add_to_end);
}

int PHYSFSX_removeRelFromSearchPath(const char *relname)
{
	char relname2[PATH_MAX], pathname[PATH_MAX];

	snprintf(relname2, sizeof(relname2), "%s", relname);
	PHYSFSEXT_locateCorrectCase(relname2);

	if (!PHYSFSX_getRealPath(relname2, pathname))
		return 0;

	return PHYSFS_removeFromSearchPath(pathname);
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
	con_printf(CON_DEBUG, "PHYSFS: Listing contents of Search Path.");
	PHYSFSX_uncounted_list list{PHYSFS_getSearchPath()};
	range_for (const auto i, list)
		con_printf(CON_DEBUG, "PHYSFS: [%s] is in the Search Path.", i);
	list.reset();
	list.reset(PHYSFS_enumerateFiles(""));
	range_for (const auto i, list)
		con_printf(CON_DEBUG, "PHYSFS: * We've got [%s].", i);
}

// checks which archives are supported by PhysFS. Return 0 if some essential (HOG) is not supported
int PHYSFSX_checkSupportedArchiveTypes()
{
	int hog_sup = 0;
#ifdef DXX_BUILD_DESCENT_II
	int mvl_sup = 0;
#endif

	con_printf(CON_DEBUG, "PHYSFS: Checking supported archive types.");
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
		con_printf(CON_CRITICAL, "PHYSFS: HOG not supported. The game will not work without!");
#ifdef DXX_BUILD_DESCENT_II
	if (!mvl_sup)
		con_printf(CON_URGENT, "PHYSFS: MVL not supported. Won't be able to play movies!");
#endif

	return hog_sup;
}

int PHYSFSX_getRealPath(const char *stdPath, char *realPath)
{
	const char *realDir = PHYSFS_getRealDir(stdPath);
	const char *sep = PHYSFS_getDirSeparator();
	char *p;
	
	if (!realDir)
	{
		realDir = PHYSFS_getWriteDir();
		if (!realDir)
			return 0;
	}
	
	strncpy(realPath, realDir, PATH_MAX - 1);
	if (strlen(realPath) >= strlen(sep))
	{
		p = realPath + strlen(realPath) - strlen(sep);
		if (strcmp(p, sep)) // no sep at end of realPath
			strncat(realPath, sep, PATH_MAX - 1 - strlen(realPath));
	}
	
	if (strlen(stdPath) >= 1)
		if (*stdPath == '/')
			stdPath++;
	
	while (*stdPath)
	{
		if (*stdPath == '/')
			strncat(realPath, sep, PATH_MAX - 1 - strlen(realPath));
		else
		{
			if (strlen(realPath) < PATH_MAX - 2)
			{
				p = realPath + strlen(realPath);
				p[0] = *stdPath;
				p[1] = '\0';
			}
		}
		stdPath++;
	}
	
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
	char old[PATH_MAX], n[PATH_MAX];
	
	PHYSFSX_getRealPath(oldpath, old);
	PHYSFSX_getRealPath(newpath, n);
	return (rename(old, n) == 0);
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
PHYSFSX_uncounted_list PHYSFSX_findFiles(const char *path, const file_extension_t *exts, uint_fast32_t count)
{
	const auto predicate = [&](const char *i) {
		return PHYSFSX_checkMatchingExtension(i, exts, count);
	};
	return PHYSFSX_findPredicateFiles(path, predicate);
}

// Same function as above but takes a real directory as second argument, only adding files originating from this directory.
// This can be used to further seperate files in search path but it must be made sure realpath is properly formatted.
PHYSFSX_uncounted_list PHYSFSX_findabsoluteFiles(const char *path, const char *realpath, const file_extension_t *exts, uint_fast32_t count)
{
	const auto predicate = [&](const char *i) {
		return PHYSFSX_checkMatchingExtension(i, exts, count) && (!strcmp(PHYSFS_getRealDir(i), realpath));
	};
	return PHYSFSX_findPredicateFiles(path, predicate);
}

#if 0
// returns -1 if error
// Gets bytes free in current write dir
PHYSFS_sint64 PHYSFSX_getFreeDiskSpace()
{
#if defined(__linux__) || (defined(__APPLE__) && defined(__MACH__))
	struct statfs sfs;
	
	if (!statfs(PHYSFS_getWriteDir(), &sfs))
		return (PHYSFS_sint64)(sfs.f_bavail * sfs.f_bsize);
	
	return -1;
#else
	return 0x7FFFFFFF;
#endif
}
#endif

int PHYSFSX_exists_ignorecase(const char *filename)
{
	char filename2[PATH_MAX];
	snprintf(filename2, sizeof(filename2), "%s", filename);
	PHYSFSEXT_locateCorrectCase(filename2);
	return PHYSFS_exists(filename2);
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

	con_printf(CON_DEBUG, "PHYSFS: Adding archives to the game.");
	// find files in Searchpath ...
	auto list = PHYSFSX_findFiles("", archive_exts);
	// if found, add them...
	range_for (const auto i, list)
	{
		char realfile[PATH_MAX];
		PHYSFSX_getRealPath(i,realfile);
		if (PHYSFS_addToSearchPath(realfile, 0))
		{
			con_printf(CON_DEBUG, "PHYSFS: Added %s to Search Path",realfile);
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
		char demofile[PATH_MAX], realfile[PATH_MAX];
		snprintf(demofile, sizeof(demofile), DEMO_DIR "%s", i);
		PHYSFSX_getRealPath(demofile,realfile);
		if (PHYSFS_mount(realfile, DEMO_DIR, 0))
		{
			con_printf(CON_DEBUG, "PHYSFS: Added %s to " DEMO_DIR, realfile);
			content_updated = 1;
		}
	}
#endif
	list.reset();

	if (content_updated)
	{
		con_printf(CON_DEBUG, "Game content updated!");
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
		char realfile[PATH_MAX];
		PHYSFSX_getRealPath(i, realfile);
		PHYSFS_removeFromSearchPath(realfile);
	}
	list.reset();
	// find files in DEMO_DIR ...
	list = PHYSFSX_findFiles(DEMO_DIR, archive_exts);
	// if found, remove them...
	range_for (const auto i, list)
	{
		char demofile[PATH_MAX], realfile[PATH_MAX];
		snprintf(demofile, sizeof(demofile), DEMO_DIR "%s", i);
		PHYSFSX_getRealPath(demofile,realfile);
		PHYSFS_removeFromSearchPath(realfile);
	}
}
