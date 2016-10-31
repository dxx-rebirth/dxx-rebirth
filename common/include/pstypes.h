/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 *
 * Common types and defines
 *
 */

#ifndef _TYPES_H
#define _TYPES_H

#ifndef macintosh
#include <sys/types.h>
#endif

#include <cstdint>
#include <limits.h>

//define a signed byte
typedef signed char sbyte;

//define unsigned types;
typedef unsigned char ubyte;
#if defined(_WIN32) || defined(macintosh)
typedef unsigned short ushort;
typedef unsigned int uint;
#endif

#if defined(_WIN32) || defined(__sun__) // platforms missing (u_)int??_t
# include <SDL_types.h>
#elif defined(macintosh) // misses (u_)int??_t and does not like SDL_types.h
# include <MacTypes.h>
 typedef SInt16 int16_t;
 typedef SInt32 int32_t;
 typedef SInt64 int64_t;
 typedef UInt16 uint16_t;
 typedef UInt32 uint32_t;
 typedef UInt64 uint64_t;
#endif // macintosh
#if defined(_WIN32) || defined(__sun__) // platforms missing u_int??_t
 typedef Uint16 uint16_t;
 typedef Uint32 uint32_t;
 typedef Uint64 uint64_t;
#endif // defined(_WIN32) || defined(__sun__)

#ifdef _MSC_VER
# include <stdlib.h> // this is where min and max are defined
#endif

// the following stuff has nothing to do with types but needed everywhere,
// and since this file is included everywhere, it's here.
#if  defined(__i386__) || defined(__ia64__) || defined(_WIN32) || \
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
#ifdef WIN32
# define __pack__ __attribute__((gcc_struct, packed))
#else
# define __pack__ __attribute__((packed))
#endif
#elif defined(_MSC_VER)
# pragma pack(push, packing)
# pragma pack(1)
# define __pack__
#elif defined(macintosh)
# pragma options align=packed
# define __pack__
#else
# error "This program will not work without packed structures"
#endif

#endif //_TYPES_H

