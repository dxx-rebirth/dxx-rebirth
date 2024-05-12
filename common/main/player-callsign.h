/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license below.
 * See COPYING.txt for license details.
 */

#pragma once

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstddef>
#include <ranges>
#include <span>
#include "dxxsconf.h"
#include <array>

#define CALLSIGN_LEN                8       // so can use as filename (was: 12)

struct callsign_t
{
	static constexpr std::size_t array_length{CALLSIGN_LEN + 1};
	operator const void *() const = delete;
	std::array<char, array_length> a;
	[[nodiscard]]
	static char lower_predicate(char c)
	{
		return std::tolower(static_cast<unsigned>(c));
	}
	template <std::size_t Extent>
		requires(Extent == std::dynamic_extent || Extent <= array_length)
	void copy(const std::span<const char, Extent> s)
	{
		/* Zero the entire array first, so that word-sized moves can be used to
		 * store the null bytes in bulk.  Then copy an appropriate number of
		 * characters using byte-sized moves, with a limit to prevent
		 * overwriting the final null byte even if the input string is too
		 * long.
		 */
		a = {};
		std::ranges::copy_n(s.begin(), std::min(array_length - 1, s.size()), a.begin());
	}
	template <std::size_t Extent>
		requires(Extent == std::dynamic_extent || Extent <= array_length)
	void copy_lower(const std::span<const char, Extent> sc)
	{
		a = {};
		const auto input_begin{std::ranges::begin(sc)};
		std::ranges::transform(input_begin, std::next(input_begin, std::min(array_length - 1, sc.size())), a.begin(), lower_predicate);
	}
	template <std::size_t Extent>
	void copy_lower(const std::span<char, Extent> sc)
	{
		copy_lower(std::span<const char, Extent>(sc.begin(), sc.size()));
	}
	void lower()
	{
		const auto ba{a.begin()};
		/* The buffer should be null-terminated before calling `lower()`. */
		assert(!a.back());
		std::ranges::transform(ba, std::prev(a.end()), ba, lower_predicate);
	}
	[[nodiscard]]
		auto &buffer()
	{
		return *reinterpret_cast<char(*)[array_length]>(a.data());
	}
	template <std::size_t N>
		void operator=(const char (&s)[N])
		{
			copy(std::span(s));
		}
	void fill(const char c)
	{
		a.fill(c);
	}
	/* Only expose the `const` version of `operator[]`.  Callers should not use
	 * `operator[]` to modify the underlying array.
	 */
	[[nodiscard]]
	constexpr const char &operator[](const std::size_t i) const
	{
		return a[i];
	}
	[[nodiscard]]
	operator const char *() const
	{
		return a.data();
	};
	[[nodiscard]]
	constexpr bool operator==(const callsign_t &r) const = default;
};
static_assert(sizeof(callsign_t) == CALLSIGN_LEN + 1, "callsign_t too big");
