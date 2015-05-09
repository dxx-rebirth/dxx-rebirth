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
 * Definitions for the laser code.
 *
 */

#ifndef _LASER_H
#define _LASER_H

#include "maths.h"
#include "vecmat.h"

#ifdef __cplusplus
#include "objnum.h"
#include "segnum.h"
#include "fwdvalptridx.h"

enum weapon_type_t
{
	LASER_ID_L1,
	LASER_ID = LASER_ID_L1, //0..3 are lasers
	LASER_ID_L2,
	LASER_ID_L3,
	LASER_ID_L4,
	CLS1_DRONE_FIRE = 5,
	CONTROLCEN_WEAPON_NUM = 6,
	CONCUSSION_ID = 8,
	FLARE_ID = 9,   //  NOTE: This MUST correspond to the ID generated at bitmaps.tbl read time.
	CLS2_DRONE_LASER = 10,
	VULCAN_ID = 11,  //  NOTE: This MUST correspond to the ID generated at bitmaps.tbl read time.
#if defined(DXX_BUILD_DESCENT_II)
	SPREADFIRE_ID = 12,  //  NOTE: This MUST correspond to the ID generated at bitmaps.tbl read time.
#endif
	PLASMA_ID = 13,  //  NOTE: This MUST correspond to the ID generated at bitmaps.tbl read time.
	FUSION_ID = 14,  //  NOTE: This MUST correspond to the ID generated at bitmaps.tbl read time.
	HOMING_ID = 15,
	PROXIMITY_ID = 16,
	SMART_ID = 17,
	MEGA_ID = 18,

	PLAYER_SMART_HOMING_ID = 19,
#if defined(DXX_BUILD_DESCENT_I)
	SPREADFIRE_ID = 20,
#endif
	SUPER_MECH_MISS = 21,
	REGULAR_MECH_MISS = 22,
	SILENT_SPREADFIRE_ID = 23,
	MEDIUM_LIFTER_LASER = 24,
	SMALL_HULK_FIRE = 25,
	HEAVY_DRILLER_PLASMA = 26,
	SPIDER_ROBOT_FIRE = 27,
	ROBOT_MEGA_ID = 28,
	ROBOT_SMART_HOMING_ID = 29,
#if defined(DXX_BUILD_DESCENT_II)
	SUPER_LASER_ID = 30,  // 30,31 are super lasers (level 5,6)
	LASER_ID_L5 = SUPER_LASER_ID,
	LASER_ID_L6,

	GAUSS_ID = 32,  //  NOTE: This MUST correspond to the ID generated at bitmaps.tbl read time.
	HELIX_ID = 33,  //  NOTE: This MUST correspond to the ID generated at bitmaps.tbl read time.
	PHOENIX_ID = 34,  //  NOTE: This MUST correspond to the ID generated at bitmaps.tbl read time.
	OMEGA_ID = 35,  //  NOTE: This MUST correspond to the ID generated at bitmaps.tbl read time.

	FLASH_ID = 36,
	GUIDEDMISS_ID = 37,
	SUPERPROX_ID = 38,
	MERCURY_ID = 39,
	EARTHSHAKER_ID = 40,
	SMELTER_PHOENIX_ID = 44,

	SMART_MINE_HOMING_ID = 47,
	BPER_PHASE_ENERGY_ID = 48,
	ROBOT_SMART_MINE_HOMING_ID = 49,
	ROBOT_SUPERPROX_ID = 53,
	EARTHSHAKER_MEGA_ID = 54,
	ROBOT_EARTHSHAKER_ID = 58,

	PMINE_ID = 51,  //the mine that the designers can place

	ROBOT_26_WEAPON_46_ID = 46,
	ROBOT_27_WEAPON_52_ID = 52,
	ROBOT_28_WEAPON_42_ID = 42,
	ROBOT_29_WEAPON_20_ID = 20,
	ROBOT_30_WEAPON_48_ID = 48,
	ROBOT_36_WEAPON_41_ID = 41,
	ROBOT_37_WEAPON_41_ID = 41,
	ROBOT_38_WEAPON_42_ID = 42,
	ROBOT_39_WEAPON_43_ID = 43,
	ROBOT_43_WEAPON_55_ID = 55,
	ROBOT_45_WEAPON_45_ID = 45,
	ROBOT_46_WEAPON_55_ID = 55,
	ROBOT_47_WEAPON_26_ID = 26,
	ROBOT_50_WEAPON_50_ID = 50,
	ROBOT_52_WEAPON_52_ID = 52,
	ROBOT_53_WEAPON_45_ID = 45,
	ROBOT_55_WEAPON_44_ID = 44,
	ROBOT_57_WEAPON_44_ID = 44,
	ROBOT_59_WEAPON_48_ID = 48,
	ROBOT_62_WEAPON_60_ID = 60,
	ROBOT_47_WEAPON_57_ID = 57,
	ROBOT_62_WEAPON_61_ID = 61,
	ROBOT_71_WEAPON_62_ID = 62,	// M.A.X. homing flash missile
#endif
};

