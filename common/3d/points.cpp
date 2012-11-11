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
 * Routines for point definition, rotation, etc.
 * 
 */


#include "3d.h"
#include "globvars.h"


//code a point.  fills in the p3_codes field of the point, and returns the codes
ubyte g3_code_point(g3s_point *p)
{
	ubyte cc=0;

	if (p->p3_x > p->p3_z)
		cc |= CC_OFF_RIGHT;

	if (p->p3_y > p->p3_z)
		cc |= CC_OFF_TOP;

	if (p->p3_x < -p->p3_z)
		cc |= CC_OFF_LEFT;

	if (p->p3_y < -p->p3_z)
		cc |= CC_OFF_BOT;

	if (p->p3_z < 0)
		cc |= CC_BEHIND;

	return p->p3_codes = cc;

}

//rotates a point. returns codes.  does not check if already rotated
ubyte g3_rotate_point(g3s_point *dest,const vms_vector *src)
{
	vms_vector tempv;

	vm_vec_sub(&tempv,src,&View_position);

	vm_vec_rotate(&dest->p3_vec,&tempv,&View_matrix);

	dest->p3_flags = 0;	//no projected

	return g3_code_point(dest);

}

//checks for overflow & divides if ok, fillig in r
//returns true if div is ok, else false
int checkmuldiv(fix *r,fix a,fix b,fix c)
{
	quadint q,qt;

	q.low=q.high=0;
	fixmulaccum(&q,a,b);

	qt = q;
	if (qt.high < 0)
		fixquadnegate(&qt);

	qt.high *= 2;
	if (qt.low > 0x7fff)
		qt.high++;

	if (qt.high >= c)
		return 0;
	else {
		*r = fixdivquadlong(q.low,q.high,c);
		return 1;
	}
}

//projects a point
void g3_project_point(g3s_point *p)
{
#ifndef __powerc
	fix tx,ty;

	if (p->p3_flags & PF_PROJECTED || p->p3_codes & CC_BEHIND)
		return;

	if (checkmuldiv(&tx,p->p3_x,Canv_w2,p->p3_z) && checkmuldiv(&ty,p->p3_y,Canv_h2,p->p3_z)) {
		p->p3_sx = Canv_w2 + tx;
		p->p3_sy = Canv_h2 - ty;
		p->p3_flags |= PF_PROJECTED;
	}
	else
		p->p3_flags |= PF_OVERFLOW;
#else
	double fz;
	
	if ((p->p3_flags & PF_PROJECTED) || (p->p3_codes & CC_BEHIND))
		return;
	
	if ( p->p3_z <= 0 )	{
		p->p3_flags |= PF_OVERFLOW;
		return;
	}

	fz = f2fl(p->p3_z);
	p->p3_sx = fl2f(fCanv_w2 + (f2fl(p->p3_x)*fCanv_w2 / fz));
	p->p3_sy = fl2f(fCanv_h2 - (f2fl(p->p3_y)*fCanv_h2 / fz));

	p->p3_flags |= PF_PROJECTED;
#endif
}

//from a 2d point, compute the vector through that point
void g3_point_2_vec(vms_vector *v,short sx,short sy)
{
	vms_vector tempv;
	vms_matrix tempm;

	tempv.x =  fixmuldiv(fixdiv((sx<<16) - Canv_w2,Canv_w2),Matrix_scale.z,Matrix_scale.x);
	tempv.y = -fixmuldiv(fixdiv((sy<<16) - Canv_h2,Canv_h2),Matrix_scale.z,Matrix_scale.y);
	tempv.z = f1_0;

	vm_vec_normalize(&tempv);

	vm_copy_transpose_matrix(&tempm,&Unscaled_matrix);

	vm_vec_rotate(v,&tempv,&tempm);

}

//delta rotation functions
vms_vector *g3_rotate_delta_x(vms_vector *dest,fix dx)
{
	dest->x = fixmul(View_matrix.rvec.x,dx);
	dest->y = fixmul(View_matrix.uvec.x,dx);
	dest->z = fixmul(View_matrix.fvec.x,dx);

	return dest;
}

vms_vector *g3_rotate_delta_y(vms_vector *dest,fix dy)
{
	dest->x = fixmul(View_matrix.rvec.y,dy);
	dest->y = fixmul(View_matrix.uvec.y,dy);
	dest->z = fixmul(View_matrix.fvec.y,dy);

	return dest;
}

vms_vector *g3_rotate_delta_z(vms_vector *dest,fix dz)
{
	dest->x = fixmul(View_matrix.rvec.z,dz);
	dest->y = fixmul(View_matrix.uvec.z,dz);
	dest->z = fixmul(View_matrix.fvec.z,dz);

	return dest;
}


vms_vector *g3_rotate_delta_vec(vms_vector *dest,const vms_vector *src)
{
	return vm_vec_rotate(dest,src,&View_matrix);
}

ubyte g3_add_delta_vec(g3s_point *dest,const g3s_point *src,const vms_vector *deltav)
{
	vm_vec_add(&dest->p3_vec,&src->p3_vec,deltav);

	dest->p3_flags = 0;		//not projected

	return g3_code_point(dest);
}

//calculate the depth of a point - returns the z coord of the rotated point
fix g3_calc_point_depth(const vms_vector *pnt)
{
	quadint q;

	q.low=q.high=0;
	fixmulaccum(&q,(pnt->x - View_position.x),View_matrix.fvec.x);
	fixmulaccum(&q,(pnt->y - View_position.y),View_matrix.fvec.y);
	fixmulaccum(&q,(pnt->z - View_position.z),View_matrix.fvec.z);

	return fixquadadjust(&q);
}



