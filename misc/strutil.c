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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/
/*
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/misc/strutil.c,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:45:05 $
 *
 * string manipulation utility code
 * 
 * $Log: strutil.c,v $
 * Revision 1.1.1.1  2006/03/17 19:45:05  zicodxx
 * initial import
 *
 * Revision 1.4  1999/10/25 08:09:19  sekmu
 * compilable in dos again. __DOS__ is never defined.  use MSDOS or DJGPP
 *
 * Revision 1.3  1999/10/17 23:15:23  donut
 * made str(n)icmp have const args
 *
 * Revision 1.2  1999/10/14 04:48:21  donut
 * alpha fixes, and gl_font args
 *
 * Revision 1.1.1.1  1999/06/14 22:13:53  donut
 * Import of d1x 1.37 source.
 *
 * Revision 1.3  1995/07/17  10:44:11  allender
 * fixed strrev
 *
 * Revision 1.2  1995/05/04  20:10:59  allender
 * added some string routines
 *
 * Revision 1.1  1995/03/30  15:03:55  allender
 * Initial revision
 *
*/
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "u_mem.h"


#ifdef macintosh
# if defined(NDEBUG)
char *strdup(const char *str)
{
	char *newstr;
	
	newstr = malloc(strlen(str) + 1);
	strcpy(newstr, str);
	
	return newstr;
}
# endif // NDEBUG

// string compare without regard to case

int stricmp( const char *s1, const char *s2 )
{
	int u1;
	int u2;
	
	do {
		u1 = toupper((int) *s1);
		u2 = toupper((int) *s2);
		if (u1 != u2)
			return (u1 > u2) ? 1 : -1;
		
		s1++;
		s2++;
	} while (u1 && u2);
	
	return 0;
}

int strnicmp( const char *s1, const char *s2, int n )
{
	int u1;
	int u2;
	
	do {
		u1 = toupper((int) *s1);
		u2 = toupper((int) *s2);
		if (u1 != u2)
			return (u1 > u2) ? 1 : -1;
		
		s1++;
		s2++;
		n--;
	} while (u1 && u2 && n);
	
	return 0;
}
#endif // macintosh

#ifdef _MSC_VER
int strcasecmp( char *s1, char *s2 )
{ return _stricmp(s1,s2); }

int strncasecmp(char *s1, char *s2, int n)
{ return _strnicmp(s1,s2,n); }

#endif

#ifndef __WINDOWS__
#ifndef __DJGPP__
void strlwr( char *s1 )
{
	while( *s1 )	{
		*s1 = tolower(*s1);
		s1++;
	}
}

void strupr( char *s1 )
{
        while( *s1 )    {
              *s1 = toupper(*s1);
              s1++;
        }
}

#endif

void strrev( char *s1 )
{
	char *h, *t;
	h = s1;
	t = s1 + strlen(s1) - 1;
	while (h < t) {
		char c;
		c = *h;
		*h++ = *t;
		*t-- = c;
	}
}
#endif
