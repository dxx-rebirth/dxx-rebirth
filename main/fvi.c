/* $Id: fvi.c,v 1.3 2003-10-10 09:36:35 btb Exp $ */
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

/*
 *
 * New home for find_vector_intersection()
 *
 * Old Log:
 * Revision 1.7  1995/10/21  23:52:18  allender
 * #ifdef'ed out stack debug stuff
 *
 * Revision 1.6  1995/10/10  12:07:42  allender
 * add forgotten ;
 *
 * Revision 1.5  1995/10/10  11:47:27  allender
 * put in stack space check
 *
 * Revision 1.4  1995/08/23  21:34:08  allender
 * fix mcc compiler warning
 *
 * Revision 1.3  1995/08/14  14:35:18  allender
 * changed transparency to 0
 *
 * Revision 1.2  1995/07/05  16:50:51  allender
 * transparency/kitchen change
 *
 * Revision 1.1  1995/05/16  15:24:59  allender
 * Initial revision
 *
 * Revision 2.3  1995/03/24  14:49:04  john
 * Added cheat for player to go thru walls.
 *
 * Revision 2.2  1995/03/21  17:58:32  john
 * Fixed bug with normals..
 *
 *
 * Revision 2.1  1995/03/20  18:15:37  john
 * Added code to not store the normals in the segment structure.
 *
 * Revision 2.0  1995/02/27  11:27:41  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.49  1995/02/22  14:45:47  allender
 * remove anonymous unions from object structure
 *
 * Revision 1.48  1995/02/22  13:24:50  john
 * Removed the vecmat anonymous unions.
 *
 * Revision 1.47  1995/02/07  16:17:26  matt
 * Disabled all robot-robot collisions except those involving two green
 * guys.  Used to do collisions if either robot was green guy.
 *
 * Revision 1.46  1995/02/02  14:07:53  matt
 * Fixed confusion about which segment you are touching when you're
 * touching a wall.  This manifested itself in spurious lava burns.
 *
 * Revision 1.45  1995/02/02  13:45:53  matt
 * Made a bunch of lint-inspired changes
 *
 * Revision 1.44  1995/01/24  12:10:17  matt
 * Fudged collisions for player/player, and player weapon/other player in
 * coop games.
 *
 * Revision 1.43  1995/01/14  19:16:45  john
 * First version of new bitmap paging code.
 *
 * Revision 1.42  1994/12/15  12:22:40  matt
 * Small change which may or may not help
 *
 * Revision 1.41  1994/12/14  11:45:51  matt
 * Fixed (hopefully) little bug with invalid segnum
 *
 * Revision 1.40  1994/12/13  17:12:01  matt
 * Increased edge tolerance a bunch more
 *
 * Revision 1.39  1994/12/13  14:37:59  matt
 * Fixed another stupid little bug
 *
 * Revision 1.38  1994/12/13  13:25:44  matt
 * Increased tolerance massively to avoid catching on corners
 *
 * Revision 1.37  1994/12/13  12:02:20  matt
 * Fixed small bug
 *
 * Revision 1.36  1994/12/13  11:17:35  matt
 * Lots of changes to hopefully fix objects leaving the mine.  Note that
 * this code should be considered somewhat experimental - one problem I
 * know about is that you can get stuck on edges more easily than before.
 * There may be other problems I don't know about yet.
 *
 * Revision 1.35  1994/12/12  01:20:57  matt
 * Added hack in object-object collisions that treats claw guys as
 * if they have 3/4 of their actual radius.
 *
 * Revision 1.34  1994/12/04  22:48:39  matt
 * Physics & FVI now only build seglist for player objects, and they
 * responsilby deal with buffer full conditions
 *
 * Revision 1.33  1994/12/04  22:07:05  matt
 * Added better handing of buffer full condition
 *
 * Revision 1.32  1994/12/01  21:06:33  matt
 * Several important changes:
 *  (1) Checking against triangulated sides has been standardized a bit
 *  (2) Code has been added to de-triangulate some sides
 *  (3) BIG ONE: the tolerance for checking a point against a plane has
 *      been drastically relaxed
 *
 *
 * Revision 1.31  1994/11/27  23:15:03  matt
 * Made changes for new mprintf calling convention
 *
 * Revision 1.30  1994/11/19  15:20:30  mike
 * rip out unused code and data
 *
 * Revision 1.29  1994/11/16  12:18:17  mike
 * hack for green_guy:green_guy collision detection.
 *
 * Revision 1.28  1994/11/10  13:08:54  matt
 * Added support for new run-length-encoded bitmaps
 *
 * Revision 1.27  1994/10/31  12:27:51  matt
 * Added new function object_intersects_wall()
 *
 * Revision 1.26  1994/10/20  13:59:27  matt
 * Added assert
 *
 * Revision 1.25  1994/10/09  23:51:09  matt
 * Made find_hitpoint_uv() work with triangulated sides
 *
 * Revision 1.24  1994/09/25  00:39:29  matt
 * Took out mprintf's
 *
 * Revision 1.23  1994/09/25  00:37:53  matt
 * Made the 'find the point in the bitmap where something hit' system
 * publicly accessible.
 *
 * Revision 1.22  1994/09/21  16:58:22  matt
 * Fixed bug in trans wall check that was checking against verically
 * flipped bitmap (i.e., the y coord was negative when checking).
 *
 * Revision 1.21  1994/09/02  11:31:40  matt
 * Fixed object/object collisions, so you can't fly through robots anymore.
 * Cleaned up object damage system.
 *
 * Revision 1.20  1994/08/26  09:42:03  matt
 * Increased the size of a buffer
 *
 * Revision 1.19  1994/08/11  18:57:53  mike
 * Convert shorts to ints for optimization.
 *
 * Revision 1.18  1994/08/08  21:38:24  matt
 * Put in small optimization
 *
 * Revision 1.17  1994/08/08  12:21:52  yuan
 * Fixed assert
 *
 * Revision 1.16  1994/08/08  11:47:04  matt
 * Cleaned up fvi and physics a little
 *
 * Revision 1.15  1994/08/04  00:21:04  matt
 * Cleaned up fvi & physics error handling; put in code to make sure objects
 * are in correct segment; simplified segment finding for objects and points
 *
 * Revision 1.14  1994/08/02  19:04:26  matt
 * Cleaned up vertex list functions
 *
 * Revision 1.13  1994/08/02  09:56:28  matt
 * Put in check for bad value find_plane_line_intersection()
 *
 * Revision 1.12  1994/08/01  17:27:26  matt
 * Added support for triangulated walls in trans point check
 *
 * Revision 1.11  1994/08/01  13:30:40  matt
 * Made fvi() check holes in transparent walls, and changed fvi() calling
 * parms to take all input data in query structure.
 *
 * Revision 1.10  1994/07/13  21:47:17  matt
 * FVI() and physics now keep lists of segments passed through which the
 * trigger code uses.
 *
 * Revision 1.9  1994/07/09  21:21:40  matt
 * Fixed, hopefull, bugs in sphere-to-vector intersection code
 *
 * Revision 1.8  1994/07/08  14:26:42  matt
 * Non-needed powerups don't get picked up now; this required changing FVI to
 * take a list of ingore objects rather than just one ignore object.
 *
 * Revision 1.7  1994/07/06  20:02:37  matt
 * Made change to match gameseg that uses lowest point number as reference
 * point when checking against a plane
 *
 * Revision 1.6  1994/06/29  15:43:58  matt
 * When computing intersection of vector and sphere, use the radii of both
 * objects.
 *
 * Revision 1.5  1994/06/14  15:57:58  matt
 * Took out asserts, and added other hacks, pending real bug fixes
 *
 * Revision 1.4  1994/06/13  23:10:08  matt
 * Fixed problems with triangulated sides
 *
 * Revision 1.3  1994/06/09  12:11:14  matt
 * Fixed confusing use of two variables, hit_objnum & fvi_hit_object, to
 * keep the same information in different ways.
 *
 * Revision 1.2  1994/06/09  09:58:38  matt
 * Moved find_vector_intersection() from physics.c to new file fvi.c
 *
 * Revision 1.1  1994/06/09  09:25:57  matt
 * Initial revision
 *
 *
 */


