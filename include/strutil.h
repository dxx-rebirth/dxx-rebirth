//Created on 6/15/99 by Owen Evans to finally remove all those "implicit
//declaration" warnings for the string functions in Linux

#if defined(macintosh)
extern int stricmp(const char *s1, const char *s2);
extern int strnicmp(const char *s1, const char *s2, int n);
#elif !defined(__WINDOWS__)
# include <string.h>
# define stricmp(a,b) strcasecmp(a,b)
# define strnicmp(a,b,c) strncasecmp(a,b,c)
#endif

#ifdef _WIN32_WCE
# define stricmp _stricmp
# define strnicmp _strnicmp
# define strlwr _strlwr
# define strrev _strrev
#endif

#ifndef __WINDOWS__
#ifndef __DJGPP__
void strupr( char *s1 );
void strlwr( char *s1 );
#endif

void strrev( char *s1 );

//give a filename a new extension
extern void change_filename_extension( char *dest, char *src, char *new_ext );

#endif
