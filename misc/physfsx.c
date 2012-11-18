/*
 *
 * Some simple physfs extensions
 *
 */


#if !defined(macintosh) && !defined(_MSC_VER)
#include <sys/param.h>
#endif
#if defined(__MACH__) && defined(__APPLE__)
#include <sys/mount.h>
#include <unistd.h>	// for chdir hack
#include <HIServices/Processes.h>
#endif

#include "physfsx.h"
#include "args.h"
#include "object.h"
#include "newdemo.h"

// Initialise PhysicsFS, set up basic search paths and add arguments from .ini file.
// The .ini file can be in either the user directory or the same directory as the program.
// The user directory is searched first.
void PHYSFSX_init(int argc, char *argv[])
{
#if defined(__unix__) || defined(__APPLE__) || defined(__MACH__)
	char fullPath[PATH_MAX + 5];
#endif
#if defined(__unix__)
	char *path = NULL;
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
# if !(defined(__APPLE__) && defined(__MACH__))
	path = "~/.d2x-rebirth/";
# else
	path = "~/Library/Preferences/D2X Rebirth/";
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
	
	PHYSFS_addToSearchPath(PHYSFS_getWriteDir(), 1);
#endif
	
	PHYSFS_addToSearchPath(base_dir, 1);
	InitArgs( argc,argv );
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
	if (GameArg.SysHogDir)
		PHYSFS_addToSearchPath(GameArg.SysHogDir,1);
#if defined(__unix__)
	else if (!GameArg.SysNoHogDir)
		PHYSFS_addToSearchPath(SHAREPATH, 1);
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
		PHYSFS_addToSearchPath(base_dir, 1);
	}
#endif
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
	PHYSFS_file *fp;
	char hogname2[PATH_MAX];
	int size;

	snprintf(hogname2, sizeof(hogname2), "%s", hogname);
	PHYSFSEXT_locateCorrectCase(hogname2);

	fp = PHYSFS_openRead(hogname2);
	if (fp == NULL)
		return -1;

	size = PHYSFS_fileLength(fp);
	PHYSFS_close(fp);

	return size;
}

void PHYSFSX_listSearchPathContent()
{
	char **i, **list;

	con_printf(CON_DEBUG, "PHYSFS: Listing contents of Search Path.\n");
	list = PHYSFS_getSearchPath();
	for (i = list; *i != NULL; i++)
		con_printf(CON_DEBUG, "PHYSFS: [%s] is in the Search Path.\n", *i);
	PHYSFS_freeList(list);

	list = PHYSFS_enumerateFiles("");
	for (i = list; *i != NULL; i++)
		con_printf(CON_DEBUG, "PHYSFS: * We've got [%s].\n", *i);
	PHYSFS_freeList(list);
	
	con_printf(CON_DEBUG, "\n");
}

// checks which archives are supported by PhysFS. Return 0 if some essential (HOG) is not supported
int PHYSFSX_checkSupportedArchiveTypes()
{
	const PHYSFS_ArchiveInfo **i;
	int hog_sup = 0, mvl_sup = 0;

	con_printf(CON_DEBUG, "PHYSFS: Checking supported archive types.\n");
	for (i = PHYSFS_supportedArchiveTypes(); *i != NULL; i++)
	{
		con_printf(CON_DEBUG, "PHYSFS: Supported archive: [%s], which is [%s].\n", (*i)->extension, (*i)->description);
		if (!d_stricmp((*i)->extension, "HOG"))
			hog_sup = 1;
		if (!d_stricmp((*i)->extension, "MVL"))
			mvl_sup = 1;
	}

	if (!hog_sup)
		con_printf(CON_CRITICAL, "PHYSFS: HOG not supported. The game will not work without!\n");
	if (!mvl_sup)
		con_printf(CON_URGENT, "PHYSFS: MVL not supported. Won't be able to play movies!\n");

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
	char **i, **list;
	
	list = PHYSFS_getSearchPath();
	for (i = list; *i != NULL; i++)
	{
		if (!strcmp(path, *i))
		{
			is_new_path = 0;
		}
	}
	PHYSFS_freeList(list);
	
	return is_new_path;
}

