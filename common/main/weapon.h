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
 * Protypes for weapon stuff.
 *
 */

#ifndef _WEAPON_H
#define _WEAPON_H

#include "inferno.h"
#include "gr.h"
#include "game.h"
#include "piggy.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
typedef struct weapon_info {
	sbyte   render_type;        // How to draw 0=laser, 1=blob, 2=object
#if defined(DXX_BUILD_DESCENT_I)
	sbyte	model_num;					// Model num if rendertype==2.
	sbyte	model_num_inner;			// Model num of inner part if rendertype==2.
	sbyte   persistent;         // 0 = dies when it hits something, 1 = continues (eg, fusion cannon)

	sbyte   flash_vclip;        // What vclip to use for muzzle flash
	short   flash_sound;        // What sound to play when fired
	sbyte   robot_hit_vclip;    // What vclip for impact with robot
	short   robot_hit_sound;    // What sound for impact with robot

	sbyte   wall_hit_vclip;     // What vclip for impact with wall
	short   wall_hit_sound;     // What sound for impact with wall
	sbyte   fire_count;         // Number of bursts fired from EACH GUN per firing.  For weapons which fire from both sides, 3*fire_count shots will be fired.
	sbyte   ammo_usage;         // How many units of ammunition it uses.

	sbyte   weapon_vclip;       // Vclip to render for the weapon, itself.
	sbyte   destroyable;        // If !0, this weapon can be destroyed by another weapon.
	sbyte   matter;             // Flag: set if this object is matter (as opposed to energy)
	sbyte	bounce;						//	Flag: set if this object bounces off walls

	sbyte   homing_flag;        // Set if this weapon can home in on a target.
	sbyte	dum1, dum2, dum3;

	fix energy_usage;           // How much fuel is consumed to fire this weapon.
	fix fire_wait;              // Time until this weapon can be fired again.

	bitmap_index bitmap;        // Pointer to bitmap if rendertype==0 or 1.

	fix blob_size;              // Size of blob if blob type
	fix flash_size;             // How big to draw the flash
	fix impact_size;            // How big of an impact
	fix strength[NDL];          // How much damage it can inflict
	fix speed[NDL];             // How fast it can move, difficulty level based.
	fix mass;                   // How much mass it has
	fix drag;                   // How much drag it has
	fix thrust;                 // How much thrust it has
	fix po_len_to_width_ratio;  // For polyobjects, the ratio of len/width. (10 maybe?)
	fix light;                  // Amount of light this weapon casts.
	fix lifetime;               // Lifetime in seconds of this weapon.
	fix damage_radius;          // Radius of damage caused by weapon, used for missiles (not lasers) to apply to damage to things it did not hit
//-- unused--   fix damage_force;           // Force of damage caused by weapon, used for missiles (not lasers) to apply to damage to things it did not hit
// damage_force was a real mess.  Wasn't Difficulty_level based, and was being applied instead of weapon's actual strength.  Now use 2*strength instead. --MK, 01/19/95
	bitmap_index    picture;    // a picture of the weapon for the cockpit
#elif defined(DXX_BUILD_DESCENT_II)
	sbyte   persistent;         // 0 = dies when it hits something, 1 = continues (eg, fusion cannon)
	short   model_num;          // Model num if rendertype==2.
	short   model_num_inner;    // Model num of inner part if rendertype==2.

	sbyte   flash_vclip;        // What vclip to use for muzzle flash
	sbyte   robot_hit_vclip;    // What vclip for impact with robot
	short   flash_sound;        // What sound to play when fired

	sbyte   wall_hit_vclip;     // What vclip for impact with wall
	sbyte   fire_count;         // Number of bursts fired from EACH GUN per firing.  For weapons which fire from both sides, 3*fire_count shots will be fired.
	short   robot_hit_sound;    // What sound for impact with robot

	sbyte   ammo_usage;         // How many units of ammunition it uses.
	sbyte   weapon_vclip;       // Vclip to render for the weapon, itself.
	short   wall_hit_sound;     // What sound for impact with wall

	sbyte   destroyable;        // If !0, this weapon can be destroyed by another weapon.
	sbyte   matter;             // Flag: set if this object is matter (as opposed to energy)
	sbyte   bounce;             // 1==always bounces, 2=bounces twice
	sbyte   homing_flag;        // Set if this weapon can home in on a target.

	ubyte   speedvar;           // allowed variance in speed below average, /128: 64 = 50% meaning if speed = 100, can be 50..100

	ubyte   flags;              // see values above

	sbyte   flash;              // Flash effect
	sbyte   afterburner_size;   // Size of blobs in F1_0/16 units, specify in bitmaps.tbl as floating point.  Player afterburner size = 2.5.

	/* not present in shareware datafiles */
	sbyte   children;           // ID of weapon to drop if this contains children.  -1 means no children.

	fix energy_usage;           // How much fuel is consumed to fire this weapon.
	fix fire_wait;              // Time until this weapon can be fired again.

	/* not present in shareware datafiles */
	fix multi_damage_scale;     // Scale damage by this amount when applying to player in multiplayer.  F1_0 means no change.

	bitmap_index bitmap;        // Pointer to bitmap if rendertype==0 or 1.

	fix blob_size;              // Size of blob if blob type
	fix flash_size;             // How big to draw the flash
	fix impact_size;            // How big of an impact
	fix strength[NDL];          // How much damage it can inflict
	fix speed[NDL];             // How fast it can move, difficulty level based.
	fix mass;                   // How much mass it has
	fix drag;                   // How much drag it has
	fix thrust;                 // How much thrust it has
	fix po_len_to_width_ratio;  // For polyobjects, the ratio of len/width. (10 maybe?)
	fix light;                  // Amount of light this weapon casts.
	fix lifetime;               // Lifetime in seconds of this weapon.
	fix damage_radius;          // Radius of damage caused by weapon, used for missiles (not lasers) to apply to damage to things it did not hit
//-- unused--   fix damage_force;           // Force of damage caused by weapon, used for missiles (not lasers) to apply to damage to things it did not hit
// damage_force was a real mess.  Wasn't Difficulty_level based, and was being applied instead of weapon's actual strength.  Now use 2*strength instead. --MK, 01/19/95
	bitmap_index    picture;    // a picture of the weapon for the cockpit
	/* not present in shareware datafiles */
	bitmap_index    hires_picture;  // a hires picture of the above
#endif
} __pack__ weapon_info;
#endif

