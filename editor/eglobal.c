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
 * Globals for editor.
 *
 */


#include <stdlib.h>
#include "inferno.h"
#include "segment.h"
#include "editor.h"

// Global pointer to current vertices, right now always Vertices.  Set in create_new_mine.
// segment New_segment;  // segment which can be added to the mine. // replaced by a macro
segment *Cursegp = NULL;        // Pointer to current segment in mine.
int Curside;             // Side index in 0..MAX_SIDES_PER_SEGMENT of active side.
int Curedge;             // Current edge on current side, in 0..3
int Curvert;             // Current vertex on current side, in 0..3
int AttachSide = WFRONT; // Side on segment to attach.
segment *Markedsegp = NULL;     // Marked segment, used in conjunction with *Cursegp to form joints.
int Markedside;          // Marked side on Markedsegp.

int Draw_all_segments;   // Set to 1 means draw_world draws all segments in Segments, else draw only connected segments

sbyte Vertex_active[MAX_VERTICES]; // !0 means vertex is in use, 0 means not in use.

int N_selected_segs = 0;  // Number of segments found at Selected_segs
short Selected_segs[MAX_SELECTED_SEGS]; // List of segment numbers currently selected

int N_warning_segs = 0;   // Number of segments warning-worthy, such as a concave segment
short Warning_segs[MAX_WARNING_SEGS]; // List of segment numbers currently selected

int N_found_segs = 0;    // Number of segments found with last shift-mouse-click
short Found_segs[MAX_FOUND_SEGS]; // List of warning-worthy segments

int Show_axes_flag = 0; // 0 = don't show, !0 = do show coordinate axes in *Cursegp orientation

sbyte Been_visited[MAX_SEGMENTS]; // List of segments visited in a recursive search, if element n set, segment n done been visited

// Variables global to this editor.c and the k?????.c files.
uint        Update_flags = UF_ALL;  //force total redraw
int         Funky_chase_mode = 0;
vms_angvec  Seg_orientation = {0,0,0};
vms_vector  Seg_scale = {F1_0*20,F1_0*20,F1_0*20};
int         mine_changed = 0;
int         ModeFlag;
editor_view *current_view;

int         SegSizeMode = 1; // Mode = 0/1 = not/is legal to move bound vertices, 

//the view for the different windows.
editor_view LargeView = {0,1, NULL, i2f(100),IDENTITY_MATRIX,f1_0};
#if ORTHO_VIEWS
editor_view TopView   = {1,1, NULL, i2f(100),{{f1_0,0,0},{0,0,-f1_0},{0,f1_0,0}},f1_0};
editor_view FrontView = {2,1, NULL, i2f(100),{{f1_0,0,0},{0,f1_0,0},{0,0,f1_0}},f1_0};
editor_view RightView = {3,1, NULL, i2f(100),{{0,0,f1_0},{0,f1_0,0},{f1_0,0,0}},f1_0};
#endif


editor_view *Views[ORTHO_VIEWS ? 4 : 1] = {&LargeView,
#if ORTHO_VIEWS
	&TopView,&FrontView,&RightView
#endif
	};

int N_views = (sizeof(Views) / sizeof(*Views));

int	Lock_view_to_cursegp = 1;		// !0 means whenever cursegp changes, view it

int	Num_tilings = 1;					// Number of tilings per wall

short Cur_object_index = -1;

// The current object type and id
short Cur_object_type = 4;	// OBJ_PLAYER
short Cur_object_id = 0;

//	!0 if a degenerate segment has been found.
int	Degenerate_segment_found=0;

