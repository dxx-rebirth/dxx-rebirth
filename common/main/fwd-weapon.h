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
 * Protypes for weapon stuff.
 *
 */

#pragma once

#ifdef __cplusplus
#include "pstypes.h"
#include "maths.h"
#include "dxxsconf.h"
#include "compiler-array.h"

#include "fwdobject.h"

enum powerup_type_t : uint8_t;

enum laser_level_t : uint8_t;
class stored_laser_level;

#if defined(DXX_BUILD_DESCENT_II)
#define LASER_HELIX_FLAG0           4   // helix uses 3 bits for angle
#define LASER_HELIX_FLAG1           8   // helix uses 3 bits for angle
#define LASER_HELIX_FLAG2           16  // helix uses 3 bits for angle
#define LASER_HELIX_SHIFT       2   // how far to shift count to put in flags
#define LASER_HELIX_MASK        7   // must match number of bits in flags
#define MAX_SUPER_LASER_LEVEL   LASER_LEVEL_6   // Note, laser levels are numbered from 0.
#endif

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
struct weapon_info;

struct PHYSFS_File;
void weapon_info_write(PHYSFS_File *, const weapon_info &);
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
const unsigned MAX_WEAPON_TYPES = 30;

const unsigned MAX_PRIMARY_WEAPONS = 5;
const unsigned MAX_SECONDARY_WEAPONS = 5;

#elif defined(DXX_BUILD_DESCENT_II)
// weapon info flags
#define WIF_PLACABLE        1   // can be placed by level designer
const unsigned MAX_WEAPON_TYPES = 70;

const unsigned MAX_PRIMARY_WEAPONS = 10;
const unsigned MAX_SECONDARY_WEAPONS = 10;
#endif

extern const array<ubyte, MAX_PRIMARY_WEAPONS> Primary_weapon_to_weapon_info;
//for each primary weapon, what kind of powerup gives weapon
extern const array<powerup_type_t, MAX_PRIMARY_WEAPONS> Primary_weapon_to_powerup;
extern const array<ubyte, MAX_SECONDARY_WEAPONS> Secondary_weapon_to_weapon_info;
//for each Secondary weapon, what kind of powerup gives weapon
extern const array<powerup_type_t, MAX_SECONDARY_WEAPONS> Secondary_weapon_to_powerup;
extern const array<uint8_t, MAX_SECONDARY_WEAPONS>    Secondary_ammo_max;
/*
 * reads n weapon_info structs from a PHYSFS_file
 */
typedef array<weapon_info, MAX_WEAPON_TYPES> weapon_info_array;
extern weapon_info_array Weapon_info;
void weapon_info_read_n(weapon_info_array &wi, std::size_t count, PHYSFS_File *fp, int file_version, std::size_t offset = 0);
#endif

//given a weapon index, return the flag value
#define  HAS_PRIMARY_FLAG(index)  (1<<(index))
#define  HAS_SECONDARY_FLAG(index)  (1<<(index))

// Weapon flags, if player->weapon_flags & WEAPON_FLAG is set, then the player has this weapon
#define HAS_LASER_FLAG      HAS_PRIMARY_FLAG(LASER_INDEX)
#define HAS_VULCAN_FLAG     HAS_PRIMARY_FLAG(VULCAN_INDEX)
#define HAS_SPREADFIRE_FLAG HAS_PRIMARY_FLAG(SPREADFIRE_INDEX)
#define HAS_PLASMA_FLAG     HAS_PRIMARY_FLAG(PLASMA_INDEX)
#define HAS_FUSION_FLAG     HAS_PRIMARY_FLAG(FUSION_INDEX)

#define HAS_CONCUSSION_FLAG HAS_SECONDARY_FLAG(CONCUSSION_INDEX)
#define HAS_HOMING_FLAG HAS_SECONDARY_FLAG(HOMING_INDEX)
#define HAS_PROXIMITY_BOMB_FLAG HAS_SECONDARY_FLAG(PROXIMITY_INDEX)
#define HAS_SMART_FLAG      HAS_SECONDARY_FLAG(SMART_INDEX)
#define HAS_MEGA_FLAG       HAS_SECONDARY_FLAG(MEGA_INDEX)

#define CLASS_PRIMARY       0
#define CLASS_SECONDARY     1

