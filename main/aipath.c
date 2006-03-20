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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/aipath.c,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:42:49 $
 * 
 * AI path forming stuff.
 * 
 * $Log: aipath.c,v $
 * Revision 1.1.1.1  2006/03/17 19:42:49  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:05:15  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:30:48  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.101  1995/02/22  13:42:44  allender
 * remove anonymous unions for object structure
 * 
 * Revision 1.100  1995/02/10  16:20:04  mike
 * fix bogosity in create_path_points, assumed all objects were robots.
 * 
 * Revision 1.99  1995/02/07  21:09:30  mike
 * make run_from guys have diff level based speed.
 * 
 * Revision 1.98  1995/02/04  17:28:29  mike
 * make station guys return better.
 * 
 * Revision 1.97  1995/02/04  10:28:39  mike
 * fix compile error!
 * 
 * Revision 1.96  1995/02/04  10:03:37  mike
 * Fly to exit cheat.
 * 
 * Revision 1.95  1995/02/01  21:10:36  mike
 * Array name was dereferenced.  Not a bug, but unclean.
 * 
 * Revision 1.94  1995/02/01  17:14:12  mike
 * comment out some common mprintfs which didn't matter.
 * 
 * Revision 1.93  1995/01/30  13:01:23  mike
 * Make robots fire at player other than one they are controlled by sometimes.
 * 
 * Revision 1.92  1995/01/29  22:29:32  mike
 * add more debug info for guys that get lost.
 * 
 * Revision 1.91  1995/01/20  16:56:05  mike
 * station stuff.
 * 
 * Revision 1.90  1995/01/18  10:59:45  mike
 * comment out some mprintfs.
 * 
 * Revision 1.89  1995/01/17  16:58:34  mike
 * make path following work for multiplayer.
 * 
 * Revision 1.88  1995/01/17  14:21:44  mike
 * make run_from guys run better.
 * 
 * Revision 1.87  1995/01/14  17:09:04  mike
 * playing with crazy josh, he's kinda slow and dumb now.
 * 
 * Revision 1.86  1995/01/13  18:52:28  mike
 * comment out int3.
 * 
 * Revision 1.85  1995/01/05  09:42:11  mike
 * compile out code based on SHAREWARE.
 * 
 * Revision 1.84  1995/01/02  12:38:32  mike
 * make crazy josh turn faster, therefore evade player better.
 * 
 * Revision 1.83  1994/12/27  15:59:40  mike
 * tweak ai_multiplayer_awareness constants.
 * 
 * Revision 1.82  1994/12/19  17:07:10  mike
 * deal with new ai_multiplayer_awareness which returns a value saying whether this object can be moved by this player.
 * 
 * Revision 1.81  1994/12/15  13:04:30  mike
 * Replace Players[Player_num].time_total references with GameTime.
 * 
 * Revision 1.80  1994/12/09  16:13:23  mike
 * remove debug code.
 * 
 * Revision 1.79  1994/12/07  00:36:54  mike
 * make robots get out of matcens better and be aware of player.
 * 
 * Revision 1.78  1994/11/30  00:59:05  mike
 * optimizations.
 * 
 * Revision 1.77  1994/11/27  23:13:39  matt
 * Made changes for new mprintf calling convention
 * 
 * Revision 1.76  1994/11/23  21:59:34  mike
 * comment out some mprintfs.
 * 
 * Revision 1.75  1994/11/21  16:07:14  mike
 * flip PARALLAX flag, prevent annoying debug information.
 * 
 * Revision 1.74  1994/11/19  15:13:28  mike
 * remove unused code and data.
 * 
 * Revision 1.73  1994/11/17  14:53:15  mike
 * segment validation functions moved from editor to main.
 * 
 * Revision 1.72  1994/11/16  23:38:42  mike
 * new improved boss teleportation behavior.
 * 
 * Revision 1.71  1994/11/13  17:18:30  mike
 * debug code, then comment it out.
 * 
 * Revision 1.70  1994/11/11  16:41:43  mike
 * flip the PARALLAX flag.
 * 
 * Revision 1.69  1994/11/11  16:33:45  mike
 * twiddle the PARALLAX flag.
 * 
 * 
 * Revision 1.68  1994/11/10  21:32:29  mike
 * debug code.
 * 
 * Revision 1.67  1994/11/10  20:15:07  mike
 * fix stupid bug: uninitialized pointer.
 * 
 * Revision 1.66  1994/11/10  17:45:15  mike
 * debugging.
 * 
 * Revision 1.65  1994/11/10  17:28:10  mike
 * debugging.
 * 
 */

#ifdef RCS
static char rcsid[] = "$Id: aipath.c,v 1.1.1.1 2006/03/17 19:42:49 zicodxx Exp $";
#endif

#include <stdio.h>		//	for printf()
#include <stdlib.h>		// for d_rand() and qsort()
#include <string.h>		// for memset()

#include "inferno.h"
#include "mono.h"
#include "3d.h"

#include "object.h"
#include "error.h"
#include "ai.h"
#include "robot.h"
#include "fvi.h"
#include "physics.h"
#include "wall.h"
#ifdef EDITOR
#include "editor/editor.h"
#endif
#include "player.h"
#include "fireball.h"
#include "game.h"

void validate_all_paths(void);
void ai_path_set_orient_and_vel(object *objp, vms_vector *goal_point);
void maybe_ai_path_garbage_collect(void);
#ifndef NDEBUG
int validate_path(int debug_flag, point_seg *psegs, int num_points);
#endif

#define	PARALLAX	0		//	If !0, then special debugging for Parallax eyes enabled.

//	Length in segments of avoidance path
#define	AVOID_SEG_LENGTH	7

void create_random_xlate(byte *xt)
{
	int	i;

	for (i=0; i<MAX_SIDES_PER_SEGMENT; i++)
		xt[i] = i;

	for (i=0; i<MAX_SIDES_PER_SEGMENT; i++) {
                int     j = (d_rand()*MAX_SIDES_PER_SEGMENT)/(D_RAND_MAX+1);
		byte	temp_byte;
		Assert((j >= 0) && (j < MAX_SIDES_PER_SEGMENT));

		temp_byte = xt[j];
		xt[j] = xt[i];
		xt[i] = temp_byte;
	}

}

//	-----------------------------------------------------------------------------------------------------------
//	Insert the point at the center of the side connecting two segments between the two points.
// This is messy because we must insert into the list.  The simplest (and not too slow) way to do this is to start
// at the end of the list and go backwards.
void insert_center_points(point_seg *psegs, short *num_points)
{
	int	i, last_point;

	last_point = *num_points-1;

//	printf("---------- Original points ----------\n");
//	for (i=0; i<*num_points; i++)
//		printf("%2i: %3i [%7.3f %7.3f %7.3f]\n", i, psegs[i].segnum, f2fl(psegs[i].point.x), f2fl(psegs[i].point.y), f2fl(psegs[i].point.z));
//	printf("\n");

	for (i=last_point; i>0; i--) {
		int			connect_side;
		vms_vector	center_point, new_point;

		psegs[2*i] = psegs[i];
		connect_side = find_connect_side(&Segments[psegs[i].segnum], &Segments[psegs[i-1].segnum]);
		Assert(connect_side != -1);	//	Impossible!  These two segments must be connected, they were created by create_path_points (which was created by mk!)
		if (connect_side == -1)			//	Try to blow past the assert, this should at least prevent a hang.
			connect_side = 0;
		compute_center_point_on_side(&center_point, &Segments[psegs[i-1].segnum], connect_side);
		vm_vec_sub(&new_point, &psegs[i-1].point, &center_point);
		new_point.x /= 16;
		new_point.y /= 16;
		new_point.z /= 16;
		vm_vec_sub(&psegs[2*i-1].point, &center_point, &new_point);
		psegs[2*i-1].segnum = psegs[2*i].segnum;
		(*num_points)++;
	}

//	printf("---------- With inserted points ----------\n");
//	for (i=0; i<*num_points; i++)
//		printf("%2i: %3i [%7.3f %7.3f %7.3f]\n", i, psegs[i].segnum, f2fl(psegs[i].point.x), f2fl(psegs[i].point.y), f2fl(psegs[i].point.z));
//	printf("\n\n");

}

