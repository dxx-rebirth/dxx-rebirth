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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/state.c,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:42:43 $
 * 
 * Functions to save/restore game state.
 * 
 * $Log: state.c,v $
 * Revision 1.1.1.1  2006/03/17 19:42:43  zicodxx
 * initial import
 *
 * Revision 1.9  2003/03/09 06:41:01  donut
 * change byte typedef to sbyte to avoid conflict with win32 byte which is unsigned
 *
 * Revision 1.8  2002/03/27 00:39:57  donut
 * missed little bit of last patch
 *
 * Revision 1.7  2002/03/27 00:30:35  donut
 * show savegame thumbnail in save mode too (and refactor savegame chooser code) (patch from Steven Mueller)
 *
 * Revision 1.6  2002/03/26 05:33:14  donut
 * fix savegame thumbnail saving in OGL
 *
 * Revision 1.5  2000/11/13 07:55:34  donut
 * fix OGL level load crash
 *
 * Revision 1.4  2000/10/26 23:58:45  donut
 * increased thumbnailtext size to remove aw too big error at high res with small font
 *
 * Revision 1.3  1999/11/20 10:05:18  donut
 * variable size menu patch from Jan Bobrowski.  Variable menu font size support and a bunch of fixes for menus that didn't work quite right, by me (MPM).
 *
 * Revision 1.2  1999/06/14 23:44:12  donut
 * Orulz' svgalib/ggi/noerror patches.
 *
 * Revision 1.1.1.1  1999/06/14 22:11:39  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.14  1995/05/26  16:16:10  john
 * Split SATURN into define's for requiring cd, using cd, etc.
 * Also started adding all the Rockwell stuff.
 * 
 * Revision 2.13  1995/04/06  15:12:20  john
 * Fixed bug with lunacy not working.
 * 
 * Revision 2.12  1995/04/04  13:33:05  john
 * Removed multiplayer save.
 * 
 * Revision 2.11  1995/03/31  13:42:10  john
 * Made saved games from the bogus saturn version read in
 * correctly.
 * 
 * Revision 2.10  1995/03/31  12:45:28  john
 * Fixed bug with previous.
 * 
 * Revision 2.9  1995/03/31  12:24:40  john
 * I had changed alt_textures from a pointer to a byte. This hosed old
 * saved games, so I restored it to an int.
 * 
 * Revision 2.8  1995/03/28  11:22:47  john
 * Added cheats to save file. Changed lunacy text.
 * 
 * Revision 2.7  1995/03/27  21:41:03  john
 * Added code to verify that the proper multi save file
 * is used when restoring a network game.
 * 
 * Revision 2.6  1995/03/27  18:04:18  john
 * Made multi save/restore require the -multisave command line arg.
 * 
 * Revision 2.5  1995/03/27  17:01:52  john
 * Made deafult choice work better.
 * 
 * Revision 2.4  1995/03/27  15:49:44  john
 * Added slots to save games.
 * 
 * Revision 2.3  1995/03/27  12:59:19  john
 * Initial version of multiplayer save games.
 * 
 * Revision 2.2  1995/03/24  13:11:35  john
 * Added save game during briefing screens.
 * 
 * Revision 2.1  1995/03/21  14:38:36  john
 * Ifdef'd out the NETWORK code.
 * 
 * Revision 2.0  1995/02/27  11:27:00  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.43  1995/02/22  14:32:41  allender
 * remove anonymous unions from object structure
 * 
 * Revision 1.42  1995/02/13  20:34:33  john
 * Lintized
 * 
 * Revision 1.41  1995/02/13  10:37:30  john
 * Saved Buggin' cheat mode to save file.
 * 
 * Revision 1.40  1995/02/09  10:24:25  john
 * *** empty log message ***
 * 
 * Revision 1.39  1995/02/09  10:22:20  john
 * Fixed bug with callsign getting trashed if you copy somebody else
 * 's save game file into your directory.
 * 
 * Revision 1.38  1995/02/08  21:01:27  john
 * Closed state file around the code that shows briefing screens so that the
 * code works on machines with clean boot with only 5 file handles.
 * 
 * Revision 1.37  1995/02/07  14:02:33  john
 * Added code to verify game restore.
 * 
 * Revision 1.36  1995/02/07  11:07:43  john
 * Added hooks for confirm on game state restore.
 * 
 * Revision 1.35  1995/02/03  11:27:36  john
 * Made inbetween level save's thumbnail's have correct aspect.
 * 
 * Revision 1.34  1995/02/03  10:58:43  john
 * Added code to save shareware style saved games into new format...
 * Also, made new player file format not have the saved game array in it.
 * 
 * Revision 1.33  1995/02/02  19:40:52  john
 * Added 10 save game slots.
 * 
 * Revision 1.32  1995/02/02  12:23:20  john
 * Made between level saves have picture.
 * 
 * Revision 1.31  1995/01/31  11:21:43  john
 * Added code for fixed with menus.
 * 
 * Revision 1.30  1995/01/29  21:37:29  mike
 * initialize variables on game load so you don't drain your energy when you fire.
 * 
 * Revision 1.29  1995/01/29  13:47:58  mike
 * Restore some variables on game load (in game).
 * 
 * Revision 1.28  1995/01/26  10:46:57  john
 * Fixed bug with state names getting hosed.
 * 
 * Revision 1.27  1995/01/26  09:51:23  john
 * Fixed bug with game descriptions getting hosed.
 * 
 * Revision 1.26  1995/01/25  16:35:49  john
 * Made so that when you hit enter during
 * game save, -empty- goes away.
 * 
 * Revision 1.25  1995/01/25  15:01:39  john
 * Upped the save file version.
 * 
 * Revision 1.24  1995/01/24  20:35:35  john
 * *** empty log message ***
 * 
 * Revision 1.23  1995/01/24  20:34:24  john
 * Fixed bug with player stats not being set right for in
 * between level saves.
 * 
 * Revision 1.22  1995/01/23  10:39:03  john
 * Added mission stuff to game saves.
 * 
 * Revision 1.21  1995/01/22  16:07:12  mike
 * localization.
 * 
 * Revision 1.20  1995/01/22  15:58:32  mike
 * localization
 * 
 * Revision 1.19  1995/01/20  11:04:40  john
 * Upped state save version.
 * 
 * Revision 1.18  1995/01/19  17:00:44  john
 * Made save game work between levels.
 * 
 * Revision 1.17  1995/01/17  14:27:33  john
 * *** empty log message ***
 * 
 * Revision 1.16  1995/01/17  13:36:37  john
 * Moved pig loading into StartNewLevelSub.
 * 
 * Revision 1.15  1995/01/16  16:53:38  john
 * Added code to save cheat state during save game.
 * 
 * Revision 1.14  1995/01/15  16:55:22  john
 * Improved mine texture parsing.
 * 
 * Revision 1.13  1995/01/12  10:45:15  john
 * Added difficulty level to save/restore game.
 * 
 * Revision 1.12  1995/01/05  15:46:55  john
 * Made weapons not rearm when starting a saved game.
 * 
 * Revision 1.11  1995/01/05  11:51:45  john
 * Added better Abort game menu.
 * Made save state return success or nopt.
 * 
 * Revision 1.10  1995/01/05  11:34:51  john
 * Took out endlevel save stuff for registered.
 * 
 * Revision 1.9  1995/01/04  18:19:52  john
 * Added automap visited list saving.
 * 
 * Revision 1.8  1995/01/04  17:29:56  john
 * Made save/restore ALT+F?. Also made them not work
 * in network mode, and if recording a demo, will 
 * quit recording.
 * 
 * Revision 1.7  1995/01/04  13:18:31  john
 * Added cool 6 game save.
 * 
 * Revision 1.6  1995/01/03  20:38:46  john
 * Saved morph objects.
 * 
 * Revision 1.5  1995/01/03  20:19:29  john
 * Pretty good working version of game save.
 * 
 * Revision 1.4  1995/01/03  14:18:18  matt
 * ifdefs added to compile code add.  Added by Mike, I think.
 * 
 * Revision 1.3  1994/12/29  18:40:19  john
 * Initial version.
 * 
 * Revision 1.2  1994/12/29  15:26:40  john
 * Put in hooks for saving/restoring game state.
 * 
 * Revision 1.1  1994/12/29  15:16:02  john
 * Initial revision
 * 
 * 
 */


