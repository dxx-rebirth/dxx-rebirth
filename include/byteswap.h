/* $Id: byteswap.h,v 1.8 2003-10-03 04:01:21 btb Exp $ */
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
 * SWAP{INT,SHORT}(x): returns a swapped version of x
 * INTEL_{INT,SHORT}(x): returns x after conversion to/from little endian
 * GET_INTEL_{INT,SHORT}(dest, src): gets value dest from buffer src
 * PUT_INTEL_{INT,SHORT}(dest, src): puts value src into buffer dest
 *
 * the GET/PUT macros are safe to use on platforms which segfault on unaligned word access
 *
 * Old Log:
 * Revision 1.4  1995/08/23  21:28:15  allender
 * fix mcc compiler warning
 *
 * Revision 1.3  1995/08/18  15:51:42  allender
 * put back in old byteswapping code
 *
 * Revision 1.2  1995/05/04  20:10:18  allender
 * proper prototypes
 *
 * Revision 1.1  1995/03/30  15:02:11  allender
 * Initial revision
 *
 */

#ifndef _BYTESWAP_H
#define _BYTESWAP_H

#include "pstypes.h"

#define SWAPSHORT(x) (((ubyte)(x) << 8) | (((ushort)(x)) >> 8))
#define SWAPINT(x)   (((x)<<24) | (((uint)(x)) >> 24) | (((x) &0x0000ff00) << 8) | (((x) & 0x00ff0000) >> 8))

#ifndef WORDS_BIGENDIAN
#define INTEL_INT(x)    x
#define INTEL_SHORT(x)  x
#else // ! WORDS_BIGENDIAN
#define INTEL_INT(x)    SWAPINT(x)
#define INTEL_SHORT(x)  SWAPSHORT(x)
#endif // ! WORDS_BIGENDIAN

#ifndef WORDS_NEED_ALIGNMENT
#define GET_INTEL_INT(d, s)     { (uint)(d) = INTEL_INT(*(uint *)(s)); }
#define GET_INTEL_SHORT(d, s)   { (ushort)(d) = INTEL_SHORT(*(ushort *)(s)); }
#define PUT_INTEL_INT(d, s)     { *(uint *)(d) = INTEL_INT((uint)(s)); }
#define PUT_INTEL_SHORT(d, s)   { *(ushort *)(d) = INTEL_SHORT((ushort)(s)); }
#else // ! WORDS_NEED_ALIGNMENT
#define GET_INTEL_INT(d, s)     { uint tmp; \
                                  memcpy((void *)&tmp, (void *)(s), 4); \
                                  (uint)(d) = INTEL_INT(tmp); }
#define GET_INTEL_SHORT(d, s)   { ushort tmp; \
                                  memcpy((void *)&tmp, (void *)(s), 4); \
                                  (ushort)(d) = INTEL_SHORT(tmp); }
#define PUT_INTEL_INT(d, s)     { uint tmp = INTEL_INT(s); \
                                  memcpy((void *)d, (void *)&tmp, 4);}
#define PUT_INTEL_SHORT(d, s)   { ushort tmp = INTEL_SHORT(s); \
                                  memcpy((void *)d, (void *)&tmp, 4);}
#endif // ! WORDS_NEED_ALIGNMENT

#endif // ! _BYTESWAP_H