#ifdef EDITOR
int	Safety_flag_override = 0;
int	Random_flag_override = 0;
int	Ai_path_debug=0;
#endif

//	-----------------------------------------------------------------------------------------------------------
//	Create a path from objp->pos to the center of end_seg.
//	Return a list of (segment_num, point_locations) at psegs
//	Return number of points in *num_points.
//	if max_depth == -1, then there is no maximum depth.
//	If unable to create path, return -1, else return 0.
//	If random_flag !0, then introduce randomness into path by looking at sides in random order.  This means
//	that a path between two segments won't always be the same, unless it is unique.
//	If safety_flag is set, then additional points are added to "make sure" that points are reachable.  I would
//	like to say that it ensures that the object can move between the points, but that would require knowing what
//	the object is (which isn't passed, right?) and making fvi calls (slow, right?).  So, consider it the more_or_less_safe_flag.
//	If end_seg == -2, then end seg will never be found and this routine will drop out due to depth (probably called by create_n_segment_path).
int create_path_points(object *objp, int start_seg, int end_seg, point_seg *psegs, short *num_points, int max_depth, int random_flag, int safety_flag, int avoid_seg)
{
	int		cur_seg;
	int		sidenum;
	int		qtail = 0, qhead = 0;
	int		i;
	byte		visited[MAX_SEGMENTS];
	seg_seg	seg_queue[MAX_SEGMENTS];
	short		depth[MAX_SEGMENTS];
	int		cur_depth;
	byte		random_xlate[MAX_SIDES_PER_SEGMENT];
	point_seg	*original_psegs = psegs;
#ifndef NDEBUG
	point_seg	*other_original_psegs = psegs;
#endif

#ifndef NDEBUG
	validate_all_paths();
#endif

if ((objp->type == OBJ_ROBOT) && (objp->ctype.ai_info.behavior == AIB_RUN_FROM)) {
	random_flag = 1;
	avoid_seg = ConsoleObject->segnum;
	// Int3();
}

	if (max_depth == -1)
		max_depth = MAX_PATH_LENGTH;

	*num_points = 0;
//random_flag = Random_flag_override; //!! debug!!
//safety_flag = Safety_flag_override; //!! debug!!

//	for (i=0; i<=Highest_segment_index; i++) {
//		visited[i] = 0;
//		depth[i] = 0;
//	}
	memset(visited, 0, sizeof(visited[0])*(Highest_segment_index+1));
	memset(depth, 0, sizeof(depth[0])*(Highest_segment_index+1));

	//	If there is a segment we're not allowed to visit, mark it.
	if (avoid_seg != -1) {
		Assert(avoid_seg <= Highest_segment_index);
		if ((start_seg != avoid_seg) && (end_seg != avoid_seg)) {
			visited[avoid_seg] = 1;
			depth[avoid_seg] = 0;
		}
	}

	if (random_flag)
		create_random_xlate(random_xlate);

	cur_seg = start_seg;
	visited[cur_seg] = 1;
	cur_depth = 0;

	while (cur_seg != end_seg) {
		segment	*segp = &Segments[cur_seg];

		// mprintf((0, "\n"));
		for (sidenum = 0; sidenum < MAX_SIDES_PER_SEGMENT; sidenum++) {

			int	snum = sidenum;

			if (random_flag)
				snum = random_xlate[sidenum];

			if ((WALL_IS_DOORWAY(segp, snum) & WID_FLY_FLAG) || (ai_door_is_openable(objp, segp, snum))) {
				int	this_seg = segp->children[snum];

				if (!visited[this_seg]) {
					seg_queue[qtail].start = cur_seg;
					seg_queue[qtail].end = this_seg;
					visited[this_seg] = 1;
					depth[qtail++] = cur_depth+1;
					if (depth[qtail-1] == max_depth) {
						// mprintf((0, "\ndepth == max_depth == %i\n", max_depth));
						end_seg = seg_queue[qtail-1].end;
						goto cpp_done1;
					}
				}

			}
		}	//	for (sidenum...

		if (qhead >= qtail) {
			//	Couldn't get to goal, return a path as far as we got, which probably acceptable to the unparticular caller.
			end_seg = seg_queue[qtail-1].end;
			break;
		}

		cur_seg = seg_queue[qhead].end;
		cur_depth = depth[qhead];
		qhead++;

cpp_done1: ;
	}	//	while (cur_seg ...

	//	Set qtail to the segment which ends at the goal.
	while (seg_queue[--qtail].end != end_seg)
		if (qtail < 0) {
			// mprintf((0, "\nNo path!\n"));
			// printf("UNABLE TO FORM PATH");
			// Int3();
			return -1;
		}

	#ifdef EDITOR
	N_selected_segs = 0;
	#endif
//printf("Object #%3i, start: %3i ", objp-Objects, psegs-Point_segs);
	while (qtail >= 0) {
		int	parent_seg, this_seg;

		this_seg = seg_queue[qtail].end;
		parent_seg = seg_queue[qtail].start;
		psegs->segnum = this_seg;
//printf("%3i ", this_seg);
		compute_segment_center(&psegs->point,&Segments[this_seg]);
		psegs++;
		(*num_points)++;
		#ifdef EDITOR
		Selected_segs[N_selected_segs++] = this_seg;
		#endif

		if (parent_seg == start_seg)
			break;

		while (seg_queue[--qtail].end != parent_seg)
			Assert(qtail >= 0);
	}

	psegs->segnum = start_seg;
//printf("%3i\n", start_seg);
	compute_segment_center(&psegs->point,&Segments[start_seg]);
	psegs++;
	(*num_points)++;

#ifndef NDEBUG
	validate_path(1, original_psegs, *num_points);
#endif

	//	Now, reverse point_segs in place.
	for (i=0; i< (*num_points)/2; i++) {
		point_seg		temp_point_seg = *(original_psegs + i);
		*(original_psegs + i) = *(original_psegs + *num_points - i - 1);
		*(original_psegs + *num_points - i - 1) = temp_point_seg;
	}
#ifndef NDEBUG
	validate_path(2, original_psegs, *num_points);
#endif

	//	Now, if safety_flag set, then insert the point at the center of the side connecting two segments
	//	between the two points.  This is messy because we must insert into the list.  The simplest (and not too slow)
	//	way to do this is to start at the end of the list and go backwards.
	if (safety_flag) {
		if (psegs - Point_segs + *num_points + 2 > MAX_POINT_SEGS) {
			//	Ouch!  Cannot insert center points in path.  So return unsafe path.
//			Int3();	// Contact Mike:  This is impossible.
//			force_dump_ai_objects_all("Error in create_path_points");
			ai_reset_all_paths();
			return -1;
		} else {
			//mprintf((0, "Old num_points = %i, new one should be %i. ", *num_points, 2 * (*num_points) - 1));
			insert_center_points(original_psegs, num_points);
			//mprintf((0, "New num_points = %i\n", *num_points));
		}
	}
#ifndef NDEBUG
	validate_path(3, original_psegs, *num_points);
#endif

#ifndef NDEBUG
//--debug	if (objp == ConsoleObject) {
//--debug		int	i;
//--debug
//--debug		for (i=0; i<*num_points; i++) {
//--debug			mprintf((0, "%3i ", original_psegs[i].segnum));
//--debug		}
//--debug		mprintf((0, "\n"));
//--debug	}

	validate_path(0, original_psegs, *num_points);

// mprintf((0, "Created path for object %3i at index %4i, length = %2i\n", objp-Objects, psegs-Point_segs, *num_points));
	Assert(other_original_psegs == original_psegs);

//--debug 01/18/95--	if (objp->ctype.ai_info.behavior == AIB_RUN_FROM) {
//--debug 01/18/95--		int	i;
//--debug 01/18/95--
//--debug 01/18/95--		mprintf((0, "%3i: ", objp-Objects));
//--debug 01/18/95--		for (i=0; i<*num_points; i++)
//--debug 01/18/95--			mprintf((0, "%3i ", original_psegs[i].segnum));
//--debug 01/18/95--		mprintf((0, "\n"));
//--debug 01/18/95--	}
#endif


	return 0;
}

