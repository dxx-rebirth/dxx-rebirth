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
 * $Source: /cvs/cvsroot/d2x/main/bm.c,v $
 * $Revision: 1.5 $
 * $Author: bradleyb $
 * $Date: 2001-10-25 02:15:55 $
 *
 * Bitmap and palette loading functions.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.4  2001/10/19 08:06:20  bradleyb
 * Partial application of linux/alpha patch.  Courtesy of Falk Hueffner <falk.hueffner@student.uni-tuebingen.de>
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "pstypes.h"
#include "inferno.h"
#include "gr.h"
#include "bm.h"
#include "u_mem.h"
#include "mono.h"
#include "error.h"
#include "object.h"
#include "vclip.h"
#include "effects.h"
#include "polyobj.h"
#include "wall.h"
#include "textures.h"
#include "game.h"
#ifdef NETWORK
#include "multi.h"
#endif
#include "iff.h"
#include "cfile.h"
#include "powerup.h"
#include "sounds.h"
#include "piggy.h"
#include "aistruct.h"
#include "robot.h"
#include "weapon.h"
#include "gauges.h"
#include "player.h"
#include "endlevel.h"
#include "cntrlcen.h"
#include "byteswap.h"

ubyte Sounds[MAX_SOUNDS];
ubyte AltSounds[MAX_SOUNDS];

#ifdef EDITOR
int Num_total_object_types;
byte	ObjType[MAX_OBJTYPE];
byte	ObjId[MAX_OBJTYPE];
fix	ObjStrength[MAX_OBJTYPE];
#endif

//for each model, a model number for dying & dead variants, or -1 if none
int Dying_modelnums[MAX_POLYGON_MODELS];
int Dead_modelnums[MAX_POLYGON_MODELS];

//the polygon model number to use for the marker
int	Marker_model_num = -1;

//right now there's only one player ship, but we can have another by 
//adding an array and setting the pointer to the active ship.
player_ship only_player_ship,*Player_ship=&only_player_ship;

//----------------- Miscellaneous bitmap pointers ---------------
int					Num_cockpits = 0;
bitmap_index		cockpit_bitmap[N_COCKPIT_BITMAPS];

//---------------- Variables for wall textures ------------------
int 					Num_tmaps;
tmap_info 			TmapInfo[MAX_TEXTURES];

//---------------- Variables for object textures ----------------

int					First_multi_bitmap_num=-1;

bitmap_index		ObjBitmaps[MAX_OBJ_BITMAPS];
ushort				ObjBitmapPtrs[MAX_OBJ_BITMAPS];		// These point back into ObjBitmaps, since some are used twice.

#ifdef PORTABLE_LOADER
void read_tmap_info(CFILE *fp, int inNumTexturesToRead, int inOffset)
{
	int i;
	
	for (i = inOffset; i < (inNumTexturesToRead + inOffset); i++)
	{
		TmapInfo[i].flags = cfile_read_byte(fp);
		TmapInfo[i].pad[0] = cfile_read_byte(fp);
		TmapInfo[i].pad[1] = cfile_read_byte(fp);
		TmapInfo[i].pad[2] = cfile_read_byte(fp);
		TmapInfo[i].lighting = cfile_read_fix(fp);
		TmapInfo[i].damage = cfile_read_fix(fp);
		TmapInfo[i].eclip_num = cfile_read_short(fp);
		TmapInfo[i].destroyed = cfile_read_short(fp);
		TmapInfo[i].slide_u = cfile_read_short(fp);
		TmapInfo[i].slide_v = cfile_read_short(fp);
	}
}

void read_vclip_info(CFILE *fp, int inNumVClipsToRead, int inOffset)
{
	int i, j;
	
	for (i = inOffset; i < (inNumVClipsToRead + inOffset); i++)
	{
		Vclip[i].play_time = cfile_read_fix(fp);
		Vclip[i].num_frames = cfile_read_int(fp);
		Vclip[i].frame_time = cfile_read_fix(fp);
		Vclip[i].flags = cfile_read_int(fp);
		Vclip[i].sound_num = cfile_read_short(fp);
		for (j = 0; j < VCLIP_MAX_FRAMES; j++)
			Vclip[i].frames[j].index = cfile_read_short(fp);
		Vclip[i].light_value = cfile_read_fix(fp);
	}
}

void read_effect_info(CFILE *fp, int inNumEffectsToRead, int inOffset)
{
	int i, j;


	for (i = inOffset; i < (inNumEffectsToRead + inOffset); i++)
	{
		Effects[i].vc.play_time = cfile_read_fix(fp);
		Effects[i].vc.num_frames = cfile_read_int(fp);
		Effects[i].vc.frame_time = cfile_read_fix(fp);
		Effects[i].vc.flags = cfile_read_int(fp);
		Effects[i].vc.sound_num = cfile_read_short(fp);
		for (j = 0; j < VCLIP_MAX_FRAMES; j++)
			Effects[i].vc.frames[j].index = cfile_read_short(fp);
		Effects[i].vc.light_value = cfile_read_fix(fp);
		Effects[i].time_left = cfile_read_fix(fp);
		Effects[i].frame_count = cfile_read_int(fp);
		Effects[i].changing_wall_texture = cfile_read_short(fp);
		Effects[i].changing_object_texture = cfile_read_short(fp);
		Effects[i].flags = cfile_read_int(fp);
		Effects[i].crit_clip = cfile_read_int(fp);
		Effects[i].dest_bm_num = cfile_read_int(fp);
		Effects[i].dest_vclip = cfile_read_int(fp);
		Effects[i].dest_eclip = cfile_read_int(fp);
		Effects[i].dest_size = cfile_read_fix(fp);
		Effects[i].sound_num = cfile_read_int(fp);
		Effects[i].segnum = cfile_read_int(fp);
		Effects[i].sidenum = cfile_read_int(fp);
	}
}

