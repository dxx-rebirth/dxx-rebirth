/* $Id: strutil.h,v 1.14 2004-12-20 07:12:25 btb Exp $ */
/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifndef _STRUTILS_H
#define _STRUTILS_H

#if defined(macintosh)
 extern int stricmp(const char *str1, const char *str2);
 extern int strnicmp(const char *str1, const char *str2, int n);
#elif !defined(_WIN32)
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

#ifndef _WIN32
#ifndef __DJGPP__
void strupr( char *s1 );
void strlwr( char *s1 );
#endif

void strrev( char *s1 );
#endif

// remove extension from filename, doesn't work with paths.
void removeext(const char *filename, char *out);

#if !defined(__MSDOS__) && !(defined(_WIN32) && !defined(_WIN32_WCE))
void _splitpath(char *name, char *drive, char *path, char *base, char *ext);
#endif

#endif /* _STRUTILS_H */
