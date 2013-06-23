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
 * Created from version 1.11 of main\wall.c
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
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

int wall_add_door_flag(sbyte flag);
int wall_add_to_side(segment *segp, int side, sbyte type);
int wall_remove_door_flag(sbyte flag);
//-------------------------------------------------------------------------
// Variables for this module...
//-------------------------------------------------------------------------
static UI_DIALOG 				*MainWindow = NULL;

typedef struct wall_dialog
{
	UI_GADGET_USERBOX	*wallViewBox;
	UI_GADGET_BUTTON 	*quitButton;
	UI_GADGET_CHECKBOX	*doorFlag[4];
	UI_GADGET_RADIO		*keyFlag[4];
	int old_wall_num;
	fix64 time;
	int framenum;
} wall_dialog;

static int Current_door_type=1;

typedef struct count_wall {
	short wallnum;
	short	segnum,sidenum;	
} count_wall;

//---------------------------------------------------------------------
extern void create_removable_wall(segment *sp, int sidenum, int tmap_num);

// Add a wall (removable 2 sided)
int add_wall(segment *seg, short side)
{
	int Connectside;
	segment *csegp;

	if (Num_walls < MAX_WALLS-2)
  	if (IS_CHILD(seg->children[side])) {
		if (seg->sides[side].wall_num == -1) {
 			seg->sides[side].wall_num = Num_walls;
			Num_walls++;
			}
				 
		csegp = &Segments[seg->children[side]];
		Connectside = find_connect_side(seg, csegp);

		if (csegp->sides[Connectside].wall_num == -1) {
			csegp->sides[Connectside].wall_num = Num_walls;
			Num_walls++;
			}
		
		create_removable_wall( seg, side, CurrentTexture );
		create_removable_wall( csegp, Connectside, CurrentTexture );

		return 1;
		}

	return 0;
}

