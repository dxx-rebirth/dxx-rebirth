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
 * $Source: /cvs/cvsroot/d2x/main/editor/kbuild.c,v $
 * $Revision: 1.1 $
 * $Author: btb $
 * $Date: 2004-12-19 13:54:27 $
 *
 * Functions for building parts of mines.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  1999/06/14 22:03:19  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:35:43  john
 * Version 2.0! No anonymous unions, Watcom 10.0, with no need
 * for bitmaps.tbl.
 * 
 * Revision 1.20  1995/02/22  11:00:47  yuan
 * prototype include.
 * 
 * Revision 1.19  1995/02/22  10:59:01  yuan
 * Save sloppy mine before punching.
 * 
 * Revision 1.18  1994/08/25  21:57:56  mike
 * IS_CHILD stuff.
 * 
 * Revision 1.17  1994/05/16  12:00:52  mike
 * Call med_combine_duplicate_vertices before various build functions.
 * 
 * Revision 1.16  1994/05/09  23:34:31  mike
 * Punch all sloppy sides in a group.
 * 
 * Revision 1.15  1994/02/16  15:23:06  yuan
 * Checking in for editor make.
 * 
 * Revision 1.14  1994/01/21  12:01:31  yuan
 * Added clearer editor_status messages (sloppy joint vs. joint)
 * 
 * Revision 1.13  1994/01/14  11:59:52  yuan
 * New function in build menu. 
 * "Punch" through walls to force a joint formation with
 * closest segment:side, if the closest segment:side allows
 * a connection.
 * 
 * Revision 1.12  1994/01/07  17:45:05  yuan
 * Just changed some tabs and formatting I believe.
 * 
 * Revision 1.11  1993/12/06  19:33:36  yuan
 * Fixed autosave stuff so that undo restores Cursegp and
 * Markedsegp
 * 
 * Revision 1.10  1993/12/02  12:39:15  matt
 * Removed extra includes
 * 
 * Revision 1.9  1993/11/12  14:31:31  yuan
 * Added warn_if_concave_segments.
 * 
 * Revision 1.8  1993/11/11  17:12:45  yuan
 * Fixed display of messages, so that concave segment
 * warning doesn't wipe them out immediately.
 * 
 * Revision 1.7  1993/11/09  12:09:28  mike
 * Remove extern for mine_filename, put it in editor.h
 * 
 * Revision 1.6  1993/11/08  19:14:06  yuan
 * Added Undo command (not working yet)
 * 
 * Revision 1.5  1993/11/05  17:32:36  john
 * added funcs
 * .,
 * 
 * Revision 1.4  1993/11/01  16:53:51  mike
 * Add CreateAdjacentJointsSegment and CreateAdjacentJointsAll
 * 
 * Revision 1.3  1993/11/01  11:24:59  mike
 * Add CreateJointAdjacent
 * 
 * Revision 1.2  1993/10/29  19:13:11  yuan
 * Added diagnostic messages
 * 
 * Revision 1.1  1993/10/13  18:53:27  john
 * Initial revision
 * 
 *
 */

#ifdef RCS
static char rcsid[] = "$Id: kbuild.c,v 1.1 2004-12-19 13:54:27 btb Exp $";
#endif

#include <string.h>

#include "inferno.h"
#include "editor/editor.h"
#include "gameseg.h"
#include "gamesave.h"
#include "mono.h"

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


