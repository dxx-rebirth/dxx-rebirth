/* $Id: state.c,v 1.9 2003-06-16 06:57:34 btb Exp $ */
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
 * Game save/restore functions
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
#include <unistd.h>
#include <errno.h>
#ifdef MACINTOSH
#include <Files.h>
#endif

#ifdef OGL
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
extern byte robot_fire_buf[MAX_ROBOTS_CONTROLLED][18+3];


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

int state_get_save_file(char * fname, char * dsc, int multi )
{
	CFILE *fp;
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
		fp = cfopen(filename[i], "rb");
		if ( fp ) {
			//Read id
			cfread(id, sizeof(char)*4, 1, fp);
			if ( !memcmp( id, dgss_id, 4 )) {
				//Read version
				cfread(&version, sizeof(int), 1, fp);
				if (version >= STATE_COMPATIBLE_VERSION)	{
					// Read description
					cfread(desc[i], sizeof(char)*DESC_LENGTH, 1, fp);
					//rpad_string( desc[i], DESC_LENGTH-1 );
					// Read thumbnail
					//sc_bmp[i] = gr_create_bitmap(THUMBNAIL_W,THUMBNAIL_H );
					//cfread(sc_bmp[i]->bm_data, THUMBNAIL_W * THUMBNAIL_H, 1, fp);
					valid = 1;
				}
			} 
			cfclose(fp);
		}
		if (!valid) {
			strcpy( desc[i], TXT_EMPTY );
			//rpad_string( desc[i], DESC_LENGTH-1 );
		}
		m[i].type = NM_TYPE_INPUT_MENU; m[i].text = desc[i]; m[i].text_len = DESC_LENGTH-1;
	}

	sc_last_item = -1;
	choice = newmenu_do1( NULL, "Save Game", NUM_SAVES, m, NULL, state_default_item );

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
	CFILE *fp;
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
		fp = cfopen(filename[i], "rb");
		if ( fp ) {
			//Read id
			cfread(id, sizeof(char)*4, 1, fp);
			if ( !memcmp( id, dgss_id, 4 )) {
				//Read version
				cfread(&version, sizeof(int), 1, fp);
				if (version >= STATE_COMPATIBLE_VERSION)	{
					// Read description
					cfread(desc[i], sizeof(char)*DESC_LENGTH, 1, fp);
					//rpad_string( desc[i], DESC_LENGTH-1 );
					m[i+1].type = NM_TYPE_MENU; m[i+1].text = desc[i];
					// Read thumbnail
					sc_bmp[i] = gr_create_bitmap(THUMBNAIL_W,THUMBNAIL_H );
					cfread(sc_bmp[i]->bm_data, THUMBNAIL_W * THUMBNAIL_H, 1, fp);
					if (version >= 9) {
						ubyte pal[256*3];
						cfread(pal, 3, 256, fp);
						gr_remap_bitmap_good( sc_bmp[i], pal, -1, -1 );
					}
					nsaves++;
					valid = 1;
				} 
			}
			cfclose(fp);
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
	byte	buf[CF_BUF_SIZE];
	CFILE   *in_file, *out_file;

	out_file = cfopen(new_file, "wb");

	if (out_file == NULL)
		return -1;

	in_file = cfopen(old_file, "rb");

	if (in_file == NULL)
		return -2;

	while (!cfeof(in_file))
	{
		int bytes_read;

		bytes_read = cfread(buf, 1, CF_BUF_SIZE, in_file);
		if (cferror(in_file))
			Error("Cannot read from file <%s>: %s", old_file, strerror(errno));

		Assert(bytes_read == CF_BUF_SIZE || cfeof(in_file));

		cfwrite(buf, 1, bytes_read, out_file);

		if (cferror(out_file))
			Error("Cannot write to file <%s>: %s", new_file, strerror(errno));
	}

	if (cfclose(in_file)) {
		cfclose(out_file);
		return -3;
	}

	if (cfclose(out_file))
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
int state_save_all(int between_levels, int secret_save, char *filename_override)
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
		cfile_delete(SECRETB_FILENAME);
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
		} else if (!(filenum = state_get_save_file(filename,desc,0))) {
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

			if (cfexist(temp_fname))
			{
				mprintf((0, "Deleting file %s\n", temp_fname));
				rval = cfile_delete(temp_fname);
				Assert(rval == 0);	//	Oops, error deleting file in temp_fname.
			}

			if (cfexist(SECRETC_FILENAME))
			{
				mprintf((0, "Copying secret.sgc to %s.\n", temp_fname));
				rval = copy_file(SECRETC_FILENAME, temp_fname);
				Assert(rval == 0);	//	Oops, error copying temp_fname to secret.sgc!
			}
		}
	}

	//	Save file we're going to save over in last slot and call "[autosave backup]"
	if (!filename_override) {
		CFILE *tfp;
	
		tfp = cfopen(filename, "rb");

		if ( tfp ) {
			char	newname[128];

			#ifndef MACINTOSH
			sprintf( newname, "%s.sg%x", Players[Player_num].callsign, NUM_SAVES );
			#else
			sprintf( newname, ":Players:%s.sg%x", Players[Player_num].callsign, NUM_SAVES );
			#endif
			
			cfseek(tfp, DESC_OFFSET, SEEK_SET);
			cfwrite("[autosave backup]", sizeof(char)*DESC_LENGTH, 1, tfp);
			cfclose(tfp);
			cfile_delete(newname);
			cfile_rename(filename, newname);
		}
	}
	
	rval = state_save_all_sub(filename, desc, between_levels);

	return rval;
}