#ifndef NDEBUG
//	-------------------------------------------------------------------------------------------------------
//	Make sure that there are connections between all segments on path.
//	Note that if path has been optimized, connections may not be direct, so this function is useless, or worse.
//	Return true if valid, else return false.
int validate_path(int debug_flag, point_seg *psegs, int num_points)
{
#if PARALLAX
	int		i, curseg;

	//	Trap a common bug elsewhere in aipath.
	if (psegs > Point_segs_free_ptr) {
		//Int3();		//	Contact Mike: Debug trap for elusive, nasty bug.
		return 0;
	}

	curseg = psegs->segnum;
	if ((curseg < 0) || (curseg > Highest_segment_index)) {
		mprintf((0, "Path beginning at index %i, length=%i is bogus!\n", psegs-Point_segs, num_points));
		return 0;
	}

if (debug_flag == 999)
	mprintf((0, "That's curious...\n"));

if (num_points == 0)
	return 1;

// printf("(%i) Validating path at psegs=%i, num_points=%i, segments = %3i ", debug_flag, psegs-Point_segs, num_points, psegs[0].segnum);
	for (i=1; i<num_points; i++) {
		int	sidenum;
		int	nextseg = psegs[i].segnum;

		if ((nextseg < 0) || (nextseg > Highest_segment_index)) {
			mprintf((0, "Path beginning at index %i, length=%i is bogus!\n", psegs-Point_segs, num_points));
			return 0;
		}

// printf("%3i ", nextseg);
		if (curseg != nextseg) {
			for (sidenum=0; sidenum<MAX_SIDES_PER_SEGMENT; sidenum++)
				if (Segments[curseg].children[sidenum] == nextseg)
					break;

			// Assert(sidenum != MAX_SIDES_PER_SEGMENT);	//	Hey, created path is not contiguous, why!?
			if (sidenum == MAX_SIDES_PER_SEGMENT) {
				mprintf((0, "Path beginning at index %i, length=%i is bogus!\n", psegs-Point_segs, num_points));
				// printf("BOGUS");
				// Int3();
				return 0;
			}
			curseg = nextseg;
		}
	}
//printf("\n");
#endif
	return 1;

}
#endif

#ifndef NDEBUG
//	-----------------------------------------------------------------------------------------------------------
void validate_all_paths(void)
{

#if PARALLAX
	int	i;

	for (i=0; i<=Highest_object_index; i++) {
		if (Objects[i].type == OBJ_ROBOT) {
			object		*objp = &Objects[i];
			ai_static	*aip = &objp->ctype.ai_info;
			//ai_local		*ailp = &Ai_local_info[i];

			if (objp->control_type == CT_AI) {
				if ((aip->hide_index != -1) && (aip->path_length > 0))
					if (!validate_path(4, &Point_segs[aip->hide_index], aip->path_length)) {
						//Int3();	//	This path is bogus!  Who corrupted it!  Danger! Danger!
									//	Contact Mike, he caused this mess.
						//force_dump_ai_objects_all("Error in validate_all_paths");
						aip->path_length=0;	//	This allows people to resume without harm...
					}
			}
		}
	}
#endif

}
#endif

//	-------------------------------------------------------------------------------------------------------
//	Creates a path from the objects current segment (objp->segnum) to the specified segment for the object to
//	hide in Ai_local_info[objnum].goal_segment.
//	Sets	objp->ctype.ai_info.hide_index,		a pointer into Point_segs, the first point_seg of the path.
//			objp->ctype.ai_info.path_length,		length of path
//			Point_segs_free_ptr				global pointer into Point_segs array
void create_path(object *objp)
{
	ai_static	*aip = &objp->ctype.ai_info;
	ai_local		*ailp = &Ai_local_info[objp-Objects];
	int			start_seg, end_seg;

	start_seg = objp->segnum;
	end_seg = ailp->goal_segment;

	if (end_seg == -1)
		create_n_segment_path(objp, 3, -1);

	if (end_seg == -1) {
		; //mprintf((0, "Object %i, hide_segment = -1, not creating path.\n", objp-Objects));
	} else {
		create_path_points(objp, start_seg, end_seg, Point_segs_free_ptr, &aip->path_length, -1, 0, 0, -1);
		aip->hide_index = Point_segs_free_ptr - Point_segs;
		aip->cur_path_index = 0;
#ifndef NDEBUG
		validate_path(5, Point_segs_free_ptr, aip->path_length);
#endif
		Point_segs_free_ptr += aip->path_length;
		if (Point_segs_free_ptr - Point_segs + MAX_PATH_LENGTH*2 > MAX_POINT_SEGS) {
			//Int3();	//	Contact Mike: This is curious, though not deadly. /eip++;g
			//force_dump_ai_objects_all("Error in create_path");
			ai_reset_all_paths();
		}
		aip->PATH_DIR = 1;		//	Initialize to moving forward.
		aip->SUBMODE = AISM_HIDING;		//	Pretend we are hiding, so we sit here until bothered.
	}

	maybe_ai_path_garbage_collect();

}

//	-------------------------------------------------------------------------------------------------------
//	Creates a path from the objects current segment (objp->segnum) to the specified segment for the object to
//	hide in Ai_local_info[objnum].goal_segment.
//	Sets	objp->ctype.ai_info.hide_index,		a pointer into Point_segs, the first point_seg of the path.
//			objp->ctype.ai_info.path_length,		length of path
//			Point_segs_free_ptr				global pointer into Point_segs array
void create_path_to_player(object *objp, int max_length, int safety_flag)
{
	ai_static	*aip = &objp->ctype.ai_info;
	ai_local		*ailp = &Ai_local_info[objp-Objects];
	int			start_seg, end_seg;

//mprintf((0, "Creating path to player.\n"));
	if (max_length == -1)
		max_length = MAX_DEPTH_TO_SEARCH_FOR_PLAYER;

	ailp->time_player_seen = GameTime;			//	Prevent from resetting path quickly.
	ailp->goal_segment = ConsoleObject->segnum;

	start_seg = objp->segnum;
	end_seg = ailp->goal_segment;

	// mprintf((0, "Creating path for object #%i, from segment #%i to #%i\n", objp-Objects, start_seg, end_seg));

	if (end_seg == -1) {
		; //mprintf((0, "Object %i, hide_segment = -1, not creating path.\n", objp-Objects));
	} else {
#ifndef NDEBUG
point_seg *pseg0 = Point_segs_free_ptr;
#endif
		create_path_points(objp, start_seg, end_seg, Point_segs_free_ptr, &aip->path_length, max_length, 1, safety_flag, -1);
		aip->hide_index = Point_segs_free_ptr - Point_segs;
		aip->cur_path_index = 0;
#ifndef NDEBUG
//Enclosed this Assert in an ifdef, because if NDEBUG isn't defined,
//pseg0 doesn't exist!  -KRB
Assert(Point_segs_free_ptr == pseg0);
#endif

#ifndef NDEBUG
		validate_path(6, Point_segs_free_ptr, aip->path_length);
#endif
		// mprintf((0, "Created path to player, len = %i, EndSeg = %i, PlayerSeg = %i\n", aip->path_length, Point_segs[aip->hide_index+aip->path_length-1].segnum, ConsoleObject->segnum));
		Point_segs_free_ptr += aip->path_length;
		if (Point_segs_free_ptr - Point_segs + MAX_PATH_LENGTH*2 > MAX_POINT_SEGS) {
			//Int3();	//	Contact Mike: This is stupid.  Should call maybe_ai_garbage_collect before the add.
			//force_dump_ai_objects_all("Error in create_path_to_player");
			ai_reset_all_paths();
			return;
		}
//		Assert(Point_segs_free_ptr - Point_segs + MAX_PATH_LENGTH*2 < MAX_POINT_SEGS);
		aip->PATH_DIR = 1;		//	Initialize to moving forward.
		aip->SUBMODE = AISM_GOHIDE;		//	This forces immediate movement.
		ailp->mode = AIM_FOLLOW_PATH;
		ailp->player_awareness_type = 0;		//	If robot too aware of player, will set mode to chase
		// mprintf((0, "Created %i segment path to player.\n", aip->path_length));
	}

	maybe_ai_path_garbage_collect();

}

