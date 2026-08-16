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

#include <type_traits>
#include "game.h"
#include "piggy.h"

#ifdef __cplusplus
#include "dxxsconf.h"
#include "dsx-ns.h"
#include "d_bit_enum.h"
#include "objnum.h"
#include "pack.h"
#include "fwd-valptridx.h"
#include "fwd-weapon.h"

#ifdef DXX_BUILD_DESCENT
#include <array>

namespace dcx {

enum class laser_level : uint8_t
{
	_1,
	_2,
	_3,
	_4,
	/* if DXX_BUILD_DESCENT == 2 */
	_5,
	_6,
	/* endif */
};

enum class has_primary_weapon_result : uint8_t
{
	weapon = 1,
	energy = 2,
	ammo = 4,
};

enum class has_secondary_weapon_result : bool
{
	absent,
	present,
};

template <>
inline constexpr bool enable_bit_enum_and<has_primary_weapon_result, has_primary_weapon_result>{true};

template <>
inline constexpr bool enable_bit_enum_or<has_primary_weapon_result, has_primary_weapon_result>{true};

static constexpr uint8_t has_weapon(const has_primary_weapon_result r)
{
	return static_cast<uint8_t>(r & has_primary_weapon_result::weapon);
}

static constexpr bool has_all(const has_primary_weapon_result r)
{
	constexpr auto m = has_primary_weapon_result::weapon | has_primary_weapon_result::energy | has_primary_weapon_result::ammo;
	return (r & m) == m;
}

static constexpr uint8_t has_all(const has_secondary_weapon_result r)
{
	return static_cast<uint8_t>(r);
}

}

namespace dsx {

static inline laser_level &operator+=(laser_level &a, const laser_level b)
{
	return (a = static_cast<laser_level>(static_cast<uint8_t>(a) + static_cast<uint8_t>(b)));
}

static inline laser_level &operator-=(laser_level &a, const laser_level b)
{
	return (a = static_cast<laser_level>(static_cast<uint8_t>(a) - static_cast<uint8_t>(b)));
}

static inline laser_level &operator++(laser_level &a)
{
	return (a = static_cast<laser_level>(static_cast<uint8_t>(a) + 1u));
}

static inline laser_level &operator--(laser_level &a)
{
	return (a = static_cast<laser_level>(static_cast<uint8_t>(a) - 1u));
}

struct weapon_info : prohibit_void_ptr<>
{
	enum class render_type : uint8_t
	{
		laser,
		blob,
		polymodel,
		vclip,
		None = 0xff,
	};
	// Flag: set if this object is matter (as opposed to energy)
	enum class matter_flag : bool
	{
		energy,
		matter,
	};
	//	Flag: set if this object bounces off walls
	enum class bounce_type : uint8_t
	{
		never,
		always,
		twice,
	};
	enum class persistence_flag : bool
	{
		terminate_on_impact,
		persistent,
	};
	render_type render;        // How to draw 0=laser, 1=blob, 2=object
	matter_flag matter;
	bounce_type bounce;
	persistence_flag persistent;         // 0 = dies when it hits something, 1 = continues (eg, fusion cannon)
	polygon_model_index	model_num;		// Model num if rendertype==2.
	polygon_model_index	model_num_inner;// Model num of inner part if rendertype==2.

	vclip_index flash_vclip;        // What vclip to use for muzzle flash
	vclip_index robot_hit_vclip;    // What vclip for impact with robot
	vclip_index wall_hit_vclip;     // What vclip for impact with wall
	sound_effect flash_sound;        // What sound to play when fired
	sound_effect robot_hit_sound;    // What sound for impact with robot
	sound_effect wall_hit_sound;     // What sound for impact with wall
	sbyte   fire_count;         // Number of bursts fired from EACH GUN per firing.  For weapons which fire from both sides, 3*fire_count shots will be fired.
	sbyte   ammo_usage;         // How many units of ammunition it uses.

	vclip_index weapon_vclip;       // Vclip to render for the weapon, itself.
	sbyte   destroyable;        // If !0, this weapon can be destroyed by another weapon.

	sbyte   homing_flag;        // Set if this weapon can home in on a target.
#if DXX_BUILD_DESCENT == 2
	uint8_t flags;	// WIF_PLACABLE or 0
#endif

	bitmap_index bitmap;        // Pointer to bitmap if rendertype==0 or 1.

	fix energy_usage;           // How much fuel is consumed to fire this weapon.
	fix fire_wait;              // Time until this weapon can be fired again.

