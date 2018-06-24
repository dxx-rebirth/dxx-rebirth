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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * New home for find_vector_intersection()
 *
 */

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pstypes.h"
#include "u_mem.h"
#include "dxxerror.h"
#include "inferno.h"
#include "fvi.h"
#include "segment.h"
#include "object.h"
#include "wall.h"
#include "laser.h"
#include "gameseg.h"
#include "rle.h"
#include "robot.h"
#include "piggy.h"
#include "player.h"
#include "compiler-range_for.h"
#include "segiter.h"

using std::min;

#define face_type_num(nfaces,face_num,tri_edge) ((nfaces==1)?0:(tri_edge*2 + face_num))

//find the point on the specified plane where the line intersects
//returns true if point found, false if line parallel to plane
//new_pnt is the found point on the plane
//plane_pnt & plane_norm describe the plane
//p0 & p1 are the ends of the line
__attribute_warn_unused_result
static int find_plane_line_intersection(vms_vector &new_pnt,const vms_vector &plane_pnt,const vms_vector &plane_norm,const vms_vector &p0,const vms_vector &p1,fix rad)
{
	auto d = vm_vec_sub(p1,p0);
	const fix den = -vm_vec_dot(plane_norm,d);
	if (unlikely(!den)) // moving parallel to wall, so can't hit it
		return 0;

	const auto w = vm_vec_sub(p0,plane_pnt);
	fix num = vm_vec_dot(plane_norm,w) - rad; //move point out by rad

	//check for various bad values
	if (den > 0 && (-num>>15) >= den) //will overflow (large negative)
		num = (f1_0-f0_5)*den;
	if (den > 0 && num > den) //frac greater than one
		return 0;
	if (den < 0 && num < den) //frac greater than one
		return 0;
	if (labs (num) / (f1_0 / 2) >= labs (den))
		return 0;

	vm_vec_scale2(d,num,den);
	vm_vec_add(new_pnt,p0,d);

	return 1;

}

namespace {

struct vec2d {
	fix i,j;
};

//intersection types
#define IT_NONE 0       //doesn't touch face at all
#define IT_FACE 1       //touches face
#define IT_EDGE 2       //touches edge of face
#define IT_POINT        3       //touches vertex

struct ij_pair
{
	fix vms_vector::*largest_normal;
	fix vms_vector::*i;
	fix vms_vector::*j;
};

}

__attribute_warn_unused_result
static ij_pair find_largest_normal(vms_vector t)
{
	t.x = labs(t.x);
	t.y = labs(t.y);
	t.z = labs(t.z);
	if (t.x > t.y)
	{
		if (t.x > t.z)
			return {&vms_vector::x, &vms_vector::z, &vms_vector::y};
	}
	else if (t.y > t.z)
		return {&vms_vector::y, &vms_vector::x, &vms_vector::z};
	return {&vms_vector::z, &vms_vector::y, &vms_vector::x};
}

//see if a point in inside a face by projecting into 2d
__attribute_warn_unused_result
static unsigned check_point_to_face(const vms_vector &checkp, const vms_vector &norm, const unsigned facenum, const unsigned nv, const vertex_array_list_t &vertex_list)
{
///
	int edge;
	uint edgemask;
	fix check_i,check_j;
	//now do 2d check to see if point is in side

	//project polygon onto plane by finding largest component of normal
	ij_pair ij = find_largest_normal(norm);
	if (norm.*ij.largest_normal <= 0)
	{
		using std::swap;
		swap(ij.i, ij.j);
	}

	//now do the 2d problem in the i,j plane

	check_i = checkp.*ij.i;
	check_j = checkp.*ij.j;

	for (edge=edgemask=0;edge<nv;edge++) {
		vec2d edgevec,checkvec;
		fix64 d;

		auto &v0 = *vcvertptr(vertex_list[facenum * 3 + edge]);
		auto &v1 = *vcvertptr(vertex_list[facenum * 3 + ((edge + 1) % nv)]);

		edgevec.i = v1.*ij.i - v0.*ij.i;
		edgevec.j = v1.*ij.j - v0.*ij.j;

		checkvec.i = check_i - v0.*ij.i;
		checkvec.j = check_j - v0.*ij.j;

		d = fixmul64(checkvec.i,edgevec.j) - fixmul64(checkvec.j,edgevec.i);

		if (d < 0)              		//we are outside of triangle
			edgemask |= (1<<edge);
	}

	return edgemask;

}

