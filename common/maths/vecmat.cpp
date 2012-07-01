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
 * C version of vecmat library
 *
 */

#include <stdlib.h>
#include <stdint.h>
#include <math.h>           // for sqrt

#include "maths.h"
#include "vecmat.h"
#include "dxxerror.h"

//#define USE_ISQRT 1

const vms_vector vmd_zero_vector = ZERO_VECTOR;
const vms_matrix vmd_identity_matrix = IDENTITY_MATRIX;

//adds two vectors, fills in dest, returns ptr to dest
//ok for dest to equal either source, but should use vm_vec_add2() if so
vms_vector *vm_vec_add(vms_vector *dest,const vms_vector *src0,const vms_vector *src1)
{
	dest->x = src0->x + src1->x;
	dest->y = src0->y + src1->y;
	dest->z = src0->z + src1->z;

	return dest;
}


//subs two vectors, fills in dest, returns ptr to dest
//ok for dest to equal either source, but should use vm_vec_sub2() if so
vms_vector *vm_vec_sub(vms_vector *dest,const vms_vector *src0,const vms_vector *src1)
{
	dest->x = src0->x - src1->x;
	dest->y = src0->y - src1->y;
	dest->z = src0->z - src1->z;

	return dest;
}

//adds one vector to another. returns ptr to dest
//dest can equal source
vms_vector *vm_vec_add2(vms_vector *dest,const vms_vector *src)
{
	dest->x += src->x;
	dest->y += src->y;
	dest->z += src->z;

	return dest;
}

//subs one vector from another, returns ptr to dest
//dest can equal source
vms_vector *vm_vec_sub2(vms_vector *dest,const vms_vector *src)
{
	dest->x -= src->x;
	dest->y -= src->y;
	dest->z -= src->z;

	return dest;
}

//averages two vectors. returns ptr to dest
//dest can equal either source
vms_vector *vm_vec_avg(vms_vector *dest,const vms_vector *src0,const vms_vector *src1)
{
	dest->x = (src0->x + src1->x)/2;
	dest->y = (src0->y + src1->y)/2;
	dest->z = (src0->z + src1->z)/2;

	return dest;
}


//averages four vectors. returns ptr to dest
//dest can equal any source
vms_vector *vm_vec_avg4(vms_vector *dest,const vms_vector *src0,const vms_vector *src1,const vms_vector *src2,const vms_vector *src3)
{
	dest->x = (src0->x + src1->x + src2->x + src3->x)/4;
	dest->y = (src0->y + src1->y + src2->y + src3->y)/4;
	dest->z = (src0->z + src1->z + src2->z + src3->z)/4;

	return dest;
}


//scales a vector in place.  returns ptr to vector
vms_vector *vm_vec_scale(vms_vector *dest,fix s)
{
	dest->x = fixmul(dest->x,s);
	dest->y = fixmul(dest->y,s);
	dest->z = fixmul(dest->z,s);

	return dest;
}

//scales and copies a vector.  returns ptr to dest
vms_vector *vm_vec_copy_scale(vms_vector *dest,const vms_vector *src,fix s)
{
	dest->x = fixmul(src->x,s);
	dest->y = fixmul(src->y,s);
	dest->z = fixmul(src->z,s);

	return dest;
}

//scales a vector, adds it to another, and stores in a 3rd vector
//dest = src1 + k * src2
vms_vector *vm_vec_scale_add(vms_vector *dest,const vms_vector *src1,const vms_vector *src2,fix k)
{
	dest->x = src1->x + fixmul(src2->x,k);
	dest->y = src1->y + fixmul(src2->y,k);
	dest->z = src1->z + fixmul(src2->z,k);

	return dest;
}

//scales a vector and adds it to another
//dest += k * src
vms_vector *vm_vec_scale_add2(vms_vector *dest,const vms_vector *src,fix k)
{
	dest->x += fixmul(src->x,k);
	dest->y += fixmul(src->y,k);
	dest->z += fixmul(src->z,k);

	return dest;
}

