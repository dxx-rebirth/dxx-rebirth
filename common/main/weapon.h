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

#include "game.h"
#include "piggy.h"

#ifdef __cplusplus
#include "dxxsconf.h"
#include "compiler-array.h"
#include "objnum.h"
#include "pack.h"
#include "fwdvalptridx.h"
#include "fwd-weapon.h"

#include "compiler-type_traits.h"

void delayed_autoselect();

enum laser_level_t : uint8_t
{
	LASER_LEVEL_1,
	LASER_LEVEL_2,
	LASER_LEVEL_3,
	LASER_LEVEL_4,
#if defined(DXX_BUILD_DESCENT_II)
	LASER_LEVEL_5,
	LASER_LEVEL_6,
#endif
};

class stored_laser_level
{
	laser_level_t m_level;
public:
	stored_laser_level() = default;
	constexpr stored_laser_level(const laser_level_t l) :
		m_level(l)
	{
	}
	constexpr explicit stored_laser_level(uint8_t i) :
		m_level(static_cast<laser_level_t>(i))
	{
	}
	operator laser_level_t() const
	{
		return m_level;
	}
	/* Assume no overflow/underflow.
	 * This was never checked when it was a simple ubyte.
	 */
	stored_laser_level &operator+=(uint8_t i)
	{
		m_level = static_cast<laser_level_t>(static_cast<uint8_t>(m_level) + i);
		return *this;
	}
	stored_laser_level &operator-=(uint8_t i)
	{
		m_level = static_cast<laser_level_t>(static_cast<uint8_t>(m_level) - i);
		return *this;
	}
	stored_laser_level &operator++()
	{
		return *this += 1;
	}
	stored_laser_level &operator--()
	{
		return *this -= 1;
	}
};

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
struct weapon_info : prohibit_void_ptr<weapon_info>
{
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
	array<fix, NDL> strength;          // How much damage it can inflict
	array<fix, NDL> speed;             // How fast it can move, difficulty level based.
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
	array<fix, NDL> strength;          // How much damage it can inflict
	array<fix, NDL> speed;             // How fast it can move, difficulty level based.
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
};
#endif

enum primary_weapon_index_t : uint8_t
{
	LASER_INDEX = 0,
	VULCAN_INDEX = 1,
	SPREADFIRE_INDEX = 2,
	PLASMA_INDEX = 3,
	FUSION_INDEX = 4,
#if defined(DXX_BUILD_DESCENT_II)
	SUPER_LASER_INDEX = 5,
	GAUSS_INDEX = 6,
	HELIX_INDEX = 7,
	PHOENIX_INDEX = 8,
	OMEGA_INDEX = 9,
#define HAS_SUPER_LASER_FLAG	HAS_PRIMARY_FLAG(SUPER_LASER_INDEX)
#define HAS_GAUSS_FLAG     HAS_PRIMARY_FLAG(GAUSS_INDEX)
#define HAS_HELIX_FLAG     HAS_PRIMARY_FLAG(HELIX_INDEX)
#define HAS_PHOENIX_FLAG   HAS_PRIMARY_FLAG(PHOENIX_INDEX)
#define HAS_OMEGA_FLAG     HAS_PRIMARY_FLAG(OMEGA_INDEX)
#endif
};

enum secondary_weapon_index_t : uint8_t
{
	CONCUSSION_INDEX = 0,
	HOMING_INDEX = 1,
	PROXIMITY_INDEX = 2,
	SMART_INDEX = 3,
	MEGA_INDEX = 4,
#if defined(DXX_BUILD_DESCENT_II)
	SMISSILE1_INDEX = 5,
	GUIDED_INDEX = 6,
	SMART_MINE_INDEX = 7,
	SMISSILE4_INDEX = 8,
	SMISSILE5_INDEX = 9,
#endif
};

class has_weapon_result
{
	uint8_t m_result;
public:
	static constexpr auto has_weapon_flag = tt::integral_constant<uint8_t, 1>{};
	static constexpr auto has_energy_flag = tt::integral_constant<uint8_t, 2>{};
	static constexpr auto has_ammo_flag   = tt::integral_constant<uint8_t, 4>{};
	has_weapon_result() = default;
	constexpr has_weapon_result(uint8_t r) : m_result(r)
	{
	}
	uint8_t has_weapon() const
	{
		return m_result & has_weapon_flag;
	}
	uint8_t has_energy() const
	{
		return m_result & has_energy_flag;
	}
	uint8_t has_ammo() const
	{
		return m_result & has_ammo_flag;
	}
	uint8_t flags() const
	{
		return m_result;
	}
	bool has_all() const
	{
		return m_result == (has_weapon_flag | has_energy_flag | has_ammo_flag);
	}
};

//-----------------------------------------------------------------------------
// Return:
// Bits set:
//      HAS_WEAPON_FLAG
//      HAS_ENERGY_FLAG
//      HAS_AMMO_FLAG
static inline has_weapon_result player_has_weapon(int weapon_num, int secondary_flag)
{
	return secondary_flag ? player_has_secondary_weapon(weapon_num) : player_has_primary_weapon(weapon_num);
}

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

#endif
