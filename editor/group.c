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
#include "dxxerror.h"
#include "gamemine.h"
#include "gameseg.h"
#include "bm.h"				// For MAX_TEXTURES.
#include "textures.h"
#include "hash.h"
#include "fuelcen.h"

#include "medwall.h"

void validate_selected_segments(void);

struct {
	int     fileinfo_version;
	int     fileinfo_sizeof;
} group_top_fileinfo;    // Should be same as first two fields below...

struct {
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
	int     texture_howmany;
	int     texture_sizeof;
} group_fileinfo;

struct {
	int     num_vertices;
	int     num_segments;
} group_header;

struct {
	int     current_seg;
	int     newsegment_offset;
	int     newsegment_size;
	int     Groupsegp;
	int     Groupside;
} group_editor;

group		GroupList[MAX_GROUPS+1];
segment  *Groupsegp[MAX_GROUPS+1];
int		Groupside[MAX_GROUPS+1];
int		Group_orientation[MAX_GROUPS+1];
int		current_group=-1;
int		num_groups=0;

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
void med_create_group_rotation_matrix(vms_matrix *result_mat, int delta_flag, segment *first_seg, int first_side, segment *base_seg, int base_side, vms_matrix *orient_matrix, int orientation)
{
	vms_matrix	rotmat2,rotmat,rotmat3,rotmat4;
	vms_angvec	pbh = {0,0,0};

	//	Determine whether this rotation is a delta rotation, meaning to just rotate in place, or an absolute rotation,
	//	which means that the destination rotation is specified, not as a delta, but as an absolute
	if (delta_flag) {
	 	//	Create rotation matrix describing rotation.
	 	med_extract_matrix_from_segment(first_seg, &rotmat4);		// get rotation matrix describing current orientation of first seg
		set_matrix_based_on_side(&rotmat4, first_side);
		rotmat3 = *orient_matrix;
		vm_transpose_matrix(&rotmat3);
	 	vm_matrix_x_matrix(&rotmat,&rotmat4,&rotmat3);			// this is the desired orientation of the new segment
		vm_transpose_matrix(&rotmat4);
	 	vm_matrix_x_matrix(&rotmat2,&rotmat,&rotmat4);			// this is the desired orientation of the new segment
	} else {
	 	//	Create rotation matrix describing rotation.
 
	 	med_extract_matrix_from_segment(base_seg, &rotmat);		// get rotation matrix describing desired orientation
	 	set_matrix_based_on_side(&rotmat, base_side);				// modify rotation matrix for desired side
 
	 	//	If the new segment is to be attached without rotation, then its orientation is the same as the base_segment
	 	vm_matrix_x_matrix(&rotmat4,&rotmat,orient_matrix);			// this is the desired orientation of the new segment

		pbh.b = orientation*16384;
		vm_angles_2_matrix(&rotmat3,&pbh);
		vm_matrix_x_matrix(&rotmat, &rotmat4, &rotmat3);
		rotmat4 = rotmat;

	 	rotmat = rotmat4;

	 	med_extract_matrix_from_segment(first_seg, &rotmat3);		// get rotation matrix describing current orientation of first seg
 
	 	// It is curious that the following statement has no analogue in the med_attach_segment_rotated code.
	 	//	Perhaps it is because segments are always attached at their front side.  If the back side is the side
	 	//	passed to the function, then the matrix is not modified, which might suggest that what you need to do below
	 	//	is use Side_opposite[first_side].
	 	set_matrix_based_on_side(&rotmat3, Side_opposite[first_side]);				// modify rotation matrix for desired side
 
	 	vm_transpose_matrix(&rotmat3);								// get the inverse of the current orientation matrix
	 	vm_matrix_x_matrix(&rotmat2,&rotmat,&rotmat3);			// now rotmat2 takes the current segment to the desired orientation
	 	vm_transpose_matrix(&rotmat2);
	}

	*result_mat = rotmat2;

}

// -----------------------------------------------------------------------------------------
// Rotate all vertices and objects in group.
void med_rotate_group(vms_matrix *rotmat, short *group_seglist, int group_size, segment *first_seg, int first_side)
{
	int			v,s, objnum;
	sbyte			vertex_list[MAX_VERTICES];
	vms_vector	rotate_center;

	compute_center_point_on_side(&rotate_center, first_seg, first_side);

	//	Create list of points to rotate.
	for (v=0; v<=Highest_vertex_index; v++)
		vertex_list[v] = 0;

	for (s=0; s<group_size; s++) {
		segment *sp = &Segments[group_seglist[s]];

		for (v=0; v<MAX_VERTICES_PER_SEGMENT; v++)
			vertex_list[sp->verts[v]] = 1;

		//	Rotate center of all objects in group.
		objnum = sp->objects;
		while (objnum != -1) {
			vms_vector	tv, tv1;

			vm_vec_sub(&tv1,&Objects[objnum].pos,&rotate_center);
			vm_vec_rotate(&tv,&tv1,rotmat);
			vm_vec_add(&Objects[objnum].pos, &tv, &rotate_center);

			objnum = Objects[objnum].next;
		}			
	}

	// Do the pre-rotation xlate, do the rotation, do the post-rotation xlate
	for (v=0; v<=Highest_vertex_index; v++)
		if (vertex_list[v]) {
			vms_vector	tv,tv1;

			vm_vec_sub(&tv1,&Vertices[v],&rotate_center);
			vm_vec_rotate(&tv,&tv1,rotmat);
			vm_vec_add(&Vertices[v],&tv,&rotate_center);

		}

}


