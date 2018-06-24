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
 * group functions
 *
 */

#include <stdio.h>
#include <string.h>

#include "gr.h"
#include "ui.h"
#include "inferno.h"
#include "segment.h"
#include "editor/editor.h"
#include "editor/esegment.h"
#include "editor/medmisc.h"
#include "dxxerror.h"
#include "gamemine.h"
#include "physfsx.h"
#include "gameseg.h"
#include "bm.h"				// For MAX_TEXTURES.
#include "textures.h"
#include "hash.h"
#include "fuelcen.h"
#include "kdefs.h"
#include "fwd-wall.h"
#include "medwall.h"
#include "dxxsconf.h"
#include "compiler-range_for.h"
#include "partial_range.h"
#include "segiter.h"

static void validate_selected_segments(void);

struct group_top_fileinfo {
	int     fileinfo_version;
	int     fileinfo_sizeof;
} group_top_fileinfo;    // Should be same as first two fields below...

struct group_fileinfo {
	int     fileinfo_version;
	int     fileinfo_sizeof;
	int     header_offset;          	// Stuff common to game & editor
	int     header_size;
	int     editor_offset;   			// Editor specific stuff
	int     editor_size;
	int     vertex_offset;
	int     vertex_howmany;
	int     vertex_sizeof;
	int     segment_offset;
	int     segment_howmany;
	int     segment_sizeof;
	int     texture_offset;
	uint32_t texture_howmany;
	int     texture_sizeof;
} group_fileinfo;

struct group_header {
	int     num_vertices;
	int     num_segments;
} group_header;

struct group_editor {
	int     current_seg;
	int     newsegment_offset;
	int     newsegment_size;
	int     Groupsegp;
	int     Groupside;
} group_editor;

array<group, MAX_GROUPS+1> GroupList;
array<segment *, MAX_GROUPS+1> Groupsegp;
array<int, MAX_GROUPS+1> Groupside;
array<int, MAX_GROUPS+1> Group_orientation;
int		current_group=-1;
unsigned num_groups;

// -- void swap_negate_columns(vms_matrix *rotmat, int col1, int col2)
// -- {
// -- 	fix	col1_1,col1_2,col1_3;
// -- 	fix	col2_1,col2_2,col2_3;
// -- 
// -- 	switch (col1) {
// -- 		case 0:
// -- 			col1_1 = rotmat->m1;
// -- 			col1_2 = rotmat->m2;
// -- 			col1_3 = rotmat->m3;
// -- 			break;
// -- 
// -- 		case 1:
// -- 			col1_1 = rotmat->m4;
// -- 			col1_2 = rotmat->m5;
// -- 			col1_3 = rotmat->m6;
// -- 			break;
// -- 
// -- 		case 2:
// -- 			col1_1 = rotmat->m7;
// -- 			col1_2 = rotmat->m8;
// -- 			col1_3 = rotmat->m9;
// -- 			break;
// -- 	}
// -- 
// -- 	switch (col2) {
// -- 		case 0:
// -- 			col2_1 = rotmat->m1;
// -- 			col2_2 = rotmat->m2;
// -- 			col2_3 = rotmat->m3;
// -- 			break;
// -- 
// -- 		case 1:
// -- 			col2_1 = rotmat->m4;
// -- 			col2_2 = rotmat->m5;
// -- 			col2_3 = rotmat->m6;
// -- 			break;
// -- 
// -- 		case 2:
// -- 			col2_1 = rotmat->m7;
// -- 			col2_2 = rotmat->m8;
// -- 			col2_3 = rotmat->m9;
// -- 			break;
// -- 	}
// -- 
// -- 	switch (col2) {
// -- 		case 0:
// -- 			rotmat->m1 = -col1_1;
// -- 			rotmat->m2 = -col1_2;
// -- 			rotmat->m3 = -col1_3;
// -- 			break;
// -- 
// -- 		case 1:
// -- 			rotmat->m4 = -col1_1;
// -- 			rotmat->m5 = -col1_2;
// -- 			rotmat->m6 = -col1_3;
// -- 			break;
// -- 
// -- 		case 2:
// -- 			rotmat->m7 = -col1_1;
// -- 			rotmat->m8 = -col1_2;
// -- 			rotmat->m9 = -col1_3;
// -- 			break;
// -- 	}
// -- 
// -- 	switch (col1) {
// -- 		case 0:
// -- 			rotmat->m1 = -col2_1;
// -- 			rotmat->m2 = -col2_2;
// -- 			rotmat->m3 = -col2_3;
// -- 			break;
// -- 
// -- 		case 1:
// -- 			rotmat->m4 = -col2_1;
// -- 			rotmat->m5 = -col2_2;
// -- 			rotmat->m6 = -col2_3;
// -- 			break;
// -- 
// -- 		case 2:
// -- 			rotmat->m7 = -col2_1;
// -- 			rotmat->m8 = -col2_2;
// -- 			rotmat->m9 = -col2_3;
// -- 			break;
// -- 	}
// -- 
// -- }
// -- 
// -- void swap_negate_rows(vms_matrix *rotmat, int row1, int row2)
// -- {
// -- 	fix	row1_1,row1_2,row1_3;
// -- 	fix	row2_1,row2_2,row2_3;
// -- 
// -- 	switch (row1) {
// -- 		case 0:
// -- 			row1_1 = rotmat->m1;
// -- 			row1_2 = rotmat->m4;
// -- 			row1_3 = rotmat->m7;
// -- 			break;
// -- 
// -- 		case 1:
// -- 			row1_1 = rotmat->m2;
// -- 			row1_2 = rotmat->m5;
// -- 			row1_3 = rotmat->m8;
// -- 			break;
// -- 
// -- 		case 2:
// -- 			row1_1 = rotmat->m3;
// -- 			row1_2 = rotmat->m6;
// -- 			row1_3 = rotmat->m9;
// -- 			break;
// -- 	}
// -- 
// -- 	switch (row2) {
// -- 		case 0:
// -- 			row2_1 = rotmat->m1;
// -- 			row2_2 = rotmat->m4;
// -- 			row2_3 = rotmat->m7;
// -- 			break;
// -- 
// -- 		case 1:
// -- 			row2_1 = rotmat->m2;
// -- 			row2_2 = rotmat->m5;
// -- 			row2_3 = rotmat->m8;
// -- 			break;
// -- 
// -- 		case 2:
// -- 			row2_1 = rotmat->m3;
// -- 			row2_2 = rotmat->m6;
// -- 			row2_3 = rotmat->m9;
// -- 			break;
// -- 	}
// -- 
// -- 	switch (row2) {
// -- 		case 0:
// -- 			rotmat->m1 = -row1_1;
// -- 			rotmat->m4 = -row1_2;
// -- 			rotmat->m7 = -row1_3;
// -- 			break;
// -- 
// -- 		case 1:
// -- 			rotmat->m2 = -row1_1;
// -- 			rotmat->m5 = -row1_2;
// -- 			rotmat->m8 = -row1_3;
// -- 			break;
// -- 
// -- 		case 2:
// -- 			rotmat->m3 = -row1_1;
// -- 			rotmat->m6 = -row1_2;
// -- 			rotmat->m9 = -row1_3;
// -- 			break;
// -- 	}
// -- 
// -- 	switch (row1) {
// -- 		case 0:
// -- 			rotmat->m1 = -row2_1;
// -- 			rotmat->m4 = -row2_2;
// -- 			rotmat->m7 = -row2_3;
// -- 			break;
// -- 
// -- 		case 1:
// -- 			rotmat->m2 = -row2_1;
// -- 			rotmat->m5 = -row2_2;
// -- 			rotmat->m8 = -row2_3;
// -- 			break;
// -- 
// -- 		case 2:
// -- 			rotmat->m3 = -row2_1;
// -- 			rotmat->m6 = -row2_2;
// -- 			rotmat->m9 = -row2_3;
// -- 			break;
// -- 	}
// -- 
// -- }
// -- 
// -- // ------------------------------------------------------------------------------------------------
// -- void	side_based_matrix(vms_matrix *rotmat,int destside)
// -- {
// -- 	vms_angvec	rotvec;
// -- 	vms_matrix	r1,rtemp;
// -- 
// -- 	switch (destside) {
// -- 		case WLEFT:
// -- //			swap_negate_columns(rotmat,1,2);
// -- //			swap_negate_rows(rotmat,1,2);
// -- 			break;
// -- 
// -- 		case WTOP:
// -- 			break;
// -- 
// -- 		case WRIGHT:
// -- //			swap_negate_columns(rotmat,1,2);
// -- //			swap_negate_rows(rotmat,1,2);
// -- 			break;
// -- 
// -- 		case WBOTTOM:
// -- 			break;
// -- 
// -- 		case WFRONT:
// -- 			break;
// -- 
// -- 		case WBACK:
// -- 			break;
// -- 	}
// -- 
// -- }


