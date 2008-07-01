/* $Id: physfsx.h,v 1.1.1.1 2006/03/17 20:01:28 zicodxx Exp $ */

/*
 *
 * Some simple physfs extensions
 *
 */

#ifndef PHYSFSX_H
#define PHYSFSX_H

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif
#if !defined(macintosh) && !defined(_MSC_VER)
#include <sys/param.h>
#endif
#if defined(__LINUX__)
#include <sys/vfs.h>
#elif defined(__MACH__) && defined(__APPLE__)
#include <sys/mount.h>
#include <unistd.h>	// for chdir hack
#endif
#include <string.h>
#include <stdarg.h>

#if !(defined(__APPLE__) && defined(__MACH__))
#include <physfs.h>
#else
#include <physfs/physfs.h>
#endif

#include "pstypes.h"
#include "strutil.h"
#include "u_mem.h"
#include "error.h"
#include "vecmat.h"
#include "args.h"
#include "ignorecase.h"

// Initialise PhysicsFS, set up basic search paths and add arguments from .ini file.
// The .ini file can be in either the user directory or the same directory as the program.
// The user directory is searched first.
static inline void PHYSFSX_init(int argc, char *argv[])
{
#if defined(__unix__)
	char *path = NULL;
	char fullPath[PATH_MAX + 5];
#endif
#ifdef macintosh	// Mac OS 9
	char base_dir[PATH_MAX];
#else
#define base_dir PHYSFS_getBaseDir()
#endif
	
	PHYSFS_init(argv[0]);
	PHYSFS_permitSymbolicLinks(1);

#ifdef macintosh
	strcpy(base_dir, PHYSFS_getBaseDir());
	if (strstr(base_dir, ".app:Contents:MacOSClassic:"))	// the Mac OS 9 program is still in the .app bundle
		strncat(base_dir, ":::", PATH_MAX - 1 - strlen(base_dir));	// go outside the .app bundle (the lazy way)
#endif

#if (defined(__APPLE__) && defined(__MACH__))	// others?
	chdir(base_dir);	// make sure relative hogdir and userdir paths work
#endif

#if defined(__unix__)
# if !(defined(__APPLE__) && defined(__MACH__))
	path = "~/.d1x-rebirth/";
# else
	path = "~/Library/Preferences/D1X Rebirth/";
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

	//tell cfile where hogdir is
	if (GameArg.SysHogDir)
		PHYSFS_addToSearchPath(GameArg.SysHogDir,1);
#if defined(__unix__)
	else if (!GameArg.SysNoHogDir)
		PHYSFS_addToSearchPath(SHAREPATH, 1);
#endif
}

static inline int PHYSFSX_readString(PHYSFS_file *file, char *s)
{
	char *ptr = s;

	if (PHYSFS_eof(file))
		*ptr = 0;
	else
		do
			PHYSFS_read(file, ptr, 1, 1);
		while (!PHYSFS_eof(file) && *ptr++ != 0);

	return strlen(s);
}

static inline int PHYSFSX_gets(PHYSFS_file *file, char *s)
{
	char *ptr = s;

	if (PHYSFS_eof(file))
		*ptr = 0;
	else
		do
			PHYSFS_read(file, ptr, 1, 1);
		while (!PHYSFS_eof(file) && *ptr++ != '\n');

	return strlen(s);
}

static inline int PHYSFSX_writeU8(PHYSFS_file *file, PHYSFS_uint8 val)
{
	return PHYSFS_write(file, &val, 1, 1);
}

static inline int PHYSFSX_writeString(PHYSFS_file *file, char *s)
{
	return PHYSFS_write(file, s, 1, strlen(s) + 1);
}

static inline int PHYSFSX_puts(PHYSFS_file *file, char *s)
{
	return PHYSFS_write(file, s, 1, strlen(s));
}

static inline int PHYSFSX_putc(PHYSFS_file *file, int c)
{
	unsigned char ch = (unsigned char)c;

	if (PHYSFS_write(file, &ch, 1, 1) < 1)
		return -1;
	else
		return (int)c;
}

static inline int PHYSFSX_printf(PHYSFS_file *file, char *format, ...)
{
	char buffer[1024];
	va_list args;

	va_start(args, format);
	vsprintf(buffer, format, args);

	return PHYSFSX_puts(file, buffer);
}