//	-------------------------------------------------------------------------------------------------------
//	Creates a path from the objects current segment (objp->segnum) to the specified segment for the object to
//	hide in Ai_local_info[objnum].goal_segment
//	Sets	objp->ctype.ai_info.hide_index,		a pointer into Point_segs, the first point_seg of the path.
//			objp->ctype.ai_info.path_length,		length of path
//			Point_segs_free_ptr				global pointer into Point_segs array
void create_path_to_station(object *objp, int max_length)
{
	ai_static	*aip = &objp->ctype.ai_info;
	ai_local		*ailp = &Ai_local_info[objp-Objects];
	int			start_seg, end_seg;

	if (max_length == -1)
		max_length = MAX_DEPTH_TO_SEARCH_FOR_PLAYER;

	ailp->time_player_seen = GameTime;			//	Prevent from resetting path quickly.

	start_seg = objp->segnum;
	end_seg = aip->hide_segment;

	//1001: mprintf((0, "Back to station for object #%i, from segment #%i to #%i\n", objp-Objects, start_seg, end_seg));

	if (end_seg == -1) {
		; //mprintf((0, "Object %i, hide_segment = -1, not creating path.\n", objp-Objects));
	} else {
		create_path_points(objp, start_seg, end_seg, Point_segs_free_ptr, &aip->path_length, max_length, 1, 1, -1);
		aip->hide_index = Point_segs_free_ptr - Point_segs;
		aip->cur_path_index = 0;
#ifndef NDEBUG
		validate_path(7, Point_segs_free_ptr, aip->path_length);
#endif
		//mprintf((0, "Created station path, len = %i, EndSeg = %i\n", aip->path_length, Point_segs[aip->hide_index+aip->path_length-1].segnum));
		Point_segs_free_ptr += aip->path_length;
		if (Point_segs_free_ptr - Point_segs + MAX_PATH_LENGTH*2 > MAX_POINT_SEGS) {
			//Int3();	//	Contact Mike: Stupid.
			//force_dump_ai_objects_all("Error in create_path_to_station");
			ai_reset_all_paths();
			return;
		}
//		Assert(Point_segs_free_ptr - Point_segs + MAX_PATH_LENGTH*2 < MAX_POINT_SEGS);
		aip->PATH_DIR = 1;		//	Initialize to moving forward.
		// aip->SUBMODE = AISM_GOHIDE;		//	This forces immediate movement.
		ailp->mode = AIM_FOLLOW_PATH;
		ailp->player_awareness_type = 0;
	}


	maybe_ai_path_garbage_collect();

}


//	-------------------------------------------------------------------------------------------------------
//	Create a path of length path_length for an object, stuffing info in ai_info field.
void create_n_segment_path(object *objp, int path_length, int avoid_seg)
{
	ai_static	*aip=&objp->ctype.ai_info;
	ai_local		*ailp = &Ai_local_info[objp-Objects];

//mprintf((0, "Creating %i segment path.\n", path_length));

	if (create_path_points(objp, objp->segnum, -2, Point_segs_free_ptr, &aip->path_length, path_length, 1, 0, avoid_seg) == -1) {
		//Int3();	//	Contact Mike:  Considering removing this code.  Can I?
		//force_dump_ai_objects_all("Error in create_n_segment_path");
		Point_segs_free_ptr += aip->path_length;
		//mprintf((0, "Unable to form path of length %i for object %i\n", path_length, objp-Objects));
		while ((create_path_points(objp, objp->segnum, -2, Point_segs_free_ptr, &aip->path_length, --path_length, 1, 0, -1) == -1)) {
			//mprintf((0, "Unable to form path of length %i for object %i\n", path_length, objp-Objects));
			Assert(path_length);
		}
	}

	aip->hide_index = Point_segs_free_ptr - Point_segs;
	aip->cur_path_index = 0;
#ifndef NDEBUG
	validate_path(8, Point_segs_free_ptr, aip->path_length);
#endif
	Point_segs_free_ptr += aip->path_length;
	if (Point_segs_free_ptr - Point_segs + MAX_PATH_LENGTH*2 > MAX_POINT_SEGS) {
		//Int3();	//	Contact Mike: This is curious, though not deadly. /eip++;g
		//force_dump_ai_objects_all("Error in crete_n_segment_path 2");
		ai_reset_all_paths();
	}

	aip->PATH_DIR = 1;		//	Initialize to moving forward.
	aip->SUBMODE = -1;		//	Don't know what this means.
	ailp->mode = AIM_FOLLOW_PATH;

	maybe_ai_path_garbage_collect();

}

//	-------------------------------------------------------------------------------------------------------
void create_n_segment_path_to_door(object *objp, int path_length, int avoid_seg)
{
	create_n_segment_path(objp, path_length, avoid_seg);
}

//--unused-- //	-------------------------------------------------------------------------------------------------------
//--unused-- //	For all AI objects which have mode HIDE or RUN_FROM_OBJECT, create a path to run to.
//--unused-- void create_all_paths(void)
//--unused-- {
//--unused-- 	int	i;
//--unused-- 
//--unused-- 	for (i=0; i<=Highest_object_index; i++) {
//--unused-- 		object	*objp = &Objects[i];
//--unused-- 		if (Objects[i].control_type == CT_AI) {
//--unused-- 			ai_static	*aip = &objp->ctype.ai_info;
//--unused-- 			if ((aip->behavior == AIB_HIDE) || (aip->behavior == AIB_RUN_FROM) || (aip->behavior == AIB_FOLLOW_PATH))
//--unused-- 				create_path(&Objects[i]);
//--unused-- 		}
//--unused-- 	}
//--unused-- 
//--unused-- }

extern int Connected_segment_distance;

//	----------------------------------------------------------------------------------------------------
void move_object_to_goal(object *objp, vms_vector *goal_point, int goal_seg)
{
	ai_static	*aip = &objp->ctype.ai_info;
	int			segnum;

	Assert(objp->segnum != -1);

	// mprintf((0, "[%i -> %i]\n", objp-Objects, goal_seg));

#ifndef NDEBUG
	if (objp->segnum != goal_seg)
		if (find_connect_side(&Segments[objp->segnum], &Segments[goal_seg]) == -1) {
			fix	dist;
			dist = find_connected_distance(&objp->pos, objp->segnum, goal_point, goal_seg, 30, WID_FLY_FLAG);
			if (Connected_segment_distance > 2) {	//	This global is set in find_connected_distance
				// Int3();
				mprintf((1, "Warning: Object %i hopped across %i segments, a distance of %7.3f.\n", objp-Objects, Connected_segment_distance, f2fl(dist)));
			}
		}
#endif

	aip->cur_path_index += aip->PATH_DIR;

	if (aip->cur_path_index <= 0) {
		if (aip->behavior == AIB_STATION) {
			mprintf((0, "Object #%i, creating path back to station.\n", objp-Objects));
			create_path_to_station(objp, 15);
			return;
		}
		aip->cur_path_index = 0;
		aip->PATH_DIR = -aip->PATH_DIR;
	} else if (aip->cur_path_index >= aip->path_length) {
		if (aip->behavior == AIB_STATION) {
			mprintf((0, "Object #%i, creating path back to station.\n", objp-Objects));
			create_path_to_station(objp, 15);
			return;
		}
		aip->cur_path_index = aip->path_length-1;
		aip->PATH_DIR = -aip->PATH_DIR;
	}

	objp->pos = *goal_point;
	segnum = find_object_seg(objp);
	if (segnum != goal_seg)
		mprintf((1, "Object #%i goal supposed to be in segment #%i, but in segment #%i\n", objp-Objects, goal_seg, segnum));

	if (segnum == -1) {
		Int3();	//	Oops, object is not in any segment.
					// Contact Mike: This is impossible.
		//	Hack, move object to center of segment it used to be in.
		compute_segment_center(&objp->pos, &Segments[objp->segnum]);
	} else
		obj_relink(objp-Objects, segnum);
}