int wall_assign_door(int door_type)
{
	int Connectside;
	segment *csegp;

	if (Cursegp->sides[Curside].wall_num == -1) {
		editor_status("Cannot assign door. No wall at Curside.");
		return 0;
	}

	if (Walls[Cursegp->sides[Curside].wall_num].type != WALL_DOOR  &&  Walls[Cursegp->sides[Curside].wall_num].type != WALL_BLASTABLE) {
		editor_status("Cannot assign door. No door at Curside.");
		return 0;
	}

	Current_door_type = door_type;

 	csegp = &Segments[Cursegp->children[Curside]];
 	Connectside = find_connect_side(Cursegp, csegp);
	
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
	if (Cursegp->children[Curside] == -2) {
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

int GotoPrevWall() {
	int current_wall;

	if (Cursegp->sides[Curside].wall_num < 0)
		current_wall = Num_walls;
	else
		current_wall = Cursegp->sides[Curside].wall_num;

	current_wall--;
	if (current_wall < 0) current_wall = Num_walls-1;
	if (current_wall >= Num_walls) current_wall = Num_walls-1;

	if (Walls[current_wall].segnum == -1) {
		return 0;
	}

	if (Walls[current_wall].sidenum == -1) {
		return 0;
	}

	Cursegp = &Segments[Walls[current_wall].segnum];
	Curside = Walls[current_wall].sidenum;

	return 1;
}


int GotoNextWall() {
	int current_wall;

	current_wall = Cursegp->sides[Curside].wall_num; // It's ok to be -1 because it will immediately become 0

	current_wall++;

	if (current_wall >= Num_walls) current_wall = 0;
	if (current_wall < 0) current_wall = 0;

	if (Walls[current_wall].segnum == -1) {
		return 0;
	}

	if (Walls[current_wall].sidenum == -1) {
		return 0;
	}

	Cursegp = &Segments[Walls[current_wall].segnum];
	Curside = Walls[current_wall].sidenum;	

	return 1;
}


int PrevWall() {
	int wall_type;

	if (Cursegp->sides[Curside].wall_num == -1) {
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

int NextWall() {
	int wall_type;

	if (Cursegp->sides[Curside].wall_num == -1) {
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

int wall_dialog_handler(UI_DIALOG *dlg, d_event *event, wall_dialog *wd);

//-------------------------------------------------------------------------
// Called from the editor... does one instance of the wall dialog box
//-------------------------------------------------------------------------
int do_wall_dialog()
{
	int i;
	wall_dialog *wd;

	// Only open 1 instance of this window...
	if ( MainWindow != NULL ) return 0;

	MALLOC(wd, wall_dialog, 1);
	if (!wd)
		return 0;

	wd->framenum = 0;

	// Close other windows.	
	close_all_windows();

	// Open a window with a quit button
	MainWindow = ui_create_dialog( TMAPBOX_X+20, TMAPBOX_Y+20, 765-TMAPBOX_X, 545-TMAPBOX_Y, DF_DIALOG, (int (*)(UI_DIALOG *, d_event *, void *))wall_dialog_handler, wd );
	wd->quitButton = ui_add_gadget_button( MainWindow, 20, 252, 48, 40, "Done", NULL );

	// These are the checkboxes for each door flag.
	i = 80;
	wd->doorFlag[0] = ui_add_gadget_checkbox( MainWindow, 22, i, 16, 16, 0, "Locked" ); i += 24;
	wd->doorFlag[1] = ui_add_gadget_checkbox( MainWindow, 22, i, 16, 16, 0, "Auto" ); i += 24;
	wd->doorFlag[2] = ui_add_gadget_checkbox( MainWindow, 22, i, 16, 16, 0, "Illusion OFF" ); i += 24;

	wd->keyFlag[0] = ui_add_gadget_radio( MainWindow, 22, i, 16, 16, 0, "NONE" ); i += 24;
	wd->keyFlag[1] = ui_add_gadget_radio( MainWindow, 22, i, 16, 16, 0, "Blue" ); i += 24;
	wd->keyFlag[2] = ui_add_gadget_radio( MainWindow, 22, i, 16, 16, 0, "Red" );  i += 24;
	wd->keyFlag[3] = ui_add_gadget_radio( MainWindow, 22, i, 16, 16, 0, "Yellow" ); i += 24;

	// The little box the wall will appear in.
	wd->wallViewBox = ui_add_gadget_userbox( MainWindow, 155, 5, 64, 64 );

	// A bunch of buttons...
	i = 80;
	ui_add_gadget_button( MainWindow,155,i,70, 22, "<< Clip", PrevWall );
	ui_add_gadget_button( MainWindow,155+70,i,70, 22, "Clip >>", NextWall );i += 25;		
	ui_add_gadget_button( MainWindow,155,i,140, 22, "Add Blastable", wall_add_blastable ); i += 25;
	ui_add_gadget_button( MainWindow,155,i,140, 22, "Add Door", wall_add_door  );	i += 25;
	ui_add_gadget_button( MainWindow,155,i,140, 22, "Add Illusory", wall_add_illusion);	i += 25;
	ui_add_gadget_button( MainWindow,155,i,140, 22, "Add Closed Wall", wall_add_closed_wall ); i+=25;
//	ui_add_gadget_button( MainWindow,155,i,140, 22, "Restore All Walls", wall_restore_all );	i += 25;
	ui_add_gadget_button( MainWindow,155,i,70, 22, "<< Prev", GotoPrevWall );
	ui_add_gadget_button( MainWindow,155+70,i,70, 22, "Next >>", GotoNextWall );i += 25;
	ui_add_gadget_button( MainWindow,155,i,140, 22, "Remove Wall", wall_remove ); i += 25;
	ui_add_gadget_button( MainWindow,155,i,140, 22, "Bind to Trigger", bind_wall_to_trigger ); i += 25;
	ui_add_gadget_button( MainWindow,155,i,140, 22, "Bind to Control", bind_wall_to_control_center ); i+=25;
	
	wd->old_wall_num = -2;		// Set to some dummy value so everything works ok on the first frame.

	return 1;
}

void close_wall_window()
{
	if ( MainWindow!=NULL )	{
		ui_close_dialog( MainWindow );
		MainWindow = NULL;
	}
}

int wall_dialog_handler(UI_DIALOG *dlg, d_event *event, wall_dialog *wd)
{
	int i;
	sbyte type;
	fix DeltaTime;
	fix64 Temp;
	int keypress = 0;
	int rval = 0;
	
	if (event->type == EVENT_KEY_COMMAND)
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
		if ( Cursegp->sides[Curside].wall_num != -1)
		{
			wall *w = &Walls[Cursegp->sides[Curside].wall_num];

			ui_checkbox_check(wd->doorFlag[0], w->flags & WALL_DOOR_LOCKED);
			ui_checkbox_check(wd->doorFlag[1], w->flags & WALL_DOOR_AUTO);
			ui_checkbox_check(wd->doorFlag[2], w->flags & WALL_ILLUSION_OFF);

			ui_radio_set_value(wd->keyFlag[0], w->keys & KEY_NONE);
			ui_radio_set_value(wd->keyFlag[1], w->keys & KEY_BLUE);
			ui_radio_set_value(wd->keyFlag[2], w->keys & KEY_RED);
			ui_radio_set_value(wd->keyFlag[3], w->keys & KEY_GOLD);
		}
	}
	
	//------------------------------------------------------------
	// If any of the checkboxes that control the wallflags are set, then
	// update the corresponding wall flag.
	//------------------------------------------------------------

	if (Walls[Cursegp->sides[Curside].wall_num].type == WALL_DOOR) {
		if (GADGET_PRESSED(wd->doorFlag[0]))
		{
			if ( wd->doorFlag[0]->flag == 1 )	
				Walls[Cursegp->sides[Curside].wall_num].flags |= WALL_DOOR_LOCKED;
			else
				Walls[Cursegp->sides[Curside].wall_num].flags &= ~WALL_DOOR_LOCKED;
			rval = 1;
		}
		else if (GADGET_PRESSED(wd->doorFlag[1]))
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
		for (	i=0; i < 4; i++ )	{
			if (GADGET_PRESSED(wd->keyFlag[i]))
			{
				Walls[Cursegp->sides[Curside].wall_num].keys = 1<<i;		// Set the ai_state to the cooresponding radio button
				rval = 1;
			}
		}
	} else {
		for (i = 0; i < 2; i++)
			ui_checkbox_check(wd->doorFlag[i], 0);
		for (	i=0; i < 4; i++ )
			ui_radio_set_value(wd->keyFlag[i], 0);
	}

	if (Walls[Cursegp->sides[Curside].wall_num].type == WALL_ILLUSION) {
		if (GADGET_PRESSED(wd->doorFlag[2]))
		{
			if ( wd->doorFlag[2]->flag == 1 )	
				Walls[Cursegp->sides[Curside].wall_num].flags |= WALL_ILLUSION_OFF;
			else
				Walls[Cursegp->sides[Curside].wall_num].flags &= ~WALL_ILLUSION_OFF;
			rval = 1;
		}
	} else 
		for (	i=2; i < 3; i++ )	
			if (wd->doorFlag[i]->flag == 1) { 
				wd->doorFlag[i]->flag = 0;		// Tells ui that this button isn't checked
				wd->doorFlag[i]->status = 1;	// Tells ui to redraw button
			}

	//------------------------------------------------------------
	// Draw the wall in the little 64x64 box
	//------------------------------------------------------------
	if (event->type == EVENT_UI_DIALOG_DRAW)
	{
		// A simple frame time counter for animating the walls...
		Temp = timer_query();
		DeltaTime = Temp - wd->time;

		gr_set_current_canvas( wd->wallViewBox->canvas );
		if (Cursegp->sides[Curside].wall_num != -1) {
			type = Walls[Cursegp->sides[Curside].wall_num].type;
			if ((type == WALL_DOOR) || (type == WALL_BLASTABLE)) {
				if (DeltaTime > ((F1_0*200)/1000)) {
					wd->framenum++;
					wd->time = Temp;
				}
				if (wd->framenum >= WallAnims[Walls[Cursegp->sides[Curside].wall_num].clip_num].num_frames)
					wd->framenum=0;
				PIGGY_PAGE_IN(Textures[WallAnims[Walls[Cursegp->sides[Curside].wall_num].clip_num].frames[wd->framenum]]);
				gr_ubitmap(0,0, &GameBitmaps[Textures[WallAnims[Walls[Cursegp->sides[Curside].wall_num].clip_num].frames[wd->framenum]].index]);
			} else {
				if (type == WALL_OPEN)
					gr_clear_canvas( CBLACK );
				else {
					if (Cursegp->sides[Curside].tmap_num2 > 0)
						gr_ubitmap(0,0, texmerge_get_cached_bitmap( Cursegp->sides[Curside].tmap_num, Cursegp->sides[Curside].tmap_num2));
					else	{
						PIGGY_PAGE_IN(Textures[Cursegp->sides[Curside].tmap_num]);
						gr_ubitmap(0,0, &GameBitmaps[Textures[Cursegp->sides[Curside].tmap_num].index]);
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
	if (event->type == EVENT_UI_DIALOG_DRAW)
	{
		if ( Cursegp->sides[Curside].wall_num > -1 )	{
			ui_dprintf_at( MainWindow, 12, 6, "Wall: %d    ", Cursegp->sides[Curside].wall_num);
			switch (Walls[Cursegp->sides[Curside].wall_num].type) {
				case WALL_NORMAL:
					ui_dprintf_at( MainWindow, 12, 23, " Type: Normal   " );
					break;
				case WALL_BLASTABLE:
					ui_dprintf_at( MainWindow, 12, 23, " Type: Blastable" );
					break;
				case WALL_DOOR:
					ui_dprintf_at( MainWindow, 12, 23, " Type: Door     " );
					ui_dprintf_at( MainWindow, 223, 6, "%s", WallAnims[Walls[Cursegp->sides[Curside].wall_num].clip_num].filename);
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

	if (event->type == EVENT_WINDOW_CLOSE)
	{
		d_free(wd);
		MainWindow = NULL;
		return 0;
	}

	if ( GADGET_PRESSED(wd->quitButton) || (keypress==KEY_ESC) )
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
	int i, j;
	int wall_num;

	for (i=0;i<Num_walls;i++) {
		if (Walls[i].flags & WALL_BLASTED) {
			Walls[i].hps = WALL_HPS;
			Walls[i].flags &= ~WALL_BLASTED;
		}
		if (Walls[i].flags & WALL_DOOR_OPENED)
			Walls[i].flags &= ~WALL_DOOR_OPENED;
		if (Walls[i].flags & WALL_DOOR_OPENING)
			Walls[i].flags &= ~WALL_DOOR_OPENING;
	}

	for (i=0;i<Num_open_doors;i++)
		wall_close_door_num(i);

	for (i=0;i<Num_segments;i++)
		for (j=0;j<MAX_SIDES_PER_SEGMENT;j++) {
			wall_num = Segments[i].sides[j].wall_num;
			if (wall_num != -1)
				if ((Walls[wall_num].type == WALL_BLASTABLE) ||
					 (Walls[wall_num].type == WALL_DOOR))
					Segments[i].sides[j].tmap_num2 = WallAnims[Walls[wall_num].clip_num].frames[0];
 		}

	for (i=0;i<Num_triggers;i++)
		Triggers[i].flags |= TRIGGER_ON;
	
	Update_flags |= UF_GAME_VIEW_CHANGED;

	return 1;
}


//---------------------------------------------------------------------
//	Delete a specific wall.
int wall_delete_bogus(short wall_num)
{
	int w;
	int seg, side;

	if ((Walls[wall_num].segnum != -1) && (Walls[wall_num].sidenum != -1)) {
		return 0;
	}

	// Delete bogus wall and slide all above walls down one slot
	for (w=wall_num; w<Num_walls; w++) {
		Walls[w] = Walls[w+1];
	}
		
	Num_walls--;

	for (seg=0;seg<=Highest_segment_index;seg++)
		if (Segments[seg].segnum != -1)
		for (side=0;side<MAX_SIDES_PER_SEGMENT;side++)
			if	(Segments[seg].sides[side].wall_num > wall_num)
				Segments[seg].sides[side].wall_num--;

	return 1;
}


//---------------------------------------------------------------------
//	Remove a specific side.
int wall_remove_side(segment *seg, short side)
{
	int Connectside;
	segment *csegp;
	int lower_wallnum;
	int w, s, t, l, t1;

	if (IS_CHILD(seg->children[side]) && IS_CHILD(seg->sides[side].wall_num)) {
		csegp = &Segments[seg->children[side]];
		Connectside = find_connect_side(seg, csegp);

		remove_trigger(seg, side);
		remove_trigger(csegp, Connectside);

		// Remove walls 'wall_num' and connecting side 'wall_num'
		//  from Walls array.  
	 	lower_wallnum = seg->sides[side].wall_num;
		if (csegp->sides[Connectside].wall_num < lower_wallnum)
			 lower_wallnum = csegp->sides[Connectside].wall_num;

		if (Walls[lower_wallnum].linked_wall != -1)
			Walls[Walls[lower_wallnum].linked_wall].linked_wall = -1;
		if (Walls[lower_wallnum+1].linked_wall != -1)
			Walls[Walls[lower_wallnum+1].linked_wall].linked_wall = -1;

		for (w=lower_wallnum;w<Num_walls-2;w++)
			Walls[w] = Walls[w+2];

		Num_walls -= 2;

		for (s=0;s<=Highest_segment_index;s++)
			if (Segments[s].segnum != -1)
			for (w=0;w<MAX_SIDES_PER_SEGMENT;w++)
				if	(Segments[s].sides[w].wall_num > lower_wallnum+1)
					Segments[s].sides[w].wall_num -= 2;

		// Destroy any links to the deleted wall.
		for (t=0;t<Num_triggers;t++)
			for (l=0;l<Triggers[t].num_links;l++)
				if ((Triggers[t].seg[l] == seg-Segments) && (Triggers[t].side[l] == side)) {
					for (t1=0;t1<Triggers[t].num_links-1;t1++) {
						Triggers[t].seg[t1] = Triggers[t].seg[t1+1];
						Triggers[t].side[t1] = Triggers[t].side[t1+1];
					}
					Triggers[t].num_links--;	
				}

		// Destroy control center links as well.
		for (l=0;l<ControlCenterTriggers.num_links;l++)
			if ((ControlCenterTriggers.seg[l] == seg-Segments) && (ControlCenterTriggers.side[l] == side)) {
				for (t1=0;t1<ControlCenterTriggers.num_links-1;t1++) {
					ControlCenterTriggers.seg[t1] = ControlCenterTriggers.seg[t1+1];
					ControlCenterTriggers.side[t1] = ControlCenterTriggers.side[t1+1];
				}
				ControlCenterTriggers.num_links--;	
			}

		seg->sides[side].wall_num = -1;
		csegp->sides[Connectside].wall_num = -1;

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
int wall_add_to_side(segment *segp, int side, sbyte type)
{
	int connectside;
	segment *csegp;

	if (add_wall(segp, side)) {
		csegp = &Segments[segp->children[side]];
		connectside = find_connect_side(segp, csegp);

		Walls[segp->sides[side].wall_num].segnum = segp-Segments;
		Walls[csegp->sides[connectside].wall_num].segnum = csegp-Segments;

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
	int Connectside;
	segment *csegp;

	if (add_wall(Markedsegp, Markedside)) {
		int	wall_num, cwall_num;
		csegp = &Segments[Markedsegp->children[Markedside]];

		Connectside = find_connect_side(Markedsegp, csegp);

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

		Walls[wall_num].trigger = -1;
		Walls[cwall_num].trigger = -1;

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
	int Connectside;
	segment *csegp;

	if (Cursegp->sides[Curside].wall_num == -1)
		{
		editor_status("Cannot change flag. No wall at Curside.");
		return 0;
		}

	if (Walls[Cursegp->sides[Curside].wall_num].type != WALL_DOOR)
		{
		editor_status("Cannot change flag. No door at Curside.");
		return 0;
		}

 	csegp = &Segments[Cursegp->children[Curside]];
 	Connectside = find_connect_side(Cursegp, csegp);

 	Walls[Cursegp->sides[Curside].wall_num].flags |= flag;
  	Walls[csegp->sides[Connectside].wall_num].flags |= flag;

	Update_flags |= UF_ED_STATE_CHANGED;
	return 1;
}

int wall_remove_door_flag(sbyte flag)
{
	int Connectside;
	segment *csegp;

	if (Cursegp->sides[Curside].wall_num == -1)
		{
		editor_status("Cannot change flag. No wall at Curside.");
		return 0;
		}

	if (Walls[Cursegp->sides[Curside].wall_num].type != WALL_DOOR)
		{
		editor_status("Cannot change flag. No door at Curside.");
		return 0;
		}

 	csegp = &Segments[Cursegp->children[Curside]];
 	Connectside = find_connect_side(Cursegp, csegp);

 	Walls[Cursegp->sides[Curside].wall_num].flags &= ~flag;
  	Walls[csegp->sides[Connectside].wall_num].flags &= ~flag;

	Update_flags |= UF_ED_STATE_CHANGED;
	return 1;
}


int bind_wall_to_control_center() {

	int link_num;
	int i;

	if (Cursegp->sides[Curside].wall_num == -1) {
		editor_status("No wall at Curside.");
		return 0;
	}

	link_num = ControlCenterTriggers.num_links;
	for (i=0;i<link_num;i++)
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

	if (Cursegp->sides[Curside].wall_num != -1)
		w1 = &Walls[Cursegp->sides[Curside].wall_num];

	if (Markedsegp->sides[Markedside].wall_num != -1)
		w2 = &Walls[Markedsegp->sides[Markedside].wall_num];

	if (!w1 || w1->type != WALL_DOOR) {
		editor_status("Curseg/curside is not a door");
		return 0;
	}

	if (!w2 || w2->type != WALL_DOOR) {
		editor_status("Markedseg/markedside is not a door");
		return 0;
	}

	if (w1->linked_wall != -1)
		editor_status("Curseg/curside is already linked");

	if (w2->linked_wall != -1)
		editor_status("Markedseg/markedside is already linked");

	w1->linked_wall = w2-Walls;
	w2->linked_wall = w1-Walls;

	return 1;
}

int wall_unlink_door()
{
	wall *w1=NULL;

	if (Cursegp->sides[Curside].wall_num != -1)
		w1 = &Walls[Cursegp->sides[Curside].wall_num];

	if (!w1 || w1->type != WALL_DOOR) {
		editor_status("Curseg/curside is not a door");
		return 0;
	}

	if (w1->linked_wall == -1)
		editor_status("Curseg/curside is not linked");

	Assert(Walls[w1->linked_wall].linked_wall == w1-Walls);

	Walls[w1->linked_wall].linked_wall = -1;
	w1->linked_wall = -1;

	return 1;

}

#define	DIAGNOSTIC_MESSAGE_MAX				150

int check_walls() 
{
	int w, seg, side, wall_count, trigger_count;
	int w1;
	count_wall CountedWalls[MAX_WALLS];
	char Message[DIAGNOSTIC_MESSAGE_MAX];
	int matcen_num;

	wall_count = 0;
	for (seg=0;seg<=Highest_segment_index;seg++) 
		if (Segments[seg].segnum != -1) {
			// Check fuelcenters
			matcen_num = Segment2s[seg].matcen_num;
			if (matcen_num == 0)
				if (RobotCenters[0].segnum != seg) {
				 	Segment2s[seg].matcen_num = -1;
				}
	
			if (matcen_num > -1)
				if (RobotCenters[matcen_num].segnum != seg) {
					RobotCenters[matcen_num].segnum = seg;
				}
	
			for (side=0;side<MAX_SIDES_PER_SEGMENT;side++)
				if (Segments[seg].sides[side].wall_num != -1) {
					CountedWalls[wall_count].wallnum = Segments[seg].sides[side].wall_num;
					CountedWalls[wall_count].segnum = seg;
					CountedWalls[wall_count].sidenum = side;
					wall_count++;
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
	for (w=0; w<Num_walls; w++) {
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
	for (w1=0; w1<wall_count; w1++) {
		if (Walls[w1].trigger != -1) trigger_count++;
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
	int seg, side;

	sprintf( Message, "Are you sure that walls are hosed so\n badly that you want them ALL GONE!?\n");
	if (ui_messagebox( -2, -2, 2, Message, "YES!", "No" )==1) {
		for (seg=0;seg<=Highest_segment_index;seg++)
			for (side=0;side<MAX_SIDES_PER_SEGMENT;side++)
				Segments[seg].sides[side].wall_num = -1;
		Num_walls=0;
		Num_triggers=0;

		return 1;
	}

	return 0;
}

int delete_all_triggers()
{
	char Message[DIAGNOSTIC_MESSAGE_MAX];
	int w;

	sprintf( Message, "Are you sure that triggers are hosed so\n badly that you want them ALL GONE!?\n");
	if (ui_messagebox( -2, -2, 2, Message, "YES!", "No" )==1) {

		for (w=0; w<Num_walls; w++)
			Walls[w].trigger=-1;
		Num_triggers=0;

		return 1;
	}

	return 0;
}

int dump_walls_info() 
{
	int w; 
	PHYSFS_file *fp;

	fp = PHYSFSX_openWriteBuffered("WALL.OUT");

	PHYSFSX_printf(fp, "Num_walls %d\n", Num_walls);

	for (w=0; w<Num_walls; w++) {

		PHYSFSX_printf(fp, "WALL #%d\n", w);
		PHYSFSX_printf(fp, "  seg: %d\n", Walls[w].segnum);
		PHYSFSX_printf(fp, "  sidenum: %d\n", Walls[w].sidenum);
	
		switch (Walls[w].type) {
			case WALL_NORMAL:
				PHYSFSX_printf(fp, "  type: NORMAL\n");
				break;
			case WALL_BLASTABLE:
				PHYSFSX_printf(fp, "  type: BLASTABLE\n");
				break;
			case WALL_DOOR:
				PHYSFSX_printf(fp, "  type: DOOR\n");
				break;
			case WALL_ILLUSION:
				PHYSFSX_printf(fp, "  type: ILLUSION\n");
				break;
			case WALL_OPEN:
				PHYSFSX_printf(fp, "  type: OPEN\n");
				break;
			case WALL_CLOSED:
				PHYSFSX_printf(fp, "  type: CLOSED\n");
				break;
			default:
				PHYSFSX_printf(fp, "  type: ILLEGAL!!!!! <-----------------\n");
				break;
		}
	
		PHYSFSX_printf(fp, "  flags:\n");

		if (Walls[w].flags & WALL_BLASTED)
			PHYSFSX_printf(fp, "   BLASTED\n");
		if (Walls[w].flags & WALL_DOOR_OPENED)
			PHYSFSX_printf(fp, "   DOOR_OPENED <----------------- BAD!!!\n");
		if (Walls[w].flags & WALL_DOOR_OPENING)
			PHYSFSX_printf(fp, "   DOOR_OPENING <---------------- BAD!!!\n");
		if (Walls[w].flags & WALL_DOOR_LOCKED)
			PHYSFSX_printf(fp, "   DOOR_LOCKED\n");
		if (Walls[w].flags & WALL_DOOR_AUTO)
			PHYSFSX_printf(fp, "   DOOR_AUTO\n");
		if (Walls[w].flags & WALL_ILLUSION_OFF)
			PHYSFSX_printf(fp, "   ILLUSION_OFF <---------------- OUTDATED\n");
		//if (Walls[w].flags & WALL_FUELCEN)
		//	PHYSFSX_printf(fp, "   FUELCEN <--------------------- OUTDATED\n");

		PHYSFSX_printf(fp, "  trigger: %d\n", Walls[w].trigger);
		PHYSFSX_printf(fp, "  clip_num: %d\n", Walls[w].clip_num);

		switch (Walls[w].keys) {
			case KEY_NONE:
				PHYSFSX_printf(fp, "	key: NONE\n");
				break;
			case KEY_BLUE:
				PHYSFSX_printf(fp, "	key: BLUE\n");
				break;
			case KEY_RED:
				PHYSFSX_printf(fp, "	key: RED\n");
				break;
			case KEY_GOLD:
				PHYSFSX_printf(fp, "	key: NONE\n");
				break;
			default:
				PHYSFSX_printf(fp, "  key: ILLEGAL!!!!!! <-----------------\n");
				break;
		}

		PHYSFSX_printf(fp, "  linked_wall %d\n", Walls[w].linked_wall);
	}
	
	PHYSFS_close(fp);
	return 1;
}

// ------------------------------------------------------------------------------------------------
void copy_old_wall_data_to_new(int owall, int nwall)
{
	Walls[nwall].flags = Walls[owall].flags;
	Walls[nwall].type = Walls[owall].type;
	Walls[nwall].clip_num = Walls[owall].clip_num;
	Walls[nwall].keys = Walls[owall].keys;
	Walls[nwall].hps = Walls[owall].hps;
	Walls[nwall].state = Walls[owall].state;
	Walls[nwall].linked_wall = -1;

	Walls[nwall].trigger = -1;

	if (Walls[owall].trigger != -1) {
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
	int	i,j,old_seg, new_seg;

	for (i=0; i<GroupList[old_group].num_segments; i++) {
		old_seg = GroupList[old_group].segments[i];
		new_seg = GroupList[new_group].segments[i];

		for (j=0; j<MAX_SIDES_PER_SEGMENT; j++) {
			if (Segments[old_seg].sides[j].wall_num != -1) {
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
	int	i, j;
	int	segnum, sidenum, wall_num;
	sbyte	wall_flags[MAX_WALLS];

	if (!Validate_walls)
		return;

	for (i=0; i<Num_walls; i++) {
		segnum = Walls[i].segnum;
		sidenum = Walls[i].sidenum;

		if (Segments[segnum].sides[sidenum].wall_num != i) {
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

	for (i=0; i<MAX_WALLS; i++)
		wall_flags[i] = 0;

	for (i=0; i<=Highest_segment_index; i++) {
		if (Segments[i].segnum != -1)
			for (j=0; j<MAX_SIDES_PER_SEGMENT; j++) {
				// Check walls
				wall_num = Segments[i].sides[j].wall_num;
				if (wall_num != -1) {
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

