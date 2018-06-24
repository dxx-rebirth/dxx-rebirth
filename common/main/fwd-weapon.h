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
#include "pack.h"
#include "pstypes.h"
#include "maths.h"
#include "dxxsconf.h"
#include "dsx-ns.h"
#include "compiler-array.h"
#include "weapon_id.h"

#include "fwd-object.h"

enum powerup_type_t : uint8_t;

#if defined(DXX_BUILD_DESCENT_II)
#define LASER_HELIX_MASK        7   // must match number of bits in flags
#define MAX_SUPER_LASER_LEVEL   LASER_LEVEL_6   // Note, laser levels are numbered from 0.
#endif

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)

struct PHYSFS_File;
#if 0
void weapon_info_write(PHYSFS_File *, const weapon_info &);
#endif

namespace dsx {
enum laser_level_t : uint8_t;
class stored_laser_level;
struct weapon_info;

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

#if defined(DXX_BUILD_DESCENT_I)
constexpr std::integral_constant<unsigned, 30> MAX_WEAPON_TYPES{};

constexpr std::integral_constant<unsigned, 5> MAX_PRIMARY_WEAPONS{};
constexpr std::integral_constant<unsigned, 5> MAX_SECONDARY_WEAPONS{};

#elif defined(DXX_BUILD_DESCENT_II)
// weapon info flags
#define WIF_PLACABLE        1   // can be placed by level designer
constexpr std::integral_constant<unsigned, 70> MAX_WEAPON_TYPES{};

constexpr std::integral_constant<unsigned, 10> MAX_PRIMARY_WEAPONS{};
constexpr std::integral_constant<unsigned, 10> MAX_SECONDARY_WEAPONS{};
#endif

extern const array<weapon_id_type, MAX_PRIMARY_WEAPONS> Primary_weapon_to_weapon_info;
//for each primary weapon, what kind of powerup gives weapon
extern const array<powerup_type_t, MAX_PRIMARY_WEAPONS> Primary_weapon_to_powerup;
extern const array<weapon_id_type, MAX_SECONDARY_WEAPONS> Secondary_weapon_to_weapon_info;
//for each Secondary weapon, what kind of powerup gives weapon
extern const array<powerup_type_t, MAX_SECONDARY_WEAPONS> Secondary_weapon_to_powerup;
extern const array<uint8_t, MAX_SECONDARY_WEAPONS>    Secondary_ammo_max;
/*
 * reads n weapon_info structs from a PHYSFS_File
 */
typedef array<weapon_info, MAX_WEAPON_TYPES> weapon_info_array;
extern weapon_info_array Weapon_info;
void weapon_info_read_n(weapon_info_array &wi, std::size_t count, PHYSFS_File *fp, int file_version, std::size_t offset = 0);

//given a weapon index, return the flag value
#define  HAS_PRIMARY_FLAG(index)  (1<<(index))
#define  HAS_SECONDARY_FLAG(index)  (1<<(index))

// Weapon flags, if player->weapon_flags & WEAPON_FLAG is set, then the player has this weapon
#define HAS_LASER_FLAG      HAS_PRIMARY_FLAG(primary_weapon_index_t::LASER_INDEX)
#define HAS_VULCAN_FLAG     HAS_PRIMARY_FLAG(primary_weapon_index_t::VULCAN_INDEX)
#define HAS_SPREADFIRE_FLAG HAS_PRIMARY_FLAG(primary_weapon_index_t::SPREADFIRE_INDEX)
#define HAS_PLASMA_FLAG     HAS_PRIMARY_FLAG(primary_weapon_index_t::PLASMA_INDEX)
#define HAS_FUSION_FLAG     HAS_PRIMARY_FLAG(primary_weapon_index_t::FUSION_INDEX)

enum primary_weapon_index_t : uint8_t;
enum secondary_weapon_index_t : uint8_t;

#define NUM_SMART_CHILDREN  6   // Number of smart children created by default.
#if defined(DXX_BUILD_DESCENT_I)
#define	NUM_SHAREWARE_WEAPONS	3		//in shareware, old get first 3 of each
#elif defined(DXX_BUILD_DESCENT_II)
#define HAS_SUPER_LASER_FLAG	HAS_PRIMARY_FLAG(primary_weapon_index_t::SUPER_LASER_INDEX)
#define HAS_GAUSS_FLAG     HAS_PRIMARY_FLAG(primary_weapon_index_t::GAUSS_INDEX)
#define HAS_HELIX_FLAG     HAS_PRIMARY_FLAG(primary_weapon_index_t::HELIX_INDEX)
#define HAS_PHOENIX_FLAG   HAS_PRIMARY_FLAG(primary_weapon_index_t::PHOENIX_INDEX)
#define HAS_OMEGA_FLAG     HAS_PRIMARY_FLAG(primary_weapon_index_t::OMEGA_INDEX)
#define SUPER_WEAPON        5
//flags whether the last time we use this weapon, it was the 'super' version
#endif
//for each Secondary weapon, which gun it fires out of
extern const array<uint8_t, MAX_SECONDARY_WEAPONS> Secondary_weapon_to_gun_num;
}

