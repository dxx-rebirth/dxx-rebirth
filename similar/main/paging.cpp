/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license below.
 * See COPYING.txt for license details.

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
 * Routines for paging in/out textures.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "pstypes.h"
#include "inferno.h"
#include "segment.h"
#include "textures.h"
#include "wall.h"
#include "object.h"
#include "gamemine.h"
#include "dxxerror.h"
#include "gameseg.h"
#include "game.h"
#include "piggy.h"
#include "texmerge.h"
#include "paging.h"
#include "laser.h"
#include "polyobj.h"
#include "vclip.h"
#include "effects.h"
#include "fireball.h"
#include "weapon.h"
#include "palette.h"
#include "timer.h"
#include "text.h"
#include "cntrlcen.h"
#include "gauges.h"
#include "powerup.h"
#include "fuelcen.h"
#include "mission.h"

#include "compiler-range_for.h"
#include "highest_valid.h"
#include "partial_range.h"
#include "segiter.h"

static void paging_touch_vclip( vclip * vc )
{
	range_for (auto &i, partial_range(vc->frames, vc->num_frames))
	{
		PIGGY_PAGE_IN(i);
	}
}


static void paging_touch_wall_effects( int tmap_num )
{
	range_for (auto &i, partial_range(Effects, Num_effects))
	{
		if ( i.changing_wall_texture == tmap_num )	{
			paging_touch_vclip( &i.vc );

			if (i.dest_bm_num > -1)
				PIGGY_PAGE_IN( Textures[i.dest_bm_num] );	//use this bitmap when monitor destroyed
			if ( i.dest_vclip > -1 )
				paging_touch_vclip( &Vclip[i.dest_vclip] );		  //what vclip to play when exploding

			if ( i.dest_eclip > -1 )
				paging_touch_vclip( &Effects[i.dest_eclip].vc ); //what eclip to play when exploding

			if ( i.crit_clip > -1 )
				paging_touch_vclip( &Effects[i.crit_clip].vc ); //what eclip to play when mine critical
		}

	}
}

static void paging_touch_object_effects( int tmap_num )
{
	range_for (auto &i, partial_range(Effects, Num_effects))
	{
		if ( i.changing_object_texture == tmap_num )	{
			paging_touch_vclip( &i.vc );
		}
	}
}


static void paging_touch_model( int modelnum )
{
	polymodel *pm = &Polygon_models[modelnum];

	for (int i=0;i<pm->n_textures;i++) {
		PIGGY_PAGE_IN( ObjBitmaps[ObjBitmapPtrs[pm->first_texture+i]] );
		paging_touch_object_effects( ObjBitmapPtrs[pm->first_texture+i] );
	}
}

static void paging_touch_weapon( int weapon_type )
{
	// Page in the robot's weapons.
	
	if ( (weapon_type < 0) || (weapon_type > N_weapon_types) ) return;

	if ( Weapon_info[weapon_type].picture.index )	{
		PIGGY_PAGE_IN( Weapon_info[weapon_type].picture );
	}		
	
	if ( Weapon_info[weapon_type].flash_vclip > -1 )
		paging_touch_vclip(&Vclip[Weapon_info[weapon_type].flash_vclip]);
	if ( Weapon_info[weapon_type].wall_hit_vclip > -1 )
		paging_touch_vclip(&Vclip[Weapon_info[weapon_type].wall_hit_vclip]);
	if ( Weapon_info[weapon_type].damage_radius )	{
		// Robot_hit_vclips are actually badass_vclips
		if ( Weapon_info[weapon_type].robot_hit_vclip > -1 )
			paging_touch_vclip(&Vclip[Weapon_info[weapon_type].robot_hit_vclip]);
	}

	switch( Weapon_info[weapon_type].render_type )	{
	case WEAPON_RENDER_VCLIP:
		if ( Weapon_info[weapon_type].weapon_vclip > -1 )
			paging_touch_vclip( &Vclip[Weapon_info[weapon_type].weapon_vclip] );
		break;
	case WEAPON_RENDER_NONE:
		break;
	case WEAPON_RENDER_POLYMODEL:
		paging_touch_model( Weapon_info[weapon_type].model_num );
		break;
	case WEAPON_RENDER_BLOB:
		PIGGY_PAGE_IN( Weapon_info[weapon_type].bitmap );
		break;
	}
}



static const sbyte super_boss_gate_type_list[13] = {0, 1, 8, 9, 10, 11, 12, 15, 16, 18, 19, 20, 22 };

static void paging_touch_robot( int robot_index )
{
	// Page in robot_index
	paging_touch_model(Robot_info[robot_index].model_num);
	if ( Robot_info[robot_index].exp1_vclip_num>-1 )
		paging_touch_vclip(&Vclip[Robot_info[robot_index].exp1_vclip_num]);
	if ( Robot_info[robot_index].exp2_vclip_num>-1 )
		paging_touch_vclip(&Vclip[Robot_info[robot_index].exp2_vclip_num]);

	// Page in his weapons
	paging_touch_weapon( Robot_info[robot_index].weapon_type );

	// A super-boss can gate in robots...
	if ( Robot_info[robot_index].boss_flag==2 )	{
		range_for (const auto i, super_boss_gate_type_list)
			paging_touch_robot(i);

		paging_touch_vclip( &Vclip[VCLIP_MORPHING_ROBOT] );
	}
}