void read_wallanim_info(CFILE *fp, int inNumWallAnimsToRead, int inOffset)
{
	int i, j;
	
	for (i = inOffset; i < (inNumWallAnimsToRead + inOffset); i++)
	{
		WallAnims[i].play_time = cfile_read_fix(fp);;
		WallAnims[i].num_frames = cfile_read_short(fp);;
		for (j = 0; j < MAX_CLIP_FRAMES; j++)
			WallAnims[i].frames[j] = cfile_read_short(fp);
		WallAnims[i].open_sound = cfile_read_short(fp);
		WallAnims[i].close_sound = cfile_read_short(fp);
		WallAnims[i].flags = cfile_read_short(fp);
		cfread(WallAnims[i].filename, 13, 1, fp);
		WallAnims[i].pad = cfile_read_byte(fp);
	}		
}

void read_robot_info(CFILE *fp, int inNumRobotsToRead, int inOffset)
{
	int i, j, k;
	
	for (i = inOffset; i < (inNumRobotsToRead + inOffset); i++)
	{
		Robot_info[i].model_num = cfile_read_int(fp);
		for (j = 0; j < MAX_GUNS; j++)
			cfile_read_vector(&(Robot_info[i].gun_points[j]), fp);
		for (j = 0; j < MAX_GUNS; j++)
			Robot_info[i].gun_submodels[j] = cfile_read_byte(fp);

		Robot_info[i].exp1_vclip_num = cfile_read_short(fp);
		Robot_info[i].exp1_sound_num = cfile_read_short(fp);

		Robot_info[i].exp2_vclip_num = cfile_read_short(fp);
		Robot_info[i].exp2_sound_num = cfile_read_short(fp);

		Robot_info[i].weapon_type = cfile_read_byte(fp);
		Robot_info[i].weapon_type2 = cfile_read_byte(fp);
		Robot_info[i].n_guns = cfile_read_byte(fp);
		Robot_info[i].contains_id = cfile_read_byte(fp);

		Robot_info[i].contains_count = cfile_read_byte(fp);
		Robot_info[i].contains_prob = cfile_read_byte(fp);
		Robot_info[i].contains_type = cfile_read_byte(fp);
		Robot_info[i].kamikaze = cfile_read_byte(fp);

		Robot_info[i].score_value = cfile_read_short(fp);
		Robot_info[i].badass = cfile_read_byte(fp);
		Robot_info[i].energy_drain = cfile_read_byte(fp);
		
		Robot_info[i].lighting = cfile_read_fix(fp);
		Robot_info[i].strength = cfile_read_fix(fp);

		Robot_info[i].mass = cfile_read_fix(fp);
		Robot_info[i].drag = cfile_read_fix(fp);

		for (j = 0; j < NDL; j++)
			Robot_info[i].field_of_view[j] = cfile_read_fix(fp);
		for (j = 0; j < NDL; j++)
			Robot_info[i].firing_wait[j] = cfile_read_fix(fp);
		for (j = 0; j < NDL; j++)
			Robot_info[i].firing_wait2[j] = cfile_read_fix(fp);
		for (j = 0; j < NDL; j++)
			Robot_info[i].turn_time[j] = cfile_read_fix(fp);
		for (j = 0; j < NDL; j++)
			Robot_info[i].max_speed[j] = cfile_read_fix(fp);
		for (j = 0; j < NDL; j++)
			Robot_info[i].circle_distance[j] = cfile_read_fix(fp);
		for (j = 0; j < NDL; j++)
			cfread(&(Robot_info[i].rapidfire_count[j]), sizeof(byte), 1, fp);
		for (j = 0; j < NDL; j++)
			cfread(&(Robot_info[i].evade_speed[j]), sizeof(byte), 1, fp);
		Robot_info[i].cloak_type = cfile_read_byte(fp);
		Robot_info[i].attack_type = cfile_read_byte(fp);

		Robot_info[i].see_sound = cfile_read_byte(fp);
		Robot_info[i].attack_sound = cfile_read_byte(fp);
		Robot_info[i].claw_sound = cfile_read_byte(fp);
		Robot_info[i].taunt_sound = cfile_read_byte(fp);

		Robot_info[i].boss_flag = cfile_read_byte(fp);
		Robot_info[i].companion = cfile_read_byte(fp);
		Robot_info[i].smart_blobs = cfile_read_byte(fp);
		Robot_info[i].energy_blobs = cfile_read_byte(fp);

		Robot_info[i].thief = cfile_read_byte(fp);
		Robot_info[i].pursuit = cfile_read_byte(fp);
		Robot_info[i].lightcast = cfile_read_byte(fp);
		Robot_info[i].death_roll = cfile_read_byte(fp);

		Robot_info[i].flags = cfile_read_byte(fp);
		Robot_info[i].pad[0] = cfile_read_byte(fp);
		Robot_info[i].pad[1] = cfile_read_byte(fp);
		Robot_info[i].pad[2] = cfile_read_byte(fp);

		Robot_info[i].deathroll_sound = cfile_read_byte(fp);
		Robot_info[i].glow = cfile_read_byte(fp);
		Robot_info[i].behavior = cfile_read_byte(fp);
		Robot_info[i].aim = cfile_read_byte(fp);

		for (j = 0; j < MAX_GUNS + 1; j++) {
			for (k = 0; k < N_ANIM_STATES; k++) {
				Robot_info[i].anim_states[j][k].n_joints = cfile_read_short(fp);
				Robot_info[i].anim_states[j][k].offset = cfile_read_short(fp);
			}
		}

		Robot_info[i].always_0xabcd = cfile_read_int(fp);
	}
}

