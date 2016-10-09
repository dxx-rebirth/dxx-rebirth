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

#include <cstdint>
#include <string.h>    // for memcpy
#include "pstypes.h"

static inline uint16_t SWAPSHORT(const uint16_t &x)
{
	return (x << 8) | (x >> 8);
}

static inline int16_t SWAPSHORT(const int16_t &i)
{
	return SWAPSHORT(static_cast<uint16_t>(i));
}

static inline uint32_t SWAPINT(const uint32_t &x)
{
	return (x << 24) | (x >> 24) | ((x & 0xff00) << 8) | ((x >> 8) & 0xff00);
}

static inline int32_t SWAPINT(const int32_t &i)
{
	return SWAPINT(static_cast<uint32_t>(i));
}

#if !DXX_WORDS_BIGENDIAN
/* Always resolve F(a), so ambiguous calls are flagged even on little
 * endian.
 */
#define byteutil_choose_endian(F,a)	(static_cast<void>(static_cast<decltype(F(a))>(0)), a)
constexpr int words_bigendian = 0;
#else // ! WORDS_BIGENDIAN
#define byteutil_choose_endian(F,a)	(F(a))
constexpr int words_bigendian = 1;
#endif // ! WORDS_BIGENDIAN

#if !DXX_WORDS_NEED_ALIGNMENT
#define byteutil_unaligned_copy(dt, d, s)	(static_cast<dt &>(d) = *reinterpret_cast<const dt *>(s))
#else // ! WORDS_NEED_ALIGNMENT
#define byteutil_unaligned_copy(dt, d, s)	memcpy(&static_cast<dt &>(d), (s), sizeof(d))
#endif // ! WORDS_NEED_ALIGNMENT

template <typename T>
static inline T INTEL_SHORT(const T &x)
{
	return byteutil_choose_endian(SWAPSHORT, x);
}

template <typename T>
static inline T INTEL_INT(const T &x)
{
	return byteutil_choose_endian(SWAPINT, x);
}
#undef byteutil_choose_endian

template <typename T>
static inline uint32_t GET_INTEL_INT(const T *p)
{
	uint32_t u;
	byteutil_unaligned_copy(uint32_t, u, p);
	return INTEL_INT(u);
}

template <typename T>
static inline uint16_t GET_INTEL_SHORT(const T *p)
{
	uint16_t u;
	byteutil_unaligned_copy(uint16_t, u, p);
	return INTEL_SHORT(u);
}

template <typename T>
static inline void PUT_INTEL_SHORT(uint16_t *d, const T &s)
{
	uint16_t u = INTEL_SHORT(s);
	byteutil_unaligned_copy(uint16_t, *d, &u);
}

template <typename T>
static inline void PUT_INTEL_SHORT(uint8_t *d, const T &s)
{
	PUT_INTEL_SHORT<T>(reinterpret_cast<uint16_t *>(d), s);
}

template <typename T>
static inline void PUT_INTEL_INT(uint32_t *d, const T &s)
{
	uint32_t u = INTEL_INT(s);
	byteutil_unaligned_copy(uint32_t, *d, &u);
}

template <typename T>
static inline void PUT_INTEL_INT(uint8_t *d, const T &s)
{
	PUT_INTEL_INT<T>(reinterpret_cast<uint32_t *>(d), s);
}

#undef byteutil_unaligned_copy
