/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#ifdef __cplusplus
#include <cstddef>
#include <cstdint>
#include <cstring>
#include "dxxsconf.h"
#include "dsx-ns.h"
#include <array>
#include <vector>
#if DXX_USE_EDITOR
#include <limits.h>	/* PATH_MAX */
#endif

namespace dcx {

#if defined(macintosh)
#define snprintf macintosh_snprintf
extern void snprintf(char *out_string, int size, const char * format, ... );
#endif

#ifdef DXX_HAVE_STRCASECMP
#define d_stricmp strcasecmp
static inline int d_strnicmp(const char *s1, const char *s2, size_t n)
{
	return strncasecmp(s1, s2, n);
}
#else
__attribute_nonnull()
extern int d_stricmp( const char *s1, const char *s2 );
__attribute_nonnull()
int d_strnicmp(const char *s1, const char *s2, uint_fast32_t n);
#endif
extern void d_strlwr( char *s1 );
extern void d_strrev( char *s1 );
#ifdef DEBUG_MEMORY_ALLOCATIONS
char *d_strdup(const char *str, const char *, const char *, unsigned) __attribute_malloc();
#define d_strdup(str)	(d_strdup(str, #str, __FILE__,__LINE__))
#else
#include <cstring>
#define d_strdup strdup
#endif

#if DXX_USE_EDITOR
void d_strupr(std::array<char, PATH_MAX> &out, const std::array<char, PATH_MAX> &in);
#endif

template <std::size_t N>
static inline int d_strnicmp(const char *s1, const char (&s2)[N])
{
	return d_strnicmp(s1, s2, N - 1);
}

struct splitpath_t
{
	const char *drive_start, *drive_end, *path_start, *path_end, *base_start, *base_end, *ext_start;
};

// remove extension from filename, doesn't work with paths.
void removeext(const char *filename, std::array<char, 20> &out);

//give a filename a new extension, doesn't work with paths with no extension already there
extern void change_filename_extension( char *dest, const char *src, const char *new_ext );

// split an MS-DOS path into drive, directory path, filename without the extension (base) and extension.
// if it's just a filename with no directory specified, this function will get 'base' and 'ext'
void d_splitpath(const char *name, struct splitpath_t *path);

class string_array_t
{
	typedef std::vector<const char *> ptr_t;
	std::vector<char> buffer;
	ptr_t ptr;
public:
	ptr_t &pointer() { return ptr; }
	void clear()
	{
		ptr.clear();
		buffer.clear();
	}
	void add(const char *);
	void tidy(std::size_t offset);
};

int string_array_sort_func(const void *v0, const void *v1);

/* Compute the number of digits required to represent MaximumValue in
 * the given Base.  The result does not account for any characters other
 * than the required digits.
 */
template <std::size_t MaximumValue, std::size_t Base = 10>
constexpr std::size_t number_to_text_length = (MaximumValue < Base) ? 1 : 1 + number_to_text_length<MaximumValue / Base, Base>;
static_assert(number_to_text_length<1> == 1, "");
static_assert(number_to_text_length<9> == 1, "");
static_assert(number_to_text_length<10> == 2, "");
static_assert(number_to_text_length<255, 16> == 2, "");
static_assert(number_to_text_length<256, 16> == 3, "");

}

#endif
