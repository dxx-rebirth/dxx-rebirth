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
#include "gameseg.h"
#include "textures.h"
#include "screens.h"
#include "switch.h"
#include "editor/eswitch.h"
#include "texmerge.h"
#include "medrobot.h"
#include "timer.h"
#include "cntrlcen.h"
#include "key.h"
#include "ehostage.h"
#include "centers.h"
#include "piggy.h"
#include "kdefs.h"
#include "u_mem.h"

#include "compiler-exchange.h"
#include "compiler-make_unique.h"
#include "compiler-range_for.h"
#include "partial_range.h"

static int wall_add_to_side(const vsegptridx_t segp, int side, sbyte type);

//-------------------------------------------------------------------------
// Variables for this module...
//-------------------------------------------------------------------------
static UI_DIALOG 				*MainWindow = NULL;

namespace {

struct wall_dialog
{
	std::unique_ptr<UI_GADGET_USERBOX> wallViewBox;
	std::unique_ptr<UI_GADGET_BUTTON> quitButton, prev_wall, next_wall, blastable, door, illusory, closed_wall, goto_prev_wall, goto_next_wall, remove, bind_trigger, bind_control;
	array<std::unique_ptr<UI_GADGET_CHECKBOX>, 3> doorFlag;
	array<std::unique_ptr<UI_GADGET_RADIO>, 4> keyFlag;
	int old_wall_num;
	fix64 time;
	int framenum;
};

static int Current_door_type=1;

struct count_wall
{
	wallnum_t wallnum;
	segnum_t	segnum;
	short sidenum;
};

}

static int wall_dialog_handler(UI_DIALOG *dlg,const d_event &event, wall_dialog *wd);

//---------------------------------------------------------------------
// Add a wall (removable 2 sided)
static int add_wall(const vsegptridx_t seg, short side)
{
	if (Num_walls < MAX_WALLS-2)
  	if (IS_CHILD(seg->children[side])) {
		if (seg->sides[side].wall_num == wall_none) {
 			seg->sides[side].wall_num = Num_walls;
			Walls.set_count(Num_walls + 1);
			}
				 
		const auto &&csegp = seg.absolute_sibling(seg->children[side]);
		auto Connectside = find_connect_side(seg, csegp);

		if (csegp->sides[Connectside].wall_num == wall_none) {
			csegp->sides[Connectside].wall_num = Num_walls;
			Walls.set_count(Num_walls + 1);
			}
		
		create_removable_wall( seg, side, CurrentTexture );
		create_removable_wall( csegp, Connectside, CurrentTexture );

		return 1;
		}

	return 0;
}

static int wall_assign_door(int door_type)
{
	if (Cursegp->sides[Curside].wall_num == wall_none) {
		editor_status("Cannot assign door. No wall at Curside.");
		return 0;
	}

	auto &wall0 = *vwallptr(Cursegp->sides[Curside].wall_num);
	if (wall0.type != WALL_DOOR && wall0.type != WALL_BLASTABLE)
	{
		editor_status("Cannot assign door. No door at Curside.");
		return 0;
	}

	Current_door_type = door_type;

	const auto &&csegp = vsegptr(Cursegp->children[Curside]);
	auto Connectside = find_connect_side(Cursegp, csegp);
	
 	wall0.clip_num = door_type;
	vwallptr(csegp->sides[Connectside].wall_num)->clip_num = door_type;

	if (WallAnims[door_type].flags & WCF_TMAP1) {
		Cursegp->sides[Curside].tmap_num = WallAnims[door_type].frames[0];
		csegp->sides[Connectside].tmap_num = WallAnims[door_type].frames[0];
		Cursegp->sides[Curside].tmap_num2 = 0;
		csegp->sides[Connectside].tmap_num2 = 0;
	}
	else {
		Cursegp->sides[Curside].tmap_num2 = WallAnims[door_type].frames[0];
		csegp->sides[Connectside].tmap_num2 = WallAnims[door_type].frames[0];
	}

	Update_flags |= UF_WORLD_CHANGED;
	return 1;
}

int wall_add_blastable()
{
	return wall_add_to_side(Cursegp, Curside, WALL_BLASTABLE);
}

