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
 * Definitions for the laser code.
 *
 */

#ifndef _LASER_H
#define _LASER_H

enum weapon_type_t
{
#if defined(DXX_BUILD_DESCENT_II)
	LASER_ID_L1,
	LASER_ID = LASER_ID_L1, //0..3 are lasers
	LASER_ID_L2,
	LASER_ID_L3,
	LASER_ID_L4,
#endif
	CONCUSSION_ID = 8,
	FLARE_ID = 9,   //  NOTE: This MUST correspond to the ID generated at bitmaps.tbl read time.
#if defined(DXX_BUILD_DESCENT_I)
	LASER_ID = 10,
#endif
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

	SMART_MINE_HOMING_ID = 47,
	ROBOT_SMART_MINE_HOMING_ID = 49,
	ROBOT_SUPERPROX_ID = 53,
	EARTHSHAKER_MEGA_ID = 54,
	ROBOT_EARTHSHAKER_ID = 58,

	PMINE_ID = 51,  //the mine that the designers can place
#endif
};

// These are new defines for the value of 'flags' passed to do_laser_firing.
// The purpose is to collect other flags like QUAD_LASER and Spreadfire_toggle
// into a single 8-bit quantity so it can be easily used in network mode.

#define LASER_QUAD                  1
#define LASER_SPREADFIRE_TOGGLED    2

#define MAX_LASER_LEVEL         3   // Note, laser levels are numbered from 0.

#if defined(DXX_BUILD_DESCENT_II)
#define LASER_HELIX_FLAG0           4   // helix uses 3 bits for angle
#define LASER_HELIX_FLAG1           8   // helix uses 3 bits for angle
#define LASER_HELIX_FLAG2           16  // helix uses 3 bits for angle

#define LASER_HELIX_SHIFT       2   // how far to shift count to put in flags
#define LASER_HELIX_MASK        7   // must match number of bits in flags
#define MAX_SUPER_LASER_LEVEL   5   // Note, laser levels are numbered from 0.
#endif

#define MAX_LASER_BITMAPS   6

// For muzzle firing casting light.
#define MUZZLE_QUEUE_MAX    8

// Constants governing homing missile behavior.
#define HOMING_MAX_TRACKABLE_DOT        (3*F1_0/4) // was (7*F1_0/8) in original Descent 2
#define HOMING_MIN_TRACKABLE_DOT        (3*(F1_0 - HOMING_MAX_TRACKABLE_DOT)/4 + HOMING_MAX_TRACKABLE_DOT)
#define HOMING_MAX_TRACKABLE_DIST       (F1_0*250)
#define HOMING_FLY_STRAIGHT_TIME        (F1_0/8)
#define HOMING_TURN_TIME                (DESIGNATED_GAME_FRAMETIME)

struct object;

void Laser_render(struct object *obj);
int Laser_player_fire(struct object * obj, int type, int gun_num, int make_sound, int harmless_flag);
int Laser_player_fire_spread(struct object *obj, int laser_type, int gun_num, fix spreadr, fix spreadu, int make_sound, int harmless);
void Laser_do_weapon_sequence(struct object *obj);
void Flare_create(struct object *obj);
int laser_are_related(int o1, int o2);

extern int do_laser_firing_player(void);
extern void do_missile_firing(int drop_bomb);
extern void net_missile_firing(int player, int weapon, int flags);
extern int Network_laser_track;

int Laser_create_new(vms_vector * direction, vms_vector * position, int segnum, int parent, int type, int make_sound);

// Fires a laser-type weapon (a Primary weapon)
// Fires from object objnum, weapon type weapon_id.
// Assumes that it is firing from a player object, so it knows which
// gun to fire from.
// Returns the number of shots actually fired, which will typically be
// 1, but could be higher for low frame rates when rapidfire weapons,
// such as vulcan or plasma are fired.
extern int do_laser_firing(int objnum, int weapon_id, int level, int flags, int nfires);

// Easier to call than Laser_create_new because it determines the
// segment containing the firing point and deals with it being stuck
// in an object or through a wall.
// Fires a laser of type "weapon_type" from an object (parent) in the
// direction "direction" from the position "position"
// Returns object number of laser fired or -1 if not possible to fire
// laser.
int Laser_create_new_easy(vms_vector * direction, vms_vector * position, int parent, int weapon_type, int make_sound);

#if defined(DXX_BUILD_DESCENT_II)
// creates a weapon object
int create_weapon_object(int weapon_type,int segnum,vms_vector *position);

// give up control of the guided missile
void release_guided_missile(int player_num);

// Omega cannon stuff.
#define MAX_OMEGA_CHARGE    (F1_0)  //  Maximum charge level for omega cannonw
extern fix Omega_charge;
extern int Smartmines_dropped;
// NOTE: OMEGA_CHARGE_SCALE moved to laser.c to avoid long rebuilds if changed
extern int ok_to_do_omega_damage(struct object *weapon);
#endif

extern void create_smart_children(struct object *objp, int count);
extern int object_to_object_visibility(struct object *obj1, struct object *obj2, int trans_type);

extern int Muzzle_queue_index;
extern int Missile_gun;
extern int Proximity_dropped;

typedef struct muzzle_info {
	fix64       create_time;
	short       segnum;
	vms_vector  pos;
} muzzle_info;

extern muzzle_info Muzzle_data[MUZZLE_QUEUE_MAX];

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

#endif /* _LASER_H */