#ifdef RCS
#pragma off (unreferenced)
static char rcsid[] = "$Id: state.c,v 1.1.1.1 2006/03/17 19:42:43 zicodxx Exp $";
#pragma on (unreferenced)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "mono.h"
#include "inferno.h"
#include "segment.h"
#include "textures.h"
#include "wall.h"
#include "object.h"
#include "gamemine.h"
#include "error.h"
#include "gameseg.h"
#include "switch.h"
#include "game.h"
#include "newmenu.h"
#include "cfile.h"		
#include "fuelcen.h"
#include "hash.h"
#include "key.h"
#include "piggy.h"
#include "player.h"
#include "cntrlcen.h"
#include "morph.h"
#include "weapon.h"
#include "render.h"
#include "gameseq.h"
#include "gauges.h"
#include "newdemo.h"
#include "automap.h"
#include "piggy.h"
#include "paging.h"
#include "titles.h"
#include "text.h"
#include "mission.h"
#include "pcx.h"
#include "u_mem.h"
#include "network.h"
#include "args.h"
//added 6/15/99 - Owen Evans
#include "strutil.h"
//end added
#include "gamefont.h"
#ifdef OGL
#include "ogl_init.h"
#endif


#ifndef SHAREWARE

#define STATE_VERSION 7
#define STATE_COMPATIBLE_VERSION 6
// 0 - Put DGSS (Descent Game State Save) id at tof.
// 1 - Added Difficulty level save
// 2 - Added Cheats_enabled flag
// 3 - Added between levels save.
// 4 - Added mission support
// 5 - Mike changed ai and object structure.
// 6 - Added buggin' cheat save
// 7 - Added other cheat saves and game_id.

#define NUM_SAVES 10
#define THUMBNAIL_W 100
#define THUMBNAIL_H 50
#define DESC_LENGTH 20

extern int ai_save_state( FILE * fp );
extern int ai_restore_state( FILE * fp );

extern void multi_initiate_save_game();
extern void multi_initiate_restore_game();

extern int Do_appearance_effect;
extern fix Fusion_next_sound_time;

extern int Laser_rapid_fire, Ugly_robot_cheat, Ugly_robot_texture;
extern int Physics_cheat_flag;
extern int	Lunacy;
extern void do_lunacy_on(void);
extern void do_lunacy_off(void);