// ------------------------------------------------------------------------------------------------
//	Rotate a group about a point.
//	The segments in the group are indicated (by segment number) in group_seglist.  There are group_size segments.
//	The point about which the groups is rotated is the center of first_seg:first_side.
//	delta_flag:
//		0	absolute rotation, destination specified in terms of base_seg:base_side, used in moving or copying a group
//		1	relative rotation, destination specified relative to current orientation of first_seg:first_side
//	Note: The group must exist in the mine, consisting of actual points in the world.  If any points in the
//			segments in the group are shared by segments not in the group, those points will get rotated and the
//			segments not in the group will have their shapes modified.
//	Return value:
//		0	group rotated
//		1	unable to rotate group
static void med_create_group_rotation_matrix(vms_matrix &result_mat, int delta_flag, const vcsegptr_t first_seg, int first_side, const vcsegptr_t base_seg, int base_side, const vms_matrix &orient_matrix, int orientation)
{
	vms_matrix	rotmat2,rotmat,rotmat3,rotmat4;
	vms_angvec	pbh = {0,0,0};

	//	Determine whether this rotation is a delta rotation, meaning to just rotate in place, or an absolute rotation,
	//	which means that the destination rotation is specified, not as a delta, but as an absolute
	if (delta_flag) {
	 	//	Create rotation matrix describing rotation.
		med_extract_matrix_from_segment(first_seg, rotmat4);		// get rotation matrix describing current orientation of first seg
		update_matrix_based_on_side(rotmat4, first_side);
		rotmat3 = vm_transposed_matrix(orient_matrix);
		const auto vm_desired_orientation = vm_matrix_x_matrix(rotmat4,rotmat3);			// this is the desired orientation of the new segment
		vm_transpose_matrix(rotmat4);
	 	vm_matrix_x_matrix(rotmat2,vm_desired_orientation,rotmat4);			// this is the desired orientation of the new segment
	} else {
	 	//	Create rotation matrix describing rotation.
 
		med_extract_matrix_from_segment(base_seg, rotmat);		// get rotation matrix describing desired orientation
	 	update_matrix_based_on_side(rotmat, base_side);				// modify rotation matrix for desired side
 
	 	//	If the new segment is to be attached without rotation, then its orientation is the same as the base_segment
	 	vm_matrix_x_matrix(rotmat4,rotmat,orient_matrix);			// this is the desired orientation of the new segment

		pbh.b = orientation*16384;
		vm_angles_2_matrix(rotmat3,pbh);
		rotmat4 = rotmat = vm_matrix_x_matrix(rotmat4, rotmat3);
		med_extract_matrix_from_segment(first_seg, rotmat3);		// get rotation matrix describing current orientation of first seg
 
	 	// It is curious that the following statement has no analogue in the med_attach_segment_rotated code.
	 	//	Perhaps it is because segments are always attached at their front side.  If the back side is the side
	 	//	passed to the function, then the matrix is not modified, which might suggest that what you need to do below
	 	//	is use Side_opposite[first_side].
	 	update_matrix_based_on_side(rotmat3, Side_opposite[first_side]);				// modify rotation matrix for desired side
 
	 	vm_transpose_matrix(rotmat3);								// get the inverse of the current orientation matrix
		rotmat2 = vm_transposed_matrix(vm_matrix_x_matrix(rotmat,rotmat3));			// now rotmat2 takes the current segment to the desired orientation
	}
	result_mat = rotmat2;
}

static inline vms_matrix med_create_group_rotation_matrix(int delta_flag, const vcsegptr_t first_seg, int first_side, const vcsegptr_t base_seg, int base_side, const vms_matrix &orient_matrix, int orientation)
{
	vms_matrix result_mat;
	return med_create_group_rotation_matrix(result_mat, delta_flag, first_seg, first_side, base_seg, base_side, orient_matrix, orientation), result_mat;
}

// -----------------------------------------------------------------------------------------
// Rotate all vertices and objects in group.
static void med_rotate_group(const vms_matrix &rotmat, group::segment_array_type_t &group_seglist, const vcsegptr_t first_seg, int first_side)
{
	array<int8_t, MAX_VERTICES> vertex_list;
	const auto &&rotate_center = compute_center_point_on_side(vcvertptr, first_seg, first_side);

	//	Create list of points to rotate.
	vertex_list = {};

	range_for (const auto &gs, group_seglist)
	{
		auto &sp = *vmsegptr(gs);

		range_for (const auto v, sp.verts)
			vertex_list[v] = 1;

		//	Rotate center of all objects in group.
		range_for (const auto objp, objects_in(sp, vmobjptridx, vcsegptr))
		{
			const auto tv1 = vm_vec_sub(objp->pos,rotate_center);
			const auto tv = vm_vec_rotate(tv1,rotmat);
			vm_vec_add(objp->pos, tv, rotate_center);
		}			
	}

	// Do the pre-rotation xlate, do the rotation, do the post-rotation xlate
	range_for (auto &&v, vmvertptridx)
		if (vertex_list[v]) {
			const auto &&tv1 = vm_vec_sub(*v, rotate_center);
			const auto tv = vm_vec_rotate(tv1,rotmat);
			vm_vec_add(*v, tv, rotate_center);
		}
}

// ------------------------------------------------------------------------------------------------
static void cgl_aux(const vmsegptridx_t segp, group::segment_array_type_t &seglistp, selected_segment_array_t *ignore_list, visited_segment_bitarray_t &visited)
{
	if (ignore_list)
		if (ignore_list->contains(segp))
			return;

	if (!visited[segp]) {
		visited[segp] = true;
		seglistp.emplace_back(segp);

		range_for (const auto c, segp->children)
			if (IS_CHILD(c))
				cgl_aux(segp.absolute_sibling(c), seglistp, ignore_list, visited);
	}
}

// ------------------------------------------------------------------------------------------------
//	Sets Been_visited[n] if n is reachable from segp
static void create_group_list(const vmsegptridx_t segp, group::segment_array_type_t &seglistp, selected_segment_array_t *ignore_list)
{
	visited_segment_bitarray_t visited;
	cgl_aux(segp, seglistp, ignore_list, visited);
}


#define MXS MAX_SEGMENTS
#define MXV MAX_VERTICES

