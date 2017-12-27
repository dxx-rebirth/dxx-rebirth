/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#pragma once

#include <type_traits>
#include <utility>
#include "dxxsconf.h"

template <bool capture_source_location = false>
class location_wrapper
{
public:
	location_wrapper() = default;
	/* Allow callers to pass explicit file/line, for signature
	 * compatibility with `location_wrapper<true>`.
	 */
	location_wrapper(const char *, unsigned)
	{
	}
	/* This must be a template to be compatible with the `scratch_buffer`
	 * template-typedef in `location_wrapper<true>`.
	 */
	template <std::size_t N>
		using scratch_buffer = std::false_type;
	template <std::size_t N>
		static std::pair<char (&)[N], std::integral_constant<std::size_t, N>> insert_location_leader(char (&buffer)[N])
		{
			return {buffer, {}};
		}
	/* Define overloads to preserve const qualification */
	static std::pair<char *, std::size_t> prepare_buffer(scratch_buffer<0> &, char *const text, const std::size_t len)
	{
		return {text, len};
	}
	static std::pair<const char *, std::size_t> prepare_buffer(scratch_buffer<0> &, const char *const text, const std::size_t len)
	{
		return {text, len};
	}
};

#ifdef DXX_HAVE_CXX_BUILTIN_FILE_LINE
#include <cstdio>

template <>
class location_wrapper<true>
{
	const char *file;
	unsigned line;
public:
	template <std::size_t N>
		using scratch_buffer = char[N];
	location_wrapper(const char *const f = __builtin_FILE(), const unsigned l = __builtin_LINE()) :
		file(f), line(l)
	{
	}
	template <std::size_t N>
		std::pair<char *, std::size_t> insert_location_leader(char (&buffer)[N]) const
		{
			const auto written = std::snprintf(buffer, sizeof(buffer), "%s:%u: ", file, line);
			return {buffer + written, sizeof(buffer) - written};
		}
	template <std::size_t N>
		std::pair<const char *, std::size_t> prepare_buffer(char (&buffer)[N], const char *const text, const std::size_t len) const
		{
			const auto written = std::snprintf(buffer, sizeof(buffer), "%s:%u: %.*s", file, line, static_cast<int>(len), text);
			return {buffer, len + written};
		}
	/* Delegate to the const-qualified version, but return a `char *` to
	 * match the non-const input `char *`, to preserve the choice of
	 * overload for the function to receive this result.
	 */
	template <std::size_t N>
		std::pair<char *, std::size_t> prepare_buffer(char (&buffer)[N], char *const text, const std::size_t len) const
		{
			return {buffer, prepare_buffer(buffer, const_cast<const char *>(text), len).second};
		}
};
#endif

template <typename T, bool capture_source_location = false>
class location_value_wrapper : public location_wrapper<capture_source_location>
{
	T value;
public:
	/* Allow callers to pass explicit file/line, for signature
	 * compatibility with `location_value_wrapper<T, true>`.
	 */
#ifndef DXX_HAVE_CXX_BUILTIN_FILE_LINE
	location_value_wrapper(const T &v) :
		value(v)
	{
	}
#endif
	location_value_wrapper(const T &v,
		const char *const f
#ifdef DXX_HAVE_CXX_BUILTIN_FILE_LINE
		= __builtin_FILE()
#endif
		, const unsigned l
#ifdef DXX_HAVE_CXX_BUILTIN_FILE_LINE
		= __builtin_LINE()
#endif
		) :
		location_wrapper<capture_source_location>(f, l),
		value(v)
	{
	}
	operator T() const
	{
		return value;
	}
};
