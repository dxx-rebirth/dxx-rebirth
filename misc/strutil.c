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

char *d_strdup(char *str)
{
	char *newstr;

	MALLOC(newstr, char, strlen(str) + 1);
	strcpy(newstr, str);

	return newstr;
}

// remove extension from filename
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


//give a filename a new extension, won't append if strlen(dest) > 8 chars.
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

void d_splitpath(char *name, char *drive, char *path, char *base, char *ext)
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

// create a growing 2D array with a single growing buffer for the text
// this system is likely to cause less memory fragmentation than having one malloc'd buffer per string
int string_array_new(char ***list, char **list_buf, int *num_str, int *max_str, int *max_buf)
{
	*max_str = 1024;
	MALLOC(*list, char *, 1024);
	if (*list == NULL)
		return 0;
	
	*max_buf = 1024;			// bigger?
	MALLOC(*list_buf, char, 1024);
	if (*list_buf == NULL)
	{
		d_free(*list);
		return 0;
	}
	*num_str = 0;
	(*list)[0] = *list_buf;
	
	return 1;
}

int string_array_add(char ***list, char **list_buf, int *num_str, int *max_str, int *max_buf, const char *str)
{
	char *next_str = *num_str ? (*list)[*num_str - 1] + strlen((*list)[*num_str - 1]) + 1 : *list_buf;
	
	// Need to grow an array?
	if (*num_str >= *max_str)
	{
		char **new_list = (char **) d_realloc(*list, *max_str*sizeof(char *)*MEM_K);
		if (new_list == NULL)
			return 0;
		*max_str *= MEM_K;
		*list = new_list;
	}
	
	if (next_str + strlen(str) + 1 - *list_buf >= *max_buf)
	{
		int i;
		char *new_buf;
		
		new_buf = (char *) d_realloc(*list_buf, *max_buf*sizeof(char)*MEM_K);
		if (new_buf == NULL)
			return 0;

		// Update all the pointers in the pointer list
		for (i = 0; i < *num_str; i++)
			(*list)[i] += (new_buf - *list_buf);

		*max_buf *= MEM_K;
		*list_buf = new_buf;
		next_str = *num_str ? (*list)[*num_str - 1] + strlen((*list)[*num_str - 1]) + 1 : *list_buf;
	}
	
	strcpy(next_str, str);
	(*list)[(*num_str)++] = next_str;
	
	return 1;
}

int string_array_sort_func(char **e0, char **e1)
{
	return d_stricmp(*e0, *e1);
}

void string_array_tidy(char ***list, char **list_buf, int *num_str, int *max_str, int *max_buf, int offset, int (*comp)( const char *, const char * ))
{
	char *temp_buf, **temp_list;
	int i, j;
	
	// Reduce memory fragmentation
	temp_list = (char **) d_realloc(*list, sizeof(char *)*(*num_str ? *num_str : 1));
	if (temp_list)
	{
		*list = temp_list;
		*max_str = *num_str;
	}
	
	j = *num_str ? (*list)[*num_str - 1] + strlen((*list)[*num_str - 1]) + 1 - *list_buf : 1;	// buffer size - a bit of variable recycling
	temp_buf = (char *) d_realloc(*list_buf, j);
	if (temp_buf)
	{
		for (i = 0; i < *num_str; i++)
			(*list)[i] += (temp_buf - *list_buf);
		*list_buf = temp_buf;
		*max_buf = j;	// set to buffer size used
	}
	
	// Sort by name, starting at offset
	qsort(&(*list)[offset], *num_str - offset, sizeof(char *), (int (*)( const void *, const void * ))string_array_sort_func);
	
	// Remove duplicates
	// Can't do this before reallocating, otherwise it makes a mess of things (the strings in the buffer aren't ordered)
	for (i = offset, j = offset; i < *num_str; i++)
		if ((i == offset) || comp((*list)[i - 1], (*list)[i]))
			(*list)[j++] = (*list)[i];

	*num_str = j;
}
