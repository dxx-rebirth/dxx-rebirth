/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license.
 * See COPYING.txt for license details.
 */

#pragma once

#include <cstdint>
#include "dsx-ns.h"

#ifdef dsx
namespace dsx {

enum weapon_id_type : uint8_t
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
#if DXX_BUILD_DESCENT == 1
	/* Descent shipped with two spreadfire weapons.  One cost 0.5 energy
	 * to fire; the other cost 1.0 energy to fire.  The two have
	 * differing flight speeds.  For reasons unknown, the original code
	 * used the cost of one spreadfire weapon, but used the other weapon
	 * for all other purposes.  CHEAP_SPREADFIRE_ID represents the lower
	 * cost weapon, so that it can be named in
	 * Primary_weapon_to_weapon_info.  SPREADFIRE_ID represents the
	 * weapon used for all other purposes.
	 */
	CHEAP_SPREADFIRE_ID,
#elif defined(DXX_BUILD_DESCENT_II)
	SPREADFIRE_ID = 12,  //  NOTE: This MUST correspond to the ID generated at bitmaps.tbl read time.
#endif
	PLASMA_ID = 13,  //  NOTE: This MUST correspond to the ID generated at bitmaps.tbl read time.
	FUSION_ID = 14,  //  NOTE: This MUST correspond to the ID generated at bitmaps.tbl read time.
	HOMING_ID = 15,
	PROXIMITY_ID = 16,
	SMART_ID = 17,
	MEGA_ID = 18,

	PLAYER_SMART_HOMING_ID = 19,
#if DXX_BUILD_DESCENT == 1
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
#if DXX_BUILD_DESCENT == 2
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
	ROBOT_EARTHSHAKER_SUBMUNITION_ID = 59,

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
	ROBOT_70_WEAPON_64_ID = 64,	// Vertigo smelter Phoenix
#endif
	unspecified = 0xff,
};

}
#endif
