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
 * Created from version 1.11 of main\wall.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "wall.h"
#include "editor/medwall.h"
#include "inferno.h"
#include "editor/editor.h"
#include "editor/esegment.h"
#include "segment.h"
#include "dxxerror.h"
#include "event.h"
#include "game.h"
#include "gameseg.h"
#include "textures.h"
#include "switch.h"
#include "editor/eswitch.h"
#include "texmerge.h"
#include "timer.h"
#include "cntrlcen.h"
#include "key.h"
#include "centers.h"
#include "piggy.h"
#include "kdefs.h"
#include "u_mem.h"
#include "d_enumerate.h"

#include "compiler-range_for.h"
#include "d_range.h"
#include "partial_range.h"
#include "d_zip.h"
#include <memory>
#include <utility>

//-------------------------------------------------------------------------
// Variables for this module...
//-------------------------------------------------------------------------

namespace {

struct wall_dialog : UI_DIALOG
{
	using UI_DIALOG::UI_DIALOG;
	std::unique_ptr<UI_GADGET_USERBOX> wallViewBox;
	std::unique_ptr<UI_GADGET_BUTTON> quitButton, prev_wall, next_wall, blastable, door, illusory, closed_wall, goto_prev_wall, goto_next_wall, remove, bind_trigger, bind_control;
	std::array<std::unique_ptr<UI_GADGET_CHECKBOX>, 3> doorFlag;
	std::array<std::unique_ptr<UI_GADGET_RADIO>, 4> keyFlag;
	wallnum_t old_wall_num = wall_none;		// Set to some dummy value so everything works ok on the first frame.
	fix64 time;
	int framenum = 0;
	virtual window_event_result callback_handler(const d_event &) override;
};

static wall_dialog *MainWindow;
static int Current_door_type=1;

struct count_wall
{
	wallnum_t wallnum;
	segnum_t	segnum;
	short sidenum;
};

static unsigned predicate_find_nonblastable_wall(const wclip &w)
{
	if (w.num_frames == wclip_frames_none)
		return 0;
	return !(w.flags & WCF_BLASTABLE);
}

static unsigned predicate_find_blastable_wall(const wclip &w)
{
	if (w.num_frames == wclip_frames_none)
		return 0;
	return w.flags & WCF_BLASTABLE;
}

static wallnum_t allocate_wall(wall_array &Walls)
{
	using T = typename std::underlying_type<wallnum_t>::type;
	const auto current_count = Walls.get_count();
	const wallnum_t r{static_cast<T>(current_count)};
	static_assert(MAX_WALLS <= std::numeric_limits<T>::max());
	Walls.set_count(current_count + 1);
	return r;
}

//---------------------------------------------------------------------
// Add a wall (removable 2 sided)
static int add_wall(fvcvertptr &vcvertptr, wall_array &Walls, const vmsegptridx_t seg, const unsigned side)
{
	if (Walls.get_count() < MAX_WALLS-2)
  	if (IS_CHILD(seg->children[side])) {
		shared_segment &sseg = seg;
		auto &side0 = sseg.sides[side];
		if (side0.wall_num == wall_none) {
 			side0.wall_num = allocate_wall(Walls);
			}
				 
		const auto &&csegp = seg.absolute_sibling(seg->children[side]);
		auto Connectside = find_connect_side(seg, csegp);

		shared_segment &scseg = csegp;
		auto &side1 = scseg.sides[Connectside];
		if (side1.wall_num == wall_none) {
 			side1.wall_num = allocate_wall(Walls);
			}
		
		const auto t1 = build_texture1_value(CurrentTexture);
		create_removable_wall(vcvertptr, seg, side, t1);
		create_removable_wall(vcvertptr, csegp, Connectside, t1);

		return 1;
		}

	return 0;
}

static int wall_assign_door(int door_type)
{
	shared_segment &sseg = Cursegp;
	unique_segment &useg = Cursegp;
	if (sseg.sides[Curside].wall_num == wall_none) {
		editor_status("Cannot assign door. No wall at Curside.");
		return 0;
	}

	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vmwallptr = Walls.vmptr;
	auto &wall0 = *vmwallptr(sseg.sides[Curside].wall_num);
	if (wall0.type != WALL_DOOR && wall0.type != WALL_BLASTABLE)
	{
		editor_status("Cannot assign door. No door at Curside.");
		return 0;
	}

	Current_door_type = door_type;

	auto &csegp = *vmsegptr(Cursegp->children[Curside]);
	auto Connectside = find_connect_side(Cursegp, csegp);
	
 	wall0.clip_num = door_type;
	shared_segment &scseg = csegp;
	unique_segment &ucseg = csegp;
	vmwallptr(scseg.sides[Connectside].wall_num)->clip_num = door_type;

	auto &wa = GameSharedState.WallAnims[door_type];
	auto &side0 = useg.sides[Curside];
	auto &side1 = ucseg.sides[Connectside];
	if (wa.flags & WCF_TMAP1) {
		const auto t1 = build_texture1_value(wa.frames[0]);
		side0.tmap_num = t1;
		side1.tmap_num = t1;
		side0.tmap_num2 = texture2_value::None;
		side1.tmap_num2 = texture2_value::None;
	}
	else {
		side0.tmap_num2 = side1.tmap_num2 = texture2_value{wa.frames[0]};
	}

	Update_flags |= UF_WORLD_CHANGED;
	return 1;
}

static int wall_add_to_side(fvcvertptr &vcvertptr, wall_array &Walls, const vmsegptridx_t segp, unsigned side, unsigned type);

}