// ------------------------------------------------------------------------------------------------
void cgl_aux(segment *segp, short *seglistp, int *num_segs, short *ignore_list, int num_ignore_segs)
{
	int	i, side;
	int	curseg = segp-Segments;

	for (i=0; i<num_ignore_segs; i++)
		if (curseg == ignore_list[i])
			return;

	if ((segp-Segments < 0) || (segp-Segments >= MAX_SEGMENTS)) {
		Int3();
	}

	if (!Been_visited[segp-Segments]) {
		seglistp[(*num_segs)++] = segp-Segments;
		Been_visited[segp-Segments] = 1;

		for (side=0; side<MAX_SIDES_PER_SEGMENT; side++)
			if (IS_CHILD(segp->children[side]))
				cgl_aux(&Segments[segp->children[side]], seglistp, num_segs, ignore_list, num_ignore_segs);
	}
}

// ------------------------------------------------------------------------------------------------
//	Sets Been_visited[n] if n is reachable from segp
void create_group_list(segment *segp, short *seglistp, int *num_segs, short *ignore_list, int num_ignore_segs)
{
	int	i;

	for (i=0; i<MAX_SEGMENTS; i++)
		Been_visited[i] = 0;

	cgl_aux(segp, seglistp, num_segs, ignore_list, num_ignore_segs);
}


#define MXS MAX_SEGMENTS
#define MXV MAX_VERTICES

// ------------------------------------------------------------------------------------------------
void duplicate_group(sbyte *vertex_ids, short *segment_ids, int num_segments)
{
	int	v,s,ss,new_vertex_id,new_segment_id,sidenum;
	short	new_segment_ids[MAX_SEGMENTS];
	int	new_vertex_ids[MAX_VERTICES];		// If new_vertex_ids[v] != -1, then vertex v has been remapped to new_vertex_ids[v]

	//	duplicate vertices
	for (v=0; v<MXV; v++)
		new_vertex_ids[v] = -1;

	//	duplicate vertices
	for (v=0; v<=Highest_vertex_index; v++) {
		if (vertex_ids[v]) {
			new_vertex_id = med_create_duplicate_vertex(&Vertices[v]);
			new_vertex_ids[v] = new_vertex_id;
		}
	}

	//	duplicate segments
	for (s=0; s<num_segments; s++) {
		int	objnum;

		new_segment_id = med_create_duplicate_segment(&Segments[segment_ids[s]]);
		new_segment_ids[s] = new_segment_id;
		objnum = Segments[new_segment_id].objects;
		Segments[new_segment_id].objects = -1;
		while (objnum != -1) {
			if (Objects[objnum].type != OBJ_PLAYER) {
				int new_obj_id;
				new_obj_id = obj_create_copy(objnum, &Objects[objnum].pos, new_segment_id);
				(void)new_obj_id; // FIXME!
			}
			objnum = Objects[objnum].next;
		}
	}

	//	Now, for each segment in segment_ids, correct its children numbers by translating through new_segment_ids
	//	and correct its vertex numbers by translating through new_vertex_ids
	for (s=0; s<num_segments; s++) {
		segment *sp = &Segments[new_segment_ids[s]];
		for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++) {
			int seg = sp->children[sidenum];
			if (IS_CHILD(seg)) {
				for (ss=0; ss<num_segments; ss++) {
					if (seg == segment_ids[ss])
						Segments[new_segment_ids[s]].children[sidenum] = new_segment_ids[ss];
				}
			}
		}	// end for (sidenum=0...

		//	Now fixup vertex ids
		for (v=0; v<MAX_VERTICES_PER_SEGMENT; v++) {
			if (vertex_ids[sp->verts[v]]) {
				sp->verts[v] = new_vertex_ids[sp->verts[v]];
			}
		}
	}	// end for (s=0...

	//	Now, copy new_segment_ids into segment_ids
	for (s=0; s<num_segments; s++) {
		segment_ids[s] = new_segment_ids[s];
	}

	//	Now, copy new_vertex_ids into vertex_ids
	for (v=0; v<MXV; v++)
		vertex_ids[v] = 0;

	for (v=0; v<MXV; v++) {
		if (new_vertex_ids[v] != -1)
			vertex_ids[new_vertex_ids[v]] = 1;

	}
}


// ------------------------------------------------------------------------------------------------
int in_group(int segnum, int group_num)
{
	int	i;

	for (i=0; i<GroupList[group_num].num_segments; i++)
		if (segnum == GroupList[group_num].segments[i])
			return 1;

	return 0;
}

