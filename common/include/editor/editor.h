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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Header for editor functions, data strcutures, etc.
 *
 */

#pragma once

#include "dxxsconf.h"
#include "dsx-ns.h"
#include "vecmat.h"
#include "ui.h"
#include "fmtcheck.h"

#ifdef __cplusplus
#include "fwd-window.h"
#include "fwd-segment.h"
#include "objnum.h"

/*
 * Constants
 *
 */

#define ORTHO_VIEWS 0			// set to 1 to enable 3 orthogonal views
#define ED_SCREEN_W	800		//width of editor screen
#define ED_SCREEN_H	600		//height of editor screen

#define MENUBAR_H		16

#define GAMEVIEW_X	1		//where the 320x200 game window goes
#define GAMEVIEW_Y	1+MENUBAR_H
#define GAMEVIEW_W	320
#define GAMEVIEW_H	200

#define STATUS_X	0
#define STATUS_H	18
#define STATUS_Y	(ED_SCREEN_H-STATUS_H)
#define STATUS_W	ED_SCREEN_W

#define LVIEW_X	1			//large view
#define LVIEW_Y	(GAMEVIEW_Y+GAMEVIEW_H+2)
#define LVIEW_W	447
#define LVIEW_H	(STATUS_Y-LVIEW_Y-2)

#define TMAPBOX_X	(LVIEW_X+LVIEW_W+4)	//location of first one
#define TMAPBOX_Y	(LVIEW_Y+2)
#define TMAPBOX_W	64
#define TMAPBOX_H	64

#define TMAPCURBOX_X (TMAPBOX_X + 4*(TMAPBOX_W + 3))
#define TMAPCURBOX_Y (TMAPBOX_Y + TMAPBOX_H)

#define OBJCURBOX_X (TMAPCURBOX_X)
#define OBJCURBOX_Y (TMAPCURBOX_Y + 3*(TMAPBOX_H + 2) -40)

#define PAD_X (GAMEVIEW_X + GAMEVIEW_W + 16)
#define PAD_Y (GAMEVIEW_Y + 4)

#define SMALLVIEW_W	173		//width of small view windows
#define SMALLVIEW_H	148		//height of small view windows

#define TVIEW_X	(LVIEW_X+LVIEW_W+2)		//top view
#define TVIEW_Y	LVIEW_Y
#define TVIEW_W	SMALLVIEW_W
#define TVIEW_H	SMALLVIEW_H

#define FVIEW_X	TVIEW_X						//front view
#define FVIEW_Y	(TVIEW_Y+SMALLVIEW_H+2)
#define FVIEW_W	SMALLVIEW_W
#define FVIEW_H	SMALLVIEW_H

#define RVIEW_X	(TVIEW_X+SMALLVIEW_W+2)	//right view
#define RVIEW_Y	FVIEW_Y	
#define RVIEW_W	SMALLVIEW_W
#define RVIEW_H	SMALLVIEW_H

#define GVIEW_X	RVIEW_X						//group view
#define GVIEW_Y	TVIEW_Y	
#define GVIEW_W	SMALLVIEW_W
#define GVIEW_H	SMALLVIEW_H

//there were color constants here, but I moved them to meddraw.c - Matt

#define	SEGMOVE_PAD_ID		0
#define	SEGSIZE_PAD_ID		1
#define	CURVE_PAD_ID		2	
#define	TEXTURE_PAD_ID		3
#define	OBJECT_PAD_ID		4
#define	OBJMOV_PAD_ID		5
#define	GROUP_PAD_ID		6
#define	LIGHTING_PAD_ID	7
#define	TEST_PAD_ID			8
#define	MAX_PAD_ID			8

/*
 * Strucures
 * 
 */

#define VF_ANGLES 0
#define VF_MATRIX 1

// Default size of a segment
#define	DEFAULT_X_SIZE		F1_0*20
#define	DEFAULT_Y_SIZE		F1_0*20
#define	DEFAULT_Z_SIZE		F1_0*20

//	Scale factor from 3d units (integer portion) to uv coordinates (integer portion)
#define	VMAG	(F1_0 / (DEFAULT_X_SIZE/F1_0))
#define	UMAG	VMAG		// unused

