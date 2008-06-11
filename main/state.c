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
 * Functions to save/restore game state.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

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
#include "args.h"
//added 6/15/99 - Owen Evans
#include "strutil.h"
//end added
#include "gamefont.h"
#ifdef NETWORK
#include "network.h"
#endif
#ifdef OGL
#include "ogl_init.h"
#endif
#include "physfsx.h"


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

extern int ai_save_state( PHYSFS_file * fp );
extern int ai_restore_state( PHYSFS_file * fp );

extern int Do_appearance_effect;
extern fix Fusion_next_sound_time;

extern int Laser_rapid_fire, Ugly_robot_cheat, Ugly_robot_texture;
extern int Physics_cheat_flag;
extern int	Lunacy;
extern void do_lunacy_on(void);
extern void do_lunacy_off(void);

int state_save_all_sub(char *filename, char *desc, int between_levels);
int state_restore_all_sub(char *filename);

int sc_last_item= 0;
grs_bitmap *sc_bmp[NUM_SAVES];

char dgss_id[4] = "DGSS";

uint state_game_id;

void state_callback(int nitems,newmenu_item * items, int * last_key, int citem)
{
	nitems = nitems;
	last_key = last_key;
	
	if ( citem > 0 )
	{
		if ( sc_bmp[citem-1] )	{
			grs_canvas *save_canv = grd_curcanv;
			grs_canvas *temp_canv = gr_create_canvas(THUMBNAIL_W*2,(THUMBNAIL_H*24/10));
			grs_point vertbuf[3] = {{0,0}, {0,0}, {i2f(THUMBNAIL_W*2),i2f(THUMBNAIL_H*24/10)} };
			gr_set_current_canvas(temp_canv);
			scale_bitmap(sc_bmp[citem-1], vertbuf);
			gr_set_current_canvas( save_canv );
#ifndef OGL
			gr_bitmap( (grd_curcanv->cv_bitmap.bm_w-THUMBNAIL_W*2)/2,items[0].y-3, &temp_canv->cv_bitmap);
#else
			ogl_ubitmapm_cs((grd_curcanv->cv_bitmap.bm_w/2)-FSPACX(THUMBNAIL_W/2),items[0].y-FSPACY(3),FSPACX(THUMBNAIL_W),FSPACY(THUMBNAIL_H),&temp_canv->cv_bitmap,255,F1_0);
#endif
			gr_free_canvas(temp_canv);
		}
	}
}

#if 0
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
#endif

/* Present a menu for selection of a savegame filename.
 * For saving, dsc should be a pre-allocated buffer into which the new
 * savegame description will be stored.
 * For restoring, dsc should be NULL, in which case empty slots will not be
 * selectable and savagames descriptions will not be editable.
 */
