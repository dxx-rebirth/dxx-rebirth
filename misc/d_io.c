/* $Id: d_io.c,v 1.7 2003-11-27 00:21:04 btb Exp $ */
/*
 * some misc. file/disk routines
 * Arne de Bruijn, 1998
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <string.h>
#include "d_io.h"
#ifdef __DJGPP__
#include "dos_disk.h"
#endif
//added 05/17/99 Matt Mueller
#include "u_mem.h"
//end addition -MM

#if defined(_WIN32) && !defined(_WIN32_WCE)
#include <windows.h>
#define lseek(a,b,c) _lseek(a,b,c)
#endif

#if 0
long filelength(int fd) {
	long old_pos, size;

	if ((old_pos = lseek(fd, 0, SEEK_CUR)) == -1 ||
	    (size = lseek(fd, 0, SEEK_END)) == -1 ||
	    (lseek(fd, old_pos, SEEK_SET)) == -1)
		return -1L;
	return size;
}
#endif

long ffilelength(FILE *file)
{
	long old_pos, size;

	if ((old_pos = ftell(file)) == -1 ||
	    fseek(file, 0, SEEK_END) == -1 ||
	    (size = ftell(file)) == -1 ||
	    fseek(file, old_pos, SEEK_SET) == -1)
		return -1L;
	return size;
}


unsigned long d_getdiskfree()
{
#ifdef __MSDOS__
  return getdiskfree();
#else
#if 0//def __WINDOWS__
	ULONG cbCluster = 0;
	ULONG cClusters = 0;

	GetDiskFreeSpace (
		NULL,
                (int *) &cbCluster,
		NULL,
                (int *) &cClusters,
		NULL);
	
	return cbCluster * cClusters;
#else
  // FIXME:
  return 999999999;
#endif
#endif
}

unsigned long GetDiskFree()
{
 return d_getdiskfree();
}

// remove extension from filename, doesn't work with paths.
void removeext(const char *filename, char *out) {
	char *p;
	if ((p = strrchr(filename, '.'))) {
		strncpy(out, filename, p - filename);
		out[p - filename] = 0;
	} else
		strcpy(out, filename);
}