namespace dcx {
extern unsigned N_weapon_types;
template <typename T>
class player_selected_weapon : public prohibit_void_ptr<player_selected_weapon<T>>
{
	T active, delayed;
public:
	operator T() const
	{
		return get_active();
	}
	T get_active() const
	{
		return active;
	}
	T get_delayed() const
	{
		return delayed;
	}
	player_selected_weapon &operator=(T v)
	{
		active = v;
		set_delayed(v);
		return *this;
	}
	void set_delayed(T v)
	{
		delayed = v;
	}
};

}
#ifdef dsx
namespace dsx {

struct player_info;
void do_primary_weapon_select(player_info &, uint_fast32_t weapon_num);
void do_secondary_weapon_select(player_info &, secondary_weapon_index_t weapon_num);
void auto_select_primary_weapon(player_info &);
void auto_select_secondary_weapon(player_info &);
void set_primary_weapon(player_info &, uint_fast32_t weapon_num);
void select_primary_weapon(player_info &, const char *weapon_name, uint_fast32_t weapon_num, int wait_for_rearm);
void set_secondary_weapon_to_concussion(player_info &);
void select_secondary_weapon(player_info &, const char *weapon_name, uint_fast32_t weapon_num, int wait_for_rearm);

}
#endif
class has_weapon_result;

//-----------------------------------------------------------------------------
// Return:
// Bits set:
//      HAS_WEAPON_FLAG
//      HAS_ENERGY_FLAG
//      HAS_AMMO_FLAG
#ifdef dsx
namespace dsx {
has_weapon_result player_has_primary_weapon(const player_info &, int weapon_num);
has_weapon_result player_has_secondary_weapon(const player_info &, secondary_weapon_index_t weapon_num);

//called when one of these weapons is picked up
//when you pick up a secondary, you always get the weapon & ammo for it
int pick_up_primary(player_info &, int weapon_index);
int pick_up_secondary(player_info &, int weapon_index,int count);

//called when a primary weapon is picked up
//returns true if actually picked up
}
#endif

//called when ammo (for the vulcan cannon) is picked up

namespace dsx {
int pick_up_vulcan_ammo(player_info &player_info, uint_fast32_t ammo_count, bool change_weapon = true);
//this function is for when the player intentionally drops a powerup
imobjptridx_t spit_powerup(const object_base &spitter, unsigned id, unsigned seed);

#if defined(DXX_BUILD_DESCENT_II)
int attempt_to_steal_item(vmobjptridx_t objp, vmobjptr_t playerobjp);

#define SMEGA_ID    40

void weapons_homing_all();
void weapons_homing_all_reset();

extern void rock_the_mine_frame(void);
extern void smega_rock_stuff(void);
extern void init_smega_detonates(void);
extern fix64 Seismic_disturbance_end_time;
#endif
}
#endif

#ifdef dsx
namespace dsx {
void InitWeaponOrdering();
void CyclePrimary(player_info &);
void CycleSecondary(player_info &);
}
#endif
void ReorderPrimary();
void ReorderSecondary();
#ifdef dsx
namespace dsx {
#if defined(DXX_BUILD_DESCENT_II)
void check_to_use_primary_super_laser(player_info &player_info);
void init_seismic_disturbances(void);
void process_super_mines_frame(void);
void do_seismic_stuff();
#endif
void DropCurrentWeapon(player_info &);
void DropSecondaryWeapon(player_info &);
}
#endif

#endif
