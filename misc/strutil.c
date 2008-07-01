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
 *
 * string manipulation utility code
 *
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "u_mem.h"
#include "inferno.h"
#include "error.h"

#ifdef macintosh
void snprintf(char *out_string, int size, char * format, ... )
{
	va_list		args;
	char		buf[1024];
	
	va_start(args, format );
	vsprintf(buf,format,args);
	va_end(args);

	// Hack! Don't know any other [simple] way to do this, but I doubt it would ever exceed 1024 long.
	Assert(strlen(buf) < 1024);
	Assert(size < 1024);

	strncpy(out_string, buf, size);
}

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

#ifndef _WIN32
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

// remove extension from filename, doesn't work with paths.
void removeext(const char *filename, char *out)
{
	char *p;
	
	if ((p = strrchr(filename, '.')))
	{
		strncpy(out, filename, p - filename);
		out[p - filename] = 0;
	}
	else
		strcpy(out, filename);
}


//give a filename a new extension
void change_filename_extension( char *dest, const char *src, char *ext )
{
	char *p;
	
	strcpy (dest, src);
	
	if (ext[0] == '.')
		ext++;
	
	p = strrchr(dest, '.');
	if (!p) {
		if (strlen(dest) > FILENAME_LEN - 5)
			return;	// a non-opened file is better than a bad memory access
		
		p = dest + strlen(dest);
		*p = '.';
	}
	
	strcpy(p+1,ext);
}