// ------------------------------------------------------------------------------------------------
static void duplicate_group(array<uint8_t, MAX_VERTICES> &vertex_ids, group::segment_array_type_t &segments)
{
	group::segment_array_type_t new_segments;
	array<int, MAX_VERTICES> new_vertex_ids;		// If new_vertex_ids[v] != -1, then vertex v has been remapped to new_vertex_ids[v]

	//	duplicate vertices
	new_vertex_ids.fill(-1);

	//	duplicate vertices
	range_for (auto &&v, vcvertptridx)
	{
		if (vertex_ids[v])
		{
			new_vertex_ids[v] = med_create_duplicate_vertex(*v);
		}
	}

	//	duplicate segments
	range_for(const auto &gs, segments)
	{
		const auto &&segp = vmsegptr(gs);
		const auto &&new_segment_id = med_create_duplicate_segment(segp);
		new_segments.emplace_back(new_segment_id);
		range_for (const auto objp, objects_in(segp, vmobjptridx, vmsegptr))
		{
			if (objp->type != OBJ_PLAYER) {
				const auto &&new_obj_id = obj_create_copy(objp, objp->pos, vmsegptridx(new_segment_id));
				(void)new_obj_id; // FIXME!
			}
		}
	}

	//	Now, for each segment in segment_ids, correct its children numbers by translating through new_segment_ids
	//	and correct its vertex numbers by translating through new_vertex_ids
	range_for(const auto &gs, new_segments)
	{
		auto &sp = *vmsegptr(gs);
		range_for (auto &seg, sp.children)
		{
			if (IS_CHILD(seg)) {
				group::segment_array_type_t::iterator inew = new_segments.begin();
				range_for (const auto i, segments)
				{
					if (seg == i)
					{
						seg = *inew;
						break;
					}
					++inew;
				}
			}
		}	// end for (sidenum=0...

		//	Now fixup vertex ids
		range_for (auto &v, sp.verts)
		{
			if (vertex_ids[v]) {
				v = new_vertex_ids[v];
			}
		}
	}	// end for (s=0...

	//	Now, copy new_segment_ids into segment_ids
	segments = new_segments;

	//	Now, copy new_vertex_ids into vertex_ids
	vertex_ids = {};

	range_for (auto &v, new_vertex_ids)
		if (v != -1)
			vertex_ids[v] = 1;
}


// ------------------------------------------------------------------------------------------------
static int in_group(segnum_t segnum, int group_num)
{
	range_for(const auto& s, GroupList[group_num].segments)
		if (segnum == s)
			return 1;

	return 0;
}

// ------------------------------------------------------------------------------------------------
//	Copy a group of segments.
//	The group is defined as all segments accessible from group_seg.
//	The group is copied so group_seg:group_side is incident upon base_seg:base_side.
//	group_seg and its vertices are bashed to coincide with base_seg.
//	If any vertex of base_seg is contained in a segment that is reachable from group_seg, then errror.
static int med_copy_group(int delta_flag, const vmsegptridx_t base_seg, int base_side, vcsegptr_t group_seg, int group_side, const vms_matrix &orient_matrix)
{
	int 			x;
	int			new_current_group;
	int 			c;

	if (IS_CHILD(base_seg->children[base_side])) {
		editor_status("Error -- unable to copy group, base_seg:base_side must be free.");
		return 1;
	}

	if (num_groups == MAX_GROUPS) {
		x = ui_messagebox( -2, -2, 2, "Warning: You have reached the MAXIMUM group number limit. Continue?", "No", "Yes" );
		if (x==1)
			return 0;
	}

	if (num_groups < MAX_GROUPS) {
		num_groups++;
		new_current_group = num_groups-1;
	} else
		new_current_group = 0;

	Assert(current_group >= 0);

	// Find groupsegp index
	auto gb = GroupList[current_group].segments.begin();
	auto ge = GroupList[current_group].segments.end();
	auto gp = Groupsegp[current_group];
	auto gi = std::find_if(gb, ge, [gp](const segnum_t segnum){ return vcsegptr(segnum) == gp; });
	int gs_index = (gi == ge) ? 0 : std::distance(gb, gi);

	GroupList[new_current_group] = GroupList[current_group];

	//	Make a list of all vertices in group.
	array<uint8_t, MAX_VERTICES> in_vertex_list{};
	if (group_seg == &New_segment)
		range_for (auto &v, group_seg->verts)
			in_vertex_list[v] = 1;
	else {
		range_for(const auto &gs, GroupList[new_current_group].segments)
			range_for (auto &v, vmsegptr(gs)->verts)
				in_vertex_list[v] = 1;
	}

	// Given a list of vertex indices (indicated by !0 in in_vertex_list) and segment indices (in list GroupList[current_group].segments, there
	//	are GroupList[current_group].num_segments segments), copy all segments and vertices
	//	Return updated lists of vertices and segments in in_vertex_list and GroupList[current_group].segments
	duplicate_group(in_vertex_list, GroupList[new_current_group].segments);

	//group_seg = &Segments[GroupList[new_current_group].segments[0]];					// connecting segment in group has been changed, so update group_seg

	{
		const auto &&gs = vmsegptr(GroupList[new_current_group].segments[gs_index]);
		group_seg = gs;
		Groupsegp[new_current_group] = gs;
	}
	Groupside[new_current_group] = Groupside[current_group];

	range_for(const auto &gs, GroupList[new_current_group].segments)
	{
		auto &s = *vmsegptr(gs);
		s.group = new_current_group;
		s.special = SEGMENT_IS_NOTHING;
		s.matcen_num = -1;
	}

	// Breaking connections between segments in the current group and segments not in the group.
	range_for(const auto &gs, GroupList[new_current_group].segments)
	{
		const auto &&segp = base_seg.absolute_sibling(gs);
		for (c=0; c < MAX_SIDES_PER_SEGMENT; c++) 
			if (IS_CHILD(segp->children[c])) {
				if (!in_group(segp->children[c], new_current_group)) {
					segp->children[c] = segment_none;
					validate_segment_side(segp,c);					// we have converted a connection to a side so validate the segment
				}
			}
	}

	copy_uvs_seg_to_seg(vmsegptr(&New_segment), group_seg);
	
	//	Now do the copy
	//	First, xlate all vertices so center of group_seg:group_side is at origin
	const auto &&srcv = compute_center_point_on_side(vcvertptr, group_seg, group_side);
	range_for (auto &&v, vmvertptridx)
		if (in_vertex_list[v])
			vm_vec_sub2(*v, srcv);

	//	Now, translate all object positions.
	range_for(const auto &segnum, GroupList[new_current_group].segments)
	{
		range_for (const auto objp, objects_in(vmsegptr(segnum), vmobjptridx, vmsegptr))
			vm_vec_sub2(objp->pos, srcv);
	}

	//	Now, rotate segments in group so orientation of group_seg is same as base_seg.
	const auto rotmat = med_create_group_rotation_matrix(delta_flag, group_seg, group_side, base_seg, base_side, orient_matrix, 0);
	med_rotate_group(rotmat, GroupList[new_current_group].segments, group_seg, group_side);

	//	Now xlate all vertices so group_seg:group_side shares center point with base_seg:base_side
	const auto &&destv = compute_center_point_on_side(vcvertptr, base_seg, base_side);
	range_for (auto &&v, vmvertptridx)
		if (in_vertex_list[v])
			vm_vec_add2(*v, destv);

	//	Now, xlate all object positions.
	range_for(const auto &segnum, GroupList[new_current_group].segments)
	{
		range_for (const auto objp, objects_in(vmsegptr(segnum), vmobjptridx, vmsegptr))
			vm_vec_add2(objp->pos, destv);
	}

	//	Now, copy all walls (ie, doors, illusionary, etc.) into the new group.
	copy_group_walls(current_group, new_current_group);

	current_group = new_current_group;

	//	Now, form joint on connecting sides.
	med_form_joint(base_seg,base_side,vmsegptridx(Groupsegp[current_group]),Groupside[new_current_group]);

	validate_selected_segments();
	med_combine_duplicate_vertices(in_vertex_list);

	return 0;
}


