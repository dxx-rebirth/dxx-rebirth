/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

/*
 *
 * C version of fixed point library
 *
 */

#include <cstdint>
#include <stdlib.h>
#include <math.h>
#include <type_traits>

#include "maths.h"

namespace dcx {

namespace {

class fix_sincos_input
{
public:
	const uint8_t m_idx;
	const signed m_mul;
	fix_sincos_input(const fixang a) :
		m_idx{static_cast<uint8_t>(a >> 8)},
		m_mul{static_cast<uint8_t>(a)}
	{
	}
};

}

fix64 fixmul64(fix a, fix b)
{
	return (fix64{a} * fix64{b}) / 65536;
}

fix fixdiv(fix a, fix b)
{
	if (!b)
		return 1;
	return static_cast<fix>((fix64{a} * 65536) / b);
}

fix fixmuldiv(fix a, fix b, fix c)
{
	if (!c)
		return 1;
	return static_cast<fix>((fix64{a} * b) / c);
}

//given cos & sin of an angle, return that angle.
//parms need not be normalized, that is, the ratio of the parms cos/sin must
//equal the ratio of the actual cos & sin for the result angle, but the parms 
//need not be the actual cos & sin.  
//NOTE: this is different from the standard C atan2, since it is left-handed.

fixang fix_atan2(fix cos,fix sin)
{
	//find smaller of two

	const auto dsin{static_cast<double>(sin)};
	const auto dcos{static_cast<double>(cos)};
	const double d{sqrt((dsin * dsin) + (dcos * dcos))};

	if (d==0.0)
		return 0;

	if (labs(sin) < labs(cos)) {				//sin is smaller, use arcsin
		const auto t{fix_asin(static_cast<fix>((dsin / d) * 65536.0))};
		return cos < 0 ? static_cast<fixang>(0x8000 - t) : t;
	}
	else {
		const auto t{fix_acos(static_cast<fix>((dcos / d) * 65536.0))};
		return sin < 0 ? static_cast<fixang>(-t) : t;
	}
}

[[nodiscard]]
static unsigned fixdivquadlongu(quadint n, uint64_t d)
{
	return static_cast<std::underlying_type_t<quadint>>(n) / d;
}

uint32_t quad_sqrt(const quadint iq)
{
	const uint32_t low{static_cast<uint32_t>(iq)};
	const int32_t high{static_cast<int32_t>(static_cast<std::underlying_type_t<quadint>>(iq) >> 32)};
	int i, cnt;
	uint32_t r,old_r,t;

	if (high<0)
		return 0;

	if (high==0 && static_cast<int32_t>(low)>=0)
		return long_sqrt(static_cast<int32_t>(low));

	if ((i = high >> 24)) {
		cnt=12+16;
	}
	else if ((i = high >> 16))
	{
		cnt=8+16;
	}
	else if ((i = high >> 8))
	{
		cnt=4+16;
	} else {
		cnt=0+16; i = high;
	}
	
	r = guess_table[i]<<cnt;

	//quad loop usually executed 4 times

	r = fixdivquadlongu(iq,r)/2 + r/2;
	r = fixdivquadlongu(iq,r)/2 + r/2;
	r = fixdivquadlongu(iq,r)/2 + r/2;

	do {

		old_r = r;
		t = fixdivquadlongu(iq,r);

		if (t==r)	//got it!
			return r;
 
		r = t/2 + r/2;

	} while (!(r==t || r==old_r));

	t = fixdivquadlongu(iq,r);
	const auto tq{fixmulaccum({}, r, t)};
	if (tq != iq)
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
	return static_cast<fix>(long_sqrt(a)) << 8;
}

[[nodiscard]]
static fix fix_sincos(const uint8_t idx0, const signed mul)
{
	const fix t0{sincos_table[idx0]};
	/* `idx1` is `uint8_t` to truncate the value, since sincos_table is
	 * only 256 elements long.
	 */
	const uint8_t idx1 = idx0 + 1;
	const fix t1{sincos_table[idx1]};
	return (t0 + (((t1 - t0) * mul) >> 8)) << 2;
}

[[nodiscard]]
static fix fix_sin(const fix_sincos_input sci)
{
	return fix_sincos(sci.m_idx, sci.m_mul);
}

[[nodiscard]]
static fix fix_cos(const fix_sincos_input sci)
{
	return fix_sincos(static_cast<uint8_t>(sci.m_idx + 64), sci.m_mul);
}

//compute sine and cosine of an angle, filling in the variables
//either of the pointers can be NULL
//with interpolation
fix_sincos_result fix_sincos(const fixang a)
{
	const fix_sincos_input i{a};
	return {fix_sin(i), fix_cos(i)};
}

fix fix_sin(const fixang a)
{
	return {fix_sin(fix_sincos_input{a})};
}

fix fix_cos(const fixang a)
{
	return {fix_cos(fix_sincos_input{a})};
}

//compute sine and cosine of an angle, filling in the variables
//no interpolation
fix fix_fastsin(const fixang a)
{
	const uint8_t i = static_cast<uint8_t>(a >> 8);
	return sincos_table[i] << 2;
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

}