//scales a vector in place, taking n/d for scale.  returns ptr to vector
//dest *= n/d
vms_vector *vm_vec_scale2(vms_vector *dest,fix n,fix d)
{
#if 1 // DPH: Kludge: this was overflowing a lot, so I made it use the FPU.
	float nd;
	nd = f2fl(n) / f2fl(d);
	dest->x = fl2f( f2fl(dest->x) * nd);
	dest->y = fl2f( f2fl(dest->y) * nd);
	dest->z = fl2f( f2fl(dest->z) * nd);
#else
	dest->x = fixmuldiv(dest->x,n,d);
	dest->y = fixmuldiv(dest->y,n,d);
	dest->z = fixmuldiv(dest->z,n,d);
#endif

	return dest;
}

fix vm_vec_dotprod(const vms_vector *v0,const vms_vector *v1)
{
#if 0
	quadint q;

	q.low = q.high = 0;

	fixmulaccum(&q,v0->x,v1->x);
	fixmulaccum(&q,v0->y,v1->y);
	fixmulaccum(&q,v0->z,v1->z);

	return fixquadadjust(&q);
#else
	int64_t x0 = v0->x;
	int64_t x1 = v1->x;
	int64_t y0 = v0->y;
	int64_t y1 = v1->y;
	int64_t z0 = v0->z;
	int64_t z1 = v1->z;
	int64_t p = (x0 * x1) + (y0 * y1) + (z0 * z1);
	/* Convert back to fix and return. */
	return p >> 16;
#endif
}

fix vm_vec_dot3(fix x,fix y,fix z,const vms_vector *v)
{
#if 0
	quadint q;

	q.low = q.high = 0;

	fixmulaccum(&q,x,v->x);
	fixmulaccum(&q,y,v->y);
	fixmulaccum(&q,z,v->z);

	return fixquadadjust(&q);
#else
	int64_t x0 = x;
	int64_t x1 = v->x;
	int64_t y0 = y;
	int64_t y1 = v->y;
	int64_t z0 = z;
	int64_t z1 = v->z;
	int64_t p = (x0 * x1) + (y0 * y1) + (z0 * z1);
	/* Convert back to fix and return. */
	return p >> 16;
#endif
}

//returns magnitude of a vector
fix vm_vec_mag(const vms_vector *v)
{
	quadint q;

	q.low = q.high = 0;

	fixmulaccum(&q,v->x,v->x);
	fixmulaccum(&q,v->y,v->y);
	fixmulaccum(&q,v->z,v->z);

	return quad_sqrt(q.low,q.high);
}

//computes the distance between two points. (does sub and mag)
fix vm_vec_dist(const vms_vector *v0,const vms_vector *v1)
{
	vms_vector t;

	vm_vec_sub(&t,v0,v1);

	return vm_vec_mag(&t);
}


//computes an approximation of the magnitude of the vector
//uses dist = largest + next_largest*3/8 + smallest*3/16
fix vm_vec_mag_quick(const vms_vector *v)
{
	fix a,b,c,bc;

	a = labs(v->x);
	b = labs(v->y);
	c = labs(v->z);

	if (a < b) {
		fix t=a; a=b; b=t;
	}

	if (b < c) {
		fix t=b; b=c; c=t;

		if (a < b) {
			fix t=a; a=b; b=t;
		}
	}

	bc = (b>>2) + (c>>3);

	return a + bc + (bc>>1);
}


//computes an approximation of the distance between two points.
//uses dist = largest + next_largest*3/8 + smallest*3/16
fix vm_vec_dist_quick(const vms_vector *v0,const vms_vector *v1)
{
	vms_vector t;

	vm_vec_sub(&t,v0,v1);

	return vm_vec_mag_quick(&t);
}

//normalize a vector. returns mag of source vec
fix vm_vec_copy_normalize(vms_vector *dest,const vms_vector *src)
{
	fix m;

	m = vm_vec_mag(src);

	if (m > 0) {
		dest->x = fixdiv(src->x,m);
		dest->y = fixdiv(src->y,m);
		dest->z = fixdiv(src->z,m);
	}

	return m;
}

//normalize a vector. returns mag of source vec
fix vm_vec_normalize(vms_vector *v)
{
	return vm_vec_copy_normalize(v,v);
}

