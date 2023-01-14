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
#include <optional>
#include <type_traits>
#include "dxxsconf.h"
#include "ntstring.h"

template <std::size_t N>
static inline bool cmp(const char *token, const char *equal, const char (&name)[N])
{
	return &token[N - 1] == equal && !strncmp(token, name, N - 1);
}

template <typename T>
static inline std::optional<T> convert_integer(const char *value, int base, auto libc_str_to_integer)
{
	char *e;
	const auto r = libc_str_to_integer(value, &e, base);
	if (*e)
		/* Trailing garbage found */
		return std::nullopt;
	if constexpr (std::is_same<T, bool>::value)
	{
		if (r != 0 && r != 1)
			return std::nullopt;
	}
	else if (!std::in_range<T>(r))
		/* Result truncated */
		return std::nullopt;
	return static_cast<T>(r);
}

template <typename T>
requires(std::is_signed<T>::value)
static inline auto convert_integer(const char *value, int base = 10)
{
	return convert_integer<T>(value, base, std::strtol);
}

template <typename T>
requires(std::is_unsigned<T>::value)
static inline auto convert_integer(const char *value, int base = 10)
{
	return convert_integer<T>(value, base, std::strtoul);
}

template <typename T>
requires(std::is_integral<T>::value)
static inline void convert_integer(T &t, const char *value, int base = 10)
{
	if (auto r = convert_integer<T>(value, base))
		t = *r;
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
