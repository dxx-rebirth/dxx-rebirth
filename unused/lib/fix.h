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

#ifndef _FIX_H
#define _FIX_H

#include "pstypes.h"

typedef long fix;				//16 bits int, 16 bits frac
typedef short fixang;		//angles

typedef struct quad {
	ulong low;
	long high;
} quad;

//Convert an int to a fix
#define i2f(i) ((i)<<16)

//Get the int part of a fix
#define f2i(f) ((f)>>16)

//Get the int part of a fix, with rounding
#define f2ir(f) (((f)+f0_5)>>16)

//Convert fix to float and float to fix
#define f2fl(f) (((float) (f)) / 65536.0)
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

fix fixmul(fix a,fix b);
#pragma aux fixmul parm [eax] [edx] = \
	"imul	edx"				\
	"shrd	eax,edx,16";


fix fixdiv(fix a,fix b);
#pragma aux fixdiv parm [eax] [ebx] modify exact [eax edx] = \
	"mov	edx,eax"	\
	"sar	edx,16"	\
	"shl	eax,16"	\
	"idiv	ebx";

fix fixmuldiv(fix a,fix b,fix c);
#pragma aux fixmuldiv parm [eax] [edx] [ebx] modify exact [eax edx] = \
	"imul	edx"	\
	"idiv	ebx";

#pragma aux fixmulaccum parm [esi] [eax] [edx] modify exact [eax edx] = \
	"imul	edx"			\
	"add  [esi],eax"	\
	"adc	4[esi],edx";

#pragma aux fixquadadjust parm [esi] modify exact [eax edx] = \
	"mov  eax,[esi]"		\
	"mov  edx,4[esi]"		\
	"shrd	eax,edx,16";

#pragma aux fixquadnegate parm [eax] modify exact [ebx] = \
	"mov	ebx,[eax]"		\
	"neg	ebx"				\
	"mov	[eax],ebx"		\
	"mov	ebx,4[eax]"		\
	"not	ebx"				\
	"sbb	ebx,-1"			\
	"mov	4[eax],ebx";

#pragma aux fixdivquadlong parm [eax] [edx] [ebx] modify exact [eax edx] = \
	"idiv	ebx";

//computes the square root of a long, returning a short
ushort long_sqrt(long a);

//computes the square root of a quad, returning a long
ulong quad_sqrt(long low,long high);

//computes the square root of a fix, returning a fix
fix fix_sqrt(fix a);

//multiply two fixes, and add 64-bit product to a quad
void fixmulaccum(quad *q,fix a,fix b);

//extract a fix from a quad product
fix fixquadadjust(quad *q);

//divide a quad by a long
long fixdivquadlong(ulong qlow,long qhigh,long d);

//negate a quad
void fixquadnegate(quad *q);

//compute sine and cosine of an angle, filling in the variables
//either of the pointers can be NULL
void fix_sincos(fix a,fix *s,fix *c);		//with interpolation
void fix_fastsincos(fix a,fix *s,fix *c);	//no interpolation

//compute inverse sine & cosine
fixang fix_asin(fix v); 
fixang fix_acos(fix v); 

//given cos & sin of an angle, return that angle.
//parms need not be normalized, that is, the ratio of the parms cos/sin must
//equal the ratio of the actual cos & sin for the result angle, but the parms 
//need not be the actual cos & sin.  
//NOTE: this is different from the standard C atan2, since it is left-handed.
fixang fix_atan2(fix cos,fix sin); 

#pragma aux fix_fastsincos parm [eax] [esi] [edi] modify exact [eax ebx];
#pragma aux fix_sincos parm [eax] [esi] [edi] modify exact [eax ebx];

#pragma aux fix_asin "*" parm [eax] value [ax] modify exact [eax];
#pragma aux fix_acos "*" parm [eax] value [ax] modify exact [eax];
#pragma aux fix_atan2 "*" parm [eax] [ebx] value [ax] modify exact [eax ebx];

#pragma aux long_sqrt "*" parm [eax] value [ax] modify [];
#pragma aux fix_sqrt "*" parm [eax] value [eax] modify [];
#pragma aux quad_sqrt "*" parm [eax] [edx] value [eax] modify [];

#endif
