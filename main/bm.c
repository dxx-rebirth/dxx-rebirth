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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/bm.c,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:41:41 $
 *
 * Bitmap and palette loading functions.
 *
 * $Log: bm.c,v $
 * Revision 1.1.1.1  2006/03/17 19:41:41  zicodxx
 * initial import
 *
 * Revision 1.2  1999/10/14 04:48:20  donut
 * alpha fixes, and gl_font args
 *
 * Revision 1.1.1.1  1999/06/14 22:05:21  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.3  1995/03/14  16:22:04  john
 * Added cdrom alternate directory stuff.
 * 
 * Revision 2.2  1995/03/07  16:51:48  john
 * Fixed robots not moving without edtiro bug.
 * 
 * Revision 2.1  1995/03/06  15:23:06  john
 * New screen techniques.
 * 
 * Revision 2.0  1995/02/27  11:27:05  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * 
 */


#ifdef RCS
#pragma off (unreferenced)
static char rcsid[] = "$Id: bm.c,v 1.1.1.1 2006/03/17 19:41:41 zicodxx Exp $";
#pragma on (unreferenced)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
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

#ifndef SHAREWARE
//-----------------------------------------------------------------
// Initializes all bitmaps from BITMAPS.TBL file.
int bm_init()
{
	init_polygon_models();
	init_endlevel();//adb: added, is also in bm_init_use_tbl
	piggy_init();				// This calls bm_read_all
	piggy_read_sounds();
	return 0;
}

void bm_read_all(CFILE * fp)
{
	int i;

	cfread( &NumTextures, sizeof(int), 1, fp );
	cfread( Textures, sizeof(bitmap_index), MAX_TEXTURES, fp );
	cfread( TmapInfo, sizeof(tmap_info), MAX_TEXTURES, fp );
	
	cfread( Sounds, sizeof(ubyte), MAX_SOUNDS, fp );
	cfread( AltSounds, sizeof(ubyte), MAX_SOUNDS, fp );

	cfread( &Num_vclips, sizeof(int), 1, fp );
	cfread( Vclip, sizeof(vclip), VCLIP_MAXNUM, fp );

	cfread( &Num_effects, sizeof(int), 1, fp );
	cfread( Effects, sizeof(eclip), MAX_EFFECTS, fp );

	cfread( &Num_wall_anims, sizeof(int), 1, fp );
	cfread( WallAnims, sizeof(wclip), MAX_WALL_ANIMS, fp );

	cfread( &N_robot_types, sizeof(int), 1, fp );
	cfread( Robot_info, sizeof(robot_info), MAX_ROBOT_TYPES, fp );

	cfread( &N_robot_joints, sizeof(int), 1, fp );
	cfread( Robot_joints, sizeof(jointpos), MAX_ROBOT_JOINTS, fp );

	cfread( &N_weapon_types, sizeof(int), 1, fp );
	cfread( Weapon_info, sizeof(weapon_info), MAX_WEAPON_TYPES, fp );

	cfread( &N_powerup_types, sizeof(int), 1, fp );
	cfread( Powerup_info, sizeof(powerup_type_info), MAX_POWERUP_TYPES, fp );
	
	cfread( &N_polygon_models, sizeof(int), 1, fp );

#if defined(__alpha__) || defined(_LP64)
       for (i=0; i<N_polygon_models; i++ ) {
               cfread( &Polygon_models[i], sizeof(polymodel)-4, 1, fp );
               /* this is a dirty hack */
               memmove ((char *)(&Polygon_models[i].model_data)+4, &Polygon_models[i].model_data, sizeof (polymodel)-12);
       }
#else
        cfread( Polygon_models, sizeof(polymodel), N_polygon_models, fp );
#endif    
    
       for (i=0; i<N_polygon_models; i++ ) {
		Polygon_models[i].model_data = malloc(Polygon_models[i].model_data_size);
/*		  printf("%d. size=%d, ptr=%p\n",i,Polygon_models[i].model_data_size,
		 Polygon_models[i].model_data);*/
		Assert( Polygon_models[i].model_data != NULL );
		cfread( Polygon_models[i].model_data, sizeof(ubyte), Polygon_models[i].model_data_size, fp );
	}

	cfread( Gauges, sizeof(bitmap_index), MAX_GAUGE_BMS, fp );

	cfread( Dying_modelnums, sizeof(int), MAX_POLYGON_MODELS, fp );
	cfread( Dead_modelnums, sizeof(int), MAX_POLYGON_MODELS, fp );

	cfread( ObjBitmaps, sizeof(bitmap_index), MAX_OBJ_BITMAPS, fp );
	cfread( ObjBitmapPtrs, sizeof(ushort), MAX_OBJ_BITMAPS, fp );

	cfread( &only_player_ship, sizeof(player_ship), 1, fp );

	cfread( &Num_cockpits, sizeof(int), 1, fp );
	cfread( cockpit_bitmap, sizeof(bitmap_index), N_COCKPIT_BITMAPS, fp );

	cfread( Sounds, sizeof(ubyte), MAX_SOUNDS, fp );
	cfread( AltSounds, sizeof(ubyte), MAX_SOUNDS, fp );

	cfread( &Num_total_object_types, sizeof(int), 1, fp );
	cfread( ObjType, sizeof(sbyte), MAX_OBJTYPE, fp );
	cfread( ObjId, sizeof(sbyte), MAX_OBJTYPE, fp );
	cfread( ObjStrength, sizeof(fix), MAX_OBJTYPE, fp );

	cfread( &First_multi_bitmap_num, sizeof(int), 1, fp );

	cfread( &N_controlcen_guns, sizeof(int), 1, fp );
	cfread( controlcen_gun_points, sizeof(vms_vector), MAX_CONTROLCEN_GUNS, fp );
	cfread( controlcen_gun_dirs, sizeof(vms_vector), MAX_CONTROLCEN_GUNS, fp );
	cfread( &exit_modelnum, sizeof(int), 1, fp );
	cfread( &destroyed_exit_modelnum, sizeof(int), 1, fp );

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

#endif