//	----------------------------------------------------------------------------------------------------------
//	Optimization: If current velocity will take robot near goal, don't change velocity
void ai_follow_path(object *objp, int player_visibility)
{
	ai_static		*aip = &objp->ctype.ai_info;

	vms_vector	goal_point, new_goal_point;
	fix			dist_to_goal;
	// robot_info	*robptr = &Robot_info[objp->id];
	int			forced_break, original_dir, original_index;
	fix			dist_to_player;
	int			goal_seg;
	ai_local		*ailp = &Ai_local_info[objp-Objects];
	fix			threshold_distance;

//int debug_count=0;
//int Cur_index[100], Cur_dir[100];

// mprintf((0, "Obj %i, dist=%6.1f index=%i len=%i seg=%i pos = %6.1f %6.1f %6.1f.\n", objp-Objects, f2fl(vm_vec_dist_quick(&objp->pos, &ConsoleObject->pos)), aip->cur_path_index, aip->path_length, objp->segnum, f2fl(objp->pos.x), f2fl(objp->pos.y), f2fl(objp->pos.z)));

//--debug 01/17/95--	//	If in multiplayer mode, only follow path if player is visible.
//--debug 01/17/95--	if (Game_mode & GM_MULTI)
//--debug 01/17/95--		if (player_visibility) {
//--debug 01/17/95--			#ifndef SHAREWARE
//--debug 01/17/95--			if (!ai_multiplayer_awareness(objp, 69))
//--debug 01/17/95--				return;
//--debug 01/17/95--			#endif
//--debug 01/17/95--		} else
//--debug 01/17/95--			return;

	if ((aip->hide_index == -1) || (aip->path_length == 0))
         {
                if (ailp->mode == AIM_RUN_FROM_OBJECT) {
			create_n_segment_path(objp, 5, -1);
			ailp->mode = AIM_RUN_FROM_OBJECT;
		} else
			create_path(objp);
         }

if ((aip->hide_index + aip->path_length > Point_segs_free_ptr - Point_segs) && (aip->path_length>0)) {
	//Int3();	//	Contact Mike: Bad.  Path goes into what is believed to be free space.
	//force_dump_ai_objects_all("Error in ai_follow_path");
	ai_reset_all_paths();
}

	if (aip->path_length < 2) {
		if (ailp->mode == AIM_RUN_FROM_OBJECT) {
			if (ConsoleObject->segnum == objp->segnum)
				create_n_segment_path(objp, AVOID_SEG_LENGTH, -1);			//	Can't avoid segment player is in, robot is already in it! (That's what the -1 is for) 
			else
				create_n_segment_path(objp, AVOID_SEG_LENGTH, ConsoleObject->segnum);
			ailp->mode = AIM_RUN_FROM_OBJECT;	//	It gets bashed in create_n_segment_path
		} else {
			ailp->mode = AIM_STILL;
		}
		return;
	}

	Assert((aip->PATH_DIR == -1) || (aip->PATH_DIR == 1));

	if ((aip->SUBMODE == AISM_HIDING) && (aip->behavior == AIB_HIDE))
		return;

	goal_point = Point_segs[aip->hide_index + aip->cur_path_index].point;
	goal_seg = Point_segs[aip->hide_index + aip->cur_path_index].segnum;
	dist_to_goal = vm_vec_dist_quick(&goal_point, &objp->pos);

	if (Player_is_dead)
		dist_to_player = vm_vec_dist_quick(&objp->pos, &Viewer->pos);
	else
		dist_to_player = vm_vec_dist_quick(&objp->pos, &ConsoleObject->pos);

	//	Efficiency hack: If far away from player, move in big quantized jumps.
	if ((dist_to_player > F1_0*200) && !(Game_mode & GM_MULTI)) {
		if (dist_to_goal < F1_0*2)
			move_object_to_goal(objp, &goal_point, goal_seg);
		else {
			robot_info	*robptr = &Robot_info[objp->id];
			fix	cur_speed = robptr->max_speed[Difficulty_level]/2;
			fix	distance_travellable = fixmul(FrameTime, cur_speed);

			if (distance_travellable >= dist_to_goal)
				move_object_to_goal(objp, &goal_point, goal_seg);
			else {
				fix	prob = fixdiv(distance_travellable, dist_to_goal);

				int	rand_num = d_rand();
				if ( (rand_num >> 1) < prob)
					move_object_to_goal(objp, &goal_point, goal_seg);
			}
		}

		//	If we are hiding, we stop when we get to the goal.
		if (ailp->mode == AIM_HIDE)
			ailp->mode = AIM_STILL;

		return;
	}

//	if ((dist_to_player > F1_0*200) && !(Game_mode & GM_MULTI))	//	ROB!
//		mprintf((0, "+"));		//	 ROB! Enable this to see how much more often follow path code would be called.

	//	If running from player, only run until can't be seen.
	if (ailp->mode == AIM_RUN_FROM_OBJECT) {
		if ((player_visibility == 0) && (ailp->player_awareness_type == 0)) {
			fix	vel_scale;

			vel_scale = F1_0 - FrameTime/2;
			if (vel_scale < F1_0/2)
				vel_scale = F1_0/2;

			vm_vec_scale(&objp->mtype.phys_info.velocity, vel_scale);

			return;
		} else {
			//	If player on path (beyond point robot is now at), then create a new path.
			point_seg	*curpsp = &Point_segs[aip->hide_index];
			int			player_segnum = ConsoleObject->segnum;
			int			i;

			//	This is probably being done every frame, which is wasteful.
			for (i=aip->cur_path_index; i<aip->path_length; i++)
				if (curpsp[i].segnum == player_segnum) {
					if (player_segnum != objp->segnum)
						create_n_segment_path(objp, AVOID_SEG_LENGTH, player_segnum);
					else
						create_n_segment_path(objp, AVOID_SEG_LENGTH, -1);
					ailp->mode = AIM_RUN_FROM_OBJECT;	//	It gets bashed in create_n_segment_path
					break;
				}
			if (player_visibility) {
				ailp->player_awareness_type = 1;
				ailp->player_awareness_time = F1_0;
			}
		}
	}

	if (aip->cur_path_index < 0)
		aip->cur_path_index = 0;
	else if (aip->cur_path_index >= aip->path_length)
              {
                if (ailp->mode == AIM_RUN_FROM_OBJECT) {
			create_n_segment_path(objp, AVOID_SEG_LENGTH, ConsoleObject->segnum);
			ailp->mode = AIM_RUN_FROM_OBJECT;	//	It gets bashed in create_n_segment_path
		} else
			aip->cur_path_index = aip->path_length-1;
              }

	goal_point = Point_segs[aip->hide_index + aip->cur_path_index].point;

	//	If near goal, pick another goal point.
	forced_break = 0;		//	Gets set for short paths.
	original_dir = aip->PATH_DIR;
	original_index = aip->cur_path_index;
	threshold_distance = fixmul(vm_vec_mag_quick(&objp->mtype.phys_info.velocity), FrameTime)*2 + F1_0*2;

	while ((dist_to_goal < threshold_distance) && !forced_break) {

		//	Advance to next point on path.
		aip->cur_path_index += aip->PATH_DIR;

		//	See if next point wraps past end of path (in either direction), and if so, deal with it based on mode.
		if ((aip->cur_path_index >= aip->path_length) || (aip->cur_path_index < 0)) {

			//mprintf((0, "Object %i reached end of the line!\n", objp-Objects));
			//	If mode = hiding, then stay here until get bonked or hit by player.
			if (ailp->mode == AIM_HIDE) {
				ailp->mode = AIM_STILL;
				return;		// Stay here until bonked or hit by player.
			} else if (aip->behavior == AIB_STATION) {
				mprintf((0, "Object %i reached end of line, creating path back to station.\n", objp-Objects));
				create_path_to_station(objp, 15);
				if (aip->hide_segment != Point_segs[aip->hide_index+aip->path_length-1].segnum) {
					mprintf((0, "Couldn't make it back to path.  Put in still mode.\n"));
					ailp->mode = AIM_STILL;
				}
				return;
			} else if ((ailp->mode == AIM_FOLLOW_PATH) && (aip->behavior != AIB_FOLLOW_PATH)) {
				create_path_to_player(objp, 10, 1);
			} else if (ailp->mode == AIM_RUN_FROM_OBJECT) {
				create_n_segment_path(objp, AVOID_SEG_LENGTH, ConsoleObject->segnum);
				ailp->mode = AIM_RUN_FROM_OBJECT;	//	It gets bashed in create_n_segment_path
			} else {
				//	Reached end of the line.  First see if opposite end point is reachable, and if so, go there.
				//	If not, turn around.
				int			opposite_end_index;
				vms_vector	*opposite_end_point;
				fvi_info		hit_data;
				int			fate;
				fvi_query	fq;

				// See which end we're nearer and look at the opposite end point.
				if (abs(aip->cur_path_index - aip->path_length) < aip->cur_path_index) {
					//	Nearer to far end (ie, index not 0), so try to reach 0.
					opposite_end_index = 0;
				} else {
					//	Nearer to 0 end, so try to reach far end.
					opposite_end_index = aip->path_length-1;
				}

				opposite_end_point = &Point_segs[aip->hide_index + opposite_end_index].point;

				fq.p0						= &objp->pos;
				fq.startseg				= objp->segnum;
				fq.p1						= opposite_end_point;
				fq.rad					= objp->size;
				fq.thisobjnum			= objp-Objects;
				fq.ignore_obj_list	= NULL;
				fq.flags					= 0; 				//what about trans walls???

				fate = find_vector_intersection(&fq,&hit_data);

				if (fate != HIT_WALL) {
					//	We can be circular!  Do it!
					//	Path direction is unchanged.
					aip->cur_path_index = opposite_end_index;
				} else {
					aip->PATH_DIR = -aip->PATH_DIR;
				}
			}
			break;
		} else {
			new_goal_point = Point_segs[aip->hide_index + aip->cur_path_index].point;
			goal_point = new_goal_point;
			dist_to_goal = vm_vec_dist_quick(&goal_point, &objp->pos);
		}

		//	If went all the way around to original point, in same direction, then get out of here!
//		Cur_index[debug_count] = aip->cur_path_index;
//		Cur_dir[debug_count] = aip->PATH_DIR;
//		debug_count++;
		if ((aip->cur_path_index == original_index) && (aip->PATH_DIR == original_dir)) {
			create_path_to_player(objp, 3, 1);
			forced_break = 1;
		}
	}	//	end while

	//	Set velocity (objp->mtype.phys_info.velocity) and orientation (objp->orient) for this object.
	ai_path_set_orient_and_vel(objp, &goal_point);

}

