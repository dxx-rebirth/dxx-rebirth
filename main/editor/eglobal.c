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
 * $Source: /cvs/cvsroot/d2x/main/editor/eglobal.c,v $
 * $Revision: 1.1 $
 * $Author: btb $
 * $Date: 2004-12-19 13:54:27 $
 *
 * Globals for editor.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2003/03/09 06:34:09  donut
 * change byte typedef to sbyte to avoid conflict with win32 byte which is unsigned
 *
 * Revision 1.1.1.1  1999/06/14 22:02:51  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:35:52  john
 * Version 2.0! No anonymous unions, Watcom 10.0, with no need
 * for bitmaps.tbl.
 * 
 * Revision 1.35  1994/05/23  14:48:15  mike
 * make current segment be add segment.
 * 
 * Revision 1.34  1994/05/19  12:10:30  matt
 * Use new vecmat macros and globals
 * 
 * Revision 1.33  1994/05/04  19:16:27  mike
 * Add Degenerate_segment_found.
 * 
 * Revision 1.32  1994/05/03  11:02:57  mike
 * Add SegSizeMode.
 * 
 * Revision 1.31  1994/02/16  13:49:12  mike
 * enable editor to compile out.
 * 
 * Revision 1.30  1994/02/10  15:36:35  matt
 * Various changes to make editor compile out.
 * 
 * Revision 1.29  1993/12/16  14:00:33  mike
 * Add Curvert and Curedge.
 * 
 * Revision 1.28  1993/12/10  14:48:28  mike
 * Kill orthogonal views.
 * 
 * Revision 1.27  1993/12/08  10:58:49  mike
 * Add Cur_object_index.
 * 
 * Revision 1.26  1993/12/06  18:45:45  matt
 * Changed object loading & handling
 * 
 * Revision 1.25  1993/12/02  17:51:49  john
 * Changed my variable to match Mike's.
 * 
 * Revision 1.24  1993/12/02  17:36:13  john
 * Added cur_obj_type
 * 
 * Revision 1.23  1993/11/24  14:41:16  mike
 * Add variable Num_tilings.
 * 
 * Revision 1.22  1993/11/12  16:40:55  mike
 * Add Identity_matrix, which is an identity matrix.
 * 
 * Revision 1.21  1993/11/02  13:08:17  mike
 * Add N_warning_segs and Warning_segs
 * 
 * Revision 1.20  1993/11/02  10:31:53  mike
 * Document some variables,
 * Add Been_visited, removing it from editor.c
 * Add Selected_segs[] and N_selected_segs.
 * 
 * Revision 1.19  1993/10/31  18:07:48  mike
 * Add variable Lock_view_to_cursegp.
 * 
 * Revision 1.18  1993/10/19  20:54:51  matt
 * Changed/cleaned up window updates
 * 
 * Revision 1.17  1993/10/18  18:35:43  mike
 * Move Highest_vertex_index and Highest_segment_index here because they need
 * to be globals.
 * 
 * Revision 1.16  1993/10/15  13:10:00  mike
 * Move globals from editor.c to eglobal.c
 * 
 * Revision 1.15  1993/10/14  18:08:55  mike
 * Change use of CONNECTIVITY to MAX_SIDES_PER_SEGMENT
 * 
 * Revision 1.14  1993/10/13  11:11:38  matt
 * Made coordinate axes off by default
 * 
 * Revision 1.13  1993/10/12  09:59:27  mike
 * Remove definition of Side_to_verts, which belongs in the game, not in the editor.
 * 
 * Revision 1.12  1993/10/09  15:48:07  mike
 * Change type of Vertex_active and Side_to_verts from char to byte
 * Move N_found_segs and Found_segs here from render.c
 * Add Show_axes_flag.
 * 
 * Revision 1.11  1993/10/06  11:29:58  mike
 * Add prototype for Side_opposite
 * 
 * Revision 1.10  1993/10/05  17:00:17  mike
 * Add Vertex_active.
 * 
 * Revision 1.9  1993/10/04  17:18:16  mike
 * Add variables Markedsegp, Markedside
 * 
 * Revision 1.8  1993/10/02  18:18:02  mike
 * Added Draw_all_segments.  If !0, then all segments are drawn in draw_world.  If not set, then only those segments which
 * are connected to the first segment are drawn.
 * 
 * Revision 1.7  1993/10/01  10:03:15  mike
 * Fix ordering of vertices on front face: Used to be 0,1,2,3 made it 3,2,1,0
 * 
 * Revision 1.6  1993/09/27  16:04:28  mike
 * Add Side_to_verts to replace _verts, which was local to segment.c
 * 
 * Revision 1.5  1993/09/27  15:20:52  mike
 * Add Curside, which is current side, so we can make a certain side active.
 * 
 * Revision 1.4  1993/09/23  15:01:13  mike
 * Remove game specific variables, put in mglobal.c
 * 
 * Revision 1.3  1993/09/22  10:52:17  mike
 * Add global New_segment
 * 
 * Revision 1.2  1993/09/22  09:41:21  mike
 * Change constand and variable names to conform to coding standards.
 * 
 * Revision 1.1  1993/09/20  17:06:09  mike
 * Initial revision
 * 
 *
 */

