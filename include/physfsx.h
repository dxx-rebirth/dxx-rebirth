/* $Id: physfsx.h,v 1.10 2005-06-13 09:36:45 chris Exp $ */

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

#include "pstypes.h"
#include "error.h"
#include "vecmat.h"

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

#endif /* PHYSFSX_H */
