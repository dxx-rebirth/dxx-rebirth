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
 * $Source: /cvs/cvsroot/d2x/main/editor/medmisc.c,v $
 * $Revision: 1.1 $
 * $Author: btb $
 * $Date: 2004-12-19 13:54:27 $
 * 
 * Miscellaneous functions stripped out of med.c
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  1999/06/14 22:03:51  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.1  1995/03/06  15:20:50  john
 * New screen mode method.
 * 
 * Revision 2.0  1995/02/27  11:36:40  john
 * Version 2.0. Ansi-fied.
 * 
 * Revision 1.31  1994/11/27  23:17:20  matt
 * Made changes for new mprintf calling convention
 * 
 * Revision 1.30  1994/11/17  14:48:11  mike
 * validation functions moved from editor to game.
 * 
 * Revision 1.29  1994/08/25  21:56:15  mike
 * IS_CHILD stuff.
 * 
 * Revision 1.28  1994/08/09  16:06:00  john
 * Added the ability to place players.  Made old
 * Player variable be ConsoleObject.
 * 
 * Revision 1.27  1994/07/21  17:25:43  matt
 * Took out unused func medlisp_create_new_mine() and its prototype
 * 
 * Revision 1.26  1994/07/21  13:27:01  matt
 * Cleaned up render code and added error checking
 * 
 * Revision 1.25  1994/07/20  15:32:52  matt
 * Added func to call g3_point_2_vec() for texture-mapped window
 * 
 * Revision 1.24  1994/07/15  15:26:53  yuan
 * Fixed warning
 * 
 * Revision 1.23  1994/07/14  14:45:16  yuan
 * Added function to set default segment and attach.
 * 
 * Revision 1.22  1994/07/14  09:46:34  yuan
 * Make E attach segment as well as make default.
 * 
 * 
 * Revision 1.21  1994/07/11  18:39:17  john
 * Reversed y axis roll.
 * 
 * Revision 1.20  1994/07/06  16:36:32  mike
 * Add hook for game to render wireframe view: draw_world_from_game.
 * 
 * Revision 1.19  1994/06/24  14:08:31  john
 * Changed calling params for render_frame.
 * 
 * Revision 1.18  1994/06/23  15:54:02  matt
 * Finished hacking in 3d rendering in big window
 * 
 * Revision 1.17  1994/06/22  00:32:56  matt
 * New version, without all the errors of the last version. Sorry.
 * 
 * Revision 1.15  1994/05/23  14:48:54  mike
 * make current segment be add segment.
 * 
 * Revision 1.14  1994/05/19  12:09:35  matt
 * Use new vecmat macros and globals
 * 
 * Revision 1.13  1994/05/14  17:17:55  matt
 * Got rid of externs in source (non-header) files
 * 
 * Revision 1.12  1994/05/09  23:35:06  mike
 * Add ClearFoundList, which is probably no longer being called.
 * 
 * Revision 1.11  1994/05/04  14:11:40  mike
 * Increase render depth from 4 to 6 by default.
 * 
 * Revision 1.10  1994/04/27  21:00:25  matt
 * Made texture-mapped window redraw when editor state variables (such as
 * current object) have changed.
 * 
 * Revision 1.9  1994/03/31  12:03:38  matt
 * Cleaned up includes
 * 
 * Revision 1.8  1994/02/17  11:31:21  matt
 * Changes in object system
 * 
 * Revision 1.7  1994/02/11  11:05:14  yuan
 * Make chase mode unsettable... Gives a warning on the mono.
 * 
 * Revision 1.6  1994/01/21  17:37:24  matt
 * Moved code from render_frame() to caller, making code cleaner
 * 
 * Revision 1.5  1994/01/11  18:12:43  yuan
 * compress_mines removed.  Now it is called within
 * the gamesave.min save whenever we go into the game.
 * 
 * Revision 1.4  1994/01/05  10:54:15  john
 * New object code by John
 * 
 * Revision 1.3  1993/12/29  16:15:27  mike
 * Kill scale field from segment struct.
 * 
 * Revision 1.2  1993/12/17  12:05:00  john
 * Took stuff out of med.c; moved into medsel.c, meddraw.c, medmisc.c
 * 
 * Revision 1.1  1993/12/17  08:35:47  john
 * Initial revision
 * 
 * 
 */