enum primary_weapon_index_t : uint8_t;
enum secondary_weapon_index_t : uint8_t;

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
#define NUM_SMART_CHILDREN  6   // Number of smart children created by default.
#if defined(DXX_BUILD_DESCENT_I)
#define	NUM_SHAREWARE_WEAPONS	3		//in shareware, old get first 3 of each
#define	VULCAN_AMMO_SCALE		(0x198300/2)		//multiply ammo by this before displaying
#elif defined(DXX_BUILD_DESCENT_II)
#define HAS_SUPER_LASER_FLAG	HAS_PRIMARY_FLAG(SUPER_LASER_INDEX)
#define HAS_GAUSS_FLAG     HAS_PRIMARY_FLAG(GAUSS_INDEX)
#define HAS_HELIX_FLAG     HAS_PRIMARY_FLAG(HELIX_INDEX)
#define HAS_PHOENIX_FLAG   HAS_PRIMARY_FLAG(PHOENIX_INDEX)
#define HAS_OMEGA_FLAG     HAS_PRIMARY_FLAG(OMEGA_INDEX)
#define SUPER_WEAPON        5
#define VULCAN_AMMO_SCALE   0xcc163 //(0x198300/2)      //multiply ammo by this before displaying
#define HAS_FLASH_FLAG	HAS_SECONDARY_FLAG(SMISSILE1_INDEX)
#define HAS_GUIDED_FLAG	HAS_SECONDARY_FLAG(GUIDED_INDEX)
#define HAS_SMART_BOMB_FLAG	HAS_SECONDARY_FLAG(SMART_MINE_INDEX)
#define HAS_MERCURY_FLAG	HAS_SECONDARY_FLAG(SMISSILE4_INDEX)
#define HAS_EARTHSHAKER_FLAG	HAS_SECONDARY_FLAG(SMISSILE5_INDEX)
//flags whether the last time we use this weapon, it was the 'super' version
extern array<uint8_t, MAX_PRIMARY_WEAPONS> Primary_last_was_super;
extern array<uint8_t, MAX_SECONDARY_WEAPONS> Secondary_last_was_super;
#endif
//for each Secondary weapon, which gun it fires out of
extern const array<uint8_t, MAX_SECONDARY_WEAPONS> Secondary_weapon_to_gun_num;
#endif

extern unsigned N_weapon_types;
void do_primary_weapon_select(uint_fast32_t weapon_num);
void do_secondary_weapon_select(uint_fast32_t weapon_num);

extern primary_weapon_index_t Primary_weapon;
extern sbyte Secondary_weapon;

void auto_select_primary_weapon();
void auto_select_secondary_weapon();
void select_primary_weapon(const char *weapon_name, uint_fast32_t weapon_num, int wait_for_rearm);
void select_secondary_weapon(const char *weapon_name, uint_fast32_t weapon_num, int wait_for_rearm);

class has_weapon_result;

//-----------------------------------------------------------------------------
// Return:
// Bits set:
//      HAS_WEAPON_FLAG
//      HAS_ENERGY_FLAG
//      HAS_AMMO_FLAG
has_weapon_result player_has_primary_weapon(int weapon_num);
has_weapon_result player_has_secondary_weapon(int weapon_num);

//called when one of these weapons is picked up
//when you pick up a secondary, you always get the weapon & ammo for it
int pick_up_secondary(int weapon_index,int count);

//called when a primary weapon is picked up
//returns true if actually picked up
int pick_up_primary(int weapon_index);

//called when ammo (for the vulcan cannon) is picked up
int pick_up_vulcan_ammo(uint_fast32_t ammo_count, bool change_weapon = true);

//this function is for when the player intentionally drops a powerup
objptridx_t spit_powerup(vobjptr_t spitter, int id, int seed);

#if defined(DXX_BUILD_DESCENT_II)
int attempt_to_steal_item(vobjptridx_t objp, vobjptr_t playerobjp);

#define SMEGA_ID    40

extern void rock_the_mine_frame(void);
extern void smega_rock_stuff(void);
extern void init_smega_detonates(void);
extern fix64 Seismic_disturbance_end_time;
#endif

void InitWeaponOrdering();
void CyclePrimary();
void CycleSecondary();
void ReorderPrimary();
void ReorderSecondary();
void check_to_use_primary_super_laser();
void init_seismic_disturbances(void);
void process_super_mines_frame(void);
void DropCurrentWeapon();
void DropSecondaryWeapon();
void do_seismic_stuff(void);

#endif
