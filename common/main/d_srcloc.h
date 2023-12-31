/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

#pragma once

#include <span>
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
		static std::pair<std::size_t, std::span<char, N>> insert_location_leader(std::array<char, N> &buffer)
		{
			return {{}, buffer};
		}
	/* Define overloads to preserve const qualification */
	static std::span<char> prepare_buffer(scratch_buffer<0> &, const std::span<char> text)
	{
		return text;
	}
	static std::span<const char> prepare_buffer(scratch_buffer<0> &, const std::span<const char> text)
	{
		return text;
	}
};

#if DXX_HAVE_CXX_BUILTIN_FILE_LINE
#include <cstdio>

template <>
class location_wrapper<true>
{
	const char *file;
	unsigned line;
public:
	template <std::size_t N>
		using scratch_buffer = std::array<char, N>;
	location_wrapper(const char *const f = __builtin_FILE(), const unsigned l = __builtin_LINE()) :
		file{f}, line{l}
	{
	}
	/* Return a span describing the unwritten area into which the caller can
	 * place further text.
	 */
	template <std::size_t N>
		std::pair<std::size_t, std::span<char>> insert_location_leader(std::array<char, N> &buffer)
		{
			const auto written = std::snprintf(buffer.data(), buffer.size(), "%s:%u: ", file, line);
			return {written, std::span(buffer).subspan(written)};
		}
	/* Return a span describing the written area that the caller can read
	 * without accessing undefined bytes.
	 */
	template <std::size_t N>
		std::span<const char> prepare_buffer(std::array<char, N> &buffer, const std::span<const char> text) const
		{
			const auto written = std::snprintf(buffer, sizeof(buffer), "%s:%u: %.*s", file, line, static_cast<int>(text.size()), text.data());
			return {buffer, written};
		}
	/* Delegate to the const-qualified version, but return a `char *` to
	 * match the non-const input `char *`, to preserve the choice of
	 * overload for the function to receive this result.
	 */
	template <std::size_t N>
		std::span<char> prepare_buffer(std::array<char, N> &buffer, const std::span<char> text) const
		{
			return {buffer, prepare_buffer(buffer, std::span<const char>(text)).size()};
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
#if !DXX_HAVE_CXX_BUILTIN_FILE_LINE
	location_value_wrapper(const T &v) :
		value{v}
	{
	}
#endif
	location_value_wrapper(const T &v,
		const char *const f
#if DXX_HAVE_CXX_BUILTIN_FILE_LINE
		= __builtin_FILE()
#endif
		, const unsigned l
#if DXX_HAVE_CXX_BUILTIN_FILE_LINE
		= __builtin_LINE()
#endif
		) :
		location_wrapper<capture_source_location>{f, l},
		value{v}
	{
	}
	operator T() const
	{
		return value;
	}
};