// ------------------------------------------------------------------------------------------------
//	Move a group of segments.
//	The group is defined as all segments accessible from group_seg.
//	The group is moved so group_seg:group_side is incident upon base_seg:base_side.
//	group_seg and its vertices are bashed to coincide with base_seg.
//	If any vertex of base_seg is contained in a segment that is reachable from group_seg, then errror.
static int med_move_group(int delta_flag, const vmsegptridx_t base_seg, int base_side, const vmsegptridx_t group_seg, int group_side, const vms_matrix &orient_matrix, int orientation)
{
	int			c, d;

	if (IS_CHILD(base_seg->children[base_side]))
		if (base_seg->children[base_side] != group_seg) {
			editor_status("Error -- unable to move group, base_seg:base_side must be free or point to group_seg.");
			return 1;
	}

//	// See if any vertices in base_seg are contained in any vertex in group_list
//	for (v=0; v<MAX_VERTICES_PER_SEGMENT; v++)
//		for (s=0; s<GroupList[current_group].num_segments; s++)
//			for (vv=0; vv<MAX_VERTICES_PER_SEGMENT; vv++)
//				if (Segments[GroupList[current_group].segments[s]].verts[vv] == base_seg->verts[v]) {
//					editor_status("Error -- unable to move group, it shares a vertex with destination segment.");
//					return 1;
//				}

	array<uint8_t, MAX_VERTICES> in_vertex_list{};
	array<int8_t, MAX_VERTICES> out_vertex_list{};

	//	Make a list of all vertices in group.
	range_for(const auto &gs, GroupList[current_group].segments)
		range_for (auto &v, vmsegptr(gs)->verts)
			in_vertex_list[v] = 1;

	//	For all segments which are not in GroupList[current_group].segments, mark all their vertices in the out list.
	range_for (const auto &&segp, vmsegptridx)
	{
		if (!GroupList[current_group].segments.contains(segp))
			{
				range_for (auto &v, segp->verts)
					out_vertex_list[v] = 1;
			}
	}

	//	Now, for all vertices present in both the in (part of group segment) and out (part of non-group segment)
	// create an extra copy of the vertex so we can just move the ones in the in list.
	//	Can't use Highest_vertex_index as loop termination because it gets increased by med_create_duplicate_vertex.

	range_for (auto &&v, vmvertptridx)
		if (in_vertex_list[v])
			if (out_vertex_list[v]) {
				const auto new_vertex_id = med_create_duplicate_vertex(*v);
				in_vertex_list[v] = 0;
				in_vertex_list[new_vertex_id] = 1;

				// Create a new vertex and assign all occurrences of vertex v in IN list to new vertex number.
				range_for(const auto &gs, GroupList[current_group].segments)
				{
					auto &sp = *vmsegptr(gs);
					range_for (auto &vv, sp.verts)
						if (vv == v)
							vv = new_vertex_id;
				}
			}

	range_for(const auto &gs, GroupList[current_group].segments)
		vmsegptr(gs)->group = current_group;

	// Breaking connections between segments in the group and segments not in the group.
	range_for(const auto &gs, GroupList[current_group].segments)
		{
		const auto &&segp = base_seg.absolute_sibling(gs);
		for (c=0; c < MAX_SIDES_PER_SEGMENT; c++) 
			if (IS_CHILD(segp->children[c]))
				{
				const auto &&csegp = base_seg.absolute_sibling(segp->children[c]);
				if (csegp->group != current_group)
					{
					for (d=0; d<MAX_SIDES_PER_SEGMENT; d++)
						if (IS_CHILD(csegp->children[d]))
							{
							const auto &&dsegp = vmsegptr(csegp->children[d]);
							if (dsegp->group == current_group)
								{
								csegp->children[d] = segment_none;
								validate_segment_side(csegp,d);					// we have converted a connection to a side so validate the segment
								}
							}
					segp->children[c] = segment_none;
					validate_segment_side(segp,c);					// we have converted a connection to a side so validate the segment
					}
				}
		}

	copy_uvs_seg_to_seg(vmsegptr(&New_segment), vcsegptr(Groupsegp[current_group]));

	//	Now do the move
	//	First, xlate all vertices so center of group_seg:group_side is at origin
	const auto &&srcv = compute_center_point_on_side(vcvertptr, group_seg, group_side);
	range_for (auto &&v, vmvertptridx)
		if (in_vertex_list[v])
			vm_vec_sub2(*v, srcv);

	//	Now, move all object positions.
	range_for(const auto &segnum, GroupList[current_group].segments)
	{
		range_for (const auto objp, objects_in(vmsegptr(segnum), vmobjptridx, vmsegptr))
			vm_vec_sub2(objp->pos, srcv);
	}

	//	Now, rotate segments in group so orientation of group_seg is same as base_seg.
	const auto rotmat = med_create_group_rotation_matrix(delta_flag, group_seg, group_side, base_seg, base_side, orient_matrix, orientation);
	med_rotate_group(rotmat, GroupList[current_group].segments, group_seg, group_side);

	//	Now xlate all vertices so group_seg:group_side shares center point with base_seg:base_side
	const auto &&destv = compute_center_point_on_side(vcvertptr, base_seg, base_side);
	range_for (auto &&v, vmvertptridx)
		if (in_vertex_list[v])
			vm_vec_add2(*v, destv);

	//	Now, rotate all object positions.
	range_for(const auto &segnum, GroupList[current_group].segments)
	{
		range_for (const auto objp, objects_in(vmsegptr(segnum), vmobjptridx, vmsegptr))
			vm_vec_add2(objp->pos, destv);
	}

	//	Now, form joint on connecting sides.
	med_form_joint(base_seg,base_side,group_seg,group_side);

	validate_selected_segments();
	med_combine_duplicate_vertices(in_vertex_list);

	return 0;
}


//	-----------------------------------------------------------------------------
static segnum_t place_new_segment_in_world(void)
{
	const auto &&segnum = vmsegptridx(get_free_segment_number());
	auto &seg = *segnum;
	seg = New_segment;

	for (unsigned v = 0; v != MAX_VERTICES_PER_SEGMENT; ++v)
		seg.verts[v] = med_create_duplicate_vertex(vcvertptr(New_segment.verts[v]));

	return segnum;

}

//	-----------------------------------------------------------------------------
//	Attach segment in the new-fangled way, which is by using the CopyGroup code.
static int AttachSegmentNewAng(const vms_angvec &pbh)
{
	GroupList[current_group].segments.clear();
	const auto newseg = place_new_segment_in_world();
	GroupList[current_group].segments.emplace_back(newseg);

	const auto &&nsegp = vmsegptridx(newseg);
	if (!med_move_group(1, Cursegp, Curside, nsegp, AttachSide, vm_angles_2_matrix(pbh),0))
	{
		autosave_mine(mine_filename);

		med_propagate_tmaps_to_segments(Cursegp,nsegp,0);
		med_propagate_tmaps_to_back_side(nsegp, Side_opposite[AttachSide],0);
		copy_uvs_seg_to_seg(vmsegptr(&New_segment),nsegp);

		Cursegp = nsegp;
		Curside = Side_opposite[AttachSide];
		med_create_new_segment_from_cursegp();

		if (Lock_view_to_cursegp)
			set_view_target_from_segment(Cursegp);

		Update_flags |= UF_WORLD_CHANGED;
		mine_changed = 1;
		warn_if_concave_segment(Cursegp);
	}

	return 1;
}

int AttachSegmentNew(void)
{
	vms_angvec	pbh;

	pbh.p = 0;
	pbh.b = 0;
	pbh.h = 0;

	AttachSegmentNewAng(pbh);
	return 1;

}

//	-----------------------------------------------------------------------------
void validate_selected_segments(void)
{
	range_for (const auto &gs, GroupList[current_group].segments)
		validate_segment(vmsegptridx(gs));
}

// =====================================================================================


//	-----------------------------------------------------------------------------
namespace dsx {
void delete_segment_from_group(const vmsegptridx_t segment_num, unsigned group_num)
{
	segment_num->group = -1;
	GroupList[group_num].segments.erase(segment_num);
}
}
// =====================================================================================