void read_robot_joint_info(CFILE *fp, int inNumRobotJointsToRead, int inOffset)
{
	int i;

	for (i = inOffset; i < (inNumRobotJointsToRead + inOffset); i++)
	{
		Robot_joints[i].jointnum = cfile_read_short(fp);
		cfile_read_angvec(&(Robot_joints[i].angles), fp);
	}
}

void read_weapon_info(CFILE *fp, int inNumWeaponsToRead, int inOffset)
{
	int i, j;
	
	for (i = inOffset; i < (inNumWeaponsToRead + inOffset); i++)
	{
		Weapon_info[i].render_type = cfile_read_byte(fp);
		Weapon_info[i].persistent = cfile_read_byte(fp);
		Weapon_info[i].model_num = cfile_read_short(fp);
		Weapon_info[i].model_num_inner = cfile_read_short(fp);

		Weapon_info[i].flash_vclip = cfile_read_byte(fp);
		Weapon_info[i].robot_hit_vclip = cfile_read_byte(fp);
		Weapon_info[i].flash_sound = cfile_read_short(fp);		

		Weapon_info[i].wall_hit_vclip = cfile_read_byte(fp);
		Weapon_info[i].fire_count = cfile_read_byte(fp);
		Weapon_info[i].robot_hit_sound = cfile_read_short(fp);
		
		Weapon_info[i].ammo_usage = cfile_read_byte(fp);
		Weapon_info[i].weapon_vclip = cfile_read_byte(fp);
		Weapon_info[i].wall_hit_sound = cfile_read_short(fp);		

		Weapon_info[i].destroyable = cfile_read_byte(fp);
		Weapon_info[i].matter = cfile_read_byte(fp);
		Weapon_info[i].bounce = cfile_read_byte(fp);
		Weapon_info[i].homing_flag = cfile_read_byte(fp);

		Weapon_info[i].speedvar = cfile_read_byte(fp);
		Weapon_info[i].flags = cfile_read_byte(fp);
		Weapon_info[i].flash = cfile_read_byte(fp);
		Weapon_info[i].afterburner_size = cfile_read_byte(fp);
		
		Weapon_info[i].children = cfile_read_byte(fp);

		Weapon_info[i].energy_usage = cfile_read_fix(fp);
		Weapon_info[i].fire_wait = cfile_read_fix(fp);
		
		Weapon_info[i].multi_damage_scale = cfile_read_fix(fp);
		
		Weapon_info[i].bitmap.index = cfile_read_short(fp);	// bitmap_index = short

		Weapon_info[i].blob_size = cfile_read_fix(fp);
		Weapon_info[i].flash_size = cfile_read_fix(fp);
		Weapon_info[i].impact_size = cfile_read_fix(fp);
		for (j = 0; j < NDL; j++)
			Weapon_info[i].strength[j] = cfile_read_fix(fp);
		for (j = 0; j < NDL; j++)
			Weapon_info[i].speed[j] = cfile_read_fix(fp);
		Weapon_info[i].mass = cfile_read_fix(fp);
		Weapon_info[i].drag = cfile_read_fix(fp);
		Weapon_info[i].thrust = cfile_read_fix(fp);
		Weapon_info[i].po_len_to_width_ratio = cfile_read_fix(fp);
		Weapon_info[i].light = cfile_read_fix(fp);
		Weapon_info[i].lifetime = cfile_read_fix(fp);
		Weapon_info[i].damage_radius = cfile_read_fix(fp);
		Weapon_info[i].picture.index = cfile_read_short(fp);		// bitmap_index is a short
		Weapon_info[i].hires_picture.index = cfile_read_short(fp);		// bitmap_index is a short
	}
}

void read_powerup_info(CFILE *fp, int inNumPowerupsToRead, int inOffset)
{
	int i;
	
	for (i = inOffset; i < (inNumPowerupsToRead + inOffset); i++)
	{
		Powerup_info[i].vclip_num = cfile_read_int(fp);
		Powerup_info[i].hit_sound = cfile_read_int(fp);
		Powerup_info[i].size = cfile_read_fix(fp);
		Powerup_info[i].light = cfile_read_fix(fp);
	}
}