//	Number of segments which can be found (size of Found_segs[])

#define	MAX_GROUPS			10
#define	ROT_GROUP			MAX_GROUPS

//	Modes for segment sizing
#define SEGSIZEMODE_FREE 		1
#define SEGSIZEMODE_ALL	 		2
#define SEGSIZEMODE_CURSIDE	3
#define SEGSIZEMODE_EDGE		4
#define SEGSIZEMODE_VERTEX		5

#define SEGSIZEMODE_MIN			SEGSIZEMODE_FREE
#define SEGSIZEMODE_MAX			SEGSIZEMODE_VERTEX

//defines a view for an editor window
struct editor_view
{
	short ev_num;				//each view has it's own number
	short ev_changed;			//set to true if view changed
	grs_canvas *ev_canv;		//points to this window's canvas
	fix ev_dist;				//the distance from the view point
	vms_matrix ev_matrix;	//the view matrix
	fix ev_zoom;				//zoom for this window
};

enum class editor_gamestate : uint8_t
{
	none,		// editing a level, not a gamestate
	unsaved,	// just pressed delete-e from a game
	saved		// saved state. Do we want to restore it when launching editor from menu? Ask user.
};

/*
 * Global variables
 * 
 */

extern array<editor_view *, ORTHO_VIEWS ? 4 : 1> Views;
extern int Large_view_index;
extern std::unique_ptr<UI_GADGET_USERBOX> LargeViewBox, GameViewBox, GroupViewBox;
extern int Found_seg_index;				// Index in Found_segs corresponding to Cursegp
extern editor_gamestate gamestate;
extern grs_font_ptr editor_font;

extern	vms_vector Ed_view_target;		// what editor is looking at

extern	class window *Pad_info;		// Keypad text

extern	int		Show_axes_flag;		// 0 = don't show, !0 = do show coordinate axes in *Cursegp orientation

namespace dcx {
extern   int		Autosave_count;		// Current counter for which autosave mine we are "on"
extern	int		Autosave_flag;			// Whether or not Autosave is on.
extern	struct tm Editor_time_of_day;
}

extern	int		SegSizeMode;			// Mode = 0/1 = not/is legal to move bound vertices, 

#ifdef dsx
namespace dsx {
void init_editor(void);

}
#endif
#ifdef dsx
namespace dsx {

//	Initialize all vertices to inactive status.
extern void init_all_vertices(void);

}
#endif

//	Returns true if vertex vi is contained in exactly one segment, else returns false.
extern int is_free_vertex(int vi);

//	Set existing vertex vnum to value *vp.
int med_set_vertex(unsigned vnum, const vertex &vp);

void med_combine_duplicate_vertices(array<uint8_t, MAX_VERTICES> &);

#ifdef dsx
namespace dsx {
// Attach side newside of newseg to side destside of destseg.
// Copies *newseg into global array Segments, increments Num_segments.
// Forms a weld between the two segments by making the new segment fit to the old segment.
// Updates number of faces per side if necessitated by new vertex coordinates.
// Return value:
//  0 = successful attach
//  1 = No room in Segments[].
//  2 = No room in Vertices[].
int med_attach_segment(vmsegptridx_t destseg, vmsegptr_t newseg, int destside, int newside);

// Delete a segment.
// Deletes a segment from the global array Segments.
// Updates Cursegp to be the segment to which the deleted segment was connected.  If there is
//	more than one connected segment, the new Cursegp will be the segment with the highest index
//	of connection in the deleted segment (highest index = front)
// Return value:
//  0 = successful deletion
//  1 = unable to delete
int med_delete_segment(vmsegptridx_t sp);

// Rotate the segment *seg by the pitch, bank, heading defined by *rot, destructively
// modifying its four free vertices in the global array Vertices.
// It is illegal to rotate a segment which has MAX_SIDES_PER_SEGMENT != 1.
// Pitch, bank, heading are about the point which is the average of the four points
// forming the side of connection.
// Return value:
//  0 = successful rotation
//  1 = MAX_SIDES_PER_SEGMENT makes rotation illegal (connected to 0 or 2+ segments)
//  2 = Rotation causes degeneracy, such as self-intersecting segment.
int med_rotate_segment(vmsegptridx_t seg, const vms_matrix &rotmat);

//    Create a wall which can be removed.
//    Creates wall at sp->sides[side], making it part of segment sp
//    Removable walls must be placed between two connected segments.  You should add the removable
//    wall on both sides.  In fact, you really must.
void create_removable_wall(vmsegptridx_t sp, int side, int tmap_num);
}
#endif

