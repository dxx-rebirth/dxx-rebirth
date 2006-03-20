/* Maths.h library header file */

#ifndef _MATHS_H
#define _MATHS_H

#define NO_FIX_INLINE 1

#include <stdlib.h>		// for RAND_MAX
#include "types.h"



#define D_RAND_MAX 32767

void d_srand (unsigned int seed);
int d_rand ();			// Random number function which returns in the range 0-0x7FFF


//=============================== FIXED POINT ===============================

typedef int32_t fix;		//16 bits int, 16 bits frac
typedef int16_t fixang;		//angles

typedef struct quad
  {
    u_int32_t low;
    int32_t high;
  }
quad;


//Convert an int to a fix
#define i2f(i) ((i)<<16)

//Get the int part of a fix
#define f2i(f) ((f)>>16)

//Get the int part of a fix, with rounding
#define f2ir(f) (((f)+f0_5)>>16)

//Convert fix to float and float to fix
#define f2fl(f) (((float)  (f)) / 65536.0)
#define f2db(f) (((double) (f)) / 65536.0)
#define fl2f(f) ((fix) ((f) * 65536))

//Some handy constants
#define f0_0	0
#define f1_0	0x10000
#define f2_0	0x20000
#define f3_0	0x30000
#define f10_0	0xa0000

#define f0_5 0x8000
#define f0_1 0x199a

#define F0_0	f0_0
#define F1_0	f1_0
#define F2_0	f2_0
#define F3_0	f3_0
#define F10_0	f10_0

#define F0_5 	f0_5
#define F0_1 	f0_1

//#if defined(NO_FIX_INLINE) || (!defined(__GNUC__) && !defined(__WATCOMC__))
//multiply two fixes, return a fix
fix fixmul (fix a, fix b);

//divide two fixes, return a fix
fix fixdiv (fix a, fix b);

//multiply two fixes, then divide by a third, return a fix
fix fixmuldiv (fix a, fix b, fix c);
#if 0

//#else

//#ifdef __WATCOMC__
fix fixmul (fix a, fix b);

#pragma aux fixmul parm [eax] [edx] = \
"imul	edx" \
"shrd	eax,edx,16";



fix fixdiv (fix a, fix b);

#pragma aux fixdiv parm [eax] [ebx] modify exact [eax edx] = \
"mov	edx,eax" \
"sar	edx,16" \
"shl	eax,16" \
"idiv	ebx";


fix fixmuldiv (fix a, fix b, fix c);

#pragma aux fixmuldiv parm [eax] [edx] [ebx] modify exact [eax edx] = \
"imul	edx" \
"idiv	ebx";

//#elif defined (__GNUC__)
/*
static inline fix
fixmul (fix a, fix b)
{
  
    fix __retval;
  
    asm (" imul   %2;"
	 " shrd   $16,%%edx,%%eax" 
: "=a" (__retval): "a" (a), "d" (b):"%edx");
  
    return __retval;
  
} */

//multiply two fixes, return a fix
fix fixmul (fix a, fix b);


//divide two fixes, return a fix
fix fixdiv (fix a, fix b);

//multiply two fixes, then divide by a third, return a fix
fix fixmuldiv (fix a, fix b, fix c);

//#if 0
static inline fix
fixdiv (fix a, fix b)
{
  
    register fix __retval;
  
    asm (" mov %%eax,%%edx;" 
	 " sar $16,%%edx;" 
	 " shl $16,%%eax;" 
	 " idiv %%ecx" /* adb: how to make this %0 w/o chance of edx? */  
: "=a" (__retval): "a" (a), "c" (b):"%edx");
  
    return __retval;
  
}

static inline fix
fixmuldiv (fix a, fix b, fix c)
{
  
    register fix __retval;
  
    asm (" imul   %0;" 
	 " idiv   %%ecx" /* adb: how to make this %0 w/o chance of edx? */  
: "=a" (__retval): "a" (a), "r" (b), "c" (c):"%edx");
  
    return __retval;

}
//#endif

//#endif // __GNUC__ / __WATCOMC__
//#endif /* defined NO_FIX_INLINE || (!defined __GNUC__ && !defined __WATCOMC__) */

#endif

//multiply two fixes, and add 64-bit product to a quad
void fixmulaccum (quad * q, fix a, fix b);

//extract a fix from a quad product
fix fixquadadjust (quad * q);

//divide a quad by a long
int32_t fixdivquadlong (u_int32_t qlow, u_int32_t qhigh, u_int32_t d);

//negate a quad
void fixquadnegate (quad * q);

//computes the square root of a long, returning a short
ushort long_sqrt (int32_t a);

//computes the square root of a quad, returning a long
u_int32_t quad_sqrt (u_int32_t low, int32_t high);
//ulong quad_sqrt (long low, long high);

//computes the square root of a fix, returning a fix
fix fix_sqrt (fix a);

//compute sine and cosine of an angle, filling in the variables
//either of the pointers can be NULL
void fix_sincos (fix a, fix * s, fix * c);	//with interpolation

void fix_fastsincos (fix a, fix * s, fix * c);	//no interpolation

//compute inverse sine & cosine
fixang fix_asin (fix v);

fixang fix_acos (fix v);

//given cos & sin of an angle, return that angle.
//parms need not be normalized, that is, the ratio of the parms cos/sin must
//equal the ratio of the actual cos & sin for the result angle, but the parms 
//need not be the actual cos & sin.  
//NOTE: this is different from the standard C atan2, since it is left-handed.
fixang fix_atan2 (fix cos, fix sin);

//for passed value a, returns 1/sqrt(a) 
fix fix_isqrt (fix a);

#endif
