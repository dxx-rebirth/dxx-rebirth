/* $Id: physfsx.h,v 1.1.1.1 2006/03/17 20:01:28 zicodxx Exp $ */

/*
 *
 * Some simple physfs extensions
 *
 */

#ifndef PHYSFSX_H
#define PHYSFSX_H

#include <string.h>
#include <stdarg.h>

// When PhysicsFS can *easily* be built as a framework on Mac OS X,
// the framework form will be supported again -kreatordxx
#if 1	//!(defined(__APPLE__) && defined(__MACH__))
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
#include "byteswap.h"

extern void PHYSFSX_init(int argc, char *argv[]);

static inline int PHYSFSX_readSXE16(PHYSFS_file *file, int swap)
{
	PHYSFS_sint16 val;

	PHYSFS_read(file, &val, sizeof(val), 1);

	return swap ? SWAPSHORT(val) : val;
}

static inline int PHYSFSX_readSXE32(PHYSFS_file *file, int swap)
{
	PHYSFS_sint32 val;

	PHYSFS_read(file, &val, sizeof(val), 1);

	return swap ? SWAPINT(val) : val;
}

static inline void PHYSFSX_readVectorX(PHYSFS_file *file, vms_vector *v, int swap)
{
	v->x = PHYSFSX_readSXE32(file, swap);
	v->y = PHYSFSX_readSXE32(file, swap);
	v->z = PHYSFSX_readSXE32(file, swap);
}

static inline void PHYSFSX_readAngleVecX(PHYSFS_file *file, vms_angvec *v, int swap)
{
	v->p = PHYSFSX_readSXE16(file, swap);
	v->b = PHYSFSX_readSXE16(file, swap);
	v->h = PHYSFSX_readSXE16(file, swap);
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

extern void PHYSFSX_listSearchPathContent();
extern int PHYSFSX_checkSupportedArchiveTypes();
extern int PHYSFSX_getRealPath(const char *stdPath, char *realPath);
extern int PHYSFSX_isNewPath(char *path);
extern int PHYSFSX_rename(char *oldpath, char *newpath);
extern char **PHYSFSX_findFiles(char *path, char **exts);
extern char **PHYSFSX_findabsoluteFiles(char *path, char *realpath, char **exts);
extern PHYSFS_sint64 PHYSFSX_getFreeDiskSpace();
extern PHYSFS_file *PHYSFSX_openReadBuffered(char *filename);
extern PHYSFS_file *PHYSFSX_openWriteBuffered(char *filename);
extern void PHYSFSX_addArchiveContent();
extern void PHYSFSX_removeArchiveContent();

#endif /* PHYSFSX_H */