// Saves mine contained in Segments[] and Vertices[].
// Num_segments = number of segments in mine.
// Num_vertices = number of vertices in mine.
// Cursegp = current segment.
// Saves Num_segments, and index of current segment (which is Cursegp - Segments), which will be converted to a pointer
// and written to Cursegp in med_load_mine.
// Returns:
//  0 = successfully saved.
//  1 = unable to save.
extern	int med_save_mine(const char *name);

// Updates the screen... (I put the prototype here for curves.c)
extern   int medlisp_update_screen();

// Returns 0 if no error, 1 if error, whatever that might be.
// Sets globals:
//    Num_segments
//    Num_vertices
//    Cursegp = pointer to only segment.
#ifdef dsx
namespace dsx {
extern	int create_new_mine(void);

}
#endif
#ifdef dsx
namespace dsx {
//	Create a segment given center, dimensions, rotation matrix.
//	Note that the created segment will always have planar sides and rectangular cross sections.
//	It will be created with walls on all sides, ie not connected to anything.
void med_create_segment(vmsegptridx_t sp,fix cx, fix cy, fix cz, fix length, fix width, fix height, const vms_matrix &mp);

//	Create New_segment with sizes found in *scale.
void med_create_new_segment(const vms_vector &scale);

//	Create New_segment with sizes found in Cursegp.
extern void med_create_new_segment_from_cursegp(void);

//	Create a new segment and use it to form a bridge between two existing segments.
//	Specify two segment:side pairs.  If either segment:side is not open (ie, segment->children[side] != -1)
//	then it is not legal to form the brider.
//	Return:
//		0	bridge segment formed
//		1	unable to form bridge because one (or both) of the sides is not open.
//	Note that no new vertices are created by this process.
int med_form_bridge_segment(vmsegptridx_t seg1, int side1, vmsegptridx_t seg2, int side2);
}
#endif

//	Compress mine at Segments and Vertices by squeezing out all holes.
//	If no holes (ie, an unused segment followed by a used segment), then no action.
//	If Cursegp or Markedsegp is a segment which gets moved to fill in a hole, then
//	they are properly updated.
extern	void med_compress_mine(void);

void update_matrix_based_on_side(vms_matrix &rotmat,int destside);

// Curves stuff.

#define ACCURACY 0.1*F1_0

struct vms_equation
{
    union {
            struct {fix x3, x2, x1, x0, y3, y2, y1, y0, z3, z2, z1, z0;} n;
		array<array<fix, 4>, 3> xyz;
    };
};

// Q(t) = (2t^3 - 3t^2 + 1) p1 + (-2t^3 + 3t^2) p4 + (t^3 - 2t^2 + t) r1 + (t^3 - t^2 ) r4

extern vms_vector evaluate_curve(vms_equation *coeffs, int degree, fix t);

fix curve_dist(vms_equation *coeffs, int degree, fix t0, const vms_vector &p0, fix dist);

extern void plot_parametric(vms_equation *coeffs, fix min_t, fix max_t, fix del_t);

// Curve generation routine.
// Returns 1 if curve is generated.
// Returns 0 if no curve.
extern int generate_curve( fix r1scale, fix r4scale );

// Deletes existing curve generated in generate_curve().
extern void delete_curve();