#define NEW_FVI_STUFF 1

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef MACINTOSH
#include <Memory.h>
#endif

#include "pstypes.h"
#include "u_mem.h"
#include "error.h"
#include "mono.h"

#include "inferno.h"
#include "fvi.h"
#include "segment.h"
#include "object.h"
#include "wall.h"
#include "laser.h"
#include "rle.h"
#include "robot.h"
#include "piggy.h"
#include "player.h"

extern int Physics_cheat_flag;

#define face_type_num(nfaces,face_num,tri_edge) ((nfaces==1)?0:(tri_edge*2 + face_num))

#include "fvi_a.h"

//find the point on the specified plane where the line intersects
//returns true if point found, false if line parallel to plane
//new_pnt is the found point on the plane
//plane_pnt & plane_norm describe the plane
//p0 & p1 are the ends of the line
int find_plane_line_intersection(vms_vector *new_pnt,vms_vector *plane_pnt,vms_vector *plane_norm,vms_vector *p0,vms_vector *p1,fix rad)
{
	vms_vector d,w;
	fix num,den;

	vm_vec_sub(&d,p1,p0);
	vm_vec_sub(&w,p0,plane_pnt);

	num =  vm_vec_dot(plane_norm,&w);
	den = -vm_vec_dot(plane_norm,&d);

//Why does this assert hit so often
//	Assert(num > -rad);

	num -= rad;			//move point out by rad

	//check for various bad values

	if ( (den==0) ||					//moving parallel to wall, so can't hit it
		  ((den>0) &&
			( (num>den) ||				//frac greater than one
		     (-num>>15)>=den)) ||	//will overflow (large negative)
		  (den<0 && num<den))		//frac greater than one
		return 0;

//if (num>0) {mprintf(1,"HEY! num>0 in FVI!!!"); return 0;}
//??	Assert(num>=0);
//    Assert(num >= den);

	//do check for potenial overflow
	{
		fix k;

		if (labs(num)/(f1_0/2) >= labs(den)) {Int3(); return 0;}
		k = fixdiv(num,den);

		Assert(k<=f1_0);		//should be trapped above

//		Assert(k>=0);
		if (oflow_check(d.x,k) || oflow_check(d.y,k) || oflow_check(d.z,k)) return 0;
		//Note: it is ok for k to be greater than 1, since this might mean
		//that an object with a non-zero radius that moved from p0 to p1
		//actually hit the wall on the "other side" of p0.
	}

	vm_vec_scale2(&d,num,den);

	vm_vec_add(new_pnt,p0,&d);

	//we should have vm_vec_scale2_add2()

	return 1;

}

typedef struct vec2d {
	fix i,j;
} vec2d;

//given largest componant of normal, return i & j
//if largest componant is negative, swap i & j
int ij_table[3][2] =        {
							{2,1},          //pos x biggest
							{0,2},          //pos y biggest
							{1,0},          //pos z biggest
						};

//intersection types
#define IT_NONE 0       //doesn't touch face at all
#define IT_FACE 1       //touches face
#define IT_EDGE 2       //touches edge of face
#define IT_POINT        3       //touches vertex

//see if a point in inside a face by projecting into 2d
uint check_point_to_face(vms_vector *checkp, side *s,int facenum,int nv,int *vertex_list)
{
	vms_vector_array *checkp_array;
	vms_vector_array norm;
	vms_vector t;
	int biggest;
///
	int i,j,edge;
	uint edgemask;
	fix check_i,check_j;
	vms_vector_array *v0,*v1;

	#ifdef COMPACT_SEGS
		get_side_normal(sp, s-sp->sides, facenum, (vms_vector *)&norm );
	#else
		memcpy( &norm, &s->normals[facenum], sizeof(vms_vector_array));
	#endif
	checkp_array = (vms_vector_array *)checkp;

	//now do 2d check to see if point is in side

	//project polygon onto plane by finding largest component of normal
	t.x = labs(norm.xyz[0]); t.y = labs(norm.xyz[1]); t.z = labs(norm.xyz[2]);

	if (t.x > t.y) if (t.x > t.z) biggest=0; else biggest=2;
	else if (t.y > t.z) biggest=1; else biggest=2;

	if (norm.xyz[biggest] > 0) {
		i = ij_table[biggest][0];
		j = ij_table[biggest][1];
	}
	else {
		i = ij_table[biggest][1];
		j = ij_table[biggest][0];
	}

	//now do the 2d problem in the i,j plane

	check_i = checkp_array->xyz[i];
	check_j = checkp_array->xyz[j];

	for (edge=edgemask=0;edge<nv;edge++) {
		vec2d edgevec,checkvec;
		fix d;

		v0 = (vms_vector_array *)&Vertices[vertex_list[facenum*3+edge]];
		v1 = (vms_vector_array *)&Vertices[vertex_list[facenum*3+((edge+1)%nv)]];

		edgevec.i = v1->xyz[i] - v0->xyz[i];
		edgevec.j = v1->xyz[j] - v0->xyz[j];

		checkvec.i = check_i - v0->xyz[i];
		checkvec.j = check_j - v0->xyz[j];

		d = fixmul(checkvec.i,edgevec.j) - fixmul(checkvec.j,edgevec.i);

		if (d < 0)              		//we are outside of triangle
			edgemask |= (1<<edge);
	}

	return edgemask;

}