static void paging_touch_object(const vcobjptr_t obj)
{
	int v;

	switch (obj->render_type) {

		case RT_NONE:	break;		//doesn't render, like the player

		case RT_POLYOBJ:
			if ( obj->rtype.pobj_info.tmap_override != -1 )
				PIGGY_PAGE_IN( Textures[obj->rtype.pobj_info.tmap_override] );
			else
				paging_touch_model(obj->rtype.pobj_info.model_num);
			break;

		case RT_POWERUP:
			if ( obj->rtype.vclip_info.vclip_num > -1 ) {
				paging_touch_vclip(&Vclip[obj->rtype.vclip_info.vclip_num]);
			}
			break;

		case RT_MORPH:	break;

		case RT_FIREBALL: break;

		case RT_WEAPON_VCLIP: break;

		case RT_HOSTAGE:
			paging_touch_vclip(&Vclip[obj->rtype.vclip_info.vclip_num]);
			break;

		case RT_LASER: break;
 	}

	switch (obj->type) {	
	case OBJ_PLAYER:	
		v = get_explosion_vclip(obj, 0);
		if ( v > -1 )
			paging_touch_vclip(&Vclip[v]);
		break;
	case OBJ_ROBOT:
		paging_touch_robot( get_robot_id(obj) );
		break;
	case OBJ_CNTRLCEN:
		paging_touch_weapon( CONTROLCEN_WEAPON_NUM );
		if (Dead_modelnums[obj->rtype.pobj_info.model_num] != -1)	{
			paging_touch_model( Dead_modelnums[obj->rtype.pobj_info.model_num] );
		}
		break;
	}
}

	

static void paging_touch_side(const vcsegptr_t segp, int sidenum )
{
	int tmap1, tmap2;

	if (!(WALL_IS_DOORWAY(segp,sidenum) & WID_RENDER_FLAG))
		return;
	
	tmap1 = segp->sides[sidenum].tmap_num;
	paging_touch_wall_effects(tmap1);
	tmap2 = segp->sides[sidenum].tmap_num2;
	if (tmap2 != 0)	{
		texmerge_get_cached_bitmap( tmap1, tmap2 );
		paging_touch_wall_effects( tmap2 & 0x3FFF );
	} else	{
		PIGGY_PAGE_IN( Textures[tmap1] );
	}
}

static void paging_touch_robot_maker(const vcsegptr_t segp )
{
	if ( segp->special == SEGMENT_IS_ROBOTMAKER )	{
		paging_touch_vclip(&Vclip[VCLIP_MORPHING_ROBOT]);
			uint	flags;
			int	robot_index;

			for (unsigned i=0;i<sizeof(RobotCenters[0].robot_flags)/sizeof(RobotCenters[0].robot_flags[0]);i++) {
				robot_index = i*32;
				flags = RobotCenters[segp->matcen_num].robot_flags[i];
				while (flags) {
					if (flags & 1)	{
						// Page in robot_index
						paging_touch_robot( robot_index );
					}
					flags >>= 1;
					robot_index++;
				}
			}
	}
}


static void paging_touch_segment(const vcsegptr_t segp)
{
	if ( segp->special == SEGMENT_IS_ROBOTMAKER )
		paging_touch_robot_maker(segp);

//	paging_draw_orb();
	for (int sn=0;sn<MAX_SIDES_PER_SEGMENT;sn++) {
//		paging_draw_orb();
		paging_touch_side( segp, sn );
	}

	range_for (const auto objp, objects_in(*segp))
		paging_touch_object(objp);
}



static void paging_touch_walls()
{
	wclip *anim;

	range_for (auto &w, partial_range(Walls, Num_walls))
	{
//		paging_draw_orb();
		if ( w.clip_num > -1 )	{
			anim = &WallAnims[w.clip_num];
			for (int j=0; j < anim->num_frames; j++ ) {
				PIGGY_PAGE_IN( Textures[anim->frames[j]] );
			}
		}
	}
}

void paging_touch_all()
{
	stop_time();

#if defined(DXX_BUILD_DESCENT_I)
	show_boxed_message(TXT_LOADING, 0);
#endif
	range_for (const auto s, highest_valid(Segments))
	{
		paging_touch_segment( &Segments[s] );
	}	
	paging_touch_walls();

	range_for (auto &s, partial_range(Powerup_info, N_powerup_types))
	{
		if ( s.vclip_num > -1 )	
			paging_touch_vclip(&Vclip[s.vclip_num]);
	}

	for ( int s=0; s<N_weapon_types; s++ ) {
		paging_touch_weapon(s);
	}

	range_for (auto &s, partial_range(Powerup_info, N_powerup_types))
	{
		if ( s.vclip_num > -1 )	
			paging_touch_vclip(&Vclip[s.vclip_num]);
	}

	range_for (auto &s, Gauges)
	{
		if ( s.index )	{
			PIGGY_PAGE_IN( s );
		}
	}
	paging_touch_vclip( &Vclip[VCLIP_PLAYER_APPEARANCE] );
	paging_touch_vclip( &Vclip[VCLIP_POWERUP_DISAPPEARANCE] );

	start_time();
	reset_cockpit();		//force cockpit redraw next time
}

