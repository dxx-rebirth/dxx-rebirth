
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
#include "dxxerror.h"
#include "vecmat.h"
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

static inline int PHYSFSX_writeString(PHYSFS_file *file, const char *s)
{
	return PHYSFS_write(file, s, 1, strlen(s) + 1);
}

static inline int PHYSFSX_puts(PHYSFS_file *file, const char *s)
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

static inline int PHYSFSX_fgetc(PHYSFS_file *const fp)
{
	unsigned char c;

	if (PHYSFS_read(fp, &c, 1, 1) != 1)
		return EOF;

	return c;
}

static inline int PHYSFSX_fseek(PHYSFS_file *fp, long int offset, int where)
{
	int c, goal_position;

	switch(where)
	{
	case SEEK_SET:
		goal_position = offset;
		break;
	case SEEK_CUR:
		goal_position = PHYSFS_tell(fp) + offset;
		break;
	case SEEK_END:
		goal_position = PHYSFS_fileLength(fp) + offset;
		break;
	default:
		return 1;
	}
	c = PHYSFS_seek(fp, goal_position);
	return !c;
}

static inline char * PHYSFSX_fgets(char *buf, size_t n, PHYSFS_file *const fp)
{
	size_t i;
	int c;

	for (i = 0; i < n - 1; i++)
	{
		do
		{
			c = PHYSFSX_fgetc(fp);
			if (c == EOF)
			{
				*buf = 0;

				return NULL;
			}
			if (c == 0 || c == 10)  // Unix line ending
				break;
			if (c == 13)            // Mac or DOS line ending
			{
				int c1;

				c1 = PHYSFSX_fgetc(fp);
				if (c1 != EOF)  // The file could end with a Mac line ending
					PHYSFSX_fseek(fp, -1, SEEK_CUR);
				if (c1 == 10) // DOS line ending
					continue;
				else            // Mac line ending
					break;
			}
		} while (c == 13);
		if (c == 13)    // because cr-lf is a bad thing on the mac
			c = '\n';   // and anyway -- 0xod is CR on mac, not 0x0a
		if (c == '\n')
			break;
		*buf++ = c;
	}
	*buf = 0;

	return buf;
}

static inline int PHYSFSX_printf(PHYSFS_file *file, const char *format, ...)
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

static inline int PHYSFSX_readInt(PHYSFS_file *file)
{
	int i;

	if (!PHYSFS_readSLE32(file, &i))
	{
		Error("Error reading int in PHYSFSX_readInt()");
		exit(1);
	}

	return i;
}

static inline short PHYSFSX_readShort(PHYSFS_file *file)
{
	int16_t s;

	if (!PHYSFS_readSLE16(file, &s))
	{
		Error("Error reading short in PHYSFSX_readShort()");
		exit(1);
	}

	return s;
}

static inline sbyte PHYSFSX_readByte(PHYSFS_file *file)
{
	sbyte b;

	if (PHYSFS_read(file, &b, sizeof(b), 1) != 1)
	{
		Error("Error reading byte in PHYSFSX_readByte()");
		exit(1);
	}

	return b;
}

static inline fix PHYSFSX_readFix(PHYSFS_file *file)
{
	int f;  // a fix is defined as a long for Mac OS 9, and MPW can't convert from (long *) to (int *)

	if (!PHYSFS_readSLE32(file, &f))
	{
		Error("Error reading fix in PHYSFSX_readFix()");
		exit(1);
	}

	return f;
}

static inline fixang PHYSFSX_readFixAng(PHYSFS_file *file)
{
	fixang f;

	if (!PHYSFS_readSLE16(file, &f))
	{
		Error("Error reading fixang in PHYSFSX_readFixAng()");
		exit(1);
	}

	return f;
}

static inline void PHYSFSX_readVector(vms_vector *v, PHYSFS_file *file)
{
	v->x = PHYSFSX_readFix(file);
	v->y = PHYSFSX_readFix(file);
	v->z = PHYSFSX_readFix(file);
}

static inline void PHYSFSX_readAngleVec(vms_angvec *v, PHYSFS_file *file)
{
	v->p = PHYSFSX_readFixAng(file);
	v->b = PHYSFSX_readFixAng(file);
	v->h = PHYSFSX_readFixAng(file);
}

static inline void PHYSFSX_readMatrix(vms_matrix *m,PHYSFS_file *file)
{
	PHYSFSX_readVector(&m->rvec,file);
	PHYSFSX_readVector(&m->uvec,file);
	PHYSFSX_readVector(&m->fvec,file);
}

#define PHYSFSX_contfile_init PHYSFSX_addRelToSearchPath
#define PHYSFSX_contfile_close PHYSFSX_removeRelFromSearchPath

extern int PHYSFSX_addRelToSearchPath(const char *relname, int add_to_end);
extern int PHYSFSX_removeRelFromSearchPath(const char *relname);
extern int PHYSFSX_fsize(const char *hogname);
extern void PHYSFSX_listSearchPathContent();
extern int PHYSFSX_checkSupportedArchiveTypes();
extern int PHYSFSX_getRealPath(const char *stdPath, char *realPath);
extern int PHYSFSX_isNewPath(const char *path);
extern int PHYSFSX_rename(const char *oldpath, const char *newpath);
extern char **PHYSFSX_findFiles(const char *path, const char *const *exts);
extern char **PHYSFSX_findabsoluteFiles(const char *path, const char *realpath, const char *const *exts);
extern PHYSFS_sint64 PHYSFSX_getFreeDiskSpace();
extern int PHYSFSX_exists(const char *filename, int ignorecase);
extern PHYSFS_file *PHYSFSX_openReadBuffered(const char *filename);
extern PHYSFS_file *PHYSFSX_openWriteBuffered(const char *filename);
extern void PHYSFSX_addArchiveContent();
extern void PHYSFSX_removeArchiveContent();

#endif /* PHYSFSX_H */
