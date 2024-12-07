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
#include "d_levelstate.h"
#include "segiter.h"

using std::min;

namespace {

//find the point on the specified plane where the line intersects
//returns true if point found, false if line parallel to plane
//new_pnt is the found point on the plane
//plane_pnt & plane_norm describe the plane
//p0 & p1 are the ends of the line
[[nodiscard]]
static int find_plane_line_intersection(vms_vector &new_pnt, const vms_vector &plane_pnt, const vms_vector &plane_norm, const vms_vector &p0, const vms_vector &p1, const fix rad)
{
	auto d{vm_vec_sub(p1, p0)};
	const fix den{-vm_vec_dot(plane_norm, d)};
	if (unlikely(!den)) // moving parallel to wall, so can't hit it
		return 0;

	const auto w{vm_vec_sub(p0, plane_pnt)};
	fix num{vm_vec_dot(plane_norm,w) - rad}; //move point out by rad

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

struct vec2d {
	fix i,j;
};

//intersection types
enum class intersection_type : uint8_t
{
	None,	//doesn't touch face at all
	Face,	//touches face
	Edge,	//touches edge of face
};

struct ij_pair
{
	fix vms_vector::*largest_normal;
	fix vms_vector::*i;
	fix vms_vector::*j;
};

[[nodiscard]]
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
[[nodiscard]]
static unsigned check_point_to_face(const vms_vector &checkp, const vms_vector &norm, const unsigned facenum, const unsigned nv, const vertnum_array_list_t &vertex_list)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
///
	//now do 2d check to see if point is in side

	//project polygon onto plane by finding largest component of normal
	ij_pair ij = find_largest_normal(norm);
	if (norm.*ij.largest_normal <= 0)
	{
		using std::swap;
		swap(ij.i, ij.j);
	}

	//now do the 2d problem in the i,j plane

	const auto check_i{checkp.*ij.i};
	const auto check_j{checkp.*ij.j};

	auto &vcvertptr = Vertices.vcptr;
	unsigned edgemask{};
	for (const auto edge : xrange(nv))
	{
		auto &v0 = *vcvertptr(vertex_list[facenum * 3 + edge]);
		auto &v1 = *vcvertptr(vertex_list[facenum * 3 + ((edge + 1) % nv)]);
		const vec2d edgevec{
			.i = v1.*ij.i - v0.*ij.i,
			.j = v1.*ij.j - v0.*ij.j
		};
		const vec2d checkvec{
			.i = check_i - v0.*ij.i,
			.j = check_j - v0.*ij.j
		};
		const fix64 d{fixmul64(checkvec.i, edgevec.j) - fixmul64(checkvec.j, edgevec.i)};
		if (d < 0)              		//we are outside of triangle
			edgemask |= (1<<edge);
	}
	return edgemask;
}

//check if a sphere intersects a face
[[nodiscard]]
static intersection_type check_sphere_to_face(const vms_vector &pnt, const vms_vector &normal, const unsigned facenum, const unsigned nv, const fix rad, const vertnum_array_list_t &vertex_list)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	const auto checkp{pnt};
	//now do 2d check to see if point is in side

	auto edgemask{check_point_to_face(pnt, normal, facenum, nv, vertex_list)};

	//we've gone through all the sides, are we inside?

	if (edgemask == 0)
		return intersection_type::Face;
	else {
		vms_vector edgevec;            //this time, real 3d vectors
		int edgenum;

		//get verts for edge we're behind

		for (edgenum=0;!(edgemask&1);(edgemask>>=1),edgenum++);

		auto &vcvertptr = Vertices.vcptr;
		auto &v0 = *vcvertptr(vertex_list[facenum * 3 + edgenum]);
		auto &v1 = *vcvertptr(vertex_list[facenum * 3 + ((edgenum + 1) % nv)]);

		//check if we are touching an edge or point

		const auto edgelen = vm_vec_normalized_dir(edgevec,v1,v0);
		
		//find point dist from planes of ends of edge

		const auto d = vm_vec_dot(edgevec, vm_vec_sub(checkp, v0));
		if (d < 0)
			return intersection_type::None;
		else if (d > edgelen)
			return intersection_type::None;

		if (d+rad < 0)
			return intersection_type::None;                  //too far behind start point

		if (d-rad > edgelen)
			return intersection_type::None;    //too far part end point

		//find closest point on edge to check point
		const auto dist = vm_vec_dist2(checkp, /* closest_point */ vm_vec_scale_add(v0, edgevec, d));
		const fix64 rad64{rad};
		if (dist > vm_distance_squared{rad64 * rad64})
			return intersection_type::None;
		return intersection_type::Edge;
	}
}