int wall_add_blastable()
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vcvertptr = Vertices.vcptr;
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	return wall_add_to_side(vcvertptr, Walls, Cursegp, Curside, WALL_BLASTABLE);
}

int wall_add_door()
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vcvertptr = Vertices.vcptr;
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	return wall_add_to_side(vcvertptr, Walls, Cursegp, Curside, WALL_DOOR);
}

int wall_add_closed_wall()
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vcvertptr = Vertices.vcptr;
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	return wall_add_to_side(vcvertptr, Walls, Cursegp, Curside, WALL_CLOSED);
}

int wall_add_external_wall()
{
	if (Cursegp->children[Curside] == segment_exit)
	{
		editor_status( "Wall is already external!" );
		return 1;
	}

	if (IS_CHILD(Cursegp->children[Curside])) {
		editor_status( "Cannot add external wall here - seg has children" );
		return 0;
	}

	Cursegp->children[Curside] = -2;

	return 1;
}

int wall_add_illusion()
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vcvertptr = Vertices.vcptr;
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	return wall_add_to_side(vcvertptr, Walls, Cursegp, Curside, WALL_ILLUSION);
}

namespace {

static int GotoPrevWall()
{
	shared_segment &sseg = Cursegp;
	auto &side = sseg.sides[Curside];
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptr = Walls.vcptr;
	const auto wall_count = Walls.get_count();
	if (!wall_count)
		/* No walls exist; nothing to do. */
		return 0;
	const auto current_wall = side.wall_num == wall_none
		? wall_count
		: static_cast<typename std::underlying_type<wallnum_t>::type>(side.wall_num);
	const auto previous_wall = static_cast<wallnum_t>((current_wall ? current_wall : wall_count) - 1u);
	auto &w = *vcwallptr(previous_wall);
	if (w.segnum == segment_none)
	{
		return 0;
	}

	if (w.sidenum == side_none)
	{
		return 0;
	}

	Cursegp = imsegptridx(w.segnum);
	Curside = w.sidenum;

	return 1;
}

static int GotoNextWall() {
	shared_segment &sseg = Cursegp;
	auto &side = sseg.sides[Curside];
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptr = Walls.vcptr;

	const auto wall_count = Walls.get_count();
	if (!wall_count)
		/* No walls exist; nothing to do. */
		return 0;
	const auto current_wall = side.wall_num; // It's ok to be -1 because it will immediately become 0
	using T = typename std::underlying_type<wallnum_t>::type;
	const wallnum_t next_wall = (current_wall == wall_none || static_cast<T>(current_wall) >= wall_count)
		? wallnum_t{0}
		: static_cast<wallnum_t>(static_cast<T>(current_wall) + 1u);
	auto &w = *vcwallptr(next_wall);
	if (w.segnum == segment_none)
	{
		return 0;
	}

	if (w.sidenum == side_none)
	{
		return 0;
	}

	Cursegp = imsegptridx(w.segnum);
	Curside = w.sidenum;

	return 1;
}

template <typename I, typename P>
I wraparound_find_if(const I begin, const I start, const I end, P &&predicate)
{
	for (I iter = start;;)
	{
		++ iter;
		if (iter == end)
			iter = begin;
		if (iter == start)
			return iter;
		if (predicate(*iter))
			return iter;
	}
}

/*
 * Given a range defined by [`begin`, `end`), a starting point `start`
 * that is within that range, and a predicate `predicate`, examine each
 * element in the range (`start`, `begin`].  If `predicate(*iter)`
 * returns true, return `iter`.  Otherwise, perform the same search on
 * the range (`end`, `start`).  If traversal reaches `start` without
 * finding such an element, return `start` without calling
 * `predicate(*start)`.
 */
template <typename I, typename P>
I wraparound_backward_find_if(const I begin, const I start, const I end, P &&predicate)
{
	for (I iter = start;;)
	{
		if (iter == begin)
			iter = end;
		-- iter;
		if (iter == start)
			return iter;
		if (predicate(*iter))
			return iter;
	}
}

static int PrevWall() {
	int wall_type;
	const auto cur_wall_num = Cursegp->shared_segment::sides[Curside].wall_num;
	if (cur_wall_num == wall_none)
	{
		editor_status("Cannot assign new wall. No wall on curside.");
		return 0;
	}

	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptr = Walls.vcptr;
	auto &w = *vcwallptr(cur_wall_num);
	wall_type = w.clip_num;
	auto &WallAnims = GameSharedState.WallAnims;

	const auto b = WallAnims.begin();
	const auto s = std::next(b, wall_type);
	const auto e = std::next(b, Num_wall_anims);
	if (w.type == WALL_DOOR)
	{
		auto iter = wraparound_backward_find_if(b, s, e, predicate_find_nonblastable_wall);
		if (iter == s)
			throw std::runtime_error("Cannot find clip for door.");
		wall_type = std::distance(b, iter);
	}
	else if (w.type == WALL_BLASTABLE)
	{
		auto iter = wraparound_backward_find_if(b, s, e, predicate_find_blastable_wall);
		if (iter == s)
			throw std::runtime_error("Cannot find clip for blastable wall.");
		wall_type = std::distance(b, iter);
	}

	wall_assign_door(wall_type);

	Update_flags |= UF_WORLD_CHANGED;
	return 1;
}

static int NextWall() {
	int wall_type;
	const auto cur_wall_num = Cursegp->shared_segment::sides[Curside].wall_num;
	if (cur_wall_num == wall_none)
	{
		editor_status("Cannot assign new wall. No wall on curside.");
		return 0;
	}

	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &WallAnims = GameSharedState.WallAnims;
	auto &vcwallptr = Walls.vcptr;
	auto &w = *vcwallptr(cur_wall_num);
	wall_type = w.clip_num;

	const auto b = WallAnims.begin();
	const auto s = std::next(b, wall_type);
	const auto e = std::next(b, Num_wall_anims);
	if (w.type == WALL_DOOR)
	{
		auto iter = wraparound_find_if(b, s, e, predicate_find_nonblastable_wall);
		if (iter == s)
			throw std::runtime_error("Cannot find clip for door.");
		wall_type = std::distance(b, iter);
	}
	else if (w.type == WALL_BLASTABLE)
	{
		auto iter = wraparound_find_if(b, s, e, predicate_find_blastable_wall);
		if (iter == s)
			throw std::runtime_error("Cannot find clip for blastable wall.");
		wall_type = std::distance(b, iter);
	}

	wall_assign_door(wall_type);	

	Update_flags |= UF_WORLD_CHANGED;
	return 1;
}

}

