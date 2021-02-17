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
 * Miscellaneous functions stripped out of med.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "gr.h"
#include "ui.h"
#include "3d.h"
#include "u_mem.h"
#include "dxxerror.h"
#include "func.h"
#include "inferno.h"
#include "editor/editor.h"
#include "editor/esegment.h"
#include "editor/medmisc.h"
#include "segment.h"
#include "render.h"
#include "object.h"
#include "meddraw.h"		// For draw_World
#include "kdefs.h"

#if DXX_USE_OGL
#include "ogl_init.h"
#endif

#include "compiler-range_for.h"

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
static long dist_2d(long x,long y) {
	return sqrt(static_cast<double>(x) * static_cast<double>(x) + static_cast<double>(y) * static_cast<double>(y));
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
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	Gameview_lockstep = !Gameview_lockstep;
    if (Gameview_lockstep == 0) {
        //if (keypress != KEY_L)
            diagnostic_message("[Ctrl-L] - Lock mode OFF");
        //else
        //    diagnostic_message("Lock mode OFF");
    }
    if (Gameview_lockstep) {
        //if (keypress != KEY_L)
            diagnostic_message("[Ctrl-L] Lock mode ON");
        //else
        //    diagnostic_message("Lock mode ON");

		Cursegp = imsegptridx(ConsoleObject->segnum);
		med_create_new_segment_from_cursegp();
		auto &vcvertptr = Vertices.vcptr;
		set_view_target_from_segment(vcvertptr, Cursegp);
		Update_flags = UF_ED_STATE_CHANGED;
	}
    return Gameview_lockstep;
}

int medlisp_delete_segment(void)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
    if (!med_delete_segment(Cursegp)) {
        if (Lock_view_to_cursegp)
		{
			auto &vcvertptr = Vertices.vcptr;
            set_view_target_from_segment(vcvertptr, Cursegp);
		}
		  autosave_mine(mine_filename);
		undo_status[Autosave_count] = "Delete Segment UNDONE.";
        Update_flags |= UF_WORLD_CHANGED;
        mine_changed = 1;
        diagnostic_message("Segment deleted.");
        warn_if_concave_segments();     // This could be faster -- just check if deleted segment was concave, warn accordingly
    }

	return 1;
}

int medlisp_scale_segment(void)
{
	vms_vector	scale;

	scale.x = fl2f(static_cast<float>(func_get_param(0)));
	scale.y = fl2f(static_cast<float>(func_get_param(1)));
	scale.z = fl2f(static_cast<float>(func_get_param(2)));
	med_create_new_segment(scale);
	med_rotate_segment(Cursegp,vm_angles_2_matrix(Seg_orientation));
	Update_flags |= UF_WORLD_CHANGED;
	mine_changed = 1;

	return 1;
}

int medlisp_rotate_segment(void)
{
	Seg_orientation.p = func_get_param(0);
	Seg_orientation.b = func_get_param(1);
	Seg_orientation.h = func_get_param(2);
	med_rotate_segment(Cursegp,vm_angles_2_matrix(Seg_orientation));
	Update_flags |= UF_WORLD_CHANGED | UF_VIEWPOINT_MOVED;
	mine_changed = 1;
	return 1;
}

int ToggleLockViewToCursegp(void)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	Lock_view_to_cursegp = !Lock_view_to_cursegp;
	Update_flags = UF_ED_STATE_CHANGED;
	if (Lock_view_to_cursegp) {
        //if (keypress != KEY_V+KEY_CTRLED)
            diagnostic_message("[ctrl-V] View locked to Cursegp.");
        //else
        //    diagnostic_message("View locked to Cursegp.");
		auto &vcvertptr = Vertices.vcptr;
        set_view_target_from_segment(vcvertptr, Cursegp);
    } else {
        //if (keypress != KEY_V+KEY_CTRLED)
            diagnostic_message("[ctrl-V] View not locked to Cursegp.");
        //else
        //    diagnostic_message("View not locked to Cursegp.");
    }
    return Lock_view_to_cursegp;
}