//check if a sphere intersects a face
__attribute_warn_unused_result
static int check_sphere_to_face(const vms_vector &pnt, const vms_vector &normal, const unsigned facenum, const unsigned nv, const fix rad, const vertex_array_list_t &vertex_list)
{
	const auto checkp = pnt;
	uint edgemask;

	//now do 2d check to see if point is in side

	edgemask = check_point_to_face(pnt, normal, facenum, nv, vertex_list);

	//we've gone through all the sides, are we inside?

	if (edgemask == 0)
		return IT_FACE;
	else {
		vms_vector edgevec;            //this time, real 3d vectors
		vms_vector closest_point;
		int itype;
		int edgenum;

		//get verts for edge we're behind

		for (edgenum=0;!(edgemask&1);(edgemask>>=1),edgenum++);

		auto &v0 = *vcvertptr(vertex_list[facenum * 3 + edgenum]);
		auto &v1 = *vcvertptr(vertex_list[facenum * 3 + ((edgenum + 1) % nv)]);

		//check if we are touching an edge or point

		const auto checkvec = vm_vec_sub(checkp,v0);
		const auto edgelen = vm_vec_normalized_dir(edgevec,v1,v0);
		
		//find point dist from planes of ends of edge

		const auto d = vm_vec_dot(edgevec,checkvec);
		if (d < 0)
			return IT_NONE;
		else if (d > edgelen)
			return IT_NONE;

		if (d+rad < 0) return IT_NONE;                  //too far behind start point

		if (d-rad > edgelen) return IT_NONE;    //too far part end point

		//find closest point on edge to check point

		else {
			itype = IT_EDGE;

			//vm_vec_scale(&edgevec,d);
			//vm_vec_add(&closest_point,v0,&edgevec);

			vm_vec_scale_add(closest_point,v0,edgevec,d);
		}

		const auto dist = vm_vec_dist2(checkp,closest_point);
		const fix64 rad64 = rad;
		if (dist > vm_distance_squared{rad64 * rad64})
			return IT_NONE;
		return itype;
	}


}

//returns true if line intersects with face. fills in newp with intersection
//point on plane, whether or not line intersects side
//facenum determines which of four possible faces we have
//note: the seg parm is temporary, until the face itself has a point field
__attribute_warn_unused_result
static int check_line_to_face(vms_vector &newp,const vms_vector &p0,const vms_vector &p1,const vcsegptridx_t seg,int side,int facenum,int nv,fix rad)
{
	auto &s = seg->sides[side];
	const vms_vector &norm = s.normals[facenum];

	const auto v = create_abs_vertex_lists(seg, s, side);
	const auto &num_faces = v.first;
	const auto &vertex_list = v.second;

	//use lowest point number
	unsigned vertnum;
	if (num_faces==2) {
		vertnum = min(vertex_list[0],vertex_list[2]);
	}
	else {
		auto b = begin(vertex_list);
		vertnum = *std::min_element(b, std::next(b, 4));
	}

	auto pli = find_plane_line_intersection(newp, vcvertptr(vertnum), norm, p0, p1, rad);

	if (!pli) return IT_NONE;

	auto checkp = newp;

	//if rad != 0, project the point down onto the plane of the polygon

	if (rad!=0)
		vm_vec_scale_add2(checkp,norm,-rad);

	return check_sphere_to_face(checkp, s.normals[facenum], facenum, nv, rad, vertex_list);
}

//returns the value of a determinant
__attribute_warn_unused_result
static fix calc_det_value(const vms_matrix *det)
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
static int check_line_to_line(fix *t1,fix *t2,const vms_vector &p1,const vms_vector &v1,const vms_vector &p2,const vms_vector &v2)
{
	vms_matrix det;
	fix d,cross_mag2;		//mag squared cross product

	vm_vec_cross(det.fvec,v1,v2);
	cross_mag2 = vm_vec_dot(det.fvec,det.fvec);

	if (cross_mag2 == 0)
		return 0;			//lines are parallel

	vm_vec_sub(det.rvec,p2,p1);
	det.uvec = v2;
	d = calc_det_value(&det);
	*t1 = fixdiv(d,cross_mag2);

	det.uvec = v1;
	d = calc_det_value(&det);
	*t2 = fixdiv(d,cross_mag2);

	return 1;		//found point
}

