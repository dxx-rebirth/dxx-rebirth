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
#include "d_zip.h"

namespace dcx {

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

#ifndef DXX_HAVE_STRCASECMP
int d_stricmp( const char *s1, const char *s2 )
{
	for (;; ++s1, ++s2)
	{
		auto u1 = toupper(static_cast<unsigned>(*s1));
		auto u2 = toupper(static_cast<unsigned>(*s2));
		if (u1 != u2)
			return (u1 > u2) ? 1 : -1;
		if (!u1)
			return u1;
	}
}

int d_strnicmp(const char *s1, const char *s2, uint_fast32_t n)
{
	for (; n; ++s1, ++s2, --n)
	{
		auto u1 = toupper(static_cast<unsigned>(*s1));
		auto u2 = toupper(static_cast<unsigned>(*s2));
		if (u1 != u2)
			return (u1 > u2) ? 1 : -1;
		if (!u1)
			return u1;
	}
	return 0;
}
#endif

void d_strlwr( char *s1 )
{
	while( *s1 )	{
		*s1 = tolower(*s1);
		s1++;
	}
}

#if DXX_USE_EDITOR
void d_strupr(std::array<char, PATH_MAX> &out, const std::array<char, PATH_MAX> &in)
{
	for (auto &&[i, o] : zip(in, out))
	{
		o = std::toupper(static_cast<unsigned char>(i));
		if (!o)
			break;
	}
}
#endif

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
char *(d_strdup)(const char *str, const char *var, const char *file, unsigned line)
{
	char *newstr;

	const auto len = strlen(str) + 1;
	MALLOC<char>(newstr, len, var, file, line);
	return static_cast<char *>(memcpy(newstr, str, len));
}
#endif

// remove extension from filename
void removeext(const char *const filename, std::array<char, 20> &out)
{
	const char *p = nullptr;
	auto i = filename;
	for (; const char c = *i; ++i)
	{
		if (c == '.')
			p = i;
		/* No break - find the last '.', not the first. */
	}
	if (!p)
		p = i;
	const std::size_t rawlen = p - filename;
	const std::size_t copy_len = rawlen < out.size() ? rawlen : 0;
	out[copy_len] = 0;
	memcpy(out.data(), filename, copy_len);
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

int string_array_sort_func(const void *v0, const void *v1)
{
	const auto e0 = reinterpret_cast<const char *const *>(v0);
	const auto e1 = reinterpret_cast<const char *const *>(v1);
	return d_stricmp(*e0, *e1);
}

void string_array_t::add(const char *s)
{
	const auto insert_string = [this, s]{
		auto &b = this->buffer;
		b.insert(b.end(), s, s + strlen(s) + 1);
	};
	if (buffer.empty())
	{
		insert_string();
		ptr.emplace_back(&buffer.front());
		return;
	}
	const char *ob = &buffer.front();
	ptr.emplace_back(1 + &buffer.back());
	insert_string();
	if (auto d = &buffer.front() - ob)
	{
		// Update all the pointers in the pointer list
		range_for (auto &i, ptr)
			i += d;
	}
}

void string_array_t::tidy(std::size_t offset)
{
	// Sort by name, starting at offset
	auto b = std::next(ptr.begin(), offset);
	auto e = ptr.end();
#ifdef __linux__
#define comp strcmp
#else
#define comp d_stricmp
#endif
	std::sort(b, e, [](const char *sa, const char *sb) { return d_stricmp(sa, sb) < 0; });
					  
	// Remove duplicates
	// Can't do this before reallocating, otherwise it makes a mess of things (the strings in the buffer aren't ordered)
	ptr.erase(std::unique(b, e, [=](const char *sa, const char *sb) { return comp(sa, sb) == 0; }), e);
#undef comp
}

}