//returns true if line intersects with face. fills in newp with intersection
//point on plane, whether or not line intersects side
//facenum determines which of four possible faces we have
//note: the seg parm is temporary, until the face itself has a point field
[[nodiscard]]
static intersection_type check_line_to_face(vms_vector &newp, const vms_vector &p0, const vms_vector &p1, const shared_segment &seg, const sidenum_t side, const unsigned facenum, const unsigned nv, const fix rad)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &s = seg.sides[side];
	const vms_vector &norm = s.normals[facenum];

	const auto &&[num_faces, vertex_list] = create_abs_vertex_lists(seg, s, side);

	//use lowest point number
	const auto vertnum{
		(num_faces == 2)
			? std::min(vertex_list[0], vertex_list[2])
			: *std::ranges::min_element(std::span(vertex_list).first<4>())
	};

	auto &vcvertptr = Vertices.vcptr;
	if (!find_plane_line_intersection(newp, vcvertptr(vertnum), norm, p0, p1, rad))
		return intersection_type::None;

	auto checkp = newp;

	//if rad != 0, project the point down onto the plane of the polygon

	if (rad!=0)
		vm_vec_scale_add2(checkp,norm,-rad);

	return check_sphere_to_face(checkp, s.normals[facenum], facenum, nv, rad, vertex_list);
}

//returns the value of a determinant
[[nodiscard]]
static fix calc_det_value(const std::pair<vms_vector, vms_vector> &rfvec, const vms_vector &uvec)
{
	return fixmul(rfvec.first.x, fixmul(uvec.y, rfvec.second.z)) -
				fixmul(rfvec.first.x, fixmul(uvec.z, rfvec.second.y)) -
				fixmul(rfvec.first.y, fixmul(uvec.x, rfvec.second.z)) +
				fixmul(rfvec.first.y, fixmul(uvec.z, rfvec.second.x)) +
				fixmul(rfvec.first.z, fixmul(uvec.x, rfvec.second.y)) -
				fixmul(rfvec.first.z, fixmul(uvec.y, rfvec.second.x));
}

//computes the parameters of closest approach of two lines
//fill in two parameters, t0 & t1.  returns 0 if lines are parallel, else 1
[[nodiscard]]
static std::optional<std::pair<fix, fix>> check_line_to_line(const vms_vector &p1, const vms_vector &v1, const vms_vector &p2, const vms_vector &v2)
{
	std::pair<vms_vector, vms_vector> rfvec;
	auto &detf = rfvec.second;
	detf = vm_vec_cross(v1, v2);
	const auto cross_mag2{vm_vec_dot(detf, detf)};

	if (cross_mag2 == 0)
		return std::nullopt;			//lines are parallel

	auto &detr = rfvec.first;
	vm_vec_sub(detr, p2, p1);
	const auto dv2{calc_det_value(rfvec, v2)};
	const auto dv1{calc_det_value(rfvec, v1)};

	const auto t1{fixdiv(dv2, cross_mag2)};
	const auto t2{fixdiv(dv1, cross_mag2)};
	return std::pair(t1, t2);		//found point
}

//this version is for when the start and end positions both poke through
//the plane of a side.  In this case, we must do checks against the edge
//of faces
[[nodiscard]]
static intersection_type special_check_line_to_face(vms_vector &newp, const vms_vector &p0, const vms_vector &p1, const shared_segment &seg, const sidenum_t side, const unsigned facenum, const unsigned nv, const fix rad)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	fix edge_t2{0};
	int edgenum;
	auto &s = seg.sides[side];

	//calc some basic stuff

	const auto &&vertex_list = create_abs_vertex_lists(seg, s, side).second;

	//figure out which edge(s) to check against

	unsigned edgemask = check_point_to_face(p0, s.normals[facenum], facenum, nv, vertex_list);

	if (edgemask == 0)
		return check_line_to_face(newp,p0,p1,seg,side,facenum,nv,rad);

	for (edgenum=0;!(edgemask&1);edgemask>>=1,edgenum++);

	auto &vcvertptr = Vertices.vcptr;
	auto &edge_v0 = *vcvertptr(vertex_list[facenum * 3 + edgenum]);
	auto &edge_v1 = *vcvertptr(vertex_list[facenum * 3 + ((edgenum + 1) % nv)]);

	auto edge_vec{vm_vec_sub(edge_v1, edge_v0)};

	//is the start point already touching the edge?

	//??

	//first, find point of closest approach of vec & edge

	const auto edge_len{vm_vec_normalize(edge_vec)};
	auto move_vec{vm_vec_sub(p1, p0)};
	const auto move_len{vm_vec_normalize(move_vec)};

	const auto &cll = check_line_to_line(edge_v0,edge_vec,p0,move_vec);
	if (!cll)
		return intersection_type::None;
	auto &&[edge_t, move_t] = *cll;

	//make sure t values are in valid range
	if (move_t<0 || move_t>move_len+rad)
		return intersection_type::None;

	const fix move_t2{
		(move_t > move_len)
		? move_len
		: move_t
	};

	if (edge_t < 0)		//saturate at points
		edge_t2 = 0;
	else
		edge_t2 = edge_t;
	
	if (edge_t2 > edge_len)		//saturate at points
		edge_t2 = edge_len;
	
	//now, edge_t & move_t determine closest points.  calculate the points.

	const auto closest_point_edge{vm_vec_scale_add(edge_v0, edge_vec, edge_t2)};
	const auto closest_point_move{vm_vec_scale_add(p0, move_vec, move_t2)};

	//find dist between closest points

	const auto closest_dist{vm_vec_dist2(closest_point_edge, closest_point_move)};

	//could we hit with this dist?

	//note massive tolerance here
	const vm_distance fudge_rad{(rad * 15) / 20};
	/* First test whether the square of the closest distance is less than the
	 * radius.  If yes, then the intersection is very close, and the
	 * multiplication can be skipped.  If no, test whether the square of the
	 * closest distance is less than the square of the radius.
	 */
	if (underlying_value(closest_dist) < fudge_rad || closest_dist < fudge_rad * fudge_rad)		//we hit.  figure out where
	{
		//now figure out where we hit

		newp = vm_vec_scale_add(p0, move_vec, move_t - rad);
		return intersection_type::Edge;
	}
	else
		return intersection_type::None;			//no hit
}

