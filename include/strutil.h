#ifndef _STRUTILS_H
#define _STRUTILS_H

#if defined(macintosh)
extern void snprintf(char *out_string, int size, char * format, ... );
extern int stricmp(const char *str1, const char *str2);
extern int strnicmp(const char *str1, const char *str2, int n);
#elif !defined(_WIN32)
# include <string.h>
# define stricmp(a,b) strcasecmp(a,b)
# define strnicmp(a,b,c) strncasecmp(a,b,c)
#endif

#ifndef _WIN32
#ifndef __DJGPP__
void strupr( char *s1 );
void strlwr( char *s1 );
#endif

void strrev( char *s1 );
#endif

// remove extension from filename, doesn't work with paths.
void removeext(const char *filename, char *out);

//give a filename a new extension, doesn't work with paths.
extern void change_filename_extension( char *dest, const char *src, char *new_ext );

#if !(defined(_WIN32))
void _splitpath(char *name, char *drive, char *path, char *base, char *ext);
#endif

#endif /* _STRUTILS_H */