//-------------------------------------------------------------------------
// Called from the editor... does one instance of the wall dialog box
//-------------------------------------------------------------------------
int do_wall_dialog()
{
	// Only open 1 instance of this window...
	if ( MainWindow != NULL ) return 0;

	// Close other windows.	
	close_all_windows();

	// Open a window with a quit button
	MainWindow = window_create<wall_dialog>(TMAPBOX_X + 20, TMAPBOX_Y + 20, 765 - TMAPBOX_X, 545 - TMAPBOX_Y, DF_DIALOG);
	return 1;
}

static window_event_result wall_dialog_created(wall_dialog *const wd)
{
	wd->quitButton = ui_add_gadget_button(*wd, 20, 252, 48, 40, "Done", nullptr);
	// These are the checkboxes for each door flag.
	int i = 80;
	wd->doorFlag[0] = ui_add_gadget_checkbox(*wd, 22, i, 16, 16, 0, "Locked"); i += 24;
	wd->doorFlag[1] = ui_add_gadget_checkbox(*wd, 22, i, 16, 16, 0, "Auto"); i += 24;
	wd->doorFlag[2] = ui_add_gadget_checkbox(*wd, 22, i, 16, 16, 0, "Illusion OFF"); i += 24;
	wd->keyFlag[0] = ui_add_gadget_radio(*wd, 22, i, 16, 16, 0, "NONE"); i += 24;
	wd->keyFlag[1] = ui_add_gadget_radio(*wd, 22, i, 16, 16, 0, "Blue"); i += 24;
	wd->keyFlag[2] = ui_add_gadget_radio(*wd, 22, i, 16, 16, 0, "Red");  i += 24;
	wd->keyFlag[3] = ui_add_gadget_radio(*wd, 22, i, 16, 16, 0, "Yellow"); i += 24;
	// The little box the wall will appear in.
	wd->wallViewBox = ui_add_gadget_userbox(*wd, 155, 5, 64, 64);
	// A bunch of buttons...
	i = 80;
	wd->prev_wall = ui_add_gadget_button(*wd, 155, i, 70, 22, "<< Clip", PrevWall);
	wd->next_wall = ui_add_gadget_button(*wd, 155+70, i, 70, 22, "Clip >>", NextWall);i += 25;
	wd->blastable = ui_add_gadget_button(*wd, 155, i, 140, 22, "Add Blastable", wall_add_blastable); i += 25;
	wd->door = ui_add_gadget_button(*wd, 155, i, 140, 22, "Add Door", wall_add_door );	i += 25;
	wd->illusory = ui_add_gadget_button(*wd, 155, i, 140, 22, "Add Illusory", wall_add_illusion);	i += 25;
	wd->closed_wall = ui_add_gadget_button(*wd, 155, i, 140, 22, "Add Closed Wall", wall_add_closed_wall); i+=25;
	wd->goto_prev_wall = ui_add_gadget_button(*wd, 155, i, 70, 22, "<< Prev", GotoPrevWall);
	wd->goto_next_wall = ui_add_gadget_button(*wd, 155+70, i, 70, 22, "Next >>", GotoNextWall);i += 25;
	wd->remove = ui_add_gadget_button(*wd, 155, i, 140, 22, "Remove Wall", wall_remove); i += 25;
	wd->bind_trigger = ui_add_gadget_button(*wd, 155, i, 140, 22, "Bind to Trigger", bind_wall_to_trigger); i += 25;
	wd->bind_control = ui_add_gadget_button(*wd, 155, i, 140, 22, "Bind to Control", bind_wall_to_control_center); i+=25;

	return window_event_result::handled;
}

void close_wall_window()
{
	if (MainWindow)
		ui_close_dialog(*std::exchange(MainWindow, nullptr));
}

