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
 
#ifndef _STRUTILS_
#define _STRUTILS_

char *d_strdup(char *str);

#if defined __CYGWIN__
/* nothing needed */

#elif defined __ENV_LINUX__
#define stricmp(a,b) strcasecmp(a,b)
#define strnicmp(a,b,c) strncasecmp(a,b,c)

void strupr( char *s1 );
void strlwr( char *s1 );
#elif defined __ENV_DJGPP__
// Nothing needed

#else
extern int stricmp(char *str1, char *str2);
extern int strnicmp(char *str1, char *str2, int n);

void strupr( char *s1 );
void strlwr( char *s1 );
#endif

void strrev( char *s1 );

void _splitpath(char *name, char *drive, char *path, char *base, char *ext);

#endif
