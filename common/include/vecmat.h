/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license below.
 * See COPYING.txt for license details.

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
 * Header file for vector/matrix library
 *
 */

#ifndef _VECMAT_H
#define _VECMAT_H

#include "maths.h"

#ifdef __cplusplus
#include "dxxsconf.h"
#include <algorithm>

//The basic fixed-point vector.  Access elements by name or position
struct vms_vector
{
	fix x, y, z;
};

#define DEFINE_SERIAL_VMS_VECTOR_TO_MESSAGE()	\
	DEFINE_SERIAL_UDT_TO_MESSAGE(vms_vector, v, (v.x, v.y, v.z));	\
	ASSERT_SERIAL_UDT_MESSAGE_SIZE(vms_vector, 12)

//Angle vector.  Used to store orientations
struct vms_angvec
{
	fixang p, b, h;
};


//A 3x3 rotation matrix.  Sorry about the numbering starting with one.
//Ordering is across then down, so <m1,m2,m3> is the first row
struct vms_matrix
{
	vms_vector rvec, uvec, fvec;
};

// Quaternion structure
struct vms_quaternion
{
    signed short w, x, y, z;
};


//Macros/functions to fill in fields of structures

//macro to set a vector to zero.  we could do this with an in-line assembly
//macro, but it's probably better to let the compiler optimize it.
//Note: NO RETURN VALUE
static inline void vm_vec_zero(vms_vector &v)
{
	v = {};
}

//macro set set a matrix to the identity. Note: NO RETURN VALUE

// DPH (18/9/98): Begin mod to fix linefeed problem under linux. Uses an
// inline function instead of a multi-line macro to fix CR/LF problems.

// DPH (19/8/98): End changes.

vms_vector * vm_vec_make (vms_vector * v, fix x, fix y, fix z);

//Global constants

extern const vms_matrix vmd_identity_matrix;


//Here's a handy constant

#define ZERO_VECTOR {0,0,0}
#define IDENTITY_MATRIX { {f1_0,0,0}, {0,f1_0,0}, {0,0,f1_0} }

const vms_vector vmd_zero_vector ZERO_VECTOR;

//negate a vector
static inline void vm_vec_negate(vms_vector &v)
{
	v.x = -v.x;
	v.y = -v.y;
	v.z = -v.z;
}

//Functions in library

//adds two vectors, fills in dest, returns ptr to dest
//ok for dest to equal either source, but should use vm_vec_add2() if so
vms_vector &vm_vec_add (vms_vector &dest, const vms_vector &src0, const vms_vector &src1);


//subs two vectors, fills in dest, returns ptr to dest
//ok for dest to equal either source, but should use vm_vec_sub2() if so
vms_vector &vm_vec_sub (vms_vector &dest, const vms_vector &src0, const vms_vector &src1);


//adds one vector to another. returns ptr to dest
//dest can equal source
vms_vector &vm_vec_add2 (vms_vector &dest, const vms_vector &src);


//subs one vector from another, returns ptr to dest
//dest can equal source
vms_vector &vm_vec_sub2 (vms_vector &dest, const vms_vector &src);

//averages two vectors. returns ptr to dest
//dest can equal either source
vms_vector &vm_vec_avg (vms_vector &dest, const vms_vector &src0, const vms_vector &src1);

//scales a vector in place.  returns ptr to vector
vms_vector &vm_vec_scale (vms_vector &dest, fix s);


//scales and copies a vector.  returns ptr to dest
vms_vector &vm_vec_copy_scale (vms_vector &dest, const vms_vector &src, fix s);


//scales a vector, adds it to another, and stores in a 3rd vector
//dest = src1 + k * src2
vms_vector &vm_vec_scale_add (vms_vector &dest, const vms_vector &src1, const vms_vector &src2, fix k);


//scales a vector and adds it to another
//dest += k * src
vms_vector &vm_vec_scale_add2 (vms_vector &dest, const vms_vector &src, fix k);


//scales a vector in place, taking n/d for scale.  returns ptr to vector
//dest *= n/d
vms_vector &vm_vec_scale2 (vms_vector &dest, fix n, fix d);


//returns magnitude of a vector
fix vm_vec_mag (const vms_vector &v) __attribute_warn_unused_result;


//computes the distance between two points. (does sub and mag)
fix vm_vec_dist (const vms_vector &v0, const vms_vector &v1) __attribute_warn_unused_result;


//computes an approximation of the magnitude of the vector
//uses dist = largest + next_largest*3/8 + smallest*3/16
fix vm_vec_mag_quick (const vms_vector &v) __attribute_warn_unused_result;


//computes an approximation of the distance between two points.
//uses dist = largest + next_largest*3/8 + smallest*3/16
fix vm_vec_dist_quick (const vms_vector &v0, const vms_vector &v1) __attribute_warn_unused_result;

//normalize a vector. returns mag of source vec
fix vm_vec_copy_normalize (vms_vector &dest, const vms_vector &src);

fix vm_vec_normalize (vms_vector &v);