typedef struct {
	short	path_start, objnum;
} obj_path;

int path_index_compare(obj_path *i1, obj_path *i2)
{
	if (i1->path_start < i2->path_start)
		return -1;
	else if (i1->path_start == i2->path_start)
		return 0;
	else
		return 1;
}

//	----------------------------------------------------------------------------------------------------------
//	Set orientation matrix and velocity for objp based on its desire to get to a point.
void ai_path_set_orient_and_vel(object *objp, vms_vector *goal_point)
{
	vms_vector	cur_vel = objp->mtype.phys_info.velocity;
	vms_vector	norm_cur_vel;
	vms_vector	norm_vec_to_goal;
	vms_vector	cur_pos = objp->pos;
	vms_vector	norm_fvec;
	fix			speed_scale;
	fix			dot;
	robot_info	*robptr = &Robot_info[objp->id];
	fix			max_speed;

	//	If evading player, use highest difficulty level speed, plus something based on diff level
	max_speed = robptr->max_speed[Difficulty_level];
	if (Ai_local_info[objp-Objects].mode == AIM_RUN_FROM_OBJECT)
		max_speed = max_speed*3/2;

	vm_vec_sub(&norm_vec_to_goal, goal_point, &cur_pos);
	vm_vec_normalize_quick(&norm_vec_to_goal);

	norm_cur_vel = cur_vel;
	vm_vec_normalize_quick(&norm_cur_vel);

	norm_fvec = objp->orient.fvec;
	vm_vec_normalize_quick(&norm_fvec);

	dot = vm_vec_dot(&norm_vec_to_goal, &norm_fvec);

	//	If very close to facing opposite desired vector, perturb vector
	if (dot < -15*F1_0/16) {
		//mprintf((0, "Facing away from goal, abruptly turning\n"));
		norm_cur_vel = norm_vec_to_goal;
	} else {
		norm_cur_vel.x += norm_vec_to_goal.x/2;
		norm_cur_vel.y += norm_vec_to_goal.y/2;
		norm_cur_vel.z += norm_vec_to_goal.z/2;
	}

	vm_vec_normalize_quick(&norm_cur_vel);

	//	Set speed based on this robot type's maximum allowed speed and how hard it is turning.
	//	How hard it is turning is based on the dot product of (vector to goal) and (current velocity vector)
	//	Note that since 3*F1_0/4 is added to dot product, it is possible for the robot to back up.

	//	Set speed and orientation.
	if (dot < 0)
		dot /= -4;

	speed_scale = fixmul(max_speed, dot);
	vm_vec_scale(&norm_cur_vel, speed_scale);
	objp->mtype.phys_info.velocity = norm_cur_vel;

	if (Ai_local_info[objp-Objects].mode == AIM_RUN_FROM_OBJECT)
		ai_turn_towards_vector(&norm_vec_to_goal, objp, robptr->turn_time[NDL-1]/2);
	else
		ai_turn_towards_vector(&norm_vec_to_goal, objp, robptr->turn_time[Difficulty_level]);

}

int	Last_frame_garbage_collected = 0;

//	----------------------------------------------------------------------------------------------------------
//	Garbage colledion -- Free all unused records in Point_segs and compress all paths.
void ai_path_garbage_collect(void)
{
	int	free_path_index = 0;
	int	num_path_objects = 0;
	int	objnum;
	int	objind;
	obj_path		object_list[MAX_OBJECTS];

#ifndef NDEBUG
	force_dump_ai_objects_all("***** Start ai_path_garbage_collect *****");
#endif

	// -- mprintf((0, "Garbage collection frame %i, last frame %i!  Old free index = %i ", FrameCount, Last_frame_garbage_collected, Point_segs_free_ptr - Point_segs));

	Last_frame_garbage_collected = FrameCount;

#ifndef NDEBUG
	validate_all_paths();
#endif
	//	Create a list of objects which have paths of length 1 or more.
	for (objnum=0; objnum <= Highest_object_index; objnum++) {
		object	*objp = &Objects[objnum];

		if ((objp->type == OBJ_ROBOT) && (objp->control_type == CT_AI)) {
			ai_static	*aip = &objp->ctype.ai_info;

			if (aip->path_length) {
				object_list[num_path_objects].path_start = aip->hide_index;
				object_list[num_path_objects++].objnum = objnum;
			}
		}
	}

	qsort(object_list, num_path_objects, sizeof(object_list[0]), (int (*)(const void *, const void *))path_index_compare);

	for (objind=0; objind < num_path_objects; objind++) {
		object		*objp;
		ai_static	*aip;
		int			i;
		int			old_index;

		objnum = object_list[objind].objnum;
		objp = &Objects[objnum];
		aip = &objp->ctype.ai_info;
		old_index = aip->hide_index;

		aip->hide_index = free_path_index;
		for (i=0; i<aip->path_length; i++)
			Point_segs[free_path_index++] = Point_segs[old_index++];
	}

	Point_segs_free_ptr = &Point_segs[free_path_index];

	mprintf((0, "new = %i\n", free_path_index));
//printf("After garbage collection, free index = %i\n", Point_segs_free_ptr - Point_segs);
#ifndef NDEBUG
	{
	int i;

	force_dump_ai_objects_all("***** Finish ai_path_garbage_collect *****");

	for (i=0; i<=Highest_object_index; i++) {
		ai_static	*aip = &Objects[i].ctype.ai_info;

		if ((Objects[i].type == OBJ_ROBOT) && (Objects[i].control_type == CT_AI))
			if ((aip->hide_index + aip->path_length > Point_segs_free_ptr - Point_segs) && (aip->path_length>0))
				Int3();		//	Contact Mike: Debug trap for nasty, elusive bug.
	}

	validate_all_paths();
	}
#endif

}

