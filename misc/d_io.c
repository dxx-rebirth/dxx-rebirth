// some misc. file/disk routines
// Arne de Bruijn, 1998
#include <stdio.h>
#include <string.h>
#include "d_io.h"
#ifdef __MSDOS__
#include "disk.h"
#endif
//added 05/17/99 Matt Mueller
#include "u_mem.h"
//end addition -MM

#ifdef __WINDOWS__
#include <windows.h>
#define lseek(a,b,c) _lseek(a,b,c)
#endif

long ffilelength(FILE *fh) {
	long old_pos, size;
	int fd = fileno(fh);

	if ((old_pos = lseek(fd, 0, SEEK_CUR)) == -1 ||
	    (size = lseek(fd, 0, SEEK_END)) == -1 ||
	    (lseek(fd, old_pos, SEEK_SET)) == -1)
		return -1L;
	return size;
}

unsigned long d_getdiskfree()
{
#ifdef __MSDOS__
  return getdiskfree();
#else
#ifdef __WINDOWS__
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

// remove extension from filename, doesn't work with paths.
void removeext(const char *filename, char *out) {
	char *p;
	if ((p = strrchr(filename, '.'))) {
		strncpy(out, filename, p - filename);
		out[p - filename] = 0;
	} else
		strcpy(out, filename);
}
