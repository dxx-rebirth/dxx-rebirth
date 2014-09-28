/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license below.
 * See COPYING.txt for license details.

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

/*
 *
 * string manipulation utility code
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>

#include "u_mem.h"
#include "dxxerror.h"
#include "strutil.h"
#include "inferno.h"

#include "compiler-range_for.h"

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
#endif // macintosh

// string compare without regard to case

int d_stricmp( const char *s1, const char *s2 )
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

int d_strnicmp( const char *s1, const char *s2, int n )
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

void d_strlwr( char *s1 )
{
	while( *s1 )	{
		*s1 = tolower(*s1);
		s1++;
	}
}

void d_strupr( char *s1 )
{
	while( *s1 )	{
		*s1 = toupper(*s1);
		s1++;
	}
}

void d_strrev( char *s1 )
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

#ifdef DEBUG_MEMORY_ALLOCATIONS
char *d_strdup(const char *str)
{
	char *newstr;

	MALLOC(newstr, char, strlen(str) + 1);
	strcpy(newstr, str);

	return newstr;
}
#endif

// remove extension from filename
void removeext(const char *filename, char *out)
{
	const char *p;

	if ((p = strrchr(filename, '.')))
	{
		strncpy(out, filename, p - filename);
		out[p - filename] = 0;
	}
	else
		strcpy(out, filename);
}


//give a filename a new extension, won't append if strlen(dest) > 8 chars.
void change_filename_extension( char *dest, const char *src, const char *ext )
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

void d_splitpath(const char *name, struct splitpath_t *path)
{
	const char *s, *p;

	p = name;
	s = strchr(p, ':');
	if ( s != NULL ) {
		path->drive_start = p;
		path->drive_end = s;
		p = s+1;
	} else
		path->drive_start = path->drive_end = NULL;
	s = strrchr(p, '\\');
	if ( s != NULL) {
		path->path_start = p;
		path->path_end = s + 1;
		p = s+1;
	} else
		path->path_start = path->path_end = NULL;

	s = strchr(p, '.');
	if ( s != NULL) {
		path->base_start = p;
		path->base_end = s;
		p = s+1;
	} else
		path->base_start = path->base_end = NULL;
	path->ext_start = p;
}

int string_array_sort_func(char **e0, char **e1)
{
	return d_stricmp(*e0, *e1);
}

void string_array_t::add(const char *s)
{
	const char *ob = &buffer.front();
	ptr.emplace_back(1 + &buffer.back());
	buffer.insert(buffer.end(), s, s + strlen(s) + 1);
	const char *nb = &buffer.front();
	if (ob != nb)
	{
		// Update all the pointers in the pointer list
		range_for (auto &i, ptr)
			i += (nb - ob);
	}
}

void string_array_t::tidy(std::size_t offset, int (*comp)( const char *, const char * ))
{
	// Sort by name, starting at offset
	auto b = std::next(ptr.begin(), offset);
	auto e = ptr.end();
	std::sort(b, e, [](const char *a, const char *b) { return d_stricmp(a, b) < 0; });
	// Remove duplicates
	// Can't do this before reallocating, otherwise it makes a mess of things (the strings in the buffer aren't ordered)
	ptr.erase(std::unique(b, e, comp), e);
}