//this version is for when the start and end positions both poke through
//the plane of a side.  In this case, we must do checks against the edge
//of faces
__attribute_warn_unused_result
static int special_check_line_to_face(vms_vector &newp,const vms_vector &p0,const vms_vector &p1,const vcsegptridx_t seg,int side,int facenum,int nv,fix rad)
{
	fix edge_t=0,move_t=0,edge_t2=0,move_t2=0;
	int edgenum;
	auto &s = seg->sides[side];

	//calc some basic stuff

	const auto v = create_abs_vertex_lists(seg, s, side);
	const auto &vertex_list = v.second;
	auto move_vec = vm_vec_sub(p1,p0);

	//figure out which edge(s) to check against

	unsigned edgemask = check_point_to_face(p0, s.normals[facenum], facenum, nv, vertex_list);

	if (edgemask == 0)
		return check_line_to_face(newp,p0,p1,seg,side,facenum,nv,rad);

	for (edgenum=0;!(edgemask&1);edgemask>>=1,edgenum++);

	auto &edge_v0 = *vcvertptr(vertex_list[facenum * 3 + edgenum]);
	auto &edge_v1 = *vcvertptr(vertex_list[facenum * 3 + ((edgenum + 1) % nv)]);

	auto edge_vec = vm_vec_sub(edge_v1,edge_v0);

	//is the start point already touching the edge?

	//??

	//first, find point of closest approach of vec & edge

	const auto edge_len = vm_vec_normalize(edge_vec);
	const auto move_len = vm_vec_normalize(move_vec);

	check_line_to_line(&edge_t,&move_t,edge_v0,edge_vec,p0,move_vec);

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

	const auto closest_point_edge = vm_vec_scale_add(edge_v0,edge_vec,edge_t2);
	const auto closest_point_move = vm_vec_scale_add(p0,move_vec,move_t2);

	//find dist between closest points

	const auto closest_dist = vm_vec_dist2(closest_point_edge,closest_point_move);

	//could we hit with this dist?

	//note massive tolerance here
	const vm_distance fudge_rad{(rad * 15) / 20};
	if (closest_dist.d2 < fudge_rad || closest_dist < fudge_rad * fudge_rad)		//we hit.  figure out where
	{

		//now figure out where we hit

		vm_vec_scale_add(newp,p0,move_vec,move_t-rad);

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
__attribute_warn_unused_result
static vm_distance_squared check_vector_to_sphere_1(vms_vector &intp,const vms_vector &p0,const vms_vector &p1,const vms_vector &sphere_pos,fix sphere_rad)
{
	vms_vector dn;

	//this routine could be optimized if it's taking too much time!

	const auto d = vm_vec_sub(p1,p0);
	const auto w = vm_vec_sub(sphere_pos,p0);

	const auto mag_d = vm_vec_copy_normalize(dn,d);

	if (mag_d == 0) {
		const auto int_dist = vm_vec_mag2(w);
		intp = p0;
		if (int_dist.d2 < sphere_rad)
			return int_dist;
		const fix64 sphere_rad64 = sphere_rad;
		if (int_dist < vm_distance_squared{sphere_rad64 * sphere_rad64})
			return int_dist;
		return vm_distance_squared::minimum_value();
	}

	const fix w_dist = vm_vec_dot(dn,w);

	if (w_dist < 0)		//moving away from object
		return vm_distance_squared::minimum_value();

	if (w_dist > mag_d+sphere_rad)
		return vm_distance_squared::minimum_value();		//cannot hit

	const auto closest_point = vm_vec_scale_add(p0,dn,w_dist);

	const auto dist2 = vm_vec_dist2(closest_point,sphere_pos);
	const fix64 sphere_rad64 = sphere_rad;
	const vm_distance_squared sphere_rad_squared{sphere_rad64 * sphere_rad64};
	if (dist2 < sphere_rad_squared)
	{
		const fix64 delta_squared = static_cast<fix64>(sphere_rad_squared) - static_cast<fix64>(dist2);
		const fix delta = static_cast<fix>(delta_squared >> 16);
		const auto shorten = fix_sqrt(delta);
		const auto int_dist = w_dist-shorten;

		if (int_dist > mag_d || int_dist < 0) //past one or the other end of vector, which means we're inside
		{
			//past one or the other end of vector, which means we're inside? WRONG! Either you're inside OR you didn't quite make it!
			if (vm_vec_dist2(p0, sphere_pos) < sphere_rad_squared)
			{
				intp = p0; //don't move at all
				return vm_distance_squared{static_cast<fix64>(1)}; // note that we do not calculate a valid collision point. This is up to collision handling.
			} else {
				return vm_distance_squared::minimum_value();
			}
		}

		vm_vec_scale_add(intp,p0,dn,int_dist);         //calc intersection point
		return vm_distance_squared{static_cast<fix64>(int_dist) * int_dist};
	}
	else
		return vm_distance_squared::minimum_value();
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
__attribute_warn_unused_result
static vm_distance_squared check_vector_to_object(vms_vector &intp, const vms_vector &p0, const vms_vector &p1, const fix rad, const object_base &obj, const object &otherobj)
{
	fix size = obj.size;

	if (obj.type == OBJ_ROBOT && Robot_info[get_robot_id(obj)].attack_type)
		size = (size*3)/4;

	//if obj is player, and bumping into other player or a weapon of another coop player, reduce radius
	if (obj.type == OBJ_PLAYER &&
		 	(otherobj.type == OBJ_PLAYER ||
	 		((Game_mode & GM_MULTI_COOP) && otherobj.type == OBJ_WEAPON && otherobj.ctype.laser_info.parent_type == OBJ_PLAYER)))
		size = size/2;

	return check_vector_to_sphere_1(intp, p0, p1, obj.pos, size+rad);
}


namespace {

#define MAX_SEGS_VISITED 100
struct fvi_segment_visit_count_t
{
	unsigned count = 0;
};

struct fvi_segments_visited_t : public fvi_segment_visit_count_t, public visited_segment_bitarray_t
{
};

//these vars are used to pass vars from fvi_sub() to find_vector_intersection()

}

namespace dsx {
static int fvi_sub(vms_vector &intp, segnum_t &ints, const vms_vector &p0, const vcsegptridx_t startseg, const vms_vector &p1, fix rad, const icobjptridx_t thisobjnum, const std::pair<const objnum_t *, const objnum_t *> ignore_obj_list, int flags, fvi_info::segment_array_t &seglist, segnum_t entry_seg, fvi_segments_visited_t &visited, unsigned &fvi_hit_side, icsegidx_t &fvi_hit_side_seg, unsigned &fvi_nest_count, icsegidx_t &fvi_hit_pt_seg, const vms_vector *&wall_norm, icobjidx_t &fvi_hit_object);

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
}
int find_vector_intersection(const fvi_query &fq, fvi_info &hit_data)
{
	int hit_type;
	segnum_t hit_seg2;
	vms_vector hit_pnt;

	icobjidx_t fvi_hit_object = object_none;	// object number of object hit in last find_vector_intersection call.

	//check to make sure start point is in seg its supposed to be in
	//Assert(check_point_in_seg(p0,startseg,0).centermask==0);	//start point not in seg

	// invalid segnum, so say there is no hit.
	if(fq.startseg > Highest_segment_index)
	{
		Assert(fq.startseg <= Highest_segment_index);
		hit_data.hit_type = HIT_BAD_P0;
		hit_data.hit_pnt = *fq.p0;
		hit_data.hit_seg = hit_data.hit_side = hit_data.hit_object = 0;
		hit_data.hit_side_seg = segment_none;

		return hit_data.hit_type;
	}

	// Viewer is not in segment as claimed, so say there is no hit.
	if(!(get_seg_masks(vcvertptr, *fq.p0, vcsegptr(fq.startseg), 0).centermask == 0))
	{

		hit_data.hit_type = HIT_BAD_P0;
		hit_data.hit_pnt = *fq.p0;
		hit_data.hit_seg = fq.startseg;
		hit_data.hit_side = hit_data.hit_object = 0;
		hit_data.hit_side_seg = segment_none;

		return hit_data.hit_type;
	}

	fvi_segments_visited_t visited;
	visited[fq.startseg] = true;

	unsigned fvi_hit_side = ~0u;
	icsegidx_t fvi_hit_side_seg = segment_none;	// what seg the hitside is in
	unsigned fvi_nest_count = 0;

	icsegidx_t fvi_hit_pt_seg = segment_none;		// what segment the hit point is in
	hit_seg2 = segment_none;

	const vms_vector *wall_norm = nullptr;	//surface normal of hit wall
	hit_type = fvi_sub(hit_pnt, hit_seg2, *fq.p0, vcsegptridx(fq.startseg), *fq.p1, fq.rad, imobjptridx(fq.thisobjnum), fq.ignore_obj_list, fq.flags, hit_data.seglist, segment_exit, visited, fvi_hit_side, fvi_hit_side_seg, fvi_nest_count, fvi_hit_pt_seg, wall_norm, fvi_hit_object);
	segnum_t hit_seg;
	if (hit_seg2 != segment_none && !get_seg_masks(vcvertptr, hit_pnt, vcsegptr(hit_seg2), 0).centermask)
		hit_seg = hit_seg2;
	else
		hit_seg = find_point_seg(hit_pnt, imsegptridx(fq.startseg));

//MATT: TAKE OUT THIS HACK AND FIX THE BUGS!
	if (hit_type == HIT_WALL && hit_seg==segment_none)
		if (fvi_hit_pt_seg != segment_none && get_seg_masks(vcvertptr, hit_pnt, vcsegptr(fvi_hit_pt_seg), 0).centermask == 0)
			hit_seg = fvi_hit_pt_seg;

	if (hit_seg == segment_none) {
		int new_hit_type;
		segnum_t new_hit_seg2=segment_none;
		vms_vector new_hit_pnt;

		//because of code that deal with object with non-zero radius has
		//problems, try using zero radius and see if we hit a wall

		new_hit_type = fvi_sub(new_hit_pnt, new_hit_seg2, *fq.p0, vcsegptridx(fq.startseg), *fq.p1, 0, imobjptridx(fq.thisobjnum), fq.ignore_obj_list, fq.flags, hit_data.seglist, segment_exit, visited, fvi_hit_side, fvi_hit_side_seg, fvi_nest_count, fvi_hit_pt_seg, wall_norm, fvi_hit_object);
		(void)new_hit_type; // FIXME! This should become hit_type, right?

		if (new_hit_seg2 != segment_none) {
			hit_seg = new_hit_seg2;
			hit_pnt = new_hit_pnt;
		}
	}


	if (hit_seg!=segment_none && (fq.flags & FQ_GET_SEGLIST))
	{
		fvi_info::segment_array_t::iterator i = hit_data.seglist.find(hit_seg), e = hit_data.seglist.end();
		if (i != e)
			hit_data.seglist.erase(++i);
		else if (hit_data.seglist.size() < hit_data.seglist.max_size())
			hit_data.seglist.emplace_back(hit_seg);
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

	Assert(!(hit_type==HIT_OBJECT && fvi_hit_object==object_none));

	hit_data.hit_type		= hit_type;
	hit_data.hit_pnt 		= hit_pnt;
	hit_data.hit_seg 		= hit_seg;
	hit_data.hit_side 		= fvi_hit_side;	//looks at global
	hit_data.hit_side_seg	= fvi_hit_side_seg;	//looks at global
	hit_data.hit_object		= fvi_hit_object;	//looks at global
	if (wall_norm)
		hit_data.hit_wallnorm = *wall_norm;
	else
	{
		hit_data.hit_wallnorm = {};
		DXX_MAKE_VAR_UNDEFINED(hit_data.hit_wallnorm);
	}

//	if(hit_seg != -1 && get_seg_masks(&hit_data->hit_pnt, hit_data->hit_seg, 0, __FILE__, __LINE__).centermask != 0)
//		Int3();

	return hit_type;

}

__attribute_warn_unused_result
static bool obj_in_list(objnum_t objnum,const std::pair<const objnum_t *, const objnum_t *> obj_list)
{
	if (unlikely(!obj_list.first))
		return false;
	return std::find(obj_list.first, obj_list.second, objnum) != obj_list.second;
}

namespace dsx {
static int check_trans_wall(const vms_vector &pnt,const vcsegptridx_t seg,int sidenum,int facenum);
}

static void append_segments(fvi_info::segment_array_t &dst, const fvi_info::segment_array_t &src)
{
	/* Avoid overflow.  Original code had n_segs < MAX_SEGS_VISITED-1,
	 * so leave an extra slot on min.
	 */
	const size_t scount = src.size(), dcount = dst.size(), count = std::min(scount, dst.max_size() - dcount - 1);
	std::copy(src.begin(), src.begin() + count, std::back_inserter(dst));
}

namespace dsx {
static int fvi_sub(vms_vector &intp, segnum_t &ints, const vms_vector &p0, const vcsegptridx_t startseg, const vms_vector &p1, fix rad, icobjptridx_t thisobjnum, const std::pair<const objnum_t *, const objnum_t *> ignore_obj_list, int flags, fvi_info::segment_array_t &seglist, segnum_t entry_seg, fvi_segments_visited_t &visited, unsigned &fvi_hit_side, icsegidx_t &fvi_hit_side_seg, unsigned &fvi_nest_count, icsegidx_t &fvi_hit_pt_seg, const vms_vector *&wall_norm, icobjidx_t &fvi_hit_object)
{
	int startmask,endmask;	//mask of faces
	//@@int sidemask;				//mask of sides - can be on back of face but not side
	int centermask;			//where the center point is
	vms_vector closest_hit_point{}; 	//where we hit
	auto closest_d = vm_distance_squared::maximum_value();					//distance to hit point
	int hit_type=HIT_NONE;							//what sort of hit
	segnum_t hit_seg=segment_none;
	segnum_t hit_none_seg=segment_none;
	fvi_info::segment_array_t hit_none_seglist;

	seglist.clear();
	if (flags&FQ_GET_SEGLIST)
		seglist.emplace_back(startseg);

	auto &seg = startseg;				//the segment we're looking at

	const unsigned cur_nest_level = fvi_nest_count;
	fvi_nest_count++;

	//first, see if vector hit any objects in this segment
	if (flags & FQ_CHECK_OBJS)
	{
		const auto &collision = CollisionResult[likely(thisobjnum != object_none) ? thisobjnum->type : 0];
		range_for (const auto objnum, objects_in(*seg, vcobjptridx, vcsegptr))
		{
			if (objnum->flags & OF_SHOULD_BE_DEAD)
				continue;
			if (thisobjnum != object_none)
			{
				if (thisobjnum == objnum)
					continue;
				if (laser_are_related(objnum, thisobjnum))
					continue;
				if (collision[objnum->type] == RESULT_NOTHING)
					continue;
			}
			if (obj_in_list(objnum, ignore_obj_list))
				continue;
			int fudged_rad = rad;

#if defined(DXX_BUILD_DESCENT_II)
			//	If this is a powerup, don't do collision if flag FQ_IGNORE_POWERUPS is set
			if (objnum->type == OBJ_POWERUP)
				if (flags & FQ_IGNORE_POWERUPS)
					continue;
#endif

			//	If this is a robot:robot collision, only do it if both of them have attack_type != 0 (eg, green guy)
			const auto &thisobjp = thisobjnum;
			if (thisobjp->type == OBJ_ROBOT)
				if (objnum->type == OBJ_ROBOT)
#if defined(DXX_BUILD_DESCENT_I)
					if (!(Robot_info[get_robot_id(objnum)].attack_type && Robot_info[get_robot_id(thisobjp)].attack_type))
#endif
					// -- MK: 11/18/95, 4claws glomming together...this is easy.  -- if (!(Robot_info[Objects[objnum].id].attack_type && Robot_info[Objects[thisobjnum].id].attack_type))
						continue;

			if (thisobjp->type == OBJ_ROBOT && Robot_info[get_robot_id(thisobjp)].attack_type)
				fudged_rad = (rad*3)/4;

			//if obj is player, and bumping into other player or a weapon of another coop player, reduce radius
			if (thisobjp->type == OBJ_PLAYER &&
					((objnum->type == OBJ_PLAYER) ||
					((Game_mode&GM_MULTI_COOP) &&  objnum->type == OBJ_WEAPON && objnum->ctype.laser_info.parent_type == OBJ_PLAYER)))
				fudged_rad = rad/2;	//(rad*3)/4;

			vms_vector hit_point;
			const auto &&d = check_vector_to_object(hit_point,p0,p1,fudged_rad,objnum, thisobjp);

			if (d)          //we have intersection
				if (d < closest_d) {
					fvi_hit_object = objnum;
					Assert(fvi_hit_object!=object_none);
					closest_d = d;
					closest_hit_point = hit_point;
					hit_type=HIT_OBJECT;
				}
		}
	}

	if (thisobjnum != object_none && CollisionResult[thisobjnum->type][OBJ_WALL] == RESULT_NOTHING)
		rad = 0;		//HACK - ignore when edges hit walls

	//now, check segment walls

	startmask = get_seg_masks(vcvertptr, p0, startseg, rad).facemask;

	const auto &&masks = get_seg_masks(vcvertptr, p1, startseg, rad);    //on back of which faces?
	endmask = masks.facemask;
	//@@sidemask = masks.sidemask;
	centermask = masks.centermask;

	if (centermask==0) hit_none_seg = startseg;

	if (endmask != 0) {                             //on the back of at least one face

		int side,bit,face;

		//for each face we are on the back of, check if intersected

		for (side=0,bit=1;side<6 && endmask>=bit;side++) {
			const unsigned nv = get_side_is_quad(seg->sides[side]) ? 4 : 3;
			// commented out by mk on 02/13/94:: if ((num_faces=seg->sides[side].num_faces)==0) num_faces=1;

			for (face=0;face<2;face++,bit<<=1) {

				if (endmask & bit) {            //on the back of this face
					int face_hit_type;      //in what way did we hit the face?


					if (seg->children[side] == entry_seg)
						continue;		//don't go back through entry side

					//did we go through this wall/door?

					vms_vector hit_point;
					if (startmask & bit)		//start was also though.  Do extra check
						face_hit_type = special_check_line_to_face(hit_point,
										p0,p1,seg,side,
										face,
										nv,rad);
					else
						//NOTE LINK TO ABOVE!!
						face_hit_type = check_line_to_face(hit_point,
										p0,p1,seg,side,
										face,
										nv,rad);

	
					if (face_hit_type) {            //through this wall/door
						auto wid_flag = WALL_IS_DOORWAY(GameBitmaps, Textures, vcwallptr, seg, seg, side);

						//if what we have hit is a door, check the adjoining seg

						if (thisobjnum == get_local_player().objnum && cheats.ghostphysics)
						{
							if (IS_CHILD(seg->children[side]))
 								wid_flag |= WID_FLY_FLAG;
						}

						if ((wid_flag & WID_FLY_FLAG) ||
							(
#if defined(DXX_BUILD_DESCENT_I)
								(wid_flag == WID_TRANSPARENT_WALL) && 
#elif defined(DXX_BUILD_DESCENT_II)
								((wid_flag & WID_RENDER_FLAG) && (wid_flag & WID_RENDPAST_FLAG)) &&
#endif
								((flags & FQ_TRANSWALL) || (flags & FQ_TRANSPOINT && check_trans_wall(hit_point,seg,side,face))))) {

							segnum_t newsegnum,sub_hit_seg;
							vms_vector sub_hit_point;
							int sub_hit_type;
							const auto save_wall_norm = wall_norm;
							auto save_hit_objnum = fvi_hit_object;

							//do the check recursively on the next seg.

							newsegnum = seg->children[side];

							if (!visited[newsegnum]) {                //haven't visited here yet
								visited[newsegnum] = true;
								++ visited.count;

								if (visited.count >= MAX_SEGS_VISITED)
									goto quit_looking;		//we've looked a long time, so give up

								fvi_info::segment_array_t temp_seglist;
								sub_hit_type = fvi_sub(sub_hit_point, sub_hit_seg, p0, startseg.absolute_sibling(newsegnum), p1, rad, thisobjnum, ignore_obj_list, flags, temp_seglist, startseg, visited, fvi_hit_side, fvi_hit_side_seg, fvi_nest_count, fvi_hit_pt_seg, wall_norm, fvi_hit_object);

								if (sub_hit_type != HIT_NONE) {

									const auto d = vm_vec_dist2(sub_hit_point,p0);

									if (d < closest_d) {

										closest_d = d;
										closest_hit_point = sub_hit_point;
										hit_type = sub_hit_type;
										if (sub_hit_seg!=segment_none) hit_seg = sub_hit_seg;

										//copy seglist
										if (flags&FQ_GET_SEGLIST) {
											append_segments(seglist, temp_seglist);
										}
									}
									else {
										wall_norm = save_wall_norm;     //global could be trashed
										fvi_hit_object = save_hit_objnum;
 									}

								}
								else {
									wall_norm = save_wall_norm;     //global could be trashed
									if (sub_hit_seg!=segment_none) hit_none_seg = sub_hit_seg;
									//copy seglist
									if (flags&FQ_GET_SEGLIST) {
										hit_none_seglist = temp_seglist;
									}
								}
							}
						}
						else {          //a wall
																
								//is this the closest hit?
	
								const auto d = vm_vec_dist2(hit_point,p0);
	
								if (d < closest_d) {
									closest_d = d;
									closest_hit_point = hit_point;
									hit_type = HIT_WALL;
									wall_norm = &seg->sides[side].normals[face];
									if (get_seg_masks(vcvertptr, hit_point, startseg, rad).centermask == 0)
										hit_seg = startseg;             //hit in this segment
									else
										fvi_hit_pt_seg = startseg;

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
		intp = p1;
		ints = hit_none_seg;
		//MATT: MUST FIX THIS!!!!
		//Assert(!centermask);

		if (hit_none_seg!=segment_none) {			///(centermask == 0)
			if (flags&FQ_GET_SEGLIST)
				//copy seglist
				append_segments(seglist, hit_none_seglist);
		}
		else
			if (cur_nest_level!=0)
				seglist.clear();

	}
	else {
		intp = closest_hit_point;
		if (hit_seg==segment_none)
			if (fvi_hit_pt_seg != segment_none)
				ints = fvi_hit_pt_seg;
			else
				ints = hit_none_seg;
		else
			ints = hit_seg;
	}

	Assert(!(hit_type==HIT_OBJECT && fvi_hit_object==object_none));

	return hit_type;

}
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
namespace dsx {
fvi_hitpoint find_hitpoint_uv(const vms_vector &pnt, const vcsegptridx_t seg, const uint_fast32_t sidenum, const uint_fast32_t facenum)
{
	auto &side = seg->sides[sidenum];
	fix k0,k1;
	int i;

	//do lasers pass through illusory walls?

	//when do I return 0 & 1 for non-transparent walls?

	const auto &&vn = create_all_vertnum_lists(seg, side, sidenum);

	//now the hard work.

	//1. find what plane to project this wall onto to make it a 2d case

	const auto &normal_array = side.normals[facenum];
	auto fmax = [](const vms_vector &v, fix vms_vector::*a, fix vms_vector::*b) {
		return abs(v.*a) > abs(v.*b) ? a : b;
	};
	const auto biggest = fmax(normal_array, &vms_vector::z, fmax(normal_array, &vms_vector::y, &vms_vector::x));
	const auto ii = (biggest == &vms_vector::x) ? &vms_vector::y : &vms_vector::x;
	const auto jj = (biggest == &vms_vector::z) ? &vms_vector::y : &vms_vector::z;

	//2. compute u,v of intersection point

	//vec from 1 -> 0
	auto &vf1 = *vcvertptr(vn[facenum * 3 + 1].vertex);
	const vec2d p1{vf1.*ii, vf1.*jj};

	auto &vf0 = *vcvertptr(vn[facenum * 3 + 0].vertex);
	const vec2d vec0{vf0.*ii - p1.i, vf0.*jj - p1.j};
	//vec from 1 -> 2
	auto &vf2 = *vcvertptr(vn[facenum * 3 + 2].vertex);
	const vec2d vec1{vf2.*ii - p1.i, vf2.*jj - p1.j};

	//vec from 1 -> checkpoint
	const vec2d checkp{pnt.*ii, pnt.*jj};

	//@@checkv.i = checkp.i - p1.i;
	//@@checkv.j = checkp.j - p1.j;

	k1 = -fixdiv(cross(&checkp,&vec0) + cross(&vec0,&p1),cross(&vec0,&vec1));
#if defined(DXX_BUILD_DESCENT_I)
	if (vec0.i)
#elif defined(DXX_BUILD_DESCENT_II)
	if (abs(vec0.i) > abs(vec0.j))
#endif
		k0 = fixdiv(fixmul(-k1,vec1.i) + checkp.i - p1.i,vec0.i);
	else
		k0 = fixdiv(fixmul(-k1,vec1.j) + checkp.j - p1.j,vec0.j);

	array<uvl, 3> uvls;
	for (i=0;i<3;i++)
		uvls[i] = side.uvls[vn[facenum * 3 + i].vertnum];

	auto p = [&uvls, k0, k1](fix uvl::*pmf) {
		return uvls[1].*pmf + fixmul(k0,uvls[0].*pmf - uvls[1].*pmf) + fixmul(k1,uvls[2].*pmf - uvls[1].*pmf);
	};
	return {
		p(&uvl::u),
		p(&uvl::v)
	};
}

//check if a particular point on a wall is a transparent pixel
//returns 1 if can pass though the wall, else 0
int check_trans_wall(const vms_vector &pnt,const vcsegptridx_t seg,int sidenum,int facenum)
{
	auto *side = &seg->sides[sidenum];
	int bmx,bmy;

#if defined(DXX_BUILD_DESCENT_I)
	assert(WALL_IS_DOORWAY(GameBitmaps, Textures, vcwallptr, seg, seg, sidenum) == WID_TRANSPARENT_WALL);
#endif

	const auto hitpoint = find_hitpoint_uv(pnt,seg,sidenum,facenum);	//	Don't compute light value.
	auto &u = hitpoint.u;
	auto &v = hitpoint.v;

	const grs_bitmap &rbm = (side->tmap_num2 != 0) ? texmerge_get_cached_bitmap( side->tmap_num, side->tmap_num2 ) :
		GameBitmaps[Textures[PIGGY_PAGE_IN(Textures[side->tmap_num]), side->tmap_num].index];
	const auto bm = rle_expand_texture(rbm);

	bmx = static_cast<unsigned>(f2i(u*bm->bm_w)) % bm->bm_w;
	bmy = static_cast<unsigned>(f2i(v*bm->bm_h)) % bm->bm_h;

//note: the line above had -v, but that was wrong, so I changed it.  if
//something doesn't work, and you want to make it negative again, you
//should figure out what's going on.

#if defined(DXX_BUILD_DESCENT_I)
	return (gr_gpixel (*bm, bmx, bmy) == 255);
#elif defined(DXX_BUILD_DESCENT_II)
	return (bm->bm_data[bmy*bm->bm_w+bmx] == TRANSPARENCY_COLOR);
#endif
}
}

//new function for Mike
//note: n_segs_visited must be set to zero before this is called
static int sphere_intersects_wall(const vms_vector &pnt, const vcsegptridx_t segnum, const fix rad, object_intersects_wall_result_t *const hresult, fvi_segments_visited_t &visited)
{
	int facemask;
	visited[segnum] = true;
	++visited.count;

	facemask = get_seg_masks(vcvertptr, pnt, segnum, rad).facemask;

	const auto &seg = segnum;

	if (facemask != 0) {				//on the back of at least one face

		int side,bit,face;

		//for each face we are on the back of, check if intersected

		for (side=0,bit=1;side<6 && facemask>=bit;side++) {

			for (face=0;face<2;face++,bit<<=1) {

				if (facemask & bit) {            //on the back of this face
					int face_hit_type;      //in what way did we hit the face?

					//did we go through this wall/door?
					auto &sidep = seg->sides[side];
					const auto v = create_abs_vertex_lists(seg, sidep, side);
					const auto &num_faces = v.first;
					const auto &vertex_list = v.second;

					face_hit_type = check_sphere_to_face(pnt, sidep.normals[face],
										face,((num_faces==1)?4:3),rad,vertex_list);

					if (face_hit_type) {            //through this wall/door
						//if what we have hit is a door, check the adjoining seg

						auto child = seg->children[side];

						if (!IS_CHILD(child))
						{
							if (hresult)
							{
								hresult->seg = segnum;
								hresult->side = side;
							}
							return 1;
						}
						else if (!visited[child]) {                //haven't visited here yet
							if (auto r = sphere_intersects_wall(pnt, seg.absolute_sibling(child), rad, hresult, visited))
								return r;
						}
					}
				}
			}
		}
	}
	return 0;
}

int sphere_intersects_wall(const vms_vector &pnt, const vcsegptridx_t seg, const fix rad, object_intersects_wall_result_t *const hresult)
{
	fvi_segments_visited_t visited;
	return sphere_intersects_wall(pnt, seg, rad, hresult, visited);
}

//Returns true if the object is through any walls
int object_intersects_wall(const vcobjptr_t objp)
{
	return sphere_intersects_wall(objp->pos, vcsegptridx(objp->segnum), objp->size, nullptr);
}

int object_intersects_wall_d(const vcobjptr_t objp, object_intersects_wall_result_t &result)
{
	return sphere_intersects_wall(objp->pos, vcsegptridx(objp->segnum), objp->size, &result);
}