extern	fix	Flash_effect, Time_flash_last_played;


int state_save_all_sub(char *filename, char *desc, int between_levels)
{
	int i,j;
	CFILE *fp;
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

	fp = cfopen(filename, "wb");
	if ( !fp ) {
		if ( !(Game_mode & GM_MULTI) )
			nm_messagebox(NULL, 1, TXT_OK, "Error writing savegame.\nPossibly out of disk\nspace.");
		start_time();
		return 0;
	}

//Save id
	cfwrite(dgss_id, sizeof(char)*4, 1, fp);

//Save version
	i = STATE_VERSION;
	cfwrite(&i, sizeof(int), 1, fp);

//Save description
	cfwrite(desc, sizeof(char) * DESC_LENGTH, 1, fp);
	
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
#endif

					pal = gr_palette;

					cfwrite(cnv->cv_bitmap.bm_data, THUMBNAIL_W * THUMBNAIL_H, 1, fp);

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
			cfwrite(cnv->cv_bitmap.bm_data, THUMBNAIL_W * THUMBNAIL_H, 1, fp);
			
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
		cfwrite(pal, 3, 256, fp);
	}
	else
	{
	 	ubyte color = 0;
	 	for ( i=0; i<THUMBNAIL_W*THUMBNAIL_H; i++ )
			cfwrite(&color, sizeof(ubyte), 1, fp);
	} 

// Save the Between levels flag...
	cfwrite(&between_levels, sizeof(int), 1, fp);

// Save the mission info...
        mprintf ((0,"HEY! Mission name is %s\n",Mission_list[Current_mission_num].filename));
	cfwrite(&Mission_list[Current_mission_num], sizeof(char), 9, fp);

//Save level info
	cfwrite(&Current_level_num, sizeof(int), 1, fp);
	cfwrite(&Next_level_num, sizeof(int), 1, fp);

//Save GameTime
	cfwrite(&GameTime, sizeof(fix), 1, fp);

