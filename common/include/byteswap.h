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

#ifndef _BYTESWAP_H
#define _BYTESWAP_H

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
#ifndef macintosh
#define SWAPINT64(x) ((((x) & 0xff00000000000000LL) >> 56) | (((x) & 0x00ff000000000000LL) >> 40) | (((x) & 0x0000ff0000000000LL) >> 24) | (((x) & 0x000000ff00000000LL) >> 8) | (((x) & 0x00000000ff000000LL) << 8) | (((x) & 0x0000000000ff0000LL) << 24) | (((x) & 0x000000000000ff00LL) << 40) | (((x) & 0x00000000000000ffLL) << 56))
#else
#define SWAPINT64(x) ((((x) & 0xff00000000000000LL)/(2^56)) | (((x) & 0x00ff000000000000LL)/(2^40)) | (((x) & 0x0000ff0000000000LL)/(2^24)) | (((x) & 0x000000ff00000000LL)/(2^8)) | (((x) & 0x00000000ff000000LL)*(2^8)) | (((x) & 0x0000000000ff0000LL)*(2^24)) | (((x) & 0x000000000000ff00LL)*(2^40)) | (((x) & 0x00000000000000ffLL)*(2^56)))
#endif

#ifndef WORDS_BIGENDIAN
#define INTEL_INT64(x)  x
#define INTEL_INT(x)    x
#define INTEL_SHORT(x)  x
#else // ! WORDS_BIGENDIAN
#define INTEL_INT64(x)  SWAPINT64(x)
#define INTEL_INT(x)    SWAPINT(x)
#define INTEL_SHORT(x)  SWAPSHORT(x)
#endif // ! WORDS_BIGENDIAN

#ifndef WORDS_NEED_ALIGNMENT
#define GET_INTEL_INT(s)        INTEL_INT(*(const uint *)(s))
#define GET_INTEL_SHORT(s)      INTEL_SHORT(*(const ushort *)(s))
#define PUT_INTEL_INT(d, s)     { *(uint *)(d) = INTEL_INT((uint)(s)); }
#define PUT_INTEL_SHORT(d, s)   { *(ushort *)(d) = INTEL_SHORT((ushort)(s)); }
#else // ! WORDS_NEED_ALIGNMENT
static inline uint GET_INTEL_INT(const void *s)
{
	uint tmp;
	memcpy((void *)&tmp, s, 4);
	return INTEL_INT(tmp);
}
static inline ushort GET_INTEL_SHORT(const void *s)
{
	ushort tmp;
	memcpy((void *)&tmp, s, 2);
	return INTEL_SHORT(tmp);
}
#define PUT_INTEL_INT(d, s)     { uint tmp = INTEL_INT(s); \
                                  memcpy((void *)(d), (void *)&tmp, 4); }
#define PUT_INTEL_SHORT(d, s)   { ushort tmp = INTEL_SHORT(s); \
                                  memcpy((void *)(d), (void *)&tmp, 2); }
#endif // ! WORDS_NEED_ALIGNMENT

#endif // ! _BYTESWAP_H
