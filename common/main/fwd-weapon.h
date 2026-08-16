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

#include "pack.h"
#include "pstypes.h"
#include "maths.h"
#include "dxxsconf.h"
#include "dsx-ns.h"
#include "kconfig.h"
#include "weapon_id.h"

#include "fwd-player.h"
#include "fwd-powerup.h"

#ifdef DXX_BUILD_DESCENT
#if DXX_BUILD_DESCENT == 2
#define MAX_SUPER_LASER_LEVEL   laser_level::_6   // Note, laser levels are numbered from 0.
#endif

struct PHYSFS_File;
#if 0
void weapon_info_write(PHYSFS_File *, const weapon_info &);
#endif

namespace dcx {
enum class laser_level : uint8_t;
enum class has_primary_weapon_result : uint8_t;
enum class has_secondary_weapon_result : bool;
}

namespace dsx {
struct weapon_info;

#define REARM_TIME                  (F1_0)

#define WEAPON_DEFAULT_LIFETIME     (F1_0*12)   // Lifetime of an object if a bozo forgets to define it.

#if DXX_BUILD_DESCENT == 1
constexpr std::integral_constant<unsigned, 30> MAX_WEAPON_TYPES{};

constexpr std::integral_constant<unsigned, 5> MAX_PRIMARY_WEAPONS{};
constexpr std::integral_constant<unsigned, 5> MAX_SECONDARY_WEAPONS{};

#elif DXX_BUILD_DESCENT == 2
// weapon info flags
#define WIF_PLACABLE        1   // can be placed by level designer
constexpr std::integral_constant<unsigned, 70> MAX_WEAPON_TYPES{};

constexpr std::integral_constant<unsigned, 10> MAX_PRIMARY_WEAPONS{};
constexpr std::integral_constant<unsigned, 10> MAX_SECONDARY_WEAPONS{};

enum class pig_hamfile_version : uint8_t;
extern pig_hamfile_version Piggy_hamfile_version;
#endif

enum class primary_weapon_index : uint8_t;
enum class secondary_weapon_index : uint8_t;

template <typename T>
	using per_primary_weapon_array = enumerated_array<T, MAX_PRIMARY_WEAPONS, primary_weapon_index>;
template <typename T>
	using per_secondary_weapon_array = enumerated_array<T, MAX_SECONDARY_WEAPONS, secondary_weapon_index>;
extern const per_primary_weapon_array<weapon_id_type> Primary_weapon_to_weapon_info;
//for each primary weapon, what kind of powerup gives weapon
extern const per_primary_weapon_array<powerup_type_t> Primary_weapon_to_powerup;
extern const per_secondary_weapon_array<weapon_id_type> Secondary_weapon_to_weapon_info;
//for each Secondary weapon, what kind of powerup gives weapon
extern const per_secondary_weapon_array<powerup_type_t> Secondary_weapon_to_powerup;
extern const per_secondary_weapon_array<uint8_t>    Secondary_ammo_max;
/*
 * reads n weapon_info structs from a PHYSFS_File
 */
using weapon_info_array = enumerated_array<weapon_info, MAX_WEAPON_TYPES, weapon_id_type>;
extern weapon_info_array Weapon_info;
void weapon_info_read_current_version(weapon_info_array &wi, std::size_t offset, std::size_t count, NamedPHYSFS_File fp);
#if DXX_BUILD_DESCENT == 2
void weapon_info_read_specified_version(weapon_info_array &wi, std::size_t offset, std::size_t count, NamedPHYSFS_File fp, pig_hamfile_version file_version);
#endif

// Weapon flags, if player->weapon_flags & WEAPON_FLAG is set, then the player has this weapon
#define HAS_LASER_FLAG      HAS_PRIMARY_FLAG(primary_weapon_index::laser)
#define HAS_VULCAN_FLAG     HAS_PRIMARY_FLAG(primary_weapon_index::vulcan)
#define HAS_SPREADFIRE_FLAG HAS_PRIMARY_FLAG(primary_weapon_index::spreadfire)
#define HAS_PLASMA_FLAG     HAS_PRIMARY_FLAG(primary_weapon_index::plasma)
#define HAS_FUSION_FLAG     HAS_PRIMARY_FLAG(primary_weapon_index::fusion)

#define NUM_SMART_CHILDREN  6   // Number of smart children created by default.
#if DXX_BUILD_DESCENT == 1
#elif DXX_BUILD_DESCENT == 2
#define HAS_SUPER_LASER_FLAG	HAS_PRIMARY_FLAG(primary_weapon_index::super_laser)
#define HAS_GAUSS_FLAG     HAS_PRIMARY_FLAG(primary_weapon_index::gauss)
#define HAS_HELIX_FLAG     HAS_PRIMARY_FLAG(primary_weapon_index::helix)
#define HAS_PHOENIX_FLAG   HAS_PRIMARY_FLAG(primary_weapon_index::phoenix)
#define HAS_OMEGA_FLAG     HAS_PRIMARY_FLAG(primary_weapon_index::omega)

static constexpr uint8_t SUPER_WEAPON{5};

static constexpr bool is_super_weapon(const primary_weapon_index i)
{
	return static_cast<uint8_t>(i) >= SUPER_WEAPON;
}

static constexpr bool is_super_weapon(const secondary_weapon_index i)
{
	return static_cast<uint8_t>(i) >= SUPER_WEAPON;
}

//flags whether the last time we use this weapon, it was the 'super' version
#endif
//for each Secondary weapon, which gun it fires out of
extern const per_secondary_weapon_array<player_gun_number> Secondary_weapon_to_gun_num;
}