//normalize a vector. returns mag of source vec. uses approx mag
fix vm_vec_copy_normalize_quick (vms_vector &dest, const vms_vector &src);

fix vm_vec_normalize_quick (vms_vector &v);


//return the normalized direction vector between two points
//dest = normalized(end - start).  Returns mag of direction vector
//NOTE: the order of the parameters matches the vector subtraction
fix vm_vec_normalized_dir (vms_vector &dest, const vms_vector &end, const vms_vector &start);

fix vm_vec_normalized_dir_quick (vms_vector &dest, const vms_vector &end, const vms_vector &start);


////returns dot product of two vectors
fix vm_vec_dot (const vms_vector &v0, const vms_vector &v1) __attribute_warn_unused_result;

//computes cross product of two vectors. returns ptr to dest
//dest CANNOT equal either source
vms_vector &vm_vec_cross (vms_vector &dest, const vms_vector &src0, const vms_vector &src1);

//computes surface normal from three points. result is normalized
//returns ptr to dest
//dest CANNOT equal either source
vms_vector &vm_vec_normal (vms_vector &dest, const vms_vector &p0, const vms_vector &p1, const vms_vector &p2);

//computes non-normalized surface normal from three points.
//returns ptr to dest
//dest CANNOT equal either source
vms_vector &vm_vec_perp (vms_vector &dest, const vms_vector &p0, const vms_vector &p1, const vms_vector &p2);


//computes the delta angle between two vectors.
//vectors need not be normalized. if they are, call vm_vec_delta_ang_norm()
//the forward vector (third parameter) can be NULL, in which case the absolute
//value of the angle in returned.  Otherwise the angle around that vector is
//returned.
fixang vm_vec_delta_ang (const vms_vector &v0, const vms_vector &v1, const vms_vector &fvec) __attribute_warn_unused_result;

//computes the delta angle between two normalized vectors.
fixang vm_vec_delta_ang_norm (const vms_vector &v0, const vms_vector &v1, const vms_vector &fvec) __attribute_warn_unused_result;


//computes a matrix from a set of three angles.  returns ptr to matrix
vms_matrix &vm_angles_2_matrix (vms_matrix &m, const vms_angvec &a);


//computes a matrix from a forward vector and an angle
void vm_vec_ang_2_matrix (vms_matrix &m, const vms_vector &v, fixang a);


//computes a matrix from one or more vectors. The forward vector is required,
//with the other two being optional.  If both up & right vectors are passed,
//the up vector is used.  If only the forward vector is passed, a bank of
//zero is assumed
//returns ptr to matrix
vms_matrix * vm_vector_2_matrix (vms_matrix * m, vms_vector * fvec, vms_vector * uvec, vms_vector * rvec);

//rotates a vector through a matrix. returns ptr to dest vector
//dest CANNOT equal either source
vms_vector * vm_vec_rotate (vms_vector * dest, const vms_vector * src, const vms_matrix * m);


//transpose a matrix in place. returns ptr to matrix
static inline void vm_transpose_matrix(vms_matrix &m)
{
	using std::swap;
	swap(m.uvec.x, m.rvec.y);
	swap(m.fvec.x, m.rvec.z);
	swap(m.fvec.y, m.uvec.z);
}

static inline vms_matrix vm_transposed_matrix(vms_matrix m) __attribute_warn_unused_result;
static inline vms_matrix vm_transposed_matrix(vms_matrix m)
{
	vm_transpose_matrix(m);
	return m;
}

//mulitply 2 matrices, fill in dest.  returns ptr to dest
void vm_matrix_x_matrix (vms_matrix * dest, const vms_matrix * src0, const vms_matrix * src1);

static inline vms_matrix vm_matrix_x_matrix(const vms_matrix &src0, const vms_matrix &src1)
{
	vms_matrix dest;
	vm_matrix_x_matrix(&dest, &src0, &src1);
	return dest;
}

//extract angles from a matrix
vms_angvec * vm_extract_angles_matrix (vms_angvec * a, const vms_matrix * m);


//extract heading and pitch from a vector, assuming bank==0
vms_angvec * vm_extract_angles_vector (vms_angvec * a, const vms_vector * v);


//compute the distance from a point to a plane.  takes the normalized normal
//of the plane (ebx), a point on the plane (edi), and the point to check (esi).
//returns distance in eax
//distance is signed, so negative dist is on the back of the plane
fix vm_dist_to_plane (const vms_vector * checkp, const vms_vector * norm, const vms_vector * planep);


//fills in fields of an angle vector
static inline void vm_angvec_make(vms_angvec *v, fixang p, fixang b, fixang h)
{
	v->p = p;
	v->b = b;
	v->h = h;
}

// convert from quaternion to vector matrix and back
void vms_quaternion_from_matrix(vms_quaternion * q, const vms_matrix * m);
void vms_matrix_from_quaternion(vms_matrix * m, const vms_quaternion * q);

#endif

#endif  /* !_VECMAT_H */