// If coop save, save all
#ifdef NETWORK
   if (Game_mode & GM_MULTI_COOP)
	 {
		cfwrite(&state_game_id, sizeof(int), 1, fp);
		cfwrite(&Netgame, sizeof(netgame_info), 1, fp);
		cfwrite(&NetPlayers, sizeof(AllNetPlayers_info), 1, fp);
		cfwrite(&N_players, sizeof(int), 1, fp);
		cfwrite(&Player_num, sizeof(int), 1, fp);
		for (i=0;i<N_players;i++)
			cfwrite(&Players[i], sizeof(player), 1, fp);

#ifdef RISKY_PROPOSITION
		cfwrite(&robot_controlled[0], 4, MAX_ROBOTS_CONTROLLED, fp);
		cfwrite(&robot_agitation[0], 4, MAX_ROBOTS_CONTROLLED, fp);
		cfwrite(&robot_controlled_time[0], 4, MAX_ROBOTS_CONTROLLED, fp);
		cfwrite(&robot_last_send_time[0], 4, MAX_ROBOTS_CONTROLLED, fp);
		cfwrite(&robot_last_message_time[0], 4, MAX_ROBOTS_CONTROLLED, fp);
		cfwrite(&robot_send_pending[0], 4, MAX_ROBOTS_CONTROLLED, fp);
		cfwrite(&robot_fired[0], 4, MAX_ROBOTS_CONTROLLED, fp);
 
      for (i=0;i<MAX_ROBOTS_CONTROLLED;i++)
			cfwrite(robot_fire_buf[i][0], 18 + 3, 1, fp);
#endif

	 }
#endif

//Save player info
	cfwrite(&Players[Player_num], sizeof(player), 1, fp);

// Save the current weapon info
	cfwrite(&Primary_weapon, sizeof(byte), 1, fp);
	cfwrite(&Secondary_weapon, sizeof(byte), 1, fp);

// Save the difficulty level
	cfwrite(&Difficulty_level, sizeof(int), 1, fp);