//normalize a vector. returns mag of source vec. uses approx mag
fix vm_vec_copy_normalize_quick(vms_vector *dest,const vms_vector *src)
{
	fix m;

	m = vm_vec_mag_quick(src);

	if (m > 0) {
		dest->x = fixdiv(src->x,m);
		dest->y = fixdiv(src->y,m);
		dest->z = fixdiv(src->z,m);
	}

	return m;
}

//normalize a vector. returns 1/mag of source vec. uses approx 1/mag
fix vm_vec_normalize_quick(vms_vector *v)
{
	return vm_vec_copy_normalize_quick(v,v);
}

//return the normalized direction vector between two points
//dest = normalized(end - start).  Returns 1/mag of direction vector
//NOTE: the order of the parameters matches the vector subtraction
fix vm_vec_normalized_dir_quick(vms_vector *dest,const vms_vector *end,const vms_vector *start)
{
	vm_vec_sub(dest,end,start);

	return vm_vec_normalize_quick(dest);
}

//return the normalized direction vector between two points
//dest = normalized(end - start).  Returns mag of direction vector
//NOTE: the order of the parameters matches the vector subtraction
fix vm_vec_normalized_dir(vms_vector *dest,const vms_vector *end,const vms_vector *start)
{
	vm_vec_sub(dest,end,start);

	return vm_vec_normalize(dest);
}

//computes surface normal from three points. result is normalized
//returns ptr to dest
//dest CANNOT equal either source
vms_vector *vm_vec_normal(vms_vector *dest,const vms_vector *p0,const vms_vector *p1,const vms_vector *p2)
{
	vm_vec_perp(dest,p0,p1,p2);

	vm_vec_normalize(dest);

	return dest;
}

//make sure a vector is reasonably sized to go into a cross product
void check_vec(vms_vector *v)
{
	fix check;
	int cnt = 0;

	check = labs(v->x) | labs(v->y) | labs(v->z);
	
	if (check == 0)
		return;

	if (check & 0xfffc0000) {		//too big

		while (check & 0xfff00000) {
			cnt += 4;
			check >>= 4;
		}

		while (check & 0xfffc0000) {
			cnt += 2;
			check >>= 2;
		}

		v->x >>= cnt;
		v->y >>= cnt;
		v->z >>= cnt;
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

			v->x >>= cnt;
			v->y >>= cnt;
			v->z >>= cnt;
		}
}

//computes cross product of two vectors. 
//Note: this magnitude of the resultant vector is the
//product of the magnitudes of the two source vectors.  This means it is
//quite easy for this routine to overflow and underflow.  Be careful that
//your inputs are ok.
//#ifndef __powerc
#if 0
vms_vector *vm_vec_crossprod(vms_vector *dest,vms_vector *src0,vms_vector *src1)
{
	double d;
	Assert(dest!=src0 && dest!=src1);

	d = (double)(src0->y) * (double)(src1->z);
	d += (double)-(src0->z) * (double)(src1->y);
	d /= 65536.0;
	if (d < 0.0)
		d = d - 1.0;
	dest->x = (fix)d;

	d = (double)(src0->z) * (double)(src1->x);
	d += (double)-(src0->x) * (double)(src1->z);
	d /= 65536.0;
	if (d < 0.0)
		d = d - 1.0;
	dest->y = (fix)d;

	d = (double)(src0->x) * (double)(src1->y);
	d += (double)-(src0->y) * (double)(src1->x);
	d /= 65536.0;
	if (d < 0.0)
		d = d - 1.0;
	dest->z = (fix)d;

	return dest;
}
#else

vms_vector *vm_vec_crossprod(vms_vector *dest,const vms_vector *src0,const vms_vector *src1)
{
	quadint q;

	Assert(dest!=src0 && dest!=src1);

	q.low = q.high = 0;
	fixmulaccum(&q,src0->y,src1->z);
	fixmulaccum(&q,-src0->z,src1->y);
	dest->x = fixquadadjust(&q);

	q.low = q.high = 0;
	fixmulaccum(&q,src0->z,src1->x);
	fixmulaccum(&q,-src0->x,src1->z);
	dest->y = fixquadadjust(&q);

	q.low = q.high = 0;
	fixmulaccum(&q,src0->x,src1->y);
	fixmulaccum(&q,-src0->y,src1->x);
	dest->z = fixquadadjust(&q);

	return dest;
}