//	-----------------------------------------------------------------------------
void add_segment_to_group(segnum_t segment_num, int group_num)
{  
	GroupList[group_num].segments.emplace_back(segment_num);
}
// =====================================================================================


//	-----------------------------------------------------------------------------
int rotate_segment_new(const vms_angvec &pbh)
{
	int			newseg_side;
	vms_matrix	tm1;
	group::segment_array_type_t selected_segs_save;
	int			child_save;
	int			current_group_save;

        if (!IS_CHILD(Cursegp->children[static_cast<int>(Side_opposite[Curside])]))
		// -- I don't understand this, MK, 01/25/94: if (Cursegp->children[Curside] != group_seg-Segments)
		{
			editor_status("Error -- unable to rotate group, Cursegp:Side_opposite[Curside] cannot be free.");
			return 1;
	}

	current_group_save = current_group;
	current_group = ROT_GROUP;
	Groupsegp[ROT_GROUP] = Cursegp;
	
	selected_segs_save = GroupList[current_group].segments;
	GroupList[ROT_GROUP].segments.clear();
	const auto newseg = Cursegp;
	newseg_side = Side_opposite[Curside];

	// Create list of segments to rotate.
	//	Sever connection between first seg to rotate and its connection on Side_opposite[Curside].
	child_save = Cursegp->children[newseg_side];	// save connection we are about to sever
	Cursegp->children[newseg_side] = segment_none;			// sever connection
	create_group_list(Cursegp, GroupList[ROT_GROUP].segments, NULL);       // create list of segments in group
	Cursegp->children[newseg_side] = child_save;	// restore severed connection
	GroupList[ROT_GROUP].segments.emplace_back(newseg);

	const auto baseseg = newseg->children[newseg_side];
	if (!IS_CHILD(baseseg)) {
		editor_status("Error -- unable to rotate segment, side opposite curside is not attached.");
		GroupList[current_group].segments = selected_segs_save;
		current_group = current_group_save;
		return 1;
	}
	const auto &&basesegp = vmsegptridx(baseseg);
	const auto &&baseseg_side = find_connect_side(newseg, basesegp);

	med_extract_matrix_from_segment(newseg, tm1);
	tm1 = vmd_identity_matrix;
	const auto tm2 = vm_angles_2_matrix(pbh);
	const auto orient_matrix = vm_matrix_x_matrix(tm1,tm2);

	basesegp->children[baseseg_side] = segment_none;
	newseg->children[newseg_side] = segment_none;

	if (!med_move_group(1, basesegp, baseseg_side, newseg, newseg_side, orient_matrix, 0))
	{
		Cursegp = newseg;
		med_create_new_segment_from_cursegp();
//		validate_selected_segments();
		med_propagate_tmaps_to_segments(basesegp, newseg, 1);
		med_propagate_tmaps_to_back_side(newseg, Curside, 1);
	}

	GroupList[current_group].segments = selected_segs_save;
	current_group = current_group_save;

	return 1;
}

//	-----------------------------------------------------------------------------
//	Attach segment in the new-fangled way, which is by using the CopyGroup code.
int RotateSegmentNew(vms_angvec *pbh)
{
	int	rval;

	autosave_mine(mine_filename);

	rval = rotate_segment_new(*pbh);

	if (Lock_view_to_cursegp)
		set_view_target_from_segment(Cursegp);

	Update_flags |= UF_WORLD_CHANGED;
	mine_changed = 1;
	warn_if_concave_segment(Cursegp);

	return rval;
}

#if 0
static array<d_fname, MAX_TEXTURES> current_tmap_list;

// -----------------------------------------------------------------------------
// Save mine will:
// 1. Write file info, header info, editor info, vertex data, segment data,
//    and new_segment in that order, marking their file offset.
// 2. Go through all the fields and fill in the offset, size, and sizeof
//    values in the headers.
static int med_save_group( const char *filename, const group::vertex_array_type_t &vertex_ids, const group::segment_array_type_t &segment_ids)
{
	int header_offset, editor_offset, vertex_offset, segment_offset, texture_offset;
	char ErrorMessage[100];
	int j;

	auto SaveFile = PHYSFSX_openWriteBuffered(filename);
	if (!SaveFile)
	{
		snprintf(ErrorMessage, sizeof(ErrorMessage), "ERROR: Unable to open %s\n", filename);
		ui_messagebox( -2, -2, 1, ErrorMessage, "Ok" );
		return 1;
	}

	//===================== SAVE FILE INFO ========================

	group_fileinfo.fileinfo_version  =   MINE_VERSION;
	group_fileinfo.fileinfo_sizeof   =   sizeof(group_fileinfo);
	group_fileinfo.header_offset     =   -1;
	group_fileinfo.header_size       =   sizeof(group_header);
	group_fileinfo.editor_offset     =   -1;
	group_fileinfo.editor_size       =   sizeof(group_editor);
	group_fileinfo.vertex_offset     =   -1;
	group_fileinfo.vertex_howmany    =   vertex_ids.size();
	group_fileinfo.vertex_sizeof     =   sizeof(vms_vector);
	group_fileinfo.segment_offset    =   -1;
	group_fileinfo.segment_howmany   =   segment_ids.size();
	group_fileinfo.segment_sizeof    =   sizeof(segment);
	group_fileinfo.texture_offset    =   -1;
	group_fileinfo.texture_howmany   =   0;
	group_fileinfo.texture_sizeof    =   13;  // num characters in a name

	// Write the fileinfo
	PHYSFS_write( SaveFile, &group_fileinfo, sizeof(group_fileinfo), 1);

	//===================== SAVE HEADER INFO ========================

	group_header.num_vertices        =   vertex_ids.size();
	group_header.num_segments        =   segment_ids.size();

	// Write the editor info
	header_offset = PHYSFS_tell(SaveFile);
	PHYSFS_write( SaveFile, &group_header, sizeof(group_header), 1);

	//===================== SAVE EDITOR INFO ==========================
	group_editor.newsegment_offset   =   -1; // To be written
	group_editor.newsegment_size     =   sizeof(segment);
	// Next 3 vars added 10/07 by JAS
	group_editor.Groupsegp =   0;
	if (Groupsegp[current_group]) {
		const auto i = segment_ids.find(vmsegptridx(Groupsegp[current_group]));
		if (i != segment_ids.end())
			group_editor.Groupsegp = std::distance(segment_ids.begin(), i);
	} 
	group_editor.Groupside		 =   Groupside[current_group];

	editor_offset = PHYSFS_tell(SaveFile);
	PHYSFS_write( SaveFile, &group_editor, sizeof(group_editor), 1);


	//===================== SAVE VERTEX INFO ==========================

	vertex_offset = PHYSFS_tell(SaveFile);
	range_for (const auto &gv, vertex_ids)
	{
		const vertex tvert = *vcvertptr(gv);
		PHYSFS_write(SaveFile, &tvert, sizeof(tvert), 1);
	}

	//===================== SAVE SEGMENT INFO =========================


	segment_offset = PHYSFS_tell(SaveFile);
	range_for (const auto &gs, segment_ids)
	{
		auto &&tseg = *vmsegptr(gs);
		
		for (j=0;j<6;j++)	{
			group::segment_array_type_t::const_iterator i = segment_ids.find(tseg.children[j]);
			tseg.children[j] = (i == segment_ids.end()) ? segment_none : std::distance(segment_ids.begin(), i);
		}

		for (j=0;j<8;j++)
		{
			group::vertex_array_type_t::const_iterator i = vertex_ids.find(tseg.verts[j]);
			if (i != vertex_ids.end())
				tseg.verts[j] = std::distance(vertex_ids.begin(), i);
		}
		PHYSFS_write( SaveFile, &tseg, sizeof(tseg), 1);

	 }

	//===================== SAVE TEXTURE INFO ==========================

	texture_offset = PHYSFS_tell(SaveFile);

	for (unsigned i = 0, n = NumTextures; i < n; ++i)
	{
		current_tmap_list[i] = TmapInfo[i].filename;
		PHYSFS_write(SaveFile, current_tmap_list[i].data(), current_tmap_list[i].size(), 1);
	}

	//============= REWRITE FILE INFO, TO SAVE OFFSETS ===============

	// Update the offset fields
	group_fileinfo.header_offset     =   header_offset;
	group_fileinfo.editor_offset     =   editor_offset;
	group_fileinfo.vertex_offset     =   vertex_offset;
	group_fileinfo.segment_offset    =   segment_offset;
	group_fileinfo.texture_offset    =   texture_offset;
	
	// Write the fileinfo
	PHYSFSX_fseek(  SaveFile, 0, SEEK_SET );  // Move to TOF
	PHYSFS_write( SaveFile, &group_fileinfo, sizeof(group_fileinfo), 1);

	//==================== CLOSE THE FILE =============================
	return 0;
}