#ifdef RCS
static char rcsid[] = "$Id: medmisc.c,v 1.1 2004-12-19 13:54:27 btb Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#ifdef __MSDOS__
#include <process.h>
#endif

#include "gr.h"
#include "ui.h"
#include "3d.h"

#include "u_mem.h"
#include "error.h"
#include "mono.h"
#include "key.h"
#include "func.h"

#include "inferno.h"
#include "editor/editor.h"
#include "segment.h"

#include "render.h"
#include "screens.h"
#include "object.h"

#include "texpage.h"		// For texpage_goto_first
#include "meddraw.h"		// For draw_World
#include "game.h"

//return 2d distance, i.e, sqrt(x*x + y*y)
#ifdef __WATCOMC__
long dist_2d(long x,long y);

#pragma aux dist_2d parm [eax] [ebx] value [eax] modify [ecx edx] = \
	"imul	eax"			\
	"xchg	ebx,eax"		\
	"mov	ecx,edx"		\
	"imul	eax"			\
	"add	eax,ebx"		\
	"adc	edx,ecx"		\
	"call	quad_sqrt";
#else
#include <math.h>
long dist_2d(long x,long y) {
	return (long)sqrt((double)x * (double)x + (double)y * (double)y);
}
#endif

// Given mouse movement in dx, dy, returns a 3x3 rotation matrix in RotMat.
// Taken from Graphics Gems III, page 51, "The Rolling Ball"

void GetMouseRotation( int idx, int idy, vms_matrix * RotMat )
{
	fix dr, cos_theta, sin_theta, denom, cos_theta1;
	fix Radius = i2f(100);
	fix dx,dy;
	fix dxdr,dydr;

	idy *= -1;

	dx = i2f(idx); dy = i2f(idy);

	dr = dist_2d(dx,dy);

	denom = dist_2d(Radius,dr);

	cos_theta = fixdiv(Radius,denom);
	sin_theta = fixdiv(dr,denom);

	cos_theta1 = f1_0 - cos_theta;

	dxdr = fixdiv(dx,dr);
	dydr = fixdiv(dy,dr);

	RotMat->rvec.x = cos_theta + fixmul(fixmul(dydr,dydr),cos_theta1);
	RotMat->uvec.x = - fixmul(fixmul(dxdr,dydr),cos_theta1);
	RotMat->fvec.x = fixmul(dxdr,sin_theta);

	RotMat->rvec.y = RotMat->uvec.x;
	RotMat->uvec.y = cos_theta + fixmul(fixmul(dxdr,dxdr),cos_theta1);
	RotMat->fvec.y = fixmul(dydr,sin_theta);

	RotMat->rvec.z = -RotMat->fvec.x;
	RotMat->uvec.z = -RotMat->fvec.y;
	RotMat->fvec.z = cos_theta;

}

int Gameview_lockstep;		//if set, view is locked to Curseg

int ToggleLockstep()
{
	Gameview_lockstep = !Gameview_lockstep;
    if (Gameview_lockstep == 0) {
        if (last_keypress != KEY_L)
            diagnostic_message("[L] - Lock mode OFF");
        else
            diagnostic_message("Lock mode OFF");
    }
    if (Gameview_lockstep) {
        if (last_keypress != KEY_L)
            diagnostic_message("[L] Lock mode ON");
        else
            diagnostic_message("Lock mode ON");

      Cursegp = &Segments[ConsoleObject->segnum];
		med_create_new_segment_from_cursegp();
		set_view_target_from_segment(Cursegp);
		Update_flags = UF_ED_STATE_CHANGED;
	}
    return Gameview_lockstep;
}

int medlisp_delete_segment(void)
{
    if (!med_delete_segment(Cursegp)) {
        if (Lock_view_to_cursegp)
            set_view_target_from_segment(Cursegp);
		  autosave_mine(mine_filename);
		  strcpy(undo_status[Autosave_count], "Delete Segment UNDONE.");
        Update_flags |= UF_WORLD_CHANGED;
        mine_changed = 1;
        diagnostic_message("Segment deleted.");
        warn_if_concave_segments();     // This could be faster -- just check if deleted segment was concave, warn accordingly
    }

	return 1;
}