#endif


//computes non-normalized surface normal from three points. 
//returns ptr to dest
//dest CANNOT equal either source
vms_vector *vm_vec_perp(vms_vector *dest,const vms_vector *p0,const vms_vector *p1,const vms_vector *p2)
{
	vms_vector t0,t1;

	vm_vec_sub(&t0,p1,p0);
	vm_vec_sub(&t1,p2,p1);

	check_vec(&t0);
	check_vec(&t1);

	return vm_vec_crossprod(dest,&t0,&t1);
}


//computes the delta angle between two vectors. 
//vectors need not be normalized. if they are, call vm_vec_delta_ang_norm()
//the forward vector (third parameter) can be NULL, in which case the absolute
//value of the angle in returned.  Otherwise the angle around that vector is
//returned.
fixang vm_vec_delta_ang(const vms_vector *v0,const vms_vector *v1,const vms_vector *fvec)
{
	vms_vector t0,t1;

	vm_vec_copy_normalize(&t0,v0);
	vm_vec_copy_normalize(&t1,v1);

	return vm_vec_delta_ang_norm(&t0,&t1,fvec);
}

//computes the delta angle between two normalized vectors. 
fixang vm_vec_delta_ang_norm(const vms_vector *v0,const vms_vector *v1,const vms_vector *fvec)
{
	fixang a;

	a = fix_acos(vm_vec_dot(v0,v1));

	if (fvec) {
		vms_vector t;

		vm_vec_cross(&t,v0,v1);

		if (vm_vec_dot(&t,fvec) < 0)
			a = -a;
	}

	return a;
}

vms_matrix *sincos_2_matrix(vms_matrix *m,fix sinp,fix cosp,fix sinb,fix cosb,fix sinh,fix cosh)
{
	fix sbsh,cbch,cbsh,sbch;

	sbsh = fixmul(sinb,sinh);
	cbch = fixmul(cosb,cosh);
	cbsh = fixmul(cosb,sinh);
	sbch = fixmul(sinb,cosh);

	m->rvec.x = cbch + fixmul(sinp,sbsh);		//m1
	m->uvec.z = sbsh + fixmul(sinp,cbch);		//m8

	m->uvec.x = fixmul(sinp,cbsh) - sbch;		//m2
	m->rvec.z = fixmul(sinp,sbch) - cbsh;		//m7

	m->fvec.x = fixmul(sinh,cosp);				//m3
	m->rvec.y = fixmul(sinb,cosp);				//m4
	m->uvec.y = fixmul(cosb,cosp);				//m5
	m->fvec.z = fixmul(cosh,cosp);				//m9

	m->fvec.y = -sinp;								//m6

	return m;

}

//computes a matrix from a set of three angles.  returns ptr to matrix
vms_matrix *vm_angles_2_matrix(vms_matrix *m,const vms_angvec *a)
{
	fix sinp,cosp,sinb,cosb,sinh,cosh;

	fix_sincos(a->p,&sinp,&cosp);
	fix_sincos(a->b,&sinb,&cosb);
	fix_sincos(a->h,&sinh,&cosh);

	return sincos_2_matrix(m,sinp,cosp,sinb,cosb,sinh,cosh);

}

//computes a matrix from a forward vector and an angle
vms_matrix *vm_vec_ang_2_matrix(vms_matrix *m,const vms_vector *v,fixang a)
{
	fix sinb,cosb,sinp,cosp,sinh,cosh;

	fix_sincos(a,&sinb,&cosb);

	sinp = -v->y;
	cosp = fix_sqrt(f1_0 - fixmul(sinp,sinp));

	sinh = fixdiv(v->x,cosp);
	cosh = fixdiv(v->z,cosp);

	return sincos_2_matrix(m,sinp,cosp,sinb,cosb,sinh,cosh);
}