//	-----------------------------------------------------------------------------
//	Do garbage collection if not been done for awhile, or things getting really critical.
void maybe_ai_path_garbage_collect(void)
{
	if (Point_segs_free_ptr - Point_segs > MAX_POINT_SEGS - MAX_PATH_LENGTH) {
		if (Last_frame_garbage_collected+1 >= FrameCount) {
			//	This is kind of bad.  Garbage collected last frame or this frame.
			//	Just destroy all paths.  Too bad for the robots.  They are memory wasteful.
			ai_reset_all_paths();
			mprintf((1, "Warning: Resetting all paths.  Point_segs buffer nearly exhausted.\n"));
		} else {
			//	We are really close to full, but didn't just garbage collect, so maybe this is recoverable.
			mprintf((1, "Warning: Almost full garbage collection being performed: "));
			ai_path_garbage_collect();
			mprintf((1, "Free records = %i/%i\n", MAX_POINT_SEGS - (Point_segs_free_ptr - Point_segs), MAX_POINT_SEGS));
		}
	} else if (Point_segs_free_ptr - Point_segs > 3*MAX_POINT_SEGS/4) {
		if (Last_frame_garbage_collected + 16 < FrameCount) {
			ai_path_garbage_collect();
		}
	} else if (Point_segs_free_ptr - Point_segs > MAX_POINT_SEGS/2) {
		if (Last_frame_garbage_collected + 256 < FrameCount) {
			ai_path_garbage_collect();
		}
	}
}

//	-----------------------------------------------------------------------------
//	Reset all paths.  Do garbage collection.
//	Should be called at the start of each level.
void ai_reset_all_paths(void)
{
	int	i;

	for (i=0; i<=Highest_object_index; i++)
		if (Objects[i].control_type == CT_AI) {
			Objects[i].ctype.ai_info.hide_index = -1;
			Objects[i].ctype.ai_info.path_length = 0;
		}

	ai_path_garbage_collect();

}

//	---------------------------------------------------------------------------------------------------------
//	Probably called because a robot bashed a wall, getting a bunch of retries.
//	Try to resume path.
void attempt_to_resume_path(object *objp)
{
	//int				objnum = objp-Objects;
	ai_static		*aip = &objp->ctype.ai_info;
//	int				goal_segnum, object_segnum,
	int				abs_index, new_path_index;

	mprintf((0, "Object %i trying to resume path at index %i\n", objp-Objects, aip->cur_path_index));

	if (aip->behavior == AIB_STATION)
		if (d_rand() > 8192) {
			ai_local			*ailp = &Ai_local_info[objp-Objects];

			aip->hide_segment = objp->segnum;
			ailp->mode = AIM_STILL;
			mprintf((1, "Note: Bashing hide segment of robot %i to current segment because he's lost.\n", objp-Objects));
		}

//	object_segnum = objp->segnum;
	abs_index = aip->hide_index+aip->cur_path_index;
//	goal_segnum = Point_segs[abs_index].segnum;

//	if (object_segnum == goal_segnum)
//		mprintf((0, "Very peculiar, goal segnum = object's segnum = %i.\n", goal_segnum));

	new_path_index = aip->cur_path_index - aip->PATH_DIR;

	if ((new_path_index >= 0) && (new_path_index < aip->path_length)) {
		// mprintf((0, "Trying path index of %i\n", new_path_index));
		aip->cur_path_index = new_path_index;
	} else {
		//	At end of line and have nowhere to go.
		// mprintf((0, "At end of line and can't get to goal.  Creating new path.  Frame %i\n", FrameCount));
		move_towards_segment_center(objp);
		create_path_to_station(objp, 15);
	}

}

//	----------------------------------------------------------------------------------------------------------
//					DEBUG FUNCTIONS FOLLOW
//	----------------------------------------------------------------------------------------------------------

#ifdef EDITOR
int	Test_size = 1000;

void test_create_path_many(void)
{
	point_seg	point_segs[200];
	short			num_points;

	int			i;

	for (i=0; i<Test_size; i++) {
                Cursegp = &Segments[(d_rand() * (Highest_segment_index + 1)) / D_RAND_MAX];
                Markedsegp = &Segments[(d_rand() * (Highest_segment_index + 1)) / D_RAND_MAX];
		create_path_points(&Objects[0], Cursegp-Segments, Markedsegp-Segments, point_segs, &num_points, -1, 0, 0, -1);
	}

}

void test_create_path(void)
{
	point_seg	point_segs[200];
	short			num_points;

	create_path_points(&Objects[0], Cursegp-Segments, Markedsegp-Segments, point_segs, &num_points, -1, 0, 0, -1);

}

void show_path(int start_seg, int end_seg, point_seg *psp, short length)
{
	printf("[%3i:%3i (%3i):] ", start_seg, end_seg, length);

	while (length--)
		printf("%3i ", psp[length].segnum);

	printf("\n");
}

//	For all segments in mine, create paths to all segments in mine, print results.
void test_create_all_paths(void)
{
	int	start_seg, end_seg;
	short	resultant_length;

	Point_segs_free_ptr = Point_segs;

	for (start_seg=0; start_seg<=Highest_segment_index-1; start_seg++) {
		mprintf((0, "."));
		if (Segments[start_seg].segnum != -1) {
			for (end_seg=start_seg+1; end_seg<=Highest_segment_index; end_seg++) {
				if (Segments[end_seg].segnum != -1) {
					create_path_points(&Objects[0], start_seg, end_seg, Point_segs_free_ptr, &resultant_length, -1, 0, 0, -1);
					show_path(start_seg, end_seg, Point_segs_free_ptr, resultant_length);
				}
			}
		}
	}
}

//--anchor--int	Num_anchors;
//--anchor--int	Anchor_distance = 3;
//--anchor--int	End_distance = 1;
//--anchor--int	Anchors[MAX_SEGMENTS];

