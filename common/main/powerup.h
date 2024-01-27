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
 * Powerup header file.
 *
 */

#pragma once
#include "fwd-powerup.h"
#include "pack.h"

namespace dcx {

struct powerup_type_info : public prohibit_void_ptr<powerup_type_info>
{
	vclip_index vclip_num;
	int hit_sound;
	fix size;       // 3d size of longest dimension
	fix light;      // amount of light cast by this powerup, set in bitmaps.tbl
};

}

#ifdef dsx
namespace dsx {

enum class powerup_type_t : uint8_t
{
	POW_EXTRA_LIFE = 0,
	POW_ENERGY = 1,
	POW_SHIELD_BOOST = 2,
	POW_LASER = 3,

	POW_KEY_BLUE = 4,
	POW_KEY_RED = 5,
	POW_KEY_GOLD = 6,

//	POW_RADAR_ROBOTS = 7,
//	POW_RADAR_POWERUPS = 8,

	POW_MISSILE_1 = 10,
	POW_MISSILE_4 = 11,      // 4-pack MUST follow single missile

	POW_QUAD_FIRE = 12,

	POW_VULCAN_WEAPON = 13,
	POW_SPREADFIRE_WEAPON = 14,
	POW_PLASMA_WEAPON = 15,
	POW_FUSION_WEAPON = 16,
	POW_PROXIMITY_WEAPON = 17,
	POW_HOMING_AMMO_1 = 18,
	POW_HOMING_AMMO_4 = 19,      // 4-pack MUST follow single missile
	POW_SMARTBOMB_WEAPON = 20,
	POW_MEGA_WEAPON = 21,
	POW_VULCAN_AMMO = 22,
	POW_CLOAK = 23,
	POW_TURBO = 24,
	POW_INVULNERABILITY = 25,
	POW_MEGAWOW = 27,
#if defined(DXX_BUILD_DESCENT_II)
	POW_GAUSS_WEAPON = 28,
	POW_HELIX_WEAPON = 29,
	POW_PHOENIX_WEAPON = 30,
	POW_OMEGA_WEAPON = 31,

	POW_SUPER_LASER = 32,
	POW_FULL_MAP = 33,
	POW_CONVERTER = 34,
	POW_AMMO_RACK = 35,
	POW_AFTERBURNER = 36,
	POW_HEADLIGHT = 37,

	POW_SMISSILE1_1 = 38,
	POW_SMISSILE1_4 = 39,      // 4-pack MUST follow single missile
	POW_GUIDED_MISSILE_1 = 40,
	POW_GUIDED_MISSILE_4 = 41,      // 4-pack MUST follow single missile
	POW_SMART_MINE = 42,
	POW_MERCURY_MISSILE_1 = 43,
	POW_MERCURY_MISSILE_4 = 44,      // 4-pack MUST follow single missile
	POW_EARTHSHAKER_MISSILE = 45,

	POW_FLAG_BLUE = 46,
	POW_FLAG_RED = 47,

	POW_HOARD_ORB = 7,       // use unused slot
#endif
};

using d_powerup_info_array = enumerated_array<powerup_type_info, MAX_POWERUP_TYPES, powerup_type_t>;
extern d_powerup_info_array Powerup_info;
}
#endif