//computes a matrix from one or more vectors. The forward vector is required,
//with the other two being optional.  If both up & right vectors are passed,
//the up vector is used.  If only the forward vector is passed, a bank of
//zero is assumed
//returns ptr to matrix
vms_matrix *vm_vector_2_matrix(vms_matrix *m,vms_vector *fvec,vms_vector *uvec,vms_vector *rvec)
{
	vms_vector *xvec=&m->rvec,*yvec=&m->uvec,*zvec=&m->fvec;

	Assert(fvec != NULL);

	if (vm_vec_copy_normalize(zvec,fvec) == 0) {
		Int3();		//forward vec should not be zero-length
		return m;
	}

	if (uvec == NULL) {

		if (rvec == NULL) {		//just forward vec

bad_vector2:
	;

			if (zvec->x==0 && zvec->z==0) {		//forward vec is straight up or down

				m->rvec.x = f1_0;
				m->uvec.z = (zvec->y<0)?f1_0:-f1_0;

				m->rvec.y = m->rvec.z = m->uvec.x = m->uvec.y = 0;
			}
			else { 		//not straight up or down

				xvec->x = zvec->z;
				xvec->y = 0;
				xvec->z = -zvec->x;

				vm_vec_normalize(xvec);

				vm_vec_crossprod(yvec,zvec,xvec);

			}

		}
		else {						//use right vec

			if (vm_vec_copy_normalize(xvec,rvec) == 0)
				goto bad_vector2;

			vm_vec_crossprod(yvec,zvec,xvec);

			//normalize new perpendicular vector
			if (vm_vec_normalize(yvec) == 0)
				goto bad_vector2;

			//now recompute right vector, in case it wasn't entirely perpendiclar
			vm_vec_crossprod(xvec,yvec,zvec);

		}
	}
	else {		//use up vec

		if (vm_vec_copy_normalize(yvec,uvec) == 0)
			goto bad_vector2;

		vm_vec_crossprod(xvec,yvec,zvec);
		
		//normalize new perpendicular vector
		if (vm_vec_normalize(xvec) == 0)
			goto bad_vector2;

		//now recompute up vector, in case it wasn't entirely perpendiclar
		vm_vec_crossprod(yvec,zvec,xvec);

	}

	return m;
}


//rotates a vector through a matrix. returns ptr to dest vector
//dest CANNOT equal source
vms_vector *vm_vec_rotate(vms_vector *dest,const vms_vector *src,const vms_matrix *m)
{
	Assert(dest != src);

	dest->x = vm_vec_dot(src,&m->rvec);
	dest->y = vm_vec_dot(src,&m->uvec);
	dest->z = vm_vec_dot(src,&m->fvec);

	return dest;
}


//transpose a matrix in place. returns ptr to matrix
vms_matrix *vm_transpose_matrix(vms_matrix *m)
{
	fix t;

	t = m->uvec.x;  m->uvec.x = m->rvec.y;  m->rvec.y = t;
	t = m->fvec.x;  m->fvec.x = m->rvec.z;  m->rvec.z = t;
	t = m->fvec.y;  m->fvec.y = m->uvec.z;  m->uvec.z = t;

	return m;
}

//copy and transpose a matrix. returns ptr to matrix
//dest CANNOT equal source. use vm_transpose_matrix() if this is the case
vms_matrix *vm_copy_transpose_matrix(vms_matrix *dest,const vms_matrix *src)
{
	Assert(dest != src);

	dest->rvec.x = src->rvec.x;
	dest->rvec.y = src->uvec.x;
	dest->rvec.z = src->fvec.x;

	dest->uvec.x = src->rvec.y;
	dest->uvec.y = src->uvec.y;
	dest->uvec.z = src->fvec.y;

	dest->fvec.x = src->rvec.z;
	dest->fvec.y = src->uvec.z;
	dest->fvec.z = src->fvec.z;

	return dest;
}