static array<d_fname, MAX_TEXTURES> old_tmap_list;
// static short tmap_xlate_table[MAX_TEXTURES]; // ZICO - FIXME

// -----------------------------------------------------------------------------
// Load group will:
//int med_load_group(char * filename)
static int med_load_group( const char *filename, group::vertex_array_type_t &vertex_ids, group::segment_array_type_t &segment_ids)
{
	int vertnum;
	char ErrorMessage[200];
	short tmap_xlate;
        int     translate=0;
	char 	*temptr;
	segment tseg;
	auto LoadFile = PHYSFSX_openReadBuffered(filename);
	if (!LoadFile)
	{
		snprintf(ErrorMessage, sizeof(ErrorMessage), "ERROR: Unable to open %s\n", filename);
		ui_messagebox( -2, -2, 1, ErrorMessage, "Ok" );
		return 1;
	}

	//===================== READ FILE INFO ========================

	// These are the default values... version and fileinfo_sizeof
	// don't have defaults.
	group_fileinfo.header_offset     =   -1;
	group_fileinfo.header_size       =   sizeof(group_header);
	group_fileinfo.editor_offset     =   -1;
	group_fileinfo.editor_size       =   sizeof(group_editor);
	group_fileinfo.vertex_offset     =   -1;
	group_fileinfo.vertex_howmany    =   0;
	group_fileinfo.vertex_sizeof     =   sizeof(vms_vector);
	group_fileinfo.segment_offset    =   -1;
	group_fileinfo.segment_howmany   =   0;
	group_fileinfo.segment_sizeof    =   sizeof(segment);
	group_fileinfo.texture_offset    =   -1;
	group_fileinfo.texture_howmany   =   0;
	group_fileinfo.texture_sizeof    =   13;  // num characters in a name

	// Read in group_top_fileinfo to get size of saved fileinfo.

	if (PHYSFSX_fseek( LoadFile, 0, SEEK_SET ))
		Error( "Error seeking to 0 in group.c" );

	if (PHYSFS_read( LoadFile, &group_top_fileinfo, sizeof(group_top_fileinfo),1 )!=1)
		Error( "Error reading top_fileinfo in group.c" );

	// Check version number
	if (group_top_fileinfo.fileinfo_version < COMPATIBLE_VERSION )
	{
		snprintf(ErrorMessage, sizeof(ErrorMessage), "You are trying to load %s\n" \
						  "a version %d group, which is known to be incompatible\n" \
						  "with the current expected version %d groups.", \
						  filename, group_top_fileinfo.fileinfo_version, MINE_VERSION );

		if (ui_messagebox( -2, -2, 2, ErrorMessage, "Forget it", "Try anyway" )==1)
		{
			return 1;
		}

		ui_messagebox( -2, -2, 1, "Good luck!", "I need it" );
	}

	// Now, Read in the fileinfo

	if (PHYSFSX_fseek( LoadFile, 0, SEEK_SET ))
		Error( "Error seeking to 0b in group.c" );

	if (PHYSFS_read( LoadFile, &group_fileinfo, group_top_fileinfo.fileinfo_sizeof,1 )!=1)
		Error( "Error reading group_fileinfo in group.c" );

	//===================== READ HEADER INFO ========================

	// Set default values.
	group_header.num_vertices        =   0;
	group_header.num_segments        =   0;

	if (group_fileinfo.header_offset > -1 )
	{
		if (PHYSFSX_fseek( LoadFile,group_fileinfo.header_offset, SEEK_SET ))
			Error( "Error seeking to header_offset in group.c" );

		if (PHYSFS_read( LoadFile, &group_header, group_fileinfo.header_size,1 )!=1)
			Error( "Error reading group_header in group.c" );
	}

	//===================== READ EDITOR INFO ==========================

	// Set default values
	group_editor.current_seg         =   0;
	group_editor.newsegment_offset   =   -1; // To be written
	group_editor.newsegment_size     =   sizeof(segment);
	group_editor.Groupsegp				=   -1;
	group_editor.Groupside				=   0;

	if (group_fileinfo.editor_offset > -1 )
	{
		if (PHYSFSX_fseek( LoadFile,group_fileinfo.editor_offset, SEEK_SET ))
			Error( "Error seeking to editor_offset in group.c" );

		if (PHYSFS_read( LoadFile, &group_editor, group_fileinfo.editor_size,1 )!=1)
			Error( "Error reading group_editor in group.c" );

	}

	//===================== READ VERTEX INFO ==========================

	if ( (group_fileinfo.vertex_offset > -1) && (group_fileinfo.vertex_howmany > 0))
	{
		if (PHYSFSX_fseek( LoadFile,group_fileinfo.vertex_offset, SEEK_SET ))
			Error( "Error seeking to vertex_offset in group.c" );

		vertex_ids.clear();
			for (unsigned i = 0; i< group_header.num_vertices; ++i)
			{
				vertex tvert;
				if (PHYSFS_read( LoadFile, &tvert, sizeof(tvert),1 )!=1)
					Error( "Error reading tvert in group.c" );
				vertex_ids.emplace_back(med_create_duplicate_vertex(tvert));
			}

		}

	//==================== READ SEGMENT INFO ===========================

	if ( (group_fileinfo.segment_offset > -1) && (group_fileinfo.segment_howmany > 0))
	{
		if (PHYSFSX_fseek( LoadFile,group_fileinfo.segment_offset, SEEK_SET ))
			Error( "Error seeking to segment_offset in group.c" );

		segment_ids.clear();
		for (unsigned i = 0; i < group_header.num_segments; ++i)
		{
			if (PHYSFS_read( LoadFile, &tseg, sizeof(segment),1 )!=1)
				Error( "Error reading tseg in group.c" );
				
			group::segment_array_type_t::value_type s = get_free_segment_number();
			segment_ids.emplace_back(s);
			const auto &&segp = vmsegptridx(s);
			*segp = tseg; 
			segp->objects = object_none;

			fuelcen_activate(segp);
			}

		range_for (const auto &gs, segment_ids)
		{
			auto &segp = *vmsegptr(gs);
			// Fix vertices
			range_for (auto &j, segp.verts)
			{
				vertnum = vertex_ids[j];
				j = vertnum;
				}

			// Fix children and walls.
			for (unsigned j = 0; j < MAX_SIDES_PER_SEGMENT; ++j)
			{
				Segments[gs].sides[j].wall_num = wall_none;
				if (IS_CHILD(Segments[gs].children[j])) {
					segnum_t segnum;
					segnum = segment_ids[Segments[gs].children[j]];
					Segments[gs].children[j] = segnum;
					} 
				//Translate textures.
				if (translate == 1) {
					int	temp;
					tmap_xlate = Segments[gs].sides[j].tmap_num;
					Segments[gs].sides[j].tmap_num = tmap_xlate_table[tmap_xlate];
					temp = Segments[gs].sides[j].tmap_num2;
					tmap_xlate = temp & 0x3fff;			// strip off orientation bits
					if (tmap_xlate != 0)
                                                Segments[gs].sides[j].tmap_num2 = (temp & (~0x3fff)) | tmap_xlate_table[tmap_xlate];  // mask on original orientation bits
					}
				}
			}
	}
	
	//===================== READ TEXTURE INFO ==========================

	if ( (group_fileinfo.texture_offset > -1) && (group_fileinfo.texture_howmany > 0))
	{
		if (PHYSFSX_fseek( LoadFile, group_fileinfo.texture_offset, SEEK_SET ))
			Error( "Error seeking to texture_offset in gamemine.c" );

		range_for (auto &i, partial_range(old_tmap_list, group_fileinfo.texture_howmany))
		{
			array<char, FILENAME_LEN> a;
			if (PHYSFS_read(LoadFile, a.data(), std::min(static_cast<size_t>(group_fileinfo.texture_sizeof), a.size()), 1) != 1)
				Error( "Error reading old_tmap_list[i] in gamemine.c" );
			i.copy_if(a);
		}
	}

	//=============== GENERATE TEXTURE TRANSLATION TABLE ===============

	translate = 0;
	
	Assert (NumTextures < MAX_TEXTURES);
{
	hashtable ht;
	// Remove all the file extensions in the textures list

	for (unsigned i = 0; i < NumTextures; ++i)
	{
		temptr = strchr(&TmapInfo[i].filename[0u], '.');
		if (temptr) *temptr = '\0';
		hashtable_insert( &ht, &TmapInfo[i].filename[0u], i );
	}

	// For every texture, search through the texture list
	// to find a matching name.
	for (unsigned j = 0; j < group_fileinfo.texture_howmany; ++j)
	{
		// Remove this texture name's extension
		temptr = strchr(&old_tmap_list[j][0u], '.');
		if (temptr) *temptr = '\0';

		tmap_xlate_table[j] = hashtable_search( &ht, static_cast<const char *>(old_tmap_list[j]));
		if (tmap_xlate_table[j]	< 0 )
			tmap_xlate_table[j] = 0;
		if (tmap_xlate_table[j] != j ) translate = 1;
	}
}


	//======================== CLOSE FILE ==============================
	LoadFile.reset();

	//========================= UPDATE VARIABLES ======================

	if (group_editor.Groupsegp != -1 ) 
		Groupsegp[current_group] = &Segments[segment_ids[group_editor.Groupsegp]];
	else
		Groupsegp[current_group] = NULL;

	Groupside[current_group] = group_editor.Groupside;

	warn_if_concave_segments();
	
	return 0;
}