int state_save_all_sub(char *filename, char *desc, int between_levels);
int state_restore_all_sub(char *filename, int multi);

int sc_last_item= 0;
grs_bitmap *sc_bmp[NUM_SAVES];

char dgss_id[4] = "DGSS";

int state_default_item = 0;

uint state_game_id;

void state_callback(int nitems,newmenu_item * items, int * last_key, int citem)
/*{
	nitems = nitems;
	last_key = last_key;
	
//	if ( sc_last_item != citem )	{
//		sc_last_item = citem;
		if ( citem > 0 )	{
			int w=SWIDTH * THUMBNAIL_W / 320;
			int h=SHEIGHT * THUMBNAIL_H / 200;
			if ( sc_bmp[citem-1] )	{
//				gr_set_current_canvas( NULL );
				if (SWIDTH>320 || SHEIGHT>200){
					grs_canvas *tmp,*old;
					old=grd_curcanv;
					tmp=gr_create_sub_canvas(grd_curcanv, (grd_curcanv->cv_bitmap.bm_w-w)/2,
							items[0].y-5, w, h);
					gr_set_current_canvas(tmp);
					show_fullscr( sc_bmp[citem-1] );
//					gr_set_current_canvas(NULL);
					gr_set_current_canvas(old);
					gr_free_sub_canvas(tmp);
				}else
					gr_bitmap( (grd_curcanv->cv_bitmap.bm_w-THUMBNAIL_W)/2,items[0].y-5, sc_bmp[citem-1] );
			} else {
				gr_setcolor( BM_XRGB( 0, 0, 0 ) );
				gr_rect( (grd_curcanv->cv_bitmap.bm_w - w) / 2,     items[0].y - 5, 
				         (grd_curcanv->cv_bitmap.bm_w + w) / 2 - 1, items[0].y - 5 + h - 1); 
			}
		}
//	}	
}*/
{
	nitems = nitems;
	last_key = last_key;
	
		if ( citem > 0 )	{
			if ( sc_bmp[citem-1] )	{
				if (grd_curcanv->cv_bitmap.bm_w > 320) {
					grs_canvas *save_canv = grd_curcanv;
					grs_canvas *temp_canv = gr_create_canvas(THUMBNAIL_W*2,(THUMBNAIL_H*24/10));
					grs_point vertbuf[3] = {{0,0}, {0,0}, {i2f(THUMBNAIL_W*2),i2f(THUMBNAIL_H*24/10)} };
					gr_set_current_canvas(temp_canv);
					scale_bitmap(sc_bmp[citem-1], vertbuf);
					gr_set_current_canvas( save_canv );
#ifndef OGL
					gr_bitmap( (grd_curcanv->cv_bitmap.bm_w-THUMBNAIL_W*2)/2,items[0].y-10, &temp_canv->cv_bitmap);
#else
					ogl_ubitmapm_cf((grd_curcanv->cv_bitmap.bm_w/2)-((double)(THUMBNAIL_W/2)*(SWIDTH/320)),items[0].y-10,((double)THUMBNAIL_W*(SWIDTH/320)),((double)THUMBNAIL_H*(SHEIGHT/200)),&temp_canv->cv_bitmap,255,F1_0);
#endif
					gr_free_canvas(temp_canv);
				}
				else	{
					gr_bitmap( (grd_curcanv->cv_bitmap.bm_w-THUMBNAIL_W)/2,items[0].y-5, sc_bmp[citem-1] );
				}
			}
		}

}

void rpad_string( char * string, int max_chars )
{
	int i, end_found;

	end_found = 0;
	for( i=0; i<max_chars; i++ )	{
		if ( *string == 0 )
			end_found = 1;
		if ( end_found )
			*string = ' ';
		string++;
	}
	*string = 0;		// NULL terminate
}

/* Present a menu for selection of a savegame filename.
 * For saving, dsc should be a pre-allocated buffer into which the new
 * savegame description will be stored.
 * For restoring, dsc should be NULL, in which case empty slots will not be
 * selectable and savagames descriptions will not be editable.
 */
