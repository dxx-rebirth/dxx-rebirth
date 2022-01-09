/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
#pragma once

#include "dxxsconf.h"

#include "segment.h"
#include "editor/editor.h"
#include <array>

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
extern imsegptridx_t Cursegp;				// Pointer to current segment in the mine, the one to which things happen.

#define	New_segment	(Segments.back())

extern sidenum_t Curside;					// Side index in 0..MAX_SIDES_PER_SEGMENT of active side.
extern	int		Curedge;					//	Current edge on current side, in 0..3
extern	int		Curvert;					//	Current vertex on current side, in 0..3
extern sidenum_t AttachSide;				//	Side on segment to attach
extern	int		Draw_all_segments;	// Set to 1 means draw_world draws all segments in Segments, else draw only connected segments
extern imsegptridx_t Markedsegp;			// Marked segment, used in conjunction with *Cursegp to form joints.
extern std::array<segment *, MAX_GROUPS+1> Groupsegp;
#endif
extern sidenum_t Markedside;				// Marked side on Markedsegp.

// The extra group in the following arrays is used for group rotation.
extern std::array<group, MAX_GROUPS+1> GroupList;
extern std::array<sidenum_t, MAX_GROUPS+1> Groupside;
extern	int 		current_group;
extern unsigned num_groups;
extern	int		Current_group;

struct found_segment_array_t : public count_segment_array_t {};

extern found_segment_array_t Found_segs;			// List of segment numbers "found" under cursor click

struct selected_segment_array_t : public count_segment_array_t {};

extern selected_segment_array_t Selected_segs;		// List of segment numbers currently selected

struct warning_segment_array_t : public count_segment_array_t {};

extern warning_segment_array_t Warning_segs;		// List of warning-worthy segments
