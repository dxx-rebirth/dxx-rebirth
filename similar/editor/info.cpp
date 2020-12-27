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
 * Print debugging info in ui.
 *
 */

#include <cinttypes>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inferno.h"
#include "window.h"
#include "segment.h"
#include "event.h"
#include "gr.h"
#include "ui.h"
#include "editor.h"
#include "editor/esegment.h"
#include "dxxerror.h"
#include "textures.h"
#include "object.h"
#include "ai.h"
#include "texpage.h"		// Textue selection paging stuff
#include "objpage.h"		// Object selection paging stuff
#include "wall.h"
#include "switch.h"
#include "info.h"
#include "d_levelstate.h"

int init_info;

namespace dcx {

namespace {

typedef const char object_type_name[13];
typedef const char control_type_name[15];
typedef const char movement_type_name[15];
typedef const char ai_type_name[13];

static object_type_name &get_object_type(int num)
{
	switch (num) {
		case OBJ_NONE:
			return "OBJ_NONE    ";
		case OBJ_WALL:
			return "OBJ_WALL    ";
		case OBJ_FIREBALL:
			return "OBJ_FIREBALL";
		case OBJ_ROBOT:
			return "OBJ_ROBOT   ";
		case OBJ_HOSTAGE:
			return "OBJ_HOSTAGE ";
		case OBJ_PLAYER:
			return "OBJ_PLAYER  ";
		case OBJ_WEAPON:
			return "OBJ_WEAPON  ";
		case OBJ_CAMERA:
			return "OBJ_CAMERA  ";
		case OBJ_POWERUP:
			return "OBJ_POWERUP ";
		default:
			return " (unknown)  ";
	}
}

static control_type_name &get_control_type(const typename object_base::control_type num)
{
	switch (num) {
		case object::control_type::None:
			return "CT_NONE       ";
		case object::control_type::ai:
			return "CT_AI         ";
		case object::control_type::explosion:
			return "CT_EXPLOSION  ";
		case object::control_type::flying:
			return "CT_FLYING     ";
		case object::control_type::slew:
			return "CT_SLEW       ";
		case object::control_type::flythrough:
			return "CT_FLYTHROUGH ";
		case object::control_type::weapon:
			return "CT_WEAPON     ";
		default:
			return " (unknown)    ";
	}
}

static movement_type_name &get_movement_type(const typename object_base::movement_type num)
{
	switch (num) {
		case object::movement_type::None:
			return "MT_NONE       ";
		case object::movement_type::physics:
			return "MT_PHYSICS    ";
		case object::movement_type::spinning:
			return "MT_SPINNING   ";
		default:
			return " (unknown)    ";
	}
}

static void info_display_object_placement(grs_canvas &canvas, const grs_font &cv_font, const vcobjptridx_t &obj)
{
	gr_uprintf(canvas, cv_font, 0, 0, "Object id: %4d\n", obj.get_unchecked_index());
	auto &o = *obj;
	gr_uprintf(canvas, cv_font, 0, 16, "Type: %s\n", get_object_type(o.type));
	gr_uprintf(canvas, cv_font, 0, 32, "Movmnt: %s\n", get_movement_type(o.movement_source));
	gr_uprintf(canvas, cv_font, 0, 48, "Cntrl: %s\n", get_control_type(o.control_source));
}

}

}

namespace dsx {

namespace {

struct info_dialog_window : window
{
	using window::window;
	virtual window_event_result event_handler(const d_event &) override;
};

static ai_type_name &get_ai_behavior(ai_behavior num)
{
	switch (num) {
		case ai_behavior::AIB_STILL:
			return "STILL       ";
		case ai_behavior::AIB_NORMAL:
			return "NORMAL      ";
#if defined(DXX_BUILD_DESCENT_I)
		case ai_behavior::AIB_HIDE:
			return "HIDE        ";
		case ai_behavior::AIB_FOLLOW_PATH:
			return "FOLLOW_PATH ";
#elif defined(DXX_BUILD_DESCENT_II)
		case ai_behavior::AIB_BEHIND:
			return "BEHIND      ";
		case ai_behavior::AIB_SNIPE:
			return "SNIPE       ";
		case ai_behavior::AIB_FOLLOW:
			return "FOLLOW      ";
#endif
		case ai_behavior::AIB_RUN_FROM:
			return "RUN_FROM    ";
		default:
			return " (unknown)  ";
	}
}

//	---------------------------------------------------------------------------------------------------
static void info_display_object_placement(grs_canvas &canvas, const grs_font &cv_font)
{
	if (Cur_object_index == object_none)
	{
		gr_ustring(canvas, cv_font, 0, 0, "Object id: None\n");
		return;
	}
	auto &Objects = LevelUniqueObjectState.Objects;
	const auto &obj = Objects.vcptridx(Cur_object_index);
	gr_uprintf(canvas, cv_font, 0, 64, "Mode: %s\n", get_ai_behavior(obj->ctype.ai_info.behavior));
	::dcx::info_display_object_placement(canvas, cv_font, obj);
}

}

}