void read_polygon_models(CFILE *fp, int inNumPolygonModelsToRead, int inOffset)
{
	int i, j;

	for (i = inOffset; i < (inNumPolygonModelsToRead + inOffset); i++)
	{
		Polygon_models[i].n_models = cfile_read_int(fp);
		Polygon_models[i].model_data_size = cfile_read_int(fp);
		Polygon_models[i].model_data = (ubyte *) cfile_read_int(fp);
		for (j = 0; j < MAX_SUBMODELS; j++)
			Polygon_models[i].submodel_ptrs[j] = cfile_read_int(fp);
		for (j = 0; j < MAX_SUBMODELS; j++)
			cfile_read_vector(&(Polygon_models[i].submodel_offsets[j]), fp);
		for (j = 0; j < MAX_SUBMODELS; j++)
			cfile_read_vector(&(Polygon_models[i].submodel_norms[j]), fp);
		for (j = 0; j < MAX_SUBMODELS; j++)
			cfile_read_vector(&(Polygon_models[i].submodel_pnts[j]), fp);
		for (j = 0; j < MAX_SUBMODELS; j++)
			Polygon_models[i].submodel_rads[j] = cfile_read_fix(fp);
		for (j = 0; j < MAX_SUBMODELS; j++)
			Polygon_models[i].submodel_parents[j] = cfile_read_byte(fp);
		for (j = 0; j < MAX_SUBMODELS; j++)
			cfile_read_vector(&(Polygon_models[i].submodel_mins[j]), fp);
		for (j = 0; j < MAX_SUBMODELS; j++)
			cfile_read_vector(&(Polygon_models[i].submodel_maxs[j]), fp);
		cfile_read_vector(&(Polygon_models[i].mins), fp);
		cfile_read_vector(&(Polygon_models[i].maxs), fp);
		Polygon_models[i].rad = cfile_read_fix(fp);		
		Polygon_models[i].n_textures = cfile_read_byte(fp);
		Polygon_models[i].first_texture = cfile_read_short(fp);
		Polygon_models[i].simpler_model = cfile_read_byte(fp);
	}
}

void read_player_ship(CFILE *fp)
{
	int i;

	only_player_ship.model_num = cfile_read_int(fp);
	only_player_ship.expl_vclip_num = cfile_read_int(fp);
	only_player_ship.mass = cfile_read_fix(fp);
	only_player_ship.drag = cfile_read_fix(fp);
	only_player_ship.max_thrust = cfile_read_fix(fp);
	only_player_ship.reverse_thrust = cfile_read_fix(fp);
	only_player_ship.brakes = cfile_read_fix(fp);
	only_player_ship.wiggle = cfile_read_fix(fp);
	only_player_ship.max_rotthrust = cfile_read_fix(fp);
	for (i = 0; i < N_PLAYER_GUNS; i++)
		cfile_read_vector(&(only_player_ship.gun_points[i]), fp);
}

void read_reactor_info(CFILE *fp, int inNumReactorsToRead, int inOffset)
{
	int i, j;
	
	for (i = inOffset; i < (inNumReactorsToRead + inOffset); i++)
	{
		Reactors[i].model_num = cfile_read_int(fp);
		Reactors[i].n_guns = cfile_read_int(fp);
		for (j = 0; j < MAX_CONTROLCEN_GUNS; j++)
			cfile_read_vector(&(Reactors[i].gun_points[j]), fp);
		for (j = 0; j < MAX_CONTROLCEN_GUNS; j++)
			cfile_read_vector(&(Reactors[i].gun_dirs[j]), fp);
	}
}

#ifdef SHAREWARE
extern int exit_modelnum,destroyed_exit_modelnum, Num_bitmap_files;
int N_ObjBitmaps, extra_bitmap_num;

bitmap_index exitmodel_bm_load_sub( char * filename )
{
	bitmap_index bitmap_num;
	grs_bitmap * new;
	ubyte newpal[256*3];
	int i, iff_error;		//reference parm to avoid warning message

	bitmap_num.index = 0;

	MALLOC( new, grs_bitmap, 1 );
	iff_error = iff_read_bitmap(filename,new,BM_LINEAR,newpal);
	new->bm_handle=0;
	if (iff_error != IFF_NO_ERROR)		{
		Error("Error loading exit model bitmap <%s> - IFF error: %s",filename,iff_errormsg(iff_error));
	}
	
	if ( iff_has_transparency )
		gr_remap_bitmap_good( new, newpal, iff_transparent_color, 254 );
	else
		gr_remap_bitmap_good( new, newpal, -1, 254 );

	new->avg_color = 0;	//compute_average_pixel(new);

	bitmap_num.index = extra_bitmap_num;

	GameBitmaps[extra_bitmap_num++] = *new;
	
	d_free( new );
	return bitmap_num;
}

grs_bitmap *load_exit_model_bitmap(char *name)
{
	Assert(N_ObjBitmaps < MAX_OBJ_BITMAPS);

	{
		ObjBitmaps[N_ObjBitmaps] = exitmodel_bm_load_sub(name);
		if (GameBitmaps[ObjBitmaps[N_ObjBitmaps].index].bm_w!=64 || GameBitmaps[ObjBitmaps[N_ObjBitmaps].index].bm_h!=64)
			Error("Bitmap <%s> is not 64x64",name);
		ObjBitmapPtrs[N_ObjBitmaps] = N_ObjBitmaps;
		N_ObjBitmaps++;
		Assert(N_ObjBitmaps < MAX_OBJ_BITMAPS);
		return &GameBitmaps[ObjBitmaps[N_ObjBitmaps-1].index];
	}
}