//maybe this routine should just return the distance and let the caller
//decide it it's close enough to hit
//determine if and where a vector intersects with a sphere
//vector defined by p0,p1
//returns dist if intersects, and fills in intp
//else returns 0
[[nodiscard]]
static vm_distance_squared check_vector_to_sphere_1(vms_vector &intp,const vms_vector &p0,const vms_vector &p1,const vms_vector &sphere_pos,fix sphere_rad)
{
	vms_vector dn;

	//this routine could be optimized if it's taking too much time!

	const auto d{vm_vec_sub(p1, p0)};
	const auto w{vm_vec_sub(sphere_pos, p0)};

	const auto mag_d{vm_vec_copy_normalize(dn, d)};

	if (mag_d == 0) {
		const auto int_dist{vm_vec_mag2(w)};
		intp = p0;
		if (int_dist < sphere_rad)
			return build_vm_distance_squared(int_dist);
		const fix64 sphere_rad64{sphere_rad};
		if (int_dist < vm_magnitude_squared{static_cast<uint64_t>(sphere_rad64 * sphere_rad64)})
			return build_vm_distance_squared(int_dist);
		return vm_distance_squared::minimum_value;
	}

	const fix w_dist{vm_vec_dot(dn, w)};

	if (w_dist < 0)		//moving away from object
		return vm_distance_squared::minimum_value;

	if (w_dist > mag_d+sphere_rad)
		return vm_distance_squared::minimum_value;		//cannot hit

	const auto closest_point{vm_vec_scale_add(p0, dn, w_dist)};

	const auto dist2{vm_vec_dist2(closest_point, sphere_pos)};
	const fix64 sphere_rad64{sphere_rad};
	const vm_distance_squared sphere_rad_squared{sphere_rad64 * sphere_rad64};
	if (dist2 < sphere_rad_squared)
	{
		const fix64 delta_squared{static_cast<fix64>(sphere_rad_squared) - static_cast<fix64>(dist2)};
		const fix delta{static_cast<fix>(delta_squared >> 16)};
		const auto shorten{fix_sqrt(delta)};
		const auto int_dist{w_dist - shorten};

		if (int_dist > mag_d || int_dist < 0) //past one or the other end of vector, which means we're inside
		{
			//past one or the other end of vector, which means we're inside? WRONG! Either you're inside OR you didn't quite make it!
			if (vm_vec_dist2(p0, sphere_pos) < sphere_rad_squared)
			{
				intp = p0; //don't move at all
				return vm_distance_squared{static_cast<fix64>(1)}; // note that we do not calculate a valid collision point. This is up to collision handling.
			} else {
				return vm_distance_squared::minimum_value;
			}
		}

		intp = vm_vec_scale_add(p0, dn, int_dist);         //calc intersection point
		return vm_distance_squared{static_cast<fix64>(int_dist) * int_dist};
	}
	else
		return vm_distance_squared::minimum_value;
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
[[nodiscard]]
static vm_distance_squared check_vector_to_object(const d_robot_info_array *const Robot_info, vms_vector &intp, const vms_vector &p0, const vms_vector &p1, const fix rad, const object_base &obj, const object &otherobj)
{
	fix size{obj.size};

	if (obj.type == OBJ_ROBOT)
	{
		if ((*Robot_info)[get_robot_id(obj)].attack_type)
			size = (size*3)/4;
	}
	//if obj is player, and bumping into other player or a weapon of another coop player, reduce radius
	else if (obj.type == OBJ_PLAYER &&
		 	(otherobj.type == OBJ_PLAYER ||
	 		((Game_mode & GM_MULTI_COOP) && otherobj.type == OBJ_WEAPON && otherobj.ctype.laser_info.parent_type == OBJ_PLAYER)))
		size = size/2;

	return check_vector_to_sphere_1(intp, p0, p1, obj.pos, size+rad);
}

#define MAX_SEGS_VISITED 100
struct fvi_segment_visit_count_t
{
	unsigned count{};
};

struct fvi_segments_visited_t : public fvi_segment_visit_count_t, public visited_segment_bitarray_t
{
};

}