int wall_add_door()
{
	return wall_add_to_side(Cursegp, Curside, WALL_DOOR);
}

int wall_add_closed_wall()
{
	return wall_add_to_side(Cursegp, Curside, WALL_CLOSED);
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
	return wall_add_to_side(Cursegp, Curside, WALL_ILLUSION);
}

static int GotoPrevWall() {
	wallnum_t current_wall;

	if (Cursegp->sides[Curside].wall_num == wall_none)
		current_wall = Num_walls;
	else
		current_wall = Cursegp->sides[Curside].wall_num;

	current_wall--;
	if (current_wall >= Num_walls) current_wall = Num_walls-1;

	auto &w = *vcwallptr(current_wall);
	if (w.segnum == segment_none)
	{
		return 0;
	}

	if (w.sidenum == side_none)
	{
		return 0;
	}

	Cursegp = segptridx(w.segnum);
	Curside = w.sidenum;

	return 1;
}


static int GotoNextWall() {
	auto current_wall = Cursegp->sides[Curside].wall_num; // It's ok to be -1 because it will immediately become 0

	current_wall++;

	if (current_wall >= Num_walls) current_wall = 0;

	auto &w = *vcwallptr(current_wall);
	if (w.segnum == segment_none)
	{
		return 0;
	}

	if (w.sidenum == side_none)
	{
		return 0;
	}

	Cursegp = segptridx(w.segnum);
	Curside = w.sidenum;

	return 1;
}


static int PrevWall() {
	int wall_type;

	if (Cursegp->sides[Curside].wall_num == wall_none) {
		editor_status("Cannot assign new wall. No wall on curside.");
		return 0;
	}

	auto &w = *vcwallptr(Cursegp->sides[Curside].wall_num);
	wall_type = w.clip_num;

	if (w.type == WALL_DOOR)
	{
	 	do {
			wall_type--;
			if (wall_type < 0)
				wall_type = Num_wall_anims-1;
			if (wall_type == w.clip_num)
				Error("Cannot find clip for door."); 
		} while (WallAnims[wall_type].num_frames == -1 || WallAnims[wall_type].flags & WCF_BLASTABLE);
	}
	else if (w.type == WALL_BLASTABLE)
	{
	 	do {
			wall_type--;
			if (wall_type < 0)
				wall_type = Num_wall_anims-1;
			if (wall_type == w.clip_num)
				Error("Cannot find clip for blastable wall."); 
		} while (WallAnims[wall_type].num_frames == -1 || !(WallAnims[wall_type].flags & WCF_BLASTABLE));
	}

	wall_assign_door(wall_type);

	Update_flags |= UF_WORLD_CHANGED;
	return 1;
}

static int NextWall() {
	int wall_type;

	if (Cursegp->sides[Curside].wall_num == wall_none) {
		editor_status("Cannot assign new wall. No wall on curside.");
		return 0;
	}

	auto &w = *vcwallptr(Cursegp->sides[Curside].wall_num);
	wall_type = w.clip_num;

	if (w.type == WALL_DOOR)
	{
	 	do {
			wall_type++;
			if (wall_type >= Num_wall_anims) {
				wall_type = 0;
				if (w.clip_num==-1)
					Error("Cannot find clip for door."); 
			}
		} while (WallAnims[wall_type].num_frames == -1 || WallAnims[wall_type].flags & WCF_BLASTABLE);
	}
	else if (w.type == WALL_BLASTABLE)
	{
	 	do {
			wall_type++;
			if (wall_type >= Num_wall_anims) {
				wall_type = 0;
				if (w.clip_num==-1)
					Error("Cannot find clip for blastable wall."); 
			}
		} while (WallAnims[wall_type].num_frames == -1 || !(WallAnims[wall_type].flags & WCF_BLASTABLE));
	}

	wall_assign_door(wall_type);	

	Update_flags |= UF_WORLD_CHANGED;
	return 1;

}

