/* $Id: physfsx.h,v 1.5 2004-12-03 07:29:32 btb Exp $ */

/*
 *
 * Some simple physfs extensions
 *
 */

#ifndef PHYSFSX_H
#define PHYSFSX_H

#if !defined(macintosh) && !defined(_MSC_VER)
#include <sys/param.h>
#endif
#if defined(__linux__)
#include <sys/vfs.h>
#elif defined(__MACH__) && defined(__APPLE__)
#include <sys/mount.h>
#endif
#include <string.h>

#include <physfs.h>

#include "error.h"

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

static inline int PHYSFSX_getRealPath(char *stdPath, char *realPath)
{
	const char *realDir = PHYSFS_getRealDir(stdPath);
	char sep = *PHYSFS_getDirSeparator();

	if (!realDir)
		return 0;
	
	Assert(strlen(realDir) + 1 + strlen(stdPath) < PATH_MAX);

	sprintf(realPath, "%s%c%s", realDir, sep, stdPath);

	return 1;
}

static inline int PHYSFSX_rename(char *oldpath, char *newpath)
{
	char old[PATH_MAX], new[PATH_MAX];

	PHYSFSX_getRealPath(oldpath, old);
	PHYSFSX_getRealPath(newpath, new);
	return (rename(old, new) == 0);
}


// returns -1 if error
// Gets bytes free in current write dir
static inline PHYSFS_sint64 PHYSFSX_getFreeDiskSpace()
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

#endif /* PHYSFSX_H */