namespace dsx {
namespace {
static fvi_hit_type fvi_sub(const fvi_query &, vms_vector &intp, segnum_t &ints, const vcsegptridx_t startseg, fix rad, fvi_info::segment_array_t &seglist, segnum_t entry_seg, fvi_segments_visited_t &visited, sidenum_t &fvi_hit_side, icsegidx_t &fvi_hit_side_seg, unsigned &fvi_nest_count, icsegidx_t &fvi_hit_pt_seg, const vms_vector *&wall_norm, icobjidx_t &fvi_hit_object);
}

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
fvi_hit_type find_vector_intersection(const fvi_query fq, const segnum_t startseg, const fix rad, fvi_info &hit_data)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	vms_vector hit_pnt;

	icobjidx_t fvi_hit_object = object_none;	// object number of object hit in last find_vector_intersection call.

	//check to make sure start point is in seg its supposed to be in
	//Assert(check_point_in_seg(p0,startseg,0).centermask==0);	//start point not in seg

	// invalid segnum, so say there is no hit.
	if (startseg > Highest_segment_index)
	{
		assert(startseg <= Highest_segment_index);
		hit_data.hit_pnt = fq.p0;
		hit_data.hit_seg = segnum_t{};
		hit_data.hit_object = 0;
		hit_data.hit_side = side_none;
		hit_data.hit_side_seg = segment_none;
		return fvi_hit_type::BadP0;
	}

	auto &vcvertptr = Vertices.vcptr;
	// Viewer is not in segment as claimed, so say there is no hit.
	if (get_seg_masks(vcvertptr, fq.p0, vcsegptr(startseg), 0).centermask != sidemask_t{})
	{
		hit_data.hit_pnt = fq.p0;
		hit_data.hit_seg = startseg;
		hit_data.hit_side = side_none;
		hit_data.hit_object = 0;
		hit_data.hit_side_seg = segment_none;
		return fvi_hit_type::BadP0;
	}

	fvi_segments_visited_t visited;
	visited[startseg] = true;

	sidenum_t fvi_hit_side;
	icsegidx_t fvi_hit_side_seg{segment_none};	// what seg the hitside is in
	unsigned fvi_nest_count{};

	icsegidx_t fvi_hit_pt_seg{segment_none};		// what segment the hit point is in
	segnum_t hit_seg2{segment_none};

	const vms_vector *wall_norm{nullptr};	//surface normal of hit wall
	const auto hit_type = fvi_sub(fq, hit_pnt, hit_seg2, vcsegptridx(startseg), rad, hit_data.seglist, segment_exit, visited, fvi_hit_side, fvi_hit_side_seg, fvi_nest_count, fvi_hit_pt_seg, wall_norm, fvi_hit_object);
	segnum_t hit_seg;
	if (hit_seg2 != segment_none && get_seg_masks(vcvertptr, hit_pnt, vcsegptr(hit_seg2), 0).centermask == sidemask_t{})
		hit_seg = hit_seg2;
	else
		hit_seg = find_point_seg(LevelSharedSegmentState, LevelUniqueSegmentState, hit_pnt, imsegptridx(startseg));

//MATT: TAKE OUT THIS HACK AND FIX THE BUGS!
	if (hit_type == fvi_hit_type::Wall && hit_seg == segment_none)
		if (fvi_hit_pt_seg != segment_none && get_seg_masks(vcvertptr, hit_pnt, vcsegptr(fvi_hit_pt_seg), 0).centermask == sidemask_t{})
			hit_seg = fvi_hit_pt_seg;