#define PHYSFSX_writeFix	PHYSFS_writeSLE32
#define PHYSFSX_writeFixAng	PHYSFS_writeSLE16

static inline int PHYSFSX_writeVector(PHYSFS_file *file, vms_vector *v)
{
	if (PHYSFSX_writeFix(file, v->x) < 1 ||
	 PHYSFSX_writeFix(file, v->y) < 1 ||
	 PHYSFSX_writeFix(file, v->z) < 1)
		return 0;

	return 1;
}

static inline int PHYSFSX_writeAngleVec(PHYSFS_file *file, vms_angvec *v)
{
	if (PHYSFSX_writeFixAng(file, v->p) < 1 ||
	 PHYSFSX_writeFixAng(file, v->b) < 1 ||
	 PHYSFSX_writeFixAng(file, v->h) < 1)
		return 0;

	return 1;
}

static inline int PHYSFSX_writeMatrix(PHYSFS_file *file, vms_matrix *m)
{
	if (PHYSFSX_writeVector(file, &m->rvec) < 1 ||
	 PHYSFSX_writeVector(file, &m->uvec) < 1 ||
	 PHYSFSX_writeVector(file, &m->fvec) < 1)
		return 0;

	return 1;
}

static inline int PHYSFSX_getRealPath(const char *stdPath, char *realPath)
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

static inline int PHYSFSX_rename(char *oldpath, char *newpath)
{
	char old[PATH_MAX], new[PATH_MAX];

	PHYSFSX_getRealPath(oldpath, old);
	PHYSFSX_getRealPath(newpath, new);
	return (rename(old, new) == 0);
}

// Find files at path that have an extension listed in exts
// The extension list exts must be NULL-terminated, with each ext beginning with a '.'
static inline char **PHYSFSX_findFiles(char *path, char **exts)
{
	char **list = PHYSFS_enumerateFiles(path);
	char **i, **j = list, **k;
	char *ext;

	if (list == NULL)
		return NULL;	// out of memory: not so good

	for (i = list; *i; i++)
	{
		ext = strrchr(*i, '.');
		if (ext)
			for (k = exts; *k != NULL && stricmp(ext, *k); k++) {}	// see if the file is of a type we want

		if (ext && *k)
			*j++ = *i;
		else
			free(*i);
	}

	*j = NULL;
	list = realloc(list, (j - list + 1)*sizeof(char *));	// save a bit of memory (or a lot?)
	return list;
}


// returns -1 if error
// Gets bytes free in current write dir
static inline PHYSFS_sint64 PHYSFSX_getFreeDiskSpace()
{
#if defined(__LINUX__) || (defined(__MACH__) && defined(__APPLE__))
	struct statfs sfs;

	if (!statfs(PHYSFS_getWriteDir(), &sfs))
		return (PHYSFS_sint64)(sfs.f_bavail * sfs.f_bsize);

	return -1;
#else
	return 0x7FFFFFFF;
#endif
}

//Open a file for reading, set up a buffer
static inline PHYSFS_file *PHYSFSX_openReadBuffered(char *filename)
{
	PHYSFS_file *fp;
	PHYSFS_uint64 bufSize;

	if (filename[0] == '\x01')
	{
		//FIXME: don't look in dir, only in hogfile
		filename++;
	}

	PHYSFSEXT_locateCorrectCase(filename);

	fp = PHYSFS_openRead(filename);
	if (!fp)
		return NULL;

	bufSize = PHYSFS_fileLength(fp);
	while (!PHYSFS_setBuffer(fp, bufSize) && bufSize)
		bufSize /= 2;	// even if the error isn't memory full, for a 20MB file it'll only do this 8 times

	return fp;
}

//Open a file for writing, set up a buffer
static inline PHYSFS_file *PHYSFSX_openWriteBuffered(char *filename)
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

//Open a 'data' file for reading (a file that would go in the 'Data' folder for the original Mac Descent II)
//Allows backwards compatibility with old Mac directories while retaining PhysicsFS flexibility
static inline PHYSFS_file *PHYSFSX_openDataFile(char *filename)
{
	PHYSFS_file *fp = PHYSFSX_openReadBuffered(filename);

	if (!fp)
	{
		char name[255];
		
		sprintf(name, "Data/%s", filename);
		fp = PHYSFSX_openReadBuffered(name);
	}

	return fp;
}

#endif /* PHYSFSX_H */