#define REARM_TIME                  (F1_0)

#define WEAPON_DEFAULT_LIFETIME     (F1_0*12)   // Lifetime of an object if a bozo forgets to define it.

#define WEAPON_TYPE_WEAK_LASER      0
#define WEAPON_TYPE_STRONG_LASER    1
#define WEAPON_TYPE_CANNON_BALL     2
#define WEAPON_TYPE_MISSILE         3

#define WEAPON_RENDER_NONE          -1
#define WEAPON_RENDER_LASER         0
#define WEAPON_RENDER_BLOB          1
#define WEAPON_RENDER_POLYMODEL     2
#define WEAPON_RENDER_VCLIP         3

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
#if defined(DXX_BUILD_DESCENT_I)
#define MAX_WEAPON_TYPES 			30

#define	MAX_PRIMARY_WEAPONS		5
#define	MAX_SECONDARY_WEAPONS	5

#elif defined(DXX_BUILD_DESCENT_II)
// weapon info flags
#define WIF_PLACABLE        1   // can be placed by level designer
#define MAX_WEAPON_TYPES            70

#define MAX_PRIMARY_WEAPONS         10
#define MAX_SECONDARY_WEAPONS       10
#endif

extern const ubyte Primary_weapon_to_weapon_info[MAX_PRIMARY_WEAPONS];
extern const ubyte Secondary_weapon_to_weapon_info[MAX_SECONDARY_WEAPONS];
//for each primary weapon, what kind of powerup gives weapon
extern const ubyte Primary_weapon_to_powerup[MAX_PRIMARY_WEAPONS];

//for each Secondary weapon, what kind of powerup gives weapon
extern const ubyte Secondary_weapon_to_powerup[MAX_SECONDARY_WEAPONS];
extern const int  Primary_ammo_max[MAX_PRIMARY_WEAPONS];
extern const ubyte    Secondary_ammo_max[MAX_SECONDARY_WEAPONS];
/*
 * reads n weapon_info structs from a PHYSFS_file
 */
extern weapon_info Weapon_info[MAX_WEAPON_TYPES];
extern int weapon_info_read_n(weapon_info *wi, int n, PHYSFS_file *fp, int file_version);
#endif

//given a weapon index, return the flag value
#define  HAS_FLAG(index)  (1<<(index))

// Weapon flags, if player->weapon_flags & WEAPON_FLAG is set, then the player has this weapon
#define HAS_LASER_FLAG      HAS_FLAG(LASER_INDEX)
#define HAS_VULCAN_FLAG     HAS_FLAG(VULCAN_INDEX)
#define HAS_PLASMA_FLAG     HAS_FLAG(PLASMA_INDEX)
#define HAS_FUSION_FLAG     HAS_FLAG(FUSION_INDEX)

#define HAS_CONCUSSION_FLAG HAS_FLAG(CONCUSSION_INDEX)
#define HAS_SMART_FLAG      HAS_FLAG(SMART_INDEX)
#define HAS_MEGA_FLAG       HAS_FLAG(MEGA_INDEX)

#define CLASS_PRIMARY       0
#define CLASS_SECONDARY     1

#define LASER_INDEX         0
#define VULCAN_INDEX        1
#define SPREADFIRE_INDEX    2
#define PLASMA_INDEX        3
#define FUSION_INDEX        4
#if defined(DXX_BUILD_DESCENT_II)
#define SUPER_LASER_INDEX   5
#define GAUSS_INDEX         6
#define HELIX_INDEX         7
#define PHOENIX_INDEX       8
#define OMEGA_INDEX         9
#endif