//check if a sphere intersects a face
int check_sphere_to_face(vms_vector *pnt, side *s,int facenum,int nv,fix rad,int *vertex_list)
{
	vms_vector checkp=*pnt;
	uint edgemask;

	//now do 2d check to see if point is in side

	edgemask = check_point_to_face(pnt,s,facenum,nv,vertex_list);

	//we've gone through all the sides, are we inside?

	if (edgemask == 0)
		return IT_FACE;
	else {
		vms_vector edgevec,checkvec;            //this time, real 3d vectors
		vms_vector closest_point;
		fix edgelen,d,dist;
		vms_vector *v0,*v1;
		int itype;
		int edgenum;

		//get verts for edge we're behind

		for (edgenum=0;!(edgemask&1);(edgemask>>=1),edgenum++);

		v0 = &Vertices[vertex_list[facenum*3+edgenum]];
		v1 = &Vertices[vertex_list[facenum*3+((edgenum+1)%nv)]];

		//check if we are touching an edge or point

		vm_vec_sub(&checkvec,&checkp,v0);
		edgelen = vm_vec_normalized_dir(&edgevec,v1,v0);
		
		//find point dist from planes of ends of edge

		d = vm_vec_dot(&edgevec,&checkvec);

		if (d+rad < 0) return IT_NONE;                  //too far behind start point

		if (d-rad > edgelen) return IT_NONE;    //too far part end point

		//find closest point on edge to check point

		itype = IT_POINT;

		if (d < 0) closest_point = *v0;
		else if (d > edgelen) closest_point = *v1;
		else {
			itype = IT_EDGE;

			//vm_vec_scale(&edgevec,d);
			//vm_vec_add(&closest_point,v0,&edgevec);

			vm_vec_scale_add(&closest_point,v0,&edgevec,d);
		}

		dist = vm_vec_dist(&checkp,&closest_point);

		if (dist <= rad)
			return (itype==IT_POINT)?IT_NONE:itype;
		else
			return IT_NONE;
	}


}

//returns true if line intersects with face. fills in newp with intersection
//point on plane, whether or not line intersects side
//facenum determines which of four possible faces we have
//note: the seg parm is temporary, until the face itself has a point field
int check_line_to_face(vms_vector *newp,vms_vector *p0,vms_vector *p1,segment *seg,int side,int facenum,int nv,fix rad)
{
	vms_vector checkp;
	int pli;
	struct side *s=&seg->sides[side];
	int vertex_list[6];
	int num_faces;
	int vertnum;
	vms_vector norm;

	#ifdef COMPACT_SEGS
		get_side_normal(seg, side, facenum, &norm );
	#else
		norm = seg->sides[side].normals[facenum];
	#endif

	if ((seg-Segments)==-1)
		Error("segnum == -1 in check_line_to_face()");

	create_abs_vertex_lists(&num_faces,vertex_list,seg-Segments,side);

	//use lowest point number
	if (num_faces==2) {
		vertnum = min(vertex_list[0],vertex_list[2]);
	}
	else {
		int i;
		vertnum = vertex_list[0];
		for (i=1;i<4;i++)
			if (vertex_list[i] < vertnum)
				vertnum = vertex_list[i];
	}

	pli = find_plane_line_intersection(newp,&Vertices[vertnum],&norm,p0,p1,rad);

	if (!pli) return IT_NONE;

	checkp = *newp;

	//if rad != 0, project the point down onto the plane of the polygon

	if (rad!=0)
		vm_vec_scale_add2(&checkp,&norm,-rad);

	return check_sphere_to_face(&checkp,s,facenum,nv,rad,vertex_list);

}

//returns the value of a determinant
fix calc_det_value(vms_matrix *det)
{
	return 	fixmul(det->rvec.x,fixmul(det->uvec.y,det->fvec.z)) -
			 	fixmul(det->rvec.x,fixmul(det->uvec.z,det->fvec.y)) -
			 	fixmul(det->rvec.y,fixmul(det->uvec.x,det->fvec.z)) +
			 	fixmul(det->rvec.y,fixmul(det->uvec.z,det->fvec.x)) +
			 	fixmul(det->rvec.z,fixmul(det->uvec.x,det->fvec.y)) -
			 	fixmul(det->rvec.z,fixmul(det->uvec.y,det->fvec.x));
}

//computes the parameters of closest approach of two lines
//fill in two parameters, t0 & t1.  returns 0 if lines are parallel, else 1
int check_line_to_line(fix *t1,fix *t2,vms_vector *p1,vms_vector *v1,vms_vector *p2,vms_vector *v2)
{
	vms_matrix det;
	fix d,cross_mag2;		//mag squared cross product

	vm_vec_sub(&det.rvec,p2,p1);
	vm_vec_cross(&det.fvec,v1,v2);
	cross_mag2 = vm_vec_dot(&det.fvec,&det.fvec);

	if (cross_mag2 == 0)
		return 0;			//lines are parallel

	det.uvec = *v2;
	d = calc_det_value(&det);
	if (oflow_check(d,cross_mag2))
		return 0;
	else
		*t1 = fixdiv(d,cross_mag2);

	det.uvec = *v1;
	d = calc_det_value(&det);
	if (oflow_check(d,cross_mag2))
		return 0;
	else
		*t2 = fixdiv(d,cross_mag2);

	return 1;		//found point
}

#ifdef NEW_FVI_STUFF
int disable_new_fvi_stuff=0;
#else
#define disable_new_fvi_stuff 1
#endif

//this version is for when the start and end positions both poke through
//the plane of a side.  In this case, we must do checks against the edge
//of faces
int special_check_line_to_face(vms_vector *newp,vms_vector *p0,vms_vector *p1,segment *seg,int side,int facenum,int nv,fix rad)
{
	vms_vector move_vec;
	fix edge_t,move_t,edge_t2,move_t2,closest_dist;
	fix edge_len,move_len;
	int vertex_list[6];
	int num_faces,edgenum;
	uint edgemask;
	vms_vector *edge_v0,*edge_v1,edge_vec;
	struct side *s=&seg->sides[side];
	vms_vector closest_point_edge,closest_point_move;

	if (disable_new_fvi_stuff)
		return check_line_to_face(newp,p0,p1,seg,side,facenum,nv,rad);

	//calc some basic stuff

	if ((seg-Segments)==-1)
		Error("segnum == -1 in special_check_line_to_face()");

	create_abs_vertex_lists(&num_faces,vertex_list,seg-Segments,side);
	vm_vec_sub(&move_vec,p1,p0);

	//figure out which edge(s) to check against

	edgemask = check_point_to_face(p0,s,facenum,nv,vertex_list);

	if (edgemask == 0)
		return check_line_to_face(newp,p0,p1,seg,side,facenum,nv,rad);

	for (edgenum=0;!(edgemask&1);edgemask>>=1,edgenum++);

	edge_v0 = &Vertices[vertex_list[facenum*3+edgenum]];
	edge_v1 = &Vertices[vertex_list[facenum*3+((edgenum+1)%nv)]];

	vm_vec_sub(&edge_vec,edge_v1,edge_v0);

	//is the start point already touching the edge?

	//??

	//first, find point of closest approach of vec & edge

	edge_len = vm_vec_normalize(&edge_vec);
	move_len = vm_vec_normalize(&move_vec);

	check_line_to_line(&edge_t,&move_t,edge_v0,&edge_vec,p0,&move_vec);

	//make sure t values are in valid range

	if (move_t<0 || move_t>move_len+rad)
		return IT_NONE;

	if (move_t > move_len)
		move_t2 = move_len;
	else
		move_t2 = move_t;

	if (edge_t < 0)		//saturate at points
		edge_t2 = 0;
	else
		edge_t2 = edge_t;
	
	if (edge_t2 > edge_len)		//saturate at points
		edge_t2 = edge_len;
	
	//now, edge_t & move_t determine closest points.  calculate the points.

	vm_vec_scale_add(&closest_point_edge,edge_v0,&edge_vec,edge_t2);
	vm_vec_scale_add(&closest_point_move,p0,&move_vec,move_t2);

	//find dist between closest points

	closest_dist = vm_vec_dist(&closest_point_edge,&closest_point_move);

	//could we hit with this dist?

	//note massive tolerance here
//	if (closest_dist < (rad*18)/20) {		//we hit.  figure out where
	if (closest_dist < (rad*15)/20) {		//we hit.  figure out where

		//now figure out where we hit

		vm_vec_scale_add(newp,p0,&move_vec,move_t-rad);

		return IT_EDGE;

	}
	else
		return IT_NONE;			//no hit

}

