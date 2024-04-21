/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */

/*
 *
 * C version of vecmat library
 *
 */

#include <stdlib.h>
#include <stdint.h>
#include <math.h>           // for sqrt

#include "maths.h"
#include "vecmat.h"
#include "dxxerror.h"

namespace dcx {

//#define USE_ISQRT 1

constexpr vms_matrix vmd_identity_matrix{IDENTITY_MATRIX};
constexpr vms_vector vmd_zero_vector{};

namespace {

vms_vector vm_vec_divide(const vms_vector &src, const fix m)
{
	return vms_vector{
		.x = fixdiv({src.x}, {m}),
		.y = fixdiv({src.y}, {m}),
		.z = fixdiv({src.z}, {m}),
	};
}

static void vm_vector_to_matrix_f(vms_matrix &m)
{
	if (m.fvec.x == 0 && m.fvec.z == 0) {		//forward vec is straight up or down
		m.rvec = vms_vector{
			.x = F1_0,
			.y = 0,
			.z = 0
		};
		m.uvec = vms_vector{
			.x = 0,
			.y = 0,
			.z = (m.fvec.y < 0) ? F1_0 : -F1_0
		};
	}
	else { 		//not straight up or down
		m.rvec = vms_vector{
			.x = m.fvec.z,
			.y = 0,
			.z = -m.fvec.x
		};
		vm_vec_normalize(m.rvec);
		m.uvec = vm_vec_cross(m.fvec, m.rvec);
	}
}

}

//adds two vectors, fills in dest, returns ptr to dest
//ok for dest to equal either source, but should use vm_vec_add2() if so
vms_vector &vm_vec_add(vms_vector &dest,const vms_vector &src0,const vms_vector &src1)
{
	dest = vms_vector{
		.x = src0.x + src1.x,
		.y = src0.y + src1.y,
		.z = src0.z + src1.z,
	};
	return dest;
}


//subs two vectors, fills in dest, returns ptr to dest
//ok for dest to equal either source, but should use vm_vec_sub2() if so
vms_vector &_vm_vec_sub(vms_vector &dest,const vms_vector &src0,const vms_vector &src1)
{
	dest = vms_vector{
		.x = src0.x - src1.x,
		.y = src0.y - src1.y,
		.z = src0.z - src1.z,
	};
	return dest;
}

//adds one vector to another. returns ptr to dest
//dest can equal source
void vm_vec_add2(vms_vector &dest,const vms_vector &src)
{
	dest.x += src.x;
	dest.y += src.y;
	dest.z += src.z;
}

//subs one vector from another, returns ptr to dest
//dest can equal source
void vm_vec_sub2(vms_vector &dest,const vms_vector &src)
{
	dest.x -= src.x;
	dest.y -= src.y;
	dest.z -= src.z;
}

static inline fix avg_fix(fix64 a, fix64 b)
{
	return (a + b) / 2;
}

//averages two vectors. returns ptr to dest
//dest can equal either source
vms_vector vm_vec_avg(const vms_vector &src0, const vms_vector &src1)
{
	return vms_vector{
		.x = avg_fix({src0.x}, {src1.x}),
		.y = avg_fix({src0.y}, {src1.y}),
		.z = avg_fix({src0.z}, {src1.z}),
	};
}

//scales a vector in place.  returns ptr to vector
vms_vector &vm_vec_scale(vms_vector &dest, const fix s)
{
	return dest = vm_vec_copy_scale(dest, {s});
}

//scales and copies a vector.  returns scaled result
vms_vector vm_vec_copy_scale(const vms_vector src, const fix s)
{
	return vms_vector{
		.x = fixmul({src.x}, {s}),
		.y = fixmul({src.y}, {s}),
		.z = fixmul({src.z}, {s}),
	};
}

//scales a vector, adds it to another, and returns it
//dest = src1 + k * src2
vms_vector vm_vec_scale_add(const vms_vector &src1, const vms_vector &src2, const fix k)
{
	return vms_vector{
		.x = src1.x + fixmul({src2.x}, {k}),
		.y = src1.y + fixmul({src2.y}, {k}),
		.z = src1.z + fixmul({src2.z}, {k}),
	};
}

//scales a vector and adds it to another
//dest += k * src
void vm_vec_scale_add2(vms_vector &dest,const vms_vector &src,fix k)
{
	dest.x += fixmul({src.x}, {k});
	dest.y += fixmul({src.y}, {k});
	dest.z += fixmul({src.z}, {k});
}

//scales a vector in place, taking n/d for scale.  returns ptr to vector
//dest *= n/d
void vm_vec_scale2(vms_vector &dest,fix n,fix d)
{
#if 0 // DPH: Kludge: this was overflowing a lot, so I made it use the FPU.
	float nd;
	nd = f2fl(n) / f2fl(d);
	dest.x = fl2f( f2fl(dest.x) * nd);
	dest.y = fl2f( f2fl(dest.y) * nd);
	dest.z = fl2f( f2fl(dest.z) * nd);
#endif
	dest = vms_vector{
		.x = fixmuldiv({dest.x}, {n}, {d}),
		.y = fixmuldiv({dest.y}, {n}, {d}),
		.z = fixmuldiv({dest.z}, {n}, {d}),
	};
}

[[nodiscard]]
static fix vm_vec_dot3(fix x,fix y,fix z,const vms_vector &v)
{
	const int64_t x0{x};
	const int64_t x1{v.x};
	const int64_t y0{y};
	const int64_t y1{v.y};
	const int64_t z0{z};
	const int64_t z1{v.z};
	const int64_t p{(x0 * x1) + (y0 * y1) + (z0 * z1)};
	/* Convert back to fix and return. */
	return p >> 16;
}

fix vm_vec_dot(const vms_vector &v0,const vms_vector &v1)
{
	return vm_vec_dot3({v0.x}, {v0.y}, {v0.z}, v1);
}

vm_magnitude_squared vm_vec_mag2(const vms_vector &v)
{
	const int64_t x{v.x};
	const int64_t y{v.y};
	const int64_t z{v.z};
	return vm_magnitude_squared{static_cast<uint64_t>((x * x) + (y * y) + (z * z))};
}

vm_magnitude vm_vec_mag(const vms_vector &v)
{
	return vm_magnitude{quad_sqrt(quadint{static_cast<int64_t>(vm_vec_mag2(v))})};
}

//computes the distance between two points. (does sub and mag)
vm_distance vm_vec_dist(const vms_vector &v0,const vms_vector &v1)
{
	return vm_vec_mag(vm_vec_sub(v0,v1));
}

vm_distance_squared vm_vec_dist2(const vms_vector &v0,const vms_vector &v1)
{
	return build_vm_distance_squared(vm_vec_mag2(vm_vec_sub(v0,v1)));
}

//computes an approximation of the magnitude of the vector
//uses dist = largest + next_largest*3/8 + smallest*3/16
vm_magnitude vm_vec_mag_quick(const vms_vector &v)
{
	fix a{std::abs(v.x)};
	fix b{std::abs(v.y)};

	if (a < b) {
		std::swap(a, b);
	}

	fix c{std::abs(v.z)};
	if (b < c) {
		std::swap(b, c);
		if (a < b) {
			std::swap(a, b);
		}
	}
	const fix bc{(b >> 2) + (c >> 3)};
	return vm_magnitude{static_cast<uint32_t>(a + bc + (bc>>1))};
}


//computes an approximation of the distance between two points.
//uses dist = largest + next_largest*3/8 + smallest*3/16
vm_distance vm_vec_dist_quick(const vms_vector &v0,const vms_vector &v1)
{
	return vm_vec_mag_quick(vm_vec_sub(v0,v1));
}

//normalize a vector. returns mag of source vec
vm_magnitude vm_vec_copy_normalize(vms_vector &dest,const vms_vector &src)
{
	const auto m{vm_vec_mag(src)};
	if (likely(m)) {
		dest = vm_vec_divide(src, m);
	}
	return m;
}

//normalize a vector. returns mag of source vec
vm_magnitude vm_vec_normalize(vms_vector &v)
{
	return vm_vec_copy_normalize(v,v);
}

//normalize a vector. returns mag of source vec. uses approx mag
vm_magnitude vm_vec_copy_normalize_quick(vms_vector &dest,const vms_vector &src)
{
	const auto m{vm_vec_mag_quick(src)};
	if (likely(m)) {
		dest = vm_vec_divide(src, m);
	}
	return m;
}

//normalize a vector. returns 1/mag of source vec. uses approx 1/mag
vm_magnitude vm_vec_normalize_quick(vms_vector &v)
{
	return vm_vec_copy_normalize_quick(v,v);
}

//return the normalized direction vector between two points
//dest = normalized(end - start).  Returns 1/mag of direction vector
//NOTE: the order of the parameters matches the vector subtraction
vm_magnitude vm_vec_normalized_dir_quick(vms_vector &dest,const vms_vector &end,const vms_vector &start)
{
	return vm_vec_normalize_quick(vm_vec_sub(dest,end,start));
}

//return the normalized direction vector between two points
//dest = normalized(end - start).  Returns mag of direction vector
//NOTE: the order of the parameters matches the vector subtraction
vm_magnitude vm_vec_normalized_dir(vms_vector &dest,const vms_vector &end,const vms_vector &start)
{
	return vm_vec_normalize(vm_vec_sub(dest,end,start));
}

//computes surface normal from three points. result is normalized
vms_vector vm_vec_normal(const vms_vector &p0, const vms_vector &p1, const vms_vector &p2)
{
	return vm_vec_normalized(vm_vec_perp(p0, p1, p2));
}

namespace {

//make sure a vector is reasonably sized to go into a cross product
[[nodiscard]]
static vms_vector check_vec(vms_vector v)
{
	if (unlikely(v == vms_vector{}))
		return v;
	int cnt = 0;

	fix check = labs(v.x) | labs(v.y) | labs(v.z);
	if (check & 0xfffc0000) {		//too big

		while (check & 0xfff00000) {
			cnt += 4;
			check >>= 4;
		}

		while (check & 0xfffc0000) {
			cnt += 2;
			check >>= 2;
		}

		v.x >>= cnt;
		v.y >>= cnt;
		v.z >>= cnt;
	}
	else												//maybe too small
		if ((check & 0xffff8000) == 0) {		//yep, too small

			while ((check & 0xfffff000) == 0) {
				cnt += 4;
				check <<= 4;
			}

			while ((check & 0xffff8000) == 0) {
				cnt += 2;
				check <<= 2;
			}

			v.x >>= cnt;
			v.y >>= cnt;
			v.z >>= cnt;
		}
	return v;
}

}

//computes cross product of two vectors. 
//Note: this magnitude of the resultant vector is the
//product of the magnitudes of the two source vectors.  This means it is
//quite easy for this routine to overflow and underflow.  Be careful that
//your inputs are ok.
vms_vector vm_vec_cross(const vms_vector &src0, const vms_vector &src1)
{
	const auto qx0{fixmulaccum({}, src0.y, src1.z)};
	const auto qx1{fixmulaccum(qx0, -src0.z, src1.y)};

	const auto qy0{fixmulaccum({}, src0.z, src1.x)};
	const auto qy1{fixmulaccum(qy0, -src0.x, src1.z)};

	const auto qz0{fixmulaccum({}, src0.x, src1.y)};
	const auto qz1{fixmulaccum(qz0, -src0.y, src1.x)};
	return vms_vector{
		.x = fixquadadjust(qx1),
		.y = fixquadadjust(qy1),
		.z = fixquadadjust(qz1),
	};
}

//computes non-normalized surface normal from three points. 
vms_vector vm_vec_perp(const vms_vector &p0, const vms_vector &p1, const vms_vector &p2)
{
	auto t0{check_vec(vm_vec_sub(p1, p0))};
	auto t1{check_vec(vm_vec_sub(p2, p1))};
	return vm_vec_cross(t0, t1);
}

//computes the delta angle between two vectors. 
//vectors need not be normalized. if they are, call vm_vec_delta_ang_norm()
//the forward vector (third parameter) can be NULL, in which case the absolute
//value of the angle in returned.  Otherwise the angle around that vector is
//returned.
fixang vm_vec_delta_ang(const vms_vector &v0,const vms_vector &v1,const vms_vector &fvec)
{
	vms_vector t0,t1;

	if (!vm_vec_copy_normalize(t0,v0) || !vm_vec_copy_normalize(t1,v1))
		return 0;

	return vm_vec_delta_ang_norm(t0,t1,fvec);
}

//computes the delta angle between two normalized vectors. 
fixang vm_vec_delta_ang_norm(const vms_vector &v0,const vms_vector &v1,const vms_vector &fvec)
{
	fixang a{fix_acos(vm_vec_dot(v0, v1))};
	if (vm_vec_dot(vm_vec_cross(v0, v1), fvec) < 0)
			a = -a;
	return a;
}

static void sincos_2_matrix(vms_matrix &m, const fixang bank, const fix_sincos_result p, const fix_sincos_result h)
{
#define DXX_S2M_DECL(V)	\
	const auto &sin##V = V.sin;	\
	const auto &cos##V = V.cos
	DXX_S2M_DECL(p);
	DXX_S2M_DECL(h);
	m.fvec.y = -sinp;								//m6
	m.fvec.x = fixmul(sinh,cosp);				//m3
	m.fvec.z = fixmul(cosh,cosp);				//m9
	const auto &&b = fix_sincos(bank);
	DXX_S2M_DECL(b);
#undef DXX_S2M_DECL
	m.rvec.y = fixmul(sinb,cosp);				//m4
	m.uvec.y = fixmul(cosb,cosp);				//m5

	const auto cbch{fixmul(cosb, cosh)};
	const auto sbsh{fixmul(sinb, sinh)};
	m.rvec.x = cbch + fixmul(sinp,sbsh);		//m1
	m.uvec.z = sbsh + fixmul(sinp,cbch);		//m8

	const auto sbch{fixmul(sinb, cosh)};
	const auto cbsh{fixmul(cosb, sinh)};
	m.uvec.x = fixmul(sinp,cbsh) - sbch;		//m2
	m.rvec.z = fixmul(sinp,sbch) - cbsh;		//m7
}

//computes a matrix from a set of three angles.  returns ptr to matrix
void vm_angles_2_matrix(vms_matrix &m,const vms_angvec &a)
{
	const auto al = a;
	sincos_2_matrix(m, al.b, fix_sincos(al.p), fix_sincos(al.h));
}

#if DXX_USE_EDITOR
//computes a matrix from a forward vector and an angle
void vm_vec_ang_2_matrix(vms_matrix &m,const vms_vector &v,fixang a)
{
	const fix sinp{-v.y};
	const fix cosp{fix_sqrt(F1_0 - fixmul(sinp, sinp))};
	const fix sinh{fixdiv(v.x, cosp)};
	const fix cosh{fixdiv(v.z, cosp)};
	sincos_2_matrix(m, a, {sinp, cosp}, {sinh, cosh});
}
#endif

//computes a matrix from one or more vectors. The forward vector is required,
//with the other two being optional.  If both up & right vectors are passed,
//the up vector is used.  If only the forward vector is passed, a bank of
//zero is assumed
//returns ptr to matrix
void vm_vector_to_matrix(vms_matrix &m, const vms_vector &fvec)
{
	if (!vm_vec_copy_normalize(m.fvec, fvec))
	{
		Int3();		//forward vec should not be zero-length
		return;
	}
	vm_vector_to_matrix_f(m);
}

void vm_vector_to_matrix_r(vms_matrix &m, const vms_vector &fvec, const vms_vector &rvec)
{
	if (!vm_vec_copy_normalize(m.fvec, fvec))
	{
		Int3();		//forward vec should not be zero-length
		return;
	}
	if (!vm_vec_copy_normalize(m.rvec, rvec) ||
		//normalize new perpendicular vector
		!vm_vec_normalize(m.uvec = vm_vec_cross(m.fvec, m.rvec)))
	{
		vm_vector_to_matrix_f(m);
		return;
	}
	//now recompute right vector, in case it wasn't entirely perpendicular
	m.rvec = vm_vec_cross(m.uvec, m.fvec);
}

void vm_vector_to_matrix_u(vms_matrix &m, const vms_vector &fvec, const vms_vector &uvec)
{
	if (!vm_vec_copy_normalize(m.fvec, fvec))
	{
		Int3();		//forward vec should not be zero-length
		return;
	}
	if (!vm_vec_copy_normalize(m.uvec, uvec) ||
		//normalize new perpendicular vector
		!vm_vec_normalize(m.rvec = vm_vec_cross(m.uvec, m.fvec)))
	{
		vm_vector_to_matrix_f(m);
		return;
	}
	//now recompute up vector, in case it wasn't entirely perpendicular
	m.uvec = vm_vec_cross(m.fvec, m.rvec);
}


//rotates a vector through a matrix. returns ptr to dest vector
//dest CANNOT equal source
void vm_vec_rotate(vms_vector &dest,const vms_vector &src,const vms_matrix &m)
{
	dest.x = vm_vec_dot(src,m.rvec);
	dest.y = vm_vec_dot(src,m.uvec);
	dest.z = vm_vec_dot(src,m.fvec);
}

//mulitply 2 matrices, fill in dest.  returns ptr to dest
//dest CANNOT equal either source
void _vm_matrix_x_matrix(vms_matrix &dest,const vms_matrix &src0,const vms_matrix &src1)
{
	dest.rvec.x = vm_vec_dot3(src0.rvec.x,src0.uvec.x,src0.fvec.x, src1.rvec);
	dest.uvec.x = vm_vec_dot3(src0.rvec.x,src0.uvec.x,src0.fvec.x, src1.uvec);
	dest.fvec.x = vm_vec_dot3(src0.rvec.x,src0.uvec.x,src0.fvec.x, src1.fvec);

	dest.rvec.y = vm_vec_dot3(src0.rvec.y,src0.uvec.y,src0.fvec.y, src1.rvec);
	dest.uvec.y = vm_vec_dot3(src0.rvec.y,src0.uvec.y,src0.fvec.y, src1.uvec);
	dest.fvec.y = vm_vec_dot3(src0.rvec.y,src0.uvec.y,src0.fvec.y, src1.fvec);

	dest.rvec.z = vm_vec_dot3(src0.rvec.z,src0.uvec.z,src0.fvec.z, src1.rvec);
	dest.uvec.z = vm_vec_dot3(src0.rvec.z,src0.uvec.z,src0.fvec.z, src1.uvec);
	dest.fvec.z = vm_vec_dot3(src0.rvec.z,src0.uvec.z,src0.fvec.z, src1.fvec);
}

//extract angles from a matrix 
vms_angvec vm_extract_angles_matrix(const vms_matrix &m)
{
	const fixang ah{
		(m.fvec.x == 0 && m.fvec.z == 0)
			//zero head
			? fixang{0}
			: fix_atan2(m.fvec.z, m.fvec.x)
	};

	const auto &&[sinh, cosh] = fix_sincos(ah);
	const auto cosp{
		(abs(sinh) > abs(cosh))
			//sine is larger, so use it
			? fixdiv(m.fvec.x, sinh)
			//cosine is larger, so use it
			: fixdiv(m.fvec.z, cosh)
	};
	return vms_angvec{
		.p = (
			(cosp == 0 && m.fvec.y == 0)
			? fixang{0}
			: fix_atan2(cosp, -m.fvec.y)
			),
		.b = (
			(cosp == 0)	//the cosine of pitch is zero.  we're pitched straight up. say no bank
			? fixang{0}
			: ({
				const auto sinb{fixdiv(m.rvec.y, cosp)};
				const auto cosb{fixdiv(m.uvec.y, cosp)};
				(sinb == 0 && cosb == 0)
				? fixang{0}
				: fix_atan2(cosb, sinb);
			})
		),
		.h = ah
	};
}


//extract heading and pitch from a vector, assuming bank==0
static vms_angvec vm_extract_angles_vector_normalized(const vms_vector &v)
{
	return vms_angvec{
		.p = fix_asin(-v.y),
		.b = 0,		//always zero bank
		.h = (v.x == 0 && v.z == 0)
			? fixang{0}
			: fix_atan2(v.z, v.x)
	};
}

//extract heading and pitch from a vector, assuming bank==0
vms_angvec vm_extract_angles_vector(const vms_vector &v)
{
	vms_vector t;
	if (vm_vec_copy_normalize(t,v))
		return vm_extract_angles_vector_normalized(t);
	else
		return {};
}

//compute the distance from a point to a plane.  takes the normalized normal
//of the plane (ebx), a point on the plane (edi), and the point to check (esi).
//returns distance in eax
//distance is signed, so negative dist is on the back of the plane
fix vm_dist_to_plane(const vms_vector &checkp,const vms_vector &norm,const vms_vector &planep)
{
	return vm_vec_dot(vm_vec_sub(checkp,planep),norm);
}

// convert vms_matrix to vms_quaternion
vms_quaternion vms_quaternion_from_matrix(const vms_matrix &m)
{
	vms_quaternion rq{};
	const auto rx{m.rvec.x};
	const auto ry{m.rvec.y};
	const auto rz{m.rvec.z};
	const auto ux{m.uvec.x};
	const auto uy{m.uvec.y};
	const auto uz{m.uvec.z};
	const auto fx{m.fvec.x};
	const auto fy{m.fvec.y};
	const auto fz{m.fvec.z};
	const fix tr{rx + uy + fz};
	fix qw, qx, qy, qz;
	if (tr > 0) {
		const fix s{fixmul(fix_sqrt(tr + fl2f(1.0)), fl2f(2.0))};
		qw = fixmul(fl2f(0.25), {s});
		qx = fixdiv(fy - uz, {s});
		qy = fixdiv(rz - fx, {s});
		qz = fixdiv(ux - ry, {s});
	} else if ((rx > uy) & (rx > fz)) {
		const fix s{fixmul(fix_sqrt(fl2f(1.0) + rx - uy - fz), fl2f(2.0))};
		qw = fixdiv(fy - uz, {s});
		qx = fixmul(fl2f(0.25), {s});
		qy = fixdiv(ry + ux, {s});
		qz = fixdiv(rz + fx, {s});
	} else if (uy > fz) { 
		const fix s{fixmul(fix_sqrt(fl2f(1.0) + uy - rx - fz), fl2f(2.0))};
		qw = fixdiv(rz - fx, {s});
		qx = fixdiv(ry + ux, {s});
		qy = fixmul(fl2f(0.25), {s});
		qz = fixdiv(uz + fy, {s});
	} else { 
		const fix s{fixmul(fix_sqrt(fl2f(1.0) + fz - rx - uy), fl2f(2.0))};
		qw = fixdiv(ux - ry, {s});
		qx = fixdiv(rz + fx, {s});
		qy = fixdiv(uz + fy, {s});
		qz = fixmul(fl2f(0.25), {s});
	}
	rq.w = qw / 2;
	rq.x = qx / 2;
	rq.y = qy / 2;
	rq.z = qz / 2;
	return rq;
}

// convert vms_quaternion to vms_matrix
void vms_matrix_from_quaternion(vms_matrix &m, const vms_quaternion &q)
{
	const fix qw2{q.w * 2};
	const fix qx2{q.x * 2};
	const fix qy2{q.y * 2};
	const fix qz2{q.z * 2};
	const fix sqw{fixmul({qw2}, {qw2})};
	const fix sqx{fixmul({qx2}, {qx2})};
	const fix sqy{fixmul({qy2}, {qy2})};
	const fix sqz{fixmul({qz2}, {qz2})};
	const fix invs{fixdiv({fl2f(1.0)}, {sqw + sqx + sqy + sqz})};

	m.rvec.x = fixmul(sqx - sqy - sqz + sqw, {invs});
	m.uvec.y = fixmul(-sqx + sqy - sqz + sqw, {invs});
	m.fvec.z = fixmul(-sqx - sqy + sqz + sqw, {invs});
	
	const fix qxy{fixmul({qx2}, {qy2})};
	const fix qzw{fixmul({qz2}, {qw2})};
	m.uvec.x = fixmul(fixmul(fl2f(2.0), {qxy + qzw}), {invs});
	m.rvec.y = fixmul(fixmul(fl2f(2.0), {qxy - qzw}), {invs});
	
	const fix qxz{fixmul({qx2}, {qz2})};
	const fix qyw{fixmul({qy2}, {qw2})};
	m.fvec.x = fixmul(fixmul(fl2f(2.0), {qxz - qyw}), {invs});
	m.rvec.z = fixmul(fixmul(fl2f(2.0), {qxz + qyw}), {invs});
	
	const fix qyz{fixmul({qy2}, {qz2})};
	const fix qxw{fixmul({qx2}, {qw2})};
	m.fvec.y = fixmul(fixmul(fl2f(2.0), {qyz + qxw}), {invs});
	m.uvec.z = fixmul(fixmul(fl2f(2.0), {qyz - qxw}), {invs});
}

}
