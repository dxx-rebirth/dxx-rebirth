/* $Id: state.c,v 1.21 2005-01-23 14:38:04 schaffner Exp $ */
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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Functions to save/restore game state.
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef WINDOWS
#include "desw.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#if !defined(_MSC_VER) && !defined(macintosh)
#include <unistd.h>
#endif
#ifndef _WIN32_WCE
#include <errno.h>
#endif
#ifdef MACINTOSH
#include <Files.h>
#endif

#ifdef OGL
# ifdef _MSC_VER
#  include <windows.h>
# endif
#if defined(__APPLE__) && defined(__MACH__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#endif

#include "pstypes.h"
#include "pa_enabl.h"                   //$$POLY_ACC
#include "mono.h"
#include "inferno.h"
#include "segment.h"
#include "textures.h"
#include "wall.h"
#include "object.h"
#include "digi.h"
#include "gamemine.h"
#include "error.h"
#include "gameseg.h"
#include "menu.h"
#include "switch.h"
#include "game.h"
#include "screens.h"
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
#ifdef NETWORK
#include "network.h"
#endif
#include "args.h"
#include "ai.h"
#include "fireball.h"
#include "controls.h"
#include "laser.h"
#include "multibot.h"
#include "state.h"

#if defined(POLY_ACC)
#include "poly_acc.h"
#endif

#ifdef OGL
#include "gr.h"
#endif
#include "physfsx.h"

#define STATE_VERSION 22
#define STATE_COMPATIBLE_VERSION 20
// 0 - Put DGSS (Descent Game State Save) id at tof.
// 1 - Added Difficulty level save
// 2 - Added Cheats_enabled flag
// 3 - Added between levels save.
// 4 - Added mission support
// 5 - Mike changed ai and object structure.
// 6 - Added buggin' cheat save
// 7 - Added other cheat saves and game_id.
// 8 - Added AI stuff for escort and thief.
// 9 - Save palette with screen shot
// 12- Saved last_was_super array
// 13- Saved palette flash stuff
// 14- Save cloaking wall stuff
// 15- Save additional ai info
// 16- Save Light_subtracted
// 17- New marker save
// 18- Took out saving of old cheat status
// 19- Saved cheats_enabled flag
// 20- First_secret_visit
// 22- Omega_charge

#define NUM_SAVES 9
#define THUMBNAIL_W 100
#define THUMBNAIL_H 50
#define DESC_LENGTH 20

extern void multi_initiate_save_game();
extern void multi_initiate_restore_game();
extern void apply_all_changed_light(void);

extern int Do_appearance_effect;
extern fix Fusion_next_sound_time;

extern int Laser_rapid_fire;
extern int Physics_cheat_flag;
extern int Lunacy;
extern void do_lunacy_on(void);
extern void do_lunacy_off(void);
extern int First_secret_visit;

int sc_last_item= 0;
grs_bitmap *sc_bmp[NUM_SAVES+1];

char dgss_id[4] = "DGSS";

int state_default_item = 0;

uint state_game_id;

extern int robot_controlled[MAX_ROBOTS_CONTROLLED];
extern int robot_agitation[MAX_ROBOTS_CONTROLLED];
extern fix robot_controlled_time[MAX_ROBOTS_CONTROLLED];
extern fix robot_last_send_time[MAX_ROBOTS_CONTROLLED];
extern fix robot_last_message_time[MAX_ROBOTS_CONTROLLED];
extern int robot_send_pending[MAX_ROBOTS_CONTROLLED];
extern int robot_fired[MAX_ROBOTS_CONTROLLED];
extern sbyte robot_fire_buf[MAX_ROBOTS_CONTROLLED][18+3];


#if defined(WINDOWS) || defined(MACINTOSH)
extern ubyte Hack_DblClick_MenuMode;
#endif

void compute_all_static_light(void);