static char group_filename[PATH_MAX] = "*.GRP";

static void checkforgrpext( char * f )
{
	int i;

	for (i=1; f[i]; i++ )
	{
		if (f[i]=='.') return;

		if ((f[i]==' '||f[i]==0) )
		{
			f[i]='.';
			f[i+1]='G';
			f[i+2]= 'R';
			f[i+3]= 'P';
			f[i+4]=0;
			return;
		}
	}

	if (i < 123)
	{
		f[i]='.';
		f[i+1]='G';
		f[i+2]= 'R';
		f[i+3]= 'P';
		f[i+4]=0;
		return;
	}
}
#endif

//short vertex_list[MAX_VERTICES];


int SaveGroup()
{
	ui_messagebox(-2, -2, 1, "ERROR: Groups are broken.", "Ok");
	return 0;
#if 0
	// Save group
	int i;

	if (current_group == -1)
		{
		ui_messagebox(-2, -2, 1, "ERROR: No current group.", "Ok");
 		return 0;
		}

	array<int8_t, MAX_VERTICES> vertex_list{};

	//	Make a list of all vertices in group.
	range_for (const auto &gs, GroupList[current_group].segments)
		range_for (auto &v, Segments[gs].verts)
		{
			vertex_list[v] = 1;
		}	

	GroupList[current_group].vertices.clear();
	for (i=0; i<=Highest_vertex_index; i++) 
		if (vertex_list[i] == 1) { 
			GroupList[current_group].vertices.emplace_back(i);
		}
	med_save_group("TEMP.GRP", GroupList[current_group].vertices, GroupList[current_group].segments);
   if (ui_get_filename( group_filename, "*.GRP", "SAVE GROUP" ))
	{
      checkforgrpext(group_filename);
		if (med_save_group(group_filename, GroupList[current_group].vertices, GroupList[current_group].segments))
			return 0;
		mine_changed = 0;
	}
	
	return 1;
#endif
}


int LoadGroup()
{
	ui_messagebox(-2, -2, 1, "ERROR: Groups are broken.", "Ok");
	return 0;
#if 0
	int x;

	if (num_groups == MAX_GROUPS)
		{
		x = ui_messagebox( -2, -2, 2, "Warning: You are about to wipe out a group.", "ARGH! NO!", "No problemo." );
		if (x==1) return 0;
		}

	if (num_groups < MAX_GROUPS)
		{
		num_groups++;
		current_group = num_groups-1;
		}
		else current_group = 0;

   if (ui_get_filename( group_filename, "*.GRP", "LOAD GROUP" ))
	{
      checkforgrpext(group_filename);
	  med_load_group(group_filename, GroupList[current_group].vertices, GroupList[current_group].segments);
		
	if (!med_move_group(0, Cursegp, Curside, vmsegptridx(Groupsegp[current_group]), Groupside[current_group], vmd_identity_matrix, 0))
	{
		autosave_mine(mine_filename);
		set_view_target_from_segment(Cursegp);
		Update_flags |= UF_WORLD_CHANGED;
		mine_changed = 1;
		diagnostic_message("Group moved.");
		return 0;
		} else
	return 1;
	}	else

	return 1;
#endif
}


int UngroupSegment( void )
{
	if (Cursegp->group == current_group) {
	
		Cursegp->group = -1;
		delete_segment_from_group(Cursegp, current_group);
	
	   Update_flags |= UF_WORLD_CHANGED;
	   mine_changed = 1;
	   diagnostic_message_fmt("Segment Ungrouped from Group %d.", current_group);
	
		return 1;
	} else
	return 0;
}

int GroupSegment( void )
{
	if (Cursegp->group == -1) {

		Cursegp->group = current_group;
		add_segment_to_group(Cursegp, current_group);
	
	   Update_flags |= UF_WORLD_CHANGED;
	   mine_changed = 1;
	   diagnostic_message_fmt("Segment Added to Group %d.", current_group);

		return 1;
	} else
	return 0;
}

int Degroup( void )
{
	int i;

//	GroupList[current_group].num_segments = 0;
//	Groupsegp[current_group] = 0;

	if (num_groups==0) return 0;

	range_for (const auto &gs, GroupList[current_group].segments)
		delete_segment_from_group(vmsegptridx(gs), current_group);

	  //	delete_segment_from_group( &Segments[GroupList[current_group].segments[i]]-Segments, current_group );

	for (i=current_group;i<num_groups-1;i++)
		{
		GroupList[i] = GroupList[i+1];
		Groupsegp[i] = Groupsegp[i+1];
		}

	num_groups--;

	GroupList[num_groups].segments.clear();
	Groupsegp[num_groups] = 0;
	
	if (current_group > num_groups-1) current_group--;

	if (num_groups == 0)
		current_group = -1;

   if (Lock_view_to_cursegp)
       set_view_target_from_segment(Cursegp);
   Update_flags |= UF_WORLD_CHANGED;
   mine_changed = 1;
   diagnostic_message("Group UNgrouped.");

	return 1;
}

