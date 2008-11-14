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
#include "cfile.h"
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
 * reads n tmap_info structs from a CFILE
 */
int tmap_info_read_n(tmap_info *ti, int n, CFILE *fp)
{
	int i;
	
	for (i = 0; i < n; i++) {
		cfread(TmapInfo[i].filename, 13, 1, fp);
		ti[i].flags = cfile_read_byte(fp);
		ti[i].lighting = cfile_read_fix(fp);
		ti[i].damage = cfile_read_fix(fp);
		ti[i].eclip_num = cfile_read_int(fp);
	}
	return i;
}

int player_ship_read(player_ship *ps, CFILE *fp)
{
	int i;
	ps->model_num = cfile_read_int(fp);
	ps->expl_vclip_num = cfile_read_int(fp);
	ps->mass = cfile_read_fix(fp);
	ps->drag = cfile_read_fix(fp);
	ps->max_thrust = cfile_read_fix(fp);
	ps->reverse_thrust = cfile_read_fix(fp);
	ps->brakes = cfile_read_fix(fp);
	ps->wiggle = cfile_read_fix(fp);
	ps->max_rotthrust = cfile_read_fix(fp);
	for (i = 0; i < N_PLAYER_GUNS; i++)
		cfile_read_vector(&ps->gun_points[i], fp);
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
void properties_read_cmp(CFILE * fp)
{
	int i;
	
	//  bitmap_index is a short
	
	NumTextures = cfile_read_int(fp);
	bitmap_index_read_n(Textures, MAX_TEXTURES, fp );
	tmap_info_read_n(TmapInfo, MAX_TEXTURES, fp);
	
	cfread( Sounds, sizeof(ubyte), MAX_SOUNDS, fp );
	cfread( AltSounds, sizeof(ubyte), MAX_SOUNDS, fp );
	
	Num_vclips = cfile_read_int(fp);
	vclip_read_n(Vclip, VCLIP_MAXNUM, fp);
	
	Num_effects = cfile_read_int(fp);
	eclip_read_n(Effects, MAX_EFFECTS, fp);
	
	Num_wall_anims = cfile_read_int(fp);
	wclip_read_n(WallAnims, MAX_WALL_ANIMS, fp);
	
	N_robot_types = cfile_read_int(fp);
	robot_info_read_n(Robot_info, MAX_ROBOT_TYPES, fp);
	
	N_robot_joints = cfile_read_int(fp);
	jointpos_read_n(Robot_joints, MAX_ROBOT_JOINTS, fp);
	
	N_weapon_types = cfile_read_int(fp);
	weapon_info_read_n(Weapon_info, MAX_WEAPON_TYPES, fp);
	
	N_powerup_types = cfile_read_int(fp);
	powerup_type_info_read_n(Powerup_info, MAX_POWERUP_TYPES, fp);
	
	N_polygon_models = cfile_read_int(fp);	
	polymodel_read_n(Polygon_models, N_polygon_models, fp);

	for (i=0; i<N_polygon_models; i++ )
		polygon_model_data_read(&Polygon_models[i], fp);

	bitmap_index_read_n(Gauges, MAX_GAUGE_BMS, fp);
	
	for (i = 0; i < MAX_POLYGON_MODELS; i++)
		Dying_modelnums[i] = cfile_read_int(fp);
	for (i = 0; i < MAX_POLYGON_MODELS; i++)
		Dead_modelnums[i] = cfile_read_int(fp);
	
	bitmap_index_read_n(ObjBitmaps, MAX_OBJ_BITMAPS, fp);
	for (i = 0; i < MAX_OBJ_BITMAPS; i++)
		ObjBitmapPtrs[i] = cfile_read_short(fp);
	
	player_ship_read(&only_player_ship, fp);
	
	Num_cockpits = cfile_read_int(fp);
	bitmap_index_read_n(cockpit_bitmap, N_COCKPIT_BITMAPS, fp);
	
	cfread( Sounds, sizeof(ubyte), MAX_SOUNDS, fp );
	cfread( AltSounds, sizeof(ubyte), MAX_SOUNDS, fp );
	
	Num_total_object_types = cfile_read_int(fp);
	cfread( ObjType, sizeof(ubyte), MAX_OBJTYPE, fp );
	cfread( ObjId, sizeof(ubyte), MAX_OBJTYPE, fp );
	for (i = 0; i < MAX_OBJTYPE; i++)
		ObjStrength[i] = cfile_read_fix(fp);
	
	First_multi_bitmap_num = cfile_read_int(fp);
	N_controlcen_guns = cfile_read_int(fp);
	
	for (i = 0; i < MAX_CONTROLCEN_GUNS; i++)
		cfile_read_vector(&controlcen_gun_points[i], fp);
	for (i = 0; i < MAX_CONTROLCEN_GUNS; i++)
		cfile_read_vector(&controlcen_gun_dirs[i], fp);
	
	exit_modelnum = cfile_read_int(fp);	
	destroyed_exit_modelnum = cfile_read_int(fp);
	
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