namespace dcx {

namespace {

//	---------------------------------------------------------------------------------------------------
static void info_display_segsize(grs_canvas &canvas, const grs_font &cv_font)
{
		const char *name;
		switch (SegSizeMode) {
			case SEGSIZEMODE_FREE:		name = "free   ";	break;
			case SEGSIZEMODE_ALL:		name = "all    ";	break;
			case SEGSIZEMODE_CURSIDE:	name = "curside";	break;
			case SEGSIZEMODE_EDGE:		name = "edge   ";	break;
			case SEGSIZEMODE_VERTEX:	name = "vertex ";	break;
			default:
				throw std::runtime_error("illegal value for SegSizeMode in " __FILE__ "/info_display_segsize");
		}
	gr_uprintf(canvas, cv_font, 0, 0, "Mode: %s\n", name);
}

//	------------------------------------------------------------------------------------
static void clear_pad_display(grs_canvas &canvas)
{
	gr_clear_canvas(canvas, CWHITE);
	gr_set_fontcolor(canvas, CBLACK, CWHITE);
}

}

}

namespace dsx {

namespace {

//	---------------------------------------------------------------------------------------------------
static void info_display_default(grs_canvas &canvas, const grs_font &cv_font)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &TmapInfo = LevelUniqueTmapInfoState.TmapInfo;

	{
		init_info = 0;
	}

	gr_set_fontcolor(canvas, CBLACK, CWHITE);

	//--------------- Number of segments ----------------

	gr_uprintf(canvas, cv_font, 0, 0, "Segments: %4d/%4" PRIuFAST32, LevelSharedSegmentState.Num_segments, static_cast<uint_fast32_t>(MAX_SEGMENTS));

	//---------------- Number of vertics -----------------
	
	gr_uprintf(canvas, cv_font, 0, 16, "Vertices: %4d/%4" PRIuFAST32, LevelSharedVertexState.Num_vertices, static_cast<uint_fast32_t>(MAX_VERTICES));

	//---------------- Number of objects -----------------
	
	{
		const auto num_objects = LevelUniqueObjectState.num_objects;
		gr_uprintf(canvas, cv_font, 0, 32, "Objs: %3d/%3" DXX_PRI_size_type, num_objects, MAX_OBJECTS.value);
	}

  	//--------------- Current_segment_number -------------
	//--------------- Current_side_number -------------

	{
		gr_uprintf(canvas, cv_font, 0, 48, "Cursegp/side: %3hu/%1d", static_cast<segnum_t>(Cursegp), Curside);
		unique_segment &useg = *Cursegp;
		auto &uside = useg.sides[Curside];
		gr_uprintf(canvas, cv_font, 0, 128, " tmap1,2,o: %3d/%3dx%1u", get_texture_index(uside.tmap_num), get_texture_index(uside.tmap_num2), static_cast<unsigned>(get_texture_rotation_low(uside.tmap_num2)));
	}

	//--------------- Current_vertex_numbers -------------

	{
		using U = std::underlying_type<vertnum_t>::type;
		gr_uprintf(canvas, cv_font, 0, 64, "{%3u,%3u,%3u,%3u,", static_cast<U>(Cursegp->verts[0]), static_cast<U>(Cursegp->verts[1]), static_cast<U>(Cursegp->verts[2]), static_cast<U>(Cursegp->verts[3]));
		gr_uprintf(canvas, cv_font, 0, 80, " %3u,%3u,%3u,%3u}", static_cast<U>(Cursegp->verts[4]), static_cast<U>(Cursegp->verts[5]), static_cast<U>(Cursegp->verts[6]), static_cast<U>(Cursegp->verts[7]));
	}

	//--------------- Num walls/links/triggers -------------------------

	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	gr_uprintf(canvas, cv_font, 0, 96, "Walls %3d", Walls.get_count());

	//--------------- Num triggers ----------------------

	auto &Triggers = LevelUniqueWallSubsystemState.Triggers;
	gr_uprintf(canvas, cv_font, 0, 112, "Num_triggers %2d", Triggers.get_count());

	//--------------- Current texture number -------------
	gr_uprintf(canvas, cv_font, 0, 144, "Tex/Light: %3d %5.2f", CurrentTexture, f2fl(TmapInfo[CurrentTexture].lighting));
}

//	------------------------------------------------------------------------------------
window_event_result info_dialog_window::event_handler(const d_event &event)
{
	static int old_padnum = -1;
	int        padnum;		// always redraw
	grs_canvas *save_canvas = grd_curcanv;

	switch (event.type)
	{
		case EVENT_WINDOW_DRAW:
		{
			gr_set_current_canvas(w_canv);
			auto &canvas = *grd_curcanv;

			padnum = ui_pad_get_current();
			Assert(padnum <= MAX_PAD_ID);

			if (padnum != old_padnum) {
				old_padnum = padnum;
				clear_pad_display(canvas);
			}

			switch (padnum) {
				case OBJECT_PAD_ID:			// Object placement
					info_display_object_placement(canvas, *canvas.cv_font);
					break;
				case SEGSIZE_PAD_ID:			// Segment sizing
					info_display_segsize(canvas, *canvas.cv_font);
					break;
				default:
					info_display_default(canvas, *canvas.cv_font);
					break;
			}
			grd_curcanv = save_canvas;
			return window_event_result::handled;
		}
		case EVENT_WINDOW_CLOSE:
			Pad_info = NULL;
			break;
			
		default:
			break;
	}
	return window_event_result::ignored;
}

}

//	------------------------------------------------------------------------------------
window *info_window_create(void)
{
	auto wind = window_create<info_dialog_window>(*Canv_editor, PAD_X + 250, PAD_Y + 8, 180, 160);
	wind->set_modal(0);
	return wind;
}

}