#define CONCUSSION_INDEX    0
#define HOMING_INDEX        1
#define PROXIMITY_INDEX     2
#define SMART_INDEX         3
#define MEGA_INDEX          4
#if defined(DXX_BUILD_DESCENT_I)
#define	NUM_SHAREWARE_WEAPONS	3		//in shareware, old get first 3 of each

#define	VULCAN_AMMO_SCALE		(0x198300/2)		//multiply ammo by this before displaying
#elif defined(DXX_BUILD_DESCENT_II)
#define SMISSILE1_INDEX     5
#define GUIDED_INDEX        6
#define SMART_MINE_INDEX    7
#define SMISSILE4_INDEX     8
#define SMISSILE5_INDEX     9

#define SUPER_WEAPON        5

#define VULCAN_AMMO_SCALE   0xcc163 //(0x198300/2)      //multiply ammo by this before displaying

#define NUM_SMART_CHILDREN  6   // Number of smart children created by default.
#endif

extern int N_weapon_types;
extern void do_weapon_select(int weapon_num, int secondary_flag);

extern sbyte Primary_weapon, Secondary_weapon;

extern void auto_select_weapon(int weapon_type);        //parm is primary or secondary
extern void select_weapon(int weapon_num, int secondary_flag, int print_message,int wait_for_rearm);

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
//for each Secondary weapon, which gun it fires out of
extern const ubyte Secondary_weapon_to_gun_num[MAX_SECONDARY_WEAPONS];
#endif

#if defined(DXX_BUILD_DESCENT_II)
//flags whether the last time we use this weapon, it was the 'super' version
extern ubyte Primary_last_was_super[MAX_PRIMARY_WEAPONS];
extern ubyte Secondary_last_was_super[MAX_SECONDARY_WEAPONS];

extern const char *const Primary_weapon_names_short[];
extern const char *const Secondary_weapon_names_short[];
extern const char *const Primary_weapon_names[];
extern const char *const Secondary_weapon_names[];
extern const sbyte    Weapon_is_energy[MAX_WEAPON_TYPES];
#endif

#define HAS_WEAPON_FLAG 1
#define HAS_ENERGY_FLAG 2
#define HAS_AMMO_FLAG       4
#define  HAS_ALL (HAS_WEAPON_FLAG|HAS_ENERGY_FLAG|HAS_AMMO_FLAG)

//-----------------------------------------------------------------------------
// Return:
// Bits set:
//      HAS_WEAPON_FLAG
//      HAS_ENERGY_FLAG
//      HAS_AMMO_FLAG
//      HAS_SUPER_FLAG
extern int player_has_weapon(int weapon_num, int secondary_flag);

//called when one of these weapons is picked up
//when you pick up a secondary, you always get the weapon & ammo for it
int pick_up_secondary(int weapon_index,int count);

//called when a primary weapon is picked up
//returns true if actually picked up
int pick_up_primary(int weapon_index);

//called when ammo (for the vulcan cannon) is picked up
int pick_up_ammo(int class_flag,int weapon_index,int ammo_count);

#if defined(DXX_BUILD_DESCENT_II)
extern int attempt_to_steal_item(struct object *objp, int player_num);

//this function is for when the player intentionally drops a powerup
extern int spit_powerup(struct object *spitter, int id, int seed);

#define SMEGA_ID    40

extern void rock_the_mine_frame(void);
extern void smega_rock_stuff(void);
extern void init_smega_detonates(void);
#endif

void InitWeaponOrdering();
void CyclePrimary();
void CycleSecondary();
void ReorderPrimary();
void ReorderSecondary();
void check_to_use_primary(int);
void init_seismic_disturbances(void);
void process_super_mines_frame(void);
void DropCurrentWeapon();
void DropSecondaryWeapon();
void do_seismic_stuff(void);

//return which bomb will be dropped next time the bomb key is pressed
#if defined(DXX_BUILD_DESCENT_I)
static inline int which_bomb(void)
{
	return PROXIMITY_INDEX;
}

static inline int weapon_index_uses_vulcan_ammo(unsigned id)
{
	return id == VULCAN_INDEX;
}

static inline int weapon_index_is_player_bomb(unsigned id)
{
	return id == PROXIMITY_INDEX;
}
#elif defined(DXX_BUILD_DESCENT_II)
extern fix64	Seismic_disturbance_start_time, Seismic_disturbance_end_time;
int which_bomb(void);

static inline int weapon_index_uses_vulcan_ammo(unsigned id)
{
	return id == VULCAN_INDEX || id == GAUSS_INDEX;
}

static inline int weapon_index_is_player_bomb(unsigned id)
{
	return id == PROXIMITY_INDEX || id == SMART_MINE_INDEX;
}
#endif

#ifdef __cplusplus
}
#endif

#endif
