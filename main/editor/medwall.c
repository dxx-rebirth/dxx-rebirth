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
 * $Source: /cvs/cvsroot/d2x/main/editor/medwall.c,v $
 * $Revision: 1.1 $
 * $Author: btb $
 * $Date: 2004-12-19 13:54:27 $
 * 
 * Created from version 1.11 of main\wall.c
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2003/03/09 06:34:09  donut
 * change byte typedef to sbyte to avoid conflict with win32 byte which is unsigned
 *
 * Revision 1.1.1.1  1999/06/14 22:04:04  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:35:47  john
 * Version 2.0! No anonymous unions, Watcom 10.0, with no need
 * for bitmaps.tbl.
 * 
 * Revision 1.71  1995/02/01  16:30:03  yuan
 * Stabilizing triggers and matcens.
 * 
 * Revision 1.70  1995/01/28  15:28:08  yuan
 * Return proper bug description.
 * 
 * Revision 1.69  1995/01/14  19:18:07  john
 * First version of object paging.
 * 
 * Revision 1.68  1995/01/12  12:10:44  yuan
 * Added delete trigger function
 * 
 * Revision 1.67  1994/11/29  16:51:53  yuan
 * Fixed false bogus trigger info.
 * 
 * Revision 1.66  1994/11/27  23:17:29  matt
 * Made changes for new mprintf calling convention
 * 
 * Revision 1.65  1994/11/15  11:59:42  john
 * Changed timing for door to use fixed seconds instead of milliseconds.
 * 
 * Revision 1.64  1994/11/03  10:41:17  yuan
 * Made walls add whichever the previous type was.
 * 
 * Revision 1.63  1994/10/13  13:14:59  yuan
 * Fixed trigger removal bug.
 * 
 * Revision 1.62  1994/10/07  17:43:39  yuan
 * Make validate walls default to 1.
 * 
 * Revision 1.61  1994/10/03  23:40:20  mike
 * Fix hosedness in walls in group copying.
 * 
 * Revision 1.60  1994/09/29  00:20:36  matt
 * Took out reference to unused external wall type
 * 
 * Revision 1.59  1994/09/28  17:32:24  mike
 * Functions to copy walls withing groups.
 * 
 * Revision 1.58  1994/09/28  13:40:46  yuan
 * Fixed control center trigger bug.
 * 
 * Revision 1.57  1994/09/24  12:41:52  matt
 * Took out references to obsolete constants
 * 
 * Revision 1.56  1994/09/23  18:03:55  yuan
 * Finished wall checking code.
 * 
 * Revision 1.55  1994/09/22  14:35:25  matt
 * Made blastable walls work again
 * 
 * Revision 1.54  1994/09/21  16:46:07  yuan
 * Fixed bug that reset wall slot which was just deleted.
 * 
 * Revision 1.53  1994/09/20  18:31:21  yuan
 * Output right Wallnum
 * 
 * Revision 1.52  1994/09/20  18:23:24  yuan
 * Killed the BOGIFYING WALL DRAGON...
 * 
 * There was a problem with triggers being created that had bogus
 * pointers back to their segments.
 * 
 * Revision 1.51  1994/09/20  11:13:11  yuan
 * Delete all bogus walls when checking walls.
 * 
 * Revision 1.50  1994/09/19  23:31:14  yuan
 * Adding wall checking stuff.
 * 
 * Revision 1.49  1994/09/13  21:11:20  matt
 * Added wclips that use tmap1 instead of tmap2, saving lots of merging
 * 
 * Revision 1.48  1994/09/10  13:32:08  matt
 * Made exploding walls a type of blastable walls.
 * Cleaned up blastable walls, making them tmap2 bitmaps.
 * 
 * Revision 1.47  1994/09/10  09:47:47  yuan
 * Added wall checking function.
 * 
 * Revision 1.46  1994/08/26  14:14:56  yuan
 * Fixed wall clip being set to -2 bug.
 * 
 * Revision 1.45  1994/08/25  21:56:26  mike
 * IS_CHILD stuff.
 * 
 * Revision 1.44  1994/08/19  19:30:27  matt
 * Added informative message if wall is already external when making it so.
 * 
 * Revision 1.43  1994/08/17  11:13:46  matt
 * Changed way external walls work
 * 
 * Revision 1.42  1994/08/15  17:47:29  yuan
 * Added external walls
 * 
 * Revision 1.41  1994/08/05  21:18:09  matt
 * Allow two doors to be linked together
 * 
 * Revision 1.40  1994/08/02  14:18:06  mike
 * Clean up dialog boxes.
 * 
 * Revision 1.39  1994/08/01  11:04:33  yuan
 * New materialization centers.
 * 
 * Revision 1.38  1994/07/22  17:19:11  yuan
 * Working on dialog box for refuel/repair/material/control centers.
 * 
 * Revision 1.37  1994/07/20  17:35:33  yuan
 * Added new gold key.
 * 
 * Revision 1.36  1994/07/19  14:31:44  yuan
 * Fixed keys bug.
 * 
 * Revision 1.35  1994/07/18  15:58:31  yuan
 * Hopefully prevent any "Adam door bombouts"
 * 
 * Revision 1.34  1994/07/18  15:48:40  yuan
 * Made minor cosmetic change.
 * 
 * Revision 1.33  1994/07/15  16:09:22  yuan
 * Error checking
 * 
 * Revision 1.32  1994/07/14  16:47:05  yuan
 * Fixed wall dialog for selected dooranims.
 * 
 * Revision 1.31  1994/07/11  15:09:16  yuan
 * Wall anim filenames stored in wclip structure.
 * 
 * Revision 1.30  1994/07/06  10:56:01  john
 * New structures for hostages.
 * 
 * Revision 1.29  1994/07/01  16:35:54  yuan
 * Added key system
 * 
 * Revision 1.28  1994/06/21  18:50:12  john
 * Made ESC key exit dialog.
 * 
 * Revision 1.27  1994/06/20  22:29:59  yuan
 * Fixed crazy runaway trigger bug that Adam found
 * 
 * Revision 1.26  1994/06/01  15:50:25  yuan
 * Added one more door... Needs to be set by bm.c in the future.
 * 
 * Revision 1.25  1994/05/30  20:22:34  yuan
 * New triggers.
 * 
 * Revision 1.24  1994/05/27  10:34:31  yuan
 * Added new Dialog boxes for Walls and Triggers.
 * 
 * Revision 1.23  1994/05/25  18:08:45  yuan
 * Revamping walls and triggers interface.
 * Wall interface complete, but triggers are still in progress.
 * 
 * Revision 1.22  1994/05/18  18:21:56  yuan
 * Fixed delete segment and walls bug.
 * 
 * Revision 1.21  1994/05/11  18:24:29  yuan
 * Oops.. trigger not triggers..
 * 
 * Revision 1.20  1994/05/11  18:23:53  yuan
 * Fixed trigger not set to -1 bug.
 * 
 */