#ifdef dsx
namespace dsx {
void med_extract_matrix_from_segment(const shared_segment &sp, vms_matrix &rotmat);

//	Assign default u,v coordinates to all sides of a segment.
//	This routine should only be used for segments which are not connected to anything else,
//	ie the segment created at mine creation.
void assign_default_uvs_to_segment(vmsegptridx_t segp);
void assign_default_uvs_to_side(vmsegptridx_t segp, unsigned side);

//	Assign u,v coordinates to con_seg, con_common_side from base_seg, base_common_side
//	They are connected at the edge defined by the vertices abs_id1, abs_id2.
void med_assign_uvs_to_side(vmsegptridx_t con_seg, int con_common_side, vmsegptr_t base_seg, int base_common_side, int abs_id1, int abs_id2);

//	Create coordinate axes in orientation of specified segment, stores vertices at *vp.
void create_coordinate_axes_from_segment(vmsegptr_t sp, array<unsigned, 16> &vertnums);

//	Set Vertex_active to number of occurrences of each vertex.
//	Set Num_vertices.
extern	void set_vertex_counts(void);

//	Modify seg2 to share side2 with seg1:side1.  This forms a connection between
//	two segments without creating a new segment.  It modifies seg2 by sharing
//	vertices from seg1.  seg1 is not modified.  Four vertices from seg2 are
//	deleted.
//	If the four vertices forming side2 in seg2 are not free, the joint is not formed.
//	Return code:
//		0			joint formed
//		1			unable to form joint because one or more vertices of side2 is not free
//		2			unable to form joint because side1 is already used
int med_form_joint(vmsegptridx_t seg1, int side1, vmsegptridx_t seg2, int side2);
}

// The current texture... use by saying something=bm_lock_bitmap(CurrentTexture)
extern int CurrentTexture;

namespace dsx {
void med_propagate_tmaps_to_segments(vmsegptridx_t base_seg,vmsegptridx_t con_seg, int uv_only_flag);
void med_propagate_tmaps_to_back_side(vmsegptridx_t base_seg, int back_side, int uv_only_flag);

//	Find segment adjacent to sp:side.
//	Adjacent means a segment which shares all four vertices.
//	Return true if segment found and fill in segment in adj_sp and side in adj_side.
//	Return false if unable to find, in which case adj_sp and adj_side are undefined.
int med_find_adjacent_segment_side(vmsegptridx_t sp, int side, imsegptridx_t &adj_sp, int *adj_side);

// Finds the closest segment and side to sp:side.
int med_find_closest_threshold_segment_side(vmsegptridx_t sp, int side, imsegptridx_t &adj_sp, int *adj_side, fix threshold);

// Select previous segment.
//	If there is a connection on the side opposite to the current side, then choose that segment.
// If there is no connecting segment on the opposite face, try any segment.

// Select next segment.
//	If there is a connection on the current side, then choose that segment.
// If there is no connecting segment on the current side, try any segment.

//	Copy texture maps in newseg to nsp.
void copy_uvs_seg_to_seg(vmsegptr_t nsp, vcsegptr_t newseg);

//	Return true if segment is concave.

//	Return N_found_segs = number of concave segments in mine.
//	Segment ids stored at Found_segs
extern void find_concave_segs(void);

//	High level call.  Check for concave segments, print warning message (using editor_status)
//	if any concave segments.
//	Calls find_concave_segs, therefore N_found_segs gets set, and Found_segs filled in.
extern void warn_if_concave_segments(void);

//	Warn if segment s is concave.
void warn_if_concave_segment(vmsegptridx_t s);
}

//	Add a vertex to the vertex list.
int med_add_vertex(const vertex &vp);

//	Add a vertex to the vertex list which may be identical to another vertex (in terms of coordinates).
//	Don't scan list, looking for presence of a vertex with same coords, add this one.
int med_create_duplicate_vertex(const vertex &vp);

namespace dsx {
//	Create a new segment, duplicating exactly, including vertex ids and children, the passed segment.
segnum_t med_create_duplicate_segment(vmsegptr_t sp);

//	Returns the index of a free segment.
//	Scans the Segments array.
extern segnum_t get_free_segment_number(void);
}
#endif

//      Diagnostic message.
#define diagnostic_message editor_status
#define diagnostic_message_fmt editor_status_fmt

