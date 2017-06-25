/*
 * This file is copyright by Rebirth contributors and licensed as
 * described in COPYING.txt.
 * See COPYING.txt for license details.
 */

#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include "dxxsconf.h"
#include "compiler-type_traits.h"
#include "ntstring.h"

template <std::size_t N>
static inline bool cmp(const char *token, const char *equal, const char (&name)[N])
{
	return &token[N - 1] == equal && !strncmp(token, name, N - 1);
}

template <typename F, typename T>
static inline bool convert_integer(F *f, T &t, const char *value, int base)
{
	char *e;
	auto r = (*f)(value, &e, base);
	if (*e)
		/* Trailing garbage found */
		return false;
	T tr = static_cast<T>(r);
	if (r != tr)
		/* Result truncated */
		return false;
	t = tr;
	return true;
}

template <typename T>
static inline typename std::enable_if<std::is_signed<T>::value, bool>::type convert_integer(T &t, const char *value, int base = 10)
{
	return convert_integer(strtol, t, value, base);
}

template <typename T>
static inline typename std::enable_if<std::is_unsigned<T>::value, bool>::type convert_integer(T &t, const char *value, int base = 10)
{
	return convert_integer(strtoul, t, value, base);
}

template <std::size_t N>
static inline void convert_string(ntstring<N> &out, const char *const value, const char *eol)
{
	assert(*eol == 0);
	const std::size_t i = std::distance(value, ++ eol);
	if (i > out.size())
		/* Only if not truncated */
		return;
	std::copy(value, eol, out.begin());
}
