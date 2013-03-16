#pragma once
#include "segment.h"

extern	segment  *Cursegp;				// Pointer to current segment in the mine, the one to which things happen.
#define Curseg2p s2s2(Cursegp)          // Pointer to segment2 for Cursegp

// -- extern	segment  New_segment;			// The segment which can be added to the mine.
#define	New_segment	(Segments[MAX_SEGMENTS-1])

extern	int		Curside;					// Side index in 0..MAX_SIDES_PER_SEGMENT of active side.
extern	int		Curedge;					//	Current edge on current side, in 0..3
extern	int		Curvert;					//	Current vertex on current side, in 0..3
extern	int		AttachSide;				//	Side on segment to attach
extern	int		Draw_all_segments;	// Set to 1 means draw_world draws all segments in Segments, else draw only connected segments
extern	segment	*Markedsegp;			// Marked segment, used in conjunction with *Cursegp to form joints.
extern	int		Markedside;				// Marked side on Markedsegp.
extern  sbyte   Vertex_active[MAX_VERTICES]; // !0 means vertex is in use, 0 means not in use.

// The extra group in the following arrays is used for group rotation.
extern 	group		GroupList[MAX_GROUPS+1];
extern 	segment  *Groupsegp[MAX_GROUPS+1];
extern 	int		Groupside[MAX_GROUPS+1];
extern	int 		current_group;
extern	int 		num_groups; 
extern	int		Current_group;

extern	short		Found_segs[];			// List of segment numbers "found" under cursor click
extern	int		N_found_segs;			// Number of segments found at Found_segs

extern	int		N_selected_segs;		// Number of segments found at Selected_segs
extern	short		Selected_segs[];		// List of segment numbers currently selected

extern	int		N_warning_segs;		// Number of segments warning-worthy, such as a concave segment
extern	short		Warning_segs[];		// List of warning-worthy segments

extern 	int		Groupside[MAX_GROUPS+1];
extern	int 		current_group;
extern	int 		num_groups; 
extern	int		Current_group;

//	Returns true if vertex vi is contained in exactly one segment, else returns false.
extern int is_free_vertex(int vi);

//	Set existing vertex vnum to value *vp.
extern	int med_set_vertex(int vnum,vms_vector *vp);

extern void med_combine_duplicate_vertices(sbyte *vlp);

// Attach side newside of newseg to side destside of destseg.
// Copies *newseg into global array Segments, increments Num_segments.
// Forms a weld between the two segments by making the new segment fit to the old segment.
// Updates number of faces per side if necessitated by new vertex coordinates.
// Return value:
//  0 = successful attach
//  1 = No room in Segments[].
//  2 = No room in Vertices[].
extern	int med_attach_segment(segment *destseg, segment *newseg, int destside, int newside);

// Delete a segment.
// Deletes a segment from the global array Segments.
// Updates Cursegp to be the segment to which the deleted segment was connected.  If there is
//	more than one connected segment, the new Cursegp will be the segment with the highest index
//	of connection in the deleted segment (highest index = front)
// Return value:
//  0 = successful deletion
//  1 = unable to delete
extern	int med_delete_segment(segment *sp);

// Rotate the segment *seg by the pitch, bank, heading defined by *rot, destructively
// modifying its four free vertices in the global array Vertices.
// It is illegal to rotate a segment which has MAX_SIDES_PER_SEGMENT != 1.
// Pitch, bank, heading are about the point which is the average of the four points
// forming the side of connection.
// Return value:
//  0 = successful rotation
//  1 = MAX_SIDES_PER_SEGMENT makes rotation illegal (connected to 0 or 2+ segments)
//  2 = Rotation causes degeneracy, such as self-intersecting segment.
extern	int med_rotate_segment(segment *seg, vms_matrix *rotmat);
extern	int med_rotate_segment_ang(segment *seg, vms_angvec *ang);
