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
 * Globals for editor.
 *
 */


#include <stdlib.h>
#include "inferno.h"
#include "segment.h"
#include "object.h"
#include "editor.h"
#include "editor/esegment.h"

// Global pointer to current vertices, right now always Vertices.  Set in create_new_mine.
imsegptridx_t Cursegp = segment_none;        // Pointer to current segment in mine.
imsegptridx_t Markedsegp = segment_none;     // Marked segment, used in conjunction with *Cursegp to form joints.
int Curside;             // Side index in 0..MAX_SIDES_PER_SEGMENT of active side.
int Curedge;             // Current edge on current side, in 0..3
int Curvert;             // Current vertex on current side, in 0..3
int AttachSide = WFRONT; // Side on segment to attach.
int Markedside;          // Marked side on Markedsegp.

int Draw_all_segments;   // Set to 1 means draw_world draws all segments in Segments, else draw only connected segments

array<uint8_t, MAX_VERTICES> Vertex_active; // !0 means vertex is in use, 0 means not in use.

selected_segment_array_t Selected_segs; // List of segment numbers currently selected

warning_segment_array_t Warning_segs; // List of segment numbers currently selected

found_segment_array_t Found_segs; // List of warning-worthy segments

int Show_axes_flag = 0; // 0 = don't show, !0 = do show coordinate axes in *Cursegp orientation

// Variables global to this editor.c and the k?????.c files.
uint        Update_flags = UF_ALL;  //force total redraw
int         Funky_chase_mode = 0;
vms_angvec  Seg_orientation = {0,0,0};
vms_vector  Seg_scale{F1_0*20,F1_0*20,F1_0*20};
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


array<editor_view *, ORTHO_VIEWS ? 4 : 1> Views = {{&LargeView,
#if ORTHO_VIEWS
	&TopView,&FrontView,&RightView
#endif
	}};

int	Lock_view_to_cursegp = 1;		// !0 means whenever cursegp changes, view it

int	Num_tilings = 1;					// Number of tilings per wall

objnum_t Cur_object_index = object_none;

// The current object type and id
short Cur_object_type = 4;	// OBJ_PLAYER
short Cur_object_id = 0;

//	!0 if a degenerate segment has been found.
int	Degenerate_segment_found=0;