//--anchor--int get_nearest_anchor_distance(int segnum)
//--anchor--{
//--anchor--	short	resultant_length, minimum_length;
//--anchor--	int	anchor_index;
//--anchor--
//--anchor--	minimum_length = 16383;
//--anchor--
//--anchor--	for (anchor_index=0; anchor_index<Num_anchors; anchor_index++) {
//--anchor--		create_path_points(&Objects[0], segnum, Anchors[anchor_index], Point_segs_free_ptr, &resultant_length, -1, 0, 0, -1);
//--anchor--		if (resultant_length != 0)
//--anchor--			if (resultant_length < minimum_length)
//--anchor--				minimum_length = resultant_length;
//--anchor--	}
//--anchor--
//--anchor--	return minimum_length;
//--anchor--
//--anchor--}
//--anchor--
//--anchor--void create_new_anchor(int segnum)
//--anchor--{
//--anchor--	Anchors[Num_anchors++] = segnum;
//--anchor--}
//--anchor--
//--anchor--//	A set of anchors is within N units of all segments in the graph.
//--anchor--//	Anchor_distance = how close anchors can be.
//--anchor--//	End_distance = how close you can be to the end.
//--anchor--void test_create_all_anchors(void)
//--anchor--{
//--anchor--	int	nearest_anchor_distance;
//--anchor--	int	segnum,i;
//--anchor--
//--anchor--	Num_anchors = 0;
//--anchor--
//--anchor--	for (segnum=0; segnum<=Highest_segment_index; segnum++) {
//--anchor--		if (Segments[segnum].segnum != -1) {
//--anchor--			nearest_anchor_distance = get_nearest_anchor_distance(segnum);
//--anchor--			if (nearest_anchor_distance > Anchor_distance)
//--anchor--				create_new_anchor(segnum);
//--anchor--		}
//--anchor--	}
//--anchor--
//--anchor--	//	Set selected segs.
//--anchor--	for (i=0; i<Num_anchors; i++)
//--anchor--		Selected_segs[i] = Anchors[i];
//--anchor--	N_selected_segs = Num_anchors;
//--anchor--
//--anchor--}
//--anchor--
//--anchor--int	Test_path_length = 5;
//--anchor--
//--anchor--void test_create_n_segment_path(void)
//--anchor--{
//--anchor--	point_seg	point_segs[200];
//--anchor--	short			num_points;
//--anchor--
//--anchor--	create_path_points(&Objects[0], Cursegp-Segments, -2, point_segs, &num_points, Test_path_length, 0, 0, -1);
//--anchor--}

short	Player_path_length=0;
int	Player_hide_index=-1;
int	Player_cur_path_index=0;
int	Player_following_path_flag=0;

//	------------------------------------------------------------------------------------------------------------------
//	Set orientation matrix and velocity for objp based on its desire to get to a point.
void player_path_set_orient_and_vel(object *objp, vms_vector *goal_point)
{
	vms_vector	cur_vel = objp->mtype.phys_info.velocity;
	vms_vector	norm_cur_vel;
	vms_vector	norm_vec_to_goal;
	vms_vector	cur_pos = objp->pos;
	vms_vector	norm_fvec;
	fix			speed_scale;
	fix			dot;
	fix			max_speed;

	max_speed = F1_0*50;

	vm_vec_sub(&norm_vec_to_goal, goal_point, &cur_pos);
	vm_vec_normalize_quick(&norm_vec_to_goal);

	norm_cur_vel = cur_vel;
	vm_vec_normalize_quick(&norm_cur_vel);

	norm_fvec = objp->orient.fvec;
	vm_vec_normalize_quick(&norm_fvec);

	dot = vm_vec_dot(&norm_vec_to_goal, &norm_fvec);

	//	If very close to facing opposite desired vector, perturb vector
	if (dot < -15*F1_0/16) {
		//mprintf((0, "Facing away from goal, abruptly turning\n"));
		norm_cur_vel = norm_vec_to_goal;
	} else {
		norm_cur_vel.x += norm_vec_to_goal.x/2;
		norm_cur_vel.y += norm_vec_to_goal.y/2;
		norm_cur_vel.z += norm_vec_to_goal.z/2;
	}

	vm_vec_normalize_quick(&norm_cur_vel);

	//	Set speed based on this robot type's maximum allowed speed and how hard it is turning.
	//	How hard it is turning is based on the dot product of (vector to goal) and (current velocity vector)
	//	Note that since 3*F1_0/4 is added to dot product, it is possible for the robot to back up.

	//	Set speed and orientation.
	if (dot < 0)
		dot /= 4;

	speed_scale = fixmul(max_speed, dot);
	vm_vec_scale(&norm_cur_vel, speed_scale);
	objp->mtype.phys_info.velocity = norm_cur_vel;
	ai_turn_towards_vector(&norm_vec_to_goal, objp, F1_0);

}

//	----------------------------------------------------------------------------------------------------------
//	Optimization: If current velocity will take robot near goal, don't change velocity
void player_follow_path(object *objp)
{
	vms_vector	goal_point;
	fix			dist_to_goal;
	int			count, forced_break, original_index;
	int			goal_seg;
	fix			threshold_distance;

	if (!Player_following_path_flag)
		return;

	if (Player_hide_index == -1)
		return;

	if (Player_path_length < 2)
		return;

	goal_point = Point_segs[Player_hide_index + Player_cur_path_index].point;
	goal_seg = Point_segs[Player_hide_index + Player_cur_path_index].segnum;
	Assert((goal_seg >= 0) && (goal_seg <= Highest_segment_index));
	dist_to_goal = vm_vec_dist_quick(&goal_point, &objp->pos);

	if (Player_cur_path_index < 0)
		Player_cur_path_index = 0;
	else if (Player_cur_path_index >= Player_path_length)
		Player_cur_path_index = Player_path_length-1;

	goal_point = Point_segs[Player_hide_index + Player_cur_path_index].point;

	count=0;

	//	If near goal, pick another goal point.
	forced_break = 0;		//	Gets set for short paths.
	//original_dir = 1;
	original_index = Player_cur_path_index;
	threshold_distance = fixmul(vm_vec_mag_quick(&objp->mtype.phys_info.velocity), FrameTime)*2 + F1_0*2;

	while ((dist_to_goal < threshold_distance) && !forced_break) {

		if (count > 1)
			mprintf((0, "."));

		//	----- Debug stuff -----
		if (count++ > 20) {
			mprintf((1,"Problem following path for player.  Aborting.\n"));
			break;
		}

		//	Advance to next point on path.
		Player_cur_path_index += 1;

		//	See if next point wraps past end of path (in either direction), and if so, deal with it based on mode.
		if ((Player_cur_path_index >= Player_path_length) || (Player_cur_path_index < 0)) {
			Player_following_path_flag = 0;
			forced_break = 1;
		}

		//	If went all the way around to original point, in same direction, then get out of here!
		if (Player_cur_path_index == original_index) {
			mprintf((0, "Forcing break because player path wrapped, count = %i.\n", count));
			Player_following_path_flag = 0;
			forced_break = 1;
		}

		goal_point = Point_segs[Player_hide_index + Player_cur_path_index].point;
		dist_to_goal = vm_vec_dist_quick(&goal_point, &objp->pos);

	}	//	end while

	//	Set velocity (objp->mtype.phys_info.velocity) and orientation (objp->orient) for this object.
	player_path_set_orient_and_vel(objp, &goal_point);

}


//	------------------------------------------------------------------------------------------------------------------
//	Create path for player from current segment to goal segment.
void create_player_path_to_segment(int segnum)
{
	object		*objp = ConsoleObject;

	Player_path_length=0;
	Player_hide_index=-1;
	Player_cur_path_index=0;
	Player_following_path_flag=0;

	if (create_path_points(objp, objp->segnum, segnum, Point_segs_free_ptr, &Player_path_length, 100, 0, 0, -1) == -1)
		mprintf((0, "Unable to form path of length %i for myself\n", 100));

	Player_following_path_flag = 1;

	Player_hide_index = Point_segs_free_ptr - Point_segs;
	Player_cur_path_index = 0;
	Point_segs_free_ptr += Player_path_length;
	if (Point_segs_free_ptr - Point_segs + MAX_PATH_LENGTH*2 > MAX_POINT_SEGS) {
		//Int3();	//	Contact Mike: This is curious, though not deadly. /eip++;g
		ai_reset_all_paths();
	}

}

int	Player_goal_segment = -1;

void check_create_player_path(void)
{
	if (Player_goal_segment != -1)
		create_player_path_to_segment(Player_goal_segment);

	Player_goal_segment = -1;
}

#endif

//	----------------------------------------------------------------------------------------------------------
//					DEBUG FUNCTIONS ENDED
//	----------------------------------------------------------------------------------------------------------

