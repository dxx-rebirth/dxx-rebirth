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
 * $Source: /cvs/cvsroot/d2x/main/editor/kgame.c,v $
 * $Revision: 1.1 $
 * $Author: btb $
 * $Date: 2004-12-19 13:54:27 $
 * 
 * Game Loading editor functions
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  1999/06/14 22:03:24  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:34:55  john
 * Version 2.0! No anonymous unions, Watcom 10.0, with no need
 * for bitmaps.tbl.
 * 
 * Revision 1.25  1995/02/23  10:18:05  allender
 * fixed parameter mismatch with compute_segment_center
 * 
 * Revision 1.24  1994/11/17  11:38:59  matt
 * Ripped out code to load old mines
 * 
 * Revision 1.23  1994/11/09  11:58:56  matt
 * Fixed small bug
 * 
 * Revision 1.22  1994/10/20  12:48:02  matt
 * Replaced old save files (MIN/SAV/HOT) with new LVL files
 * 
 * Revision 1.21  1994/10/15  19:08:47  mike
 * Fix bug if player object out of mine at save.
 * 
 * Revision 1.20  1994/10/13  13:15:43  matt
 * Properly relink player object when bashed for "permanant" position save
 * 
 * Revision 1.19  1994/10/11  17:07:23  matt
 * Fixed problem that sometimes caused bad player segnum after compress
 * 
 * Revision 1.18  1994/10/08  17:10:40  matt
 * Correctly set current_level_num when loading/creating mine in editor
 * 
 * Revision 1.17  1994/09/26  23:46:13  matt
 * Improved player position save code
 * 
 * Revision 1.16  1994/09/26  23:22:50  matt
 * Added functions to keep player's starting position from getting messed up
 * 
 * Revision 1.15  1994/09/14  16:50:51  yuan
 * Added load mine only function
 * 
 * Revision 1.14  1994/07/22  12:36:50  matt
 * Cleaned up editor/game interactions some more.
 * 
 * Revision 1.13  1994/07/21  17:26:26  matt
 * When new mine created, the default save filename is now reset
 * 
 * Revision 1.12  1994/06/03  12:27:05  yuan
 * Fixed restore game state.
 * 
 * 
 * Revision 1.11  1994/05/30  11:36:09  yuan
 * Do gamesave if new mine is loaded and game is entered...
 * 
 * Revision 1.10  1994/05/14  18:00:33  matt
 * Got rid of externs in source (non-header) files
 * 
 * Revision 1.9  1994/05/10  12:15:44  yuan
 * Fixed load_game functions to match prototype.
 * 
 * Revision 1.8  1994/05/06  12:52:15  yuan
 * Adding some gamesave checks...
 * 
 * Revision 1.7  1994/05/04  17:32:05  yuan
 * med_load_game changed to load_game
 * med_save_game changed to save_game
 * 
 */

#ifdef RCS
static char rcsid[] = "$Id: kgame.c,v 1.1 2004-12-19 13:54:27 btb Exp $";
#endif

#include <string.h>
#include <stdio.h>

#include "inferno.h"
#include "editor.h"
#include "ui.h"
#include "game.h"
#include "gamesave.h"
#include "gameseq.h"

char game_filename[128] = "*.LVL";

extern void checkforext( char * f, char *ext );

void checkforgamext( char * f )
{
	int i;

	for (i=1; i<strlen(f); i++ )
	{
		if (f[i]=='.') return;

		if ((f[i]==' '||f[i]==0) )
		{
			f[i]='.';
			f[i+1]='L';
			f[i+2]= 'V';
			f[i+3]= 'L';
			f[i+4]=0;
			return;
		}
	}

	if (i < 123)
	{
		f[i]='.';
		f[i+1]='L';
		f[i+2]= 'V';
		f[i+3]= 'L';
		f[i+4]=0;
		return;
	}
}

//these variables store the "permanant" player position, which overrides
//whatever the player's position happens to be when the game is saved
int Perm_player_segnum=-1;		//-1 means position not set
vms_vector Perm_player_position;
vms_matrix Perm_player_orient;

//set the player's "permanant" position from the current position
int SetPlayerPosition()
{
	Perm_player_position = ConsoleObject->pos;
	Perm_player_orient = ConsoleObject->orient;
	Perm_player_segnum = ConsoleObject->segnum;

	editor_status("Player initial position set");
	return 0;
}

// Save game
// returns 1 if successful
//	returns 0 if unsuccessful
int SaveGameData()
{
	char Message[200];

	if (gamestate_not_restored) {
		sprintf( Message, "Game State has not been restored...\nContinue?\n");
		if (MessageBox( -2, -2, 2, Message, "NO", "Yes" )==1) 
			return 0;
		}
		
   if (ui_get_filename( game_filename, "*.LVL", "SAVE GAME" )) {
		int saved_flag;
		vms_vector save_pos = ConsoleObject->pos;
		vms_matrix save_orient = ConsoleObject->orient;
		int save_segnum = ConsoleObject->segnum;

      checkforgamext(game_filename);

		if (Perm_player_segnum > Highest_segment_index)
			Perm_player_segnum = -1;

		if (Perm_player_segnum!=-1) {
			if (get_seg_masks(&Perm_player_position,Perm_player_segnum,0).centermask==0) {
				ConsoleObject->pos = Perm_player_position;
				obj_relink(ConsoleObject-Objects,Perm_player_segnum);
				ConsoleObject->orient = Perm_player_orient;
			}
			else
				Perm_player_segnum=-1;		//position was bogus
		}
      saved_flag=save_level(game_filename);
		if (Perm_player_segnum!=-1) {
			int	found_save_segnum;

			if (save_segnum > Highest_segment_index)
				save_segnum = 0;

			ConsoleObject->pos = save_pos;
			found_save_segnum = find_point_seg(&save_pos,save_segnum);
			if (found_save_segnum == -1) {
				compute_segment_center(&save_pos, &(Segments[save_segnum]));
				found_save_segnum = save_segnum;
			}

			obj_relink(ConsoleObject-Objects,found_save_segnum);
			ConsoleObject->orient = save_orient;
		}
		if (saved_flag)
			return 0;
		mine_changed = 0;
	}
	return 1;
}

// returns 1 if successful
//	returns 0 if unsuccessful
int LoadGameData()
{
if (SafetyCheck())  {
	if (ui_get_filename( game_filename, "*.LVL", "LOAD GAME" ))
		{
		checkforgamext(game_filename);
		if (load_level(game_filename))
			return 0;
		Current_level_num = 0;			//not a real level
		gamestate_not_restored = 0;
		Update_flags = UF_WORLD_CHANGED;
		Perm_player_position = ConsoleObject->pos;
		Perm_player_orient = ConsoleObject->orient;
		Perm_player_segnum = ConsoleObject->segnum;
		}
	}
	return 1;
}

//called whenever a new mine is created, so new mine doesn't get name
//of last saved mine as default
void ResetFilename()
{
	strcpy(game_filename,"*.LVL");
}