int medlisp_scale_segment(void)
{
	vms_matrix	rotmat;
	vms_vector	scale;

	scale.x = fl2f((float) func_get_param(0));
	scale.y = fl2f((float) func_get_param(1));
	scale.z = fl2f((float) func_get_param(2));
	med_create_new_segment(&scale);
	med_rotate_segment(Cursegp,vm_angles_2_matrix(&rotmat,&Seg_orientation));
	Update_flags |= UF_WORLD_CHANGED;
	mine_changed = 1;

	return 1;
}

int medlisp_rotate_segment(void)
{
	vms_matrix	rotmat;

	Seg_orientation.p = func_get_param(0);
	Seg_orientation.b = func_get_param(1);
	Seg_orientation.h = func_get_param(2);
	med_rotate_segment(Cursegp,vm_angles_2_matrix(&rotmat,&Seg_orientation));
	Update_flags |= UF_WORLD_CHANGED | UF_VIEWPOINT_MOVED;
	mine_changed = 1;
	return 1;
}

int ToggleLockViewToCursegp(void)
{
	Lock_view_to_cursegp = !Lock_view_to_cursegp;
	Update_flags = UF_ED_STATE_CHANGED;
	if (Lock_view_to_cursegp) {
        if (last_keypress != KEY_V+KEY_CTRLED)
            diagnostic_message("[ctrl-V] View locked to Cursegp.");
        else
            diagnostic_message("View locked to Cursegp.");
        set_view_target_from_segment(Cursegp);
    } else {
        if (last_keypress != KEY_V+KEY_CTRLED)
            diagnostic_message("[ctrl-V] View not locked to Cursegp.");
        else
            diagnostic_message("View not locked to Cursegp.");
    }
    return Lock_view_to_cursegp;
}

int ToggleDrawAllSegments()
{
	Draw_all_segments = !Draw_all_segments;
	Update_flags = UF_ED_STATE_CHANGED;
    if (Draw_all_segments == 1) {
        if (last_keypress != KEY_A+KEY_CTRLED)
            diagnostic_message("[ctrl-A] Draw all segments ON.");
        else
            diagnostic_message("Draw all segments ON.");
    }
    if (Draw_all_segments == 0) {
        if (last_keypress != KEY_A+KEY_CTRLED)
            diagnostic_message("[ctrl-A] Draw all segments OFF.");
        else
            diagnostic_message("Draw all segments OFF.");
    }
    return Draw_all_segments;
}

int	Big_depth=6;

int IncreaseDrawDepth(void)
{
	Big_depth++;
	Update_flags = UF_ED_STATE_CHANGED;
	return 1;
}

int DecreaseDrawDepth(void)
{
	if (Big_depth > 1) {
		Big_depth--;
		Update_flags = UF_ED_STATE_CHANGED;
	}
	return 1;
}


int ToggleCoordAxes()
{
			//  Toggle display of coordinate axes.
	Show_axes_flag = !Show_axes_flag;
	LargeView.ev_changed = 1;
    if (Show_axes_flag == 1) {
        if (last_keypress != KEY_D+KEY_CTRLED)
            diagnostic_message("[ctrl-D] Coordinate axes ON.");
        else
            diagnostic_message("Coordinate axes ON.");
    }
    if (Show_axes_flag == 0) {
        if (last_keypress != KEY_D+KEY_CTRLED)
            diagnostic_message("[ctrl-D] Coordinate axes OFF.");
        else
            diagnostic_message("Coordinate axes OFF.");
    }
    return Show_axes_flag;
}

int med_keypad_goto_prev()
{
	ui_pad_goto_prev();
	return 0;
}

int med_keypad_goto_next()
{
	ui_pad_goto_next();
	return 0;
}

int med_keypad_goto()
{
	ui_pad_goto(func_get_param(0));
	return 0;
}

int render_3d_in_big_window=0;

