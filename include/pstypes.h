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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/
/*
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/include/types.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:46:36 $
 *
 * Common types for use in Miner
 *
 * $Log: types.h,v $
 * Revision 1.1.1.1  2006/03/17 19:46:36  zicodxx
 * initial import
 *
 * Revision 1.7  2000/03/01 09:25:54  sekmu
 * no u_char for djgpp??
 *
 * Revision 1.6  2000/02/07 07:45:08  donut
 * in c++ bool is a builtin type, so don't define then
 *
 * Revision 1.5  1999/12/13 09:21:09  sekmu
 * added u_int64 to non-linux
 *
 * Revision 1.4  1999/11/13 02:47:19  sekmu
 * include sys/types.h for ssize_t for djgpp
 *
 * Revision 1.3  1999/10/15 05:22:15  donut
 * typedef'd ssize_t on windows
 *
 * Revision 1.2  1999/08/05 22:53:41  sekmu
 *
 * D3D patch(es) from ADB
 *
 * Revision 1.1.1.1  1999/06/14 22:02:22  donut
 * Import of d1x 1.37 source.
 *
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
// added 03/24/99 Arne de Bruijn - fix for MSVC warning
#include <stdlib.h>
// end additions - adb

#ifdef __WINDOWS__
typedef long ssize_t;
#endif

#if defined(__DJGPP__) || defined(__unix__)
#include <sys/types.h>
#endif
#ifdef __DJGPP__
typedef unsigned char u_char;  // ehhh?
#endif

//define a byte 
typedef signed char sbyte;

//define unsigned types;
typedef unsigned char ubyte;
#if defined(__WINDOWS__) || defined(macintosh)
typedef unsigned short ushort;
typedef unsigned int uint;
#endif

#if defined(__WINDOWS__) || defined(__sun__) // platforms missing (u_)int??_t
# include <SDL/SDL_types.h>
#elif defined(macintosh) // misses (u_)int??_t and does not like SDL_types.h
# include <MacTypes.h>
typedef SInt16 int16_t;
typedef SInt32 int32_t;
typedef SInt64 int64_t;
typedef UInt16 u_int16_t;
typedef UInt32 u_int32_t;
typedef UInt64 u_int64_t;
#endif // macintosh
#if defined(__WINDOWS__) || defined(__sun__) // platforms missing u_int??_t
typedef Uint16 u_int16_t;
typedef Uint32 u_int32_t;
typedef Uint64 u_int64_t;
#endif // defined(__WINDOWS__) || defined(__sun__)

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b)) /* WARNING! sideeffect sensitive */
#define max(a,b) ((a) > (b) ? (a) : (b)) /* WARNING! sideeffect sensitive */
#endif
#ifndef MIN
#define MIN min
#define MAX max
#endif

#ifndef __cplusplus
//define a boolean
typedef ubyte bool;
#endif

#ifdef __MSDOS__ /* adb: doesn't really belong here, but so it's surely included */
#include <dir.h>
#include <pc.h>
#undef DRIVE /* adb: used internally */
#define _MAX_DRIVE 3
#define _MAX_DIR 65
#define _MAX_FNAME 8
#define _MAX_EXT 4
#define _splitpath(full,drive,dir,name,ext) fnsplit(full,drive,dir,name,ext)
#define _makepath(full,drive,dir,name,ext) fnmerge(full,drive,dir,name,ext)
#define _dos_setvect(int, handler)
#define _dos_getvect(int) NULL
#endif

#ifdef _WIN32
# ifndef __MINGW32__
#  define PATH_MAX _MAX_PATH
# endif
# define FNAME_MAX 256
#elif defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
# include <sys/types.h>
# ifndef PATH_MAX
#  define PATH_MAX 1024
# endif
# define FNAME_MAX 256
#elif defined __DJGPP__
# include <sys/types.h>
# define FNAME_MAX 9	// excluding extension
#elif defined(macintosh)
# define PATH_MAX 256
# define FNAME_MAX 32
#endif

// the following stuff has nothing to do with types but needed everywhere,
// and since this file is included everywhere, it's here.
#if  defined(__i386__) || defined(__ia64__) || defined(WIN32) || \
(defined(__alpha__) || defined(__alpha)) || \
defined(__arm__) || defined(ARM) || \
(defined(__mips__) && defined(__MIPSEL__)) || \
defined(__SYMBIAN32__) || \
defined(__x86_64__) || \
defined(__LITTLE_ENDIAN__)	// from physfs_internal.h
//# define WORDS_BIGENDIAN 0
#else
# define WORDS_BIGENDIAN 1
#endif

#ifdef __GNUC__
# define __pack__ __attribute__((packed))
#elif defined(_MSC_VER)
# pragma pack(push, packing)
# pragma pack(1)
# define __pack__
#elif defined(macintosh)
# pragma options align=packed
# define __pack__
#else
# error d2x will not work without packed structures
#endif


#ifdef _MSC_VER
#pragma warning (disable: 4103)
#ifndef inline
#define inline _inline
#endif
#ifndef far
#define far
#endif
#endif

#endif