int state_get_savegame_filename(char * fname, char * dsc, int multi, char * caption )
{
	FILE * fp;
	int i, choice, version, nsaves;
	newmenu_item m[NUM_SAVES+1];
	char filename[NUM_SAVES][20];
	char desc[NUM_SAVES][DESC_LENGTH + 16];
	char id[5];
	char thumbnailtext[50];
	int valid;

	nsaves=0;
	m[0].type = NM_TYPE_TEXT; 
	{
		int w,h,h2,aw;
		grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_1];
		gr_get_string_size("\n",&w,&h,&aw);
		gr_get_string_size("\n\n",&w,&h2,&aw);//adding successive newlines increases less than the size of a single
		h2=h2-h;
		aw=5;
		if (aw>=49) Error("state_get_restore_file: aw too big\n");
		memset(thumbnailtext,'\n',aw);
		thumbnailtext[aw]=0;
		m[0].text = thumbnailtext;
	}
	for (i=0;i<NUM_SAVES; i++ )	{
		sc_bmp[i] = NULL;
		if (!multi)
			sprintf( filename[i], "%s.sg%d", Players[Player_num].callsign, i );
		else
			sprintf( filename[i], "%s.mg%d", Players[Player_num].callsign, i );
//added on 9/30/98 by Matt Mueller to fix savegames in linux
		strlwr(filename[i]);
//end addition -MM
		valid = 0;
		fp = fopen( filename[i], "rb" );
		if ( fp ) {
			//Read id
			fread( id, sizeof(char)*4, 1, fp );
			if ( !memcmp( id, dgss_id, 4 )) {
				//Read version
				fread( &version, sizeof(int), 1, fp );
				if (version >= STATE_COMPATIBLE_VERSION)	{
					// Read description
					fread( desc[i], sizeof(char)*DESC_LENGTH, 1, fp );
					//rpad_string( desc[i], DESC_LENGTH-1 );
					if (dsc == NULL) m[i+1].type = NM_TYPE_MENU;
					// Read thumbnail
					sc_bmp[i] = gr_create_bitmap(THUMBNAIL_W,THUMBNAIL_H );
					fread( sc_bmp[i]->bm_data, THUMBNAIL_W * THUMBNAIL_H, 1, fp );
					nsaves++;
					valid = 1;
				} 
			}
			fclose(fp);
		} 
		if (!valid) {
			strcpy( desc[i], TXT_EMPTY );
			//rpad_string( desc[i], DESC_LENGTH-1 );
			if (dsc == NULL) m[i+1].type = NM_TYPE_TEXT;
		}
		if (dsc != NULL) {
			m[i+1].type = NM_TYPE_INPUT_MENU;
		}
		m[i+1].text_len = DESC_LENGTH-1;
		m[i+1].text = desc[i];
	}

	if ( dsc == NULL && nsaves < 1 )	{
		nm_messagebox( NULL, 1, "Ok", "No saved games were found!" );
		return 0;
	}

	sc_last_item = -1;
	choice = newmenu_do3( NULL, caption, NUM_SAVES+1, m, state_callback, state_default_item+1, NULL, -1, -1 );

	for (i=0; i<NUM_SAVES; i++ )	{
		if ( sc_bmp[i] )
			gr_free_bitmap( sc_bmp[i] );
	}

	if (choice > 0) {
		strcpy( fname, filename[choice-1] );
		if ( dsc != NULL ) strcpy( dsc, desc[choice-1] );
		state_default_item = choice - 1;
		return choice;
	}
	return 0;
}

int state_get_save_file(char * fname, char * dsc, int multi )
{
	return state_get_savegame_filename(fname, dsc, multi, "Save Game");
}

int state_get_restore_file(char * fname, int multi )
{
	return state_get_savegame_filename(fname, NULL, multi, "Select Game to Restore");
}

int state_save_old_game(int slotnum, char * sg_name, player * sg_player, 
                        int sg_difficulty_level, int sg_primary_weapon, 
                        int sg_secondary_weapon, int sg_next_level_num  	)
{
	int i;
	int temp_int;
	ubyte temp_byte;
	char desc[DESC_LENGTH+1];
	char filename[128];
	grs_canvas * cnv;
	FILE * fp;

	sprintf( filename, "%s.sg%d", sg_player->callsign, slotnum );
//added on 9/30/98 by Matt Mueller to fix savegames in linux
	strlwr(filename);
//end addition -MM
	fp = fopen( filename, "wb" );
	if ( !fp ) return 0;

//Save id
	fwrite( dgss_id, sizeof(char)*4, 1, fp );

//Save version
	temp_int = STATE_VERSION;
	fwrite( &temp_int, sizeof(int), 1, fp );

//Save description
	strncpy( desc, sg_name, DESC_LENGTH );
	fwrite( desc, sizeof(char)*DESC_LENGTH, 1, fp );
	
// Save the current screen shot...
	cnv = gr_create_canvas( THUMBNAIL_W, THUMBNAIL_H );
	if ( cnv )	{
		char * pcx_file;
		ubyte pcx_palette[768];

		gr_set_current_canvas( cnv );

		gr_clear_canvas( BM_XRGB(0,0,0) );
		pcx_file = get_briefing_screen( sg_next_level_num );
		if ( pcx_file != NULL )	{
			grs_bitmap bmp;
			gr_init_bitmap_data (&bmp);
			if (pcx_read_bitmap( pcx_file, &bmp, BM_LINEAR, pcx_palette )==PCX_ERROR_NONE)	{
				grs_point vertbuf[3];
				gr_clear_canvas( 255 );
				vertbuf[0].x = vertbuf[0].y = -F1_0*6;		// -6 pixel rows for ascpect
				vertbuf[1].x = vertbuf[1].y = 0;
				vertbuf[2].x = i2f(THUMBNAIL_W); vertbuf[2].y = i2f(THUMBNAIL_H+7);	// + 7 pixel rows for ascpect
				scale_bitmap(&bmp, vertbuf );
				gr_remap_bitmap_good( &cnv->cv_bitmap, pcx_palette, -1, -1 );
			}
			gr_free_bitmap_data( &bmp );
		}
		fwrite( cnv->cv_bitmap.bm_data, THUMBNAIL_W*THUMBNAIL_H, 1, fp );
		gr_free_canvas( cnv );
	} else {
	 	ubyte color = 0;
	 	for ( i=0; i<THUMBNAIL_W*THUMBNAIL_H; i++ )
	 		fwrite( &color, sizeof(ubyte), 1, fp );		
	}

// Save the Between levels flag...
	temp_int = 1;
	fwrite( &temp_int, sizeof(int), 1, fp );

// Save the mission info...
	fwrite( &Mission_list[0], sizeof(char)*9, 1, fp );

//Save level info
	temp_int = sg_player->level;
	fwrite( &temp_int, sizeof(int), 1, fp );
	temp_int = sg_next_level_num;
	fwrite( &temp_int, sizeof(int), 1, fp );

//Save GameTime
	temp_int = 0;
	fwrite( &temp_int, sizeof(fix), 1, fp );

//Save player info
	fwrite( sg_player, sizeof(player), 1, fp );

// Save the current weapon info
	temp_byte = sg_primary_weapon;
	fwrite( &temp_byte, sizeof(byte), 1, fp );
	temp_byte = sg_secondary_weapon;
	fwrite( &temp_byte, sizeof(byte), 1, fp );

// Save the difficulty level
	temp_int = sg_difficulty_level;
	fwrite( &temp_int, sizeof(int), 1, fp );

// Save the Cheats_enabled
	temp_int = 0;
	fwrite( &temp_int, sizeof(int), 1, fp );
	temp_int = 0;		// turbo mode
	fwrite( &temp_int, sizeof(int), 1, fp );

	fwrite( &state_game_id, sizeof(uint), 1, fp );
	fwrite( &Laser_rapid_fire, sizeof(int), 1, fp );
	fwrite( &Ugly_robot_cheat, sizeof(int), 1, fp );
	fwrite( &Ugly_robot_texture, sizeof(int), 1, fp );
	fwrite( &Physics_cheat_flag, sizeof(int), 1, fp );
	fwrite( &Lunacy, sizeof(int), 1, fp );

	fclose(fp);

	return 1;
}


