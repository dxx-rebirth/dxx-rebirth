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
 * code to swap bytes because of big/little endian problems.
 * contains the macros:
 * SWAP{INT64,INT,SHORT}(x): returns a swapped version of x
 * INTEL_{INT64,INT,SHORT}(x): returns x after conversion to/from little endian
 * GET_INTEL_{INT64,INT,SHORT}(src): gets value from little-endian buffer src
 * PUT_INTEL_{INT64,INT,SHORT}(dest, src): puts src into little-endian buffer dest
 *
 * the GET/PUT macros are safe to use on platforms which segfault on unaligned word access
 *
 */

#pragma once

#include <bit>
#include <concepts>
#include <cstdint>
#include <string.h>    // for memcpy
#include <type_traits>
#include "dxxsconf.h"
#include "pstypes.h"

[[nodiscard]]
static constexpr uint16_t SWAPSHORT(const uint16_t &x)
{
	return std::byteswap(x);
}

[[nodiscard]]
static constexpr int16_t SWAPSHORT(const int16_t &i)
{
	return std::byteswap(i);
}

[[nodiscard]]
static constexpr uint32_t SWAPINT(const uint32_t &x)
{
	return std::byteswap(x);
}

[[nodiscard]]
static constexpr int32_t SWAPINT(const int32_t &i)
{
	return std::byteswap(i);
}

#if !DXX_WORDS_BIGENDIAN
#define byteutil_choose_endian(F,a)	(a)
#else // ! WORDS_BIGENDIAN
#define byteutil_choose_endian(F,a)	(F(a))
#endif // ! WORDS_BIGENDIAN
constexpr std::integral_constant<int, DXX_WORDS_BIGENDIAN> words_bigendian{};

[[nodiscard]]
static constexpr uint16_t INTEL_SHORT(const uint16_t &x)
{
	return byteutil_choose_endian(SWAPSHORT, x);
}

[[nodiscard]]
static constexpr int16_t INTEL_SHORT(const int16_t &x)
{
	return byteutil_choose_endian(SWAPSHORT, x);
}

[[nodiscard]]
static constexpr uint32_t INTEL_INT(const uint32_t &x)
{
	return byteutil_choose_endian(SWAPINT, x);
}

[[nodiscard]]
static constexpr int32_t INTEL_INT(const int32_t &x)
{
	return byteutil_choose_endian(SWAPINT, x);
}
#undef byteutil_choose_endian

template <std::integral result_type = std::uint32_t>
requires(
	sizeof(result_type) == sizeof(uint32_t)
)
[[nodiscard]]
static inline result_type GET_INTEL_INT(const auto *p)
{
	result_type u;
	memcpy(&u, p, sizeof(u));
	return INTEL_INT(u);
}

template <std::integral result_type = std::uint16_t>
requires(
	sizeof(result_type) == sizeof(uint16_t)
)
[[nodiscard]]
static inline result_type GET_INTEL_SHORT(const auto *p)
{
	result_type u;
	memcpy(&u, p, sizeof(u));
	return INTEL_SHORT(u);
}

template <typename T>
requires(
	std::same_as<T, int16_t> ||
	requires(T t) { uint16_t{t}; }
)
static inline void PUT_INTEL_SHORT(uint16_t *d, const T s)
{
	const auto u = INTEL_SHORT(s);
	memcpy(d, &u, sizeof(u));
}

template <typename T>
requires(
	std::same_as<T, int16_t> ||
	requires(T t) { uint16_t{t}; }
)
static inline void PUT_INTEL_SHORT(uint8_t *d, const T s)
{
	const auto u = INTEL_SHORT(s);
	memcpy(d, &u, sizeof(u));
}

template <typename T>
requires(
	std::same_as<T, int16_t> ||
	std::same_as<T, int32_t> ||
	requires(T t) { uint32_t{t}; }
)
static inline void PUT_INTEL_INT(uint32_t *d, const T s)
{
	const auto u = INTEL_INT(s);
	memcpy(d, &u, sizeof(u));
}

template <typename T>
requires(
	std::same_as<T, int16_t> ||
	std::same_as<T, int32_t> ||
	requires(T t) { uint32_t{t}; }
)
static inline void PUT_INTEL_INT(uint8_t *d, const T s)
{
	const auto u = INTEL_INT(s);
	memcpy(d, &u, sizeof(u));
}