int ToggleDrawAllSegments()
{
	Draw_all_segments = !Draw_all_segments;
	Update_flags = UF_ED_STATE_CHANGED;
    if (Draw_all_segments == 1) {
        //if (keypress != KEY_A+KEY_CTRLED)
            diagnostic_message("[ctrl-A] Draw all segments ON.");
        //else
        //    diagnostic_message("Draw all segments ON.");
    }
    if (Draw_all_segments == 0) {
        //if (keypress != KEY_A+KEY_CTRLED)
            diagnostic_message("[ctrl-A] Draw all segments OFF.");
        //else
        //    diagnostic_message("Draw all segments OFF.");
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
        //if (keypress != KEY_D+KEY_CTRLED)
            diagnostic_message("[ctrl-D] Coordinate axes ON.");
        //else
        //    diagnostic_message("Coordinate axes ON.");
    }
    if (Show_axes_flag == 0) {
        //if (keypress != KEY_D+KEY_CTRLED)
            diagnostic_message("[ctrl-D] Coordinate axes OFF.");
        //else
        //    diagnostic_message("Coordinate axes OFF.");
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
#if 1	//def OGL
	Update_flags = UF_ALL;
#endif

	if (!render_3d_in_big_window)
		range_for (const auto vn, Views)
			if (vn->ev_changed || (Update_flags & (UF_WORLD_CHANGED | UF_VIEWPOINT_MOVED | UF_ED_STATE_CHANGED)))
			{
				draw_world(vn->ev_canv, vn, Cursegp, Big_depth);
				vn->ev_changed = 0;
			}

	if (Update_flags & (UF_WORLD_CHANGED|UF_GAME_VIEW_CHANGED|UF_ED_STATE_CHANGED)) {
		grs_canvas *render_canv,*show_canv;
		
		if (render_3d_in_big_window) {
			render_canv = LargeView.ev_canv;
			show_canv = LargeView.ev_canv;
		}
		else {
			render_canv	= Canv_editor_game;
			show_canv	= Canv_editor_game;
		}

		gr_set_current_canvas(render_canv);
		render_frame(*grd_curcanv, 0);

		Assert(render_canv->cv_bitmap.bm_w == show_canv->cv_bitmap.bm_w &&
				 render_canv->cv_bitmap.bm_h == show_canv->cv_bitmap.bm_h);
		(void)show_canv;
	}

	Update_flags=UF_NONE;       //clear flags

	return 1;
}

void med_point_2_vec(grs_canvas *canv,vms_vector &v,short sx,short sy)
{
	gr_set_current_canvas(canv);

	g3_start_frame(*grd_curcanv);
	g3_set_view_matrix(Viewer->pos,Viewer->orient,Render_zoom);
	g3_point_2_vec(v,sx,sy);
	g3_end_frame();
}
 
void draw_world_from_game(void)
{
	if (ModeFlag == 2)
		draw_world(Views[0]->ev_canv,Views[0],Cursegp,Big_depth);
}

int UndoCommand()
{  
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	int u;

    u = undo();
    if (Lock_view_to_cursegp)
	{
		auto &vcvertptr = Vertices.vcptr;
		set_view_target_from_segment(vcvertptr, Cursegp);
	}
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
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
   if (med_attach_segment(Cursegp, vmsegptr(&New_segment), Curside, AttachSide)==4) // Used to be WBACK instead of Curside
        diagnostic_message("Cannot attach segment - already a connection on current side.");
   else {
		if (Lock_view_to_cursegp)
		{
			auto &vcvertptr = Vertices.vcptr;
			set_view_target_from_segment(vcvertptr, Cursegp);
		}
		vm_angvec_make(&Seg_orientation,0,0,0);
		Curside = WBACK;
		Update_flags |= UF_WORLD_CHANGED;
	   autosave_mine(mine_filename);
		undo_status[Autosave_count] = "Attach Segment UNDONE.";
		mine_changed = 1;
		warn_if_concave_segment(Cursegp);
      }
	return 1;
}

int CreateDefaultNewSegment()
{
	// Create a default segment for New_segment.
	med_create_new_segment({DEFAULT_X_SIZE, DEFAULT_Y_SIZE, DEFAULT_Z_SIZE});
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
		if (Markedsegp->segnum != segment_none) {
			using std::swap;
			swap(Markedsegp, Cursegp);
			swap(Markedside, Curside);
			med_create_new_segment_from_cursegp();
			Update_flags |= UF_ED_STATE_CHANGED;
			mine_changed = 1;
		}
	return 1;
}

int ClearSelectedList(void)
{
	Selected_segs.clear();
	Update_flags |= UF_WORLD_CHANGED;

	diagnostic_message("Selected list cleared.");

	return 1;
}


int ClearFoundList(void)
{
	Found_segs.clear();
	Update_flags |= UF_WORLD_CHANGED;

	diagnostic_message("Found list cleared.");

	return 1;
}

// ---------------------------------------------------------------------------------------------------
void set_view_target_from_segment(fvcvertptr &vcvertptr, const shared_segment &sp)
{
	if (Funky_chase_mode)
		{
		//set_chase_matrix(sp);
		}
	else {
		vms_vector tv{};
		range_for (const auto &v, sp.verts)
			vm_vec_add2(tv, vcvertptr(v));

		vm_vec_scale(tv,F1_0/MAX_VERTICES_PER_SEGMENT);
		Ed_view_target = tv;
	}
	Update_flags |= UF_VIEWPOINT_MOVED;

}