window_event_result wall_dialog::callback_handler(const d_event &event)
{
	switch(event.type)
	{
		case EVENT_WINDOW_CREATED:
			return wall_dialog_created(this);
		case EVENT_WINDOW_CLOSE:
			MainWindow = nullptr;
			return window_event_result::ignored;
		default:
			break;
	}
	sbyte type;
	fix DeltaTime;
	fix64 Temp;
	int keypress = 0;
	window_event_result rval = window_event_result::ignored;
	
	if (event.type == EVENT_KEY_COMMAND)
		keypress = event_key_get(event);
	
	Assert(MainWindow != NULL);

	//------------------------------------------------------------
	// Call the ui code..
	//------------------------------------------------------------
	ui_button_any_drawn = 0;

	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &WallAnims = GameSharedState.WallAnims;
	auto &imwallptridx = Walls.imptridx;
	const auto &&w = imwallptridx(Cursegp->shared_segment::sides[Curside].wall_num);
	//------------------------------------------------------------
	// If we change walls, we need to reset the ui code for all
	// of the checkboxes that control the wall flags.  
	//------------------------------------------------------------
	if (old_wall_num != w)
	{
		if (w)
		{
			ui_checkbox_check(doorFlag[0].get(), w->flags & WALL_DOOR_LOCKED);
			ui_checkbox_check(doorFlag[1].get(), w->flags & WALL_DOOR_AUTO);
			ui_checkbox_check(doorFlag[2].get(), w->flags & WALL_ILLUSION_OFF);

			ui_radio_set_value(*keyFlag[0], w->keys & wall_key::none);
			ui_radio_set_value(*keyFlag[1], w->keys & wall_key::blue);
			ui_radio_set_value(*keyFlag[2], w->keys & wall_key::red);
			ui_radio_set_value(*keyFlag[3], w->keys & wall_key::gold);
		}
	}
	
	//------------------------------------------------------------
	// If any of the checkboxes that control the wallflags are set, then
	// update the corresponding wall flag.
	//------------------------------------------------------------

	if (w && w->type == WALL_DOOR)
	{
		if (GADGET_PRESSED(doorFlag[0].get()))
		{
			if (doorFlag[0]->flag == 1)
				w->flags |= WALL_DOOR_LOCKED;
			else
				w->flags &= ~WALL_DOOR_LOCKED;
			rval = window_event_result::handled;
		}
		else if (GADGET_PRESSED(doorFlag[1].get()))
		{
			if (doorFlag[1]->flag == 1)
				w->flags |= WALL_DOOR_AUTO;
			else
				w->flags &= ~WALL_DOOR_AUTO;
			rval = window_event_result::handled;
		}

		//------------------------------------------------------------
		// If any of the radio buttons that control the mode are set, then
		// update the corresponding key.
		//------------------------------------------------------------
		range_for (const int i, xrange(4u)) {
			if (GADGET_PRESSED(keyFlag[i].get()))
			{
				w->keys = static_cast<wall_key>(1 << i);
				rval = window_event_result::handled;
			}
		}
	} else {
		range_for (auto &i, partial_const_range(doorFlag, 2u))
			ui_checkbox_check(i.get(), 0);
		range_for (auto &i, keyFlag)
			ui_radio_set_value(*i, 0);
	}

	if (w && w->type == WALL_ILLUSION) {
		if (GADGET_PRESSED(doorFlag[2].get()))
		{
			if (doorFlag[2]->flag == 1)
				w->flags |= WALL_ILLUSION_OFF;
			else
				w->flags &= ~WALL_ILLUSION_OFF;
			rval = window_event_result::handled;
		}
	} else 
		for (	int i=2; i < 3; i++ ) 
			if (doorFlag[i]->flag == 1)
			{ 
				doorFlag[i]->flag = 0;		// Tells ui that this button isn't checked
				doorFlag[i]->status = 1;	// Tells ui to redraw button
			}

	//------------------------------------------------------------
	// Draw the wall in the little 64x64 box
	//------------------------------------------------------------
	if (event.type == EVENT_UI_DIALOG_DRAW)
	{
		// A simple frame time counter for animating the walls...
		Temp = timer_query();
		DeltaTime = Temp - time;

		gr_set_current_canvas(wallViewBox->canvas);
		if (w) {
			type = w->type;
			if ((type == WALL_DOOR) || (type == WALL_BLASTABLE)) {
				if (DeltaTime > ((F1_0*200)/1000)) {
					framenum++;
					time = Temp;
				}
				auto &wa = WallAnims[w->clip_num];
				if (framenum >= wa.num_frames)
					framenum=0;
				const auto frame = wa.frames[framenum];
				auto &texture = Textures[frame];
				PIGGY_PAGE_IN(texture);
				gr_ubitmap(*grd_curcanv, GameBitmaps[texture.index]);
			} else {
				if (type == WALL_OPEN)
					gr_clear_canvas(*grd_curcanv, CBLACK);
				else {
					auto &curside = Cursegp->unique_segment::sides[Curside];
					const auto tmap_num = curside.tmap_num;
					if (curside.tmap_num2 != texture2_value::None)
						gr_ubitmap(*grd_curcanv, texmerge_get_cached_bitmap(tmap_num, curside.tmap_num2));
					else	{
						auto &texture1 = Textures[get_texture_index(tmap_num)];
						PIGGY_PAGE_IN(texture1);
						gr_ubitmap(*grd_curcanv, GameBitmaps[texture1.index]);
					}
				}
			}
		} else
			gr_clear_canvas(*grd_curcanv, CGREY);
	}

	//------------------------------------------------------------
	// If anything changes in the ui system, redraw all the text that
	// identifies this wall.
	//------------------------------------------------------------
	if (event.type == EVENT_UI_DIALOG_DRAW)
	{
		if (w)	{
			ui_dprintf_at( MainWindow, 12, 6, "Wall: %hu    ", static_cast<typename std::underlying_type<wallnum_t>::type>(wallnum_t{w}));
			switch (w->type) {
				case WALL_NORMAL:
					ui_dprintf_at( MainWindow, 12, 23, " Type: Normal   " );
					break;
				case WALL_BLASTABLE:
					ui_dprintf_at( MainWindow, 12, 23, " Type: Blastable" );
					break;
				case WALL_DOOR:
					ui_dprintf_at( MainWindow, 12, 23, " Type: Door     " );
					ui_dputs_at( MainWindow, 223, 6, &WallAnims[w->clip_num].filename[0]);
					break;
				case WALL_ILLUSION:
					ui_dprintf_at( MainWindow, 12, 23, " Type: Illusion " );
					break;
				case WALL_OPEN:
					ui_dprintf_at( MainWindow, 12, 23, " Type: Open     " );
					break;
				case WALL_CLOSED:
					ui_dprintf_at( MainWindow, 12, 23, " Type: Closed   " );
					break;
				default:
					ui_dprintf_at( MainWindow, 12, 23, " Type: Unknown  " );
					break;
			}			
			if (w->type != WALL_DOOR)
					ui_dprintf_at( MainWindow, 223, 6, "            " );

			ui_dprintf_at( MainWindow, 12, 40, " Clip: %d   ", w->clip_num );
			ui_dprintf_at( MainWindow, 12, 57, " Trigger: %d  ", w->trigger );
		}	else {
			ui_dprintf_at( MainWindow, 12, 6, "Wall: none ");
			ui_dprintf_at( MainWindow, 12, 23, " Type: none ");
			ui_dprintf_at( MainWindow, 12, 40, " Clip: none   ");
			ui_dprintf_at( MainWindow, 12, 57, " Trigger: none  ");
		}
	}
	
	if (ui_button_any_drawn || old_wall_num != w)
		Update_flags |= UF_WORLD_CHANGED;
	if (GADGET_PRESSED(quitButton.get()) || keypress == KEY_ESC)
	{
		return window_event_result::close;
	}		

	old_wall_num = w;
	return rval;
}