//maybe this routine should just return the distance and let the caller
//decide it it's close enough to hit
//determine if and where a vector intersects with a sphere
//vector defined by p0,p1
//returns dist if intersects, and fills in intp
//else returns 0
int check_vector_to_sphere_1(vms_vector *intp,vms_vector *p0,vms_vector *p1,vms_vector *sphere_pos,fix sphere_rad)
{
	vms_vector d,dn,w,closest_point;
	fix mag_d,dist,w_dist,int_dist;

	//this routine could be optimized if it's taking too much time!

	vm_vec_sub(&d,p1,p0);
	vm_vec_sub(&w,sphere_pos,p0);

	mag_d = vm_vec_copy_normalize(&dn,&d);

	if (mag_d == 0) {
		int_dist = vm_vec_mag(&w);
		*intp = *p0;
		return (int_dist<sphere_rad)?int_dist:0;
	}

	w_dist = vm_vec_dot(&dn,&w);

	if (w_dist < 0)		//moving away from object
		 return 0;

	if (w_dist > mag_d+sphere_rad)
		return 0;		//cannot hit

	vm_vec_scale_add(&closest_point,p0,&dn,w_dist);

	dist = vm_vec_dist(&closest_point,sphere_pos);

	if (dist < sphere_rad) {
		fix dist2,rad2,shorten;

		dist2 = fixmul(dist,dist);
		rad2 = fixmul(sphere_rad,sphere_rad);

		shorten = fix_sqrt(rad2 - dist2);

		int_dist = w_dist-shorten;

		if (int_dist > mag_d || int_dist < 0) {
			//past one or the other end of vector, which means we're inside

			*intp = *p0;		//don't move at all
			return 1;
		}

		vm_vec_scale_add(intp,p0,&dn,int_dist);         //calc intersection point

//		{
//			fix dd = vm_vec_dist(intp,sphere_pos);
//			Assert(dd == sphere_rad);
//			mprintf(0,"dd=%x, rad=%x, delta=%x\n",dd,sphere_rad,dd-sphere_rad);
//		}


		return int_dist;
	}
	else
		return 0;
}

/*
//$$fix get_sphere_int_dist(vms_vector *w,fix dist,fix rad);
//$$
//$$#pragma aux get_sphere_int_dist parm [esi] [ebx] [ecx] value [eax] modify exact [eax ebx ecx edx] = \
//$$	"mov eax,ebx"		\
//$$	"imul eax"			\
//$$							\
//$$	"mov ebx,eax"		\
//$$   "mov eax,ecx"		\
//$$	"mov ecx,edx"		\
//$$							\
//$$	"imul eax"			\
//$$							\
//$$	"sub eax,ebx"		\
//$$	"sbb edx,ecx"		\
//$$							\
//$$	"call quad_sqrt"	\
//$$							\
//$$	"push eax"			\
//$$							\
//$$	"push ebx"			\
//$$	"push ecx"			\
//$$							\
//$$	"mov eax,[esi]"	\
//$$	"imul eax"			\
//$$	"mov ebx,eax"		\
//$$	"mov ecx,edx"		\
//$$	"mov eax,4[esi]"	\
//$$	"imul eax"			\
//$$	"add ebx,eax"		\
//$$	"adc ecx,edx"		\
//$$	"mov eax,8[esi]"	\
//$$	"imul eax"			\
//$$	"add eax,ebx"		\
//$$	"adc edx,ecx"		\
//$$							\
//$$	"pop ecx"			\
//$$	"pop ebx"			\
//$$							\
//$$	"sub eax,ebx"		\
//$$	"sbb edx,ecx"		\
//$$							\
//$$	"call quad_sqrt"	\
//$$							\
//$$	"pop ebx"			\
//$$	"sub eax,ebx";
//$$
//$$
//$$//determine if and where a vector intersects with a sphere
//$$//vector defined by p0,p1
//$$//returns dist if intersects, and fills in intp. if no intersect, return 0
//$$fix check_vector_to_sphere_2(vms_vector *intp,vms_vector *p0,vms_vector *p1,vms_vector *sphere_pos,fix sphere_rad)
//$${
//$$	vms_vector d,w,c;
//$$	fix mag_d,dist,mag_c,mag_w;
//$$	vms_vector wn,dn;
//$$
//$$	vm_vec_sub(&d,p1,p0);
//$$	vm_vec_sub(&w,sphere_pos,p0);
//$$
//$$	//wn = w; mag_w = vm_vec_normalize(&wn);
//$$	//dn = d; mag_d = vm_vec_normalize(&dn);
//$$
//$$	mag_w = vm_vec_copy_normalize(&wn,&w);
//$$	mag_d = vm_vec_copy_normalize(&dn,&d);
//$$
//$$	//vm_vec_cross(&c,&w,&d);
//$$	vm_vec_cross(&c,&wn,&dn);
//$$
//$$	mag_c = vm_vec_mag(&c);
//$$	//mag_d = vm_vec_mag(&d);
//$$
//$$	//dist = fixdiv(mag_c,mag_d);
//$$
//$$dist = fixmul(mag_c,mag_w);
//$$
//$$	if (dist < sphere_rad) {        //we intersect.  find point of intersection
//$$		fix int_dist;                   //length of vector to intersection point
//$$		fix k;                                  //portion of p0p1 we want
//$$//@@		fix dist2,rad2,shorten,mag_w2;
//$$
//$$//@@		mag_w2 = vm_vec_dot(&w,&w);     //the square of the magnitude
//$$//@@		//WHAT ABOUT OVERFLOW???
//$$//@@		dist2 = fixmul(dist,dist);
//$$//@@		rad2 = fixmul(sphere_rad,sphere_rad);
//$$//@@		shorten = fix_sqrt(rad2 - dist2);
//$$//@@		int_dist = fix_sqrt(mag_w2 - dist2) - shorten;
//$$
//$$		int_dist = get_sphere_int_dist(&w,dist,sphere_rad);
//$$
//$$if (labs(int_dist) > mag_d)	//I don't know why this would happen
//$$	if (int_dist > 0)
//$$		k = f1_0;
//$$	else
//$$		k = -f1_0;
//$$else
//$$		k = fixdiv(int_dist,mag_d);
//$$
//$$//		vm_vec_scale(&d,k);                     //vec from p0 to intersection point
//$$//		vm_vec_add(intp,p0,&d);         //intersection point
//$$		vm_vec_scale_add(intp,p0,&d,k);	//calc new intersection point
//$$
//$$		return int_dist;
//$$	}
//$$	else
//$$		return 0;       //no intersection
//$$}
*/