//mulitply 2 matrices, fill in dest.  returns ptr to dest
//dest CANNOT equal either source
vms_matrix *vm_matrix_x_matrix(vms_matrix *dest,const vms_matrix *src0,const vms_matrix *src1)
{
	Assert(dest!=src0 && dest!=src1);

	dest->rvec.x = vm_vec_dot3(src0->rvec.x,src0->uvec.x,src0->fvec.x, &src1->rvec);
	dest->uvec.x = vm_vec_dot3(src0->rvec.x,src0->uvec.x,src0->fvec.x, &src1->uvec);
	dest->fvec.x = vm_vec_dot3(src0->rvec.x,src0->uvec.x,src0->fvec.x, &src1->fvec);

	dest->rvec.y = vm_vec_dot3(src0->rvec.y,src0->uvec.y,src0->fvec.y, &src1->rvec);
	dest->uvec.y = vm_vec_dot3(src0->rvec.y,src0->uvec.y,src0->fvec.y, &src1->uvec);
	dest->fvec.y = vm_vec_dot3(src0->rvec.y,src0->uvec.y,src0->fvec.y, &src1->fvec);

	dest->rvec.z = vm_vec_dot3(src0->rvec.z,src0->uvec.z,src0->fvec.z, &src1->rvec);
	dest->uvec.z = vm_vec_dot3(src0->rvec.z,src0->uvec.z,src0->fvec.z, &src1->uvec);
	dest->fvec.z = vm_vec_dot3(src0->rvec.z,src0->uvec.z,src0->fvec.z, &src1->fvec);

	return dest;
}

//extract angles from a matrix 
vms_angvec *vm_extract_angles_matrix(vms_angvec *a,const vms_matrix *m)
{
	fix sinh,cosh,cosp;

	if (m->fvec.x==0 && m->fvec.z==0)		//zero head
		a->h = 0;
	else
		a->h = fix_atan2(m->fvec.z,m->fvec.x);

	fix_sincos(a->h,&sinh,&cosh);

	if (abs(sinh) > abs(cosh))				//sine is larger, so use it
		cosp = fixdiv(m->fvec.x,sinh);
	else											//cosine is larger, so use it
		cosp = fixdiv(m->fvec.z,cosh);

	if (cosp==0 && m->fvec.y==0)
		a->p = 0;
	else
		a->p = fix_atan2(cosp,-m->fvec.y);


	if (cosp == 0)	//the cosine of pitch is zero.  we're pitched straight up. say no bank

		a->b = 0;

	else {
		fix sinb,cosb;

		sinb = fixdiv(m->rvec.y,cosp);
		cosb = fixdiv(m->uvec.y,cosp);

		if (sinb==0 && cosb==0)
			a->b = 0;
		else
			a->b = fix_atan2(cosb,sinb);

	}

	return a;
}


//extract heading and pitch from a vector, assuming bank==0
static vms_angvec *vm_extract_angles_vector_normalized(vms_angvec *a,const vms_vector *v)
{
	a->b = 0;		//always zero bank

	a->p = fix_asin(-v->y);

	if (v->x==0 && v->z==0)
		a->h = 0;
	else
		a->h = fix_atan2(v->z,v->x);

	return a;
}

//extract heading and pitch from a vector, assuming bank==0
vms_angvec *vm_extract_angles_vector(vms_angvec *a,const vms_vector *v)
{
	vms_vector t;

	if (vm_vec_copy_normalize(&t,v) != 0)
		vm_extract_angles_vector_normalized(a,&t);

	return a;

}

//compute the distance from a point to a plane.  takes the normalized normal
//of the plane (ebx), a point on the plane (edi), and the point to check (esi).
//returns distance in eax
//distance is signed, so negative dist is on the back of the plane
fix vm_dist_to_plane(const vms_vector *checkp,const vms_vector *norm,const vms_vector *planep)
{
	vms_vector t;

	vm_vec_sub(&t,checkp,planep);

	return vm_vec_dot(&t,norm);

}

vms_vector *vm_vec_make(vms_vector *v,fix x,fix y,fix z)
{
	v->x=x; v->y=y; v->z=z;
	return v;
}