// Save cheats enabled
	cfwrite(&Cheats_enabled, sizeof(int), 1, fp);

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
		cfwrite(&i, sizeof(int), 1, fp);
		cfwrite(Objects, sizeof(object), i, fp);
		
	//Save wall info
		i = Num_walls;
		cfwrite(&i, sizeof(int), 1, fp);
		cfwrite(Walls, sizeof(wall), i, fp);

	//Save exploding wall info
		i = MAX_EXPLODING_WALLS;
		cfwrite(&i, sizeof(int), 1, fp);
		cfwrite(expl_wall_list, sizeof(*expl_wall_list), i, fp);
	
	//Save door info
		i = Num_open_doors;
		cfwrite(&i, sizeof(int), 1, fp );
		cfwrite(ActiveDoors, sizeof(active_door), i, fp);
	
	//Save cloaking wall info
		i = Num_cloaking_walls;
		cfwrite(&i, sizeof(int), 1, fp );
		cfwrite(CloakingWalls, sizeof(cloaking_wall), i, fp);
	
	//Save trigger info
		cfwrite(&Num_triggers, sizeof(int), 1, fp);
		cfwrite(Triggers, sizeof(trigger), Num_triggers, fp);
	
	//Save tmap info
		for (i=0; i<=Highest_segment_index; i++ )	{
			for (j=0; j<6; j++ )	{
				cfwrite(&Segments[i].sides[j].wall_num, sizeof(short), 1, fp);
				cfwrite(&Segments[i].sides[j].tmap_num, sizeof(short), 1, fp);
				cfwrite(&Segments[i].sides[j].tmap_num2, sizeof(short), 1, fp);
			}
		}
	
	// Save the fuelcen info
		cfwrite(&Control_center_destroyed, sizeof(int), 1, fp);
		cfwrite(&Countdown_timer, sizeof(int), 1, fp);
		cfwrite(&Num_robot_centers, sizeof(int), 1, fp);
		cfwrite(RobotCenters, sizeof(matcen_info), Num_robot_centers, fp);
		cfwrite(&ControlCenterTriggers, sizeof(control_center_triggers), 1, fp);
		cfwrite(&Num_fuelcenters, sizeof(int), 1, fp);
		cfwrite(Station, sizeof(FuelCenter), Num_fuelcenters, fp);
	
	// Save the control cen info
		cfwrite(&Control_center_been_hit, sizeof(int), 1, fp);
		cfwrite(&Control_center_player_been_seen, sizeof(int), 1, fp);
		cfwrite(&Control_center_next_fire_time, sizeof(int), 1, fp);
		cfwrite(&Control_center_present, sizeof(int), 1, fp);
		cfwrite(&Dead_controlcen_object_num, sizeof(int), 1, fp);
	
	// Save the AI state
		ai_save_state( fp );
	
	// Save the automap visited info
		cfwrite(Automap_visited, sizeof(ubyte) * MAX_SEGMENTS, 1, fp);

	}
	cfwrite(&state_game_id, sizeof(uint), 1, fp);
	cfwrite(&Laser_rapid_fire, sizeof(int), 1, fp);
	cfwrite(&Lunacy, sizeof(int), 1, fp);		//	Yes, writing this twice.  Removed the Ugly robot system, but didn't want to change savegame format.
	cfwrite(&Lunacy, sizeof(int), 1, fp);

	// Save automap marker info

	cfwrite(MarkerObject, sizeof(MarkerObject), 1, fp);
	cfwrite(MarkerOwner, sizeof(MarkerOwner), 1, fp);
	cfwrite(MarkerMessage, sizeof(MarkerMessage), 1, fp);

	cfwrite(&Afterburner_charge, sizeof(fix), 1, fp);

	//save last was super information
	cfwrite(&Primary_last_was_super, sizeof(Primary_last_was_super), 1, fp);
	cfwrite(&Secondary_last_was_super, sizeof(Secondary_last_was_super), 1, fp);

	//	Save flash effect stuff
	cfwrite(&Flash_effect, sizeof(int), 1, fp);
	cfwrite(&Time_flash_last_played, sizeof(int), 1, fp);
	cfwrite(&PaletteRedAdd, sizeof(int), 1, fp);
	cfwrite(&PaletteGreenAdd, sizeof(int), 1, fp);
	cfwrite(&PaletteBlueAdd, sizeof(int), 1, fp);

	cfwrite(Light_subtracted, sizeof(Light_subtracted[0]), MAX_SEGMENTS, fp);

	cfwrite(&First_secret_visit, sizeof(First_secret_visit), 1, fp);

	cfwrite(&Omega_charge, sizeof(Omega_charge), 1, fp);
	
	if (cferror(fp))
	{
		if ( !(Game_mode & GM_MULTI) ) {
			nm_messagebox(NULL, 1, TXT_OK, "Error writing savegame.\nPossibly out of disk\nspace.");
			cfclose(fp);
			cfile_delete(filename);
		}
	} else  {
		cfclose(fp);

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
			sprintf(temp_fname, ":Players:%csecret.sgc", fc);
			#endif

			mprintf((0, "Trying to copy %s to secret.sgc.\n", temp_fname));

			if (cfexist(temp_fname))
			{
				mprintf((0, "Copying %s to secret.sgc\n", temp_fname));
				rval = copy_file(temp_fname, SECRETC_FILENAME);
				Assert(rval == 0);	//	Oops, error copying temp_fname to secret.sgc!
			} else
				cfile_delete(SECRETC_FILENAME);
		}
	}

	//	Changed, 11/15/95, MK, don't to autosave if restoring from main menu.
	if ((filenum != (NUM_SAVES+1)) && in_game) {
		char	temp_filename[128];
		mprintf((0, "Doing autosave, filenum = %i, != %i!\n", filenum, NUM_SAVES+1));
		#ifndef MACINTOSH
		sprintf( temp_filename, "%s.sg%x", Players[Player_num].callsign, NUM_SAVES );
		#else
		sprintf( temp_filename, ":Players:%s.sg%x", Players[Player_num].callsign, NUM_SAVES );
		#endif		
		state_save_all(!in_game, secret_restore, temp_filename);
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
	CFILE *fp;
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
	if ( strncmp(filename, ":Players:", 9) )
		Int3();
	#endif

	fp = cfopen(filename, "rb");
	if ( !fp ) return 0;

//Read id
	cfread(id, sizeof(char)*4, 1, fp);
	if ( memcmp( id, dgss_id, 4 )) {
		cfclose(fp);
		return 0;
	}

//Read version
	cfread(&version, sizeof(int), 1, fp);
	if (version < STATE_COMPATIBLE_VERSION)	{
		cfclose(fp);
		return 0;
	}

// Read description
	cfread(desc, sizeof(char) * DESC_LENGTH, 1, fp);

// Skip the current screen shot...
	cfseek(fp, THUMBNAIL_W*THUMBNAIL_H, SEEK_CUR);

// And now...skip the goddamn palette stuff that somebody forgot to add
	cfseek(fp, 768, SEEK_CUR);

// Read the Between levels flag...
	cfread(&between_levels, sizeof(int), 1, fp);

	Assert(between_levels == 0);	//between levels save ripped out

// Read the mission info...
	cfread(mission, sizeof(char), 9, fp);
        mprintf ((0,"Missionname to load = %s\n",mission));

	if (!load_mission_by_name( mission ))	{
		nm_messagebox( NULL, 1, "Ok", "Error!\nUnable to load mission\n'%s'\n", mission );
		cfclose(fp);
		return 0;
	}

//Read level info
	cfread(&current_level, sizeof(int), 1, fp);
	cfread(&next_level, sizeof(int), 1, fp);

//Restore GameTime
	cfread(&GameTime, sizeof(fix), 1, fp);

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
		cfread(&state_game_id,sizeof(int), 1, fp);
		cfread(&Netgame,sizeof(netgame_info), 1, fp);
		cfread(&NetPlayers,sizeof(AllNetPlayers_info), 1, fp);
		cfread(&nplayers,sizeof(N_players), 1, fp);
		cfread(&Player_num,sizeof(Player_num), 1, fp);
		for (i=0;i<nplayers;i++)
			cfread(&restore_players[i], sizeof(player), 1, fp);
#ifdef RISKY_PROPOSITION
		cfread(&robot_controlled[0], 4, MAX_ROBOTS_CONTROLLED, fp);
		cfread(&robot_agitation[0], 4, MAX_ROBOTS_CONTROLLED, fp);
		cfread(&robot_controlled_time[0], 4, MAX_ROBOTS_CONTROLLED, fp);
		cfread(&robot_last_send_time[0], 4, MAX_ROBOTS_CONTROLLED, fp);
		cfread(&robot_last_message_time[0], 4, MAX_ROBOTS_CONTROLLED, fp);
		cfread(&robot_send_pending[0], 4, MAX_ROBOTS_CONTROLLED, fp);
		cfread(&robot_fired[0], 4, MAX_ROBOTS_CONTROLLED, fp);
 
      for (i=0;i<MAX_ROBOTS_CONTROLLED;i++)
			cfread(&robot_fire_buf[i][0], 21, 1, fp);
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

			cfread(&dummy_player, sizeof(player), 1, fp);
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
			cfread(&Players[Player_num], sizeof(player), 1, fp);
		}
	}
	strcpy( Players[Player_num].callsign, org_callsign );

