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
 
#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "u_mem.h"
#include "error.h"

char *d_strdup(char *str)
{
	char *a;

	a = d_malloc(strlen(str) + 1);
	strcpy(a, str);

	return a;
}


#if !defined  __ENV_LINUX__ && !defined __ENV_DJGPP__
// string compare without regard to case

int stricmp( char *s1, char *s2 )
{
	while( *s1 && *s2 )	{
		if ( tolower(*s1) != tolower(*s2) )	return 1;
		s1++;
		s2++;
	}
	if ( *s1 || *s2 ) return 1;
	return 0;
}

int strnicmp( char *s1, char *s2, int n )
{
	while( *s1 && *s2 && n)	{
		if ( tolower(*s1) != tolower(*s2) )	return 1;
		s1++;
		s2++;
		n--;
	}
	if (n) return 1;
	return 0;
}

#endif

#if !defined __ENV_DJGPP__ && !defined __CYGWIN__

void strlwr( char *s1 )
{
	while( *s1 )	{
		*s1 = tolower(*s1);
		s1++;
	}
}

void strupr( char *s1 )
{
	while( *s1 )	{
		*s1 = toupper(*s1);
		s1++;
	}
}

#endif

void strrev( char *s1 )
{
	int i,l;
	char *s2;
	
	s2 = (char *)d_malloc(strlen(s1) + 1);
	strcpy(s2, s1);
	l = strlen(s2);
	for (i = 0; i < l; i++)
		s1[l-1-i] = s2[i];
	d_free(s2);
}

void _splitpath(char *name, char *drive, char *path, char *base, char *ext)
{
	char *s, *p;

	p = name;
	s = strchr(p, ':');
	if ( s != NULL ) {
		if (drive) {
			*s = '\0';
			strcpy(drive, p);
			*s = ':';
		}
		p = s+1;
		if (!p)
			return;
	} else if (drive)
		*drive = '\0';
	
	s = strrchr(p, '\\');
	if ( s != NULL) {
		if (path) {
			char c;
			
			c = *(s+1);
			*(s+1) = '\0';
			strcpy(path, p);
			*(s+1) = c;
		}
		p = s+1;
		if (!p)
			return;
	} else if (path)
		*path = '\0';

	s = strchr(p, '.');
	if ( s != NULL) {
		if (base) {
			*s = '\0';
			strcpy(base, p);
			*s = '.';
		}
		p = s+1;
		if (!p)
			return;
	} else if (base)
		*base = '\0';
		
	if (ext)
		strcpy(ext, p);		
}

#if 0
void main()
{
	char drive[10], path[50], name[16], ext[5];
	
	drive[0] = path[0] = name[0] = ext[0] = '\0';
	_splitpath("f:\\tmp\\x.out", drive, path, name, ext);
	drive[0] = path[0] = name[0] = ext[0] = '\0';
	_splitpath("tmp\\x.out", drive, path, name, ext);
	drive[0] = path[0] = name[0] = ext[0] = '\0';
	_splitpath("f:\\tmp\\a.out", NULL, NULL, name, NULL);
	drive[0] = path[0] = name[0] = ext[0] = '\0';
	_splitpath("tmp\\*.dem", drive, path, NULL, NULL);
	drive[0] = path[0] = name[0] = ext[0] = '\0';
	_splitpath(".\\tmp\\*.dem", drive, path, NULL, NULL);
}
#endif