// ------------------------------------------------------------------------------------------------
//	Copy a group of segments.
//	The group is defined as all segments accessible from group_seg.
//	The group is copied so group_seg:group_side is incident upon base_seg:base_side.
//	group_seg and its vertices are bashed to coincide with base_seg.
//	If any vertex of base_seg is contained in a segment that is reachable from group_seg, then errror.
int med_copy_group(int delta_flag, segment *base_seg, int base_side, segment *group_seg, int group_side, vms_matrix *orient_matrix)
{
	int			v,s;
	vms_vector	srcv,destv;
	int 			x;
	int			new_current_group;
	segment		*segp;
	int 			c;
        int                     gs_index=0;
	sbyte			in_vertex_list[MAX_VERTICES];
	vms_matrix	rotmat;
	int			objnum;

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
	for (s=0;s<GroupList[current_group].num_segments;s++)
		if (GroupList[current_group].segments[s] == (Groupsegp[current_group]-Segments))
			gs_index=s; 

	GroupList[new_current_group] = GroupList[current_group];

	//	Make a list of all vertices in group.
	if (group_seg == &New_segment)
		for (v=0; v<MAX_VERTICES_PER_SEGMENT; v++)
			in_vertex_list[group_seg->verts[v]] = 1;
	else {
		for (v=0; v<=Highest_vertex_index; v++)
			in_vertex_list[v] = 0;

		for (s=0; s<GroupList[new_current_group].num_segments; s++)
			for (v=0; v<MAX_VERTICES_PER_SEGMENT; v++)
				in_vertex_list[Segments[GroupList[new_current_group].segments[s]].verts[v]] = 1;
	}

	// Given a list of vertex indices (indicated by !0 in in_vertex_list) and segment indices (in list GroupList[current_group].segments, there
	//	are GroupList[current_group].num_segments segments), copy all segments and vertices
	//	Return updated lists of vertices and segments in in_vertex_list and GroupList[current_group].segments
	duplicate_group(in_vertex_list, GroupList[new_current_group].segments, GroupList[new_current_group].num_segments);

	//group_seg = &Segments[GroupList[new_current_group].segments[0]];					// connecting segment in group has been changed, so update group_seg

   Groupsegp[new_current_group] = group_seg = &Segments[GroupList[new_current_group].segments[gs_index]];
	Groupside[new_current_group] = Groupside[current_group];

	for (s=0; s<GroupList[new_current_group].num_segments; s++) {
		Segments[GroupList[new_current_group].segments[s]].group = new_current_group;
		Segments[GroupList[new_current_group].segments[s]].special = SEGMENT_IS_NOTHING;
		Segments[GroupList[new_current_group].segments[s]].matcen_num = -1;
	}

	// Breaking connections between segments in the current group and segments not in the group.
	for (s=0; s<GroupList[new_current_group].num_segments; s++) {
		segp = &Segments[GroupList[new_current_group].segments[s]];
		for (c=0; c<MAX_SIDES_PER_SEGMENT; c++) 
			if (IS_CHILD(segp->children[c])) {
				if (!in_group(segp->children[c], new_current_group)) {
					segp->children[c] = -1;
					validate_segment_side(segp,c);					// we have converted a connection to a side so validate the segment
				}
			}
	}

	copy_uvs_seg_to_seg(&New_segment, Groupsegp[new_current_group]);
	
	//	Now do the copy
	//	First, xlate all vertices so center of group_seg:group_side is at origin
	compute_center_point_on_side(&srcv,group_seg,group_side);
	for (v=0; v<=Highest_vertex_index; v++)
		if (in_vertex_list[v])
			vm_vec_sub2(&Vertices[v],&srcv);

	//	Now, translate all object positions.
	for (s=0; s<GroupList[new_current_group].num_segments; s++) {
		int	segnum = GroupList[new_current_group].segments[s];

		objnum = Segments[segnum].objects;

		while (objnum != -1) {
			vm_vec_sub2(&Objects[objnum].pos, &srcv);
			objnum = Objects[objnum].next;
		}
	}

	//	Now, rotate segments in group so orientation of group_seg is same as base_seg.
	med_create_group_rotation_matrix(&rotmat, delta_flag, group_seg, group_side, base_seg, base_side, orient_matrix, 0);
	med_rotate_group(&rotmat, GroupList[new_current_group].segments, GroupList[new_current_group].num_segments, group_seg, group_side);

	//	Now xlate all vertices so group_seg:group_side shares center point with base_seg:base_side
	compute_center_point_on_side(&destv,base_seg,base_side);
	for (v=0; v<=Highest_vertex_index; v++)
		if (in_vertex_list[v])
			vm_vec_add2(&Vertices[v],&destv);

	//	Now, xlate all object positions.
	for (s=0; s<GroupList[new_current_group].num_segments; s++) {
		int	segnum = GroupList[new_current_group].segments[s];
		int	objnum = Segments[segnum].objects;

		while (objnum != -1) {
			vm_vec_add2(&Objects[objnum].pos, &destv);
			objnum = Objects[objnum].next;
		}
	}

	//	Now, copy all walls (ie, doors, illusionary, etc.) into the new group.
	copy_group_walls(current_group, new_current_group);

	current_group = new_current_group;

	//	Now, form joint on connecting sides.
	med_form_joint(base_seg,base_side,Groupsegp[current_group],Groupside[new_current_group]);

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
int med_move_group(int delta_flag, segment *base_seg, int base_side, segment *group_seg, int group_side, vms_matrix *orient_matrix, int orientation)
{
	int			v,vv,s,ss,c,d;
	vms_vector	srcv,destv;
	segment		*segp, *csegp, *dsegp;
	sbyte			in_vertex_list[MAX_VERTICES], out_vertex_list[MAX_VERTICES];
	int			local_hvi;
	vms_matrix	rotmat;

	if (IS_CHILD(base_seg->children[base_side]))
		if (base_seg->children[base_side] != group_seg-Segments) {
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

	for (v=0; v<=Highest_vertex_index; v++) {
		in_vertex_list[v] = 0;
		out_vertex_list[v] = 0;
	}

	//	Make a list of all vertices in group.
	for (s=0; s<GroupList[current_group].num_segments; s++)
		for (v=0; v<MAX_VERTICES_PER_SEGMENT; v++)
			in_vertex_list[Segments[GroupList[current_group].segments[s]].verts[v]] = 1;

	//	For all segments which are not in GroupList[current_group].segments, mark all their vertices in the out list.
	for (s=0; s<=Highest_segment_index; s++) {
		for (ss=0; ss<GroupList[current_group].num_segments; ss++)
			if (GroupList[current_group].segments[ss] == s)
				break;
		if (ss == GroupList[current_group].num_segments)
			for (v=0; v<MAX_VERTICES_PER_SEGMENT; v++)
				out_vertex_list[Segments[s].verts[v]] = 1;
	}

	//	Now, for all vertices present in both the in (part of group segment) and out (part of non-group segment)
	// create an extra copy of the vertex so we can just move the ones in the in list.
	local_hvi = Highest_vertex_index;		//	Can't use Highest_vertex_index as loop termination because it gets increased by med_create_duplicate_vertex.

	for (v=0; v<=local_hvi; v++)
		if (in_vertex_list[v])
			if (out_vertex_list[v]) {
				int new_vertex_id;

				new_vertex_id = med_create_duplicate_vertex(&Vertices[v]);
				in_vertex_list[v] = 0;
				in_vertex_list[new_vertex_id] = 1;

				// Create a new vertex and assign all occurrences of vertex v in IN list to new vertex number.
				for (s=0; s<GroupList[current_group].num_segments; s++) {
					segment *sp = &Segments[GroupList[current_group].segments[s]];
					for (vv=0; vv<MAX_VERTICES_PER_SEGMENT; vv++)
						if (sp->verts[vv] == v)
							sp->verts[vv] = new_vertex_id;
				}
			}

	for (s=0;s<GroupList[current_group].num_segments;s++)
		Segments[GroupList[current_group].segments[s]].group = current_group;

	// Breaking connections between segments in the group and segments not in the group.
	for (s=0; s<GroupList[current_group].num_segments; s++)
		{
		segp = &Segments[GroupList[current_group].segments[s]];
		for (c=0; c<MAX_SIDES_PER_SEGMENT; c++) 
			if (IS_CHILD(segp->children[c]))
				{
				csegp = &Segments[segp->children[c]];
				if (csegp->group != current_group)
					{
					for (d=0; d<MAX_SIDES_PER_SEGMENT; d++)
						if (IS_CHILD(csegp->children[d]))
							{
							dsegp = &Segments[csegp->children[d]];
							if (dsegp->group == current_group)
								{
								csegp->children[d] = -1;
								validate_segment_side(csegp,d);					// we have converted a connection to a side so validate the segment
								}
							}
					segp->children[c] = -1;
					validate_segment_side(segp,c);					// we have converted a connection to a side so validate the segment
					}
				}
		}

	copy_uvs_seg_to_seg(&New_segment, Groupsegp[current_group]);

	//	Now do the move
	//	First, xlate all vertices so center of group_seg:group_side is at origin
	compute_center_point_on_side(&srcv,group_seg,group_side);
	for (v=0; v<=Highest_vertex_index; v++)
		if (in_vertex_list[v])
			vm_vec_sub2(&Vertices[v],&srcv);

	//	Now, move all object positions.
	for (s=0; s<GroupList[current_group].num_segments; s++) {
		int	segnum = GroupList[current_group].segments[s];
		int	objnum = Segments[segnum].objects;

		while (objnum != -1) {
			vm_vec_sub2(&Objects[objnum].pos, &srcv);
			objnum = Objects[objnum].next;
		}
	}

	//	Now, rotate segments in group so orientation of group_seg is same as base_seg.
	med_create_group_rotation_matrix(&rotmat, delta_flag, group_seg, group_side, base_seg, base_side, orient_matrix, orientation);
	med_rotate_group(&rotmat, GroupList[current_group].segments, GroupList[current_group].num_segments, group_seg, group_side);

	//	Now xlate all vertices so group_seg:group_side shares center point with base_seg:base_side
	compute_center_point_on_side(&destv,base_seg,base_side);
	for (v=0; v<=Highest_vertex_index; v++)
		if (in_vertex_list[v])
			vm_vec_add2(&Vertices[v],&destv);

	//	Now, rotate all object positions.
	for (s=0; s<GroupList[current_group].num_segments; s++) {
		int	segnum = GroupList[current_group].segments[s];
		int	objnum = Segments[segnum].objects;

		while (objnum != -1) {
			vm_vec_add2(&Objects[objnum].pos, &destv);
			objnum = Objects[objnum].next;
		}
	}

	//	Now, form joint on connecting sides.
	med_form_joint(base_seg,base_side,group_seg,group_side);

	validate_selected_segments();
	med_combine_duplicate_vertices(in_vertex_list);

	return 0;
}


//	-----------------------------------------------------------------------------
int place_new_segment_in_world(void)
{
	int	v,segnum;

	segnum = get_free_segment_number();

	Segments[segnum] = New_segment;

	for (v=0; v<MAX_VERTICES_PER_SEGMENT; v++)
		Segments[segnum].verts[v] = med_create_duplicate_vertex(&Vertices[New_segment.verts[v]]);

	return segnum;

}

//	-----------------------------------------------------------------------------
//	Attach segment in the new-fangled way, which is by using the CopyGroup code.
int AttachSegmentNewAng(vms_angvec *pbh)
{
	int			newseg;
	vms_matrix	orient_matrix;

	GroupList[current_group].num_segments = 1;
	newseg = place_new_segment_in_world();
	GroupList[current_group].segments[0] = newseg;

	if (!med_move_group(1, Cursegp, Curside, &Segments[newseg], AttachSide, vm_angles_2_matrix(&orient_matrix,pbh),0)) {
		autosave_mine(mine_filename);

		med_propagate_tmaps_to_segments(Cursegp,&Segments[newseg],0);
		med_propagate_tmaps_to_back_side(&Segments[newseg], Side_opposite[AttachSide],0);
		copy_uvs_seg_to_seg(&New_segment,&Segments[newseg]);

		Cursegp = &Segments[newseg];
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

	AttachSegmentNewAng(&pbh);
	return 1;

}

//	-----------------------------------------------------------------------------
void save_selected_segs(int *num, short *segs)
{
	int	i;

	for (i=0; i<GroupList[current_group].num_segments; i++)
		segs[i] = GroupList[current_group].segments[i];

	*num = GroupList[current_group].num_segments;
}

//	-----------------------------------------------------------------------------
void restore_selected_segs(int num, short *segs)
{
	int	i;

	for (i=0; i<GroupList[current_group].num_segments; i++)
		GroupList[current_group].segments[i] = segs[i];

	GroupList[current_group].num_segments = num;
}

//	-----------------------------------------------------------------------------
void validate_selected_segments(void)
{
	int	i;

	for (i=0; i<GroupList[current_group].num_segments; i++)
		validate_segment(&Segments[GroupList[current_group].segments[i]]);
}

// =====================================================================================


//	-----------------------------------------------------------------------------
void delete_segment_from_group(int segment_num, int group_num)
{
	int g, del_seg_index;
	
	del_seg_index = -1;
	for (g=0; g<GroupList[group_num].num_segments; g++)
		if (segment_num == GroupList[group_num].segments[g]) {  
			del_seg_index = g;
			break;
		}

	if (IS_CHILD(del_seg_index)) {
		for (g=del_seg_index;g<GroupList[group_num].num_segments-1;g++) { 
			GroupList[group_num].segments[g] = GroupList[group_num].segments[g+1];
			}
		GroupList[group_num].num_segments--;
		Segments[segment_num].group = -1;		
		}

}
// =====================================================================================


//	-----------------------------------------------------------------------------
void add_segment_to_group(int segment_num, int group_num)
{  
	GroupList[group_num].num_segments++;
	GroupList[group_num].segments[GroupList[group_num].num_segments-1] = segment_num;
}
// =====================================================================================


//	-----------------------------------------------------------------------------
int rotate_segment_new(vms_angvec *pbh)
{
	int			newseg,baseseg,newseg_side,baseseg_side;
	vms_matrix	orient_matrix,tm1,tm2;
	int			n_selected_segs_save;
	short			selected_segs_save[MAX_SEGMENTS];
	int			child_save;
	int			current_group_save;

        if (!IS_CHILD(Cursegp->children[(int) Side_opposite[Curside]])) {
		// -- I don't understand this, MK, 01/25/94: if (Cursegp->children[Curside] != group_seg-Segments) {
			editor_status("Error -- unable to rotate group, Cursegp:Side_opposite[Curside] cannot be free.");
			return 1;
	}

	current_group_save = current_group;
	current_group = ROT_GROUP;
	Groupsegp[ROT_GROUP] = Cursegp;
	
	save_selected_segs(&n_selected_segs_save, selected_segs_save);
	GroupList[ROT_GROUP].num_segments = 0;
	newseg = Cursegp - Segments;
	newseg_side = Side_opposite[Curside];

	// Create list of segments to rotate.
	//	Sever connection between first seg to rotate and its connection on Side_opposite[Curside].
	child_save = Cursegp->children[newseg_side];	// save connection we are about to sever
	Cursegp->children[newseg_side] = -1;			// sever connection
	create_group_list(Cursegp, GroupList[ROT_GROUP].segments, &GroupList[ROT_GROUP].num_segments, Selected_segs, 0);       // create list of segments in group
	Cursegp->children[newseg_side] = child_save;	// restore severed connection
	GroupList[ROT_GROUP].segments[0] = newseg;

	baseseg = Segments[newseg].children[newseg_side];
	if (!IS_CHILD(baseseg)) {
		editor_status("Error -- unable to rotate segment, side opposite curside is not attached.");
		restore_selected_segs(n_selected_segs_save,selected_segs_save);
		current_group = current_group_save;
		return 1;
	}

	baseseg_side = find_connect_side(&Segments[newseg], &Segments[baseseg]);

	med_extract_matrix_from_segment(&Segments[newseg],&tm1);
	tm1 = vmd_identity_matrix;
	vm_angles_2_matrix(&tm2,pbh);
	vm_matrix_x_matrix(&orient_matrix,&tm1,&tm2);

	Segments[baseseg].children[baseseg_side] = -1;
	Segments[newseg].children[newseg_side] = -1;

	if (!med_move_group(1, &Segments[baseseg], baseseg_side, &Segments[newseg], newseg_side, &orient_matrix, 0)) {
		Cursegp = &Segments[newseg];
		med_create_new_segment_from_cursegp();
//		validate_selected_segments();
		med_propagate_tmaps_to_segments(&Segments[baseseg], &Segments[newseg], 1);
		med_propagate_tmaps_to_back_side(&Segments[newseg], Curside, 1);
	}

	restore_selected_segs(n_selected_segs_save,selected_segs_save);
	current_group = current_group_save;

	return 1;
}

//	-----------------------------------------------------------------------------
//	Attach segment in the new-fangled way, which is by using the CopyGroup code.
int RotateSegmentNew(vms_angvec *pbh)
{
	int	rval;

	autosave_mine(mine_filename);

	rval = rotate_segment_new(pbh);

	if (Lock_view_to_cursegp)
		set_view_target_from_segment(Cursegp);

	Update_flags |= UF_WORLD_CHANGED;
	mine_changed = 1;
	warn_if_concave_segment(Cursegp);

	return rval;
}

static char	 current_tmap_list[MAX_TEXTURES][13];

// -----------------------------------------------------------------------------
// Save mine will:
// 1. Write file info, header info, editor info, vertex data, segment data,
//    and new_segment in that order, marking their file offset.
// 2. Go through all the fields and fill in the offset, size, and sizeof
//    values in the headers.
int med_save_group( char *filename, int *vertex_ids, short *segment_ids, int num_vertices, int num_segments)
{
	PHYSFS_file * SaveFile;
	int header_offset, editor_offset, vertex_offset, segment_offset, texture_offset;
	char ErrorMessage[100];
	int i, j, k;
	int segnum;
	segment tseg;
   vms_vector tvert;
	int found;

	SaveFile = PHYSFSX_openWriteBuffered( filename );
	if (!SaveFile)
	{
		sprintf( ErrorMessage, "ERROR: Unable to open %s\n", filename );
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
	group_fileinfo.vertex_howmany    =   num_vertices;
	group_fileinfo.vertex_sizeof     =   sizeof(vms_vector);
	group_fileinfo.segment_offset    =   -1;
	group_fileinfo.segment_howmany   =   num_segments;
	group_fileinfo.segment_sizeof    =   sizeof(segment);
	group_fileinfo.texture_offset    =   -1;
	group_fileinfo.texture_howmany   =   0;
	group_fileinfo.texture_sizeof    =   13;  // num characters in a name

	// Write the fileinfo
	PHYSFS_write( SaveFile, &group_fileinfo, sizeof(group_fileinfo), 1);

	//===================== SAVE HEADER INFO ========================

	group_header.num_vertices        =   num_vertices;
	group_header.num_segments        =   num_segments;

	// Write the editor info
	header_offset = PHYSFS_tell(SaveFile);
	PHYSFS_write( SaveFile, &group_header, sizeof(group_header), 1);

	//===================== SAVE EDITOR INFO ==========================
	group_editor.newsegment_offset   =   -1; // To be written
	group_editor.newsegment_size     =   sizeof(segment);
	// Next 3 vars added 10/07 by JAS
	if (Groupsegp[current_group]) {
		segnum = Groupsegp[current_group]-Segments;
		for (i=0;i<num_segments;i++)
			if (segnum == segment_ids[i])	
				group_editor.Groupsegp = i;
	} 
	else
		group_editor.Groupsegp      	=   0;
	group_editor.Groupside		 =   Groupside[current_group];

	editor_offset = PHYSFS_tell(SaveFile);
	PHYSFS_write( SaveFile, &group_editor, sizeof(group_editor), 1);


	//===================== SAVE VERTEX INFO ==========================

	vertex_offset = PHYSFS_tell(SaveFile);
	for (i=0;i<num_vertices;i++) {
		tvert = Vertices[vertex_ids[i]];	
		PHYSFS_write( SaveFile, &tvert, sizeof(tvert), 1); 
	}

	//===================== SAVE SEGMENT INFO =========================


	segment_offset = PHYSFS_tell(SaveFile);
	for (i=0;i<num_segments;i++) {
		tseg = Segments[segment_ids[i]];
		
		for (j=0;j<6;j++)	{
			found = 0;
			for (k=0;k<num_segments;k++) 
				if (tseg.children[j] == segment_ids[k]) { 
					tseg.children[j] = k;
					found = 1;
					break;
					}	
			if (found==0) tseg.children[j] = -1;
		}

		for (j=0;j<8;j++)
			for (k=0;k<num_vertices;k++)
				if (tseg.verts[j] == vertex_ids[k])	{
					tseg.verts[j] = k;
					break;
					}

		PHYSFS_write( SaveFile, &tseg, sizeof(tseg), 1);

	 }

	//===================== SAVE TEXTURE INFO ==========================

	texture_offset = PHYSFS_tell(SaveFile);

	for (i=0;i<NumTextures;i++)
		strncpy(current_tmap_list[i], TmapInfo[i].filename, 13);

	PHYSFS_write( SaveFile, current_tmap_list, 13, NumTextures);

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
	PHYSFS_close(SaveFile);

	return 0;

}

static char old_tmap_list[MAX_TEXTURES][13];
// static short tmap_xlate_table[MAX_TEXTURES]; // ZICO - FIXME

// -----------------------------------------------------------------------------
// Load group will:
//int med_load_group(char * filename)
int med_load_group( char *filename, int *vertex_ids, short *segment_ids, int *num_vertices, int *num_segments)
{
	int segnum, vertnum;
	char ErrorMessage[200];
	short tmap_xlate;
        int     translate=0;
	char 	*temptr;
	int i, j; 
	segment tseg;
   vms_vector tvert;
	PHYSFS_file * LoadFile;

	LoadFile = PHYSFSX_openReadBuffered( filename );
	if (!LoadFile)
	{
		sprintf( ErrorMessage, "ERROR: Unable to open %s\n", filename );
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
		sprintf( ErrorMessage, "ErrorMessage: You are trying to load %s\n" \
						  "a version %d group, which is known to be incompatible\n" \
						  "with the current expected version %d groups.", \
						  filename, group_top_fileinfo.fileinfo_version, MINE_VERSION );

		if (ui_messagebox( -2, -2, 2, ErrorMessage, "Forget it", "Try anyway" )==1)
		{
			PHYSFS_close( LoadFile );
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

			for (i=0;i<group_header.num_vertices;i++) {

				if (PHYSFS_read( LoadFile, &tvert, sizeof(tvert),1 )!=1)
					Error( "Error reading tvert in group.c" );
				vertex_ids[i] = med_create_duplicate_vertex( &tvert ); 
			}

		}

	//==================== READ SEGMENT INFO ===========================

	if ( (group_fileinfo.segment_offset > -1) && (group_fileinfo.segment_howmany > 0))
	{
		if (PHYSFSX_fseek( LoadFile,group_fileinfo.segment_offset, SEEK_SET ))
			Error( "Error seeking to segment_offset in group.c" );

		for (i=0;i<group_header.num_segments;i++) {
			if (PHYSFS_read( LoadFile, &tseg, sizeof(segment),1 )!=1)
				Error( "Error reading tseg in group.c" );
				
			segment_ids[i] = get_free_segment_number();
			Segments[segment_ids[i]] = tseg; 
			Segments[segment_ids[i]].objects = -1;

			fuelcen_activate( &Segments[segment_ids[i]], Segments[segment_ids[i]].special );
			}

		for (i=0;i<group_header.num_segments;i++) {
			// Fix vertices
			for (j=0;j<MAX_VERTICES_PER_SEGMENT;j++) {
				vertnum = vertex_ids[Segments[segment_ids[i]].verts[j]];
				Segments[segment_ids[i]].verts[j] = vertnum;
				}

			// Fix children and walls.
			for (j=0;j<MAX_SIDES_PER_SEGMENT;j++) {
				Segments[segment_ids[i]].sides[j].wall_num = -1;
				if (IS_CHILD(Segments[segment_ids[i]].children[j])) {
					segnum = segment_ids[Segments[segment_ids[i]].children[j]];
					Segments[segment_ids[i]].children[j] = segnum;
					} 
				//Translate textures.
				if (translate == 1) {
					int	temp;
					tmap_xlate = Segments[segment_ids[i]].sides[j].tmap_num;
					Segments[segment_ids[i]].sides[j].tmap_num = tmap_xlate_table[tmap_xlate];
					temp = Segments[segment_ids[i]].sides[j].tmap_num2;
					tmap_xlate = temp & 0x3fff;			// strip off orientation bits
					if (tmap_xlate != 0)
                                                Segments[segment_ids[i]].sides[j].tmap_num2 = (temp & (!0x3fff)) | tmap_xlate_table[tmap_xlate];  // mask on original orientation bits
					}
				}
			}
	}
	
	//===================== READ TEXTURE INFO ==========================

	if ( (group_fileinfo.texture_offset > -1) && (group_fileinfo.texture_howmany > 0))
	{
		if (PHYSFSX_fseek( LoadFile, group_fileinfo.texture_offset, SEEK_SET ))
			Error( "Error seeking to texture_offset in gamemine.c" );

		for (i=0; i< group_fileinfo.texture_howmany; i++ )
		{
			if (PHYSFS_read( LoadFile, &old_tmap_list[i], group_fileinfo.texture_sizeof, 1 )!=1)
				Error( "Error reading old_tmap_list[i] in gamemine.c" );
		}
	}

	//=============== GENERATE TEXTURE TRANSLATION TABLE ===============

	translate = 0;
	
	Assert (NumTextures < MAX_TEXTURES);
{
	hashtable ht;

	hashtable_init( &ht, NumTextures );

	// Remove all the file extensions in the textures list

	for (i=0;i<NumTextures;i++)	{
		temptr = strchr(TmapInfo[i].filename, '.');
		if (temptr) *temptr = '\0';
		hashtable_insert( &ht, TmapInfo[i].filename, i );
	}

	// For every texture, search through the texture list
	// to find a matching name.
	for (j=0;j<group_fileinfo.texture_howmany;j++) 	{
		// Remove this texture name's extension
		temptr = strchr(old_tmap_list[j], '.');
		if (temptr) *temptr = '\0';

		tmap_xlate_table[j] = hashtable_search( &ht,old_tmap_list[j]);
		if (tmap_xlate_table[j]	< 0 )
			tmap_xlate_table[j] = 0;
		if (tmap_xlate_table[j] != j ) translate = 1;
	}

	hashtable_free( &ht );
}


	//======================== CLOSE FILE ==============================
	PHYSFS_close( LoadFile );

	//========================= UPDATE VARIABLES ======================

	if (group_editor.Groupsegp != -1 ) 
		Groupsegp[current_group] = &Segments[segment_ids[group_editor.Groupsegp]];
	else
		Groupsegp[current_group] = NULL;

	Groupside[current_group] = group_editor.Groupside;

	*num_vertices = group_fileinfo.vertex_howmany;
	*num_segments = group_fileinfo.segment_howmany;
	warn_if_concave_segments();
	
	return 0;
}

char group_filename[PATH_MAX] = "*.GRP";

void checkforgrpext( char * f )
{
	int i;

	for (i=1; i<strlen(f); i++ )
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

//short vertex_list[MAX_VERTICES];


int SaveGroup()
{
	// Save group
	int i, s, v;
	char  ErrorMessage[200];
	sbyte	vertex_list[MAX_VERTICES];

	if (current_group == -1)
		{
		sprintf( ErrorMessage, "ERROR: No current group." );
		ui_messagebox( -2, -2, 1, ErrorMessage, "Ok" );
 		return 0;
		}

	for (v=0; v<=Highest_vertex_index; v++) {
		vertex_list[v] = 0;
	}

	//	Make a list of all vertices in group.
	for (s=0; s<GroupList[current_group].num_segments; s++)
		for (v=0; v<MAX_VERTICES_PER_SEGMENT; v++) {
			vertex_list[Segments[GroupList[current_group].segments[s]].verts[v]] = 1;
		}	

	v=0;
	for (i=0; i<=Highest_vertex_index; i++) 
		if (vertex_list[i] == 1) { 
			GroupList[current_group].vertices[v++] = i;
		}
	GroupList[current_group].num_vertices = v;
	med_save_group("TEMP.GRP", GroupList[current_group].vertices, GroupList[current_group].segments,
		GroupList[current_group].num_vertices, GroupList[current_group].num_segments);
   if (ui_get_filename( group_filename, "*.GRP", "SAVE GROUP" ))
	{
      checkforgrpext(group_filename);
		if (med_save_group(group_filename, GroupList[current_group].vertices, GroupList[current_group].segments,
					GroupList[current_group].num_vertices, GroupList[current_group].num_segments))
			return 0;
		mine_changed = 0;
	}
	
	return 1;
}


int LoadGroup()
{
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
      med_load_group(group_filename, GroupList[current_group].vertices, GroupList[current_group].segments,
					 &GroupList[current_group].num_vertices, &GroupList[current_group].num_segments) ;
		
	if (!med_move_group(0, Cursegp, Curside, Groupsegp[current_group], Groupside[current_group], &vmd_identity_matrix, 0)) {
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
}


int UngroupSegment( void )
{
	if (Cursegp->group == current_group) {
	
		Cursegp->group = -1;
		delete_segment_from_group( Cursegp-Segments, current_group );
	
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
		add_segment_to_group( Cursegp-Segments, current_group );
	
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

	for (i=0; i<GroupList[current_group].num_segments; i++)
		delete_segment_from_group( GroupList[current_group].segments[i], current_group );

	  //	delete_segment_from_group( &Segments[GroupList[current_group].segments[i]]-Segments, current_group );

	for (i=current_group;i<num_groups-1;i++)
		{
		GroupList[i] = GroupList[i+1];
		Groupsegp[i] = Groupsegp[i+1];
		}

	num_groups--;

	GroupList[num_groups].num_segments = 0;
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

void NextGroup( void ) 
{

	if (num_groups > 0)
		{
		current_group++;
		if (current_group >= num_groups ) current_group = 0;
		
		Update_flags |= UF_ED_STATE_CHANGED;
		mine_changed = 1;
		}
	else editor_status("No Next Group\n");
}

void PrevGroup( void ) 
{
	if (num_groups > 0)
		{
		current_group--;
		if (current_group < 0 ) current_group = num_groups-1;
		
		Update_flags |= UF_ED_STATE_CHANGED;
		mine_changed = 1;
		}
	else editor_status("No Previous Group\n");
}

// Returns:
//	 0 = successfully selected
//  1 = bad group number
int select_group( int num )
{
	if ((num>=0) && (num<num_groups))
		{
		current_group = num;
		return 0;
		}
	else return 1;
}


//	-----------------------------------------------------------------------------
int MoveGroup(void)
{
	if (!Groupsegp[current_group]) {
		editor_status("Error -- Cannot move group, no group segment.");
		return 1;
	}

	med_compress_mine();

	if (!med_move_group(0, Cursegp, Curside, Groupsegp[current_group], Groupside[current_group], &vmd_identity_matrix, 0)) {
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
	int	attach_seg;

	if (!Groupsegp[current_group]) {
		editor_status("Error -- Cannot copy group, no group segment.");
		return 1;
	}

	//	See if the attach side in the group is attached to another segment.
	//	If so, it must not be in the group for group copy to be legal.
	attach_seg = Groupsegp[current_group]->children[Groupside[current_group]];
	if (attach_seg != -1) {
		int	i;

		for (i=0; i<GroupList[current_group].num_segments; i++)
			if (GroupList[current_group].segments[i] == attach_seg)
				break;

		if (i != GroupList[current_group].num_segments) {
			editor_status_fmt("Error -- Cannot copy group, attach side has a child (segment %i) attached.", Groupsegp[current_group]->children[Groupside[current_group]]);
			return 1;
		}
	}

	med_compress_mine();

	if (!med_copy_group(0, Cursegp, Curside, Groupsegp[current_group], Groupside[current_group], &vmd_identity_matrix)) {
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
	
	if (!med_move_group(0, Cursegp, Curside, Groupsegp[current_group], Groupside[current_group],
								&vmd_identity_matrix, Group_orientation[current_group]))
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
	int	x, s, original_group;
	short	*gp;
	int	cur_num_segs;

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
	GroupList[current_group].num_segments = 0;
	create_group_list(Markedsegp, GroupList[current_group].segments, &GroupList[current_group].num_segments, Selected_segs, N_selected_segs);

	//	Now, scan the two groups, forming a group which consists of only those segments common to the two groups.
	gp = GroupList[current_group].segments;
	cur_num_segs = GroupList[current_group].num_segments;
	for (s=0; s<cur_num_segs; s++) {
		short	*gp1 = GroupList[original_group].segments;
		short	s0 = gp[s];
		int	s1;

		for (s1=0; s1<GroupList[original_group].num_segments; s1++)
			if (gp1[s1] == s0)
				break;				// If break executed, then segment found in both lists.

		//	If current segment was not found in both lists, remove it by copying the last segment over
		//	it and decreasing the number of segments.
		if (s1 == GroupList[original_group].num_segments) {
			gp[s] = gp[cur_num_segs];
			cur_num_segs--;
		}
	}

	//	Go through mine and seg group number of all segments which are in group
	//	All segments which were subtracted from group get group set to -1.
	for (s=0; s<cur_num_segs; s++) {
		Segments[GroupList[current_group].segments[s]].group = current_group;
	}

	for (s=0; s<=Highest_segment_index; s++) {
		int	t;
		if (Segments[s].group == current_group) {
			for (t=0; t<cur_num_segs; t++)
				if (GroupList[current_group].segments[t] == s)
					break;
			if (s == cur_num_segs) {
				Segments[s].group = -1;
			}
		}
	}

	GroupList[current_group].num_segments = cur_num_segs;

	// Replace Marked segment with Group Segment.
	Groupsegp[current_group] = Markedsegp;
	Groupside[current_group] = Markedside;

	for (x=0;x<GroupList[current_group].num_segments;x++)
		Segments[GroupList[current_group].segments[x]].group = current_group;
	
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
	GroupList[current_group].num_segments = 0;
	create_group_list(Markedsegp, GroupList[current_group].segments, &GroupList[current_group].num_segments, Selected_segs, 0);
	
	// Replace Marked segment with Group Segment.
	Groupsegp[current_group] = Markedsegp;
	Groupside[current_group] = Markedside;
//	Markedsegp = 0;
//	Markedside = WBACK;

	for (x=0;x<GroupList[current_group].num_segments;x++)
		Segments[GroupList[current_group].segments[x]].group = current_group;
	
	Update_flags |= UF_WORLD_CHANGED;
	mine_changed = 1;
	diagnostic_message("Group created.");

	return 1; 
				  
}

//	-----------------------------------------------------------------------------
// Deletes current group.
int DeleteGroup( void )
{
	int i, numsegs;

	autosave_mine(mine_filename);
		
	if (num_groups==0) return 0;

	numsegs = GroupList[current_group].num_segments;
	
	for (i=0; i<numsegs; i++) {
		med_delete_segment(&Segments[GroupList[current_group].segments[0]]);
	}

	for (i=current_group;i<num_groups-1;i++) {
		GroupList[i] = GroupList[i+1];
		Groupsegp[i] = Groupsegp[i+1];
	}

	num_groups--;
	GroupList[num_groups].num_segments = 0;
	Groupsegp[num_groups] = 0;

	if (current_group > num_groups-1) current_group--;

	if (num_groups==0)
		current_group = -1;

	strcpy(undo_status[Autosave_count], "Delete Group UNDONE.");
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
	   strcpy(undo_status[Autosave_count], "Mark Group Segment UNDONE.");
		mine_changed = 1;
		return 1;
		}
	else return 0;
}