	if (hit_seg == segment_none) {
		segnum_t new_hit_seg2{segment_none};
		vms_vector new_hit_pnt;

		//because of code that deal with object with non-zero radius has
		//problems, try using zero radius and see if we hit a wall

		const auto new_hit_type{fvi_sub(fq, new_hit_pnt, new_hit_seg2, vcsegptridx(startseg), 0, hit_data.seglist, segment_exit, visited, fvi_hit_side, fvi_hit_side_seg, fvi_nest_count, fvi_hit_pt_seg, wall_norm, fvi_hit_object)};
		(void)new_hit_type; // FIXME! This should become hit_type, right?

		if (new_hit_seg2 != segment_none) {
			hit_seg = new_hit_seg2;
			hit_pnt = new_hit_pnt;
		}
	}


	if (hit_seg!=segment_none && (fq.flags & FQ_GET_SEGLIST))
	{
		fvi_info::segment_array_t::iterator i = hit_data.seglist.find(hit_seg), e = hit_data.seglist.end();
		/* If `hit_seg` is present in `seglist`, truncate `seglist` such that
		 * `seglist.back()` == `hit_seg`.
		 *
		 * Otherwise, `hit_seg` is not present in `seglist`.  If there is space
		 * to add it, then add it.
		 */
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

	assert(!(hit_type == fvi_hit_type::Object && fvi_hit_object == object_none));

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

namespace {

static int check_trans_wall(const vms_vector &pnt, vcsegptridx_t seg, sidenum_t sidenum, int facenum);
}
}

namespace {
static void append_segments(fvi_info::segment_array_t &dst, const fvi_info::segment_array_t &src)
{
	/* Avoid overflow.  Original code had n_segs < MAX_SEGS_VISITED-1,
	 * so leave an extra slot on min.
	 */
	const size_t scount{src.size()}, dcount{dst.size()}, count{std::min(scount, dst.max_size() - dcount - 1)};
	std::copy(src.begin(), src.begin() + count, std::back_inserter(dst));
}
}

namespace dsx {
namespace {
static fvi_hit_type fvi_sub(const fvi_query &fq, vms_vector &intp, segnum_t &ints, const vcsegptridx_t startseg, fix rad, fvi_info::segment_array_t &seglist, segnum_t entry_seg, fvi_segments_visited_t &visited, sidenum_t &fvi_hit_side, icsegidx_t &fvi_hit_side_seg, unsigned &fvi_nest_count, icsegidx_t &fvi_hit_pt_seg, const vms_vector *&wall_norm, icobjidx_t &fvi_hit_object)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	vms_vector closest_hit_point{}; 	//where we hit
	auto closest_d{vm_distance_squared::maximum_value};					//distance to hit point
	auto hit_type{fvi_hit_type::None};							//what sort of hit
	segnum_t hit_seg{segment_none};
	fvi_info::segment_array_t hit_none_seglist;

	seglist.clear();
	if (fq.flags & FQ_GET_SEGLIST)
		seglist.emplace_back(startseg);

	const unsigned cur_nest_level{fvi_nest_count};
	fvi_nest_count++;

	//first, see if vector hit any objects in this segment
	if (const auto LevelUniqueObjectState = fq.LevelUniqueObjectState)
	{
		auto &Objects = LevelUniqueObjectState->Objects;
		auto &vcobjptridx = Objects.vcptridx;
		/* A caller which provides LevelUniqueObjectState must provide a valid object.
		 * Obtain a require_valid instance once, before the loop begins.
		 */
		const vcobjptridx_t thisobjnum = fq.thisobjnum;
		const auto this_is_robot{thisobjnum->type == OBJ_ROBOT};
		const auto &collision = CollisionResult[thisobjnum->type];
		const auto Robot_info{fq.Robot_info};
		const robot_info *const robptrthis{this_is_robot
			? &(assert(Robot_info != nullptr), (*Robot_info)[get_robot_id(thisobjnum)])
			: nullptr
		};
		range_for (const auto objnum, objects_in(*startseg, vcobjptridx, vcsegptr))
		{
			if (thisobjnum == objnum)
				continue;
			if (objnum->flags & OF_SHOULD_BE_DEAD)
				continue;
			if (collision[objnum->type] == collision_result::ignore)
				continue;
			if (laser_are_related(objnum, thisobjnum))
				continue;
			if (std::ranges::find(fq.ignore_obj_list, objnum) != fq.ignore_obj_list.end())
				continue;
			int fudged_rad = rad;

#if DXX_BUILD_DESCENT == 2
			//	If this is a powerup, don't do collision if flag FQ_IGNORE_POWERUPS is set
			if (objnum->type == OBJ_POWERUP)
				if (fq.flags & FQ_IGNORE_POWERUPS)
					continue;
#endif

			/*
			 * In Descent 1:
			//	If this is a robot:robot collision, only do it if both of them have attack_type != 0 (eg, green guy)
			 *
			 * In Descent 2:
			 * Robot-vs-robot collisions never happen.  This appears to have
			 * been done for the benefit of the Diamond Claw.  However, it also
			 * has the effect of allowing the Thief to pass through other
			 * robots unharmed.
			 */
			if (robptrthis)
			{
				if (objnum->type == OBJ_ROBOT)
				{
#if DXX_BUILD_DESCENT == 1
					if (!((*Robot_info)[get_robot_id(objnum)].attack_type && robptrthis->attack_type))
#endif
					// -- MK: 11/18/95, 4claws glomming together...this is easy.  -- if (!(Robot_info[Objects[objnum].id].attack_type && Robot_info[Objects[thisobjnum].id].attack_type))
						continue;
				}
				if (robptrthis->attack_type)
					fudged_rad = (rad * 3) / 4;
			}
			//if obj is player, and bumping into other player or a weapon of another coop player, reduce radius
			else if (fq.thisobjnum->type == OBJ_PLAYER &&
					((objnum->type == OBJ_PLAYER) ||
					((Game_mode&GM_MULTI_COOP) &&  objnum->type == OBJ_WEAPON && objnum->ctype.laser_info.parent_type == OBJ_PLAYER)))
				fudged_rad = rad/2;	//(rad*3)/4;

			vms_vector hit_point;
			const auto &&d = check_vector_to_object(Robot_info, hit_point, fq.p0, fq.p1, fudged_rad, objnum, thisobjnum);
			if (d != vm_distance_squared{0})          //we have intersection
				if (d < closest_d) {
					fvi_hit_object = objnum;
					Assert(fvi_hit_object!=object_none);
					closest_d = d;
					closest_hit_point = hit_point;
					hit_type = fvi_hit_type::Object;
				}
		}
	}

