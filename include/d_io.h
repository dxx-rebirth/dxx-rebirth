// some misc. file/disk routines
// Arne de Bruijn, 1998
#ifndef _D_IO_H
#define _D_IO_H

#ifdef __WINDOWS__
#include <io.h>
#else
#include <unistd.h>
#endif

extern long ffilelength(FILE *fh);
extern long filelength(int fd);
unsigned long d_getdiskfree();
// remove extension from filename, doesn't work with paths.
void removeext(const char *filename, char *out);

unsigned long GetDiskFree();

#if defined(__WINDOWS__) || defined(__MINGW32__)
#define d_mkdir(path) mkdir(path)
#else
#define d_mkdir(path) mkdir(path, 0755)
#endif

#endif