//---------------------------------------------------------------------

// Restore all walls to original status (closed doors, repaired walls)
int wall_restore_all()
{
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &WallAnims = GameSharedState.WallAnims;
	auto &vcwallptr = Walls.vcptr;
	auto &vmwallptr = Walls.vmptr;
	range_for (const auto &&wp, vmwallptr)
	{
		auto &w = *wp;
		if (w.flags & WALL_BLASTED) {
			w.hps = WALL_HPS;
		}
		w.flags &= ~(WALL_BLASTED | WALL_DOOR_OPENED | WALL_DOOR_OPENING | WALL_EXPLODING);
	}

	auto &ActiveDoors = LevelUniqueWallSubsystemState.ActiveDoors;
	range_for (auto &&i, ActiveDoors.vmptr)
		wall_close_door_ref(Segments.vmptridx, Walls, WallAnims, i);

	for (csmusegment &&i : vmsegptr)
		for (auto &&[ss, us] : zip(i.s.sides, i.u.sides))
		{
			const auto wall_num = ss.wall_num;
			if (wall_num != wall_none)
			{
				auto &w = *vcwallptr(wall_num);
				if (w.type == WALL_BLASTABLE || w.type == WALL_DOOR)
					us.tmap_num2 = texture2_value{WallAnims[w.clip_num].frames[0]};
			}
 		}

#if defined(DXX_BUILD_DESCENT_II)
	auto &Triggers = LevelUniqueWallSubsystemState.Triggers;
	auto &vmtrgptr = Triggers.vmptr;
	range_for (const auto i, vmtrgptr)
		i->flags &= ~trigger_behavior_flags::disabled;
#endif
	Update_flags |= UF_GAME_VIEW_CHANGED;

	return 1;
}