	if (fq.thisobjnum != object_none && CollisionResult[fq.thisobjnum->type][OBJ_WALL] == collision_result::ignore)
		rad = 0;		//HACK - ignore when edges hit walls

	//now, check segment walls

	auto &vcvertptr = Vertices.vcptr;
	int startmask{get_seg_masks(vcvertptr, fq.p0, startseg, rad).facemask};

	const auto &&masks = get_seg_masks(vcvertptr, fq.p1, startseg, rad);    //on back of which faces?
	const auto centermask{masks.centermask};			//where the center point is

	segnum_t hit_none_seg{segment_none};
	if (centermask == sidemask_t{})
		hit_none_seg = startseg;

	if (const auto endmask{masks.facemask})
	{                             //on the back of at least one face
		//for each face we are on the back of, check if intersected

		int bit{1};
		for (const auto side : MAX_SIDES_PER_SEGMENT)
		{
			if (endmask < bit)
				break;
			const unsigned nv{get_side_is_quad(startseg->shared_segment::sides[side]) ? 4u : 3u};
			// commented out by mk on 02/13/94:: if ((num_faces=seg->sides[side].num_faces)==0) num_faces=1;

			for (int face=0; face < 2; ++face, bit <<= 1)
			{
				if (endmask & bit) {            //on the back of this face
					intersection_type face_hit_type;      //in what way did we hit the face?

					const auto child_segnum{startseg->shared_segment::children[side]};
					if (child_segnum == entry_seg)
						continue;		//don't go back through entry side

					//did we go through this wall/door?

					vms_vector hit_point;
					if (startmask & bit)		//start was also though.  Do extra check
						face_hit_type = special_check_line_to_face(hit_point,
										fq.p0, fq.p1, startseg, side,
										face,
										nv,rad);
					else
						//NOTE LINK TO ABOVE!!
						face_hit_type = check_line_to_face(hit_point,
										fq.p0, fq.p1, startseg, side,
										face,
										nv,rad);

					if (face_hit_type != intersection_type::None)
					{            //through this wall/door
						auto &Walls = LevelUniqueWallSubsystemState.Walls;
						auto &vcwallptr = Walls.vcptr;
						auto wid_flag = WALL_IS_DOORWAY(GameBitmaps, Textures, vcwallptr, startseg, side);

						//if what we have hit is a door, check the adjoining seg

						if (fq.thisobjnum == get_local_player().objnum && cheats.ghostphysics)
						{
							if (IS_CHILD(child_segnum))
 								wid_flag |= WALL_IS_DOORWAY_FLAG::fly;
						}

						if ((wid_flag & WALL_IS_DOORWAY_FLAG::fly) ||
							(
#if DXX_BUILD_DESCENT == 1
								(wid_flag == wall_is_doorway_result::transparent_wall) &&
#elif DXX_BUILD_DESCENT == 2
								((wid_flag & WALL_IS_DOORWAY_FLAG::render) && (wid_flag & WALL_IS_DOORWAY_FLAG::rendpast)) &&
#endif
								((fq.flags & FQ_TRANSWALL) || (fq.flags & FQ_TRANSPOINT && check_trans_wall(hit_point,startseg,side,face))))) {

							segnum_t newsegnum,sub_hit_seg;
							vms_vector sub_hit_point;
							const auto save_wall_norm{wall_norm};
							auto save_hit_objnum{fvi_hit_object};

							//do the check recursively on the next seg.

							newsegnum = child_segnum;

							if (auto &&v = visited[newsegnum]; !v) {                //haven't visited here yet
								v = true;
								++ visited.count;

								if (visited.count >= MAX_SEGS_VISITED)
									goto quit_looking;		//we've looked a long time, so give up

								fvi_info::segment_array_t temp_seglist;
								const auto sub_hit_type = fvi_sub(fq, sub_hit_point, sub_hit_seg, startseg.absolute_sibling(newsegnum), rad, temp_seglist, startseg, visited, fvi_hit_side, fvi_hit_side_seg, fvi_nest_count, fvi_hit_pt_seg, wall_norm, fvi_hit_object);

								if (sub_hit_type != fvi_hit_type::None)
								{
									const auto d{vm_vec_dist2(sub_hit_point, fq.p0)};
									if (d < closest_d) {
										closest_d = d;
										closest_hit_point = sub_hit_point;
										hit_type = sub_hit_type;
										if (sub_hit_seg!=segment_none) hit_seg = sub_hit_seg;

										//copy seglist
										if (fq.flags & FQ_GET_SEGLIST) {
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
									if (fq.flags & FQ_GET_SEGLIST)
										hit_none_seglist = temp_seglist;
								}
							}
						}
						else {          //a wall
								//is this the closest hit?
								const auto d{vm_vec_dist2(hit_point, fq.p0)};
								if (d < closest_d) {
									closest_d = d;
									closest_hit_point = hit_point;
									hit_type = fvi_hit_type::Wall;
									wall_norm = &startseg->shared_segment::sides[side].normals[face];
									if (get_seg_masks(vcvertptr, hit_point, startseg, rad).centermask == sidemask_t{})
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

	if (hit_type == fvi_hit_type::None)
	{     //didn't hit anything, return end point
		intp = fq.p1;
		ints = hit_none_seg;
		//MATT: MUST FIX THIS!!!!
		//Assert(!centermask);

		if (hit_none_seg!=segment_none) {			///(centermask == 0)
			if (fq.flags & FQ_GET_SEGLIST)
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

	assert(!(hit_type == fvi_hit_type::Object && fvi_hit_object == object_none));
	return hit_type;
}
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
fvi_hitpoint find_hitpoint_uv(const vms_vector &pnt, const cscusegment seg, const sidenum_t sidenum, const uint_fast32_t facenum)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &side = seg.s.sides[sidenum];

	//do lasers pass through illusory walls?

	//when do I return 0 & 1 for non-transparent walls?

	const auto &&vn = create_all_vertnum_lists(seg, side, sidenum);

	//now the hard work.

	//1. find what plane to project this wall onto to make it a 2d case

	const auto &normal_array = side.normals[facenum];
	auto fmax = [](const vms_vector &v, fix vms_vector::*a, fix vms_vector::*b) {
		return abs(v.*a) > abs(v.*b) ? a : b;
	};
	const auto biggest{fmax(normal_array, &vms_vector::z, fmax(normal_array, &vms_vector::y, &vms_vector::x))};
	const auto ii{(biggest == &vms_vector::x) ? &vms_vector::y : &vms_vector::x};
	const auto jj{(biggest == &vms_vector::z) ? &vms_vector::y : &vms_vector::z};

	//2. compute u,v of intersection point

	//vec from 1 -> 0
	auto &vcvertptr = Vertices.vcptr;
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

	const fix k1{-fixdiv(cross(&checkp, &vec0) + cross(&vec0, &p1), cross(&vec0, &vec1))};
	const fix k0{
#if DXX_BUILD_DESCENT == 1
		(vec0.i)
#elif DXX_BUILD_DESCENT == 2
		(abs(vec0.i) > abs(vec0.j))
#endif
		? fixdiv(fixmul(-k1, vec1.i) + checkp.i - p1.i, vec0.i)
		: fixdiv(fixmul(-k1, vec1.j) + checkp.j - p1.j, vec0.j)
	};

	std::array<uvl, 3> uvls;
	auto &uside = seg.u.sides[sidenum];
	for (const auto i : xrange(3u))
		uvls[i] = uside.uvls[vn[facenum * 3 + i].vertnum];

	auto p = [&uvls, k0, k1](fix uvl::*pmf) {
		return uvls[1].*pmf + fixmul(k0,uvls[0].*pmf - uvls[1].*pmf) + fixmul(k1,uvls[2].*pmf - uvls[1].*pmf);
	};
	return {
		p(&uvl::u),
		p(&uvl::v)
	};
}

namespace {
//check if a particular point on a wall is a transparent pixel
//returns 1 if can pass though the wall, else 0
int check_trans_wall(const vms_vector &pnt, const vcsegptridx_t seg, const sidenum_t sidenum, const int facenum)
{
	auto &side = seg->unique_segment::sides[sidenum];
	int bmx,bmy;

#if DXX_BUILD_DESCENT == 1
#ifndef NDEBUG
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptr = Walls.vcptr;
	assert(WALL_IS_DOORWAY(GameBitmaps, Textures, vcwallptr, seg, sidenum) == wall_is_doorway_result::transparent_wall);
#endif
#endif

	const auto hitpoint{find_hitpoint_uv(pnt, seg, sidenum, facenum)};	//	Don't compute light value.
	auto &u = hitpoint.u;
	auto &v = hitpoint.v;

	const auto tmap_num{side.tmap_num};
	const grs_bitmap &rbm = (side.tmap_num2 != texture2_value::None)
		? texmerge_get_cached_bitmap(tmap_num, side.tmap_num2)
		/* gcc-13 issues a -Wdangling-reference warning if this lambda returns
		 * a `const grs_bitmap &`, but no warning if the lambda returns a
		 * `const grs_bitmap *` to the same storage, and then converts it into
		 * a reference in the caller.  Since the storage is in `GameBitmaps`,
		 * both uses are safe.
		 *
		 * gcc-14 improved its heuristics and no longer warns for the lambda
		 * returning a `const grs_bitmap &`.
		 */
		: *( [](GameBitmaps_array &GameBitmaps, const bitmap_index texture1) -> const grs_bitmap * {
			PIGGY_PAGE_IN(texture1);
			return &GameBitmaps[texture1];
		} (GameBitmaps, Textures[get_texture_index(tmap_num)]) );
	const auto bm{rle_expand_texture(rbm)};

	bmx = static_cast<unsigned>(f2i(u*bm->bm_w)) % bm->bm_w;
	bmy = static_cast<unsigned>(f2i(v*bm->bm_h)) % bm->bm_h;

//note: the line above had -v, but that was wrong, so I changed it.  if
//something doesn't work, and you want to make it negative again, you
//should figure out what's going on.

#if DXX_BUILD_DESCENT == 1
	return (gr_gpixel (*bm, bmx, bmy) == TRANSPARENCY_COLOR);
#elif DXX_BUILD_DESCENT == 2
	return (bm->bm_data[bmy*bm->bm_w+bmx] == TRANSPARENCY_COLOR);
#endif
}
}
}

namespace {
//new function for Mike
//note: n_segs_visited must be set to zero before this is called
static sphere_intersects_wall_result sphere_intersects_wall(fvcsegptridx &vcsegptridx, fvcvertptr &vcvertptr, const vms_vector &pnt, const vcsegptridx_t seg, const fix rad, fvi_segments_visited_t &visited)
{
	visited[seg] = true;
	++visited.count;

	const shared_segment &sseg = seg;
	const int facemask{get_seg_masks(vcvertptr, pnt, sseg, rad).facemask};
	if (facemask != 0) {				//on the back of at least one face
		//for each face we are on the back of, check if intersected

		int bit{1};
		for (const auto side : MAX_SIDES_PER_SEGMENT)
		{
			if (facemask < bit)
				break;
			for (int face = 0;face < 2; ++face, bit <<= 1)
			{
				if (facemask & bit) {            //on the back of this face
					//did we go through this wall/door?
					auto &sidep = sseg.sides[side];
					const auto &&[num_faces, vertex_list] = create_abs_vertex_lists(sseg, sidep, side);

					//in what way did we hit the face?
					const auto face_hit_type = check_sphere_to_face(pnt, sidep.normals[face],
										face,((num_faces==1)?4:3),rad,vertex_list);

					if (face_hit_type != intersection_type::None)
					{            //through this wall/door
						//if what we have hit is a door, check the adjoining seg

						const auto child = sseg.children[side];

						if (!IS_CHILD(child))
						{
							return {&sseg, side};
						}
						else if (!visited[child]) {                //haven't visited here yet
							const auto &&r = sphere_intersects_wall(vcsegptridx, vcvertptr, pnt, vcsegptridx(child), rad, visited);
							if (r.seg)
								return r;
						}
					}
				}
			}
		}
	}
	return {};
}
}

sphere_intersects_wall_result sphere_intersects_wall(fvcsegptridx &vcsegptridx, fvcvertptr &vcvertptr, const vms_vector &pnt, const vcsegptridx_t seg, const fix rad)
{
	fvi_segments_visited_t visited;
	return sphere_intersects_wall(vcsegptridx, vcvertptr, pnt, seg, rad, visited);
}