int medlisp_update_screen()
{
	int vn;

if (!render_3d_in_big_window)
	for (vn=0;vn<N_views;vn++)
		if (Views[vn]->ev_changed || (Update_flags & (UF_WORLD_CHANGED|UF_VIEWPOINT_MOVED|UF_ED_STATE_CHANGED))) {
			draw_world(Views[vn]->ev_canv,Views[vn],Cursegp,Big_depth);
			Views[vn]->ev_changed = 0;
		}

	if (Update_flags & (UF_WORLD_CHANGED|UF_GAME_VIEW_CHANGED|UF_ED_STATE_CHANGED)) {
		grs_canvas temp_canvas;
		grs_canvas *render_canv,*show_canv;
		
		if (render_3d_in_big_window) {
			
			gr_init_sub_canvas(&temp_canvas,canv_offscreen,0,0,
				LargeView.ev_canv->cv_bitmap.bm_w,LargeView.ev_canv->cv_bitmap.bm_h);

			render_canv = &temp_canvas;
			show_canv = LargeView.ev_canv;

		}
		else {
			render_canv	= VR_offscreen_buffer;
			show_canv	= Canv_editor_game;
		}

		gr_set_current_canvas(render_canv);
		render_frame(0);

		Assert(render_canv->cv_bitmap.bm_w == show_canv->cv_bitmap.bm_w &&
				 render_canv->cv_bitmap.bm_h == show_canv->cv_bitmap.bm_h);

		ui_mouse_hide();
		gr_bm_ubitblt(show_canv->cv_bitmap.bm_w,show_canv->cv_bitmap.bm_h,
						  0,0,0,0,&render_canv->cv_bitmap,&show_canv->cv_bitmap);
		ui_mouse_show();
	}

	Update_flags=UF_NONE;       //clear flags

	return 1;
}

void med_point_2_vec(grs_canvas *canv,vms_vector *v,short sx,short sy)
{
	gr_set_current_canvas(canv);

	g3_start_frame();
	g3_set_view_matrix(&Viewer->pos,&Viewer->orient,Render_zoom);

	g3_point_2_vec(v,sx,sy);

	g3_end_frame();
}


 
void draw_world_from_game(void)
{
	if (ModeFlag == 2)
		draw_world(Views[0]->ev_canv,Views[0],Cursegp,Big_depth);
}

int UndoCommand()
{   int u;

    u = undo();
    if (Lock_view_to_cursegp)
		set_view_target_from_segment(Cursegp);
    if (u == 0) {
        if (Autosave_count==9) diagnostic_message(undo_status[0]);
            else
                diagnostic_message(undo_status[Autosave_count+1]);
        }
        else
	 if (u == 1) diagnostic_message("Can't Undo.");
        else
	 if (u == 2) diagnostic_message("Can't Undo - Autosave OFF");
    Update_flags |= UF_WORLD_CHANGED;
	 mine_changed = 1;
    warn_if_concave_segments();
    return 1;
}


int ToggleAutosave()
{
	Autosave_flag = !Autosave_flag;
	if (Autosave_flag == 1) 
      diagnostic_message("Autosave ON.");
        else
		diagnostic_message("Autosave OFF.");
   return Autosave_flag;
}


int AttachSegment()
{
   if (med_attach_segment(Cursegp, &New_segment, Curside, AttachSide)==4) // Used to be WBACK instead of Curside
        diagnostic_message("Cannot attach segment - already a connection on current side.");
   else {
		if (Lock_view_to_cursegp)
			set_view_target_from_segment(Cursegp);
		vm_angvec_make(&Seg_orientation,0,0,0);
		Curside = WBACK;
		Update_flags |= UF_WORLD_CHANGED;
	   autosave_mine(mine_filename);
	   strcpy(undo_status[Autosave_count], "Attach Segment UNDONE.\n");
		mine_changed = 1;
		warn_if_concave_segment(Cursegp);
      }
	return 1;
}

int ForceTotalRedraw()
{
	Update_flags = UF_ALL;
	return 1;
}


#if ORTHO_VIEWS
int SyncLargeView()
{
	// Make large view be same as one of the orthogonal views.
	Large_view_index = (Large_view_index + 1) % 3;  // keep in 0,1,2 for top, front, right
	switch (Large_view_index) {
		case 0: LargeView.ev_matrix = TopView.ev_matrix; break;
		case 1: LargeView.ev_matrix = FrontView.ev_matrix; break;
		case 2: LargeView.ev_matrix = RightView.ev_matrix; break;
	}
	Update_flags |= UF_VIEWPOINT_MOVED;
	return 1;
}
#endif

int DeleteCurSegment()
{
	// Delete current segment.
    med_delete_segment(Cursegp);
    autosave_mine(mine_filename);
    strcpy(undo_status[Autosave_count], "Delete segment UNDONE.");
    if (Lock_view_to_cursegp)
		set_view_target_from_segment(Cursegp);
    Update_flags |= UF_WORLD_CHANGED;
    mine_changed = 1;
    diagnostic_message("Segment deleted.");
    warn_if_concave_segments();     // This could be faster -- just check if deleted segment was concave, warn accordingly

    return 1;
}