//-------------------------------------------------------------------------
// Called from the editor... does one instance of the wall dialog box
//-------------------------------------------------------------------------
int do_wall_dialog()
{
	// Only open 1 instance of this window...
	if ( MainWindow != NULL ) return 0;

	auto wd = make_unique<wall_dialog>();
	wd->framenum = 0;

	// Close other windows.	
	close_all_windows();

	// Open a window with a quit button
	MainWindow = ui_create_dialog(TMAPBOX_X+20, TMAPBOX_Y+20, 765-TMAPBOX_X, 545-TMAPBOX_Y, DF_DIALOG, wall_dialog_handler, std::move(wd));
	return 1;
}

static int wall_dialog_created(UI_DIALOG *const w, wall_dialog *const wd)
{
	wd->quitButton = ui_add_gadget_button(w, 20, 252, 48, 40, "Done", NULL);
	// These are the checkboxes for each door flag.
	int i = 80;
	wd->doorFlag[0] = ui_add_gadget_checkbox(w, 22, i, 16, 16, 0, "Locked"); i += 24;
	wd->doorFlag[1] = ui_add_gadget_checkbox(w, 22, i, 16, 16, 0, "Auto"); i += 24;
	wd->doorFlag[2] = ui_add_gadget_checkbox(w, 22, i, 16, 16, 0, "Illusion OFF"); i += 24;
	wd->keyFlag[0] = ui_add_gadget_radio(w, 22, i, 16, 16, 0, "NONE"); i += 24;
	wd->keyFlag[1] = ui_add_gadget_radio(w, 22, i, 16, 16, 0, "Blue"); i += 24;
	wd->keyFlag[2] = ui_add_gadget_radio(w, 22, i, 16, 16, 0, "Red");  i += 24;
	wd->keyFlag[3] = ui_add_gadget_radio(w, 22, i, 16, 16, 0, "Yellow"); i += 24;
	// The little box the wall will appear in.
	wd->wallViewBox = ui_add_gadget_userbox(w, 155, 5, 64, 64);
	// A bunch of buttons...
	i = 80;
	wd->prev_wall = ui_add_gadget_button(w, 155, i, 70, 22, "<< Clip", PrevWall);
	wd->next_wall = ui_add_gadget_button(w, 155+70, i, 70, 22, "Clip >>", NextWall);i += 25;
	wd->blastable = ui_add_gadget_button(w, 155, i, 140, 22, "Add Blastable", wall_add_blastable); i += 25;
	wd->door = ui_add_gadget_button(w, 155, i, 140, 22, "Add Door", wall_add_door );	i += 25;
	wd->illusory = ui_add_gadget_button(w, 155, i, 140, 22, "Add Illusory", wall_add_illusion);	i += 25;
	wd->closed_wall = ui_add_gadget_button(w, 155, i, 140, 22, "Add Closed Wall", wall_add_closed_wall); i+=25;
	wd->goto_prev_wall = ui_add_gadget_button(w, 155, i, 70, 22, "<< Prev", GotoPrevWall);
	wd->goto_next_wall = ui_add_gadget_button(w, 155+70, i, 70, 22, "Next >>", GotoNextWall);i += 25;
	wd->remove = ui_add_gadget_button(w, 155, i, 140, 22, "Remove Wall", wall_remove); i += 25;
	wd->bind_trigger = ui_add_gadget_button(w, 155, i, 140, 22, "Bind to Trigger", bind_wall_to_trigger); i += 25;
	wd->bind_control = ui_add_gadget_button(w, 155, i, 140, 22, "Bind to Control", bind_wall_to_control_center); i+=25;
	wd->old_wall_num = -2;		// Set to some dummy value so everything works ok on the first frame.
	return 1;
}

void close_wall_window()
{
	if (likely(MainWindow))
		ui_close_dialog(exchange(MainWindow, nullptr));
}