	fix blob_size;              // Size of blob if blob type
	fix flash_size;             // How big to draw the flash
	fix impact_size;            // How big of an impact
	per_difficulty_level_array<fix> strength;          // How much damage it can inflict
	per_difficulty_level_array<fix> speed;             // How fast it can move, difficulty level based.
	fix mass;                   // How much mass it has
	fix drag;                   // How much drag it has
	fix thrust;                 // How much thrust it has
	fix po_len_to_width_ratio;  // For polyobjects, the ratio of len/width. (10 maybe?)
	fix light;                  // Amount of light this weapon casts.
	fix lifetime;               // Lifetime in seconds of this weapon.
	fix damage_radius;          // Radius of damage caused by weapon, used for missiles (not lasers) to apply to damage to things it did not hit
#if DXX_BUILD_DESCENT == 1
//-- unused--   fix damage_force;           // Force of damage caused by weapon, used for missiles (not lasers) to apply to damage to things it did not hit
// damage_force was a real mess.  Wasn't Difficulty_level based, and was being applied instead of weapon's actual strength.  Now use 2*strength instead. --MK, 01/19/95
	bitmap_index    picture;    // a picture of the weapon for the cockpit
#elif DXX_BUILD_DESCENT == 2
	ubyte   speedvar;           // allowed variance in speed below average, /128: 64 = 50% meaning if speed = 100, can be 50..100
	sbyte   flash;              // Flash effect
	sbyte   afterburner_size;   // Size of blobs in F1_0/16 units, specify in bitmaps.tbl as floating point.  Player afterburner size = 2.5.

	/* not present in shareware datafiles */
	weapon_id_type   children;           // ID of weapon to drop if this contains children.  -1 means no children.

	/* not present in shareware datafiles */
	fix multi_damage_scale;     // Scale damage by this amount when applying to player in multiplayer.  F1_0 means no change.

//-- unused--   fix damage_force;           // Force of damage caused by weapon, used for missiles (not lasers) to apply to damage to things it did not hit
// damage_force was a real mess.  Wasn't Difficulty_level based, and was being applied instead of weapon's actual strength.  Now use 2*strength instead. --MK, 01/19/95
	bitmap_index    picture;    // a picture of the weapon for the cockpit
	/* not present in shareware datafiles */
	bitmap_index    hires_picture;  // a hires picture of the above
#endif
};

enum class primary_weapon_index : uint8_t
{
	laser = 0,
	vulcan = 1,
	spreadfire = 2,
	plasma = 3,
	fusion = 4,
#if DXX_BUILD_DESCENT == 2
	super_laser = 5,
	gauss = 6,
	helix = 7,
	phoenix = 8,
	omega = 9,
#endif
};

enum class secondary_weapon_index : uint8_t
{
	concussion = 0,
	homing = 1,
	proximity = 2,
	smart = 3,
	mega = 4,
#if DXX_BUILD_DESCENT == 2
	flash = 5,
	guided = 6,
	smart_mine = 7,
	mercury = 8,
	earthshaker = 9,
#endif
};

//given a weapon index, return the flag value
static constexpr unsigned HAS_PRIMARY_FLAG(const primary_weapon_index w)
{
	return 1 << static_cast<unsigned>(w);
}

static constexpr unsigned HAS_SECONDARY_FLAG(const secondary_weapon_index w)
{
	return 1 << static_cast<unsigned>(w);
}

struct player_info;
void delayed_autoselect(player_info &, const control_info &Controls);

//return which bomb will be dropped next time the bomb key is pressed
#if DXX_BUILD_DESCENT == 1
static constexpr secondary_weapon_index which_bomb(const player_info &)
{
	return secondary_weapon_index::proximity;
}

static constexpr int weapon_index_uses_vulcan_ammo(const primary_weapon_index id)
{
	return id == primary_weapon_index::vulcan;
}

static constexpr int weapon_index_is_player_bomb(const secondary_weapon_index id)
{
	return id == secondary_weapon_index::proximity;
}

//multiply ammo by this before displaying
static constexpr unsigned vulcan_ammo_scale(const unsigned v)
{
	return (v * 0xcc180u) >> 16;
}
#elif DXX_BUILD_DESCENT == 2
secondary_weapon_index which_bomb(player_info &player_info);
secondary_weapon_index which_bomb(const player_info &player_info);

static constexpr int weapon_index_uses_vulcan_ammo(const primary_weapon_index id)
{
	return id == primary_weapon_index::vulcan || id == primary_weapon_index::gauss;
}

static constexpr int weapon_index_is_player_bomb(const secondary_weapon_index id)
{
	return id == secondary_weapon_index::proximity || id == secondary_weapon_index::smart_mine;
}

//multiply ammo by this before displaying
static constexpr unsigned vulcan_ammo_scale(const unsigned v)
{
	return (v * 0xcc163u) >> 16;
}
#endif
}
#endif

#endif