void load_exit_models()
{
	CFILE *exit_hamfile;
	int i, j;
	ubyte pal[768];
	int start_num;

	start_num = N_ObjBitmaps;
	extra_bitmap_num = Num_bitmap_files;
	load_exit_model_bitmap("steel1.bbm");
	load_exit_model_bitmap("rbot061.bbm");
	load_exit_model_bitmap("rbot062.bbm");

	load_exit_model_bitmap("steel1.bbm");
	load_exit_model_bitmap("rbot061.bbm");
	load_exit_model_bitmap("rbot063.bbm");

	exit_hamfile = cfopen(":Data:exit.ham","rb");

	exit_modelnum = N_polygon_models++;
	destroyed_exit_modelnum = N_polygon_models++;

#ifndef PORTABLE_LOADER
	cfread( &Polygon_models[exit_modelnum], sizeof(polymodel), 1, exit_hamfile );
	cfread( &Polygon_models[destroyed_exit_modelnum], sizeof(polymodel), 1, exit_hamfile );
	#else
	for (i = exit_modelnum; i <= destroyed_exit_modelnum; i++) {
		Polygon_models[i].n_models = cfile_read_int(exit_hamfile);
		Polygon_models[i].model_data_size = cfile_read_int(exit_hamfile);
		Polygon_models[i].model_data = (ubyte *)read_int_swap(exit_hamfile);
		for (j = 0; j < MAX_SUBMODELS; j++)
			Polygon_models[i].submodel_ptrs[j] = cfile_read_int(exit_hamfile);
		for (j = 0; j < MAX_SUBMODELS; j++)
			cfile_read_vector(&(Polygon_models[i].submodel_offsets), exit_hamfile);
		for (j = 0; j < MAX_SUBMODELS; j++)
			cfile_read_vector(&(Polygon_models[i].submodel_norms), exit_hamfile);
		for (j = 0; j < MAX_SUBMODELS; j++)
			cfile_read_vector(&(Polygon_models[i].submodel_pnts), exit_hamfile);
		for (j = 0; j < MAX_SUBMODELS; j++)
			Polygon_models[i].submodel_rads[j] = cfile_read_fix(exit_hamfile);
		for (j = 0; j < MAX_SUBMODELS; j++)
			Polygon_models[i].submodel_parents[j] = cfile_read_byte(exit_hamfile);
		for (j = 0; j < MAX_SUBMODELS; j++)
			cfile_read_vector(&(Polygon_models[i].submodel_mins), exit_hamfile);
		for (j = 0; j < MAX_SUBMODELS; j++)
			cfile_read_vector(&(Polygon_models[i].submodel_maxs), exit_hamfile);
		cfile_read_vector(&(Polygon_models[i].mins), exit_hamfile);
		cfile_read_vector(&(Polygon_models[i].maxs), exit_hamfile);
		Polygon_models[i].rad = cfile_read_fix(exit_hamfile);		
		Polygon_models[i].n_textures = cfile_read_byte(exit_hamfile);
		Polygon_models[i].first_texture = cfile_read_short(exit_hamfile);
		Polygon_models[i].simpler_model = cfile_read_byte(exit_hamfile);
	}
	Polygon_models[exit_modelnum].first_texture = start_num;
	Polygon_models[destroyed_exit_modelnum].first_texture = start_num+3;
	#endif

	Polygon_models[exit_modelnum].model_data = d_malloc(Polygon_models[exit_modelnum].model_data_size);
	Assert( Polygon_models[exit_modelnum].model_data != NULL );
	cfread( Polygon_models[exit_modelnum].model_data, sizeof(ubyte), Polygon_models[exit_modelnum].model_data_size, exit_hamfile );
	#ifdef MACINTOSH
	swap_polygon_model_data(Polygon_models[exit_modelnum].model_data);
	#endif
	g3_init_polygon_model(Polygon_models[exit_modelnum].model_data);

	Polygon_models[destroyed_exit_modelnum].model_data = d_malloc(Polygon_models[destroyed_exit_modelnum].model_data_size);
	Assert( Polygon_models[destroyed_exit_modelnum].model_data != NULL );
	cfread( Polygon_models[destroyed_exit_modelnum].model_data, sizeof(ubyte), Polygon_models[destroyed_exit_modelnum].model_data_size, exit_hamfile );
	#ifdef MACINTOSH
	swap_polygon_model_data(Polygon_models[destroyed_exit_modelnum].model_data);
	#endif
	g3_init_polygon_model(Polygon_models[destroyed_exit_modelnum].model_data);

	cfclose(exit_hamfile);

}
#endif		// SHAREWARE

#endif		// PORTABLE_LOADER

//-----------------------------------------------------------------
// Read data from piggy.
// This is called when the editor is OUT.  
// If editor is in, bm_init_use_table() is called.
int bm_init()
{
	init_polygon_models();
	if (! piggy_init())				// This calls bm_read_all
		Error("Cannot open pig and/or ham file");

	piggy_read_sounds();

	#ifdef SHAREWARE
	init_endlevel();		//this is in bm_init_use_tbl(), so I gues it goes here
	#endif

	return 0;
}