int wall_dialog_handler(UI_DIALOG *dlg,const d_event &event, wall_dialog *wd)
{
	switch(event.type)
	{
		case EVENT_WINDOW_CREATED:
			return wall_dialog_created(dlg, wd);
		case EVENT_WINDOW_CLOSE:
			std::default_delete<wall_dialog>()(wd);
			return 0;
		default:
			break;
	}
	sbyte type;
	fix DeltaTime;
	fix64 Temp;
	int keypress = 0;
	int rval = 0;
	
	if (event.type == EVENT_KEY_COMMAND)
		keypress = event_key_get(event);
	
	Assert(MainWindow != NULL);

	//------------------------------------------------------------
	// Call the ui code..
	//------------------------------------------------------------
	ui_button_any_drawn = 0;

	const auto &&w = vwallptr(Cursegp->sides[Curside].wall_num);
	//------------------------------------------------------------
	// If we change walls, we need to reset the ui code for all
	// of the checkboxes that control the wall flags.  
	//------------------------------------------------------------
	if (wd->old_wall_num != Cursegp->sides[Curside].wall_num)
	{
		if ( Cursegp->sides[Curside].wall_num != wall_none)
		{
			ui_checkbox_check(wd->doorFlag[0].get(), w->flags & WALL_DOOR_LOCKED);
			ui_checkbox_check(wd->doorFlag[1].get(), w->flags & WALL_DOOR_AUTO);
			ui_checkbox_check(wd->doorFlag[2].get(), w->flags & WALL_ILLUSION_OFF);

			ui_radio_set_value(wd->keyFlag[0].get(), w->keys & KEY_NONE);
			ui_radio_set_value(wd->keyFlag[1].get(), w->keys & KEY_BLUE);
			ui_radio_set_value(wd->keyFlag[2].get(), w->keys & KEY_RED);
			ui_radio_set_value(wd->keyFlag[3].get(), w->keys & KEY_GOLD);
		}
	}
	
	//------------------------------------------------------------
	// If any of the checkboxes that control the wallflags are set, then
	// update the corresponding wall flag.
	//------------------------------------------------------------

	if (w->type == WALL_DOOR)
	{
		if (GADGET_PRESSED(wd->doorFlag[0].get()))
		{
			if ( wd->doorFlag[0]->flag == 1 )	
				w->flags |= WALL_DOOR_LOCKED;
			else
				w->flags &= ~WALL_DOOR_LOCKED;
			rval = 1;
		}
		else if (GADGET_PRESSED(wd->doorFlag[1].get()))
		{
			if ( wd->doorFlag[1]->flag == 1 )	
				w->flags |= WALL_DOOR_AUTO;
			else
				w->flags &= ~WALL_DOOR_AUTO;
			rval = 1;
		}

		//------------------------------------------------------------
		// If any of the radio buttons that control the mode are set, then
		// update the corresponding key.
		//------------------------------------------------------------
		for (	int i=0; i < 4; i++ ) {
			if (GADGET_PRESSED(wd->keyFlag[i].get()))
			{
				w->keys = 1<<i;		// Set the ai_state to the cooresponding radio button
				rval = 1;
			}
		}
	} else {
		range_for (auto &i, partial_const_range(wd->doorFlag, 2u))
			ui_checkbox_check(i.get(), 0);
		range_for (auto &i, wd->keyFlag)
			ui_radio_set_value(i.get(), 0);
	}

	if (w->type == WALL_ILLUSION) {
		if (GADGET_PRESSED(wd->doorFlag[2].get()))
		{
			if ( wd->doorFlag[2]->flag == 1 )	
				w->flags |= WALL_ILLUSION_OFF;
			else
				w->flags &= ~WALL_ILLUSION_OFF;
			rval = 1;
		}
	} else 
		for (	int i=2; i < 3; i++ ) 
			if (wd->doorFlag[i]->flag == 1) { 
				wd->doorFlag[i]->flag = 0;		// Tells ui that this button isn't checked
				wd->doorFlag[i]->status = 1;	// Tells ui to redraw button
			}

	//------------------------------------------------------------
	// Draw the wall in the little 64x64 box
	//------------------------------------------------------------
	if (event.type == EVENT_UI_DIALOG_DRAW)
	{
		// A simple frame time counter for animating the walls...
		Temp = timer_query();
		DeltaTime = Temp - wd->time;

		gr_set_current_canvas( wd->wallViewBox->canvas );
		if (Cursegp->sides[Curside].wall_num != wall_none) {
			type = w->type;
			if ((type == WALL_DOOR) || (type == WALL_BLASTABLE)) {
				if (DeltaTime > ((F1_0*200)/1000)) {
					wd->framenum++;
					wd->time = Temp;
				}
				if (wd->framenum >= WallAnims[w->clip_num].num_frames)
					wd->framenum=0;
				PIGGY_PAGE_IN(Textures[WallAnims[w->clip_num].frames[wd->framenum]]);
				gr_ubitmap(GameBitmaps[Textures[WallAnims[w->clip_num].frames[wd->framenum]].index]);
			} else {
				if (type == WALL_OPEN)
					gr_clear_canvas( CBLACK );
				else {
					if (Cursegp->sides[Curside].tmap_num2 > 0)
						gr_ubitmap(texmerge_get_cached_bitmap( Cursegp->sides[Curside].tmap_num, Cursegp->sides[Curside].tmap_num2));
					else	{
						PIGGY_PAGE_IN(Textures[Cursegp->sides[Curside].tmap_num]);
						gr_ubitmap(GameBitmaps[Textures[Cursegp->sides[Curside].tmap_num].index]);
					}
				}
			}
		} else
			gr_clear_canvas( CGREY );
	}

	//------------------------------------------------------------
	// If anything changes in the ui system, redraw all the text that
	// identifies this wall.
	//------------------------------------------------------------
	if (event.type == EVENT_UI_DIALOG_DRAW)
	{
		if ( Cursegp->sides[Curside].wall_num != wall_none )	{
			ui_dprintf_at( MainWindow, 12, 6, "Wall: %hi    ", static_cast<int16_t>(Cursegp->sides[Curside].wall_num));
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
	
	if (ui_button_any_drawn || (wd->old_wall_num != Cursegp->sides[Curside].wall_num) )
		Update_flags |= UF_WORLD_CHANGED;
	if (GADGET_PRESSED(wd->quitButton.get()) || keypress == KEY_ESC)
	{
		close_wall_window();
		return 1;
	}		

	wd->old_wall_num = Cursegp->sides[Curside].wall_num;
	
	return rval;
}


//---------------------------------------------------------------------

// Restore all walls to original status (closed doors, repaired walls)
int wall_restore_all()
{
	range_for (const auto &&wp, vwallptr)
	{
		auto &w = *wp;
		if (w.flags & WALL_BLASTED) {
			w.hps = WALL_HPS;
		}
		w.flags &= ~(WALL_BLASTED | WALL_DOOR_OPENED | WALL_DOOR_OPENING);
	}

	for (int i=0;i<Num_open_doors;i++)
		wall_close_door_num(i);

	for (int i=0;i<Num_segments;i++)
		range_for (auto &j, Segments[i].sides)
		{
			const auto wall_num = j.wall_num;
			if (wall_num != wall_none)
			{
				auto &w = *vcwallptr(wall_num);
				if (w.type == WALL_BLASTABLE || w.type == WALL_DOOR)
					j.tmap_num2 = WallAnims[w.clip_num].frames[0];
			}
 		}

	range_for (const auto i, vtrgptr)
		i->flags |= TRIGGER_ON;
	
	Update_flags |= UF_GAME_VIEW_CHANGED;

	return 1;
}

//---------------------------------------------------------------------
//	Remove a specific side.
int wall_remove_side(const vsegptridx_t seg, short side)
{
	if (IS_CHILD(seg->children[side]) && seg->sides[side].wall_num != wall_none)
	{
		const auto &&csegp = vsegptr(seg->children[side]);
		const auto Connectside = find_connect_side(seg, csegp);

		remove_trigger(seg, side);
		remove_trigger(csegp, Connectside);

		// Remove walls 'wall_num' and connecting side 'wall_num'
		//  from Walls array.  
		const auto wall0 = seg->sides[side].wall_num;
		const auto wall1 = csegp->sides[Connectside].wall_num;
		const auto lower_wallnum = (wall0 < wall1) ? wall0 : wall1;

		{
			const auto linked_wall = vcwallptr(lower_wallnum)->linked_wall;
			if (linked_wall != wall_none)
				vwallptr(linked_wall)->linked_wall = wall_none;
		}
		{
			const wallnum_t upper_wallnum = lower_wallnum + 1;
			const auto linked_wall = vcwallptr(upper_wallnum)->linked_wall;
			if (linked_wall != wall_none)
				vwallptr(linked_wall)->linked_wall = wall_none;
		}

		for (int w=lower_wallnum;w<Num_walls-2;w++)
			Walls[w] = Walls[w+2];

		Walls.set_count(Num_walls - 2);

		range_for (const auto &&segp, vsegptr)
		{
			if (segp->segnum != segment_none)
				range_for (auto &w, segp->sides)
					if (w.wall_num > lower_wallnum+1)
						w.wall_num -= 2;
		}

		// Destroy any links to the deleted wall.
		range_for (const auto vt, vtrgptr)
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

		seg->sides[side].wall_num = wall_none;
		csegp->sides[Connectside].wall_num = wall_none;

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

//---------------------------------------------------------------------
// Add a wall to curside
static int wall_add_to_side(const vsegptridx_t segp, int side, sbyte type)
{
	if (add_wall(segp, side)) {
		const auto &&csegp = segp.absolute_sibling(segp->children[side]);
		auto connectside = find_connect_side(segp, csegp);

		auto &w0 = *vwallptr(segp->sides[side].wall_num);
		auto &w1 = *vwallptr(csegp->sides[connectside].wall_num);
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

		w0.keys = KEY_NONE;
		w1.keys = KEY_NONE;

		if (type == WALL_BLASTABLE) {
	  		w0.hps = WALL_HPS;
			w1.hps = WALL_HPS;
			}	

		if (type != WALL_DOOR) {
			segp->sides[side].tmap_num2 = 0;
			csegp->sides[connectside].tmap_num2 = 0;
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


//---------------------------------------------------------------------
// Add a wall to markedside
int wall_add_to_markedside(sbyte type)
{
	if (add_wall(Markedsegp, Markedside)) {
		const auto &&csegp = vsegptridx(Markedsegp->children[Markedside]);
		auto Connectside = find_connect_side(Markedsegp, csegp);

		const auto wall_num = Markedsegp->sides[Markedside].wall_num;
		const auto cwall_num = csegp->sides[Connectside].wall_num;
		auto &w0 = *vwallptr(wall_num);
		auto &w1 = *vwallptr(cwall_num);

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

		w0.keys = KEY_NONE;
		w1.keys = KEY_NONE;

		if (type == WALL_BLASTABLE) {
	  		w0.hps = WALL_HPS;
			w1.hps = WALL_HPS;
			
	  		w0.clip_num = 0;
			w1.clip_num = 0;
			}	

		if (type != WALL_DOOR) {
			Markedsegp->sides[Markedside].tmap_num2 = 0;
			csegp->sides[Connectside].tmap_num2 = 0;
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
	if (Cursegp->sides[Curside].wall_num == wall_none) {
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
	const auto cwall_num = Cursegp->sides[Curside].wall_num;
	const auto &&w1 = wallptr(cwall_num);

	if (!w1 || w1->type != WALL_DOOR) {
		editor_status("Curseg/curside is not a door");
		return 0;
	}

	const auto mwall_num = Markedsegp->sides[Markedside].wall_num;
	const auto &&w2 = wallptr(mwall_num);

	if (!w2 || w2->type != WALL_DOOR) {
		editor_status("Markedseg/markedside is not a door");
		return 0;
	}

	if (w1->linked_wall != wall_none)
		editor_status("Curseg/curside is already linked");

	if (w2->linked_wall != wall_none)
		editor_status("Markedseg/markedside is already linked");

	w1->linked_wall = Markedsegp->sides[Markedside].wall_num;
	w2->linked_wall = Cursegp->sides[Curside].wall_num;

	return 1;
}

int wall_unlink_door()
{
	const auto cwall_num = Cursegp->sides[Curside].wall_num;
	const auto &&w1 = wallptr(cwall_num);

	if (!w1 || w1->type != WALL_DOOR) {
		editor_status("Curseg/curside is not a door");
		return 0;
	}

	if (w1->linked_wall == wall_none)
		editor_status("Curseg/curside is not linked");

	auto &w2 = *vwallptr(w1->linked_wall);
	Assert(w2.linked_wall == cwall_num);

	w2.linked_wall = wall_none;
	w1->linked_wall = wall_none;

	return 1;

}

int check_walls() 
{
	array<count_wall, MAX_WALLS> CountedWalls;
	int matcen_num;

	unsigned wall_count = 0;
	range_for (const auto &&segp, vsegptridx)
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
	
			for (int side=0;side<MAX_SIDES_PER_SEGMENT;side++)
				if (segp->sides[side].wall_num != wall_none) {
					CountedWalls[wall_count].wallnum = segp->sides[side].wall_num;
					CountedWalls[wall_count].segnum = segp;
					CountedWalls[wall_count].sidenum = side;
					wall_count++;
				}
		}
	}

	if (wall_count != Num_walls) {
		if (ui_messagebox(-2, -2, 2, "Num_walls is bogus\nDo you wish to correct it?\n", "Yes", "No") == 1)
		{
			Walls.set_count(wall_count);
			editor_status_fmt("Num_walls set to %d\n", Num_walls);
		}
	}

	// Check validity of Walls array.
	range_for (auto &cw, partial_const_range(CountedWalls, Num_walls))
	{
		auto &w = *vwallptr(cw.wallnum);
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

	if (trigger_count != Num_triggers) {
		if (ui_messagebox(-2, -2, 2, "Num_triggers is bogus\nDo you wish to correct it?\n", "Yes", "No") == 1)
		{
			Triggers.set_count(trigger_count);
			editor_status_fmt("Num_triggers set to %d\n", Num_triggers);
		}
	}

	return 1;

}


int delete_all_walls() 
{
	if (ui_messagebox(-2, -2, 2, "Are you sure that walls are hosed so\n badly that you want them ALL GONE!?\n", "YES!", "No") == 1)
	{
		range_for (const auto &&segp, vsegptr)
		{
			range_for (auto &side, segp->sides)
				side.wall_num = wall_none;
		}
		Walls.set_count(0);
		Triggers.set_count(0);

		return 1;
	}

	return 0;
}

// ------------------------------------------------------------------------------------------------
static void copy_old_wall_data_to_new(wallnum_t owall, wallnum_t nwall)
{
	auto &o = *vcwallptr(owall);
	auto &n = *vwallptr(nwall);
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

//typedef struct trigger {
//	sbyte		type;
//	short		flags;
//	fix		value;
//	fix		time;
//	sbyte		link_num;
//	short 	num_links;
//	short 	seg[MAX_WALLS_PER_LINK];
//	short		side[MAX_WALLS_PER_LINK];
//	} trigger;


// ------------------------------------------------------------------------------------------------
void copy_group_walls(int old_group, int new_group)
{
	group::segment_array_type_t::const_iterator bo = GroupList[old_group].segments.begin();
	group::segment_array_type_t::const_iterator bn = GroupList[new_group].segments.begin();
	group::segment_array_type_t::const_iterator eo = GroupList[old_group].segments.end();
	int	old_seg, new_seg;

	for (; bo != eo; ++bo, ++bn)
	{
		old_seg = *bo;
		new_seg = *bn;

		for (int j=0; j<MAX_SIDES_PER_SEGMENT; j++) {
			if (Segments[old_seg].sides[j].wall_num != wall_none) {
				Segments[new_seg].sides[j].wall_num = Num_walls;
				copy_old_wall_data_to_new(Segments[old_seg].sides[j].wall_num, Num_walls);
				auto &w = *vwallptr(static_cast<wallnum_t>(Num_walls));
				w.segnum = new_seg;
				w.sidenum = j;
				Walls.set_count(Num_walls + 1);
				Assert(Num_walls < MAX_WALLS);
			}
		}
	}
}

int	Validate_walls=1;

//	--------------------------------------------------------------------------------------------------------
//	This function should be in medwall.c.
//	Make sure all wall/segment connections are valid.
void check_wall_validity(void)
{
	int sidenum;

	if (!Validate_walls)
		return;

	range_for (const auto &&w, vcwallptr)
	{
		segnum_t	segnum;
		segnum = w->segnum;
		sidenum = w->sidenum;

		if (vcwallptr(vcsegptr(segnum)->sides[sidenum].wall_num) != w) {
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

	array<bool, MAX_WALLS> wall_flags{};

	range_for (const auto &&segp, vsegptridx)
	{
		if (segp->segnum != segment_none)
			for (int j=0; j<MAX_SIDES_PER_SEGMENT; j++) {
				// Check walls
				auto wall_num = segp->sides[j].wall_num;
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