//---------------------------------------------------------------------
//	Remove a specific side.
int wall_remove_side(const vmsegptridx_t seg, short side)
{
	const shared_segment &sseg = seg;
	const auto seg_child = sseg.children[side];
	if (IS_CHILD(seg_child) && sseg.sides[side].wall_num != wall_none)
	{
		shared_segment &csegp = *vmsegptr(seg_child);
		const auto Connectside = find_connect_side(seg, csegp);

		remove_trigger(seg, side);
		remove_trigger(csegp, Connectside);

		// Remove walls 'wall_num' and connecting side 'wall_num'
		//  from Walls array.  
		const auto wall0 = seg->shared_segment::sides[side].wall_num;
		const auto wall1 = csegp.sides[Connectside].wall_num;
		const auto lower_wallnum = (wall0 < wall1) ? wall0 : wall1;

		auto &Walls = LevelUniqueWallSubsystemState.Walls;
		auto &vcwallptr = Walls.vcptr;
		auto &vmwallptr = Walls.vmptr;
		{
			const auto linked_wall = vcwallptr(lower_wallnum)->linked_wall;
			if (linked_wall != wall_none)
				vmwallptr(linked_wall)->linked_wall = wall_none;
		}
		const wallnum_t upper_wallnum = static_cast<wallnum_t>(static_cast<unsigned>(lower_wallnum) + 1u);
		{
			const auto linked_wall = vcwallptr(upper_wallnum)->linked_wall;
			if (linked_wall != wall_none)
				vmwallptr(linked_wall)->linked_wall = wall_none;
		}

		{
			const auto num_walls = Walls.get_count();
			auto &&sr = partial_const_range(Walls, static_cast<unsigned>(lower_wallnum) + 2, num_walls);
			std::move(sr.begin(), sr.end(), partial_range(Walls, static_cast<unsigned>(lower_wallnum), num_walls - 2).begin());
			Walls.set_count(num_walls - 2);
		}

		range_for (const auto &&segp, vmsegptr)
		{
			if (segp->segnum != segment_none)
				range_for (auto &w, segp->shared_segment::sides)
					if (w.wall_num != wall_none && w.wall_num > upper_wallnum)
						w.wall_num = static_cast<wallnum_t>(static_cast<unsigned>(w.wall_num) - 2u);
		}

		// Destroy any links to the deleted wall.
		auto &Triggers = LevelUniqueWallSubsystemState.Triggers;
		auto &vmtrgptr = Triggers.vmptr;
		range_for (const auto vt, vmtrgptr)
		{
			auto &t = *vt;
			for (int l=0;l < t.num_links;l++)
				if (t.seg[l] == seg && t.side[l] == side) {
					for (int t1=0;t1 < t.num_links-1;t1++) {
						t.seg[t1] = t.seg[t1+1];
						t.side[t1] = t.side[t1+1];
					}
					t.num_links--;	
				}
		}

		// Destroy control center links as well.
		for (int l=0;l<ControlCenterTriggers.num_links;l++)
			if (ControlCenterTriggers.seg[l] == seg && ControlCenterTriggers.side[l] == side) {
				for (int t1=0;t1<ControlCenterTriggers.num_links-1;t1++) {
					ControlCenterTriggers.seg[t1] = ControlCenterTriggers.seg[t1+1];
					ControlCenterTriggers.side[t1] = ControlCenterTriggers.side[t1+1];
				}
				ControlCenterTriggers.num_links--;	
			}

		seg->shared_segment::sides[side].wall_num = wall_none;
		csegp.sides[Connectside].wall_num = wall_none;

		Update_flags |= UF_WORLD_CHANGED;
		return 1;
	}

	editor_status( "Can't remove wall.  No wall present.");
	return 0;
}

//---------------------------------------------------------------------
//	Remove a special wall.
int wall_remove()
{
	return wall_remove_side(Cursegp, Curside);
}

namespace {

//---------------------------------------------------------------------
// Add a wall to curside
static int wall_add_to_side(fvcvertptr &vcvertptr, wall_array &Walls, const vmsegptridx_t segp, const unsigned side, const unsigned type)
{
	if (add_wall(vcvertptr, Walls, segp, side)) {
		const auto &&csegp = segp.absolute_sibling(segp->children[side]);
		auto connectside = find_connect_side(segp, csegp);

		auto &vmwallptr = Walls.vmptr;
		auto &w0 = *vmwallptr(segp->shared_segment::sides[side].wall_num);
		auto &w1 = *vmwallptr(csegp->shared_segment::sides[connectside].wall_num);
		w0.segnum = segp;
		w1.segnum = csegp;

		w0.sidenum = side;
		w1.sidenum = connectside;

  		w0.flags = 0;
		w1.flags = 0;

  		w0.type = type;
		w1.type = type;

		w0.clip_num = -1;
		w1.clip_num = -1;

		w0.keys = wall_key::none;
		w1.keys = wall_key::none;

		if (type == WALL_BLASTABLE) {
	  		w0.hps = WALL_HPS;
			w1.hps = WALL_HPS;
			}	

		if (type != WALL_DOOR) {
			segp->unique_segment::sides[side].tmap_num2 = texture2_value::None;
			csegp->unique_segment::sides[connectside].tmap_num2 = texture2_value::None;
			}

		if (type == WALL_DOOR) {
	  		w0.flags |= WALL_DOOR_AUTO;
			w1.flags |= WALL_DOOR_AUTO;

			w0.clip_num = Current_door_type;
			w1.clip_num = Current_door_type;
		}

		//Update_flags |= UF_WORLD_CHANGED;
		//return 1;

//		return NextWall();		//assign a clip num
		return wall_assign_door(Current_door_type);

	} else {
		editor_status( "Cannot add wall here, no children" );
		return 0;
	}
}

}

