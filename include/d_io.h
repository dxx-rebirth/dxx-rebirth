// some misc. file/disk routines
// Arne de Bruijn, 1998
#ifndef _D_IO_H
#define _D_IO_H

#ifndef _WIN32_WCE
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#endif

extern long ffilelength(FILE *fh);
#if 0
extern long filelength(int fd);
#endif
unsigned long d_getdiskfree();
// remove extension from filename, doesn't work with paths.
void removeext(const char *filename, char *out);

unsigned long GetDiskFree();

#endif