int NextGroup( void ) 
{

	if (num_groups > 0)
		{
		current_group++;
		if (current_group >= num_groups ) current_group = 0;
		
		Update_flags |= UF_ED_STATE_CHANGED;
		mine_changed = 1;
		}
	else editor_status("No Next Group\n");
	return 0;
}

int PrevGroup( void ) 
{
	if (num_groups > 0)
		{
		current_group--;
		if (current_group < 0 ) current_group = num_groups-1;
		
		Update_flags |= UF_ED_STATE_CHANGED;
		mine_changed = 1;
		}
	else editor_status("No Previous Group\n");
	return 0;
}


//	-----------------------------------------------------------------------------
int MoveGroup(void)
{
	if (!Groupsegp[current_group]) {
		editor_status("Error -- Cannot move group, no group segment.");
		return 1;
	}

	med_compress_mine();

	if (!med_move_group(0, Cursegp, Curside, vmsegptridx(Groupsegp[current_group]), Groupside[current_group], vmd_identity_matrix, 0))
	{
		autosave_mine(mine_filename);
		Update_flags |= UF_WORLD_CHANGED;
		mine_changed = 1;
		diagnostic_message("Group moved.");
		return 0;
	} else
		return 1;
}				  


//	-----------------------------------------------------------------------------
int CopyGroup(void)
{
	segnum_t	attach_seg;

	if (!Groupsegp[current_group]) {
		editor_status("Error -- Cannot copy group, no group segment.");
		return 1;
	}

	//	See if the attach side in the group is attached to another segment.
	//	If so, it must not be in the group for group copy to be legal.
	attach_seg = Groupsegp[current_group]->children[Groupside[current_group]];
	if (attach_seg != segment_none) {
		if (GroupList[current_group].segments.contains(attach_seg)) {
			editor_status_fmt("Error -- Cannot copy group, attach side has a child (segment %i) attached.", attach_seg);
			return 1;
		}
	}

	med_compress_mine();

	if (!med_copy_group(0, Cursegp, Curside, vcsegptr(Groupsegp[current_group]), Groupside[current_group], vmd_identity_matrix))
	{
		autosave_mine(mine_filename);
		Update_flags |= UF_WORLD_CHANGED;
		mine_changed = 1;
		diagnostic_message("Group copied.");
		return 0;
	} else	  
		return 1;
}


//	-----------------------------------------------------------------------------
int RotateGroup(void)
{

	if (!Groupsegp[current_group]) {
		editor_status("Error -- Cannot rotate group, no group segment.");
		return 1;
	}

	Group_orientation[current_group]++;
	if ((Group_orientation[current_group] <0) || (Group_orientation[current_group] >4))
		Group_orientation[current_group]=0;

	med_compress_mine();
	
	if (!med_move_group(0, Cursegp, Curside, vmsegptridx(Groupsegp[current_group]), Groupside[current_group],
								vmd_identity_matrix, Group_orientation[current_group]))
			{
			Update_flags |= UF_WORLD_CHANGED;
			mine_changed = 1;
			diagnostic_message("Group rotated.");
			return 0;
			} 
		else	  
			return 1;
}


//	-----------------------------------------------------------------------------
//	Creates a group from all segments connected to marked segment.
int SubtractFromGroup(void)
{
	int	x, original_group;
	if (!Markedsegp) {
		editor_status("Error -- Cannot create group, no marked segment.");
		return 1;
	}

	med_compress_mine();
	autosave_mine(mine_filename);

	if (num_groups == MAX_GROUPS) {
		x = ui_messagebox( -2, -2, 2, "Warning: You are about to wipe out a group.", "ARGH! NO!", "No problemo." );
		if (x==1) return 0;
	}					   

	if (current_group == -1) {
		editor_status("Error -- No current group.  Cannot subtract.");
		return 1;
	}

	original_group = current_group;

	current_group = (current_group + 1) % MAX_GROUPS;

	//	Create a list of segments to copy.
	GroupList[current_group].segments.clear();
	create_group_list(Markedsegp, GroupList[current_group].segments, &Selected_segs);

	//	Now, scan the two groups, forming a group which consists of only those segments common to the two groups.
	auto intersects = [original_group](group::segment_array_type_t::const_reference r) -> bool {
		bool contains = GroupList[original_group].segments.contains(r);
		if (!contains)
			Segments[r].group = -1;
		return !contains;
	};
	GroupList[current_group].segments.erase_if(intersects);

	// Replace Marked segment with Group Segment.
	Groupsegp[current_group] = Markedsegp;
	Groupside[current_group] = Markedside;

	range_for (const auto &gs, GroupList[current_group].segments)
		Segments[gs].group = current_group;
	
	Update_flags |= UF_WORLD_CHANGED;
	mine_changed = 1;
	diagnostic_message("Group created.");

	return 1; 
				  
}

//	-----------------------------------------------------------------------------
//	Creates a group from all segments already in CurrentGroup which can be reached from marked segment
//	without passing through current segment.
int CreateGroup(void)
{
	int x;

	if (!Markedsegp) {
		editor_status("Error -- Cannot create group, no marked segment.");
		return 1;
	}

	med_compress_mine();
	autosave_mine(mine_filename);

	if (num_groups == MAX_GROUPS) {
		x = ui_messagebox( -2, -2, 2, "Warning: You are about to wipe out a group.", "ARGH! NO!", "No problemo." );
		if (x==1)
			return 0;				// Aborting at user's request.
	}					   

	if (num_groups < MAX_GROUPS) {
		num_groups++;
		current_group = num_groups-1;
	} else
		current_group = 0;

	//	Create a list of segments to copy.
	GroupList[current_group].clear();
	create_group_list(Markedsegp, GroupList[current_group].segments, NULL);
	
	// Replace Marked segment with Group Segment.
	Groupsegp[current_group] = Markedsegp;
	Groupside[current_group] = Markedside;
//	Markedsegp = 0;
//	Markedside = WBACK;

	range_for (const auto &gs, GroupList[current_group].segments)
		Segments[gs].group = current_group;
	
	Update_flags |= UF_WORLD_CHANGED;
	mine_changed = 1;
	diagnostic_message("Group created.");

	return 1; 
				  
}

//	-----------------------------------------------------------------------------
// Deletes current group.
int DeleteGroup( void )
{
	int i;

	autosave_mine(mine_filename);
		
	if (num_groups==0) return 0;

	range_for (const auto &gs, GroupList[current_group].segments)
	{
		const auto &&segp = vmsegptridx(gs);
		segp->group = -1;
		med_delete_segment(segp);
	}

	for (i=current_group;i<num_groups-1;i++) {
		GroupList[i] = GroupList[i+1];
		Groupsegp[i] = Groupsegp[i+1];
	}

	num_groups--;
	GroupList[num_groups].clear();
	Groupsegp[num_groups] = 0;

	if (current_group > num_groups-1) current_group--;

	if (num_groups==0)
		current_group = -1;

	undo_status[Autosave_count] = "Delete Group UNDONE.";
   if (Lock_view_to_cursegp)
       set_view_target_from_segment(Cursegp);

   Update_flags |= UF_WORLD_CHANGED;
   mine_changed = 1;
   diagnostic_message("Group deleted.");
   // warn_if_concave_segments();     // This could be faster -- just check if deleted segment was concave, warn accordingly

	return 1;

}


int MarkGroupSegment( void )
{
	if ((Cursegp->group != -1) && (Cursegp->group == current_group))
		{
	   autosave_mine(mine_filename);
		Groupsegp[current_group] = Cursegp;
		Groupside[current_group] = Curside;
		editor_status("Group Segment Marked.");
		Update_flags |= UF_ED_STATE_CHANGED;
		undo_status[Autosave_count] = "Mark Group Segment UNDONE.";
		mine_changed = 1;
		return 1;
		}
	else return 0;
}