//-------------------------------------------------------------------
void state_callback(int nitems,newmenu_item * items, int * last_key, int citem)
{
	nitems = nitems;
	last_key = last_key;
	
//	if ( sc_last_item != citem )	{
//		sc_last_item = citem;
		if ( citem > 0 )	{
			if ( sc_bmp[citem-1] )	{
				if (MenuHires) {
				WINDOS(
					dd_grs_canvas *save_canv = dd_grd_curcanv,
					grs_canvas *save_canv = grd_curcanv
				);
					grs_canvas *temp_canv = gr_create_canvas(THUMBNAIL_W*2,(THUMBNAIL_H*24/10));
					grs_point vertbuf[3] = {{0,0}, {0,0}, {i2f(THUMBNAIL_W*2),i2f(THUMBNAIL_H*24/10)} };
					gr_set_current_canvas(temp_canv);
					scale_bitmap(sc_bmp[citem-1], vertbuf, 0 );
				WINDOS(
					dd_gr_set_current_canvas(save_canv),
					gr_set_current_canvas( save_canv )
				);
				WIN(DDGRLOCK(dd_grd_curcanv));
					gr_bitmap( (grd_curcanv->cv_bitmap.bm_w-THUMBNAIL_W*2)/2,items[0].y-10, &temp_canv->cv_bitmap);
				WIN(DDGRUNLOCK(dd_grd_curcanv));
					gr_free_canvas(temp_canv);
				}
				else	{
				#ifdef WINDOWS
					Int3();
				#else
					gr_bitmap( (grd_curcanv->cv_bitmap.bm_w-THUMBNAIL_W)/2,items[0].y-5, sc_bmp[citem-1] );
				#endif
				}
			}
		}
//	}	
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

int state_get_save_file(char * fname, char * dsc, int multi, int blind_save)
{
	PHYSFS_file *fp;
	int i, choice, version;
	newmenu_item m[NUM_SAVES+2];
	char filename[NUM_SAVES+1][30];
	char desc[NUM_SAVES+1][DESC_LENGTH+16];
	char id[5];
	int valid=0;
	
	for (i=0;i<NUM_SAVES; i++ )	{
		sc_bmp[i] = NULL;
		if ( !multi )
			#ifndef MACINTOSH
			sprintf( filename[i], "%s.sg%x", Players[Player_num].callsign, i );
			#else
			sprintf( filename[i], ":Players:%s.sg%x", Players[Player_num].callsign, i );
			#endif
		else
			#ifndef MACINTOSH
			sprintf( filename[i], "%s.mg%x", Players[Player_num].callsign, i );
			#else
			sprintf( filename[i], ":Players:%s.mg%x", Players[Player_num].callsign, i );
			#endif
		valid = 0;
		fp = PHYSFS_openRead(filename[i]);
		if ( fp ) {
			//Read id
			//FIXME: check for swapped file, react accordingly...
			PHYSFS_read(fp, id, sizeof(char) * 4, 1);
			if ( !memcmp( id, dgss_id, 4 )) {
				//Read version
				PHYSFS_read(fp, &version, sizeof(int), 1);
				if (version >= STATE_COMPATIBLE_VERSION)	{
					// Read description
					PHYSFS_read(fp, desc[i], sizeof(char) * DESC_LENGTH, 1);
					//rpad_string( desc[i], DESC_LENGTH-1 );
					// Read thumbnail
					//sc_bmp[i] = gr_create_bitmap(THUMBNAIL_W,THUMBNAIL_H );
					//PHYSFS_read(fp, sc_bmp[i]->bm_data, THUMBNAIL_W * THUMBNAIL_H, 1);
					valid = 1;
				}
			} 
			PHYSFS_close(fp);
		}
		if (!valid) {
			strcpy( desc[i], TXT_EMPTY );
			//rpad_string( desc[i], DESC_LENGTH-1 );
		}
		m[i].type = NM_TYPE_INPUT_MENU; m[i].text = desc[i]; m[i].text_len = DESC_LENGTH-1;
	}

	sc_last_item = -1;
	if (blind_save && state_default_item >= 0)
		choice = state_default_item;
	else
		choice = newmenu_do1(NULL, "Save Game", NUM_SAVES, m, NULL, state_default_item);

	for (i=0; i<NUM_SAVES; i++ )	{
		if ( sc_bmp[i] )
			gr_free_bitmap( sc_bmp[i] );
	}

	if (choice > -1) {
		strcpy( fname, filename[choice] );
		strcpy( dsc, desc[choice] );
		state_default_item = choice;
		return choice+1;
	}
	return 0;
}

int RestoringMenu=0;
extern int Current_display_mode;

int state_get_restore_file(char * fname, int multi)
{
	PHYSFS_file *fp;
	int i, choice, version, nsaves;
	newmenu_item m[NUM_SAVES+2];
	char filename[NUM_SAVES+1][30];
	char desc[NUM_SAVES+1][DESC_LENGTH + 16];
	char id[5];
	int valid;

	nsaves=0;
	m[0].type = NM_TYPE_TEXT; m[0].text = "\n\n\n\n";	
	for (i=0;i<NUM_SAVES+1; i++ )	{
		sc_bmp[i] = NULL;
		if (!multi)
			#ifndef MACINTOSH
			sprintf( filename[i], "%s.sg%x", Players[Player_num].callsign, i );
			#else
			sprintf( filename[i], ":Players:%s.sg%x", Players[Player_num].callsign, i );
			#endif
		else
			#ifndef MACINTOSH
			sprintf( filename[i], "%s.mg%x", Players[Player_num].callsign, i );
			#else
			sprintf( filename[i], ":Players:%s.mg%x", Players[Player_num].callsign, i );
			#endif
		valid = 0;
		fp = PHYSFS_openRead(filename[i]);
		if ( fp ) {
			//Read id
			//FIXME: check for swapped file, react accordingly...
			PHYSFS_read(fp, id, sizeof(char) * 4, 1);
			if ( !memcmp( id, dgss_id, 4 )) {
				//Read version
				PHYSFS_read(fp, &version, sizeof(int), 1);
				if (version >= STATE_COMPATIBLE_VERSION)	{
					// Read description
					PHYSFS_read(fp, desc[i], sizeof(char) * DESC_LENGTH, 1);
					//rpad_string( desc[i], DESC_LENGTH-1 );
					m[i+1].type = NM_TYPE_MENU; m[i+1].text = desc[i];
					// Read thumbnail
					sc_bmp[i] = gr_create_bitmap(THUMBNAIL_W,THUMBNAIL_H );
					PHYSFS_read(fp, sc_bmp[i]->bm_data, THUMBNAIL_W * THUMBNAIL_H, 1);
					if (version >= 9) {
						ubyte pal[256*3];
						PHYSFS_read(fp, pal, 3, 256);
						gr_remap_bitmap_good( sc_bmp[i], pal, -1, -1 );
					}
					nsaves++;
					valid = 1;
				}
			}
			PHYSFS_close(fp);
		}
		if (!valid) {
			strcpy( desc[i], TXT_EMPTY );
			//rpad_string( desc[i], DESC_LENGTH-1 );
			m[i+1].type = NM_TYPE_TEXT; m[i+1].text = desc[i];
		}
	}

	if ( nsaves < 1 )	{
		nm_messagebox( NULL, 1, "Ok", "No saved games were found!" );
		return 0;
	}

	if (Current_display_mode == 3)	//restore menu won't fit on 640x400
		VR_screen_flags ^= VRF_COMPATIBLE_MENUS;

	sc_last_item = -1;

#if defined(WINDOWS) || defined(MACINTOSH)
	Hack_DblClick_MenuMode = 1;
#endif

   RestoringMenu=1;
	choice = newmenu_do3( NULL, "Select Game to Restore", NUM_SAVES+2, m, state_callback, state_default_item+1, NULL, 190, -1 );
   RestoringMenu=0;

#if defined(WINDOWS) || defined(MACINTOSH)
	Hack_DblClick_MenuMode = 0;
#endif

	if (Current_display_mode == 3)	//set flag back
		VR_screen_flags ^= VRF_COMPATIBLE_MENUS;


	for (i=0; i<NUM_SAVES+1; i++ )	{
		if ( sc_bmp[i] )
			gr_free_bitmap( sc_bmp[i] );
	}

	if (choice > 0) {
		strcpy( fname, filename[choice-1] );
		if (choice != NUM_SAVES+1)		//no new default when restore from autosave
			state_default_item = choice - 1;
		return choice;
	}
	return 0;
}

#define	DESC_OFFSET	8

#define	CF_BUF_SIZE	1024

//	-----------------------------------------------------------------------------------
//	Imagine if C had a function to copy a file...
int copy_file(char *old_file, char *new_file)
{
	sbyte	buf[CF_BUF_SIZE];
	PHYSFS_file *in_file, *out_file;

	out_file = PHYSFS_openWrite(new_file);

	if (out_file == NULL)
		return -1;

	in_file = PHYSFS_openRead(old_file);

	if (in_file == NULL)
		return -2;

	while (!PHYSFS_eof(in_file))
	{
		int bytes_read;

		bytes_read = PHYSFS_read(in_file, buf, 1, CF_BUF_SIZE);
		if (bytes_read < 0)
			Error("Cannot read from file <%s>: %s", old_file, PHYSFS_getLastError());

		Assert(bytes_read == CF_BUF_SIZE || PHYSFS_eof(in_file));

		if (PHYSFS_write(out_file, buf, 1, bytes_read) < bytes_read)
			Error("Cannot write to file <%s>: %s", new_file, PHYSFS_getLastError());
	}

	if (!PHYSFS_close(in_file))
	{
		PHYSFS_close(out_file);
		return -3;
	}

	if (!PHYSFS_close(out_file))
		return -4;

	return 0;
}

#ifndef MACINTOSH
#define SECRETB_FILENAME	"secret.sgb"
#define SECRETC_FILENAME	"secret.sgc"
#else
#define SECRETB_FILENAME	":Players:secret.sgb"
#define SECRETC_FILENAME	":Players:secret.sgc"
#endif

extern int Final_boss_is_dead;

//	-----------------------------------------------------------------------------------
//	blind_save means don't prompt user for any info.
int state_save_all(int between_levels, int secret_save, char *filename_override, int blind_save)
{
	int	rval, filenum = -1;

	char	filename[128], desc[DESC_LENGTH+1];

	Assert(between_levels == 0);	//between levels save ripped out

#ifdef NETWORK
	if ( Game_mode & GM_MULTI )	{
			multi_initiate_save_game();
		return 0;
	}
#endif

	if ((Current_level_num < 0) && (secret_save == 0)) {
		HUD_init_message( "Can't save in secret level!" );
		return 0;
	}

	if (Final_boss_is_dead)		//don't allow save while final boss is dying
		return 0;

	mprintf(( 0, "CL=%d, NL=%d\n", Current_level_num, Next_level_num ));
	
	//	If this is a secret save and the control center has been destroyed, don't allow
	//	return to the base level.
	if (secret_save && (Control_center_destroyed)) {
		mprintf((0, "Deleting secret.sgb so player can't return to base level.\n"));
		PHYSFS_delete(SECRETB_FILENAME);
		return 0;
	}

	stop_time();

	if (secret_save == 1) {
		filename_override = filename;
		sprintf(filename_override, SECRETB_FILENAME);
	} else if (secret_save == 2) {
		filename_override = filename;
		sprintf(filename_override, SECRETC_FILENAME);
	} else {
		if (filename_override) {
			strcpy( filename, filename_override);
			sprintf(desc, "[autosave backup]");
		}
		else if (!(filenum = state_get_save_file(filename, desc, 0, blind_save)))
		{
			start_time();
			return 0;
		}
	}
		
	//	MK, 1/1/96
	//	If not in multiplayer, do special secret level stuff.
	//	If secret.sgc exists, then copy it to Nsecret.sgc (where N = filenum).
	//	If it doesn't exist, then delete Nsecret.sgc
	if (!secret_save && !(Game_mode & GM_MULTI)) {
		int	rval;
		char	temp_fname[32], fc;

		if (filenum != -1) {

			if (filenum >= 10)
				fc = (filenum-10) + 'a';
			else
				fc = '0' + filenum;

			#ifndef MACINTOSH
			sprintf(temp_fname, "%csecret.sgc", fc);
			#else
			sprintf(temp_fname, ":Players:%csecret.sgc", fc);
			#endif

			mprintf((0, "Trying to copy secret.sgc to %s.\n", temp_fname));

			if (PHYSFS_exists(temp_fname))
			{
				mprintf((0, "Deleting file %s\n", temp_fname));
				if (!PHYSFS_delete(temp_fname))
					Error("Cannot delete file <%s>: %s", temp_fname, PHYSFS_getLastError());
			}

			if (PHYSFS_exists(SECRETC_FILENAME))
			{
				mprintf((0, "Copying secret.sgc to %s.\n", temp_fname));
				rval = copy_file(SECRETC_FILENAME, temp_fname);
				Assert(rval == 0);	//	Oops, error copying secret.sgc to temp_fname!
			}
		}
	}

	//	Save file we're going to save over in last slot and call "[autosave backup]"
	if (!filename_override) {
		PHYSFS_file *tfp;

		tfp = PHYSFS_openWrite(filename);

		if ( tfp ) {
			char	newname[128];

			#ifndef MACINTOSH
			sprintf( newname, "%s.sg%x", Players[Player_num].callsign, NUM_SAVES );
			#else
			sprintf(newname, "Players/%s.sg%x", Players[Player_num].callsign, NUM_SAVES);
			#endif

			PHYSFS_seek(tfp, DESC_OFFSET);
			PHYSFS_write(tfp, "[autosave backup]", sizeof(char) * DESC_LENGTH, 1);
			PHYSFS_close(tfp);
			PHYSFS_delete(newname);
			PHYSFSX_rename(filename, newname);
		}
	}
	
	rval = state_save_all_sub(filename, desc, between_levels);
	if (rval && !secret_save)
		HUD_init_message("Game saved.");

	return rval;
}

extern	fix	Flash_effect, Time_flash_last_played;


int state_save_all_sub(char *filename, char *desc, int between_levels)
{
	int i,j;
	PHYSFS_file *fp;
	grs_canvas * cnv;
	#ifdef POLY_ACC
	grs_canvas cnv2,*save_cnv2;
	#endif
	ubyte *pal;

	Assert(between_levels == 0);	//between levels save ripped out

/*	if ( Game_mode & GM_MULTI )	{
		{
		start_time();
		return 0;
		}
	}*/

	#if defined(MACINTOSH) && !defined(NDEBUG)
	if ( strncmp(filename, ":Players:", 9) )
		Int3();
	#endif

	fp = PHYSFS_openWrite(filename);
	if ( !fp ) {
		if ( !(Game_mode & GM_MULTI) )
			nm_messagebox(NULL, 1, TXT_OK, "Error writing savegame.\nPossibly out of disk\nspace.");
		start_time();
		return 0;
	}

//Save id
	PHYSFS_write(fp, dgss_id, sizeof(char) * 4, 1);

//Save version
	i = STATE_VERSION;
	PHYSFS_write(fp, &i, sizeof(int), 1);

//Save description
	PHYSFS_write(fp, desc, sizeof(char) * DESC_LENGTH, 1);
	
// Save the current screen shot...

	cnv = gr_create_canvas( THUMBNAIL_W, THUMBNAIL_H );
	if ( cnv )
	{
#ifdef OGL
		ubyte *buf;
		int k;
#endif
		#ifdef WINDOWS
			dd_grs_canvas *cnv_save;
			cnv_save = dd_grd_curcanv;
		#else
			grs_canvas * cnv_save;
			cnv_save = grd_curcanv;
		#endif

		#ifndef MACINTOSH
		
			#if defined(POLY_ACC)
			
			 		PA_DFX (pa_fool_to_backbuffer());
			
					//for poly_acc, we render the frame to the normal render buffer
					//so that this doesn't show, we create yet another canvas to save
					//and restore what's on the render buffer
					PA_DFX (pa_alpha_always());	
					PA_DFX (pa_set_front_to_read());
					gr_init_sub_canvas( &cnv2, &VR_render_buffer[0], 0, 0, THUMBNAIL_W, THUMBNAIL_H );
					save_cnv2 = gr_create_canvas2(THUMBNAIL_W, THUMBNAIL_H, cnv2.cv_bitmap.bm_type);
					gr_set_current_canvas( save_cnv2 );
					PA_DFX (pa_set_front_to_read());
					gr_bitmap(0,0,&cnv2.cv_bitmap);
					gr_set_current_canvas( &cnv2 );
			#else
					gr_set_current_canvas( cnv );
			#endif
			
					PA_DFX (pa_set_backbuffer_current());
					render_frame(0, 0);
					PA_DFX (pa_alpha_always());
			
#if defined(POLY_ACC)
					#ifndef MACINTOSH
					screen_shot_pa(cnv,&cnv2);
					#else
					if ( PAEnabled )
						screen_shot_pa(cnv,&cnv2);
					#endif
#elif defined(OGL)
# if 1
		buf = d_malloc(THUMBNAIL_W * THUMBNAIL_H * 3);
		glReadBuffer(GL_FRONT);
		glReadPixels(0, SHEIGHT - THUMBNAIL_H, THUMBNAIL_W, THUMBNAIL_H, GL_RGB, GL_UNSIGNED_BYTE, buf);
		k = THUMBNAIL_H;
		for (i = 0; i < THUMBNAIL_W * THUMBNAIL_H; i++) {
			if (!(j = i % THUMBNAIL_W))
				k--;
			cnv->cv_bitmap.bm_data[THUMBNAIL_W * k + j] =
				gr_find_closest_color(buf[3*i]/4, buf[3*i+1]/4, buf[3*i+2]/4);
		}
		d_free(buf);
# else // simpler d1x method, not tested yet
		ogl_ubitblt_tolinear(grd_curcanv->cv_bitmap.bm_w, grd_curcanv->cv_bitmap.bm_h, 0, 0, 0, 0, &grd_curscreen->sc_canvas.cv_bitmap, &grd_curcanv->cv_bitmap);
# endif
#endif

					pal = gr_palette;

					PHYSFS_write(fp, cnv->cv_bitmap.bm_data, THUMBNAIL_W * THUMBNAIL_H, 1);

			#if defined(POLY_ACC)
					PA_DFX (pa_alpha_always());	
					PA_DFX (pa_set_frontbuffer_current());
					PA_DFX(gr_bitmap(0,0,&save_cnv2->cv_bitmap));
					PA_DFX (pa_set_backbuffer_current());
					gr_bitmap(0,0,&save_cnv2->cv_bitmap);
					gr_free_canvas(save_cnv2);
			 		PA_DFX (pa_fool_to_offscreen());
			
			#endif
		
		#else 	// macintosh stuff below
		{
			#if defined(POLY_ACC)
				int	savePAEnabled = PAEnabled;
				PAEnabled = false;
			#endif

			gr_set_current_canvas( cnv );
			render_frame(0, 0);
			pal = gr_palette;
			PHYSFS_write(fp, cnv->cv_bitmap.bm_data, THUMBNAIL_W * THUMBNAIL_H, 1);
			
			#if defined(POLY_ACC)
				PAEnabled = savePAEnabled;
			#endif
		}	
		#endif	// end of ifndef macintosh
		
		
		WINDOS(
			dd_gr_set_current_canvas(cnv_save),
			gr_set_current_canvas(cnv_save)
		);
		gr_free_canvas( cnv );
		PHYSFS_write(fp, pal, 3, 256);
	}
	else
	{
	 	ubyte color = 0;
	 	for ( i=0; i<THUMBNAIL_W*THUMBNAIL_H; i++ )
			PHYSFS_write(fp, &color, sizeof(ubyte), 1);		
	} 

// Save the Between levels flag...
	PHYSFS_write(fp, &between_levels, sizeof(int), 1);

// Save the mission info...
        mprintf ((0, "HEY! Mission name is %s\n", Current_mission_filename));
	PHYSFS_write(fp, Current_mission_filename, 9 * sizeof(char), 1);

//Save level info
	PHYSFS_write(fp, &Current_level_num, sizeof(int), 1);
	PHYSFS_write(fp, &Next_level_num, sizeof(int), 1);

//Save GameTime
	PHYSFS_write(fp, &GameTime, sizeof(fix), 1);

// If coop save, save all
#ifdef NETWORK
   if (Game_mode & GM_MULTI_COOP)
	 {
		PHYSFS_write(fp, &state_game_id,sizeof(int), 1);
		PHYSFS_write(fp, &Netgame,sizeof(netgame_info), 1);
		PHYSFS_write(fp, &NetPlayers,sizeof(AllNetPlayers_info), 1);
		PHYSFS_write(fp, &N_players,sizeof(int), 1);
		PHYSFS_write(fp, &Player_num,sizeof(int), 1);
		for (i=0;i<N_players;i++)
			PHYSFS_write(fp, &Players[i], sizeof(player), 1);

#ifdef RISKY_PROPOSITION
		PHYSFS_write(fp, &robot_controlled[0], 4 * MAX_ROBOTS_CONTROLLED, 1);
		PHYSFS_write(fp, &robot_agitation[0], 4 * MAX_ROBOTS_CONTROLLED, 1);
		PHYSFS_write(fp, &robot_controlled_time[0], 4 * MAX_ROBOTS_CONTROLLED, 1);
		PHYSFS_write(fp, &robot_last_send_time[0], 4 * MAX_ROBOTS_CONTROLLED, 1);
		PHYSFS_write(fp, &robot_last_message_time[0], 4 * MAX_ROBOTS_CONTROLLED, 1);
		PHYSFS_write(fp, &robot_send_pending[0], 4 * MAX_ROBOTS_CONTROLLED, 1);
		PHYSFS_write(fp, &robot_fired[0], 4 * MAX_ROBOTS_CONTROLLED, 1);
 
      for (i=0;i<MAX_ROBOTS_CONTROLLED;i++)
			PHYSFS_write(fp, robot_fire_buf[i][0], 18 + 3, 1);
#endif

	 }
#endif

//Save player info
	PHYSFS_write(fp, &Players[Player_num], sizeof(player), 1);

// Save the current weapon info
	PHYSFS_write(fp, &Primary_weapon, sizeof(sbyte), 1);
	PHYSFS_write(fp, &Secondary_weapon, sizeof(sbyte), 1);

// Save the difficulty level
	PHYSFS_write(fp, &Difficulty_level, sizeof(int), 1);
// Save cheats enabled
	PHYSFS_write(fp, &Cheats_enabled,sizeof(int), 1);

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
		PHYSFS_write(fp, &i, sizeof(int), 1);
		PHYSFS_write(fp, Objects, sizeof(object), i);
		
	//Save wall info
		i = Num_walls;
		PHYSFS_write(fp, &i, sizeof(int), 1);
		PHYSFS_write(fp, Walls, sizeof(wall), i);

	//Save exploding wall info
		i = MAX_EXPLODING_WALLS;
		PHYSFS_write(fp, &i, sizeof(int), 1);
		PHYSFS_write(fp, expl_wall_list, sizeof(*expl_wall_list), i);
	
	//Save door info
		i = Num_open_doors;
		PHYSFS_write(fp, &i, sizeof(int), 1);
		PHYSFS_write(fp, ActiveDoors, sizeof(active_door), i);
	
	//Save cloaking wall info
		i = Num_cloaking_walls;
		PHYSFS_write(fp, &i, sizeof(int), 1);
		PHYSFS_write(fp, CloakingWalls, sizeof(cloaking_wall), i);
	
	//Save trigger info
		PHYSFS_write(fp, &Num_triggers, sizeof(int), 1);
		PHYSFS_write(fp, Triggers, sizeof(trigger), Num_triggers);
	
	//Save tmap info
		for (i = 0; i <= Highest_segment_index; i++)
		{
			for (j = 0; j < 6; j++)
			{
				PHYSFS_write(fp, &Segments[i].sides[j].wall_num, sizeof(short), 1);
				PHYSFS_write(fp, &Segments[i].sides[j].tmap_num, sizeof(short), 1);
				PHYSFS_write(fp, &Segments[i].sides[j].tmap_num2, sizeof(short), 1);
			}
		}
	
	// Save the fuelcen info
		PHYSFS_write(fp, &Control_center_destroyed, sizeof(int), 1);
		PHYSFS_write(fp, &Countdown_timer, sizeof(int), 1);
		PHYSFS_write(fp, &Num_robot_centers, sizeof(int), 1);
		PHYSFS_write(fp, RobotCenters, sizeof(matcen_info), Num_robot_centers);
		PHYSFS_write(fp, &ControlCenterTriggers, sizeof(control_center_triggers), 1);
		PHYSFS_write(fp, &Num_fuelcenters, sizeof(int), 1);
		PHYSFS_write(fp, Station, sizeof(FuelCenter), Num_fuelcenters);
	
	// Save the control cen info
		PHYSFS_write(fp, &Control_center_been_hit, sizeof(int), 1);
		PHYSFS_write(fp, &Control_center_player_been_seen, sizeof(int), 1);
		PHYSFS_write(fp, &Control_center_next_fire_time, sizeof(int), 1);
		PHYSFS_write(fp, &Control_center_present, sizeof(int), 1);
		PHYSFS_write(fp, &Dead_controlcen_object_num, sizeof(int), 1);
	
	// Save the AI state
		ai_save_state( fp );
	
	// Save the automap visited info
		PHYSFS_write(fp, Automap_visited, sizeof(ubyte), MAX_SEGMENTS);

	}
	PHYSFS_write(fp, &state_game_id, sizeof(uint), 1);
	PHYSFS_write(fp, &Laser_rapid_fire, sizeof(int), 1);
	PHYSFS_write(fp, &Lunacy, sizeof(int), 1);  //  Yes, writing this twice.  Removed the Ugly robot system, but didn't want to change savegame format.
	PHYSFS_write(fp, &Lunacy, sizeof(int), 1);

	// Save automap marker info

	PHYSFS_write(fp, MarkerObject, sizeof(MarkerObject) ,1);
	PHYSFS_write(fp, MarkerOwner, sizeof(MarkerOwner), 1);
	PHYSFS_write(fp, MarkerMessage, sizeof(MarkerMessage), 1);

	PHYSFS_write(fp, &Afterburner_charge, sizeof(fix), 1);

	//save last was super information
	PHYSFS_write(fp, &Primary_last_was_super, sizeof(Primary_last_was_super), 1);
	PHYSFS_write(fp, &Secondary_last_was_super, sizeof(Secondary_last_was_super), 1);

	//	Save flash effect stuff
	PHYSFS_write(fp, &Flash_effect, sizeof(int), 1);
	PHYSFS_write(fp, &Time_flash_last_played, sizeof(int), 1);
	PHYSFS_write(fp, &PaletteRedAdd, sizeof(int), 1);
	PHYSFS_write(fp, &PaletteGreenAdd, sizeof(int), 1);
	PHYSFS_write(fp, &PaletteBlueAdd, sizeof(int), 1);

	PHYSFS_write(fp, Light_subtracted, sizeof(Light_subtracted[0]), MAX_SEGMENTS);

	PHYSFS_write(fp, &First_secret_visit, sizeof(First_secret_visit), 1);

	if (PHYSFS_write(fp, &Omega_charge, sizeof(Omega_charge), 1) < 1)
	{
		if ( !(Game_mode & GM_MULTI) ) {
			nm_messagebox(NULL, 1, TXT_OK, "Error writing savegame.\nPossibly out of disk\nspace.");
			PHYSFS_close(fp);
			PHYSFS_delete(filename);
		}
	} else  {
		PHYSFS_close(fp);

		#ifdef MACINTOSH		// set the type and creator of the saved game file
		{
			FInfo finfo;
			OSErr err;
			Str255 pfilename;
	
			strcpy(pfilename, filename);
			c2pstr(pfilename);
			err = HGetFInfo(0, 0, pfilename, &finfo);
			finfo.fdType = 'SVGM';
			finfo.fdCreator = 'DCT2';
			err = HSetFInfo(0, 0, pfilename, &finfo);
		}
		#endif
	}
	
	start_time();

	return 1;
}