void bm_read_all(CFILE * fp)
{
	int i,t;

	NumTextures = cfile_read_int(fp);
#ifndef PORTABLE_LOADER
	cfread( Textures, sizeof(bitmap_index), NumTextures, fp );
	cfread( TmapInfo, sizeof(tmap_info), NumTextures, fp );
#else
	for (i = 0; i < NumTextures; i++)
		Textures[i].index = cfile_read_short(fp);
	read_tmap_info(fp, NumTextures, 0);
#endif

	t = cfile_read_int(fp);	
	cfread( Sounds, sizeof(ubyte), t, fp );
	cfread( AltSounds, sizeof(ubyte), t, fp );

	Num_vclips = cfile_read_int(fp);
#ifndef PORTABLE_LOADER
	cfread( Vclip, sizeof(vclip), Num_vclips, fp );
#else
	read_vclip_info(fp, Num_vclips, 0);
#endif

	Num_effects = cfile_read_int(fp);
#ifndef PORTABLE_LOADER
	cfread( Effects, sizeof(eclip), Num_effects, fp );
#else	
	read_effect_info(fp, Num_effects, 0);
#endif

	Num_wall_anims = cfile_read_int(fp);
#ifndef PORTABLE_LOADER
	cfread( WallAnims, sizeof(wclip), Num_wall_anims, fp );
#else
	read_wallanim_info(fp, Num_wall_anims, 0);
#endif

	N_robot_types = cfile_read_int(fp);
#ifndef PORTABLE_LOADER
	cfread( Robot_info, sizeof(robot_info), N_robot_types, fp );
#else
	read_robot_info(fp, N_robot_types, 0);
#endif
	N_robot_joints = cfile_read_int(fp);
#ifndef PORTABLE_LOADER
	cfread( Robot_joints, sizeof(jointpos), N_robot_joints, fp );
#else
	read_robot_joint_info(fp, N_robot_joints, 0);
#endif

	N_weapon_types = cfile_read_int(fp);
#ifndef PORTABLE_LOADER
	cfread( Weapon_info, sizeof(weapon_info), N_weapon_types, fp );
#else
	read_weapon_info(fp, N_weapon_types, 0);
#endif

	N_powerup_types = cfile_read_int(fp);
#ifndef PORTABLE_LOADER
	cfread( Powerup_info, sizeof(powerup_type_info), N_powerup_types, fp );
#else
	read_powerup_info(fp, N_powerup_types, 0);
#endif
	
	N_polygon_models = cfile_read_int(fp);
#ifndef PORTABLE_LOADER
	cfread( Polygon_models, sizeof(polymodel), N_polygon_models, fp );
#else
	read_polygon_models(fp, N_polygon_models, 0);
#endif

	for (i=0; i<N_polygon_models; i++ )	{
		Polygon_models[i].model_data = d_malloc(Polygon_models[i].model_data_size);
		Assert( Polygon_models[i].model_data != NULL );
		cfread( Polygon_models[i].model_data, sizeof(ubyte), Polygon_models[i].model_data_size, fp );
#ifdef MACINTOSH
		swap_polygon_model_data(Polygon_models[i].model_data);
#endif
		g3_init_polygon_model(Polygon_models[i].model_data);
	}

	cfread( Dying_modelnums, sizeof(int), N_polygon_models, fp );
	cfread( Dead_modelnums, sizeof(int), N_polygon_models, fp );
#ifdef MACINTOSH
	for (i = 0; i < N_polygon_models; i++)
		Dying_modelnums[i]= SWAPINT(Dying_modelnums[i]);
	for (i = 0; i < N_polygon_models; i++)
		Dead_modelnums[i]= SWAPINT(Dead_modelnums[i]);
#endif

	t = cfile_read_int(fp);
	cfread( Gauges, sizeof(bitmap_index), t, fp );
	cfread( Gauges_hires, sizeof(bitmap_index), t, fp );
#ifdef MACINTOSH
	for (i = 0; i < t; i++) {
		Gauges[i].index = SWAPSHORT(Gauges[i].index);
		Gauges_hires[i].index = SWAPSHORT(Gauges_hires[i].index);
	}
#endif

	t = cfile_read_int(fp);
	cfread( ObjBitmaps, sizeof(bitmap_index), t, fp );
	cfread( ObjBitmapPtrs, sizeof(ushort), t, fp );

#ifdef MACINTOSH
#ifdef SHAREWARE
	N_ObjBitmaps = t;
#endif
	for (i = 0; i < t; i++) {
		ObjBitmaps[i].index = SWAPSHORT(ObjBitmaps[i].index);
		ObjBitmapPtrs[i] = SWAPSHORT(ObjBitmapPtrs[i]);
	}
#endif

#ifndef PORTABLE_LOADER
	cfread( &only_player_ship, sizeof(player_ship), 1, fp );
#else
	read_player_ship(fp);
#endif

	Num_cockpits = cfile_read_int(fp);
	cfread( cockpit_bitmap, sizeof(bitmap_index), Num_cockpits, fp );
#ifdef PORTABLE_LOADER
	for (i = 0; i < Num_cockpits; i++)
		cockpit_bitmap[i].index = SWAPSHORT(cockpit_bitmap[i].index);
#endif

//@@	cfread( &Num_total_object_types, sizeof(int), 1, fp );
//@@	cfread( ObjType, sizeof(byte), Num_total_object_types, fp );
//@@	cfread( ObjId, sizeof(byte), Num_total_object_types, fp );
//@@	cfread( ObjStrength, sizeof(fix), Num_total_object_types, fp );

	First_multi_bitmap_num = cfile_read_int(fp);

	Num_reactors = cfile_read_int(fp);
#ifndef MACINTOSH
	cfread( Reactors, sizeof(*Reactors), Num_reactors, fp);
#else
	read_reactor_info(fp, Num_reactors, 0);
#endif

	Marker_model_num = cfile_read_int(fp);

	//@@cfread( &N_controlcen_guns, sizeof(int), 1, fp );
	//@@cfread( controlcen_gun_points, sizeof(vms_vector), N_controlcen_guns, fp );
	//@@cfread( controlcen_gun_dirs, sizeof(vms_vector), N_controlcen_guns, fp );

	#ifdef SHAREWARE
	exit_modelnum = cfile_read_int(fp);
	destroyed_exit_modelnum = cfile_read_int(fp);
	#endif

}


