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
 * Functions for building parts of mines.
 *
 */

#include <string.h>
#include "inferno.h"
#include "editor/editor.h"
#include "editor/esegment.h"
#include "gameseg.h"
#include "gamesave.h"

//  ---------- Create a bridge segment between current segment/side and marked segment/side ----------
int CreateBridge()
{
    if (!med_form_bridge_segment(Cursegp,Curside,Markedsegp,Markedside)) {
		Update_flags |= UF_WORLD_CHANGED;
		mine_changed = 1;
    	autosave_mine(mine_filename);
    	diagnostic_message("Bridge segment formed.");
    	strcpy(undo_status[Autosave_count], "Bridge segment UNDONE.");
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
            strcpy(undo_status[Autosave_count], "Joint undone.");
    			warn_if_concave_segments();
        }
	}

	return 1;

}

//  ---------- Create a bridge segment between current segment:side adjacent segment:side ----------
int CreateAdjacentJoint()
{
	int		adj_side;
	segment	*adj_sp;

	if (med_find_adjacent_segment_side(Cursegp, Curside, &adj_sp, &adj_side)) {
		if (Cursegp->children[Curside] != adj_sp-Segments) {
			med_form_joint(Cursegp,Curside,adj_sp,adj_side);
			Update_flags |= UF_WORLD_CHANGED;
			mine_changed = 1;
         autosave_mine(mine_filename);
         diagnostic_message("Joint segment formed.");
         strcpy(undo_status[Autosave_count], "Joint segment undone.");
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
	int		adj_side;
	segment	*adj_sp;

	save_level("SLOPPY.LVL");

	if (med_find_closest_threshold_segment_side(Cursegp, Curside, &adj_sp, &adj_side, 20*F1_0)) {
		if (Cursegp->children[Curside] != adj_sp-Segments) {
			if (!med_form_joint(Cursegp,Curside,adj_sp,adj_side))
				{
				Update_flags |= UF_WORLD_CHANGED;
				mine_changed = 1;
	         autosave_mine(mine_filename);
	         diagnostic_message("Sloppy Joint segment formed.");
	         strcpy(undo_status[Autosave_count], "Sloppy Joint segment undone.");
	    		warn_if_concave_segments();
				}
			else editor_status("Couldn't form sloppy joint.\n");
		} else
			editor_status("Attempted to form sloppy joint through connected side -- joint segment not formed (you bozo).");
	} else
		editor_status("Could not find close threshold segment -- joint segment not formed.");

	return 1;
}


//  -------------- Create all sloppy joints within CurrentGroup ------------------
int CreateSloppyAdjacentJointsGroup()
{
	int		adj_side;
	segment	*adj_sp;
	int		num_segs = GroupList[current_group].num_segments;
	short		*segs = GroupList[current_group].segments;
	segment	*segp;
	int		done_been_a_change = 0;
	int		segind, sidenum;

	for (segind=0; segind<num_segs; segind++) {
		segp = &Segments[segs[segind]];

		for (sidenum=0; sidenum < MAX_SIDES_PER_SEGMENT; sidenum++)
			if (!IS_CHILD(segp->children[sidenum]))
				if (med_find_closest_threshold_segment_side(segp, sidenum, &adj_sp, &adj_side, 5*F1_0)) {
					if (adj_sp->group == segp->group) {
						if (segp->children[sidenum] != adj_sp-Segments)
							if (!med_form_joint(segp, sidenum, adj_sp,adj_side))
								done_been_a_change = 1;
					}
				}
	}

	if (done_been_a_change) {
		Update_flags |= UF_WORLD_CHANGED;
		mine_changed = 1;
		autosave_mine(mine_filename);
		diagnostic_message("Sloppy Joint segment formed.");
		strcpy(undo_status[Autosave_count], "Sloppy Joint segment undone.");
		warn_if_concave_segments();
	}

	return 1;
}


//  ---------- Create a bridge segment between current segment and all adjacent segment:side ----------
int CreateAdjacentJointsSegment()
{
	int		adj_side,s;
	segment	*adj_sp;

	med_combine_duplicate_vertices(Vertex_active);

	for (s=0; s<MAX_SIDES_PER_SEGMENT; s++) {
		if (med_find_adjacent_segment_side(Cursegp, s, &adj_sp, &adj_side))
			if (Cursegp->children[s] != adj_sp-Segments)
					{
					med_form_joint(Cursegp,s,adj_sp,adj_side);
					Update_flags |= UF_WORLD_CHANGED;
					mine_changed = 1;
	            autosave_mine(mine_filename);
	            diagnostic_message("Adjacent Joint segment formed.");
	            strcpy(undo_status[Autosave_count], "Adjacent Joint segment UNDONE.");
	    			warn_if_concave_segments();
					}
	}

	return 1;
}

//  ---------- Create a bridge segment between all segment:side and all adjacent segment:side ----------
int CreateAdjacentJointsAll()
{
	int		adj_side,seg,s;
	segment	*adj_sp;

	med_combine_duplicate_vertices(Vertex_active);

	for (seg=0; seg<=Highest_segment_index; seg++)
		for (s=0; s<MAX_SIDES_PER_SEGMENT; s++)
			if (med_find_adjacent_segment_side(&Segments[seg], s, &adj_sp, &adj_side))
				if (Segments[seg].children[s] != adj_sp-Segments)
						med_form_joint(&Segments[seg],s,adj_sp,adj_side);

	Update_flags |= UF_WORLD_CHANGED;
	mine_changed = 1;
   autosave_mine(mine_filename);
   diagnostic_message("All Adjacent Joint segments formed.");
   strcpy(undo_status[Autosave_count], "All Adjacent Joint segments UNDONE.");
 	warn_if_concave_segments();
   return 1;
}