int CreateDefaultNewSegment()
{
	// Create a default segment for New_segment.
	vms_vector  tempvec;
	med_create_new_segment(vm_vec_make(&tempvec,DEFAULT_X_SIZE,DEFAULT_Y_SIZE,DEFAULT_Z_SIZE));
	mine_changed = 1;

	return 1;
}

int CreateDefaultNewSegmentandAttach()
{
	CreateDefaultNewSegment();
	AttachSegment();

	return 1;
}

int ExchangeMarkandCurseg()
{
	// If Markedsegp != Cursegp, and Markedsegp->segnum != -1, exchange Markedsegp and Cursegp
	if (Markedsegp)
		if (Markedsegp->segnum != -1) {
			segment *tempsegp;
			int     tempside;
			tempsegp = Markedsegp;  Markedsegp = Cursegp;   Cursegp = tempsegp;
			tempside = Markedside;  Markedside = Curside;   Curside = tempside;
			med_create_new_segment_from_cursegp();
			Update_flags |= UF_ED_STATE_CHANGED;
			mine_changed = 1;
		}
	return 1;
}

int medlisp_add_segment()
{
	AttachSegment();
//segment *ocursegp = Cursegp;
//	med_attach_segment(Cursegp, &New_segment, Curside, WFRONT); // Used to be WBACK instead of Curside
//med_propagate_tmaps_to_segments(ocursegp,Cursegp);
//	set_view_target_from_segment(Cursegp);
////	while (!vm_angvec_make(&Seg_orientation,0,0,0));
//	Curside = WBACK;

	return 1;
}


int ClearSelectedList(void)
{
	N_selected_segs = 0;
	Update_flags |= UF_WORLD_CHANGED;

	diagnostic_message("Selected list cleared.");

	return 1;
}


int ClearFoundList(void)
{
	N_found_segs = 0;
	Update_flags |= UF_WORLD_CHANGED;

	diagnostic_message("Found list cleared.");

	return 1;
}




// ---------------------------------------------------------------------------------------------------
// Do chase mode.
//	View current segment (Cursegp) from the previous segment.
void set_chase_matrix(segment *sp)
{
	int			v;
	vms_vector	forvec = ZERO_VECTOR, upvec;
	vms_vector	tv = ZERO_VECTOR;
	segment		*psp;

	// move back two segments, if possible, else move back one, if possible, else use current
	if (IS_CHILD(sp->children[WFRONT])) {
		psp = &Segments[sp->children[WFRONT]];
		if (IS_CHILD(psp->children[WFRONT]))
			psp = &Segments[psp->children[WFRONT]];
	} else
		psp = sp;

	for (v=0; v<MAX_VERTICES_PER_SEGMENT; v++)
		vm_vec_add2(&forvec,&Vertices[sp->verts[v]]);
	vm_vec_scale(&forvec,F1_0/MAX_VERTICES_PER_SEGMENT);

	for (v=0; v<MAX_VERTICES_PER_SEGMENT; v++)
		vm_vec_add2(&tv,&Vertices[psp->verts[v]]);
	vm_vec_scale(&tv,F1_0/MAX_VERTICES_PER_SEGMENT);

	Ed_view_target = forvec;

	vm_vec_sub2(&forvec,&tv);

	extract_up_vector_from_segment(psp,&upvec);

	if (!((forvec.x == 0) && (forvec.y == 0) && (forvec.z == 0)))
		vm_vector_2_matrix(&LargeView.ev_matrix,&forvec,&upvec,NULL);
}



// ---------------------------------------------------------------------------------------------------
void set_view_target_from_segment(segment *sp)
{
	vms_vector	tv = ZERO_VECTOR;
	int			v;

	if (Funky_chase_mode)
		{
		mprintf((0, "Trying to set chase mode\n"));
		//set_chase_matrix(sp);
		}
	else {
		for (v=0; v<MAX_VERTICES_PER_SEGMENT; v++)
			vm_vec_add2(&tv,&Vertices[sp->verts[v]]);

		vm_vec_scale(&tv,F1_0/MAX_VERTICES_PER_SEGMENT);

		Ed_view_target = tv;

	}
	Update_flags |= UF_VIEWPOINT_MOVED;

}

















