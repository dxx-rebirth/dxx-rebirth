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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/state.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:42:43 $
 * 
 * Prototypes for state saving functions.
 * 
 * $Log: state.h,v $
 * Revision 1.1.1.1  2006/03/17 19:42:43  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:13:11  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.1  1995/03/27  21:40:35  john
 * Added code to verify that the proper multi save file
 * is used when restoring a network game.
 * 
 * Revision 2.0  1995/02/27  11:28:44  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.6  1995/02/07  10:54:05  john
 * *** empty log message ***
 * 
 * Revision 1.5  1995/02/03  10:58:12  john
 * Added code to save shareware style saved games into new format...
 * Also, made new player file format not have the saved game array in it.
 * 
 * Revision 1.4  1995/01/19  17:00:51  john
 * Made save game work between levels.
 * 
 * Revision 1.3  1995/01/05  11:51:44  john
 * Added better Abort game menu.
 * Made save state return success or nopt.
 * 
 * Revision 1.2  1994/12/29  15:26:39  john
 * Put in hooks for saving/restoring game state.
 * 
 * Revision 1.1  1994/12/29  15:15:47  john
 * Initial revision
 * 
 * 
 */



#ifndef _STATE_H
#define _STATE_H

int state_save_all(int between_levels);
int state_restore_all(int in_game );

extern int state_save_old_game(int slotnum, char * sg_name, player * sg_player, 
                        int sg_difficulty_level, int sg_primary_weapon, 
                        int sg_secondary_weapon, int sg_next_level_num  	);

int state_save_all_sub(char *filename, char *desc, int between_levels);
int state_restore_all_sub(char *filename, int multi);

extern uint state_game_id;

int state_get_save_file(char * fname, char * dsc, int multi );
int state_get_restore_file(char * fname, int multi );

#endif
 