// Set the right level
	if ( between_levels )
		Players[Player_num].level = next_level;

// Restore the weapon states
	cfread(&Primary_weapon, sizeof(byte), 1, fp);
	cfread(&Secondary_weapon, sizeof(byte), 1, fp);

	select_weapon(Primary_weapon, 0, 0, 0);
	select_weapon(Secondary_weapon, 1, 0, 0);

// Restore the difficulty level
	cfread(&Difficulty_level, sizeof(int), 1, fp);

// Restore the cheats enabled flag
 
	cfread(&Cheats_enabled, sizeof(int), 1, fp);

	if ( !between_levels )	{
		Do_appearance_effect = 0;			// Don't do this for middle o' game stuff.

		ObjectStartLocation = cftell(fp);
		//Clear out all the objects from the lvl file
		for (segnum=0; segnum <= Highest_segment_index; segnum++)
			Segments[segnum].objects = -1;
		reset_objects(1);
	
		//Read objects, and pop 'em into their respective segments.
		cfread(&i, sizeof(int), 1, fp);
		Highest_object_index = i-1;
		cfread(Objects, sizeof(object), i, fp);
	
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
		cfread(&i, sizeof(int), 1, fp);
		Num_walls = i;
		cfread(Walls, sizeof(wall), Num_walls, fp);

		//now that we have the walls, check if any sounds are linked to
		//walls that are now open
		for (i=0;i<Num_walls;i++) {
			if (Walls[i].type == WALL_OPEN)
				digi_kill_sound_linked_to_segment(Walls[i].segnum,Walls[i].sidenum,-1);	//-1 means kill any sound
		}

		//Restore exploding wall info
		if (version >= 10) {
			cfread(&i, sizeof(int), 1, fp);
			cfread(expl_wall_list, sizeof(*expl_wall_list), i, fp);
		}

		//Restore door info
		cfread(&i, sizeof(int), 1, fp);
		Num_open_doors = i;
		cfread(ActiveDoors, sizeof(active_door), Num_open_doors, fp);
	
		if (version >= 14) {		//Restore cloaking wall info
			cfread(&i, sizeof(int), 1, fp);
			Num_cloaking_walls = i;
			cfread(CloakingWalls, sizeof(cloaking_wall), Num_cloaking_walls, fp);
		}
	
		//Restore trigger info
		cfread(&Num_triggers, sizeof(int), 1, fp);
		cfread(Triggers, sizeof(trigger), Num_triggers, fp);
	
		//Restore tmap info
		for (i=0; i<=Highest_segment_index; i++ )	{
			for (j=0; j<6; j++ )	{
				cfread(&Segments[i].sides[j].wall_num, sizeof(short), 1, fp);
				cfread(&Segments[i].sides[j].tmap_num, sizeof(short), 1, fp);
				cfread(&Segments[i].sides[j].tmap_num2, sizeof(short), 1, fp);
			}
		}
	
		//Restore the fuelcen info
		cfread(&Control_center_destroyed, sizeof(int), 1, fp);
		cfread(&Countdown_timer, sizeof(int), 1, fp);
		cfread(&Num_robot_centers, sizeof(int), 1, fp);
		cfread(RobotCenters, sizeof(matcen_info), Num_robot_centers, fp);
		cfread(&ControlCenterTriggers, sizeof(control_center_triggers), 1, fp);
		cfread(&Num_fuelcenters, sizeof(int), 1, fp);
		cfread(Station, sizeof(FuelCenter), Num_fuelcenters, fp);
	
		// Restore the control cen info
		cfread(&Control_center_been_hit, sizeof(int), 1, fp);
		cfread(&Control_center_player_been_seen, sizeof(int), 1, fp);
		cfread(&Control_center_next_fire_time, sizeof(int), 1, fp);
		cfread(&Control_center_present, sizeof(int), 1, fp);
		cfread(&Dead_controlcen_object_num, sizeof(int), 1, fp);
	
		// Restore the AI state
		ai_restore_state( fp, version );
	
		// Restore the automap visited info
		cfread( Automap_visited, sizeof(ubyte), MAX_SEGMENTS, fp);

		//	Restore hacked up weapon system stuff.
		Fusion_next_sound_time = GameTime;
		Auto_fire_fusion_cannon_time = 0;
		Next_laser_fire_time = GameTime;
		Next_missile_fire_time = GameTime;
		Last_laser_fired_time = GameTime;

	}
	state_game_id = 0;

	if ( version >= 7 )	{
		cfread(&state_game_id, sizeof(uint), 1, fp);
		cfread(&Laser_rapid_fire, sizeof(int), 1, fp);
		cfread(&Lunacy, sizeof(int), 1, fp);		//	Yes, writing this twice.  Removed the Ugly robot system, but didn't want to change savegame format.
		cfread(&Lunacy, sizeof(int), 1, fp);
		if ( Lunacy )
			do_lunacy_on();
	}

	if (version >= 17) {
		cfread(MarkerObject, sizeof(MarkerObject), 1, fp);
		cfread(MarkerOwner, sizeof(MarkerOwner), 1, fp);
		cfread(MarkerMessage, sizeof(MarkerMessage), 1, fp);
	}
	else {
		int num,dummy;

		// skip dummy info

		cfread(&num, sizeof(int), 1, fp);       //was NumOfMarkers
		cfread(&dummy, sizeof(int), 1, fp);     //was CurMarker

		cfseek(fp, num * (sizeof(vms_vector) + 40), SEEK_CUR);

		for (num=0;num<NUM_MARKERS;num++)
			MarkerObject[num] = -1;
	}

	if (version>=11) {
		if (secret_restore != 1)
			cfread(&Afterburner_charge, sizeof(fix), 1, fp);
		else {
			fix	dummy_fix;
			cfread(&dummy_fix,sizeof(fix), 1, fp);
		}
	}
	if (version>=12) {
		//read last was super information
		cfread(&Primary_last_was_super, sizeof(Primary_last_was_super), 1, fp);
		cfread(&Secondary_last_was_super, sizeof(Secondary_last_was_super), 1, fp);
	}

	if (version >= 12) {
		cfread(&Flash_effect, sizeof(int), 1, fp);
		cfread(&Time_flash_last_played, sizeof(int), 1, fp);
		cfread(&PaletteRedAdd, sizeof(int), 1, fp);
		cfread(&PaletteGreenAdd, sizeof(int), 1, fp);
		cfread(&PaletteBlueAdd, sizeof(int), 1, fp);
	} else {
		Flash_effect = 0;
		Time_flash_last_played = 0;
		PaletteRedAdd = 0;
		PaletteGreenAdd = 0;
		PaletteBlueAdd = 0;
	}

	//	Load Light_subtracted
	if (version >= 16) {
		cfread(Light_subtracted, sizeof(Light_subtracted[0]), MAX_SEGMENTS, fp);
		apply_all_changed_light();
		compute_all_static_light();	//	set static_light field in segment struct.  See note at that function.
	} else {
		int	i;
		for (i=0; i<=Highest_segment_index; i++)
			Light_subtracted[i] = 0;
	}
   
	if (!secret_restore) {
		if (version >= 20) {
			cfread(&First_secret_visit, sizeof(First_secret_visit), 1, fp);
			mprintf((0, "File: [%s] Read First_secret_visit: New value = %i\n", filename, First_secret_visit));
		} else
			First_secret_visit = 1;
	} else
		First_secret_visit = 0;

	if (version >= 22)
	{
		if (secret_restore != 1)
			cfread(&Omega_charge,sizeof(fix), 1, fp);
		else {
			fix	dummy_fix;
			cfread(&dummy_fix,sizeof(fix), 1, fp);
		}
	}

	cfclose(fp);
 
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
	CFILE *fp;
	int between_levels;
	char mission[16];
	char desc[DESC_LENGTH+1];
	char id[5];
	int dumbint;

