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
 * Functions for building parts of mines.
 *
 */

#include <string.h>
#include "editor/editor.h"
#include "editor/esegment.h"
#include "gamesave.h"
#include "kdefs.h"
#include "compiler-range_for.h"

//  ---------- Create a bridge segment between current segment/side and marked segment/side ----------
int CreateBridge()
{
	if (!Markedsegp) {
		editor_status("No marked side.");
		return 0;
	}
	
    if (!med_form_bridge_segment(Cursegp,Curside,Markedsegp,Markedside)) {
		Update_flags |= UF_WORLD_CHANGED;
		mine_changed = 1;
    	autosave_mine(mine_filename);
    	diagnostic_message("Bridge segment formed.");
		undo_status[Autosave_count] = "Bridge segment UNDONE.";
    	warn_if_concave_segments();
	}
    return 1;
}



// ---------- Form a joint between current segment:side and marked segment:side, modifying marked segment ----------
int FormJoint()
{
	if (!Markedsegp)
		diagnostic_message("Marked segment not set -- unable to form joint.");
	else {
        if (!med_form_joint(Cursegp,Curside,Markedsegp,Markedside)) {
            Update_flags |= UF_WORLD_CHANGED;
            mine_changed = 1;
            autosave_mine(mine_filename);
            diagnostic_message("Joint formed.");
			undo_status[Autosave_count] = "Joint undone.";
    			warn_if_concave_segments();
        }
	}

	return 1;

}

//  ---------- Create a bridge segment between current segment:side adjacent segment:side ----------
int CreateAdjacentJoint()
{
	if (const auto o = med_find_adjacent_segment_side(Cursegp, Curside))
	{
		const auto [adj_sp, adj_side] = *o;
		if (Cursegp->children[Curside] != adj_sp) {
			med_form_joint(Cursegp,Curside,adj_sp,adj_side);
			Update_flags |= UF_WORLD_CHANGED;
			mine_changed = 1;
         autosave_mine(mine_filename);
         diagnostic_message("Joint segment formed.");
			undo_status[Autosave_count] = "Joint segment undone.";
    		warn_if_concave_segments();
		} else
			editor_status("Attempted to form joint through connected side -- joint segment not formed (you bozo).");
	} else
		editor_status("Could not find adjacent segment -- joint segment not formed.");

	return 1;
}

//  ---------- Create a bridge segment between current segment:side adjacent segment:side ----------
int CreateSloppyAdjacentJoint()
{
	save_level(
#if DXX_BUILD_DESCENT == 2
		LevelSharedSegmentState.DestructibleLights,
#endif
		"SLOPPY.LVL");

	if (const auto o = med_find_closest_threshold_segment_side(Cursegp, Curside, 20*F1_0))
	{
		const auto [adj_sp, adj_side] = *o;
		if (Cursegp->children[Curside] != adj_sp) {
			if (!med_form_joint(Cursegp,Curside,adj_sp,adj_side))
				{
				Update_flags |= UF_WORLD_CHANGED;
				mine_changed = 1;
	         autosave_mine(mine_filename);
	         diagnostic_message("Sloppy Joint segment formed.");
				undo_status[Autosave_count] = "Sloppy Joint segment undone.";
	    		warn_if_concave_segments();
				}
			else editor_status("Could not form sloppy joint.\n");
		} else
			editor_status("Attempted to form sloppy joint through connected side -- joint segment not formed.");
	} else
		editor_status("Could not find close threshold segment -- joint segment not formed.");

	return 1;
}


//  -------------- Create all sloppy joints within CurrentGroup ------------------
int CreateSloppyAdjacentJointsGroup()
{
	int		done_been_a_change = 0;
	range_for(const auto &gs, GroupList[current_group].segments)
	{
		auto segp = vmsegptridx(gs);

		for (const auto sidenum : MAX_SIDES_PER_SEGMENT)
			if (!IS_CHILD(segp->children[sidenum]))
			{
				if (const auto o = med_find_closest_threshold_segment_side(segp, sidenum, 5*F1_0))
				{
					const auto [adj_sp, adj_side] = *o;
					if (adj_sp->group == segp->group) {
						if (segp->children[sidenum] != adj_sp)
							if (!med_form_joint(segp, sidenum, adj_sp,adj_side))
								done_been_a_change = 1;
					}
				}
			}
	}

	if (done_been_a_change) {
		Update_flags |= UF_WORLD_CHANGED;
		mine_changed = 1;
		autosave_mine(mine_filename);
		diagnostic_message("Sloppy Joint segment formed.");
		undo_status[Autosave_count] = "Sloppy Joint segment undone.";
		warn_if_concave_segments();
	}

	return 1;
}


//  ---------- Create a bridge segment between current segment and all adjacent segment:side ----------
int CreateAdjacentJointsSegment()
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();

	auto &Vertex_active = LevelSharedVertexState.get_vertex_active();
	med_combine_duplicate_vertices(Vertex_active);

	for (const auto s : MAX_SIDES_PER_SEGMENT)
	{
		if (const auto o = med_find_adjacent_segment_side(Cursegp, s))
		{
			const auto [adj_sp, adj_side] = *o;
			if (Cursegp->children[s] != adj_sp)
					{
					med_form_joint(Cursegp,s,adj_sp,adj_side);
					Update_flags |= UF_WORLD_CHANGED;
					mine_changed = 1;
	            autosave_mine(mine_filename);
	            diagnostic_message("Adjacent Joint segment formed.");
				undo_status[Autosave_count] = "Adjacent Joint segment UNDONE.";
	    			warn_if_concave_segments();
					}
		}
	}

	return 1;
}

//  ---------- Create a bridge segment between all segment:side and all adjacent segment:side ----------
int CreateAdjacentJointsAll()
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();

	auto &Vertex_active = LevelSharedVertexState.get_vertex_active();
	med_combine_duplicate_vertices(Vertex_active);

	range_for (const auto &&segp, vmsegptridx)
	{
		for (const auto s : MAX_SIDES_PER_SEGMENT)
		{
			if (const auto o = med_find_adjacent_segment_side(segp, s))
			{
				const auto [adj_sp, adj_side] = *o;
				if (segp->children[s] != adj_sp)
						med_form_joint(segp,s,adj_sp,adj_side);
			}
		}
	}

	Update_flags |= UF_WORLD_CHANGED;
	mine_changed = 1;
   autosave_mine(mine_filename);
   diagnostic_message("All Adjacent Joint segments formed.");
	undo_status[Autosave_count] = "All Adjacent Joint segments UNDONE.";
 	warn_if_concave_segments();
   return 1;
}


