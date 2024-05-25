/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 * 
 * Routines for point definition, rotation, etc.
 * 
 */


#include "3d.h"
#include "globvars.h"

namespace dcx {

namespace {

clipping_code build_g3_clipping_code_from_viewer_relative_position(const vms_vector &viewer_relative_position)
{
	clipping_code cc{};

	if (viewer_relative_position.x > viewer_relative_position.z)
		cc |= clipping_code::off_right;

	if (viewer_relative_position.y > viewer_relative_position.z)
		cc |= clipping_code::off_top;

	if (viewer_relative_position.x < -viewer_relative_position.z)
		cc |= clipping_code::off_left;

	if (viewer_relative_position.y < -viewer_relative_position.z)
		cc |= clipping_code::off_bot;

	if (viewer_relative_position.z < 0)
		cc |= clipping_code::behind;
	return cc;
}

}

clipping_code g3_code_point(g3s_point &p)
{
	return p.p3_codes = build_g3_clipping_code_from_viewer_relative_position(p.p3_vec);
}

//rotates a point. returns codes.  does not check if already rotated
clipping_code g3_rotate_point(g3s_point &dest,const vms_vector &src)
{
	vm_vec_rotate(dest.p3_vec, vm_vec_sub(src, View_position), View_matrix);
	dest.p3_flags = {};	//no projected
	return g3_code_point(dest);
}

/* Multiply `a` and `b` into a 64-bit result.  Check whether ((a * b) / c) will
 * overflow when stored into a 32-bit signed integer.  If the quotient does not
 * overflow a 32-bit value, then return a std::optional that contains the
 * quotient.  If the quotient does overflow, then return a std::optional that
 * does not contain a value.
 */
std::optional<int32_t> checkmuldiv(const fix a, const fix b, const fix c)
{
	/* product will be negative if and only if the sign bits of the input
	 * values require it.  Storing the result in a 64-bit value ensures that
	 * overflow cannot occur, and so the sign bit cannot be incorrectly set as
	 * a side effect of overflow.
	 */
	const int64_t product{int64_t{a} * int64_t{b}};
	/* absolute_product will be positive, because the only negative number that
	 * remains negative after negation is too large to be produced by the
	 * multiplication of 2 32-bit signed inputs.
	 */
	const auto absolute_product{(product < 0) ? -product : product};
	if ((absolute_product >> 31) >= c)
		/* If this branch is taken, then the division would produce a value
		 * that cannot be correctly represented in `int32_t`.  Return a failure
		 * code, rather than returning an incorrect result.  This case is
		 * tested explicitly, rather than the clearer construct of:

		const auto result = q.q / static_cast<int64_t>(c);
		if (static_cast<int32_t>(result) != result)
			return std::nullopt;

		 * because that would always perform the division, before determining
		 * whether the division is valid.
		 */
		return std::nullopt;
	else {
		/* According to language rules, this is a narrowing conversion.
		 * However, runtime logic above ensured that this path is only reached
		 * if the value is not changed by the narrowing conversion.
		 */
		return static_cast<int32_t>(product / int64_t{c});
	}
}

//projects a point
void g3_project_point(g3s_point &p)
{
#ifndef __powerc
	if ((p.p3_flags & projection_flag::projected) || (p.p3_codes & clipping_code::behind) != clipping_code::None)
		return;

	const auto pz = p.p3_vec.z;
	const auto otx = checkmuldiv(p.p3_vec.x, Canv_w2, pz);
	std::optional<int32_t> oty;
	if (otx && (oty = checkmuldiv(p.p3_vec.y, Canv_h2, pz)))
	{
		p.p3_sx = Canv_w2 + *otx;
		p.p3_sy = Canv_h2 - *oty;
		p.p3_flags |= projection_flag::projected;
	}
	else
		p.p3_flags |= projection_flag::overflow;
#else
	double fz;
	
	if ((p.p3_flags & projection_flag::projected) || (p.p3_codes & clipping_code::behind) != clipping_code::None)
		return;
	
	if ( p.p3_vec.z <= 0 )	{
		p.p3_flags |= projection_flag::overflow;
		return;
	}

	fz = f2fl(p.p3_vec.z);
	p.p3_sx = fl2f(fCanv_w2 + (f2fl(p.p3_vec.x)*fCanv_w2 / fz));
	p.p3_sy = fl2f(fCanv_h2 - (f2fl(p.p3_vec.y)*fCanv_h2 / fz));

	p.p3_flags |= projection_flag::projected;
#endif
}

#if DXX_USE_EDITOR
//from a 2d point, compute the vector through that point
vms_vector g3_point_2_vec(short sx,short sy)
{
	vms_vector v;
	vm_vec_rotate(v, vm_vec_normalized(
			vms_vector{
				.x =  fixmuldiv(fixdiv((sx << 16) - Canv_w2, Canv_w2), Matrix_scale.z, Matrix_scale.x),
				.y = -fixmuldiv(fixdiv((sy << 16) - Canv_h2, Canv_h2), Matrix_scale.z, Matrix_scale.y),
				.z = f1_0
			}
			),
		vm_transposed_matrix(Unscaled_matrix));
	return v;
}
#endif

vms_vector g3_rotate_delta_vec(const vms_vector &src)
{
	return vm_vec_rotate(src, View_matrix);
}

void g3_add_delta_vec(g3s_point &dest,const g3s_point &src,const vms_vector &deltav)
{
	vm_vec_add(dest.p3_vec,src.p3_vec,deltav);
	dest.p3_flags = {};		//not projected
	g3_code_point(dest);
}

//calculate the depth of a point - returns the z coord of the rotated point
fix g3_calc_point_depth(const vms_vector &pnt)
{
	const auto q0{fixmulaccum({}, pnt.x - View_position.x, View_matrix.fvec.x)};
	const auto q1{fixmulaccum(q0, pnt.y - View_position.y, View_matrix.fvec.y)};
	const auto q2{fixmulaccum(q1, pnt.z - View_position.z, View_matrix.fvec.z)};
	return fixquadadjust(q2);
}

}