int PHYSFSX_rename(const char *oldpath, const char *newpath)
{
	char old[PATH_MAX], n[PATH_MAX];
	
	PHYSFSX_getRealPath(oldpath, old);
	PHYSFSX_getRealPath(newpath, n);
	return (rename(old, n) == 0);
}

// Find files at path that have an extension listed in exts
// The extension list exts must be NULL-terminated, with each ext beginning with a '.'
char **PHYSFSX_findFiles(const char *path, const char *const *exts)
{
	char **list = PHYSFS_enumerateFiles(path);
	char **i, **j = list;
	const char *const *k;
	char *ext;
	
	if (list == NULL)
		return NULL;	// out of memory: not so good
	
	for (i = list; *i; i++)
	{
		ext = strrchr(*i, '.');
		if (ext)
			for (k = exts; *k != NULL && d_stricmp(ext, *k); k++) {}	// see if the file is of a type we want
		
		if (ext && *k)
			*j++ = *i;
		else
			free(*i);
	}
	
	*j = NULL;
	list = realloc(list, (j - list + 1)*sizeof(char *));	// save a bit of memory (or a lot?)
	return list;
}

// Same function as above but takes a real directory as second argument, only adding files originating from this directory.
// This can be used to further seperate files in search path but it must be made sure realpath is properly formatted.
char **PHYSFSX_findabsoluteFiles(const char *path, const char *realpath, const char *const *exts)
{
	char **list = PHYSFS_enumerateFiles(path);
	char **i, **j = list;
	const char *const *k;
	char *ext;
	
	if (list == NULL)
		return NULL;	// out of memory: not so good
	
	for (i = list; *i; i++)
	{
		ext = strrchr(*i, '.');
		if (ext)
			for (k = exts; *k != NULL && d_stricmp(ext, *k); k++) {}	// see if the file is of a type we want
		
		if (ext && *k && (!strcmp(PHYSFS_getRealDir(*i),realpath)))
			*j++ = *i;
		else
			free(*i);
	}
	
	*j = NULL;
	list = realloc(list, (j - list + 1)*sizeof(char *));	// save a bit of memory (or a lot?)
	return list;
}

#if 0
// returns -1 if error
// Gets bytes free in current write dir
PHYSFS_sint64 PHYSFSX_getFreeDiskSpace()
{
#if defined(__linux__) || (defined(__MACH__) && defined(__APPLE__))
	struct statfs sfs;
	
	if (!statfs(PHYSFS_getWriteDir(), &sfs))
		return (PHYSFS_sint64)(sfs.f_bavail * sfs.f_bsize);
	
	return -1;
#else
	return 0x7FFFFFFF;
#endif
}
#endif

int PHYSFSX_exists(const char *filename, int ignorecase)
{
	char filename2[PATH_MAX];

	if (!ignorecase)
		return PHYSFS_exists(filename);

	snprintf(filename2, sizeof(filename2), "%s", filename);
	PHYSFSEXT_locateCorrectCase(filename2);

	return PHYSFS_exists(filename2);
}

//Open a file for reading, set up a buffer
PHYSFS_file *PHYSFSX_openReadBuffered(const char *filename)
{
	PHYSFS_file *fp;
	PHYSFS_uint64 bufSize;
	char filename2[PATH_MAX];
	
	if (filename[0] == '\x01')
	{
		//FIXME: don't look in dir, only in hogfile
		filename++;
	}
	
	snprintf(filename2, sizeof(filename2), "%s", filename);
	PHYSFSEXT_locateCorrectCase(filename2);
	
	fp = PHYSFS_openRead(filename2);
	if (!fp)
		return NULL;
	
	bufSize = PHYSFS_fileLength(fp);
	while (!PHYSFS_setBuffer(fp, bufSize) && bufSize)
		bufSize /= 2;	// even if the error isn't memory full, for a 20MB file it'll only do this 8 times
	
	return fp;
}

