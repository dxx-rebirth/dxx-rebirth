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
#include "highest_valid.h"
#include "partial_range.h"

static int wall_add_door_flag(sbyte flag);
static int wall_add_to_side(const vsegptridx_t segp, int side, sbyte type);
static int wall_remove_door_flag(sbyte flag);

//-------------------------------------------------------------------------
// Variables for this module...
//-------------------------------------------------------------------------
static UI_DIALOG 				*MainWindow = NULL;

struct wall_dialog
{
	std::unique_ptr<UI_GADGET_USERBOX> wallViewBox;
	std::unique_ptr<UI_GADGET_BUTTON> quitButton;
	array<std::unique_ptr<UI_GADGET_CHECKBOX>, 3> doorFlag;
	array<std::unique_ptr<UI_GADGET_RADIO>, 4> keyFlag;
	int old_wall_num;
	fix64 time;
	int framenum;
};

static int wall_dialog_handler(UI_DIALOG *dlg,const d_event &event, wall_dialog *wd);

static int Current_door_type=1;

struct count_wall
{
	short wallnum;
	segnum_t	segnum;
	short sidenum;
};

//---------------------------------------------------------------------
// Add a wall (removable 2 sided)
static int add_wall(const vsegptridx_t seg, short side)
{
	if (Num_walls < MAX_WALLS-2)
  	if (IS_CHILD(seg->children[side])) {
		if (seg->sides[side].wall_num == wall_none) {
 			seg->sides[side].wall_num = Num_walls;
			Num_walls++;
			}
				 
		auto csegp = vsegptridx(seg->children[side]);
		auto Connectside = find_connect_side(seg, csegp);

		if (csegp->sides[Connectside].wall_num == wall_none) {
			csegp->sides[Connectside].wall_num = Num_walls;
			Num_walls++;
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

	if (Walls[Cursegp->sides[Curside].wall_num].type != WALL_DOOR  &&  Walls[Cursegp->sides[Curside].wall_num].type != WALL_BLASTABLE) {
		editor_status("Cannot assign door. No door at Curside.");
		return 0;
	}

	Current_door_type = door_type;

 	auto csegp = &Segments[Cursegp->children[Curside]];
	auto Connectside = find_connect_side(Cursegp, csegp);
	
 	Walls[Cursegp->sides[Curside].wall_num].clip_num = door_type;
  	Walls[csegp->sides[Connectside].wall_num].clip_num = door_type;

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

int wall_lock_door()
{
	return wall_add_door_flag(WALL_DOOR_LOCKED);
}

int wall_unlock_door()
{
	return wall_remove_door_flag(WALL_DOOR_LOCKED);
}

int wall_automate_door()
{
	return wall_add_door_flag(WALL_DOOR_AUTO);
}
	
int wall_deautomate_door()
{
	return wall_remove_door_flag(WALL_DOOR_AUTO);
}

static int GotoPrevWall() {
	int current_wall;

	if (Cursegp->sides[Curside].wall_num == wall_none)
		current_wall = Num_walls;
	else
		current_wall = Cursegp->sides[Curside].wall_num;

	current_wall--;
	if (current_wall < 0) current_wall = Num_walls-1;
	if (current_wall >= Num_walls) current_wall = Num_walls-1;

	if (Walls[current_wall].segnum == segment_none) {
		return 0;
	}

	if (Walls[current_wall].sidenum == side_none) {
		return 0;
	}

	Cursegp = &Segments[Walls[current_wall].segnum];
	Curside = Walls[current_wall].sidenum;

	return 1;
}


static int GotoNextWall() {
	int current_wall;

	current_wall = Cursegp->sides[Curside].wall_num; // It's ok to be -1 because it will immediately become 0

	current_wall++;

	if (current_wall >= Num_walls) current_wall = 0;
	if (current_wall < 0) current_wall = 0;

	if (Walls[current_wall].segnum == segment_none) {
		return 0;
	}

	if (Walls[current_wall].sidenum == side_none) {
		return 0;
	}

	Cursegp = &Segments[Walls[current_wall].segnum];
	Curside = Walls[current_wall].sidenum;	

	return 1;
}


static int PrevWall() {
	int wall_type;

	if (Cursegp->sides[Curside].wall_num == wall_none) {
		editor_status("Cannot assign new wall. No wall on curside.");
		return 0;
	}

	wall_type = Walls[Cursegp->sides[Curside].wall_num].clip_num;

	if (Walls[Cursegp->sides[Curside].wall_num].type == WALL_DOOR) {

	 	do {

			wall_type--;

			if (wall_type < 0)
				wall_type = Num_wall_anims-1;

			if (wall_type == Walls[Cursegp->sides[Curside].wall_num].clip_num)
				Error("Cannot find clip for door."); 

		} while (WallAnims[wall_type].num_frames == -1 || WallAnims[wall_type].flags & WCF_BLASTABLE);

	}

	else if (Walls[Cursegp->sides[Curside].wall_num].type == WALL_BLASTABLE) {

	 	do {

			wall_type--;

			if (wall_type < 0)
				wall_type = Num_wall_anims-1;

			if (wall_type == Walls[Cursegp->sides[Curside].wall_num].clip_num)
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

	wall_type = Walls[Cursegp->sides[Curside].wall_num].clip_num;

	if (Walls[Cursegp->sides[Curside].wall_num].type == WALL_DOOR) {

	 	do {

			wall_type++;

			if (wall_type >= Num_wall_anims) {
				wall_type = 0;
				if (Walls[Cursegp->sides[Curside].wall_num].clip_num==-1)
					Error("Cannot find clip for door."); 
			}

		} while (WallAnims[wall_type].num_frames == -1 || WallAnims[wall_type].flags & WCF_BLASTABLE);

	}
	else if (Walls[Cursegp->sides[Curside].wall_num].type == WALL_BLASTABLE) {

	 	do {

			wall_type++;

			if (wall_type >= Num_wall_anims) {
				wall_type = 0;
				if (Walls[Cursegp->sides[Curside].wall_num].clip_num==-1)
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
	ui_add_gadget_button(w, 155, i, 70, 22, "<< Clip", PrevWall);
	ui_add_gadget_button(w, 155+70, i, 70, 22, "Clip >>", NextWall);i += 25;		
	ui_add_gadget_button(w, 155, i, 140, 22, "Add Blastable", wall_add_blastable); i += 25;
	ui_add_gadget_button(w, 155, i, 140, 22, "Add Door", wall_add_door );	i += 25;
	ui_add_gadget_button(w, 155, i, 140, 22, "Add Illusory", wall_add_illusion);	i += 25;
	ui_add_gadget_button(w, 155, i, 140, 22, "Add Closed Wall", wall_add_closed_wall); i+=25;
	ui_add_gadget_button(w, 155, i, 70, 22, "<< Prev", GotoPrevWall);
	ui_add_gadget_button(w, 155+70, i, 70, 22, "Next >>", GotoNextWall);i += 25;
	ui_add_gadget_button(w, 155, i, 140, 22, "Remove Wall", wall_remove); i += 25;
	ui_add_gadget_button(w, 155, i, 140, 22, "Bind to Trigger", bind_wall_to_trigger); i += 25;
	ui_add_gadget_button(w, 155, i, 140, 22, "Bind to Control", bind_wall_to_control_center); i+=25;
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

	//------------------------------------------------------------
	// If we change walls, we need to reset the ui code for all
	// of the checkboxes that control the wall flags.  
	//------------------------------------------------------------
	if (wd->old_wall_num != Cursegp->sides[Curside].wall_num)
	{
		if ( Cursegp->sides[Curside].wall_num != wall_none)
		{
			wall *w = &Walls[Cursegp->sides[Curside].wall_num];

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

	if (Walls[Cursegp->sides[Curside].wall_num].type == WALL_DOOR) {
		if (GADGET_PRESSED(wd->doorFlag[0].get()))
		{
			if ( wd->doorFlag[0]->flag == 1 )	
				Walls[Cursegp->sides[Curside].wall_num].flags |= WALL_DOOR_LOCKED;
			else
				Walls[Cursegp->sides[Curside].wall_num].flags &= ~WALL_DOOR_LOCKED;
			rval = 1;
		}
		else if (GADGET_PRESSED(wd->doorFlag[1].get()))
		{
			if ( wd->doorFlag[1]->flag == 1 )	
				Walls[Cursegp->sides[Curside].wall_num].flags |= WALL_DOOR_AUTO;
			else
				Walls[Cursegp->sides[Curside].wall_num].flags &= ~WALL_DOOR_AUTO;
			rval = 1;
		}

		//------------------------------------------------------------
		// If any of the radio buttons that control the mode are set, then
		// update the corresponding key.
		//------------------------------------------------------------
		for (	int i=0; i < 4; i++ ) {
			if (GADGET_PRESSED(wd->keyFlag[i].get()))
			{
				Walls[Cursegp->sides[Curside].wall_num].keys = 1<<i;		// Set the ai_state to the cooresponding radio button
				rval = 1;
			}
		}
	} else {
		range_for (auto &i, partial_range(wd->doorFlag, 2u))
			ui_checkbox_check(i.get(), 0);
		range_for (auto &i, wd->keyFlag)
			ui_radio_set_value(i.get(), 0);
	}

	if (Walls[Cursegp->sides[Curside].wall_num].type == WALL_ILLUSION) {
		if (GADGET_PRESSED(wd->doorFlag[2].get()))
		{
			if ( wd->doorFlag[2]->flag == 1 )	
				Walls[Cursegp->sides[Curside].wall_num].flags |= WALL_ILLUSION_OFF;
			else
				Walls[Cursegp->sides[Curside].wall_num].flags &= ~WALL_ILLUSION_OFF;
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
			type = Walls[Cursegp->sides[Curside].wall_num].type;
			if ((type == WALL_DOOR) || (type == WALL_BLASTABLE)) {
				if (DeltaTime > ((F1_0*200)/1000)) {
					wd->framenum++;
					wd->time = Temp;
				}
				if (wd->framenum >= WallAnims[Walls[Cursegp->sides[Curside].wall_num].clip_num].num_frames)
					wd->framenum=0;
				PIGGY_PAGE_IN(Textures[WallAnims[Walls[Cursegp->sides[Curside].wall_num].clip_num].frames[wd->framenum]]);
				gr_ubitmap(GameBitmaps[Textures[WallAnims[Walls[Cursegp->sides[Curside].wall_num].clip_num].frames[wd->framenum]].index]);
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
			switch (Walls[Cursegp->sides[Curside].wall_num].type) {
				case WALL_NORMAL:
					ui_dprintf_at( MainWindow, 12, 23, " Type: Normal   " );
					break;
				case WALL_BLASTABLE:
					ui_dprintf_at( MainWindow, 12, 23, " Type: Blastable" );
					break;
				case WALL_DOOR:
					ui_dprintf_at( MainWindow, 12, 23, " Type: Door     " );
					ui_dputs_at( MainWindow, 223, 6, &WallAnims[Walls[Cursegp->sides[Curside].wall_num].clip_num].filename[0]);
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
			if (Walls[Cursegp->sides[Curside].wall_num].type != WALL_DOOR)
					ui_dprintf_at( MainWindow, 223, 6, "            " );

			ui_dprintf_at( MainWindow, 12, 40, " Clip: %d   ", Walls[Cursegp->sides[Curside].wall_num].clip_num );
			ui_dprintf_at( MainWindow, 12, 57, " Trigger: %d  ", Walls[Cursegp->sides[Curside].wall_num].trigger );
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
	range_for (auto &w, partial_range(Walls, Num_walls))
	{
		if (w.flags & WALL_BLASTED) {
			w.hps = WALL_HPS;
			w.flags &= ~WALL_BLASTED;
		}
		if (w.flags & WALL_DOOR_OPENED)
			w.flags &= ~WALL_DOOR_OPENED;
		if (w.flags & WALL_DOOR_OPENING)
			w.flags &= ~WALL_DOOR_OPENING;
	}

	for (int i=0;i<Num_open_doors;i++)
		wall_close_door_num(i);

	for (int i=0;i<Num_segments;i++)
		for (int j=0;j<MAX_SIDES_PER_SEGMENT;j++) {
			auto wall_num = Segments[i].sides[j].wall_num;
			if (wall_num != wall_none)
				if ((Walls[wall_num].type == WALL_BLASTABLE) ||
					 (Walls[wall_num].type == WALL_DOOR))
					Segments[i].sides[j].tmap_num2 = WallAnims[Walls[wall_num].clip_num].frames[0];
 		}

	range_for (auto &i, partial_range(Triggers, Num_triggers))
		i.flags |= TRIGGER_ON;
	
	Update_flags |= UF_GAME_VIEW_CHANGED;

	return 1;
}

//---------------------------------------------------------------------
//	Remove a specific side.
int wall_remove_side(const vsegptridx_t seg, short side)
{
	int lower_wallnum;
	if (IS_CHILD(seg->children[side]) && IS_CHILD(seg->sides[side].wall_num)) {
		auto csegp = &Segments[seg->children[side]];
		auto Connectside = find_connect_side(seg, csegp);

		remove_trigger(seg, side);
		remove_trigger(csegp, Connectside);

		// Remove walls 'wall_num' and connecting side 'wall_num'
		//  from Walls array.  
	 	lower_wallnum = seg->sides[side].wall_num;
		if (csegp->sides[Connectside].wall_num < lower_wallnum)
			 lower_wallnum = csegp->sides[Connectside].wall_num;

		if (Walls[lower_wallnum].linked_wall != wall_none)
			Walls[Walls[lower_wallnum].linked_wall].linked_wall = wall_none;
		if (Walls[lower_wallnum+1].linked_wall != wall_none)
			Walls[Walls[lower_wallnum+1].linked_wall].linked_wall = wall_none;

		for (int w=lower_wallnum;w<Num_walls-2;w++)
			Walls[w] = Walls[w+2];

		Num_walls -= 2;

		range_for (const auto s, highest_valid(Segments))
		{
			const auto &&segp = vsegptr(static_cast<segnum_t>(s));
			if (segp->segnum != segment_none)
			for (int w=0;w<MAX_SIDES_PER_SEGMENT;w++)
				if (segp->sides[w].wall_num > lower_wallnum+1)
					segp->sides[w].wall_num -= 2;
		}

		// Destroy any links to the deleted wall.
		range_for (auto &t, partial_range(Triggers, Num_triggers))
			for (int l=0;l < t.num_links;l++)
				if (t.seg[l] == seg && t.side[l] == side) {
					for (int t1=0;t1 < t.num_links-1;t1++) {
						t.seg[t1] = t.seg[t1+1];
						t.side[t1] = t.side[t1+1];
					}
					t.num_links--;	
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
		auto csegp = vsegptridx(segp->children[side]);
		auto connectside = find_connect_side(segp, csegp);

		Walls[segp->sides[side].wall_num].segnum = segp;
		Walls[csegp->sides[connectside].wall_num].segnum = csegp;

		Walls[segp->sides[side].wall_num].sidenum = side;
		Walls[csegp->sides[connectside].wall_num].sidenum = connectside;

  		Walls[segp->sides[side].wall_num].flags = 0;
		Walls[csegp->sides[connectside].wall_num].flags = 0;

  		Walls[segp->sides[side].wall_num].type = type;
		Walls[csegp->sides[connectside].wall_num].type = type;

//		Walls[segp->sides[side].wall_num].trigger = -1;
//		Walls[csegp->sides[connectside].wall_num].trigger = -1;

		Walls[segp->sides[side].wall_num].clip_num = -1;
		Walls[csegp->sides[connectside].wall_num].clip_num = -1;

		Walls[segp->sides[side].wall_num].keys = KEY_NONE;
		Walls[csegp->sides[connectside].wall_num].keys = KEY_NONE;

		if (type == WALL_BLASTABLE) {
	  		Walls[segp->sides[side].wall_num].hps = WALL_HPS;
			Walls[csegp->sides[connectside].wall_num].hps = WALL_HPS;
			
	  		//Walls[segp->sides[side].wall_num].clip_num = 0;
			//Walls[csegp->sides[connectside].wall_num].clip_num = 0;
			}	

		if (type != WALL_DOOR) {
			segp->sides[side].tmap_num2 = 0;
			csegp->sides[connectside].tmap_num2 = 0;
			}

		if (type == WALL_DOOR) {
	  		Walls[segp->sides[side].wall_num].flags |= WALL_DOOR_AUTO;
			Walls[csegp->sides[connectside].wall_num].flags |= WALL_DOOR_AUTO;

			Walls[segp->sides[side].wall_num].clip_num = Current_door_type;
			Walls[csegp->sides[connectside].wall_num].clip_num = Current_door_type;
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
		int	wall_num, cwall_num;
		auto csegp = &Segments[Markedsegp->children[Markedside]];
		auto Connectside = find_connect_side(Markedsegp, csegp);

		wall_num = Markedsegp->sides[Markedside].wall_num;
		cwall_num = csegp->sides[Connectside].wall_num;

		Walls[wall_num].segnum = Markedsegp-Segments;
		Walls[cwall_num].segnum = csegp-Segments;

		Walls[wall_num].sidenum = Markedside;
		Walls[cwall_num].sidenum = Connectside;

  		Walls[wall_num].flags = 0;
		Walls[cwall_num].flags = 0;

  		Walls[wall_num].type = type;
		Walls[cwall_num].type = type;

		Walls[wall_num].trigger = trigger_none;
		Walls[cwall_num].trigger = trigger_none;

		Walls[wall_num].clip_num = -1;
		Walls[cwall_num].clip_num = -1;

		Walls[wall_num].keys = KEY_NONE;
		Walls[cwall_num].keys = KEY_NONE;

		if (type == WALL_BLASTABLE) {
	  		Walls[wall_num].hps = WALL_HPS;
			Walls[cwall_num].hps = WALL_HPS;
			
	  		Walls[wall_num].clip_num = 0;
			Walls[cwall_num].clip_num = 0;
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


int wall_add_door_flag(sbyte flag)
{
	if (Cursegp->sides[Curside].wall_num == wall_none)
		{
		editor_status("Cannot change flag. No wall at Curside.");
		return 0;
		}

	if (Walls[Cursegp->sides[Curside].wall_num].type != WALL_DOOR)
		{
		editor_status("Cannot change flag. No door at Curside.");
		return 0;
		}

 	auto csegp = &Segments[Cursegp->children[Curside]];
	auto Connectside = find_connect_side(Cursegp, csegp);

 	Walls[Cursegp->sides[Curside].wall_num].flags |= flag;
  	Walls[csegp->sides[Connectside].wall_num].flags |= flag;

	Update_flags |= UF_ED_STATE_CHANGED;
	return 1;
}

int wall_remove_door_flag(sbyte flag)
{
	if (Cursegp->sides[Curside].wall_num == wall_none)
		{
		editor_status("Cannot change flag. No wall at Curside.");
		return 0;
		}

	if (Walls[Cursegp->sides[Curside].wall_num].type != WALL_DOOR)
		{
		editor_status("Cannot change flag. No door at Curside.");
		return 0;
		}

 	auto csegp = &Segments[Cursegp->children[Curside]];
	auto Connectside = find_connect_side(Cursegp, csegp);

 	Walls[Cursegp->sides[Curside].wall_num].flags &= ~flag;
  	Walls[csegp->sides[Connectside].wall_num].flags &= ~flag;

	Update_flags |= UF_ED_STATE_CHANGED;
	return 1;
}


int bind_wall_to_control_center() {

	int link_num;
	if (Cursegp->sides[Curside].wall_num == wall_none) {
		editor_status("No wall at Curside.");
		return 0;
	}

	link_num = ControlCenterTriggers.num_links;
	for (int i=0;i<link_num;i++)
		if ((Cursegp-Segments == ControlCenterTriggers.seg[i]) && (Curside == ControlCenterTriggers.side[i])) {
			editor_status("Curside already bound to Control Center.");
			return 0;
		}

	// Error checking completed, actual binding begins
	ControlCenterTriggers.seg[link_num] = Cursegp - Segments;
	ControlCenterTriggers.side[link_num] = Curside;
	ControlCenterTriggers.num_links++;

	editor_status("Wall linked to control center");

	return 1;
}

//link two doors, curseg/curside and markedseg/markedside
int wall_link_doors()
{
	wall *w1=NULL,*w2=NULL;

	if (Cursegp->sides[Curside].wall_num != wall_none)
		w1 = &Walls[Cursegp->sides[Curside].wall_num];

	if (Markedsegp->sides[Markedside].wall_num != wall_none)
		w2 = &Walls[Markedsegp->sides[Markedside].wall_num];

	if (!w1 || w1->type != WALL_DOOR) {
		editor_status("Curseg/curside is not a door");
		return 0;
	}

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
	wall *w1=NULL;

	if (Cursegp->sides[Curside].wall_num != wall_none)
		w1 = &Walls[Cursegp->sides[Curside].wall_num];

	if (!w1 || w1->type != WALL_DOOR) {
		editor_status("Curseg/curside is not a door");
		return 0;
	}

	if (w1->linked_wall == wall_none)
		editor_status("Curseg/curside is not linked");

	Assert(Walls[w1->linked_wall].linked_wall == w1-Walls);

	Walls[w1->linked_wall].linked_wall = wall_none;
	w1->linked_wall = wall_none;

	return 1;

}

#define	DIAGNOSTIC_MESSAGE_MAX				150

int check_walls() 
{
	int wall_count, trigger_count;
	count_wall CountedWalls[MAX_WALLS];
	char Message[DIAGNOSTIC_MESSAGE_MAX];
	int matcen_num;

	wall_count = 0;
	range_for (const auto seg, highest_valid(Segments))
	{
		const auto &&segp = vsegptr(static_cast<segnum_t>(seg));
		if (segp->segnum != segment_none) {
			// Check fuelcenters
			matcen_num = segp->matcen_num;
			if (matcen_num == 0)
				if (RobotCenters[0].segnum != seg) {
				 	segp->matcen_num = -1;
				}
	
			if (matcen_num > -1)
				if (RobotCenters[matcen_num].segnum != seg) {
					RobotCenters[matcen_num].segnum = seg;
				}
	
			for (int side=0;side<MAX_SIDES_PER_SEGMENT;side++)
				if (segp->sides[side].wall_num != wall_none) {
					CountedWalls[wall_count].wallnum = segp->sides[side].wall_num;
					CountedWalls[wall_count].segnum = seg;
					CountedWalls[wall_count].sidenum = side;
					wall_count++;
				}
		}
	}

	if (wall_count != Num_walls) {
		sprintf( Message, "Num_walls is bogus\nDo you wish to correct it?\n");
		if (ui_messagebox( -2, -2, 2, Message, "Yes", "No" )==1) {
			Num_walls = wall_count;
			editor_status_fmt("Num_walls set to %d\n", Num_walls);
		}
	}

	// Check validity of Walls array.
	for (int w=0; w<Num_walls; w++) {
		if ((Walls[CountedWalls[w].wallnum].segnum != CountedWalls[w].segnum) ||
			(Walls[CountedWalls[w].wallnum].sidenum != CountedWalls[w].sidenum)) {
			sprintf( Message, "Unmatched wall detected\nDo you wish to correct it?\n");
			if (ui_messagebox( -2, -2, 2, Message, "Yes", "No" )==1) {
				Walls[CountedWalls[w].wallnum].segnum = CountedWalls[w].segnum;
				Walls[CountedWalls[w].wallnum].sidenum = CountedWalls[w].sidenum;
			}
		}
	}

	trigger_count = 0;
	for (int w1=0; w1<wall_count; w1++) {
		if (Walls[w1].trigger != trigger_none) trigger_count++;
	}

	if (trigger_count != Num_triggers) {
		sprintf( Message, "Num_triggers is bogus\nDo you wish to correct it?\n");
		if (ui_messagebox( -2, -2, 2, Message, "Yes", "No" )==1) {
			Num_triggers = trigger_count;
			editor_status_fmt("Num_triggers set to %d\n", Num_triggers);
		}
	}

	return 1;

}


int delete_all_walls() 
{
	char Message[DIAGNOSTIC_MESSAGE_MAX];
	sprintf( Message, "Are you sure that walls are hosed so\n badly that you want them ALL GONE!?\n");
	if (ui_messagebox( -2, -2, 2, Message, "YES!", "No" )==1) {
		range_for (const auto seg, highest_valid(Segments))
		{
			const auto &&segp = vsegptr(static_cast<segnum_t>(seg));
			for (int side=0;side<MAX_SIDES_PER_SEGMENT;side++)
				segp->sides[side].wall_num = wall_none;
		}
		Num_walls=0;
		Num_triggers=0;

		return 1;
	}

	return 0;
}

// ------------------------------------------------------------------------------------------------
static void copy_old_wall_data_to_new(int owall, int nwall)
{
	Walls[nwall].flags = Walls[owall].flags;
	Walls[nwall].type = Walls[owall].type;
	Walls[nwall].clip_num = Walls[owall].clip_num;
	Walls[nwall].keys = Walls[owall].keys;
	Walls[nwall].hps = Walls[owall].hps;
	Walls[nwall].state = Walls[owall].state;
	Walls[nwall].linked_wall = wall_none;

	Walls[nwall].trigger = trigger_none;

	if (Walls[owall].trigger != trigger_none) {
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
				Walls[Num_walls].segnum = new_seg;
				Walls[Num_walls].sidenum = j;
				Num_walls++;
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
	sbyte	wall_flags[MAX_WALLS];

	if (!Validate_walls)
		return;

	range_for (auto &w, partial_range(Walls, Num_walls))
	{
		segnum_t	segnum;
		segnum = w.segnum;
		sidenum = w.sidenum;

		if (&Walls[Segments[segnum].sides[sidenum].wall_num] != &w) {
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

	for (int i=0; i<MAX_WALLS; i++)
		wall_flags[i] = 0;

	range_for (const auto i, highest_valid(Segments))
	{
		const auto &&segp = vsegptr(static_cast<segnum_t>(i));
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

					if ((Walls[wall_num].segnum != i) || (Walls[wall_num].sidenum != j)) {
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