//---------------------------------------------------------------------
// Add a wall to markedside
int wall_add_to_markedside(fvcvertptr &vcvertptr, wall_array &Walls, const int8_t type)
{
	if (add_wall(vcvertptr, Walls, Markedsegp, Markedside)) {
		const auto &&csegp = vmsegptridx(Markedsegp->children[Markedside]);
		auto Connectside = find_connect_side(Markedsegp, csegp);

		const auto wall_num = Markedsegp->shared_segment::sides[Markedside].wall_num;
		const auto cwall_num = csegp->shared_segment::sides[Connectside].wall_num;
		auto &vmwallptr = Walls.vmptr;
		auto &w0 = *vmwallptr(wall_num);
		auto &w1 = *vmwallptr(cwall_num);

		w0.segnum = Markedsegp;
		w1.segnum = csegp;

		w0.sidenum = Markedside;
		w1.sidenum = Connectside;

  		w0.flags = 0;
		w1.flags = 0;

  		w0.type = type;
		w1.type = type;

		w0.trigger = trigger_none;
		w1.trigger = trigger_none;

		w0.clip_num = -1;
		w1.clip_num = -1;

		w0.keys = wall_key::none;
		w1.keys = wall_key::none;

		if (type == WALL_BLASTABLE) {
	  		w0.hps = WALL_HPS;
			w1.hps = WALL_HPS;
			
	  		w0.clip_num = 0;
			w1.clip_num = 0;
			}	

		if (type != WALL_DOOR) {
			Markedsegp->unique_segment::sides[Markedside].tmap_num2 = texture2_value::None;
			csegp->unique_segment::sides[Connectside].tmap_num2 = texture2_value::None;
			}

		Update_flags |= UF_WORLD_CHANGED;
		return 1;
	} else {
		editor_status( "Cannot add wall here, no children" );
		return 0;
	}
}

int bind_wall_to_control_center() {

	int link_num;
	if (Cursegp->shared_segment::sides[Curside].wall_num == wall_none) {
		editor_status("No wall at Curside.");
		return 0;
	}

	link_num = ControlCenterTriggers.num_links;
	for (int i=0;i<link_num;i++)
		if (Cursegp == ControlCenterTriggers.seg[i] && Curside == ControlCenterTriggers.side[i])
		{
			editor_status("Curside already bound to Control Center.");
			return 0;
		}

	// Error checking completed, actual binding begins
	ControlCenterTriggers.seg[link_num] = Cursegp;
	ControlCenterTriggers.side[link_num] = Curside;
	ControlCenterTriggers.num_links++;

	editor_status("Wall linked to control center");

	return 1;
}

//link two doors, curseg/curside and markedseg/markedside
int wall_link_doors()
{
	const auto cwall_num = Cursegp->shared_segment::sides[Curside].wall_num;
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &imwallptr = Walls.imptr;
	const auto &&w1 = imwallptr(cwall_num);

	if (!w1 || w1->type != WALL_DOOR) {
		editor_status("Curseg/curside is not a door");
		return 0;
	}

	if (!Markedsegp) {
		editor_status("No marked side.");
		return 0;
	}
	
	const auto mwall_num = Markedsegp->shared_segment::sides[Markedside].wall_num;
	const auto &&w2 = imwallptr(mwall_num);

	if (!w2 || w2->type != WALL_DOOR) {
		editor_status("Markedseg/markedside is not a door");
		return 0;
	}

	if (w1->linked_wall != wall_none)
		editor_status("Curseg/curside is already linked");

	if (w2->linked_wall != wall_none)
		editor_status("Markedseg/markedside is already linked");

	w1->linked_wall = Markedsegp->shared_segment::sides[Markedside].wall_num;
	w2->linked_wall = Cursegp->shared_segment::sides[Curside].wall_num;

	return 1;
}

int wall_unlink_door()
{
	const auto cwall_num = Cursegp->shared_segment::sides[Curside].wall_num;
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &imwallptr = Walls.imptr;
	const auto &&w1 = imwallptr(cwall_num);

	if (!w1 || w1->type != WALL_DOOR) {
		editor_status("Curseg/curside is not a door");
		return 0;
	}

	if (w1->linked_wall == wall_none)
	{
		editor_status("Curseg/curside is not linked");
		return 0;
	}

	auto &vmwallptr = Walls.vmptr;
	auto &w2 = *vmwallptr(w1->linked_wall);
	Assert(w2.linked_wall == cwall_num);

	w2.linked_wall = wall_none;
	w1->linked_wall = wall_none;

	return 1;

}

int check_walls() 
{
	auto &RobotCenters = LevelSharedRobotcenterState.RobotCenters;
	std::array<count_wall, MAX_WALLS> CountedWalls;
	int matcen_num;

	unsigned wall_count = 0;
	range_for (const auto &&segp, vmsegptridx)
	{
		if (segp->segnum != segment_none) {
			// Check fuelcenters
			matcen_num = segp->matcen_num;
			if (matcen_num == 0)
				if (RobotCenters[0].segnum != segp) {
				 	segp->matcen_num = -1;
				}
	
			if (matcen_num > -1)
					RobotCenters[matcen_num].segnum = segp;
	
			range_for (auto &&e, enumerate(segp->shared_segment::sides))
			{
				auto &s = e.value;
				if (s.wall_num != wall_none) {
					CountedWalls[wall_count].wallnum = s.wall_num;
					CountedWalls[wall_count].segnum = segp;
					CountedWalls[wall_count].sidenum = e.idx;
					wall_count++;
				}
			}
		}
	}

	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	if (wall_count != Walls.get_count()) {
		if (ui_messagebox(-2, -2, 2, "Num_walls is bogus\nDo you wish to correct it?\n", "Yes", "No") == 1)
		{
			Walls.set_count(wall_count);
			editor_status_fmt("Num_walls set to %d\n", Walls.get_count());
		}
	}

	// Check validity of Walls array.
	auto &vmwallptr = Walls.vmptr;
	range_for (auto &cw, partial_const_range(CountedWalls, Walls.get_count()))
	{
		auto &w = *vmwallptr(cw.wallnum);
		if (w.segnum != cw.segnum || w.sidenum != cw.sidenum)
		{
			if (ui_messagebox( -2, -2, 2, "Unmatched wall detected\nDo you wish to correct it?\n", "Yes", "No") == 1)
			{
				w.segnum = cw.segnum;
				w.sidenum = cw.sidenum;
			}
		}
	}

	const auto &&used_walls = partial_const_range(Walls, wall_count);
	const auto predicate = [](const wall &w) {
		return w.trigger != trigger_none;
	};
	unsigned trigger_count = std::count_if(used_walls.begin(), used_walls.end(), predicate);

	auto &Triggers = LevelUniqueWallSubsystemState.Triggers;
	if (trigger_count != Triggers.get_count()) {
		if (ui_messagebox(-2, -2, 2, "Num_triggers is bogus\nDo you wish to correct it?\n", "Yes", "No") == 1)
		{
			Triggers.set_count(trigger_count);
			editor_status_fmt("Num_triggers set to %d\n", Triggers.get_count());
		}
	}

	return 1;

}