// convert vms_matrix to vms_quaternion
void vms_quaternion_from_matrix(vms_quaternion * q, const vms_matrix * m) 
{
	fix tr = m->rvec.x + m->uvec.y + m->fvec.z;
	if (tr > 0) {
		fix s = fixmul(fix_sqrt(tr + fl2f(1.0)), fl2f(2.0));
		q->w = fixmul(fl2f(0.25), s) * .5;
		q->x = fixdiv(m->fvec.y - m->uvec.z, s) * .5;
		q->y = fixdiv(m->rvec.z - m->fvec.x, s) * .5;
		q->z = fixdiv(m->uvec.x - m->rvec.y, s) * .5;
	} else if ((m->rvec.x > m->uvec.y)&(m->rvec.x > m->fvec.z)) {
		fix s = fixmul(fix_sqrt(fl2f(1.0) + m->rvec.x - m->uvec.y - m->fvec.z), fl2f(2.0));
		q->w = fixdiv(m->fvec.y - m->uvec.z, s) * .5;
		q->x = fixmul(fl2f(0.25), s) * .5;
		q->y = fixdiv(m->rvec.y + m->uvec.x, s) * .5; 
		q->z = fixdiv(m->rvec.z + m->fvec.x, s) * .5;
	} else if (m->uvec.y > m->fvec.z) { 
		fix s = fixmul(fix_sqrt(fl2f(1.0) + m->uvec.y - m->rvec.x - m->fvec.z), fl2f(2.0));
		q->w = fixdiv(m->rvec.z - m->fvec.x, s) * .5;
		q->x = fixdiv(m->rvec.y + m->uvec.x ,s) * .5; 
		q->y = fixmul(fl2f(0.25), s) * .5;
		q->z = fixdiv(m->uvec.z + m->fvec.y , s) * .5;
	} else { 
		fix s = fixmul(fix_sqrt(fl2f(1.0) + m->fvec.z - m->rvec.x - m->uvec.y), fl2f(2.0));
		q->w = fixdiv(m->uvec.x - m->rvec.y , s) * .5;
		q->x = fixdiv(m->rvec.z + m->fvec.x , s) * .5;
		q->y = fixdiv(m->uvec.z +  m->fvec.y, s) * .5;
		q->z = fixmul(fl2f(0.25), s) * .5;
	}
}

// convert vms_quaternion to vms_matrix
void vms_matrix_from_quaternion(vms_matrix * m, const vms_quaternion * q) 
{
	fix sqw = fixmul(q->w * 2, q->w * 2);
	fix sqx = fixmul(q->x * 2, q->x * 2);
	fix sqy = fixmul(q->y * 2, q->y * 2);
	fix sqz = fixmul(q->z * 2, q->z * 2);
	fix invs = fixdiv(fl2f(1.0), (sqw + sqx + sqy + sqz));
	fix tmp1, tmp2;

	m->rvec.x = fixmul(sqx - sqy - sqz + sqw, invs);
	m->uvec.y = fixmul(-sqx + sqy - sqz + sqw, invs);
	m->fvec.z = fixmul(-sqx - sqy + sqz + sqw, invs);
	
	tmp1 = fixmul(q->x * 2, q->y * 2);
	tmp2 = fixmul(q->z * 2, q->w * 2);
	m->uvec.x = fixmul(fixmul(fl2f(2.0), (tmp1 + tmp2)), invs);
	m->rvec.y = fixmul(fixmul(fl2f(2.0), (tmp1 - tmp2)), invs);
	
	tmp1 = fixmul(q->x * 2, q->z * 2);
	tmp2 = fixmul(q->y * 2, q->w * 2);
	m->fvec.x = fixmul(fixmul(fl2f(2.0), (tmp1 - tmp2)), invs);
	m->rvec.z = fixmul(fixmul(fl2f(2.0), (tmp1 + tmp2)), invs);
	
	tmp1 = fixmul(q->y * 2, q->z * 2);
	tmp2 = fixmul(q->x * 2, q->w * 2);
	m->fvec.y = fixmul(fixmul(fl2f(2.0), (tmp1 + tmp2)), invs);
	m->uvec.z = fixmul(fixmul(fl2f(2.0), (tmp1 - tmp2)), invs);
}