namespace dcx {
extern unsigned N_weapon_types;
template <typename T>
class player_selected_weapon : public prohibit_void_ptr<>
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
#ifdef DXX_BUILD_DESCENT
namespace dsx {

struct player_info;
void do_primary_weapon_select(player_info &, primary_weapon_index weapon_num);
void do_secondary_weapon_select(player_info &, secondary_weapon_index weapon_num);
void auto_select_primary_weapon(player_info &);
void auto_select_secondary_weapon(player_info &);
void set_primary_weapon(player_info &, primary_weapon_index weapon_num);
void select_primary_weapon(player_info &, const char *weapon_name, primary_weapon_index weapon_num, int wait_for_rearm);
void set_secondary_weapon_to_concussion(player_info &);
void select_secondary_weapon(player_info &, const char *weapon_name, secondary_weapon_index weapon_num, int wait_for_rearm);

}
#endif

//-----------------------------------------------------------------------------
// Return:
// Bits set:
//      HAS_WEAPON_FLAG
//      HAS_ENERGY_FLAG
//      HAS_AMMO_FLAG
#ifdef DXX_BUILD_DESCENT
namespace dsx {
has_primary_weapon_result player_has_primary_weapon(const player_info &, primary_weapon_index weapon_num);
has_secondary_weapon_result player_has_secondary_weapon(const player_info &, secondary_weapon_index weapon_num);

//called when one of these weapons is picked up
//when you pick up a secondary, you always get the weapon & ammo for it
int pick_up_primary(player_info &, primary_weapon_index weapon_index);
int pick_up_secondary(player_info &, secondary_weapon_index weapon_index, int count, const control_info &Controls);

//called when a primary weapon is picked up
//returns true if actually picked up
}
#endif

//called when ammo (for the vulcan cannon) is picked up

namespace dsx {
int pick_up_vulcan_ammo(player_info &player_info, uint_fast32_t ammo_count, bool change_weapon = true);
//this function is for when the player intentionally drops a powerup
imobjptridx_t spit_powerup(d_level_unique_object_state &LevelUniqueObjectState, const d_level_shared_segment_state &LevelSharedSegmentState, d_level_unique_segment_state &LevelUniqueSegmentState, const d_vclip_array &Vclip, const object_base &spitter, powerup_type_t id, unsigned seed);

#if DXX_BUILD_DESCENT == 2
void attempt_to_steal_item(vmobjptridx_t objp, const robot_info &, object &playerobjp);

#define SMEGA_ID    40

void weapons_homing_all();
void weapons_homing_all_reset();

extern void rock_the_mine_frame(void);
extern void smega_rock_stuff(void);
extern void init_smega_detonates(void);
#endif
}

namespace dsx {
void InitWeaponOrdering();
void CyclePrimary(player_info &);
void CycleSecondary(player_info &);
void ReorderPrimary();
void ReorderSecondary();
#if DXX_BUILD_DESCENT == 2
void check_to_use_primary_super_laser(player_info &player_info);
void init_seismic_disturbances(void);
void process_super_mines_frame(void);
void do_seismic_stuff();
#endif
void DropCurrentWeapon(player_info &);
void DropSecondaryWeapon(player_info &);
}
#endif