mprintf((0, "Restoring multigame from [%s]\n", filename));

	fp = cfopen(filename, "rb");
	if ( !fp ) return 0;

//Read id
	cfread(id, sizeof(char)*4, 1, fp);
	if ( memcmp( id, dgss_id, 4 )) {
		cfclose(fp);
		return 0;
	}

//Read version
	cfread(&version, sizeof(int), 1, fp);
	if (version < STATE_COMPATIBLE_VERSION)	{
		cfclose(fp);
		return 0;
	}

// Read description
	cfread(desc, sizeof(char)*DESC_LENGTH, 1, fp);

// Skip the current screen shot...
	cfseek(fp, THUMBNAIL_W*THUMBNAIL_H, SEEK_CUR);

// And now...skip the palette stuff that somebody forgot to add
	cfseek(fp, 768, SEEK_CUR);

// Read the Between levels flag...
	cfread(&between_levels, sizeof(int), 1, fp);

	Assert(between_levels == 0);	//between levels save ripped out

// Read the mission info...
	cfread(mission, sizeof(char), 9, fp);
//Read level info
	cfread(&dumbint, sizeof(int), 1, fp);
	cfread(&dumbint, sizeof(int), 1, fp);

//Restore GameTime
	cfread(&dumbint, sizeof(fix), 1, fp);

	cfread(&state_game_id, sizeof(int), 1, fp);

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

