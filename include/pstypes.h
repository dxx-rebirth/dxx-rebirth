/* $Id: pstypes.h,v 1.23 2003-11-27 00:36:14 btb Exp $ */
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
 * Common types for use in Miner
 *
 * Old Log:
 * Revision 1.2  1993/09/14  12:12:30  matt
 * Added #define for NULL
 *
 * Revision 1.1  1993/08/24  12:50:40  matt
 * Initial revision
 *
 *
 */

#ifndef _TYPES_H
#define _TYPES_H

// define a dboolean
typedef int dboolean;

//define a signed byte
typedef signed char sbyte;

//define unsigned types;
typedef unsigned char ubyte;
#ifndef __unix__
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
#endif
#if defined(__APPLE__) && defined(__MACH__)
typedef unsigned long ulong;
#endif

#ifndef __linux__
#include <SDL_types.h>
typedef Sint16 int16_t;
typedef Sint32 int32_t;
typedef Sint64 int64_t;
typedef Uint16 u_int16_t;
typedef Uint32 u_int32_t;
typedef Uint64 u_int64_t;
#endif

#ifdef _MSC_VER
# include <stdlib.h> // this is where min and max are defined
#endif
#ifndef min
#define min(a,b) (((a)>(b))?(b):(a))
#endif
#ifndef max
#define max(a,b) (((a)<(b))?(b):(a))
#endif

#ifdef _WIN32
# ifndef __MINGW32__
#  define PATH_MAX _MAX_PATH
# endif
# ifdef _WIN32_WCE
#  define _MAX_DIR 256
# endif
#elif defined(__unix__)
# include <sys/types.h>
# define _MAX_PATH 1024
# define _MAX_DIR 256
#elif defined __DJGPP__
# include <sys/types.h>
# define _MAX_PATH 255
# define _MAX_DIR 63
#endif

#ifndef __cplusplus
//define a boolean
typedef ubyte bool;
#endif

#ifndef NULL
#define NULL 0
#endif

// the following stuff has nothing to do with types but needed everywhere,
// and since this file is included everywhere, it's here.
#ifdef __GNUC__
# define __pack__ __attribute__((packed))
#elif defined(_MSC_VER)
# pragma pack(push, packing)
# pragma pack(1)
# define __pack__
#else
# error d2x will not work without packed structures
#endif

#ifdef _MSC_VER
# define inline __inline
#endif

#ifndef PACKAGE_STRING
# define PACKAGE_STRING "d2x"
#endif

#endif //_TYPES_H

