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
#include "console.h"
#include "game.h"
#include "piggy.h"
#include "texmerge.h"
#include "paging.h"
#include "robot.h"
#include "vclip.h"
#include "effects.h"
#include "fireball.h"
#include "weapon.h"
#include "palette.h"
#include "text.h"
#include "gauges.h"
#include "powerup.h"
#include "fuelcen.h"
#include "ai.h"

#include "compiler-range_for.h"
#include "d_levelstate.h"
#include "partial_range.h"
#include "segiter.h"

static void paging_touch_vclip(const vclip &vc, const unsigned line)
#define paging_touch_vclip(V)	paging_touch_vclip(V, __LINE__)
{
	using range_type = partial_range_t<const bitmap_index*>;
	union {
		uint8_t storage[1];
		range_type r;
	} u{};
	try
	{
		new(&u.r) range_type((partial_const_range)(__FILE__, line, "vc.frames", vc.frames, vc.num_frames));
		static_assert(std::is_trivially_destructible<range_type>::value, "partial_range destructor not called");
	}
	catch (const range_type::partial_range_error &e)
	{
		con_puts(CON_URGENT, e.what());
		return;
	}
	range_for (auto &i, u.r)
	{
		PIGGY_PAGE_IN(i);
	}
}

static void paging_touch_wall_effects(const d_eclip_array &Effects, const Textures_array &Textures, const d_vclip_array &Vclip, const int tmap_num)
{
	range_for (auto &i, partial_const_range(Effects, Num_effects))
	{
		if ( i.changing_wall_texture == tmap_num )	{
			paging_touch_vclip(i.vc);

			if (i.dest_bm_num < Textures.size())
				PIGGY_PAGE_IN( Textures[i.dest_bm_num] );	//use this bitmap when monitor destroyed
			if ( i.dest_vclip > -1 )
				paging_touch_vclip(Vclip[i.dest_vclip]);		  //what vclip to play when exploding

			if ( i.dest_eclip > -1 )
				paging_touch_vclip(Effects[i.dest_eclip].vc); //what eclip to play when exploding

			if ( i.crit_clip > -1 )
				paging_touch_vclip(Effects[i.crit_clip].vc); //what eclip to play when mine critical
			break;
		}
	}
}

static void paging_touch_object_effects(const d_eclip_array &Effects, const object_bitmap_index tmap_num)
{
	range_for (auto &i, partial_const_range(Effects, Num_effects))
	{
		if ( i.changing_object_texture == tmap_num )	{
			paging_touch_vclip(i.vc);
			break;
		}
	}
}

static void paging_touch_model( int modelnum )
{
	auto &Effects = LevelUniqueEffectsClipState.Effects;
	auto &Polygon_models = LevelSharedPolygonModelState.Polygon_models;
	const auto &pm = Polygon_models[modelnum];
	const uint_fast32_t b = pm.first_texture;
	const uint_fast32_t e = b + pm.n_textures;
	range_for (const auto p, partial_range(ObjBitmapPtrs, b, e))
	{
		PIGGY_PAGE_IN(ObjBitmaps[p]);
		paging_touch_object_effects(Effects, p);
	}
}

static void paging_touch_weapon(const d_vclip_array &Vclip, const weapon_info &weapon)
{
	// Page in the robot's weapons.

	if(weapon.picture.index)
	{
		PIGGY_PAGE_IN(weapon.picture);
	}		
	
	if (weapon.flash_vclip > -1)
		paging_touch_vclip(Vclip[weapon.flash_vclip]);
	if (weapon.wall_hit_vclip > -1)
		paging_touch_vclip(Vclip[weapon.wall_hit_vclip]);
	if (weapon.damage_radius)
	{
		// Robot_hit_vclips are actually badass_vclips
		if (weapon.robot_hit_vclip > -1)
			paging_touch_vclip(Vclip[weapon.robot_hit_vclip]);
	}

	switch(weapon.render)
	{
	case WEAPON_RENDER_VCLIP:
		if (weapon.weapon_vclip > -1)
			paging_touch_vclip(Vclip[weapon.weapon_vclip]);
		break;
	case WEAPON_RENDER_NONE:
	case weapon_info::render_type::laser:
		break;
	case WEAPON_RENDER_POLYMODEL:
		paging_touch_model(weapon.model_num);
		break;
	case WEAPON_RENDER_BLOB:
		PIGGY_PAGE_IN(weapon.bitmap);
		break;
	}
}