//determine if a vector intersects with an object
//if no intersects, returns 0, else fills in intp and returns dist
fix check_vector_to_object(vms_vector *intp,vms_vector *p0,vms_vector *p1,fix rad,object *obj,object *otherobj)
{
	fix size = obj->size;

	if (obj->type == OBJ_ROBOT && Robot_info[obj->id].attack_type)
		size = (size*3)/4;

	//if obj is player, and bumping into other player or a weapon of another coop player, reduce radius
	if (obj->type == OBJ_PLAYER &&
		 	((otherobj->type == OBJ_PLAYER) ||
	 		((Game_mode&GM_MULTI_COOP) && otherobj->type == OBJ_WEAPON && otherobj->ctype.laser_info.parent_type == OBJ_PLAYER)))
		size = size/2;

	return check_vector_to_sphere_1(intp,p0,p1,&obj->pos,size+rad);

}


#define MAX_SEGS_VISITED 100
int n_segs_visited;
short segs_visited[MAX_SEGS_VISITED];

int fvi_nest_count;

//these vars are used to pass vars from fvi_sub() to find_vector_intersection()
int fvi_hit_object;	// object number of object hit in last find_vector_intersection call.
int fvi_hit_seg;		// what segment the hit point is in
int fvi_hit_side;		// what side was hit
int fvi_hit_side_seg;// what seg the hitside is in
vms_vector wall_norm;	//ptr to surface normal of hit wall
int fvi_hit_seg2;		// what segment the hit point is in

int fvi_sub(vms_vector *intp,int *ints,vms_vector *p0,int startseg,vms_vector *p1,fix rad,short thisobjnum,int *ignore_obj_list,int flags,int *seglist,int *n_segs,int entry_seg);

//What the hell is fvi_hit_seg for???

//Find out if a vector intersects with anything.
//Fills in hit_data, an fvi_info structure (see header file).
//Parms:
//  p0 & startseg 	describe the start of the vector
//  p1 					the end of the vector
//  rad 					the radius of the cylinder
//  thisobjnum 		used to prevent an object with colliding with itself
//  ingore_obj			ignore collisions with this object
//  check_obj_flag	determines whether collisions with objects are checked
//Returns the hit_data->hit_type
int find_vector_intersection(fvi_query *fq,fvi_info *hit_data)
{
	int hit_type,hit_seg,hit_seg2;
	vms_vector hit_pnt;
	int i;

	Assert(fq->ignore_obj_list != (int *)(-1));
	Assert((fq->startseg <= Highest_segment_index) && (fq->startseg >= 0));

	fvi_hit_seg = -1;
	fvi_hit_side = -1;

	fvi_hit_object = -1;

	//check to make sure start point is in seg its supposed to be in
	//Assert(check_point_in_seg(p0,startseg,0).centermask==0);	//start point not in seg

	// Viewer is not in segment as claimed, so say there is no hit.
	if(!(get_seg_masks(fq->p0,fq->startseg,0).centermask==0)) {

		hit_data->hit_type = HIT_BAD_P0;
		hit_data->hit_pnt = *fq->p0;
		hit_data->hit_seg = fq->startseg;
		hit_data->hit_side = hit_data->hit_object = 0;
		hit_data->hit_side_seg = -1;

		return hit_data->hit_type;
	}

	segs_visited[0] = fq->startseg;

	n_segs_visited=1;

	fvi_nest_count = 0;

	hit_seg2 = fvi_hit_seg2 = -1;

	hit_type = fvi_sub(&hit_pnt,&hit_seg2,fq->p0,fq->startseg,fq->p1,fq->rad,fq->thisobjnum,fq->ignore_obj_list,fq->flags,hit_data->seglist,&hit_data->n_segs,-2);
	//!!hit_seg = find_point_seg(&hit_pnt,fq->startseg);
	if (hit_seg2!=-1 && !get_seg_masks(&hit_pnt,hit_seg2,0).centermask)
		hit_seg = hit_seg2;
	else
		hit_seg = find_point_seg(&hit_pnt,fq->startseg);

//MATT: TAKE OUT THIS HACK AND FIX THE BUGS!
	if (hit_type == HIT_WALL && hit_seg==-1)
		if (fvi_hit_seg2!=-1 && get_seg_masks(&hit_pnt,fvi_hit_seg2,0).centermask==0)
			hit_seg = fvi_hit_seg2;

	if (hit_seg == -1) {
		int new_hit_type;
		int new_hit_seg2=-1;
		vms_vector new_hit_pnt;

		//because of code that deal with object with non-zero radius has
		//problems, try using zero radius and see if we hit a wall

		new_hit_type = fvi_sub(&new_hit_pnt,&new_hit_seg2,fq->p0,fq->startseg,fq->p1,0,fq->thisobjnum,fq->ignore_obj_list,fq->flags,hit_data->seglist,&hit_data->n_segs,-2);

		if (new_hit_seg2 != -1) {
			hit_seg = new_hit_seg2;
			hit_pnt = new_hit_pnt;
		}
	}


if (hit_seg!=-1 && fq->flags&FQ_GET_SEGLIST)
	if (hit_seg != hit_data->seglist[hit_data->n_segs-1] && hit_data->n_segs<MAX_FVI_SEGS-1)
		hit_data->seglist[hit_data->n_segs++] = hit_seg;

if (hit_seg!=-1 && fq->flags&FQ_GET_SEGLIST)
	for (i=0;i<hit_data->n_segs && i<MAX_FVI_SEGS-1;i++)
		if (hit_data->seglist[i] == hit_seg) {
			hit_data->n_segs = i+1;
			break;
		}

//I'm sorry to say that sometimes the seglist isn't correct.  I did my
//best.  Really.


//{	//verify hit list
//
//	int i,ch;
//
//	Assert(hit_data->seglist[0] == startseg);
//
//	for (i=0;i<hit_data->n_segs-1;i++) {
//		for (ch=0;ch<6;ch++)
//			if (Segments[hit_data->seglist[i]].children[ch] == hit_data->seglist[i+1])
//				break;
//		Assert(ch<6);
//	}
//
//	Assert(hit_data->seglist[hit_data->n_segs-1] == hit_seg);
//}
	

//MATT: PUT THESE ASSERTS BACK IN AND FIX THE BUGS!
//!!	Assert(hit_seg!=-1);
//!!	Assert(!((hit_type==HIT_WALL) && (hit_seg == -1)));
	//When this assert happens, get Matt.  Matt:  Look at hit_seg2 &
	//fvi_hit_seg.  At least one of these should be set.  Why didn't
	//find_new_seg() find something?

//	Assert(fvi_hit_seg==-1 || fvi_hit_seg == hit_seg);

	Assert(!(hit_type==HIT_OBJECT && fvi_hit_object==-1));

	hit_data->hit_type		= hit_type;
	hit_data->hit_pnt 		= hit_pnt;
	hit_data->hit_seg 		= hit_seg;
	hit_data->hit_side 		= fvi_hit_side;	//looks at global
	hit_data->hit_side_seg	= fvi_hit_side_seg;	//looks at global
	hit_data->hit_object		= fvi_hit_object;	//looks at global
	hit_data->hit_wallnorm	= wall_norm;		//looks at global

//	if(hit_seg!=-1 && get_seg_masks(&hit_data->hit_pnt,hit_data->hit_seg,0).centermask!=0)
//		Int3();

	return hit_type;

}

