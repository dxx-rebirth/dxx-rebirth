/* $Id: render.h,v 1.4 2003-10-10 09:36:35 btb Exp $ */
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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Header for rendering-based functions
 *
 * Old Log:
 * Revision 1.1  1995/05/16  16:01:51  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:33:00  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.17  1994/11/30  12:33:33  mike
 * prototype Clear_window.
 *
 * Revision 1.16  1994/11/02  16:19:52  matt
 * Increased size of extra object buffer
 *
 * Revision 1.15  1994/07/25  00:02:49  matt
 * Various changes to accomodate new 3d, which no longer takes point numbers
 * as parms, and now only takes pointers to points.
 *
 * Revision 1.14  1994/07/24  14:37:42  matt
 * Added angles for player head
 *
 * Revision 1.13  1994/06/24  17:01:34  john
 * Add VFX support; Took Game Sequencing, like EndGame and stuff and
 * took it out of game.c and into gameseq.c
 *
 * Revision 1.12  1994/06/16  10:55:57  matt
 * Made a bunch of test code dependent on #defines
 *
 * Revision 1.11  1994/06/01  00:01:36  matt
 * Added mine destruction flashing effect
 *
 * Revision 1.10  1994/05/22  18:47:36  mike
 * make Render_list a globally accessible variable.
 *
 * Revision 1.9  1994/05/22  15:29:32  mike
 * Separation of lighting from render.c to lighting.c.
 *
 * Revision 1.8  1994/05/14  17:59:39  matt
 * Added extern.
 *
 * Revision 1.7  1994/05/14  17:15:17  matt
 * Got rid of externs in source (non-header) files
 *
 * Revision 1.6  1994/02/17  11:32:41  matt
 * Changes in object system
 *
 * Revision 1.5  1994/01/21  17:31:48  matt
 * Moved code from render_frame() to caller, making code cleaner
 *
 * Revision 1.4  1994/01/06  09:46:12  john
 * Added removable walls... all code that checked for
 * children to see if a wall was a doorway, i changed
 * to yuan's wall_is_doorway function that is in wall.c...
 * doesn't work yet.
 *
 * Revision 1.3  1994/01/05  11:25:47  john
 * Changed Player_zoom to Render_zoom
 *
 * Revision 1.2  1994/01/05  10:53:43  john
 * New object code by John.
 *
 * Revision 1.1  1993/11/04  14:01:43  matt
 * Initial revision
 *
 *
 */


#ifndef _RENDER_H
#define _RENDER_H

#include "3d.h"

#include "object.h"

#define MAX_RENDER_SEGS     500
#define OBJS_PER_SEG        5
#define N_EXTRA_OBJ_LISTS   50

extern int Clear_window;    // 1 = Clear whole background window, 2 = clear view portals into rest of world, 0 = no clear

void render_frame(fix eye_offset, int window_num);  //draws the world into the current canvas

// cycle the flashing light for when mine destroyed
void flash_frame();

int find_seg_side_face(short x,short y,int *seg,int *side,int *face,int *poly);

// these functions change different rendering parameters
// all return the new value of the parameter

// how may levels deep to render
int inc_render_depth(void);
int dec_render_depth(void);
int reset_render_depth(void);

// how many levels deep to render in perspective
int inc_perspective_depth(void);
int dec_perspective_depth(void);
int reset_perspective_depth(void);

// misc toggles
int toggle_outline_mode(void);
int toggle_show_only_curside(void);

// When any render function needs to know what's looking at it, it
// should access Render_viewer_object members.
extern fix Render_zoom;     // the player's zoom factor

// This is used internally to render_frame(), but is included here so AI
// can use it for its own purposes.
extern unsigned char visited[MAX_SEGMENTS];

extern int N_render_segs;
extern short Render_list[MAX_RENDER_SEGS];

#ifdef EDITOR
extern int Render_only_bottom;
#endif


// Set the following to turn on player head turning
extern int Use_player_head_angles;

// If the above flag is set, these angles specify the orientation of the head
extern vms_angvec Player_head_angles;

//
// Routines for conditionally rotating & projecting points
//

// This must be called at the start of the frame if rotate_list() will be used
void render_start_frame(void);

// Given a list of point numbers, rotate any that haven't been rotated
// this frame
g3s_codes rotate_list(int nv, short *pointnumlist);

// Given a list of point numbers, project any that haven't been projected
void project_list(int nv, short *pointnumlist);

extern void render_mine(int start_seg_num, fix eye_offset, int window_num);

extern void update_rendered_data(int window_num, object *viewer, int rear_view_flag, int user);

#endif /* _RENDER_H */
