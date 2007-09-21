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
#include "3d.h"
#include "globvars.h"
#include "clipper.h"
#include "error.h"

int free_point_num=0;

g3s_point temp_points[MAX_POINTS_IN_POLY];
g3s_point *free_points[MAX_POINTS_IN_POLY];

void init_free_points(void)
{
	int i;

	for (i=0;i<MAX_POINTS_IN_POLY;i++)
		free_points[i] = &temp_points[i];
}


g3s_point *get_temp_point()
{
	g3s_point *p;

	Assert (free_point_num < MAX_POINTS_IN_POLY );
	p = free_points[free_point_num++];

	p->p3_flags = PF_TEMP_POINT;

	return p;
}

void free_temp_point(g3s_point *p)
{
	Assert(p->p3_flags & PF_TEMP_POINT);

	free_points[--free_point_num] = p;

	p->p3_flags &= ~PF_TEMP_POINT;
}

//clips an edge against one plane. 
g3s_point *clip_edge(int plane_flag,g3s_point *on_pnt,g3s_point *off_pnt)
{
	fix psx_ratio;
	fix a,b,kn,kd;
	g3s_point *tmp;

	//compute clipping value k = (xs-zs) / (xs-xe-zs+ze)
	//use x or y as appropriate, and negate x/y value as appropriate

	if (plane_flag & (CC_OFF_RIGHT | CC_OFF_LEFT)) {
		a = on_pnt->p3_x;
		b = off_pnt->p3_x;
	}
	else {
		a = on_pnt->p3_y;
		b = off_pnt->p3_y;
	}

	if (plane_flag & (CC_OFF_LEFT | CC_OFF_BOT)) {
		a = -a;
		b = -b;
	}

	kn = a - on_pnt->p3_z;						//xs-zs
	kd = kn - b + off_pnt->p3_z;				//xs-zs-xe+ze

	tmp = get_temp_point();

	psx_ratio = fixdiv( kn, kd );

	
// PSX_HACK!!!!
//	tmp->p3_x = on_pnt->p3_x + fixmuldiv(off_pnt->p3_x-on_pnt->p3_x,kn,kd);
//	tmp->p3_y = on_pnt->p3_y + fixmuldiv(off_pnt->p3_y-on_pnt->p3_y,kn,kd);

	tmp->p3_x = on_pnt->p3_x + fixmul( (off_pnt->p3_x-on_pnt->p3_x), psx_ratio);
	tmp->p3_y = on_pnt->p3_y + fixmul( (off_pnt->p3_y-on_pnt->p3_y), psx_ratio);

	if (plane_flag & (CC_OFF_TOP|CC_OFF_BOT))
		tmp->p3_z = tmp->p3_y;
	else
		tmp->p3_z = tmp->p3_x;

	if (plane_flag & (CC_OFF_LEFT|CC_OFF_BOT))
		tmp->p3_z = -tmp->p3_z;

	if (on_pnt->p3_flags & PF_UVS) {
// PSX_HACK!!!!
//		tmp->p3_u = on_pnt->p3_u + fixmuldiv(off_pnt->p3_u-on_pnt->p3_u,kn,kd);
//		tmp->p3_v = on_pnt->p3_v + fixmuldiv(off_pnt->p3_v-on_pnt->p3_v,kn,kd);
		tmp->p3_u = on_pnt->p3_u + fixmul((off_pnt->p3_u-on_pnt->p3_u), psx_ratio);
		tmp->p3_v = on_pnt->p3_v + fixmul((off_pnt->p3_v-on_pnt->p3_v), psx_ratio);

		tmp->p3_flags |= PF_UVS;
	}

	if (on_pnt->p3_flags & PF_LS) {
// PSX_HACK
//		tmp->p3_r = on_pnt->p3_r + fixmuldiv(off_pnt->p3_r-on_pnt->p3_r,kn,kd);
//		tmp->p3_g = on_pnt->p3_g + fixmuldiv(off_pnt->p3_g-on_pnt->p3_g,kn,kd);
//		tmp->p3_b = on_pnt->p3_b + fixmuldiv(off_pnt->p3_b-on_pnt->p3_b,kn,kd);

		tmp->p3_l = on_pnt->p3_l + fixmul((off_pnt->p3_l-on_pnt->p3_l), psx_ratio);

		tmp->p3_flags |= PF_LS;
	}

	g3_code_point(tmp);

	return tmp;	
}

//clips a line to the viewing pyramid.
void clip_line(g3s_point **p0,g3s_point **p1,ubyte codes_or)
{
	int plane_flag;
	g3s_point *old_p1;

	//might have these left over
	(*p0)->p3_flags &= ~(PF_UVS|PF_LS);
	(*p1)->p3_flags &= ~(PF_UVS|PF_LS);

	for (plane_flag=1;plane_flag<16;plane_flag<<=1)
		if (codes_or & plane_flag) {

			if ((*p0)->p3_codes & plane_flag)
				{g3s_point *t=*p0; *p0=*p1; *p1=t;}	//swap!

			old_p1 = *p1;

			*p1 = clip_edge(plane_flag,*p0,*p1);

			if (old_p1->p3_flags & PF_TEMP_POINT)
				free_temp_point(old_p1);
		}

}


int clip_plane(int plane_flag,g3s_point **src,g3s_point **dest,int *nv,g3s_codes *cc)
{
	int i;
	g3s_point **save_dest=dest;

	//copy first two verts to end
	src[*nv] = src[0];
	src[*nv+1] = src[1];

	cc->and = 0xff; cc->or = 0;

	for (i=1;i<=*nv;i++) {

		if (src[i]->p3_codes & plane_flag) {				//cur point off?

			if (! (src[i-1]->p3_codes & plane_flag)) {	//prev not off?

				*dest = clip_edge(plane_flag,src[i-1],src[i]);
				cc->or  |= (*dest)->p3_codes;
				cc->and &= (*dest)->p3_codes;
				dest++;
			}

			if (! (src[i+1]->p3_codes & plane_flag)) {

				*dest = clip_edge(plane_flag,src[i+1],src[i]);
				cc->or  |= (*dest)->p3_codes;
				cc->and &= (*dest)->p3_codes;
				dest++;
			}

			//see if must free discarded point

			if (src[i]->p3_flags & PF_TEMP_POINT)
				free_temp_point(src[i]);
		}
		else {			//cur not off, copy to dest buffer

			*dest++ = src[i];

			cc->or  |= src[i]->p3_codes;
			cc->and &= src[i]->p3_codes;
		}
	}

	return (dest-save_dest);
}


g3s_point **clip_polygon(g3s_point **src,g3s_point **dest,int *nv,g3s_codes *cc)
{
	int plane_flag;
	g3s_point **t;

	for (plane_flag=1;plane_flag<16;plane_flag<<=1)

		if (cc->or & plane_flag) {

			*nv = clip_plane(plane_flag,src,dest,nv,cc);

			if (cc->and)		//clipped away
				return dest;

			t = src; src = dest; dest = t;

		}

	return src;		//we swapped after we copied
}