int state_get_savegame_filename(char * fname, char * dsc, char * caption )
{
	PHYSFS_file * fp;
	int i, choice, version, nsaves;
	newmenu_item m[NUM_SAVES+1];
	char filename[NUM_SAVES][FILENAME_LEN + 9];
	char desc[NUM_SAVES][DESC_LENGTH + 16];
	char id[5];
	int valid;
	static int state_default_item = 0;

	nsaves=0;
	m[0].type = NM_TYPE_TEXT; m[0].text = "\n\n\n\n";
	for (i=0;i<NUM_SAVES; i++ )	{
		sc_bmp[i] = NULL;
		sprintf( filename[i], GameArg.SysUsePlayersDir? "Players/%s.sg%x" : "%s.sg%x", Players[Player_num].callsign, i );
		valid = 0;
		fp = PHYSFSX_openReadBuffered(filename[i]);
		if ( fp ) {
			//Read id
			PHYSFS_read(fp, id, sizeof(char) * 4, 1);
			if ( !memcmp( id, dgss_id, 4 )) {
				//Read version
				PHYSFS_read(fp, &version, sizeof(int), 1);
				if (version >= STATE_COMPATIBLE_VERSION) {
					// Read description
					PHYSFS_read(fp, desc[i], sizeof(char) * DESC_LENGTH, 1);
					//rpad_string( desc[i], DESC_LENGTH-1 );
					if (dsc == NULL) m[i+1].type = NM_TYPE_MENU;
					// Read thumbnail
					sc_bmp[i] = gr_create_bitmap(THUMBNAIL_W,THUMBNAIL_H );
					PHYSFS_read(fp, sc_bmp[i]->bm_data, THUMBNAIL_W * THUMBNAIL_H, 1);
					nsaves++;
					valid = 1;
				}
			}
			PHYSFS_close(fp);
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
		sprintf( fname, filename[choice-1] );
		if ( dsc != NULL ) strcpy( dsc, desc[choice-1] );
		state_default_item = choice - 1;
		return choice;
	}
	return 0;
}

int state_get_save_file(char * fname, char * dsc )
{
	return state_get_savegame_filename(fname, dsc, "Save Game");
}

int state_get_restore_file(char * fname )
{
	return state_get_savegame_filename(fname, NULL, "Select Game to Restore");
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
	PHYSFS_file * fp;
	ubyte *pal;
#ifdef OGL
	int j;
	GLint gl_draw_buffer;
#endif

	sprintf( filename, (GameArg.SysUsePlayersDir?"Players/%s.sg%d":"%s.sg%d"), sg_player->callsign, slotnum );
	fp = PHYSFSX_openWriteBuffered(filename);
	if ( !fp ) return 0;

//Save id
	PHYSFS_write(fp, dgss_id, sizeof(char) * 4, 1);

//Save version
	temp_int = STATE_VERSION;
	PHYSFS_write(fp, &temp_int, sizeof(int), 1);

//Save description
	strncpy( desc, sg_name, DESC_LENGTH );
	PHYSFS_write(fp, desc, sizeof(char) * DESC_LENGTH, 1);
	
// Save the current screen shot...
	cnv = gr_create_canvas( THUMBNAIL_W, THUMBNAIL_H );
	if ( cnv )
	{
#ifdef OGL
		ubyte *buf;
		int k;
#endif
		grs_canvas * cnv_save;
		cnv_save = grd_curcanv;

		gr_set_current_canvas( cnv );

		render_frame(0);

#ifdef OGL
		buf = d_malloc(THUMBNAIL_W * THUMBNAIL_H * 3);
		glGetIntegerv(GL_DRAW_BUFFER, &gl_draw_buffer);
		glReadBuffer(gl_draw_buffer);
		glReadPixels(0, SHEIGHT - THUMBNAIL_H, THUMBNAIL_W, THUMBNAIL_H, GL_RGB, GL_UNSIGNED_BYTE, buf);
		k = THUMBNAIL_H;
		for (i = 0; i < THUMBNAIL_W * THUMBNAIL_H; i++) {
			if (!(j = i % THUMBNAIL_W))
				k--;
			cnv->cv_bitmap.bm_data[THUMBNAIL_W * k + j] =
				gr_find_closest_color(buf[3*i]/4, buf[3*i+1]/4, buf[3*i+2]/4);
		}
		d_free(buf);
#endif
		pal = gr_palette;

		PHYSFS_write(fp, cnv->cv_bitmap.bm_data, THUMBNAIL_W * THUMBNAIL_H, 1);

		gr_set_current_canvas(cnv_save);
		gr_free_canvas( cnv );
	}
	else
	{
	 	ubyte color = 0;
	 	for ( i=0; i<THUMBNAIL_W*THUMBNAIL_H; i++ )
			PHYSFS_write(fp, &color, sizeof(ubyte), 1);		
	}

// Save the Between levels flag...
	temp_int = 1;
	PHYSFS_write(fp, &temp_int, sizeof(int), 1);

// Save the mission info...
#ifndef SHAREWARE
	PHYSFS_write(fp, Current_mission_filename, 9 * sizeof(char), 1);
#else
	PHYSFS_write(fp, "\0\0\0\0\0\0\0\0", 9 * sizeof(char), 1);
#endif

//Save level info
	temp_int = sg_player->level;
	PHYSFS_write(fp, &temp_int, sizeof(int), 1);
	temp_int = sg_next_level_num;
	PHYSFS_write(fp, &temp_int, sizeof(int), 1);

//Save GameTime
	temp_int = 0;
	PHYSFS_write(fp, &temp_int, sizeof(fix), 1);

//Save player info
	PHYSFS_write(fp, &sg_player, sizeof(player), 1);

// Save the current weapon info
	temp_byte = sg_primary_weapon;
	PHYSFS_write(fp, &temp_byte, sizeof(sbyte), 1);
	temp_byte = sg_secondary_weapon;
	PHYSFS_write(fp, &temp_byte, sizeof(sbyte), 1);

// Save the difficulty level
	temp_int = sg_difficulty_level;
	PHYSFS_write(fp, &temp_int, sizeof(int), 1);

// Save the Cheats_enabled
	temp_int = 0;
	PHYSFS_write(fp, &temp_int, sizeof(int), 1);
	temp_int = 0;		// turbo mode
	PHYSFS_write(fp, &temp_int, sizeof(int), 1);

	PHYSFS_write( fp, &state_game_id, sizeof(uint), 1 );
	PHYSFS_write( fp, &Laser_rapid_fire, sizeof(int), 1 );
	PHYSFS_write( fp, &Ugly_robot_cheat, sizeof(int), 1 );
	PHYSFS_write( fp, &Ugly_robot_texture, sizeof(int), 1 );
	PHYSFS_write( fp, &Physics_cheat_flag, sizeof(int), 1 );
	PHYSFS_write( fp, &Lunacy, sizeof(int), 1 );

	PHYSFS_close(fp);

	return 1;
}


int state_save_all(int between_levels)
{
	int rval;
	char filename[128], desc[DESC_LENGTH+1];

#ifndef SHAREWARE
	if ( Game_mode & GM_MULTI )	{
		return 0;
	}
#endif

	stop_time();

	if (!state_get_save_file(filename,desc))	{
		start_time();
		return 0;
	}

	rval = state_save_all_sub(filename, desc, between_levels);

	if (rval)
		HUD_init_message("Game saved.");

	return rval;
}

int state_save_all_sub(char *filename, char *desc, int between_levels)
{
	int i,j;
	PHYSFS_file * fp;
	grs_canvas * cnv;
	ubyte *pal;
#ifdef OGL
	GLint gl_draw_buffer;
#endif

	fp = PHYSFSX_openWriteBuffered(filename);
	if ( !fp ) {
		start_time();
		return 0;
	}

//Save id
	PHYSFS_write( fp, dgss_id, sizeof(char)*4, 1 );

//Save version
	i = STATE_VERSION;
	PHYSFS_write( fp, &i, sizeof(int), 1 );

//Save description
	PHYSFS_write( fp, desc, sizeof(char)*DESC_LENGTH, 1 );
	
// Save the current screen shot...
	cnv = gr_create_canvas( THUMBNAIL_W, THUMBNAIL_H );
	if ( cnv )
	{
#ifdef OGL
		ubyte *buf;
		int k;
#endif
		grs_canvas * cnv_save;
		cnv_save = grd_curcanv;

		gr_set_current_canvas( cnv );

		render_frame(0);

#ifdef OGL
		buf = d_malloc(THUMBNAIL_W * THUMBNAIL_H * 3);
		glGetIntegerv(GL_DRAW_BUFFER, &gl_draw_buffer);
		glReadBuffer(gl_draw_buffer);
		glReadPixels(0, SHEIGHT - THUMBNAIL_H, THUMBNAIL_W, THUMBNAIL_H, GL_RGB, GL_UNSIGNED_BYTE, buf);
		k = THUMBNAIL_H;
		for (i = 0; i < THUMBNAIL_W * THUMBNAIL_H; i++) {
			if (!(j = i % THUMBNAIL_W))
				k--;
			cnv->cv_bitmap.bm_data[THUMBNAIL_W * k + j] =
				gr_find_closest_color(buf[3*i]/4, buf[3*i+1]/4, buf[3*i+2]/4);
		}
		d_free(buf);
#endif
		pal = gr_palette;

		PHYSFS_write( fp,cnv->cv_bitmap.bm_data, THUMBNAIL_W * THUMBNAIL_H, 1);

		gr_set_current_canvas(cnv_save);
		gr_free_canvas( cnv );
	}
	else
	{
	 	ubyte color = 0;
	 	for ( i=0; i<THUMBNAIL_W*THUMBNAIL_H; i++ )
			PHYSFS_write( fp,&color, sizeof(ubyte), 1);		
	}

// Save the Between levels flag...
	PHYSFS_write( fp, &between_levels, sizeof(int), 1 );

// Save the mission info...
#ifndef SHAREWARE
	PHYSFS_write( fp, Current_mission_filename, sizeof(char)*9, 1 );
#else
	PHYSFS_write( fp, "\0\0\0\0\0\0\0\0", sizeof(char)*9, 1 );
#endif

//Save level info
	PHYSFS_write( fp, &Current_level_num, sizeof(int), 1 );
	PHYSFS_write( fp, &Next_level_num, sizeof(int), 1 );

//Save GameTime
	PHYSFS_write( fp, &GameTime, sizeof(fix), 1 );

//Save player info
	PHYSFS_write( fp, &Players[Player_num], sizeof(player), 1 );

// Save the current weapon info
	PHYSFS_write( fp, &Primary_weapon, sizeof(sbyte), 1 );
	PHYSFS_write( fp, &Secondary_weapon, sizeof(sbyte), 1 );

// Save the difficulty level
	PHYSFS_write( fp, &Difficulty_level, sizeof(int), 1 );

// Save the Cheats_enabled
	PHYSFS_write( fp, &Cheats_enabled, sizeof(int), 1 );
	PHYSFS_write( fp, &Game_turbo_mode, sizeof(int), 1 );


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
		PHYSFS_write( fp, &i, sizeof(int), 1 );
		PHYSFS_write( fp, Objects, sizeof(object)*i, 1 );
		
	//Save wall info
		i = Num_walls;
		PHYSFS_write( fp, &i, sizeof(int), 1 );
		PHYSFS_write( fp, Walls, sizeof(wall), i );
	
	//Save door info
		i = Num_open_doors;
		PHYSFS_write( fp, &i, sizeof(int), 1 );
		PHYSFS_write( fp, ActiveDoors, sizeof(active_door), i );
	
	//Save trigger info
		PHYSFS_write( fp, &Num_triggers, sizeof(int), 1 );
		PHYSFS_write( fp, Triggers, sizeof(trigger), Num_triggers );
	
	//Save tmap info
		for (i=0; i<=Highest_segment_index; i++ )	{
			for (j=0; j<6; j++ )	{
				PHYSFS_write( fp, &Segments[i].sides[j].wall_num, sizeof(short), 1 );
				PHYSFS_write( fp, &Segments[i].sides[j].tmap_num, sizeof(short), 1 );
				PHYSFS_write( fp, &Segments[i].sides[j].tmap_num2, sizeof(short), 1 );
			}
		}
	
	// Save the fuelcen info
		PHYSFS_write( fp, &Fuelcen_control_center_destroyed, sizeof(int), 1 );
		PHYSFS_write( fp, &Fuelcen_seconds_left, sizeof(int), 1 );
		PHYSFS_write( fp, &Num_robot_centers, sizeof(int), 1 );
		PHYSFS_write( fp, RobotCenters, sizeof(matcen_info), Num_robot_centers );
		PHYSFS_write( fp, &ControlCenterTriggers, sizeof(control_center_triggers), 1 );
		PHYSFS_write( fp, &Num_fuelcenters, sizeof(int), 1 );
		PHYSFS_write( fp, Station, sizeof(FuelCenter), Num_fuelcenters );
	
	// Save the control cen info
		PHYSFS_write( fp, &Control_center_been_hit, sizeof(int), 1 );
		PHYSFS_write( fp, &Control_center_player_been_seen, sizeof(int), 1 );
		PHYSFS_write( fp, &Control_center_next_fire_time, sizeof(int), 1 );
		PHYSFS_write( fp, &Control_center_present, sizeof(int), 1 );
		PHYSFS_write( fp, &Dead_controlcen_object_num, sizeof(int), 1 );
	
	// Save the AI state
		ai_save_state( fp );
	
	// Save the automap visited info
		PHYSFS_write( fp, Automap_visited, sizeof(ubyte)*MAX_SEGMENTS, 1 );
	}
	PHYSFS_write( fp, &state_game_id, sizeof(uint), 1 );
	PHYSFS_write( fp, &Laser_rapid_fire, sizeof(int), 1 );
	PHYSFS_write( fp, &Ugly_robot_cheat, sizeof(int), 1 );
	PHYSFS_write( fp, &Ugly_robot_texture, sizeof(int), 1 );
	PHYSFS_write( fp, &Physics_cheat_flag, sizeof(int), 1 );
	PHYSFS_write( fp, &Lunacy, sizeof(int), 1 );

	PHYSFS_close(fp);

	start_time();

	return 1;
}