//--unused-- fix check_dist(vms_vector *v0,vms_vector *v1)
//--unused-- {
//--unused-- 	return vm_vec_dist(v0,v1);
//--unused-- }

int obj_in_list(int objnum,int *obj_list)
{
	int t;

	while ((t=*obj_list)!=-1 && t!=objnum) obj_list++;

	return (t==objnum);

}

int check_trans_wall(vms_vector *pnt,segment *seg,int sidenum,int facenum);

int fvi_sub(vms_vector *intp,int *ints,vms_vector *p0,int startseg,vms_vector *p1,fix rad,short thisobjnum,int *ignore_obj_list,int flags,int *seglist,int *n_segs,int entry_seg)
{
	segment *seg;				//the segment we're looking at
	int startmask,endmask;	//mask of faces
	//@@int sidemask;				//mask of sides - can be on back of face but not side
	int centermask;			//where the center point is
	int objnum;
	segmasks masks;
	vms_vector hit_point,closest_hit_point; 	//where we hit
	fix d,closest_d=0x7fffffff;					//distance to hit point
	int hit_type=HIT_NONE;							//what sort of hit
	int hit_seg=-1;
	int hit_none_seg=-1;
	int hit_none_n_segs=0;
	int hit_none_seglist[MAX_FVI_SEGS];
	int cur_nest_level = fvi_nest_count;

	//fvi_hit_object = -1;

	if (flags&FQ_GET_SEGLIST)
		*seglist = startseg;
	*n_segs=1;

	seg = &Segments[startseg];

	fvi_nest_count++;

	//first, see if vector hit any objects in this segment
	if (flags & FQ_CHECK_OBJS)
		for (objnum=seg->objects;objnum!=-1;objnum=Objects[objnum].next)
			if (	!(Objects[objnum].flags & OF_SHOULD_BE_DEAD) &&
				 	!(thisobjnum == objnum ) &&
				 	(ignore_obj_list==NULL || !obj_in_list(objnum,ignore_obj_list)) &&
				 	!laser_are_related( objnum, thisobjnum ) &&
				 	!((thisobjnum  > -1)	&&
				  		(CollisionResult[Objects[thisobjnum].type][Objects[objnum].type] == RESULT_NOTHING ) &&
			 	 		(CollisionResult[Objects[objnum].type][Objects[thisobjnum].type] == RESULT_NOTHING ))) {
				int fudged_rad = rad;

				//	If this is a powerup, don't do collision if flag FQ_IGNORE_POWERUPS is set
				if (Objects[objnum].type == OBJ_POWERUP)
					if (flags & FQ_IGNORE_POWERUPS)
						continue;

				//	If this is a robot:robot collision, only do it if both of them have attack_type != 0 (eg, green guy)
				if (Objects[thisobjnum].type == OBJ_ROBOT)
					if (Objects[objnum].type == OBJ_ROBOT)
						// -- MK: 11/18/95, 4claws glomming together...this is easy.  -- if (!(Robot_info[Objects[objnum].id].attack_type && Robot_info[Objects[thisobjnum].id].attack_type))
							continue;

				if (Objects[thisobjnum].type == OBJ_ROBOT && Robot_info[Objects[thisobjnum].id].attack_type)
					fudged_rad = (rad*3)/4;

				//if obj is player, and bumping into other player or a weapon of another coop player, reduce radius
				if (Objects[thisobjnum].type == OBJ_PLAYER &&
						((Objects[objnum].type == OBJ_PLAYER) ||
						((Game_mode&GM_MULTI_COOP) &&  Objects[objnum].type == OBJ_WEAPON && Objects[objnum].ctype.laser_info.parent_type == OBJ_PLAYER)))
					fudged_rad = rad/2;	//(rad*3)/4;

				d = check_vector_to_object(&hit_point,p0,p1,fudged_rad,&Objects[objnum],&Objects[thisobjnum]);

				if (d)          //we have intersection
					if (d < closest_d) {
						fvi_hit_object = objnum;
						Assert(fvi_hit_object!=-1);
						closest_d = d;
						closest_hit_point = hit_point;
						hit_type=HIT_OBJECT;
					}
			}

	if (	(thisobjnum > -1 ) && (CollisionResult[Objects[thisobjnum].type][OBJ_WALL] == RESULT_NOTHING ) )
		rad = 0;		//HACK - ignore when edges hit walls

	//now, check segment walls

	startmask = get_seg_masks(p0,startseg,rad).facemask;

	masks = get_seg_masks(p1,startseg,rad);    //on back of which faces?
	endmask = masks.facemask;
	//@@sidemask = masks.sidemask;
	centermask = masks.centermask;

	if (centermask==0) hit_none_seg = startseg;

	if (endmask != 0) {                             //on the back of at least one face

		int side,bit,face;

		//for each face we are on the back of, check if intersected

		for (side=0,bit=1;side<6 && endmask>=bit;side++) {
			int num_faces;
			num_faces = get_num_faces(&seg->sides[side]);

			if (num_faces == 0)
				num_faces = 1;

			// commented out by mk on 02/13/94:: if ((num_faces=seg->sides[side].num_faces)==0) num_faces=1;

			for (face=0;face<2;face++,bit<<=1) {

				if (endmask & bit) {            //on the back of this face
					int face_hit_type;      //in what way did we hit the face?


					if (seg->children[side] == entry_seg)
						continue;		//don't go back through entry side

					//did we go through this wall/door?

					//#ifdef NEW_FVI_STUFF
					if (startmask & bit)		//start was also though.  Do extra check
						face_hit_type = special_check_line_to_face( &hit_point,
										p0,p1,seg,side,
										face,
										((num_faces==1)?4:3),rad);
					else
					//#endif
						//NOTE LINK TO ABOVE!!
						face_hit_type = check_line_to_face( &hit_point,
										p0,p1,seg,side,
										face,
										((num_faces==1)?4:3),rad);

	
					if (face_hit_type) {            //through this wall/door
						int wid_flag;

						//if what we have hit is a door, check the adjoining seg

						if ( (thisobjnum == Players[Player_num].objnum) && (Physics_cheat_flag==0xBADA55) )	{
							wid_flag = WALL_IS_DOORWAY(seg, side);
							if (seg->children[side] >= 0 )
 								wid_flag |= WID_FLY_FLAG;
						} else {
							wid_flag = WALL_IS_DOORWAY(seg, side);
						}

						if ((wid_flag & WID_FLY_FLAG) ||
							(((wid_flag & WID_RENDER_FLAG) && (wid_flag & WID_RENDPAST_FLAG)) &&
								((flags & FQ_TRANSWALL) || (flags & FQ_TRANSPOINT && check_trans_wall(&hit_point,seg,side,face))))) {

							int newsegnum;
							vms_vector sub_hit_point;
							int sub_hit_type,sub_hit_seg;
							vms_vector save_wall_norm = wall_norm;
							int save_hit_objnum=fvi_hit_object;
							int i;

							//do the check recursively on the next seg.

							newsegnum = seg->children[side];

							for (i=0;i<n_segs_visited && newsegnum!=segs_visited[i];i++);

							if (i==n_segs_visited) {                //haven't visited here yet
								int temp_seglist[MAX_FVI_SEGS],temp_n_segs;
								
								segs_visited[n_segs_visited++] = newsegnum;

								if (n_segs_visited >= MAX_SEGS_VISITED)
									goto quit_looking;		//we've looked a long time, so give up

								sub_hit_type = fvi_sub(&sub_hit_point,&sub_hit_seg,p0,newsegnum,p1,rad,thisobjnum,ignore_obj_list,flags,temp_seglist,&temp_n_segs,startseg);

								if (sub_hit_type != HIT_NONE) {

									d = vm_vec_dist(&sub_hit_point,p0);

									if (d < closest_d) {

										closest_d = d;
										closest_hit_point = sub_hit_point;
										hit_type = sub_hit_type;
										if (sub_hit_seg!=-1) hit_seg = sub_hit_seg;

										//copy seglist
										if (flags&FQ_GET_SEGLIST) {
											int ii;
											for (ii=0;i<temp_n_segs && *n_segs<MAX_FVI_SEGS-1;)
												seglist[(*n_segs)++] = temp_seglist[ii++];
										}

										Assert(*n_segs < MAX_FVI_SEGS);
									}
									else {
										wall_norm = save_wall_norm;     //global could be trashed
										fvi_hit_object = save_hit_objnum;
 									}

								}
								else {
									wall_norm = save_wall_norm;     //global could be trashed
									if (sub_hit_seg!=-1) hit_none_seg = sub_hit_seg;
									//copy seglist
									if (flags&FQ_GET_SEGLIST) {
										int ii;
										for (ii=0;ii<temp_n_segs && ii<MAX_FVI_SEGS-1;ii++)
											hit_none_seglist[ii] = temp_seglist[ii];
									}
									hit_none_n_segs = temp_n_segs;
								}
							}
						}
						else {          //a wall
																
								//is this the closest hit?
	
								d = vm_vec_dist(&hit_point,p0);
	
								if (d < closest_d) {
									closest_d = d;
									closest_hit_point = hit_point;
									hit_type = HIT_WALL;
									
									#ifdef COMPACT_SEGS
										get_side_normal(seg, side, face, &wall_norm );
									#else
										wall_norm = seg->sides[side].normals[face];	
									#endif
									
	
									if (get_seg_masks(&hit_point,startseg,rad).centermask==0)
										hit_seg = startseg;             //hit in this segment
									else
										fvi_hit_seg2 = startseg;

									//@@else	 {
									//@@	mprintf( 0, "Warning on line 991 in physics.c\n" );
									//@@	hit_seg = startseg;             //hit in this segment
									//@@	//Int3();
									//@@}

									fvi_hit_seg = hit_seg;
									fvi_hit_side =  side;
									fvi_hit_side_seg = startseg;

								}
						}
					}
				}
			}
		}
	}

//      Assert(centermask==0 || hit_seg!=startseg);

//      Assert(sidemask==0);            //Error("Didn't find side we went though");

quit_looking:
	;

	if (hit_type == HIT_NONE) {     //didn't hit anything, return end point
		int i;

		*intp = *p1;
		*ints = hit_none_seg;
		//MATT: MUST FIX THIS!!!!
		//Assert(!centermask);

		if (hit_none_seg!=-1) {			///(centermask == 0)
			if (flags&FQ_GET_SEGLIST)
				//copy seglist
				for (i=0;i<hit_none_n_segs && *n_segs<MAX_FVI_SEGS-1;)
					seglist[(*n_segs)++] = hit_none_seglist[i++];
		}
		else
			if (cur_nest_level!=0)
				*n_segs=0;

	}
	else {
		*intp = closest_hit_point;
		if (hit_seg==-1)
			if (fvi_hit_seg2 != -1)
				*ints = fvi_hit_seg2;
			else
				*ints = hit_none_seg;
		else
			*ints = hit_seg;
	}

	Assert(!(hit_type==HIT_OBJECT && fvi_hit_object==-1));

	return hit_type;

}