#ifdef RCS
static char rcsid[] = "$Id: medwall.c,v 1.1 2004-12-19 13:54:27 btb Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "editor/medwall.h"
#include "inferno.h"
#include "editor/editor.h"
#include "segment.h"
#include "error.h"
#include "gameseg.h"

#include "textures.h"
#include "screens.h"
#include "switch.h"
#include "editor/eswitch.h"

#include "texmerge.h"
#include "medrobot.h"
#include "timer.h"
#include "mono.h"
//#include "fuelcen.h"
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
static UI_WINDOW 				*MainWindow = NULL;
static UI_GADGET_USERBOX	*WallViewBox;
static UI_GADGET_BUTTON 	*QuitButton;
static UI_GADGET_CHECKBOX	*DoorFlag[4];
static UI_GADGET_RADIO		*KeyFlag[4];

static int old_wall_num;
static fix Time;
static int framenum=0;
static int Current_door_type=1;

typedef struct count_wall {
	short wallnum;
	short	segnum,sidenum;	
} count_wall;

//---------------------------------------------------------------------
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
		mprintf((0, "Trying to goto wall at bogus segnum\n"));
		return 0;
	}

	if (Walls[current_wall].sidenum == -1) {
		mprintf((0, "Trying to goto wall at bogus sidenum\n"));
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
		mprintf((0, "Trying to goto wall at bogus segnum\n"));
		return 0;
	}

	if (Walls[current_wall].sidenum == -1) {
		mprintf((0, "Trying to goto wall at bogus sidenum\n"));
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

//-------------------------------------------------------------------------
// Called from the editor... does one instance of the wall dialog box
//-------------------------------------------------------------------------
int do_wall_dialog()
{
	int i;

	// Only open 1 instance of this window...
	if ( MainWindow != NULL ) return 0;

	// Close other windows.	
	close_all_windows();

	// Open a window with a quit button
	MainWindow = ui_open_window( TMAPBOX_X+20, TMAPBOX_Y+20, 765-TMAPBOX_X, 545-TMAPBOX_Y, WIN_DIALOG );
	QuitButton = ui_add_gadget_button( MainWindow, 20, 252, 48, 40, "Done", NULL );

	// These are the checkboxes for each door flag.
	i = 80;
	DoorFlag[0] = ui_add_gadget_checkbox( MainWindow, 22, i, 16, 16, 0, "Locked" ); i += 24;
	DoorFlag[1] = ui_add_gadget_checkbox( MainWindow, 22, i, 16, 16, 0, "Auto" ); i += 24;
	DoorFlag[2] = ui_add_gadget_checkbox( MainWindow, 22, i, 16, 16, 0, "Illusion OFF" ); i += 24;

	KeyFlag[0] = ui_add_gadget_radio( MainWindow, 22, i, 16, 16, 0, "NONE" ); i += 24;
	KeyFlag[1] = ui_add_gadget_radio( MainWindow, 22, i, 16, 16, 0, "Blue" ); i += 24;
	KeyFlag[2] = ui_add_gadget_radio( MainWindow, 22, i, 16, 16, 0, "Red" );  i += 24;
	KeyFlag[3] = ui_add_gadget_radio( MainWindow, 22, i, 16, 16, 0, "Yellow" ); i += 24;

	// The little box the wall will appear in.
	WallViewBox = ui_add_gadget_userbox( MainWindow, 155, 5, 64, 64 );

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
	
	old_wall_num = -2;		// Set to some dummy value so everything works ok on the first frame.

	return 1;
}

void close_wall_window()
{
	if ( MainWindow!=NULL )	{
		ui_close_window( MainWindow );
		MainWindow = NULL;
	}
}

void do_wall_window()
{
	int i;
	sbyte type;
	fix DeltaTime, Temp;

	if ( MainWindow == NULL ) return;

	//------------------------------------------------------------
	// Call the ui code..
	//------------------------------------------------------------
	ui_button_any_drawn = 0;
	ui_window_do_gadgets(MainWindow);

	//------------------------------------------------------------
	// If we change walls, we need to reset the ui code for all
	// of the checkboxes that control the wall flags.  
	//------------------------------------------------------------
	if (old_wall_num != Cursegp->sides[Curside].wall_num) {
		for (	i=0; i < 3; i++ )	{
			DoorFlag[i]->flag = 0;		// Tells ui that this button isn't checked
			DoorFlag[i]->status = 1;	// Tells ui to redraw button
		}
		for (	i=0; i < 4; i++ )	{
			KeyFlag[i]->flag = 0;		// Tells ui that this button isn't checked
			KeyFlag[i]->status = 1;		// Tells ui to redraw button
		}

		if ( Cursegp->sides[Curside].wall_num != -1) {
			if (Walls[Cursegp->sides[Curside].wall_num].flags & WALL_DOOR_LOCKED)			
				DoorFlag[0]->flag = 1;	// Mark this button as checked
			if (Walls[Cursegp->sides[Curside].wall_num].flags & WALL_DOOR_AUTO)
				DoorFlag[1]->flag = 1;	// Mark this button as checked
			if (Walls[Cursegp->sides[Curside].wall_num].flags & WALL_ILLUSION_OFF)
				DoorFlag[2]->flag = 1;	// Mark this button as checked

			if (Walls[Cursegp->sides[Curside].wall_num].keys & KEY_NONE)
				KeyFlag[0]->flag = 1;
			if (Walls[Cursegp->sides[Curside].wall_num].keys & KEY_BLUE)
				KeyFlag[1]->flag = 1;
			if (Walls[Cursegp->sides[Curside].wall_num].keys & KEY_RED)
				KeyFlag[2]->flag = 1;
			if (Walls[Cursegp->sides[Curside].wall_num].keys & KEY_GOLD)
				KeyFlag[3]->flag = 1;
		}
	}
	
	//------------------------------------------------------------
	// If any of the checkboxes that control the wallflags are set, then
	// update the corresponding wall flag.
	//------------------------------------------------------------

	if (Walls[Cursegp->sides[Curside].wall_num].type == WALL_DOOR) {
		if ( DoorFlag[0]->flag == 1 )	
			Walls[Cursegp->sides[Curside].wall_num].flags |= WALL_DOOR_LOCKED;
		else
			Walls[Cursegp->sides[Curside].wall_num].flags &= ~WALL_DOOR_LOCKED;
		if ( DoorFlag[1]->flag == 1 )	
			Walls[Cursegp->sides[Curside].wall_num].flags |= WALL_DOOR_AUTO;
		else
			Walls[Cursegp->sides[Curside].wall_num].flags &= ~WALL_DOOR_AUTO;

		//------------------------------------------------------------
		// If any of the radio buttons that control the mode are set, then
		// update the corresponding key.
		//------------------------------------------------------------
		for (	i=0; i < 4; i++ )	{
			if ( KeyFlag[i]->flag == 1 ) {
				Walls[Cursegp->sides[Curside].wall_num].keys = 1<<i;		// Set the ai_state to the cooresponding radio button
//				mprintf((0, "1<<%d = %d\n", i, 1<<i));
			}
		}
	} else {
		for (	i=0; i < 2; i++ )	
			if (DoorFlag[i]->flag == 1) { 
				DoorFlag[i]->flag = 0;		// Tells ui that this button isn't checked
				DoorFlag[i]->status = 1;	// Tells ui to redraw button
			}
		for (	i=0; i < 4; i++ )	{
			if ( KeyFlag[i]->flag == 1 ) {
				KeyFlag[i]->flag = 0;		
				KeyFlag[i]->status = 1;		
			}
		}
	}

	if (Walls[Cursegp->sides[Curside].wall_num].type == WALL_ILLUSION) {
		if ( DoorFlag[2]->flag == 1 )	
			Walls[Cursegp->sides[Curside].wall_num].flags |= WALL_ILLUSION_OFF;
		else
			Walls[Cursegp->sides[Curside].wall_num].flags &= ~WALL_ILLUSION_OFF;
	} else 
		for (	i=2; i < 3; i++ )	
			if (DoorFlag[i]->flag == 1) { 
				DoorFlag[i]->flag = 0;		// Tells ui that this button isn't checked
				DoorFlag[i]->status = 1;	// Tells ui to redraw button
			}

	//------------------------------------------------------------
	// A simple frame time counter for animating the walls...
	//------------------------------------------------------------
	Temp = timer_get_fixed_seconds();
	DeltaTime = Temp - Time;

	//------------------------------------------------------------
	// Draw the wall in the little 64x64 box
	//------------------------------------------------------------
  	gr_set_current_canvas( WallViewBox->canvas );
	if (Cursegp->sides[Curside].wall_num != -1) {
		type = Walls[Cursegp->sides[Curside].wall_num].type;
		if ((type == WALL_DOOR) || (type == WALL_BLASTABLE)) {
			if (DeltaTime > ((F1_0*200)/1000)) {
				framenum++;
				Time = Temp;
			}
			if (framenum >= WallAnims[Walls[Cursegp->sides[Curside].wall_num].clip_num].num_frames)
				framenum=0;
			PIGGY_PAGE_IN(Textures[WallAnims[Walls[Cursegp->sides[Curside].wall_num].clip_num].frames[framenum]]);
			gr_ubitmap(0,0, &GameBitmaps[Textures[WallAnims[Walls[Cursegp->sides[Curside].wall_num].clip_num].frames[framenum]].index]);
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

	//------------------------------------------------------------
	// If anything changes in the ui system, redraw all the text that
	// identifies this wall.
	//------------------------------------------------------------
	if (ui_button_any_drawn || (old_wall_num != Cursegp->sides[Curside].wall_num) )	{
		if ( Cursegp->sides[Curside].wall_num > -1 )	{
			ui_wprintf_at( MainWindow, 12, 6, "Wall: %d    ", Cursegp->sides[Curside].wall_num);
			switch (Walls[Cursegp->sides[Curside].wall_num].type) {
				case WALL_NORMAL:
					ui_wprintf_at( MainWindow, 12, 23, " Type: Normal   " );
					break;
				case WALL_BLASTABLE:
					ui_wprintf_at( MainWindow, 12, 23, " Type: Blastable" );
					break;
				case WALL_DOOR:
					ui_wprintf_at( MainWindow, 12, 23, " Type: Door     " );
					ui_wprintf_at( MainWindow, 223, 6, "%s", WallAnims[Walls[Cursegp->sides[Curside].wall_num].clip_num].filename);
					break;
				case WALL_ILLUSION:
					ui_wprintf_at( MainWindow, 12, 23, " Type: Illusion " );
					break;
				case WALL_OPEN:
					ui_wprintf_at( MainWindow, 12, 23, " Type: Open     " );
					break;
				case WALL_CLOSED:
					ui_wprintf_at( MainWindow, 12, 23, " Type: Closed   " );
					break;
				default:
					ui_wprintf_at( MainWindow, 12, 23, " Type: Unknown  " );
					break;
			}			
			if (Walls[Cursegp->sides[Curside].wall_num].type != WALL_DOOR)
					ui_wprintf_at( MainWindow, 223, 6, "            " );

			ui_wprintf_at( MainWindow, 12, 40, " Clip: %d   ", Walls[Cursegp->sides[Curside].wall_num].clip_num );
			ui_wprintf_at( MainWindow, 12, 57, " Trigger: %d  ", Walls[Cursegp->sides[Curside].wall_num].trigger );
		}	else {
			ui_wprintf_at( MainWindow, 12, 6, "Wall: none ");
			ui_wprintf_at( MainWindow, 12, 23, " Type: none ");
			ui_wprintf_at( MainWindow, 12, 40, " Clip: none   ");
			ui_wprintf_at( MainWindow, 12, 57, " Trigger: none  ");
		}
		Update_flags |= UF_WORLD_CHANGED;
	}
	if ( QuitButton->pressed || (last_keypress==KEY_ESC) )	{
		close_wall_window();
		return;
	}		

	old_wall_num = Cursegp->sides[Curside].wall_num;
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
		wall_close_door(i);

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
		mprintf((0,"WALL IS NOT BOGUS.\n"));
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

	mprintf((0,"BOGUS WALL DELETED!!!!\n"));

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

	mprintf((0, "seg %d:side %d linked to control center link_num %d\n",
				ControlCenterTriggers.seg[link_num], ControlCenterTriggers.side[link_num], link_num)); 

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
	int w1, w2, t, l;
	count_wall CountedWalls[MAX_WALLS];
	char Message[DIAGNOSTIC_MESSAGE_MAX];
	int matcen_num;

	wall_count = 0;
	for (seg=0;seg<=Highest_segment_index;seg++) 
		if (Segments[seg].segnum != -1) {
			// Check fuelcenters
			matcen_num = Segments[seg].matcen_num;
			if (matcen_num == 0)
				if (RobotCenters[0].segnum != seg) {
					mprintf((0,"Fixing Matcen 0\n"));
				 	Segments[seg].matcen_num = -1;
				}
	
			if (matcen_num > -1)
				if (RobotCenters[matcen_num].segnum != seg) {
					mprintf((0,"Matcen [%d] (seg %d) doesn't point back to correct segment %d\n", matcen_num, RobotCenters[matcen_num].segnum, seg));
					mprintf((0,"Fixing....\n"));
					RobotCenters[matcen_num].segnum = seg;
				}
	
			for (side=0;side<MAX_SIDES_PER_SEGMENT;side++)
				if (Segments[seg].sides[side].wall_num != -1) {
					CountedWalls[wall_count].wallnum = Segments[seg].sides[side].wall_num;
					CountedWalls[wall_count].segnum = seg;
					CountedWalls[wall_count].sidenum = side;
	
					// Check if segnum is bogus
					if (Walls[Segments[seg].sides[side].wall_num].segnum == -1) {
						mprintf((0, "Wall %d at seg:side %d:%d is BOGUS\n", Segments[seg].sides[side].wall_num, seg, side));
					}
	
					if (Walls[Segments[seg].sides[side].wall_num].type == WALL_NORMAL) {
						mprintf((0, "Wall %d at seg:side %d:%d is NORMAL (BAD)\n", Segments[seg].sides[side].wall_num, seg, side));
					}
	
					wall_count++;
				}
		}

	mprintf((0,"Wall Count = %d\n", wall_count));
	
	if (wall_count != Num_walls) {
		sprintf( Message, "Num_walls is bogus\nDo you wish to correct it?\n");
		if (MessageBox( -2, -2, 2, Message, "Yes", "No" )==1) {
			Num_walls = wall_count;
			editor_status("Num_walls set to %d\n", Num_walls);
		}
	}

	// Check validity of Walls array.
	for (w=0; w<Num_walls; w++) {
		if ((Walls[CountedWalls[w].wallnum].segnum != CountedWalls[w].segnum) ||
			(Walls[CountedWalls[w].wallnum].sidenum != CountedWalls[w].sidenum)) {
			mprintf((0,"Unmatched walls on wall_num %d\n", CountedWalls[w].wallnum));
			sprintf( Message, "Unmatched wall detected\nDo you wish to correct it?\n");
			if (MessageBox( -2, -2, 2, Message, "Yes", "No" )==1) {
				Walls[CountedWalls[w].wallnum].segnum = CountedWalls[w].segnum;
				Walls[CountedWalls[w].wallnum].sidenum = CountedWalls[w].sidenum;
			}
		}

		if (CountedWalls[w].wallnum >= Num_walls)
			mprintf((0,"wallnum %d in Segments exceeds Num_walls!\n", CountedWalls[w].wallnum));

		if (Walls[w].segnum == -1) {
			mprintf((0, "Wall[%d] is BOGUS\n", w));
			for (seg=0;seg<=Highest_segment_index;seg++) 
				for (side=0;side<MAX_SIDES_PER_SEGMENT;side++)
					if (Segments[seg].sides[side].wall_num == w) {
						mprintf((0, " BOGUS WALL found at seg:side %d:%d\n", seg, side));
					} 
		}				
	}

	trigger_count = 0;
	for (w1=0; w1<wall_count; w1++) {
		for (w2=w1+1; w2<wall_count; w2++) 
			if (CountedWalls[w1].wallnum == CountedWalls[w2].wallnum) {
				mprintf((0, "Duplicate Walls %d and %d. Wallnum=%d. ", w1, w2, CountedWalls[w1].wallnum));
				mprintf((0, "Seg1:sides1 %d:%d  ", CountedWalls[w1].segnum, CountedWalls[w1].sidenum));
				mprintf((0, "Seg2:sides2 %d:%d\n", CountedWalls[w2].segnum, CountedWalls[w2].sidenum));
			}
		if (Walls[w1].trigger != -1) trigger_count++;
	}

	if (trigger_count != Num_triggers) {
		sprintf( Message, "Num_triggers is bogus\nDo you wish to correct it?\n");
		if (MessageBox( -2, -2, 2, Message, "Yes", "No" )==1) {
			Num_triggers = trigger_count;
			editor_status("Num_triggers set to %d\n", Num_triggers);
		}
	}

	mprintf((0,"Trigger Count = %d\n", trigger_count));

	for (t=0; t<trigger_count; t++) {
		if (Triggers[t].flags & TRIGGER_MATCEN)
                 {
			if (Triggers[t].num_links < 1) 
				mprintf((0,"No valid links on Matcen Trigger %d\n", t));
			else
				for (l=0;l<Triggers[t].num_links;l++) {
					if (!Segments[Triggers[t].seg[l]].special & SEGMENT_IS_ROBOTMAKER)
						mprintf((0,"Bogus Matcen trigger detected on Trigger %d, No matcen at seg %d\n", t, Triggers[t].seg[l]));
				}
                 }

		if (Triggers[t].flags & TRIGGER_EXIT)
			if (Triggers[t].num_links != 0)
				mprintf((0,"Bogus links detected on Exit Trigger %d\n", t));

		if (Triggers[t].flags & TRIGGER_CONTROL_DOORS)
			for (l=0;l<Triggers[t].num_links;l++) {
				if (Segments[Triggers[t].seg[l]].sides[Triggers[t].side[l]].wall_num == -1) {
					mprintf((0,"Bogus Link detected on Door Control Trigger %d, link %d\n", t, l));
					mprintf((0,"Bogus Link at seg %d, side %d\n", Triggers[t].seg[l], Triggers[t].side[l]));
				}
			}
	}

	for (l=0;l<ControlCenterTriggers.num_links;l++)
		if (Segments[ControlCenterTriggers.seg[l]].sides[ControlCenterTriggers.side[l]].wall_num == -1) {
			mprintf((0,"Bogus Link detected on Control Center Trigger, link %d\n", l));
			mprintf((0,"Bogus Link at seg %d, side %d\n", Triggers[t].seg[l], Triggers[t].side[l]));
		}

	return 1;

}


int delete_all_walls() 
{
	char Message[DIAGNOSTIC_MESSAGE_MAX];
	int seg, side;

	sprintf( Message, "Are you sure that walls are hosed so\n badly that you want them ALL GONE!?\n");
	if (MessageBox( -2, -2, 2, Message, "YES!", "No" )==1) {
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
	if (MessageBox( -2, -2, 2, Message, "YES!", "No" )==1) {

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
	FILE *fp;

	fp = fopen("WALL.OUT", "wt");

	fprintf(fp, "Num_walls %d\n", Num_walls);

	for (w=0; w<Num_walls; w++) {

		fprintf(fp, "WALL #%d\n", w);
		fprintf(fp, "  seg: %d\n", Walls[w].segnum);
		fprintf(fp, "  sidenum: %d\n", Walls[w].sidenum);
	
		switch (Walls[w].type) {
			case WALL_NORMAL:
				fprintf(fp, "  type: NORMAL\n");
				break;
			case WALL_BLASTABLE:
				fprintf(fp, "  type: BLASTABLE\n");
				break;
			case WALL_DOOR:
				fprintf(fp, "  type: DOOR\n");
				break;
			case WALL_ILLUSION:
				fprintf(fp, "  type: ILLUSION\n");
				break;
			case WALL_OPEN:
				fprintf(fp, "  type: OPEN\n");
				break;
			case WALL_CLOSED:
				fprintf(fp, "  type: CLOSED\n");
				break;
			default:
				fprintf(fp, "  type: ILLEGAL!!!!! <-----------------\n");
				break;
		}
	
		fprintf(fp, "  flags:\n");

		if (Walls[w].flags & WALL_BLASTED)
			fprintf(fp, "   BLASTED\n");
		if (Walls[w].flags & WALL_DOOR_OPENED)
			fprintf(fp, "   DOOR_OPENED <----------------- BAD!!!\n");
		if (Walls[w].flags & WALL_DOOR_OPENING)
			fprintf(fp, "   DOOR_OPENING <---------------- BAD!!!\n");
		if (Walls[w].flags & WALL_DOOR_LOCKED)
			fprintf(fp, "   DOOR_LOCKED\n");
		if (Walls[w].flags & WALL_DOOR_AUTO)
			fprintf(fp, "   DOOR_AUTO\n");
		if (Walls[w].flags & WALL_ILLUSION_OFF)
			fprintf(fp, "   ILLUSION_OFF <---------------- OUTDATED\n");
		//if (Walls[w].flags & WALL_FUELCEN)
		//	fprintf(fp, "   FUELCEN <--------------------- OUTDATED\n");

		fprintf(fp, "  trigger: %d\n", Walls[w].trigger);
		fprintf(fp, "  clip_num: %d\n", Walls[w].clip_num);

		switch (Walls[w].keys) {
			case KEY_NONE:
				fprintf(fp, "	key: NONE\n");
				break;
			case KEY_BLUE:
				fprintf(fp, "	key: BLUE\n");
				break;
			case KEY_RED:
				fprintf(fp, "	key: RED\n");
				break;
			case KEY_GOLD:
				fprintf(fp, "	key: NONE\n");
				break;
			default:
				fprintf(fp, "  key: ILLEGAL!!!!!! <-----------------\n");
				break;
		}

		fprintf(fp, "  linked_wall %d\n", Walls[w].linked_wall);
	}
	
	fclose(fp);
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
				mprintf((0, "Going to add wall to seg:side = %i:%i\n", new_seg, j));
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

