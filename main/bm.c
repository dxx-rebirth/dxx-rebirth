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
 * Bitmap and palette loading functions.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pstypes.h"
#include "inferno.h"
#include "gr.h"
#include "bm.h"
#include "u_mem.h"
#include "error.h"
#include "object.h"
#include "vclip.h"
#include "effects.h"
#include "polyobj.h"
#include "wall.h"
#include "textures.h"
#include "game.h"
#include "multi.h"
#include "iff.h"
#include "hostage.h"
#include "powerup.h"
#include "sounds.h"
#include "piggy.h"
#include "aistruct.h"
#include "robot.h"
#include "weapon.h"
#include "gauges.h"
#include "player.h"
#include "fuelcen.h"
#include "endlevel.h"
#include "cntrlcen.h"
#include "rle.h"

#ifdef EDITOR
#include "editor/texpage.h"
#endif

ubyte Sounds[MAX_SOUNDS];
ubyte AltSounds[MAX_SOUNDS];

#ifdef EDITOR
int Num_object_subtypes = 1;
#endif

int Num_total_object_types;

sbyte	ObjType[MAX_OBJTYPE];
sbyte	ObjId[MAX_OBJTYPE];
fix	ObjStrength[MAX_OBJTYPE];

//for each model, a model number for dying & dead variants, or -1 if none
int Dying_modelnums[MAX_POLYGON_MODELS];
int Dead_modelnums[MAX_POLYGON_MODELS];

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

/*
 * reads n tmap_info structs from a PHYSFS_file
 */
int tmap_info_read_n(tmap_info *ti, int n, PHYSFS_file *fp)
{
	int i;
	
	for (i = 0; i < n; i++) {
		PHYSFS_read(fp, TmapInfo[i].filename, 13, 1);
		ti[i].flags = PHYSFSX_readByte(fp);
		ti[i].lighting = PHYSFSX_readFix(fp);
		ti[i].damage = PHYSFSX_readFix(fp);
		ti[i].eclip_num = PHYSFSX_readInt(fp);
	}
	return i;
}

int player_ship_read(player_ship *ps, PHYSFS_file *fp)
{
	int i;
	ps->model_num = PHYSFSX_readInt(fp);
	ps->expl_vclip_num = PHYSFSX_readInt(fp);
	ps->mass = PHYSFSX_readFix(fp);
	ps->drag = PHYSFSX_readFix(fp);
	ps->max_thrust = PHYSFSX_readFix(fp);
	ps->reverse_thrust = PHYSFSX_readFix(fp);
	ps->brakes = PHYSFSX_readFix(fp);
	ps->wiggle = PHYSFSX_readFix(fp);
	ps->max_rotthrust = PHYSFSX_readFix(fp);
	for (i = 0; i < N_PLAYER_GUNS; i++)
		PHYSFSX_readVector(&ps->gun_points[i], fp);
	return i;
}

void gamedata_close()
{
	free_polygon_models();
	free_endlevel_data();
	rle_cache_close();
	piggy_close();
}

//-----------------------------------------------------------------
// Initializes game properties data (including texture caching system) and sound data.
int gamedata_init()
{
	int retval;
	
	init_polygon_models();
	init_endlevel();//adb: added, is also in bm_init_use_tbl (Chris: *Was* in bm_init_use_tbl)
	retval = properties_init();				// This calls properties_read_cmp if appropriate
	if (retval)
		gamedata_read_tbl(retval == PIGGY_PC_SHAREWARE);

	piggy_read_sounds(retval == PIGGY_PC_SHAREWARE);
	
	return 0;
}