//Open a file for writing, set up a buffer
PHYSFS_file *PHYSFSX_openWriteBuffered(const char *filename)
{
	PHYSFS_file *fp;
	PHYSFS_uint64 bufSize = 1024*1024;	// hmm, seems like an OK size.
	
	fp = PHYSFS_openWrite(filename);
	if (!fp)
		return NULL;
	
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
	char **list = NULL;
	static const char *const archive_exts[] = { ".dxa", NULL };
	char *file[2];
	int i = 0, content_updated = 0;

	con_printf(CON_DEBUG, "PHYSFS: Adding archives to the game.\n");
	// find files in Searchpath ...
	list = PHYSFSX_findFiles("", archive_exts);
	// if found, add them...
	for (i = 0; list[i] != NULL; i++)
	{
		MALLOC(file[0], char, PATH_MAX);
		MALLOC(file[1], char, PATH_MAX);
		snprintf(file[0], sizeof(char)*PATH_MAX, "%s", list[i]);
		PHYSFSX_getRealPath(file[0],file[1]);
		if (PHYSFS_addToSearchPath(file[1], 0))
		{
			con_printf(CON_DEBUG, "PHYSFS: Added %s to Search Path\n",file[1]);
			content_updated = 1;
		}
		d_free(file[0]);
		d_free(file[1]);
	}
	PHYSFS_freeList(list);
	list = NULL;

#if PHYSFS_VER_MAJOR >= 2
	// find files in DEMO_DIR ...
	list = PHYSFSX_findFiles(DEMO_DIR, archive_exts);
	// if found, add them...
	for (i = 0; list[i] != NULL; i++)
	{
		MALLOC(file[0], char, PATH_MAX);
		MALLOC(file[1], char, PATH_MAX);
		snprintf(file[0], sizeof(char)*PATH_MAX, "%s%s", DEMO_DIR, list[i]);
		PHYSFSX_getRealPath(file[0],file[1]);
		if (PHYSFS_mount(file[1], DEMO_DIR, 0))
		{
			con_printf(CON_DEBUG, "PHYSFS: Added %s to %s\n",file[1], DEMO_DIR);
			content_updated = 1;
		}
		d_free(file[0]);
		d_free(file[1]);
	}

	PHYSFS_freeList(list);
	list = NULL;
#endif

	if (content_updated)
	{
		con_printf(CON_DEBUG, "Game content updated!\n");
		PHYSFSX_listSearchPathContent();
	}
}

// Removes content added above when quitting game
void PHYSFSX_removeArchiveContent()
{
	char **list = NULL;
	static const char *const archive_exts[] = { ".dxa", NULL };
	char *file[2];
	int i = 0;

	// find files in Searchpath ...
	list = PHYSFSX_findFiles("", archive_exts);
	// if found, remove them...
	for (i = 0; list[i] != NULL; i++)
	{
		MALLOC(file[0], char, PATH_MAX);
		MALLOC(file[1], char, PATH_MAX);
		snprintf(file[0], sizeof(char)*PATH_MAX, "%s", list[i]);
		PHYSFSX_getRealPath(file[0],file[1]);
		PHYSFS_removeFromSearchPath(file[1]);
		d_free(file[0]);
		d_free(file[1]);
	}
	PHYSFS_freeList(list);
	list = NULL;

	// find files in DEMO_DIR ...
	list = PHYSFSX_findFiles(DEMO_DIR, archive_exts);
	// if found, remove them...
	for (i = 0; list[i] != NULL; i++)
	{
		MALLOC(file[0], char, PATH_MAX);
		MALLOC(file[1], char, PATH_MAX);
		snprintf(file[0], sizeof(char)*PATH_MAX, "%s%s", DEMO_DIR, list[i]);
		PHYSFSX_getRealPath(file[0],file[1]);
		PHYSFS_removeFromSearchPath(file[1]);
		d_free(file[0]);
		d_free(file[1]);
	}
	PHYSFS_freeList(list);
	list = NULL;
}