int state_save_all(int between_levels)
{
	char filename[128], desc[DESC_LENGTH+1];

	if ( Game_mode & GM_MULTI )	{
#ifdef MULTI_SAVE
		if ( FindArg( "-multisave" ) )
			multi_initiate_save_game();
		else
#endif  
			hud_message( MSGC_GAME_FEEDBACK, "Can't save in a multiplayer game!" );
		return 0;
	}

	mprintf(( 0, "CL=%d, NL=%d\n", Current_level_num, Next_level_num ));
	
	stop_time();

	if (!state_get_save_file(filename,desc,0))	{
		start_time();
		return 0;
	}
		
	return state_save_all_sub(filename, desc, between_levels);
}

int state_save_all_sub(char *filename, char *desc, int between_levels)
{
	int i,j;
	FILE * fp;
	grs_canvas * cnv;

	if ( Game_mode & GM_MULTI )	{
#ifdef MULTI_SAVE
		if ( !FindArg( "-multisave" ) ) 
#endif  
			return 0;
	}

	fp = fopen( filename, "wb" );
	if ( !fp ) {
		start_time();
		return 0;
	}

//Save id
	fwrite( dgss_id, sizeof(char)*4, 1, fp );

//Save version
	i = STATE_VERSION;
	fwrite( &i, sizeof(int), 1, fp );

//Save description
	fwrite( desc, sizeof(char)*DESC_LENGTH, 1, fp );
	
// Save the current screen shot...
	cnv = gr_create_canvas( THUMBNAIL_W, THUMBNAIL_H );
	if ( cnv )	{
		gr_set_current_canvas( cnv );
		if ( between_levels )	{
			char * pcx_file;
			ubyte pcx_palette[768];

			gr_clear_canvas( BM_XRGB(0,0,0) );
			pcx_file = get_briefing_screen( Next_level_num );
			if ( pcx_file != NULL )	{
				grs_bitmap bmp;
				gr_init_bitmap_data (&bmp);
				if (pcx_read_bitmap( pcx_file, &bmp, BM_LINEAR, pcx_palette )==PCX_ERROR_NONE)	{
					grs_point vertbuf[3];
					gr_clear_canvas( 255 );
					vertbuf[0].x = vertbuf[0].y = -F1_0*6;		// -6 pixel rows for ascpect
					vertbuf[1].x = vertbuf[1].y = 0;
					vertbuf[2].x = i2f(THUMBNAIL_W); vertbuf[2].y = i2f(THUMBNAIL_H+7);	// + 7 pixel rows for ascpect
					scale_bitmap(&bmp, vertbuf );
					gr_remap_bitmap_good( &cnv->cv_bitmap, pcx_palette, -1, -1 );
				}
				gr_free_bitmap_data (&bmp);
			}
		} else {
			render_frame(0);
#ifdef OGL
			ogl_ubitblt_tolinear(grd_curcanv->cv_bitmap.bm_w, grd_curcanv->cv_bitmap.bm_h, 0, 0, 0, 0, &grd_curscreen->sc_canvas.cv_bitmap, &grd_curcanv->cv_bitmap);
#endif
		}
		fwrite( cnv->cv_bitmap.bm_data, THUMBNAIL_W*THUMBNAIL_H, 1, fp );
//added ifdef on 9/30/98 by Matt Mueller to fix savegames in linux
// I dunno why it doesn't like this, all I can guess is that the mem system
// in descent is screwed somewhere.
#ifndef __LINUX__
		gr_free_canvas( cnv );
#endif
//end addition -MM
	} else {
	 	ubyte color = 0;
	 	for ( i=0; i<THUMBNAIL_W*THUMBNAIL_H; i++ )
	 		fwrite( &color, sizeof(ubyte), 1, fp );		
	}

// Save the Between levels flag...
	fwrite( &between_levels, sizeof(int), 1, fp );

// Save the mission info...
	fwrite( &Mission_list[Current_mission_num], sizeof(char)*9, 1, fp );

//Save level info
	fwrite( &Current_level_num, sizeof(int), 1, fp );
	fwrite( &Next_level_num, sizeof(int), 1, fp );

//Save GameTime
	fwrite( &GameTime, sizeof(fix), 1, fp );

//Save player info
	fwrite( &Players[Player_num], sizeof(player), 1, fp );

// Save the current weapon info
	fwrite( &Primary_weapon, sizeof(byte), 1, fp );
	fwrite( &Secondary_weapon, sizeof(byte), 1, fp );

// Save the difficulty level
	fwrite( &Difficulty_level, sizeof(int), 1, fp );

// Save the Cheats_enabled
	fwrite( &Cheats_enabled, sizeof(int), 1, fp );
	fwrite( &Game_turbo_mode, sizeof(int), 1, fp );


	if ( !between_levels )	{

	//Finish all morph objects
		for (i=0; i<=Highest_object_index; i++ )	{
			if ( (Objects[i].type != OBJ_NONE) && (Objects[i].render_type==RT_MORPH))	{
				morph_data *md;
				md = find_morph_data(&Objects[i]);
				if (md) {					
					md->obj->control_type = md->morph_save_control_type;
					md->obj->movement_type = md->morph_save_movement_type;
					md->obj->render_type = RT_POLYOBJ;
					md->obj->mtype.phys_info = md->morph_save_phys_info;
					md->obj = NULL;
				} else {						//maybe loaded half-morphed from disk
					Objects[i].flags |= OF_SHOULD_BE_DEAD;
					Objects[i].render_type = RT_POLYOBJ;
					Objects[i].control_type = CT_NONE;
					Objects[i].movement_type = MT_NONE;
				}
			}
		}
	
	//Save object info
		i = Highest_object_index+1;
		fwrite( &i, sizeof(int), 1, fp );
		fwrite( Objects, sizeof(object)*i, 1, fp );
		
	//Save wall info
		i = Num_walls;
		fwrite( &i, sizeof(int), 1, fp );
		fwrite( Walls, sizeof(wall)*i, 1, fp );
	
	//Save door info
		i = Num_open_doors;
		fwrite( &i, sizeof(int), 1, fp );
		fwrite( ActiveDoors, sizeof(active_door)*i, 1, fp );
	
	//Save trigger info
		fwrite( &Num_triggers, sizeof(int), 1, fp );
		fwrite( Triggers, sizeof(trigger)*Num_triggers, 1, fp );
	
	//Save tmap info
		for (i=0; i<=Highest_segment_index; i++ )	{
			for (j=0; j<6; j++ )	{
				fwrite( &Segments[i].sides[j].wall_num, sizeof(short), 1, fp );
				fwrite( &Segments[i].sides[j].tmap_num, sizeof(short), 1, fp );
				fwrite( &Segments[i].sides[j].tmap_num2, sizeof(short), 1, fp );
			}
		}
	
	// Save the fuelcen info
		fwrite( &Fuelcen_control_center_destroyed, sizeof(int), 1, fp );
		fwrite( &Fuelcen_seconds_left, sizeof(int), 1, fp );
		fwrite( &Num_robot_centers, sizeof(int), 1, fp );
		fwrite( RobotCenters, sizeof(matcen_info)*Num_robot_centers, 1, fp );
		fwrite( &ControlCenterTriggers, sizeof(control_center_triggers), 1, fp );
		fwrite( &Num_fuelcenters, sizeof(int), 1, fp );
		fwrite( Station, sizeof(FuelCenter)*Num_fuelcenters, 1, fp );
	
	// Save the control cen info
		fwrite( &Control_center_been_hit, sizeof(int), 1, fp );
		fwrite( &Control_center_player_been_seen, sizeof(int), 1, fp );
		fwrite( &Control_center_next_fire_time, sizeof(int), 1, fp );
		fwrite( &Control_center_present, sizeof(int), 1, fp );
		fwrite( &Dead_controlcen_object_num, sizeof(int), 1, fp );
	
	// Save the AI state
		ai_save_state( fp );
	
	// Save the automap visited info
		fwrite( Automap_visited, sizeof(ubyte)*MAX_SEGMENTS, 1, fp );
	}
	fwrite( &state_game_id, sizeof(uint), 1, fp );
	fwrite( &Laser_rapid_fire, sizeof(int), 1, fp );
	fwrite( &Ugly_robot_cheat, sizeof(int), 1, fp );
	fwrite( &Ugly_robot_texture, sizeof(int), 1, fp );
	fwrite( &Physics_cheat_flag, sizeof(int), 1, fp );
	fwrite( &Lunacy, sizeof(int), 1, fp );

	fclose(fp);

	start_time();

	return 1;
}