// Read compiled properties data from descent.pig
void properties_read_cmp(PHYSFS_file * fp)
{
	int i;
	
	//  bitmap_index is a short
	
	NumTextures = PHYSFSX_readInt(fp);
	bitmap_index_read_n(Textures, MAX_TEXTURES, fp );
	tmap_info_read_n(TmapInfo, MAX_TEXTURES, fp);
	
	PHYSFS_read( fp, Sounds, sizeof(ubyte), MAX_SOUNDS );
	PHYSFS_read( fp, AltSounds, sizeof(ubyte), MAX_SOUNDS );
	
	Num_vclips = PHYSFSX_readInt(fp);
	vclip_read_n(Vclip, VCLIP_MAXNUM, fp);
	
	Num_effects = PHYSFSX_readInt(fp);
	eclip_read_n(Effects, MAX_EFFECTS, fp);
	
	Num_wall_anims = PHYSFSX_readInt(fp);
	wclip_read_n(WallAnims, MAX_WALL_ANIMS, fp);
	
	N_robot_types = PHYSFSX_readInt(fp);
	robot_info_read_n(Robot_info, MAX_ROBOT_TYPES, fp);
	
	N_robot_joints = PHYSFSX_readInt(fp);
	jointpos_read_n(Robot_joints, MAX_ROBOT_JOINTS, fp);
	
	N_weapon_types = PHYSFSX_readInt(fp);
	weapon_info_read_n(Weapon_info, MAX_WEAPON_TYPES, fp);
	
	N_powerup_types = PHYSFSX_readInt(fp);
	powerup_type_info_read_n(Powerup_info, MAX_POWERUP_TYPES, fp);
	
	N_polygon_models = PHYSFSX_readInt(fp);	
	polymodel_read_n(Polygon_models, N_polygon_models, fp);

	for (i=0; i<N_polygon_models; i++ )
		polygon_model_data_read(&Polygon_models[i], fp);

	bitmap_index_read_n(Gauges, MAX_GAUGE_BMS, fp);
	
	for (i = 0; i < MAX_POLYGON_MODELS; i++)
		Dying_modelnums[i] = PHYSFSX_readInt(fp);
	for (i = 0; i < MAX_POLYGON_MODELS; i++)
		Dead_modelnums[i] = PHYSFSX_readInt(fp);
	
	bitmap_index_read_n(ObjBitmaps, MAX_OBJ_BITMAPS, fp);
	for (i = 0; i < MAX_OBJ_BITMAPS; i++)
		ObjBitmapPtrs[i] = PHYSFSX_readShort(fp);
	
	player_ship_read(&only_player_ship, fp);
	
	Num_cockpits = PHYSFSX_readInt(fp);
	bitmap_index_read_n(cockpit_bitmap, N_COCKPIT_BITMAPS, fp);
	
	PHYSFS_read( fp, Sounds, sizeof(ubyte), MAX_SOUNDS );
	PHYSFS_read( fp, AltSounds, sizeof(ubyte), MAX_SOUNDS );
	
	Num_total_object_types = PHYSFSX_readInt(fp);
	PHYSFS_read( fp, ObjType, sizeof(ubyte), MAX_OBJTYPE );
	PHYSFS_read( fp, ObjId, sizeof(ubyte), MAX_OBJTYPE );
	for (i = 0; i < MAX_OBJTYPE; i++)
		ObjStrength[i] = PHYSFSX_readFix(fp);
	
	First_multi_bitmap_num = PHYSFSX_readInt(fp);
	N_controlcen_guns = PHYSFSX_readInt(fp);
	
	for (i = 0; i < MAX_CONTROLCEN_GUNS; i++)
		PHYSFSX_readVector(&controlcen_gun_points[i], fp);
	for (i = 0; i < MAX_CONTROLCEN_GUNS; i++)
		PHYSFSX_readVector(&controlcen_gun_dirs[i], fp);
	
	exit_modelnum = PHYSFSX_readInt(fp);	
	destroyed_exit_modelnum = PHYSFSX_readInt(fp);
	
        #ifdef EDITOR
        //Hardcoded flags
        TextureMetals = 156;
        TextureLights = 263;
        TextureEffects = 327;
        //Build tmaplist
        Num_tmaps = 0;
         for (i=0; i < TextureEffects; i++)
          TmapList[Num_tmaps++] = i;
         for (i=0; i < Num_effects; i++)
          if (Effects[i].changing_wall_texture >= 0)
           TmapList[Num_tmaps++] = Effects[i].changing_wall_texture;
        #endif
}

void compute_average_rgb(grs_bitmap *bm, fix *rgb)
{
	ubyte *buf;
	int i, x, y, color, count;
	fix t_rgb[3] = { 0, 0, 0 };

	rgb[0] = rgb[1] = rgb[2] = 0;

	if (!bm->bm_data)
		return;

	MALLOC(buf, ubyte, bm->bm_w*bm->bm_h);
	memset(buf,0,bm->bm_w*bm->bm_h);

	if (bm->bm_flags & BM_FLAG_RLE){
		unsigned char * dbits;
		unsigned char * sbits;
		int data_offset;

		data_offset = 1;
		if (bm->bm_flags & BM_FLAG_RLE_BIG)
			data_offset = 2;

		sbits = &bm->bm_data[4 + (bm->bm_h * data_offset)];
		dbits = buf;

		for (i=0; i < bm->bm_h; i++ )    {
			gr_rle_decode(sbits,dbits);
			if ( bm->bm_flags & BM_FLAG_RLE_BIG )
				sbits += (int)INTEL_SHORT(*((short *)&(bm->bm_data[4+(i*data_offset)])));
			else
				sbits += (int)bm->bm_data[4+i];
			dbits += bm->bm_w;
		}
	}
	else
	{
		memcpy(buf, bm->bm_data, sizeof(unsigned char)*(bm->bm_w*bm->bm_h));
	}

	i = 0;
	for (x = 0; x < bm->bm_h; x++)
	{
		for (y = 0; y < bm->bm_w; y++)
		{
			color = buf[i++];
			t_rgb[0] = gr_palette[color*3];
			t_rgb[1] = gr_palette[color*3+1];
			t_rgb[2] = gr_palette[color*3+2];
			if (!(color == TRANSPARENCY_COLOR || (t_rgb[0] == t_rgb[1] && t_rgb[0] == t_rgb[2])))
			{
				rgb[0] += t_rgb[0];
				rgb[1] += t_rgb[1];
				rgb[2] += t_rgb[2];
				count++;
			}
		}
	}
	d_free(buf);
}