//these values are the number of each item in the release of d2
//extra items added after the release get written in an additional hamfile
#define N_D2_ROBOT_TYPES		66
#define N_D2_ROBOT_JOINTS		1145
#define N_D2_POLYGON_MODELS		166
#define N_D2_OBJBITMAPS			422
#define N_D2_OBJBITMAPPTRS		502
#define N_D2_WEAPON_TYPES		62

//type==1 means 1.1, type==2 means 1.2 (with weaons)
void bm_read_extra_robots(char *fname,int type)
{
	CFILE *fp;
	int t,i;
	int version;
	
	#ifdef MACINTOSH
		ulong varSave = 0;
	#endif

	fp = cfopen(fname,"rb");

	if (type == 2) {
		int sig;

		sig = cfile_read_int(fp);
		if (sig != 0x5848414d) /* 'XHAM' */
			return;
		version = cfile_read_int(fp);
	}
	else
		version = 0;

	//read extra weapons

	t = cfile_read_int(fp);
	N_weapon_types = N_D2_WEAPON_TYPES+t;
	if (N_weapon_types >= MAX_WEAPON_TYPES)
		Error("Too many weapons (%d) in <%s>.  Max is %d.",t,fname,MAX_WEAPON_TYPES-N_D2_WEAPON_TYPES);
	#ifdef MACINTOSH
		read_weapon_info(fp, t, N_D2_WEAPON_TYPES);
	#else
		cfread( &Weapon_info[N_D2_WEAPON_TYPES], sizeof(weapon_info), t, fp );
	#endif 
	
	//now read robot info

	t = cfile_read_int(fp);
	N_robot_types = N_D2_ROBOT_TYPES+t;
	if (N_robot_types >= MAX_ROBOT_TYPES)
		Error("Too many robots (%d) in <%s>.  Max is %d.",t,fname,MAX_ROBOT_TYPES-N_D2_ROBOT_TYPES);
	#ifdef MACINTOSH
		read_robot_info(fp, t, N_D2_ROBOT_TYPES);
	#else
		cfread( &Robot_info[N_D2_ROBOT_TYPES], sizeof(robot_info), t, fp );
	#endif
	
	t = cfile_read_int(fp);
	N_robot_joints = N_D2_ROBOT_JOINTS+t;
	if (N_robot_joints >= MAX_ROBOT_JOINTS)
		Error("Too many robot joints (%d) in <%s>.  Max is %d.",t,fname,MAX_ROBOT_JOINTS-N_D2_ROBOT_JOINTS);
	#ifdef MACINTOSH
		read_robot_joint_info(fp, t, N_D2_ROBOT_JOINTS);
	#else
		cfread( &Robot_joints[N_D2_ROBOT_JOINTS], sizeof(jointpos), t, fp );
	#endif
	
	t = cfile_read_int(fp);
	N_polygon_models = N_D2_POLYGON_MODELS+t;
	if (N_polygon_models >= MAX_POLYGON_MODELS)
		Error("Too many polygon models (%d) in <%s>.  Max is %d.",t,fname,MAX_POLYGON_MODELS-N_D2_POLYGON_MODELS);
	#ifdef MACINTOSH
		read_polygon_models(fp, t, N_D2_POLYGON_MODELS);
	#else
		cfread( &Polygon_models[N_D2_POLYGON_MODELS], sizeof(polymodel), t, fp );
	#endif
	
	for (i=N_D2_POLYGON_MODELS; i<N_polygon_models; i++ )
	{
		Polygon_models[i].model_data = d_malloc(Polygon_models[i].model_data_size);
		Assert( Polygon_models[i].model_data != NULL );
		cfread( Polygon_models[i].model_data, sizeof(ubyte), Polygon_models[i].model_data_size, fp );
		
		#ifdef MACINTOSH
			swap_polygon_model_data(Polygon_models[i].model_data);
		#endif
		
		g3_init_polygon_model(Polygon_models[i].model_data);
	}

	cfread( &Dying_modelnums[N_D2_POLYGON_MODELS], sizeof(int), t, fp );
	cfread( &Dead_modelnums[N_D2_POLYGON_MODELS], sizeof(int), t, fp );

	#ifdef MACINTOSH
		for (i = N_D2_POLYGON_MODELS; i < N_polygon_models; i++)
		{
			Dying_modelnums[i]= SWAPINT(Dying_modelnums[i]);
			Dead_modelnums[i]= SWAPINT(Dead_modelnums[i]);
		}
	#endif

	t = cfile_read_int(fp);
	if (N_D2_OBJBITMAPS+t >= MAX_OBJ_BITMAPS)
		Error("Too many object bitmaps (%d) in <%s>.  Max is %d.",t,fname,MAX_OBJ_BITMAPS-N_D2_OBJBITMAPS);
	cfread( &ObjBitmaps[N_D2_OBJBITMAPS], sizeof(bitmap_index), t, fp );
	#ifdef MACINTOSH
		for (i = N_D2_OBJBITMAPS; i < (N_D2_OBJBITMAPS + t); i++)
		{
			ObjBitmaps[i].index = SWAPSHORT(ObjBitmaps[i].index);
		}
	#endif

	t = cfile_read_int(fp);
	if (N_D2_OBJBITMAPPTRS+t >= MAX_OBJ_BITMAPS)
		Error("Too many object bitmap pointers (%d) in <%s>.  Max is %d.",t,fname,MAX_OBJ_BITMAPS-N_D2_OBJBITMAPPTRS);
	cfread( &ObjBitmapPtrs[N_D2_OBJBITMAPPTRS], sizeof(ushort), t, fp );
	#ifdef MACINTOSH
		for (i = N_D2_OBJBITMAPPTRS; i < (N_D2_OBJBITMAPPTRS + t); i++)
		{
			ObjBitmapPtrs[i] = SWAPSHORT(ObjBitmapPtrs[i]);
		}
	#endif

	cfclose(fp);
}

