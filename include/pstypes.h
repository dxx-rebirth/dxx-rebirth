/* $Id: pstypes.h,v 1.19 2003-04-11 23:51:48 btb Exp $ */
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

#ifndef _TYPES_H
#define _TYPES_H

// define a dboolean
typedef int dboolean;

//define a byte
typedef signed char byte;

//define unsigned types;
typedef unsigned char ubyte;
#ifndef __unix__
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
#endif

#ifndef min
#define min(a,b) (((a)>(b))?(b):(a))
#endif
#ifndef max
#define max(a,b) (((a)<(b))?(b):(a))
#endif

#if defined __MINGW32__
#include <stdint.h>
typedef uint64_t u_int64_t;
typedef uint32_t u_int32_t;
typedef uint16_t u_int16_t;

#elif defined __unix__
# include <sys/types.h>
# define _MAX_PATH 1024
# define _MAX_DIR 256
# if defined(__APPLE__) && defined(__MACH__)
typedef unsigned long ulong;
# endif
# ifdef __sun__
typedef uint64_t u_int64_t;
typedef uint32_t u_int32_t;
typedef uint16_t u_int16_t;
# endif

#elif defined __DJGPP__
# include <sys/types.h>
# define _MAX_PATH 255
# define _MAX_DIR 63
typedef signed int int32_t;
typedef unsigned int u_int32_t;
typedef signed short int16_t;
typedef unsigned short u_int16_t;

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
#else
# define __pack__
#endif

#endif //_TYPES_H

