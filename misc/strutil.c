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


#ifdef _MSC_VER
int strcasecmp( char *s1, char *s2 )
{ return _stricmp(s1,s2); }

int strncasecmp(char *s1, char *s2, int n)
{ return _strnicmp(s1,s2,n); }

#endif

#if ((!defined(__WINDOWS__) && !defined(__DJGPP__)) || defined(__LCC__))
// string compare without regard to case

#define strcmpi(a,b) stricmp(a,b)

int stricmp( const char *s1, const char *s2 )
{
	while( *s1 && *s2 )	{
		if ( tolower(*s1) != tolower(*s2) )	return 1;
		s1++;
		s2++;
	}
	if ( *s1 || *s2 ) return 1;
	return 0;
}


#ifndef __LCC__

int strnicmp( const char *s1, const char *s2, int n )
{
	while( *s1 && *s2 && n)	{
		if ( tolower(*s1) != tolower(*s2) )	return 1;
		s1++;
		s2++;
		n--;
	}
	return 0;
}

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
	int i,l;
	char *s2;
	
        s2 = (char *) malloc(strlen(s1) + 1);
	strcpy(s2, s1);
	l = strlen(s2);
	for (i = 0; i < l; i++)
		s1[l-1-i] = s2[i];
        free(s2);
}

#endif