static void paging_touch_weapon(const d_vclip_array &Vclip, const weapon_info_array &Weapon_info, const uint_fast32_t weapon_type)
{
	if (weapon_type < N_weapon_types)
		paging_touch_weapon(Vclip, Weapon_info[weapon_type]);
}

const std::array<sbyte, 13> super_boss_gate_type_list{{0, 1, 8, 9, 10, 11, 12, 15, 16, 18, 19, 20, 22}};

static void paging_touch_robot(const d_level_shared_robot_info_state::d_robot_info_array &Robot_info, const d_vclip_array &Vclip, const weapon_info_array &Weapon_info, const unsigned ridx)
{
	auto &ri = Robot_info[ridx];
	// Page in robot_index
	paging_touch_model(ri.model_num);
	if (ri.exp1_vclip_num > -1)
		paging_touch_vclip(Vclip[ri.exp1_vclip_num]);
	if (ri.exp2_vclip_num > -1)
		paging_touch_vclip(Vclip[ri.exp2_vclip_num]);

	// Page in his weapons
	paging_touch_weapon(Vclip, Weapon_info, ri.weapon_type);

	// A super-boss can gate in robots...
	if (ri.boss_flag == BOSS_SUPER)
	{
		range_for (const auto i, super_boss_gate_type_list)
			paging_touch_robot(Robot_info, Vclip, Weapon_info, i);
		paging_touch_vclip(Vclip[VCLIP_MORPHING_ROBOT]);
	}
}

static void paging_touch_object(const d_level_shared_robot_info_state::d_robot_info_array &Robot_info, const Textures_array &Textures, const d_vclip_array &Vclip, const weapon_info_array &Weapon_info, const object_base &obj)
{
	int v;

	switch (obj.render_type) {

		case RT_NONE:	break;		//doesn't render, like the player

		case RT_POLYOBJ:
			if (obj.rtype.pobj_info.tmap_override != -1)
				PIGGY_PAGE_IN(Textures[obj.rtype.pobj_info.tmap_override]);
			else
				paging_touch_model(obj.rtype.pobj_info.model_num);
			break;

		case RT_POWERUP:
			if (obj.rtype.vclip_info.vclip_num > -1)
			{
				paging_touch_vclip(Vclip[obj.rtype.vclip_info.vclip_num]);
			}
			break;

		case RT_MORPH:	break;

		case RT_FIREBALL: break;

		case RT_WEAPON_VCLIP: break;

		case RT_HOSTAGE:
			paging_touch_vclip(Vclip[obj.rtype.vclip_info.vclip_num]);
			break;

		case RT_LASER: break;
 	}

	switch (obj.type) {	
		default:
			break;
	case OBJ_PLAYER:	
		v = get_explosion_vclip(obj, explosion_vclip_stage::s0);
		if ( v > -1 )
			paging_touch_vclip(Vclip[v]);
		break;
	case OBJ_ROBOT:
		paging_touch_robot(Robot_info, Vclip, Weapon_info, get_robot_id(obj));
		break;
	case OBJ_CNTRLCEN:
		paging_touch_weapon(Vclip, Weapon_info, weapon_id_type::CONTROLCEN_WEAPON_NUM);
		if (Dead_modelnums[obj.rtype.pobj_info.model_num] != -1)
		{
			paging_touch_model(Dead_modelnums[obj.rtype.pobj_info.model_num]);
		}
		break;
	}
}

	

static void paging_touch_side(const d_eclip_array &Effects, const Textures_array &Textures, const d_vclip_array &Vclip, const cscusegment segp, int sidenum )
{
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptr = Walls.vcptr;
	if (!(WALL_IS_DOORWAY(GameBitmaps, Textures, vcwallptr, segp, sidenum) & WALL_IS_DOORWAY_FLAG::render))
		return;
	auto &uside = segp.u.sides[sidenum];
	const auto tmap1 = uside.tmap_num;
	paging_touch_wall_effects(Effects, Textures, Vclip, get_texture_index(tmap1));
	if (const auto tmap2 = uside.tmap_num2; tmap2 != texture2_value::None)
	{
		texmerge_get_cached_bitmap( tmap1, tmap2 );
		paging_touch_wall_effects(Effects, Textures, Vclip, get_texture_index(tmap2));
	} else	{
		PIGGY_PAGE_IN(Textures[get_texture_index(tmap1)]);
	}
}

