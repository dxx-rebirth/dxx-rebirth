/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/* Maths.h library header file */

#pragma once

#include <cstdint>
#include <stdlib.h>
#include "pstypes.h"



#define D_RAND_MAX 32767

#include <cstddef>
#include "dsx-ns.h"
#include <array>

namespace dcx {

void d_srand (unsigned int seed);
__attribute_warn_unused_result
int d_rand ();			// Random number function which returns in the range 0-0x7FFF


//=============================== FIXED POINT ===============================

typedef int64_t fix64;		//64 bits int, for timers
typedef int32_t fix;		//16 bits int, 16 bits frac
typedef int16_t fixang;		//angles

typedef struct quadint // integer 64 bit, previously called "quad"
  {
	  union {
		  struct {
    uint32_t low;
    int32_t high;
		  };
		  int64_t q;
	  };
  }
quadint;


//Convert an int to a fix/fix64 and back
static constexpr fix i2f(const int &i)
{
	return i << 16;
}

static constexpr int f2i(const fix &f)
{
	return f >> 16;
}

//Convert fix to float and float to fix
static constexpr float f2fl(const fix &f)
{
	return static_cast<float>(f) / static_cast<float>(65536.0);
}

static constexpr double f2db(const fix &f)
{
	return static_cast<double>(f) / 65536.0;
}

static constexpr fix fl2f(const float &f)
{
	return static_cast<fix>(f * 65536);
}

//Some handy constants
#define f0_0	0
#define f1_0	0x10000
#define f2_0	0x20000
#define f3_0	0x30000
#define f10_0	0xa0000

#define f0_5 0x8000
#define f0_1 0x199a

//Get the int part of a fix, with rounding
static constexpr int f2ir(const fix &f)
{
	return (f + f0_5) >> 16;
}

#define F0_0	f0_0
#define F1_0	f1_0
#define F2_0	f2_0
#define F3_0	f3_0
#define F10_0	f10_0

#define F0_5 	f0_5
#define F0_1 	f0_1

//multiply two fixes, return a fix(64)
__attribute_warn_unused_result
fix64 fixmul64 (fix a, fix b);

/* On x86/amd64 for Windows/Linux, truncating fix64->fix is free. */
__attribute_warn_unused_result
static inline fix fixmul(fix a, fix b)
{
	return static_cast<fix>(fixmul64(a, b));
}

//divide two fixes, return a fix
__attribute_warn_unused_result
fix fixdiv (fix a, fix b);

//multiply two fixes, then divide by a third, return a fix
__attribute_warn_unused_result
fix fixmuldiv (fix a, fix b, fix c);

//multiply two fixes, and add 64-bit product to a quadint
static inline void fixmulaccum (quadint * q, const fix &a, const fix &b)
{
	q->q += static_cast<int64_t>(a) * static_cast<int64_t>(b);
}

//extract a fix from a quadint product
__attribute_warn_unused_result
static inline fix fixquadadjust (const quadint *q)
{
	return static_cast<fix>(q->q >> 16);
}

//negate a quadint
static inline void fixquadnegate (quadint * q)
{
	q->q = -q->q;
}

//computes the square root of a long, returning a short
__attribute_warn_unused_result
ushort long_sqrt (int32_t a);

//computes the square root of a quadint, returning a long
__attribute_warn_unused_result
uint32_t quad_sqrt (quadint);

//computes the square root of a fix, returning a fix
__attribute_warn_unused_result
fix fix_sqrt (fix a);

struct fix_sincos_result
{
	fix sin, cos;
};

__attribute_warn_unused_result
fix_sincos_result fix_sincos(fix);

//compute sine and cosine of an angle, filling in the variables
//either of the pointers can be NULL

__attribute_warn_unused_result
fix fix_sin(fix a);

__attribute_warn_unused_result
fix fix_cos(fix a);

__attribute_warn_unused_result
fix fix_fastsin(fix a);	//no interpolation

//compute inverse sine & cosine
__attribute_warn_unused_result
fixang fix_asin (fix v);

__attribute_warn_unused_result
fixang fix_acos (fix v);

//given cos & sin of an angle, return that angle.
//parms need not be normalized, that is, the ratio of the parms cos/sin must
//equal the ratio of the actual cos & sin for the result angle, but the parms 
//need not be the actual cos & sin.  
//NOTE: this is different from the standard C atan2, since it is left-handed.
__attribute_warn_unused_result
fixang fix_atan2 (fix cos, fix sin);

__attribute_warn_unused_result
int checkmuldiv(fix *r,fix a,fix b,fix c);

extern const std::array<ubyte, 256> guess_table;
extern const std::array<int16_t, 256> sincos_table;
extern const std::array<ushort, 258> asin_table;
extern const std::array<ushort, 258> acos_table;

static inline void clamp_fix_lh(fix& f, const fix& low, const fix& high)
{
	if (f < low)
		f = low;
	else if (high < f)
		f = high;
}

static inline void clamp_fix_symmetric(fix& f, const fix& bound)
{
	clamp_fix_lh(f, -bound, bound);
}

}