int state_restore_all(int in_game)
{
	char filename[128];

	if ( Game_mode & GM_MULTI )	{
#ifdef MULTI_SAVE
		if ( FindArg( "-multisave" ) )
			multi_initiate_restore_game();
		else	
#endif
			hud_message( MSGC_GAME_FEEDBACK, "Can't restore in a multiplayer game!" );
		return 0;
	}

	if ( Newdemo_state == ND_STATE_RECORDING )
		newdemo_stop_recording();

	if ( Newdemo_state != ND_STATE_NORMAL )
		return 0;

	stop_time();
	if (!state_get_restore_file(filename,0))	{
		start_time();
		return 0;
	}

	if ( in_game )		{
		int choice;
		choice =  nm_messagebox( NULL, 2, "Yes", "No", "Restore Game?" );
		if ( choice != 0 )	{
			start_time();
			return 0;
		}
	}

	start_time();

	return state_restore_all_sub(filename, 0);
}

int state_restore_all_sub(char *filename, int multi)
{
	int ObjectStartLocation;
	int BogusSaturnShit = 0;
	int version,i, j, segnum;
	object * obj;
	FILE *fp;
	int current_level, next_level;
	int between_levels;
	char mission[16];
	char desc[DESC_LENGTH+1];
	char id[5];
	char org_callsign[CALLSIGN_LEN+16];

	if ( Game_mode & GM_MULTI )	{
#ifdef MULTI_SAVE
		if ( !FindArg( "-multisave" ) ) 
#endif
			return 0;
	}

	fp = fopen( filename, "rb" );
	if ( !fp ) return 0;

//Read id
	fread( id, sizeof(char)*4, 1, fp );
	if ( memcmp( id, dgss_id, 4 )) {
		fclose(fp);
		return 0;
	}

//Read version
	fread( &version, sizeof(int), 1, fp );
	if (version < STATE_COMPATIBLE_VERSION)	{
		fclose(fp);
		return 0;
	}

// Read description
	fread( desc, sizeof(char)*DESC_LENGTH, 1, fp );

// Skip the current screen shot...
	fseek( fp, THUMBNAIL_W*THUMBNAIL_H, SEEK_CUR );

// Read the Between levels flag...
	fread( &between_levels, sizeof(int), 1, fp );

// Read the mission info...
	fread( mission, sizeof(char)*9, 1, fp );

	if (!load_mission_by_name( mission ))	{
		nm_messagebox( NULL, 1, "Ok", "Error!\nUnable to load mission\n'%s'\n", mission );
		fclose(fp);
		return 0;
	}

//Read level info
	fread( &current_level, sizeof(int), 1, fp );
	fread( &next_level, sizeof(int), 1, fp );

//Restore GameTime
	fread( &GameTime, sizeof(fix), 1, fp );

// Start new game....
	if (!multi)	{
		Game_mode = GM_NORMAL;
		Function_mode = FMODE_GAME;
#ifdef NETWORK
		change_playernum_to(0);
#endif
		strcpy( org_callsign, Players[0].callsign );
		N_players = 1;
		InitPlayerObject();				//make sure player's object set up
		init_player_stats_game();		//clear all stats
	} else {
		strcpy( org_callsign, Players[Player_num].callsign );
	}
//Read player info

	if ( between_levels )	{
		int saved_offset;
		fread( &Players[Player_num], sizeof(player), 1, fp );
		saved_offset = ftell(fp);
		fclose( fp );
		do_briefing_screens(next_level);
		fp = fopen( filename, "rb" );
		fseek( fp, saved_offset, SEEK_SET );
 		StartNewLevelSub( next_level, 1);//use page_in_textures here to fix OGL texture precashing crash -MPM
	} else {
		StartNewLevelSub(current_level, 1);//use page_in_textures here to fix OGL texture precashing crash -MPM
		fread( &Players[Player_num], sizeof(player), 1, fp );
	}
	strcpy( Players[Player_num].callsign, org_callsign );

// Set the right level
	if ( between_levels )
		Players[Player_num].level = next_level;

// Restore the weapon states
	fread( &Primary_weapon, sizeof(byte), 1, fp );
	fread( &Secondary_weapon, sizeof(byte), 1, fp );

	select_weapon(Primary_weapon, 0, 0, 0);
	select_weapon(Secondary_weapon, 1, 0, 0);

// Restore the difficulty level
	fread( &Difficulty_level, sizeof(int), 1, fp );

// Restore the cheats enabled flag
	fread( &Cheats_enabled, sizeof(int), 1, fp );
	fread( &Game_turbo_mode, sizeof(int), 1, fp );

	if ( !between_levels )	{
		Do_appearance_effect = 0;			// Don't do this for middle o' game stuff.

		ObjectStartLocation = ftell( fp );
RetryObjectLoading:
		//Clear out all the objects from the lvl file
		for (segnum=0; segnum <= Highest_segment_index; segnum++)
			Segments[segnum].objects = -1;
		reset_objects(1);
	
		//Read objects, and pop 'em into their respective segments.
		fread( &i, sizeof(int), 1, fp );
		Highest_object_index = i-1;
		if ( !BogusSaturnShit )	
			fread( Objects, sizeof(object)*i, 1, fp );
		else {
			ubyte tmp_object[sizeof(object)];
			for (i=0; i<=Highest_object_index; i++ )	{
				fread( tmp_object, sizeof(object)-3, 1, fp );
				// Insert 3 bytes after the read in obj->rtype.pobj_info.alt_textures field.
				memcpy( &Objects[i], tmp_object, sizeof(object)-3 );
				Objects[i].rtype.pobj_info.alt_textures = -1;
			}
		}
	
		Object_next_signature = 0;
		for (i=0; i<=Highest_object_index; i++ )	{
			obj = &Objects[i];
			obj->rtype.pobj_info.alt_textures = -1;
			segnum = obj->segnum;
			obj->next = obj->prev = obj->segnum = -1;
			if ( obj->type != OBJ_NONE )	{
				// Check for a bogus Saturn version!!!!
				if (!BogusSaturnShit )	{
					if ( (segnum<0) || (segnum>Highest_segment_index) ) {
						BogusSaturnShit = 1;
						mprintf(( 1, "READING BOGUS SATURN VERSION OBJECTS!!! (Object:%d)\n", i ));
						fseek( fp, ObjectStartLocation, SEEK_SET );
						goto RetryObjectLoading;
					}
				}
				obj_link(i,segnum);
				if ( obj->signature > Object_next_signature )
					Object_next_signature = obj->signature;
			}
		}	
		special_reset_objects();
		Object_next_signature++;
	
		//Restore wall info
		fread( &i, sizeof(int), 1, fp );
		Num_walls = i;
		// Check for a bogus Saturn version!!!!
		if (!BogusSaturnShit )	{
			if ( (Num_walls<0) || (Num_walls>MAX_WALLS) ) {
				BogusSaturnShit = 1;
				mprintf(( 1, "READING BOGUS SATURN VERSION OBJECTS!!! (Num_walls)\n" ));
				fseek( fp, ObjectStartLocation, SEEK_SET );
				goto RetryObjectLoading;
			}
		}

		fread( Walls, sizeof(wall)*Num_walls, 1, fp );
		// Check for a bogus Saturn version!!!!
		if (!BogusSaturnShit )	{
			for (i=0; i<Num_walls; i++ )	{
				if ( (Walls[i].segnum<0) || (Walls[i].segnum>Highest_segment_index) || (Walls[i].sidenum<-1) || (Walls[i].sidenum>5) ) {
					BogusSaturnShit = 1;
					mprintf(( 1, "READING BOGUS SATURN VERSION OBJECTS!!! (Wall %d)\n", i ));
					fseek( fp, ObjectStartLocation, SEEK_SET );
					goto RetryObjectLoading;
				}
			}
		}
	
		//Restore door info
		fread( &i, sizeof(int), 1, fp );
		Num_open_doors = i;
		fread( ActiveDoors, sizeof(active_door)*Num_open_doors, 1, fp );
	
		//Restore trigger info
		fread( &Num_triggers, sizeof(int), 1, fp );
		fread( Triggers, sizeof(trigger)*Num_triggers, 1, fp );
	
		//Restore tmap info
		for (i=0; i<=Highest_segment_index; i++ )	{
			for (j=0; j<6; j++ )	{
				fread( &Segments[i].sides[j].wall_num, sizeof(short), 1, fp );
				fread( &Segments[i].sides[j].tmap_num, sizeof(short), 1, fp );
				fread( &Segments[i].sides[j].tmap_num2, sizeof(short), 1, fp );
			}
		}
	
		//Restore the fuelcen info
		fread( &Fuelcen_control_center_destroyed, sizeof(int), 1, fp );
		fread( &Fuelcen_seconds_left, sizeof(int), 1, fp );
		fread( &Num_robot_centers, sizeof(int), 1, fp );
		fread( RobotCenters, sizeof(matcen_info)*Num_robot_centers, 1, fp );
		fread( &ControlCenterTriggers, sizeof(control_center_triggers), 1, fp );
		fread( &Num_fuelcenters, sizeof(int), 1, fp );
		fread( Station, sizeof(FuelCenter)*Num_fuelcenters, 1, fp );
	
		// Restore the control cen info
		fread( &Control_center_been_hit, sizeof(int), 1, fp );
		fread( &Control_center_player_been_seen, sizeof(int), 1, fp );
		fread( &Control_center_next_fire_time, sizeof(int), 1, fp );
		fread( &Control_center_present, sizeof(int), 1, fp );
		fread( &Dead_controlcen_object_num, sizeof(int), 1, fp );
	
		// Restore the AI state
		ai_restore_state( fp );
	
		// Restore the automap visited info
		fread( Automap_visited, sizeof(ubyte)*MAX_SEGMENTS, 1, fp );

		//	Restore hacked up weapon system stuff.
		Fusion_next_sound_time = GameTime;
		Auto_fire_fusion_cannon_time = 0;
		Next_laser_fire_time = GameTime;
		Next_missile_fire_time = GameTime;
		Last_laser_fired_time = GameTime;

	}
	state_game_id = 0;

	if ( version >= 7 )	{
		int tmp_Lunacy;
		fread( &state_game_id, sizeof(uint), 1, fp );
		fread( &Laser_rapid_fire, sizeof(int), 1, fp );
		fread( &Ugly_robot_cheat, sizeof(int), 1, fp );
		fread( &Ugly_robot_texture, sizeof(int), 1, fp );
		fread( &Physics_cheat_flag, sizeof(int), 1, fp );
		fread( &tmp_Lunacy, sizeof(int), 1, fp );
		if ( tmp_Lunacy )
			do_lunacy_on();
	}

	fclose(fp);

// Load in bitmaps, etc..
//	piggy_load_level_data();//already used page_in_textures in StartNewLevelSub, so no need for this here. -MPM

	return 1;
}


#endif