static void paging_touch_robot_maker(const d_level_shared_robot_info_state::d_robot_info_array &Robot_info, const d_vclip_array &Vclip, const weapon_info_array &Weapon_info, const shared_segment &segp)
{
	auto &RobotCenters = LevelSharedRobotcenterState.RobotCenters;
		paging_touch_vclip(Vclip[VCLIP_MORPHING_ROBOT]);
			const auto &robot_flags = RobotCenters[segp.matcen_num].robot_flags;
			const std::size_t bits_per_robot_flags = 8 * sizeof(robot_flags[0]);
			for (uint_fast32_t i = 0; i != robot_flags.size(); ++i)
			{
				auto robot_index = i * bits_per_robot_flags;
				uint_fast32_t flags = robot_flags[i];
				if (sizeof(flags) >= 2 * sizeof(robot_flags[0]) && i + 1 != robot_flags.size())
					flags |= static_cast<uint64_t>(robot_flags[++i]) << bits_per_robot_flags;
				while (flags) {
					if (flags & 1)	{
						// Page in robot_index
						paging_touch_robot(Robot_info, Vclip, Weapon_info, robot_index);
					}
					flags >>= 1;
					robot_index++;
				}
			}
}

static void paging_touch_segment(const d_eclip_array &Effects, const d_level_shared_robot_info_state::d_robot_info_array &Robot_info, const Textures_array &Textures, const d_vclip_array &Vclip, const weapon_info_array &Weapon_info, const fvcobjptridx &vcobjptridx, const fvcsegptr &vcsegptr, const cscusegment segp)
{
	if (segp.s.special == SEGMENT_IS_ROBOTMAKER )
		paging_touch_robot_maker(Robot_info, Vclip, Weapon_info, segp);

	for (int sn=0;sn<MAX_SIDES_PER_SEGMENT;sn++) {
		paging_touch_side(Effects, Textures, Vclip, segp, sn);
	}

	range_for (const object &objp, objects_in(segp, vcobjptridx, vcsegptr))
		paging_touch_object(Robot_info, Textures, Vclip, Weapon_info, objp);
}

static void paging_touch_walls(const Textures_array &Textures, const wall_animations_array &WallAnims, const fvcwallptr &vcwallptr)
{
	range_for (const auto &&wp, vcwallptr)
	{
		auto &w = *wp;
		if ( w.clip_num > -1 )	{
			const auto &anim = WallAnims[w.clip_num];
			range_for (auto &j, partial_range(anim.frames, anim.num_frames))
				PIGGY_PAGE_IN(Textures[j]);
		}
	}
}

namespace dsx {
void paging_touch_all(const d_vclip_array &Vclip)
{
	auto &Effects = LevelUniqueEffectsClipState.Effects;
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &WallAnims = GameSharedState.WallAnims;
	auto &vcobjptridx = Objects.vcptridx;
	pause_game_world_time p;

#if defined(DXX_BUILD_DESCENT_I)
	show_boxed_message(TXT_LOADING, 0);
#endif
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	range_for (const auto &&segp, vcsegptr)
	{
		paging_touch_segment(Effects, Robot_info, Textures, Vclip, Weapon_info, vcobjptridx, vcsegptr, segp);
	}	
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptr = Walls.vcptr;
	paging_touch_walls(Textures, WallAnims, vcwallptr);

	range_for (auto &s, partial_const_range(Powerup_info, N_powerup_types))
	{
		if ( s.vclip_num > -1 )	
			paging_touch_vclip(Vclip[s.vclip_num]);
	}

	range_for (auto &w, partial_const_range(Weapon_info, N_weapon_types))
	{
		paging_touch_weapon(Vclip, w);
	}

	range_for (auto &s, partial_const_range(Powerup_info, N_powerup_types))
	{
		if ( s.vclip_num > -1 )	
			paging_touch_vclip(Vclip[s.vclip_num]);
	}

	range_for (auto &s, Gauges)
	{
		if ( s.index )	{
			PIGGY_PAGE_IN( s );
		}
	}
	paging_touch_vclip(Vclip[VCLIP_PLAYER_APPEARANCE]);
	paging_touch_vclip(Vclip[VCLIP_POWERUP_DISAPPEARANCE]);

	reset_cockpit();		//force cockpit redraw next time
}
}