/*
//--unused-- //compute the magnitude of a 2d vector
//--unused-- fix mag2d(vec2d *v);
//--unused-- #pragma aux mag2d parm [esi] value [eax] modify exact [eax ebx ecx edx] = \
//--unused-- 	"mov	eax,[esi]"		\
//--unused-- 	"imul	eax"				\
//--unused-- 	"mov	ebx,eax"			\
//--unused-- 	"mov	ecx,edx"			\
//--unused-- 	"mov	eax,4[esi]"		\
//--unused-- 	"imul	eax"				\
//--unused-- 	"add	eax,ebx"			\
//--unused-- 	"adc	edx,ecx"			\
//--unused-- 	"call	quad_sqrt";
*/

//--unused-- //returns mag
//--unused-- fix normalize_2d(vec2d *v)
//--unused-- {
//--unused-- 	fix mag;
//--unused--
//--unused-- 	mag = mag2d(v);
//--unused--
//--unused-- 	v->i = fixdiv(v->i,mag);
//--unused-- 	v->j = fixdiv(v->j,mag);
//--unused--
//--unused-- 	return mag;
//--unused-- }

#include "textures.h"
#include "texmerge.h"

#define cross(v0,v1) (fixmul((v0)->i,(v1)->j) - fixmul((v0)->j,(v1)->i))