int delete_all_walls() 
{
	if (ui_messagebox(-2, -2, 2, "Are you sure that walls are hosed so\n badly that you want them ALL GONE!?\n", "YES!", "No") == 1)
	{
		range_for (shared_segment &segp, vmsegptr)
		{
			range_for (auto &side, segp.sides)
				side.wall_num = wall_none;
		}
		auto &Triggers = LevelUniqueWallSubsystemState.Triggers;
		auto &Walls = LevelUniqueWallSubsystemState.Walls;
		Walls.set_count(0);
		Triggers.set_count(0);

		return 1;
	}

	return 0;
}

namespace {

// ------------------------------------------------------------------------------------------------
static void copy_old_wall_data_to_new(wallnum_t owall, wallnum_t nwall)
{
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &o = *Walls.vcptr(owall);
	auto &n = *Walls.vmptr(nwall);
	n.flags = o.flags;
	n.type = o.type;
	n.clip_num = o.clip_num;
	n.keys = o.keys;
	n.hps = o.hps;
	n.state = o.state;
	n.linked_wall = wall_none;

	n.trigger = trigger_none;
	if (o.trigger != trigger_none)
	{
		editor_status("Warning: Trigger not copied in group copy.");
	}
}

}

// ------------------------------------------------------------------------------------------------
void copy_group_walls(int old_group, int new_group)
{
	group::segment_array_type_t::const_iterator bn = GroupList[new_group].segments.begin();

	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vmwallptr = Walls.vmptr;
	range_for (const auto old_seg, GroupList[old_group].segments)
	{
		const auto new_seg = *bn++;
		auto &os = vcsegptr(old_seg)->shared_segment::sides;
		auto &ns = vmsegptr(new_seg)->shared_segment::sides;
		for (int j=0; j<MAX_SIDES_PER_SEGMENT; j++) {
			if (os[j].wall_num != wall_none) {
				const auto nw = static_cast<wallnum_t>(Walls.get_count());
				ns[j].wall_num = nw;
				copy_old_wall_data_to_new(os[j].wall_num, nw);
				auto &w = *vmwallptr(nw);
				w.segnum = new_seg;
				w.sidenum = j;
				Walls.set_count(Walls.get_count() + 1);
				Assert(Walls.get_count() < MAX_WALLS);
			}
		}
	}
}

static int Validate_walls=1;

//	--------------------------------------------------------------------------------------------------------
//	This function should be in medwall.c.
//	Make sure all wall/segment connections are valid.
void check_wall_validity(void)
{
	int sidenum;

	if (!Validate_walls)
		return;

	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptr = Walls.vcptr;
	range_for (const auto &&w, vcwallptr)
	{
		segnum_t	segnum;
		segnum = w->segnum;
		sidenum = w->sidenum;

		if (vcwallptr(vcsegptr(segnum)->shared_segment::sides[sidenum].wall_num) != w) {
			if (!Validate_walls)
				return;
			Int3();		//	Error! Your mine has been invalidated!
							// Do not continue!  Do not save!
							//	Remember your last action and Contact Mike!
							//	To continue, set the variable Validate_walls to 1 by doing:
							//		/Validate_walls = 1
							//	Then do the usual /eip++;g

		}
	}

	enumerated_array<bool, MAX_WALLS, wallnum_t> wall_flags{};

	range_for (const auto &&segp, vmsegptridx)
	{
		if (segp->segnum != segment_none)
			for (int j=0; j<MAX_SIDES_PER_SEGMENT; j++) {
				// Check walls
				auto wall_num = segp->shared_segment::sides[j].wall_num;
				if (wall_num != wall_none) {
					if (wall_flags[wall_num] != 0) {
						if (!Validate_walls)
							return;
						Int3();		//	Error! Your mine has been invalidated!
										// Do not continue!  Do not save!
										//	Remember your last action and Contact Mike!
										//	To continue, set the variable Validate_walls to 1 by doing:
										//		/Validate_walls = 1
										//	Then do the usual /eip++;g
					}

					auto &w = *vcwallptr(wall_num);
					if (w.segnum != segp || w.sidenum != j)
					{
						if (!Validate_walls)
							return;
						Int3();		//	Error! Your mine has been invalidated!
										// Do not continue!  Do not save!
										//	Remember your last action and Contact Mike!
										//	To continue, set the variable Validate_walls to 1 by doing:
										//		/Validate_walls = 1
										//	Then do the usual /eip++;g
					}

					wall_flags[wall_num] = 1;
				}
			}

	}
}

