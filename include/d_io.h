// some misc. file/disk routines
// Arne de Bruijn, 1998
#ifndef _D_IO_H
#define _D_IO_H

#if (defined(__MSDOS__) && !defined(__DJGPP__)) || defined(__WINDOWS__)
#include <io.h>
#else
#include <unistd.h>
#endif

#include <stdio.h>
extern long ffilelength(FILE *fh);
unsigned long d_getdiskfree();
// remove extension from filename, doesn't work with paths.
void removeext(const char *filename, char *out);

#endif
