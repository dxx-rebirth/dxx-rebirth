//Created on 6/15/99 by Owen Evans to finally remove all those "implicit
//declaration" warnings for the string functions in Linux

#if defined(macintosh)
extern void snprintf(char *out_string, int size, char * format, ... );
extern int stricmp(const char *s1, const char *s2);
extern int strnicmp(const char *s1, const char *s2, int n);
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

// remove extension from filename, doesn't work with paths.
void removeext(const char *filename, char *out);

//give a filename a new extension
extern void change_filename_extension( char *dest, const char *src, char *new_ext );

#endif