//	-----------------------------------------------------------------------------------
//	Set the player's position from the globals Secret_return_segment and Secret_return_orient.
void set_pos_from_return_segment(void)
{
	int	plobjnum = Players[Player_num].objnum;

	compute_segment_center(&Objects[plobjnum].pos, &Segments[Secret_return_segment]);
	obj_relink(plobjnum, Secret_return_segment);
	reset_player_object();
	Objects[plobjnum].orient = Secret_return_orient;
}

//	-----------------------------------------------------------------------------------
int state_restore_all(int in_game, int secret_restore, char *filename_override)
{
	char filename[128];
	int	filenum = -1;

#ifdef NETWORK
	if ( Game_mode & GM_MULTI )	{
#ifdef MULTI_SAVE
			multi_initiate_restore_game();
#endif
		return 0;
	}
#endif

	if (in_game && (Current_level_num < 0) && (secret_restore == 0)) {
		HUD_init_message( "Can't restore in secret level!" );
		return 0;
	}

	if ( Newdemo_state == ND_STATE_RECORDING )
		newdemo_stop_recording();

	if ( Newdemo_state != ND_STATE_NORMAL )
		return 0;

	stop_time();

	if (filename_override) {
		strcpy(filename, filename_override);
		filenum = NUM_SAVES+1;		//	So we don't trigger autosave
	} else if (!(filenum = state_get_restore_file(filename, 0)))	{
		start_time();
		return 0;
	}
	
	//	MK, 1/1/96
	//	If not in multiplayer, do special secret level stuff.
	//	If Nsecret.sgc (where N = filenum) exists, then copy it to secret.sgc.
	//	If it doesn't exist, then delete secret.sgc
	if (!secret_restore && !(Game_mode & GM_MULTI)) {
		int	rval;
		char	temp_fname[32], fc;

		if (filenum != -1) {
			if (filenum >= 10)
				fc = (filenum-10) + 'a';
			else
				fc = '0' + filenum;
			
			#ifndef MACINTOSH
			sprintf(temp_fname, "%csecret.sgc", fc);
			#else
			sprintf(temp_fname, "Players/%csecret.sgc", fc);
			#endif

			mprintf((0, "Trying to copy %s to secret.sgc.\n", temp_fname));

			if (PHYSFS_exists(temp_fname))
			{
				mprintf((0, "Copying %s to secret.sgc\n", temp_fname));
				rval = copy_file(temp_fname, SECRETC_FILENAME);
				Assert(rval == 0);	//	Oops, error copying temp_fname to secret.sgc!
			} else
				PHYSFS_delete(SECRETC_FILENAME);
		}
	}

	//	Changed, 11/15/95, MK, don't to autosave if restoring from main menu.
	if ((filenum != (NUM_SAVES+1)) && in_game) {
		char	temp_filename[128];
		mprintf((0, "Doing autosave, filenum = %i, != %i!\n", filenum, NUM_SAVES+1));
		#ifndef MACINTOSH
		sprintf( temp_filename, "%s.sg%x", Players[Player_num].callsign, NUM_SAVES );
		#else
		sprintf(temp_filename, "Players/%s.sg%x", Players[Player_num].callsign, NUM_SAVES);
		#endif
		state_save_all(!in_game, secret_restore, temp_filename, 0);
	}

	if ( !secret_restore && in_game ) {
		int choice;
		choice =  nm_messagebox( NULL, 2, "Yes", "No", "Restore Game?" );
		if ( choice != 0 )	{
			start_time();
			return 0;
		}
	}

	start_time();

	return state_restore_all_sub(filename, 0, secret_restore);
}

