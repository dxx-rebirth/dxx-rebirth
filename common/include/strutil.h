/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#ifndef _STRUTILS_H
#define _STRUTILS_H

#ifdef __cplusplus
#include "dxxsconf.h"
#include <vector>

#if defined(macintosh)
extern void snprintf(char *out_string, int size, const char * format, ... );
#endif
extern int d_stricmp( const char *s1, const char *s2 );
extern int d_strnicmp( const char *s1, const char *s2, int n );
extern void d_strlwr( char *s1 );
extern void d_strupr( char *s1 );
extern void d_strrev( char *s1 );
#ifdef DEBUG_MEMORY_ALLOCATIONS
extern char *d_strdup(const char *str) __attribute_malloc();
#else
#include <cstring>
#define d_strdup strdup
#endif

struct splitpath_t
{
	const char *drive_start, *drive_end, *path_start, *path_end, *base_start, *base_end, *ext_start;
};

// remove extension from filename, doesn't work with paths.
void removeext(const char *filename, char *out);

//give a filename a new extension, doesn't work with paths with no extension already there
extern void change_filename_extension( char *dest, const char *src, const char *new_ext );

// split an MS-DOS path into drive, directory path, filename without the extension (base) and extension.
// if it's just a filename with no directory specified, this function will get 'base' and 'ext'
void d_splitpath(const char *name, struct splitpath_t *path);

struct string_array_t
{
	std::vector<char> buffer;
	std::vector<const char *> ptr;
	string_array_t()
	{
	}
	void clear()
	{
		ptr.clear();
		buffer.clear();
	}
	void add(const char *);
	void tidy(std::size_t offset, int (*comp)( const char *, const char * ));
};

int string_array_sort_func(char **e0, char **e1);

#endif

#endif /* _STRUTILS_H */