#ifdef RCS
static char rcsid[] = "$Id: eglobal.c,v 1.1 2004-12-19 13:54:27 btb Exp $";
#endif

#include <stdlib.h>
#include "inferno.h"
#include "segment.h"
#include "editor.h"

// Global pointer to current vertices, right now always Vertices.  Set in create_new_mine.
segment	New_segment;				// The segment which can be added to the mine.
segment	*Cursegp;					// Pointer to current segment in mine.
int		Curside;						// Side index in 0..MAX_SIDES_PER_SEGMENT of active side.
int		Curedge;						//	Current edge on current side, in 0..3
int		Curvert;						//	Current vertex on current side, in 0..3
int		AttachSide = WFRONT;		//	Side on segment to attach.
segment	*Markedsegp;				// Marked segment, used in conjunction with *Cursegp to form joints.
int		Markedside;					// Marked side on Markedsegp.

int		Draw_all_segments;		// Set to 1 means draw_world draws all segments in Segments, else draw only connected segments

sbyte		Vertex_active[MAX_VERTICES];	// !0 means vertex is in use, 0 means not in use.

int		N_selected_segs=0;							// Number of segments found at Selected_segs
short		Selected_segs[MAX_SELECTED_SEGS];		// List of segment numbers currently selected

int		N_warning_segs=0;								// Number of segments warning-worthy, such as a concave segment
short		Warning_segs[MAX_WARNING_SEGS];			// List of segment numbers currently selected

int		N_found_segs=0;								// Number of segments found with last shift-mouse-click
short		Found_segs[MAX_FOUND_SEGS];				// List of warning-worthy segments

int		Show_axes_flag=0;								// 0 = don't show, !0 = do show coordinate axes in *Cursegp orientation

sbyte		Been_visited[MAX_SEGMENTS];				//	List of segments visited in a recursive search, if element n set, segment n done been visited

// Variables global to this editor.c and the k?????.c files.
uint        Update_flags = UF_ALL;  //force total redraw
int			Funky_chase_mode = 0;
vms_angvec	Seg_orientation = {0,0,0};
vms_vector	Seg_scale = {F1_0*20,F1_0*20,F1_0*20};
int         mine_changed = 0;
int         ModeFlag;
editor_view *current_view;

int	SegSizeMode = 1;									// Mode = 0/1 = not/is legal to move bound vertices, 

//the view for the different windows.
editor_view LargeView = {0,1, NULL, i2f(100),{{f1_0,0,0},{0,f1_0,0},{0,0,f1_0}},f1_0};
#if ORTHO_VIEWS
editor_view TopView   = {1,1, NULL, i2f(100),{{f1_0,0,0},{0,0,-f1_0},{0,f1_0,0}},f1_0};
editor_view FrontView = {2,1, NULL, i2f(100),{{f1_0,0,0},{0,f1_0,0},{0,0,f1_0}},f1_0};
editor_view RightView = {3,1, NULL, i2f(100),{{0,0,f1_0},{0,f1_0,0},{f1_0,0,0}},f1_0};
#endif


editor_view *Views[] = {&LargeView,
#if ORTHO_VIEWS
	&TopView,&FrontView,&RightView
#endif
	};

int N_views = (sizeof(Views) / sizeof(*Views));

int	Lock_view_to_cursegp = 1;		// !0 means whenever cursegp changes, view it

int	Num_tilings = 1;					// Number of tilings per wall

short Cur_object_index = -1;

// The current robot type
int Cur_robot_type = 0;

//	!0 if a degenerate segment has been found.
int	Degenerate_segment_found=0;