extern void init_player_stats_new_ship(void);

void ShowLevelIntro(int level_num);

extern void do_cloak_invul_secret_stuff(fix old_gametime);
extern void copy_defaults_to_robot(object *objp);

int state_restore_all_sub(char *filename, int multi, int secret_restore)
{
	int ObjectStartLocation;
	int version,i, j, segnum;
	object * obj;
	PHYSFS_file *fp;
	int current_level, next_level;
	int between_levels;
	char mission[16];
	char desc[DESC_LENGTH+1];
	char id[5];
	char org_callsign[CALLSIGN_LEN+16];
#ifdef NETWORK
	int found;
	int nplayers;	//,playid[12],mynum;
	player restore_players[MAX_PLAYERS];
#endif
	fix	old_gametime = GameTime;

	#if defined(MACINTOSH) && !defined(NDEBUG)
	if (strncmp(filename, "Players/", 9))
		Int3();
	#endif

	fp = cfopen(filename, "rb");
	if ( !fp ) return 0;

//Read id
	//FIXME: check for swapped file, react accordingly...
	PHYSFS_read(fp, id, sizeof(char) * 4, 1);
	if ( memcmp( id, dgss_id, 4 )) {
		PHYSFS_close(fp);
		return 0;
	}

//Read version
	PHYSFS_read(fp, &version, sizeof(int), 1);
	if (version < STATE_COMPATIBLE_VERSION)	{
		PHYSFS_close(fp);
		return 0;
	}

// Read description
	PHYSFS_read(fp, desc, sizeof(char) * DESC_LENGTH, 1);

// Skip the current screen shot...
	PHYSFS_seek(fp, PHYSFS_tell(fp) + THUMBNAIL_W * THUMBNAIL_H);

// And now...skip the goddamn palette stuff that somebody forgot to add
	PHYSFS_seek(fp, PHYSFS_tell(fp) + 768);

// Read the Between levels flag...
	PHYSFS_read(fp, &between_levels, sizeof(int), 1);

	Assert(between_levels == 0);	//between levels save ripped out

// Read the mission info...
	PHYSFS_read(fp, mission, sizeof(char) * 9, 1);
        mprintf ((0,"Missionname to load = %s\n",mission));

	if (!load_mission_by_name( mission ))	{
		nm_messagebox( NULL, 1, "Ok", "Error!\nUnable to load mission\n'%s'\n", mission );
		PHYSFS_close(fp);
		return 0;
	}

//Read level info
	PHYSFS_read(fp, &current_level, sizeof(int), 1);
	PHYSFS_read(fp, &next_level, sizeof(int), 1);

//Restore GameTime
	PHYSFS_read(fp, &GameTime, sizeof(fix), 1);

// Start new game....
	if (!multi)	{
		Game_mode = GM_NORMAL;
		Function_mode = FMODE_GAME;
#ifdef NETWORK
		change_playernum_to(0);
#endif
		strcpy( org_callsign, Players[0].callsign );
		N_players = 1;
		if (!secret_restore) {
			InitPlayerObject();				//make sure player's object set up
			init_player_stats_game();		//clear all stats
		}
	} else {
		strcpy( org_callsign, Players[Player_num].callsign );
	}

#ifdef NETWORK
   if (Game_mode & GM_MULTI)
	 {
		PHYSFS_read(fp, &state_game_id, sizeof(int), 1);
		PHYSFS_read(fp, &Netgame, sizeof(netgame_info), 1);
		PHYSFS_read(fp, &NetPlayers, sizeof(AllNetPlayers_info), 1);
		PHYSFS_read(fp, &nplayers, sizeof(N_players), 1);
		PHYSFS_read(fp, &Player_num, sizeof(Player_num), 1);
		for (i=0;i<nplayers;i++)
			PHYSFS_read(fp, &restore_players[i], sizeof(player), 1);
#ifdef RISKY_PROPOSITION
		PHYSFS_read(fp, &robot_controlled[0], 4 * MAX_ROBOTS_CONTROLLED, 1);
		PHYSFS_read(fp, &robot_agitation[0], 4 * MAX_ROBOTS_CONTROLLED, 1);
		PHYSFS_read(fp, &robot_controlled_time[0], 4 * MAX_ROBOTS_CONTROLLED, 1);
		PHYSFS_read(fp, &robot_last_send_time[0], 4 * MAX_ROBOTS_CONTROLLED, 1);
		PHYSFS_read(fp, &robot_last_message_time[0], 4 * MAX_ROBOTS_CONTROLLED, 1);
		PHYSFS_read(fp, &robot_send_pending[0], 4 * MAX_ROBOTS_CONTROLLED, 1);
		PHYSFS_read(fp, &robot_fired[0], 4 * MAX_ROBOTS_CONTROLLED, 1);

      for (i=0;i<MAX_ROBOTS_CONTROLLED;i++)
			PHYSFS_read(fp, &robot_fire_buf[i][0],21,1);
#endif

	   for (i=0;i<nplayers;i++)
		 {
		  found=0;
		  for (j=0;j<nplayers;j++)
			 {
           if ((!strcmp (restore_players[i].callsign,Players[j].callsign)) && Players[j].connected==1)
				 found=1;
			 }
		  restore_players[i].connected=found;
	    }
		memcpy (&Players,&restore_players,sizeof(player)*nplayers);
		N_players=nplayers;

      if (network_i_am_master())
		 {
		  for (i=0;i<N_players;i++)
			{
			 if (i==Player_num)
				continue;
   		 Players[i].connected=0;
			}
	 	 }

	  	//Viewer = ConsoleObject = &Objects[Players[Player_num].objnum];
	 }

#endif

//Read player info

	{
		StartNewLevelSub(current_level, 1, secret_restore);

		if (secret_restore) {
			player	dummy_player;

			PHYSFS_read(fp, &dummy_player, sizeof(player), 1);
			if (secret_restore == 1) {		//	This means he didn't die, so he keeps what he got in the secret level.
				Players[Player_num].level = dummy_player.level;
				Players[Player_num].last_score = dummy_player.last_score;
				Players[Player_num].time_level = dummy_player.time_level;

				Players[Player_num].num_robots_level = dummy_player.num_robots_level;
				Players[Player_num].num_robots_total = dummy_player.num_robots_total;
				Players[Player_num].hostages_rescued_total = dummy_player.hostages_rescued_total;
				Players[Player_num].hostages_total = dummy_player.hostages_total;
				Players[Player_num].hostages_on_board = dummy_player.hostages_on_board;
				Players[Player_num].hostages_level = dummy_player.hostages_level;
				Players[Player_num].homing_object_dist = dummy_player.homing_object_dist;
				Players[Player_num].hours_level = dummy_player.hours_level;
				Players[Player_num].hours_total = dummy_player.hours_total;
				do_cloak_invul_secret_stuff(old_gametime);
			} else {
				Players[Player_num] = dummy_player;
			}
		} else {
			PHYSFS_read(fp, &Players[Player_num], sizeof(player), 1);
		}
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

	PHYSFS_read(fp, &Cheats_enabled, sizeof(int),1);

	if ( !between_levels )	{
		Do_appearance_effect = 0;			// Don't do this for middle o' game stuff.

		ObjectStartLocation = PHYSFS_tell(fp);
		//Clear out all the objects from the lvl file
		for (segnum=0; segnum <= Highest_segment_index; segnum++)
			Segments[segnum].objects = -1;
		reset_objects(1);
	
		//Read objects, and pop 'em into their respective segments.
		PHYSFS_read(fp, &i, sizeof(int), 1);
		Highest_object_index = i-1;
		PHYSFS_read(fp, Objects, sizeof(object) * i, 1);
	
		Object_next_signature = 0;
		for (i=0; i<=Highest_object_index; i++ )	{
			obj = &Objects[i];
			obj->rtype.pobj_info.alt_textures = -1;
			segnum = obj->segnum;
			obj->next = obj->prev = obj->segnum = -1;
			if ( obj->type != OBJ_NONE )	{
				obj_link(i,segnum);
				if ( obj->signature > Object_next_signature )
					Object_next_signature = obj->signature;
			}

			//look for, and fix, boss with bogus shields
			if (obj->type == OBJ_ROBOT && Robot_info[obj->id].boss_flag) {
				fix save_shields = obj->shields;

				copy_defaults_to_robot(obj);		//calculate starting shields

				//if in valid range, use loaded shield value
				if (save_shields > 0 && save_shields <= obj->shields)
					obj->shields = save_shields;
				else
					obj->shields /= 2;  //give player a break
			}

		}	
		special_reset_objects();
		Object_next_signature++;
	
		//	1 = Didn't die on secret level.
		//	2 = Died on secret level.
		if (secret_restore && (Current_level_num >= 0)) {
			set_pos_from_return_segment();
			if (secret_restore == 2)
				init_player_stats_new_ship();
		}

		//Restore wall info
		PHYSFS_read(fp, &i, sizeof(int), 1);
		Num_walls = i;
		PHYSFS_read(fp, Walls, sizeof(wall) * Num_walls, 1);

		//now that we have the walls, check if any sounds are linked to
		//walls that are now open
		for (i=0;i<Num_walls;i++) {
			if (Walls[i].type == WALL_OPEN)
				digi_kill_sound_linked_to_segment(Walls[i].segnum,Walls[i].sidenum,-1);	//-1 means kill any sound
		}

		//Restore exploding wall info
		if (version >= 10) {
			PHYSFS_read(fp, &i, sizeof(int), 1);
			PHYSFS_read(fp, expl_wall_list, sizeof(*expl_wall_list), i);
		}

		//Restore door info
		PHYSFS_read(fp, &i, sizeof(int), 1);
		Num_open_doors = i;
		PHYSFS_read(fp, ActiveDoors, sizeof(active_door), Num_open_doors);
	
		if (version >= 14) {		//Restore cloaking wall info
			PHYSFS_read(fp, &i, sizeof(int), 1);
			Num_cloaking_walls = i;
			PHYSFS_read(fp, CloakingWalls, sizeof(cloaking_wall), Num_cloaking_walls);
		}
	
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
		PHYSFS_read(fp, &Control_center_destroyed, sizeof(int), 1);
		PHYSFS_read(fp, &Countdown_timer, sizeof(int), 1);
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
		ai_restore_state( fp, version );
	
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
		PHYSFS_read(fp, &state_game_id, sizeof(uint), 1);
		PHYSFS_read(fp, &Laser_rapid_fire, sizeof(int), 1);
		PHYSFS_read(fp, &Lunacy, sizeof(int), 1);		//	Yes, writing this twice.  Removed the Ugly robot system, but didn't want to change savegame format.
		PHYSFS_read(fp, &Lunacy, sizeof(int), 1);
		if ( Lunacy )
			do_lunacy_on();
	}

	if (version >= 17) {
		PHYSFS_read(fp, MarkerObject, sizeof(MarkerObject), 1);
		PHYSFS_read(fp, MarkerOwner, sizeof(MarkerOwner), 1);
		PHYSFS_read(fp, MarkerMessage, sizeof(MarkerMessage), 1);
	}
	else {
		int num,dummy;

		// skip dummy info

		PHYSFS_read(fp, &num,sizeof(int), 1);           // was NumOfMarkers
		PHYSFS_read(fp, &dummy,sizeof(int), 1);         // was CurMarker

		PHYSFS_seek(fp, PHYSFS_tell(fp) + num * (sizeof(vms_vector) + 40));

		for (num=0;num<NUM_MARKERS;num++)
			MarkerObject[num] = -1;
	}

	if (version>=11) {
		if (secret_restore != 1)
			PHYSFS_read(fp, &Afterburner_charge, sizeof(fix), 1);
		else {
			fix	dummy_fix;
			PHYSFS_read(fp, &dummy_fix, sizeof(fix), 1);
		}
	}
	if (version>=12) {
		//read last was super information
		PHYSFS_read(fp, &Primary_last_was_super, sizeof(Primary_last_was_super), 1);
		PHYSFS_read(fp, &Secondary_last_was_super, sizeof(Secondary_last_was_super), 1);
	}

	if (version >= 12) {
		PHYSFS_read(fp, &Flash_effect, sizeof(int), 1);
		PHYSFS_read(fp, &Time_flash_last_played, sizeof(int), 1);
		PHYSFS_read(fp, &PaletteRedAdd, sizeof(int), 1);
		PHYSFS_read(fp, &PaletteGreenAdd, sizeof(int), 1);
		PHYSFS_read(fp, &PaletteBlueAdd, sizeof(int), 1);
	} else {
		Flash_effect = 0;
		Time_flash_last_played = 0;
		PaletteRedAdd = 0;
		PaletteGreenAdd = 0;
		PaletteBlueAdd = 0;
	}

	//	Load Light_subtracted
	if (version >= 16) {
		PHYSFS_read(fp, Light_subtracted, sizeof(Light_subtracted[0]), MAX_SEGMENTS);
		apply_all_changed_light();
		compute_all_static_light();	//	set static_light field in segment struct.  See note at that function.
	} else {
		int	i;
		for (i=0; i<=Highest_segment_index; i++)
			Light_subtracted[i] = 0;
	}

	if (!secret_restore) {
		if (version >= 20) {
			PHYSFS_read(fp, &First_secret_visit, sizeof(First_secret_visit), 1);
			mprintf((0, "File: [%s] Read First_secret_visit: New value = %i\n", filename, First_secret_visit));
		} else
			First_secret_visit = 1;
	} else
		First_secret_visit = 0;

	if (version >= 22)
	{
		if (secret_restore != 1)
			PHYSFS_read(fp, &Omega_charge, sizeof(fix), 1);
		else {
			fix	dummy_fix;
			PHYSFS_read(fp, &dummy_fix, sizeof(fix), 1);
		}
	}

	PHYSFS_close(fp);

#ifdef NETWORK
   if (Game_mode & GM_MULTI)   // Get rid of ships that aren't
	 {									 // connected in the restored game
		for (i=0;i<nplayers;i++)
		 {
		  mprintf ((0,"Testing %s = %d\n",Players[i].callsign,Players[i].connected));
		  if (Players[i].connected!=1)
		   {
		    network_disconnect_player (i);
  	       create_player_appearance_effect(&Objects[Players[i].objnum]);
			 mprintf ((0,"Killing player ship %s!\n",Players[i].callsign));
	      }
		 }
			
	 }
#endif

// Load in bitmaps, etc..
//!!	piggy_load_level_data();	//already done by StartNewLevelSub()

	return 1;
}

//	When loading a saved game, segp->static_light is bogus.
//	This is because apply_all_changed_light, which is supposed to properly update this value,
//	cannot do so because it needs the original light cast from a light which is no longer there.
//	That is, a light has been blown out, so the texture remaining casts 0 light, but the static light
//	which is present in the static_light field contains the light cast from that light.
void compute_all_static_light(void)
{
	int	i, j, k;

	for (i=0; i<=Highest_segment_index; i++) {
		fix	total_light;
		segment	*segp;

		segp = &Segments[i];
		total_light = 0;

		for (j=0; j<MAX_SIDES_PER_SEGMENT; j++) {
			side	*sidep;

			sidep = &segp->sides[j];

			for (k=0; k<4; k++)
				total_light += sidep->uvls[k].l;
		}

		Segment2s[i].static_light = total_light/(MAX_SIDES_PER_SEGMENT*4);
	}

}


int state_get_game_id(char *filename)
{
	int version;
	PHYSFS_file *fp;
	int between_levels;
	char mission[16];
	char desc[DESC_LENGTH+1];
	char id[5];
	int dumbint;

mprintf((0, "Restoring multigame from [%s]\n", filename));

	fp = PHYSFS_openRead(filename);
	if (!fp) return 0;

//Read id
	//FIXME: check for swapped file, react accordingly...
	PHYSFS_read(fp, id, sizeof(char) * 4, 1);
	if ( memcmp( id, dgss_id, 4 )) {
		PHYSFS_close(fp);
		return 0;
	}

//Read version
	PHYSFS_read(fp, &version, sizeof(int), 1);
	if (version < STATE_COMPATIBLE_VERSION)	{
		PHYSFS_close(fp);
		return 0;
	}

// Read description
	PHYSFS_read(fp, desc, sizeof(char) * DESC_LENGTH, 1);

// Skip the current screen shot...
	PHYSFS_seek(fp, PHYSFS_tell(fp) + THUMBNAIL_W * THUMBNAIL_H);

// And now...skip the palette stuff that somebody forgot to add
	PHYSFS_seek(fp, PHYSFS_tell(fp) + 768);

// Read the Between levels flag...
	PHYSFS_read(fp, &between_levels, sizeof(int), 1);

	Assert(between_levels == 0);	//between levels save ripped out

// Read the mission info...
	PHYSFS_read(fp, mission, sizeof(char) * 9, 1);
//Read level info
	PHYSFS_read(fp, &dumbint, sizeof(int), 1);
	PHYSFS_read(fp, &dumbint, sizeof(int), 1);

//Restore GameTime
	PHYSFS_read(fp, &dumbint, sizeof(fix), 1);

	PHYSFS_read(fp, &state_game_id, sizeof(int), 1);

	return (state_game_id);
 }

#if defined(POLY_ACC)
//void screen_shot_pa(ubyte *dst,ushort *src)
//{
//    //ushort *src = pa_get_buffer_address(0),
//    ushort *s;
//    fix u, v, du, dv;
//    int ui, w, h;
//
//    pa_flush();
//
//    du = (640.0 / (float)THUMBNAIL_W) * 65536.0;
//    dv = (480.0 / (float)THUMBNAIL_H) * 65536.0;
//
//    for(v = h = 0; h != THUMBNAIL_H; ++h)
//    {
//        s = src + f2i(v) * 640;
//        v += dv;
//        for(u = w = 0; w != THUMBNAIL_W; ++w)
//        {
//            ui = f2i(u);
//            *dst++ = gr_find_closest_color((s[ui] >> 9) & 0x3e, (s[ui] >> 4) & 0x3e, (s[ui] << 1) & 0x3e);
//            u += du;
//        }
//    }
//}

void screen_shot_pa(grs_canvas *dcanv,grs_canvas *scanv)
{
#if !defined(MACINTOSH)
	ubyte *dst;
	ushort *src;
	int x,y;

	Assert(scanv->cv_w == dcanv->cv_w && scanv->cv_h == dcanv->cv_h);

	pa_flush();

	src = (ushort *) scanv->cv_bitmap.bm_data;
	dst = dcanv->cv_bitmap.bm_data;

	#ifdef PA_3DFX_VOODOO
   src=(ushort *)pa_set_back_to_read();
	#endif

	for (y=0; y<scanv->cv_h; y++) {
		for (x=0; x<scanv->cv_w; x++) {
			#ifdef PA_3DFX_VOODOO
			*dst++ = gr_find_closest_color((*src >> 10) & 0x3e, (*src >> 5) & 0x3f, (*src << 1) & 0x3e);
			#else
			*dst++ = gr_find_closest_color((*src >> 9) & 0x3e, (*src >> 4) & 0x3e, (*src << 1) & 0x3e);
			#endif

			src++;
		}
		src = (ushort *) (((ubyte *) src) + (scanv->cv_bitmap.bm_rowsize - (scanv->cv_bitmap.bm_w*PA_BPP)));
		dst += dcanv->cv_bitmap.bm_rowsize - dcanv->cv_bitmap.bm_w;
	}
	#ifdef PA_3DFX_VOODOO
	pa_set_front_to_read();
	#endif
#endif
}
#endif