//finds the uv coords of the given point on the given seg & side
//fills in u & v. if l is non-NULL fills it in also
void find_hitpoint_uv(fix *u,fix *v,fix *l,vms_vector *pnt,segment *seg,int sidenum,int facenum)
{
	vms_vector_array *pnt_array;
	vms_vector_array normal_array;
	int segnum = seg-Segments;
	int num_faces;
	int biggest,ii,jj;
	side *side = &seg->sides[sidenum];
	int vertex_list[6],vertnum_list[6];
 	vec2d p1,vec0,vec1,checkp;	//@@,checkv;
	uvl uvls[3];
	fix k0,k1;
	int i;

	//mprintf(0,"\ncheck_trans_wall  vec=%x,%x,%x\n",pnt->x,pnt->y,pnt->z);

	//do lasers pass through illusory walls?

	//when do I return 0 & 1 for non-transparent walls?

	if (segnum < 0 || segnum > Highest_segment_index) {
		mprintf((0,"Bad segnum (%d) in find_hitpoint_uv()\n",segnum));
		*u = *v = 0;
		return;
	}

	if (segnum==-1)
		Error("segnum == -1 in find_hitpoint_uv()");

	create_abs_vertex_lists(&num_faces,vertex_list,segnum,sidenum);
	create_all_vertnum_lists(&num_faces,vertnum_list,segnum,sidenum);

	//now the hard work.

	//1. find what plane to project this wall onto to make it a 2d case

	#ifdef COMPACT_SEGS
		get_side_normal(seg, sidenum, facenum, (vms_vector *)&normal_array );
	#else
		memcpy( &normal_array, &side->normals[facenum], sizeof(vms_vector_array) );
	#endif
  	biggest = 0;

	if (abs(normal_array.xyz[1]) > abs(normal_array.xyz[biggest])) biggest = 1;
	if (abs(normal_array.xyz[2]) > abs(normal_array.xyz[biggest])) biggest = 2;

	if (biggest == 0) ii=1; else ii=0;
	if (biggest == 2) jj=1; else jj=2;

	//2. compute u,v of intersection point

	//vec from 1 -> 0
	pnt_array = (vms_vector_array *)&Vertices[vertex_list[facenum*3+1]];
	p1.i = pnt_array->xyz[ii];
	p1.j = pnt_array->xyz[jj];

	pnt_array = (vms_vector_array *)&Vertices[vertex_list[facenum*3+0]];
	vec0.i = pnt_array->xyz[ii] - p1.i;
	vec0.j = pnt_array->xyz[jj] - p1.j;

	//vec from 1 -> 2
	pnt_array = (vms_vector_array *)&Vertices[vertex_list[facenum*3+2]];
	vec1.i = pnt_array->xyz[ii] - p1.i;
	vec1.j = pnt_array->xyz[jj] - p1.j;

	//vec from 1 -> checkpoint
	pnt_array = (vms_vector_array *)pnt;
	checkp.i = pnt_array->xyz[ii];
	checkp.j = pnt_array->xyz[jj];

	//@@checkv.i = checkp.i - p1.i;
	//@@checkv.j = checkp.j - p1.j;

	//mprintf(0," vec0   = %x,%x  ",vec0.i,vec0.j);
	//mprintf(0," vec1   = %x,%x  ",vec1.i,vec1.j);
	//mprintf(0," checkv = %x,%x\n",checkv.i,checkv.j);

	k1 = -fixdiv(cross(&checkp,&vec0) + cross(&vec0,&p1),cross(&vec0,&vec1));
	if (abs(vec0.i) > abs(vec0.j))
		k0 = fixdiv(fixmul(-k1,vec1.i) + checkp.i - p1.i,vec0.i);
	else
		k0 = fixdiv(fixmul(-k1,vec1.j) + checkp.j - p1.j,vec0.j);

	//mprintf(0," k0,k1  = %x,%x\n",k0,k1);

	for (i=0;i<3;i++)
		uvls[i] = side->uvls[vertnum_list[facenum*3+i]];

	*u = uvls[1].u + fixmul( k0,uvls[0].u - uvls[1].u) + fixmul(k1,uvls[2].u - uvls[1].u);
	*v = uvls[1].v + fixmul( k0,uvls[0].v - uvls[1].v) + fixmul(k1,uvls[2].v - uvls[1].v);

	if (l)
		*l = uvls[1].l + fixmul( k0,uvls[0].l - uvls[1].l) + fixmul(k1,uvls[2].l - uvls[1].l);

	//mprintf(0," u,v    = %x,%x\n",*u,*v);
}

//check if a particular point on a wall is a transparent pixel
//returns 1 if can pass though the wall, else 0
int check_trans_wall(vms_vector *pnt,segment *seg,int sidenum,int facenum)
{
	grs_bitmap *bm;
	side *side = &seg->sides[sidenum];
	int bmx,bmy;
	fix u,v;

//	Assert(WALL_IS_DOORWAY(seg,sidenum) == WID_TRANSPARENT_WALL);

	find_hitpoint_uv(&u,&v,NULL,pnt,seg,sidenum,facenum);	//	Don't compute light value.

	if (side->tmap_num2 != 0)	{
		bm = texmerge_get_cached_bitmap( side->tmap_num, side->tmap_num2 );
	} else {
		bm = &GameBitmaps[Textures[side->tmap_num].index];
		PIGGY_PAGE_IN( Textures[side->tmap_num] );
	}

	if (bm->bm_flags & BM_FLAG_RLE)
		bm = rle_expand_texture(bm);

	bmx = ((unsigned) f2i(u*bm->bm_w)) % bm->bm_w;
	bmy = ((unsigned) f2i(v*bm->bm_h)) % bm->bm_h;

//note: the line above had -v, but that was wrong, so I changed it.  if
//something doesn't work, and you want to make it negative again, you
//should figure out what's going on.

	//mprintf(0," bmx,y  = %d,%d, color=%x\n",bmx,bmy,bm->bm_data[bmy*64+bmx]);

	return (bm->bm_data[bmy*bm->bm_w+bmx] == TRANSPARENCY_COLOR);
}

//new function for Mike
//note: n_segs_visited must be set to zero before this is called
int sphere_intersects_wall(vms_vector *pnt,int segnum,fix rad)
{
	int facemask;
	segment *seg;

	segs_visited[n_segs_visited++] = segnum;

	facemask = get_seg_masks(pnt,segnum,rad).facemask;

	seg = &Segments[segnum];

	if (facemask != 0) {				//on the back of at least one face

		int side,bit,face;

		//for each face we are on the back of, check if intersected

		for (side=0,bit=1;side<6 && facemask>=bit;side++) {

			for (face=0;face<2;face++,bit<<=1) {

				if (facemask & bit) {            //on the back of this face
					int face_hit_type;      //in what way did we hit the face?
					int num_faces,vertex_list[6];

					//did we go through this wall/door?

					if ((seg-Segments)==-1)
						Error("segnum == -1 in sphere_intersects_wall()");

					create_abs_vertex_lists(&num_faces,vertex_list,seg-Segments,side);

					face_hit_type = check_sphere_to_face( pnt,&seg->sides[side],
										face,((num_faces==1)?4:3),rad,vertex_list);

					if (face_hit_type) {            //through this wall/door
						int child,i;

						//if what we have hit is a door, check the adjoining seg

						child = seg->children[side];

						for (i=0;i<n_segs_visited && child!=segs_visited[i];i++);

						if (i==n_segs_visited) {                //haven't visited here yet

							if (!IS_CHILD(child))
								return 1;
							else {

								if (sphere_intersects_wall(pnt,child,rad))
									return 1;
							}
						}
					}
				}
			}
		}
	}

	return 0;
}

//Returns true if the object is through any walls
int object_intersects_wall(object *objp)
{
	n_segs_visited = 0;

	return sphere_intersects_wall(&objp->pos,objp->segnum,objp->size);
}