// These are new defines for the value of 'flags' passed to do_laser_firing.
// The purpose is to collect other flags like QUAD_LASER and Spreadfire_toggle
// into a single 8-bit quantity so it can be easily used in network mode.

#define LASER_QUAD                  1
#define LASER_SPREADFIRE_TOGGLED    2

#define MAX_LASER_LEVEL         LASER_LEVEL_4   // Note, laser levels are numbered from 0.

#if defined(DXX_BUILD_DESCENT_I)
#define DXX_MAXIMUM_LASER_LEVEL	LASER_LEVEL_4
#elif defined(DXX_BUILD_DESCENT_II)
#define DXX_MAXIMUM_LASER_LEVEL	MAX_SUPER_LASER_LEVEL
#endif

#define MAX_LASER_BITMAPS   6

// For muzzle firing casting light.
#define MUZZLE_QUEUE_MAX    8

// Constants governing homing missile behavior.
#define HOMING_MAX_TRACKABLE_DOT        (3*F1_0/4) // was (7*F1_0/8) in original Descent 2
#define HOMING_MIN_TRACKABLE_DOT        (3*(F1_0 - HOMING_MAX_TRACKABLE_DOT)/4 + HOMING_MAX_TRACKABLE_DOT)
#define HOMING_FLY_STRAIGHT_TIME        (F1_0/8)
#define HOMING_TURN_TIME                (DESIGNATED_GAME_FRAMETIME)


void Laser_render(vobjptr_t obj);
objptridx_t Laser_player_fire(vobjptridx_t obj, enum weapon_type_t laser_type, int gun_num, int make_sound, vms_vector shot_orientation);
void Laser_do_weapon_sequence(vobjptridx_t obj);
void Flare_create(vobjptridx_t obj);
bool laser_are_related(vcobjptridx_t o1, vcobjptridx_t o2);

extern int do_laser_firing_player(void);
extern void do_missile_firing(int drop_bomb);
extern void net_missile_firing(int player, int weapon, int flags);
extern objnum_t Network_laser_track;

objptridx_t Laser_create_new(const vms_vector &direction, const vms_vector &position, segnum_t segnum, vobjptridx_t parent, enum weapon_type_t type, int make_sound);

// Fires a laser-type weapon (a Primary weapon)
// Fires from object objnum, weapon type weapon_id.
// Assumes that it is firing from a player object, so it knows which
// gun to fire from.
// Returns the number of shots actually fired, which will typically be
// 1, but could be higher for low frame rates when rapidfire weapons,
// such as vulcan or plasma are fired.
int do_laser_firing(vobjptridx_t objnum, int weapon_id, int level, int flags, int nfires, vms_vector shot_orientation);

// Easier to call than Laser_create_new because it determines the
// segment containing the firing point and deals with it being stuck
// in an object or through a wall.
// Fires a laser of type "weapon_type" from an object (parent) in the
// direction "direction" from the position "position"
// Returns object number of laser fired or -1 if not possible to fire
// laser.
objptridx_t Laser_create_new_easy(const vms_vector &direction, const vms_vector &position, vobjptridx_t parent, enum weapon_type_t weapon_type, int make_sound);

#if defined(DXX_BUILD_DESCENT_II)
// give up control of the guided missile
void release_guided_missile(int player_num);

// Omega cannon stuff.
#define MAX_OMEGA_CHARGE    (F1_0)  //  Maximum charge level for omega cannonw
extern fix Omega_charge;
extern int Smartmines_dropped;
// NOTE: OMEGA_CHARGE_SCALE moved to laser.c to avoid long rebuilds if changed
int ok_to_do_omega_damage(vcobjptr_t weapon);
#endif

void create_smart_children(vobjptridx_t objp, int count);
int object_to_object_visibility(vcobjptridx_t obj1, vcobjptr_t obj2, int trans_type);

extern int Muzzle_queue_index;
extern int Missile_gun;
extern int Proximity_dropped;

struct muzzle_info
{
	fix64       create_time;
	segnum_t       segnum;
	vms_vector  pos;
};

extern array<muzzle_info, MUZZLE_QUEUE_MAX> Muzzle_data;

// Omega cannon stuff.
#define MAX_OMEGA_CHARGE    (F1_0)  //  Maximum charge level for omega cannonw
void omega_charge_frame(void);

static inline int is_proximity_bomb_or_smart_mine(enum weapon_type_t id)
{
#if defined(DXX_BUILD_DESCENT_II)
	if (id == SUPERPROX_ID)
		return 1;
#endif
	return id == PROXIMITY_ID;
}

static inline int is_proximity_bomb_or_smart_mine_or_placed_mine(enum weapon_type_t id)
{
#if defined(DXX_BUILD_DESCENT_II)
	if (id == SUPERPROX_ID || id == PMINE_ID)
		return 1;
#endif
	return id == PROXIMITY_ID;
}

#endif

#endif /* _LASER_H */
