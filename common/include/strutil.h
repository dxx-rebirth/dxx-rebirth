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

// create a growing 2D array with a single growing buffer for the text
// this system is likely to cause less memory fragmentation than having one malloc'd buffer per string
int string_array_new(char ***list, char **list_buf, int *num_str, int *max_str, int *max_buf);

// add a string to a growing 2D array
int string_array_add(char ***list, char **list_buf, int *num_str, int *max_str, int *max_buf, const char *str);

// sort function passed to qsort - also useful for 2d string arrays with individual string pointers 
int string_array_sort_func(char **e0, char **e1);

// reallocate pointers to save memory, sort list alphabetically and remove duplicates according to 'comp'
void string_array_tidy(char ***list, char **list_buf, int *num_str, int *max_str, int *max_buf, int offset, int (*comp)( const char *, const char * ));

#endif

#endif /* _STRUTILS_H */