//      Editor status message.
extern void editor_status_fmt(const char *format, ... ) __attribute_format_printf(1, 2);
#define editor_status_fmt(F,...)	dxx_call_printf_checked(editor_status_fmt,editor_status,(),(F),##__VA_ARGS__)

// Variables in editor.c that the k*.c files need

#define UF_NONE             0x000       //nothing has changed
#define UF_WORLD_CHANGED    0x001       //something added or deleted
#define UF_VIEWPOINT_MOVED  0x002       //what we're watching has moved

#define UF_GAME_VIEW_CHANGED 0x004		//the game window changed    
#define UF_ED_STATE_CHANGED  0x008		//something like curside,curseg changed

#define UF_ALL					0xffffffff  //all flags

extern uint        Update_flags;
extern int         Funky_chase_mode;
extern vms_angvec  Seg_orientation;
extern int         mine_changed;
extern int         ModeFlag;
extern editor_view *current_view;

//the view for the different windows
extern editor_view LargeView;
extern editor_view TopView;
extern editor_view FrontView;
extern editor_view RightView;

extern int SafetyCheck();
#ifdef dsx
namespace dsx {
int save_mine_data_compiled(PHYSFS_File *SaveFile);

}
#endif
void editor_status( const char *text);

extern int MacroNumEvents;
extern int MacroStatus;

//extern	int	Highest_segment_index;			// Highest index in Segments, an efficiency hack
extern	int	Lock_view_to_cursegp;			// !0 means whenever cursegp changes, view it

//	eglobal.c
extern	int	Num_tilings;						// number of tilings/wall
extern	int	Degenerate_segment_found;

namespace dcx {

// Initializes autosave system.
// Sets global Autosave_count to 0.
extern void init_autosave(void);

// Closes autosave system.
// Deletes all autosaved files.
extern void close_autosave(void);

// Saves current mine to name.miX where name = suffix of mine name and X = Autosave_count.
// For example, if name = "cookie.min", and Autosave_count = 3, then writes "cookie.mi3".
// Increments Autosave_count, wrapping from 9 to 0.
// (If there is no current mine name, assume "temp.min")
// Call med_save_mine to save the mine.
extern void autosave_mine(const char *name);

// Timed autosave
extern void TimedAutosave(const char *name);
extern void set_editor_time_of_day();

// Undo function
extern int undo(void);
extern array<const char *, 10> undo_status;
}

extern char mine_filename[PATH_MAX];

//	group.c
int RotateSegmentNew(vms_angvec *pbh);
int rotate_segment_new(const vms_angvec &pbh);

// The current object type and id declared in eglobal.c
extern short Cur_object_type;
extern short Cur_object_id;

//	From med.c
extern int DisplayCurrentRobotType(void);
extern objnum_t			Cur_object_index;

extern int render_3d_in_big_window;
extern void move_object_to_mouse_click(void);

//these are instances of canvases, pointed to by variables below
extern grs_canvas _canv_editor_game;		//the game on the editor screen

//these are pointers to our canvases
extern grs_canvas *Canv_editor;			//the editor screen
extern grs_canvas *const Canv_editor_game; //the game on the editor screen

extern UI_DIALOG * EditorWindow;

void med_point_2_vec(grs_canvas *canv,vms_vector &v,short sx,short sy);

//shutdown ui on the editor screen
void close_editor_screen(void);

#ifdef dsx
namespace dsx {
//    From eobject.c
int place_object(vmsegptridx_t segp, const vms_vector &object_pos, short object_type, short object_id);

// from ksegsize.c
void med_extract_up_vector_from_segment_side(vmsegptr_t sp, int sidenum, vms_vector &vp);
void med_extract_right_vector_from_segment_side(vmsegptr_t sp, int sidenum, vms_vector &vp);
}
#endif

//	In medmisc.c
extern void draw_world_from_game(void);

//	In medrobot.c
extern void close_all_windows(void);

//	In seguvs.c

//	Amount to stretch a texture map by.
//	The two different ones are for the two dimensions of a texture map.
extern fix Stretch_scale_x, Stretch_scale_y;

#endif
