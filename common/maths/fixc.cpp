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
 *
 * C version of fixed point library
 *
 */

#include <stdlib.h>
#include <math.h>

#include "dxxerror.h"
#include "maths.h"

//negate a quad
void fixquadnegate(quadint *q)
{
	q->low  = 0 - q->low;
	q->high = 0 - q->high - (q->low != 0);
}

//multiply two ints & add 64-bit result to 64-bit sum
void fixmulaccum(quadint *q,fix a,fix b)
{
	u_int32_t aa,bb;
	u_int32_t ah,al,bh,bl;
	u_int32_t t,c=0,old;
	int neg;

	neg = ((a^b) < 0);

	aa = labs(a); bb = labs(b);

	ah = aa>>16;  al = aa&0xffff;
	bh = bb>>16;  bl = bb&0xffff;

	t = ah*bl + bh*al;

	if (neg)
		fixquadnegate(q);

	old = q->low;
	q->low += al*bl;
	if (q->low < old) q->high++;
	
	old = q->low;
	q->low += (t<<16);
	if (q->low < old) q->high++;
	
	q->high += ah*bh + (t>>16) + c;
	
	if (neg)
		fixquadnegate(q);

}

//extract a fix from a quad product
fix fixquadadjust(quadint *q)
{
	return (q->high<<16) + (q->low>>16);
}


#define EPSILON (F1_0/100)

fix fixmul(fix a, fix b)
{
	return (fix)((((fix64) a) * b) / 65536);
}

fix64 fixmul64(fix a, fix b)
{
	return (fix64)((((fix64) a) * b) / 65536);
}

fix fixdiv(fix a, fix b)
{
	return b ? (fix)((((fix64)a) *65536)/b) : 1;
}

fix fixmuldiv(fix a, fix b, fix c)
{
	return c ? (fix)((((fix64)a)*b)/c) : 1;
}

//given cos & sin of an angle, return that angle.
//parms need not be normalized, that is, the ratio of the parms cos/sin must
//equal the ratio of the actual cos & sin for the result angle, but the parms 
//need not be the actual cos & sin.  
//NOTE: this is different from the standard C atan2, since it is left-handed.

fixang fix_atan2(fix cos,fix sin)
{
	double d, dsin, dcos;
	fixang t;

	//Assert(!(cos==0 && sin==0));

	//find smaller of two

	dsin = (double)sin;
	dcos = (double)cos;
	d = sqrt((dsin * dsin) + (dcos * dcos));

	if (d==0.0)
		return 0;

	if (labs(sin) < labs(cos)) {				//sin is smaller, use arcsin
		t = fix_asin((fix)((dsin / d) * 65536.0));
		if (cos<0)
			t = 0x8000 - t;
		return t;
	}
	else {
		t = fix_acos((fix)((dcos / d) * 65536.0));
		if (sin<0)
			t = -t;
		return t;
	}
}

int32_t fixdivquadlong(u_int32_t nl,u_int32_t nh,u_int32_t d)
{
	int64_t n = (int64_t)nl | (((int64_t)nh) << 32 );
	return (signed int) (n / ((int64_t)d));
}

unsigned int fixdivquadlongu(uint nl, uint nh, uint d)
{
	u_int64_t n = (u_int64_t)nl | (((u_int64_t)nh) << 32 );
	return (unsigned int) (n / ((u_int64_t)d));
}