int state_restore_all(int in_game)
{
	char filename[128];

#ifndef SHAREWARE
	if ( Game_mode & GM_MULTI )	{
		return 0;
	}
#endif

	if ( Newdemo_state == ND_STATE_RECORDING )
		newdemo_stop_recording();

	if ( Newdemo_state != ND_STATE_NORMAL )
		return 0;

	stop_time();
	if (!state_get_restore_file(filename))	{
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

	return state_restore_all_sub(filename);
}

int state_restore_all_sub(char *filename)
{
	int ObjectStartLocation;
	int BogusSaturnShit = 0;
	int version,i, j, segnum;
	object * obj;
	PHYSFS_file *fp;
	int current_level, next_level;
	int between_levels;
	char mission[16];
	char desc[DESC_LENGTH+1];
	char id[5];
	char org_callsign[CALLSIGN_LEN+16];

	fp = PHYSFSX_openReadBuffered(filename);
	if ( !fp ) return 0;

//Read id
	PHYSFS_read(fp, id, sizeof(char)*4, 1 );
	if ( memcmp( id, dgss_id, 4 )) {
		PHYSFS_close(fp);
		return 0;
	}

//Read version
	PHYSFS_read(fp, &version, sizeof(int), 1 );
	if (version < STATE_COMPATIBLE_VERSION)	{
		PHYSFS_close(fp);
		return 0;
	}

// Read description
	PHYSFS_read(fp, desc, sizeof(char)*DESC_LENGTH, 1 );

// Skip the current screen shot...
	PHYSFS_seek(fp, PHYSFS_tell(fp) + THUMBNAIL_W * THUMBNAIL_H);

// Read the Between levels flag...
	PHYSFS_read(fp, &between_levels, sizeof(int), 1);

// Read the mission info...
	PHYSFS_read(fp, mission, sizeof(char)*9, 1);

#ifndef SHAREWARE
	if (!load_mission_by_name( mission ))	{
		nm_messagebox( NULL, 1, "Ok", "Error!\nUnable to load mission\n'%s'\n", mission );
		PHYSFS_close(fp);
		return 0;
	}
#else
	if (mission[0]) {
		nm_messagebox( NULL, 1, "Ok", "Error!\nCannot load mission '%s'\nThe shareware version only supports savegames of the shareware mission!", mission );
		PHYSFS_close(fp);
		return 0;
	}
#endif

//Read level info
	PHYSFS_read(fp, &current_level, sizeof(int), 1);
	PHYSFS_read(fp, &next_level, sizeof(int), 1);
#ifdef SHAREWARE
	if (current_level < 1 || current_level > Last_level ||
		next_level < 0 || next_level > Last_level) {
		nm_messagebox( NULL, 1, "Ok", "Error!\nCannot load level %d\nThe shareware version only supports savegames of shareware levels!",
			between_levels? next_level:current_level);
		PHYSFS_close(fp);
		return 0;
	}
#endif

//Restore GameTime
	PHYSFS_read(fp, &GameTime, sizeof(fix), 1);

// Start new game....
	Game_mode = GM_NORMAL;
	Function_mode = FMODE_GAME;
#ifdef NETWORK
	change_playernum_to(0);
#endif
	strcpy( org_callsign, Players[0].callsign );
	N_players = 1;
	InitPlayerObject();				//make sure player's object set up
	init_player_stats_game();		//clear all stats

//Read player info

	if ( between_levels )	{
		int saved_offset;
		PHYSFS_read(fp, &Players[Player_num], sizeof(player), 1);
		saved_offset = PHYSFS_tell(fp);
		PHYSFS_close( fp );
		do_briefing_screens(next_level);
		fp = PHYSFSX_openReadBuffered(filename);
		PHYSFS_seek(fp, saved_offset);
 		StartNewLevelSub( next_level, 1);//use page_in_textures here to fix OGL texture precashing crash -MPM
	} else {
		StartNewLevelSub(current_level, 1);//use page_in_textures here to fix OGL texture precashing crash -MPM
		PHYSFS_read(fp, &Players[Player_num], sizeof(player), 1);
	}
	strcpy( Players[Player_num].callsign, org_callsign );

// Set the right level
	if ( between_levels )
		Players[Player_num].level = next_level;

// Restore the weapon states
	PHYSFS_read(fp, &Primary_weapon, sizeof(sbyte), 1);
	PHYSFS_read(fp, &Secondary_weapon, sizeof(sbyte), 1);

	select_weapon(Primary_weapon, 0, 0, 0);
	select_weapon(Secondary_weapon, 1, 0, 0);

// Restore the difficulty level
	PHYSFS_read(fp, &Difficulty_level, sizeof(int), 1);

// Restore the cheats enabled flag
	PHYSFS_read(fp, &Cheats_enabled, sizeof(int), 1);
	PHYSFS_read(fp, &Game_turbo_mode, sizeof(int), 1);

	if ( !between_levels )	{
		Do_appearance_effect = 0;			// Don't do this for middle o' game stuff.

		ObjectStartLocation = PHYSFS_tell( fp );
RetryObjectLoading:
		//Clear out all the objects from the lvl file
		for (segnum=0; segnum <= Highest_segment_index; segnum++)
			Segments[segnum].objects = -1;
		reset_objects(1);
	
		//Read objects, and pop 'em into their respective segments.
		PHYSFS_read(fp, &i, sizeof(int), 1);
		Highest_object_index = i-1;
		if ( !BogusSaturnShit )	
			PHYSFS_read(fp, Objects, sizeof(object)*i, 1);
		else {
			ubyte tmp_object[sizeof(object)];
			for (i=0; i<=Highest_object_index; i++ )	{
				PHYSFS_read(fp, tmp_object, sizeof(object)-3, 1);
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
						PHYSFS_seek( fp, ObjectStartLocation );
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
		PHYSFS_read(fp, &i, sizeof(int), 1);
		Num_walls = i;
		// Check for a bogus Saturn version!!!!
		if (!BogusSaturnShit )	{
			if ( (Num_walls<0) || (Num_walls>MAX_WALLS) ) {
				BogusSaturnShit = 1;
				PHYSFS_seek( fp, ObjectStartLocation );
				goto RetryObjectLoading;
			}
		}

		PHYSFS_read(fp, Walls, sizeof(wall), Num_walls);
		// Check for a bogus Saturn version!!!!
		if (!BogusSaturnShit )	{
			for (i=0; i<Num_walls; i++ )	{
				if ( (Walls[i].segnum<0) || (Walls[i].segnum>Highest_segment_index) || (Walls[i].sidenum<-1) || (Walls[i].sidenum>5) ) {
					BogusSaturnShit = 1;
					PHYSFS_seek( fp, ObjectStartLocation );
					goto RetryObjectLoading;
				}
			}
		}
	
		//Restore door info
		PHYSFS_read(fp, &i, sizeof(int), 1);
		Num_open_doors = i;
		PHYSFS_read(fp, ActiveDoors, sizeof(active_door), Num_open_doors);
	
		//Restore trigger info
		PHYSFS_read(fp, &Num_triggers, sizeof(int), 1);
		PHYSFS_read(fp, Triggers, sizeof(trigger), Num_triggers);
	
		//Restore tmap info
		for (i=0; i<=Highest_segment_index; i++ )	{
			for (j=0; j<6; j++ )	{
				PHYSFS_read(fp, &Segments[i].sides[j].wall_num, sizeof(short), 1);
				PHYSFS_read(fp, &Segments[i].sides[j].tmap_num, sizeof(short), 1);
				PHYSFS_read(fp, &Segments[i].sides[j].tmap_num2, sizeof(short), 1);
			}
		}
	
		//Restore the fuelcen info
		PHYSFS_read(fp, &Fuelcen_control_center_destroyed, sizeof(int), 1);
		PHYSFS_read(fp, &Fuelcen_seconds_left, sizeof(int), 1);
		PHYSFS_read(fp, &Num_robot_centers, sizeof(int), 1);
		PHYSFS_read(fp, RobotCenters, sizeof(matcen_info), Num_robot_centers);
		PHYSFS_read(fp, &ControlCenterTriggers, sizeof(control_center_triggers), 1);
		PHYSFS_read(fp, &Num_fuelcenters, sizeof(int), 1);
		PHYSFS_read(fp, Station, sizeof(FuelCenter), Num_fuelcenters);
	
		// Restore the control cen info
		PHYSFS_read(fp, &Control_center_been_hit, sizeof(int), 1);
		PHYSFS_read(fp, &Control_center_player_been_seen, sizeof(int), 1);
		PHYSFS_read(fp, &Control_center_next_fire_time, sizeof(int), 1);
		PHYSFS_read(fp, &Control_center_present, sizeof(int), 1);
		PHYSFS_read(fp, &Dead_controlcen_object_num, sizeof(int), 1);
	
		// Restore the AI state
		ai_restore_state( fp );
	
		// Restore the automap visited info
		PHYSFS_read(fp, Automap_visited, sizeof(ubyte), MAX_SEGMENTS);

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
		PHYSFS_read(fp, &state_game_id, sizeof(uint), 1);
		PHYSFS_read(fp, &Laser_rapid_fire, sizeof(int), 1);
		PHYSFS_read(fp, &Ugly_robot_cheat, sizeof(int), 1);
		PHYSFS_read(fp, &Ugly_robot_texture, sizeof(int), 1);
		PHYSFS_read(fp, &Physics_cheat_flag, sizeof(int), 1);
		PHYSFS_read(fp, &tmp_Lunacy, sizeof(int), 1);
		if ( tmp_Lunacy )
			do_lunacy_on();
	}

	PHYSFS_close(fp);

// Load in bitmaps, etc..
//	piggy_load_level_data();//already used page_in_textures in StartNewLevelSub, so no need for this here. -MPM

	return 1;
}