extern void change_filename_extension( char *dest, char *src, char *new_ext );

int Robot_replacements_loaded = 0;

void load_robot_replacements(char *level_name)
{
	CFILE *fp;
	int t,i,j;
	char ifile_name[FILENAME_LEN];

	change_filename_extension(ifile_name, level_name, ".HXM" );
	
	fp = cfopen(ifile_name,"rb");

	if (!fp)		//no robot replacement file
		return;

	t = cfile_read_int(fp);			//read id "HXM!"
	if (t!= 0x21584d48) 
		Error("ID of HXM! file incorrect");

	t = cfile_read_int(fp);			//read version
	if (t<1)
		Error("HXM! version too old (%d)",t); 

	t = cfile_read_int(fp);			//read number of robots
	for (j=0;j<t;j++) {
		i = cfile_read_int(fp);		//read robot number
	   if (i<0 || i>=N_robot_types)
			Error("Robots number (%d) out of range in (%s).  Range = [0..%d].",i,level_name,N_robot_types-1);
		#ifdef MACINTOSH
			read_robot_info(fp, 1, i);
		#else
			cfread( &Robot_info[i], sizeof(robot_info), 1, fp );
		#endif
	}

	t = cfile_read_int(fp);			//read number of joints
	for (j=0;j<t;j++) {
		i = cfile_read_int(fp);		//read joint number
		if (i<0 || i>=N_robot_joints)
			Error("Robots joint (%d) out of range in (%s).  Range = [0..%d].",i,level_name,N_robot_joints-1);
		#ifdef MACINTOSH
			read_robot_joint_info(fp, 1, i);
		#else
			cfread( &Robot_joints[i], sizeof(jointpos), 1, fp );
		#endif
	}

	t = cfile_read_int(fp);			//read number of polygon models
	for (j=0;j<t;j++)
	{
		i = cfile_read_int(fp);		//read model number
		if (i<0 || i>=N_polygon_models)
			Error("Polygon model (%d) out of range in (%s).  Range = [0..%d].",i,level_name,N_polygon_models-1);
	
		#ifdef MACINTOSH
			read_polygon_models(fp, 1, i);
		#else
			cfread( &Polygon_models[i], sizeof(polymodel), 1, fp );
		#endif
	
		d_free(Polygon_models[i].model_data);
		Polygon_models[i].model_data = d_malloc(Polygon_models[i].model_data_size);
		Assert( Polygon_models[i].model_data != NULL );

		cfread( Polygon_models[i].model_data, sizeof(ubyte), Polygon_models[i].model_data_size, fp );
		#ifdef MACINTOSH
			swap_polygon_model_data(Polygon_models[i].model_data);
		#endif
		g3_init_polygon_model(Polygon_models[i].model_data);

		Dying_modelnums[i] = cfile_read_int(fp);
		Dead_modelnums[i] = cfile_read_int(fp);
	}

	t = cfile_read_int(fp);			//read number of objbitmaps
	for (j=0;j<t;j++) {
		i = cfile_read_int(fp);		//read objbitmap number
		if (i<0 || i>=MAX_OBJ_BITMAPS)
			Error("Object bitmap number (%d) out of range in (%s).  Range = [0..%d].",i,level_name,MAX_OBJ_BITMAPS-1);
		ObjBitmaps[i].index = cfile_read_short(fp);
	}

	t = cfile_read_int(fp);			//read number of objbitmapptrs
	for (j=0;j<t;j++) {
		i = cfile_read_int(fp);		//read objbitmapptr number
		if (i<0 || i>=MAX_OBJ_BITMAPS)
			Error("Object bitmap pointer (%d) out of range in (%s).  Range = [0..%d].",i,level_name,MAX_OBJ_BITMAPS-1);
		ObjBitmapPtrs[i] = cfile_read_short(fp);
	}

	cfclose(fp);
}