u_int32_t quad_sqrt(u_int32_t low,int32_t high)
{
	int i, cnt;
	u_int32_t r,old_r,t;
	quadint tq;

	if (high<0)
		return 0;

	if (high==0 && (int32_t)low>=0)
		return long_sqrt((int32_t)low);

	if (high & 0xff000000) {
		cnt=12+16; i = high >> 24;
	} else if (high & 0xff0000) {
		cnt=8+16; i = high >> 16;
	} else if (high & 0xff00) {
		cnt=4+16; i = high >> 8;
	} else {
		cnt=0+16; i = high;
	}
	
	r = guess_table[i]<<cnt;

	//quad loop usually executed 4 times

	r = fixdivquadlongu(low,high,r)/2 + r/2;
	r = fixdivquadlongu(low,high,r)/2 + r/2;
	r = fixdivquadlongu(low,high,r)/2 + r/2;

	do {

		old_r = r;
		t = fixdivquadlongu(low,high,r);

		if (t==r)	//got it!
			return r;
 
		r = t/2 + r/2;

	} while (!(r==t || r==old_r));

	t = fixdivquadlongu(low,high,r);
	//edited 05/17/99 Matt Mueller - tq.high is undefined here.. so set them to = 0
	tq.low=tq.high=0;
	//end edit -MM
	fixmulaccum(&tq,r,t);
	if (tq.low!=low || tq.high!=high)
		r++;

	return r;
}

//computes the square root of a long, returning a short
ushort long_sqrt(int32_t a)
{
	int cnt,r,old_r,t;

	if (a<=0)
		return 0;

	if (a & 0xff000000)
		cnt=12;
	else if (a & 0xff0000)
		cnt=8;
	else if (a & 0xff00)
		cnt=4;
	else
		cnt=0;
	
	r = guess_table[(a>>cnt)&0xff]<<cnt;

	//the loop nearly always executes 3 times, so we'll unroll it 2 times and
	//not do any checking until after the third time.  By my calcutations, the
	//loop is executed 2 times in 99.97% of cases, 3 times in 93.65% of cases, 
	//four times in 16.18% of cases, and five times in 0.44% of cases.  It never
	//executes more than five times.  By timing, I determined that is is faster
	//to always execute three times and not check for termination the first two
	//times through.  This means that in 93.65% of cases, we save 6 cmp/jcc pairs,
	//and in 6.35% of cases we do an extra divide.  In real life, these numbers
	//might not be the same.

	r = ((a/r)+r)/2;
	r = ((a/r)+r)/2;

	do {

		old_r = r;
		t = a/r;

		if (t==r)	//got it!
			return r;
 
		r = (t+r)/2;

	} while (!(r==t || r==old_r));

	if (a % r)
		r++;

	return r;
}

//computes the square root of a fix, returning a fix
fix fix_sqrt(fix a)
{
	return ((fix) long_sqrt(a)) << 8;
}


//compute sine and cosine of an angle, filling in the variables
//either of the pointers can be NULL
//with interpolation
void fix_sincos(fix a,fix *s,fix *c)
{
	int i,f;
	fix ss,cc;

	i = (a>>8)&0xff;
	f = a&0xff;

	ss = sincos_table[i];
	if (s) *s = (ss + (((sincos_table[i+1] - ss) * f)>>8))<<2;

	cc = sincos_table[i+64];
	if (c) *c = (cc + (((sincos_table[i+64+1] - cc) * f)>>8))<<2;
}

//compute sine and cosine of an angle, filling in the variables
//either of the pointers can be NULL
//no interpolation
void fix_fastsincos(fix a,fix *s,fix *c)
{
	int i;

	i = (a>>8)&0xff;

	if (s) *s = sincos_table[i] << 2;
	if (c) *c = sincos_table[i+64] << 2;
}

//compute inverse sine
fixang fix_asin(fix v)
{
	fix vv;
	int i,f,aa;

	vv = labs(v);

	if (vv >= f1_0)		//check for out of range
		return 0x4000;

	i = (vv>>8)&0xff;
	f = vv&0xff;

	aa = asin_table[i];
	aa = aa + (((asin_table[i+1] - aa) * f)>>8);

	if (v < 0)
		aa = -aa;

	return aa;
}

//compute inverse cosine
fixang fix_acos(fix v)
{
	fix vv;
	int i,f,aa;

	vv = labs(v);

	if (vv >= f1_0)		//check for out of range
		return 0;

	i = (vv>>8)&0xff;
	f = vv&0xff;

	aa = acos_table[i];
	aa = aa + (((acos_table[i+1] - aa) * f)>>8);

	if (v < 0)
		aa = 0x8000 - aa;

	return aa;
}
