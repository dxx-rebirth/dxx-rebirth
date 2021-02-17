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
 * Numbering system for the sounds.
 *
 */


#ifndef _SOUNDS_H
#define _SOUNDS_H

#include <array>

#ifdef dsx
#if defined(DXX_BUILD_DESCENT_I)
#define SOUND_GOOD_SELECTION_PRIMARY    (PCSharePig ? SOUND_ALREADY_SELECTED : SOUND_GOOD_SELECTION_PRIMARY_FULL_DATA)
#define SOUND_GOOD_SELECTION_SECONDARY  (PCSharePig ? SOUND_ALREADY_SELECTED : SOUND_GOOD_SELECTION_SECONDARY_FULL_DATA)    //      Adam: New sound number here! MK, 01/30/95
#define SOUND_CHEATER                   (PCSharePig ? SOUND_BAD_SELECTION : SOUND_CHEATER_FULL_DATA) // moved by Victor Rachels
#elif defined(DXX_BUILD_DESCENT_II)
#define SOUND_GOOD_SELECTION_PRIMARY	SOUND_GOOD_SELECTION_PRIMARY_FULL_DATA
#define SOUND_GOOD_SELECTION_SECONDARY	SOUND_GOOD_SELECTION_SECONDARY_FULL_DATA
#define SOUND_CHEATER	SOUND_CHEATER_FULL_DATA
#endif

namespace d1x {
constexpr std::integral_constant<unsigned, 250> MAX_SOUNDS{};
}
namespace d2x {
//--------------------------------------------------------------
// bad to have sound 255!
constexpr std::integral_constant<unsigned, 254> MAX_SOUNDS{};
}

//------------------- List of sound effects --------------------
namespace dcx {

	/* The games do not use exactly the same sounds, but every sound can
	 * be classified as:
	 * - Used in both games, with the same value
	 * - Used only in Descent 1
	 * - Used only in Descent 2
	 * Therefore, this can be in dcx without causing ambiguity.  Sounds
	 * used only for one game are unnecessarily defined in the other,
	 * and then ignored.
	 *
	 * The values of sound_effect are used to index Sounds.  A uint8_t
	 * would be sufficient to hold all values, but unsigned is used
	 * since a uint8_t would need to be zero-extended before it could be
	 * used in an array lookup.
	 */
enum sound_effect : unsigned
{
	SOUND_LASER_FIRED = 10,
	SOUND_BADASS_EXPLOSION = 11,  // need something different for this if possible
	SOUND_WEAPON_HIT_BLASTABLE = 11,
	SOUND_ROBOT_HIT_PLAYER = 17,
	SOUND_ROBOT_HIT = 20,
	SOUND_VOLATILE_WALL_HIT = 21,
	SOUND_DROP_BOMB = 26,
	SOUND_WEAPON_HIT_DOOR = 27,
	SOUND_CONTROL_CENTER_HIT = 30,
	SOUND_LASER_HIT_CLUTTER = 30,
	SOUND_CONTROL_CENTER_DESTROYED = 31,
	SOUND_EXPLODING_WALL = 31,  // one long sound
	SOUND_BIG_ENDLEVEL_EXPLOSION = SOUND_EXPLODING_WALL,
	SOUND_TUNNEL_EXPLOSION = SOUND_EXPLODING_WALL,
	SOUND_CONTROL_CENTER_WARNING_SIREN = 32,
	SOUND_MINE_BLEW_UP = 33,
	SOUND_FUSION_WARMUP = 34,
	SOUND_REFUEL_STATION_GIVING_FUEL = 62,
	SOUND_PLAYER_HIT_WALL = 70,
	SOUND_PLAYER_GOT_HIT = 71,
	SOUND_HOSTAGE_RESCUED = 91,
	SOUND_COUNTDOWN_0_SECS = 100, // countdown 100..114
	SOUND_COUNTDOWN_13_SECS = 113,
	SOUND_COUNTDOWN_29_SECS = 114,
	SOUND_HUD_MESSAGE = 117,
	SOUND_HUD_KILL = 118,
	SOUND_HOMING_WARNING = 122, // Warning beep: You are being tracked by a missile! Borrowed from old repair center sounds.
	SOUND_VOLATILE_WALL_HISS = 151, // need a hiss sound here.
	SOUND_GOOD_SELECTION_PRIMARY_FULL_DATA = 153,
	SOUND_GOOD_SELECTION_SECONDARY_FULL_DATA = 154, // Adam: New sound number here! MK, 01/30/95
	SOUND_ALREADY_SELECTED = 155, // Adam: New sound number here! MK, 01/30/95
	SOUND_BAD_SELECTION = 156,
	SOUND_CLOAK_OFF = 161, // sound when cloak goes away
	SOUND_WALL_CLOAK_OFF = SOUND_CLOAK_OFF,
	SOUND_INVULNERABILITY_OFF = 163, // sound when invulnerability goes away
	ROBOT_SEE_SOUND_DEFAULT = 170,
	ROBOT_ATTACK_SOUND_DEFAULT = 171,
	SOUND_BOSS_SHARE_SEE = 183,
	SOUND_BOSS_SHARE_DIE = 185,
	ROBOT_CLAW_SOUND_DEFAULT = 190,
	SOUND_CHEATER_FULL_DATA = 200,

	/* if DXX_BUILD_DESCENT_I */
	/* Not used even there */
#if 0
	SOUND_BOSS_SHARE_ATTACK = 184,

	SOUND_NASTY_ROBOT_HIT_1 = 190,     //      ding.raw        ; tearing metal 1
	SOUND_NASTY_ROBOT_HIT_2 = 191,     //      ding.raw        ; tearing metal 2
#endif
	/* endif */
	/* if DXX_BUILD_DESCENT_II */
	SOUND_LASER_HIT_WATER = 232,
	SOUND_MISSILE_HIT_WATER = 233,
	SOUND_DROP_WEAPON = 39,

	SOUND_FORCEFIELD_BOUNCE_PLAYER = 40,
	SOUND_FORCEFIELD_BOUNCE_WEAPON = 41,
	SOUND_FORCEFIELD_HUM = 42,
	SOUND_FORCEFIELD_OFF = 43,

	SOUND_MARKER_HIT = 50,
	SOUND_BUDDY_MET_GOAL = 51,

	SOUND_BRIEFING_HUM = 94,
	SOUND_BRIEFING_PRINTING = 95,
	SOUND_HUD_JOIN_REQUEST = 123,
	SOUND_HUD_BLUE_GOT_FLAG = 124,
	SOUND_HUD_RED_GOT_FLAG = 125,
	SOUND_HUD_YOU_GOT_FLAG = 126,
	SOUND_HUD_BLUE_GOT_GOAL = 127,
	SOUND_HUD_RED_GOT_GOAL = 128,
	SOUND_HUD_YOU_GOT_GOAL = 129,

	SOUND_LAVAFALL_HISS = 150, // under a lavafall
	SOUND_SHIP_IN_WATER = 152, // sitting (or moving though) water
	SOUND_SHIP_IN_WATERFALL = 158, // under a waterfall

	SOUND_CLOAK_ON = 160, // USED FOR WALL CLOAK
	SOUND_WALL_CLOAK_ON = SOUND_CLOAK_ON,

	SOUND_AMBIENT_LAVA = 222,
	SOUND_AMBIENT_WATER = 223,

	SOUND_CONVERT_ENERGY = 241,
	SOUND_WEAPON_STOLEN = 244,

	SOUND_LIGHT_BLOWNUP = 157,

	SOUND_WALL_REMOVED = 246, // Wall removed, probably due to a wall switch.
	SOUND_AFTERBURNER_IGNITE = 247,
	SOUND_AFTERBURNER_PLAY = 248,

	SOUND_SECRET_EXIT = 249,

	SOUND_SEISMIC_DISTURBANCE_START = 251,

	SOUND_YOU_GOT_ORB = 84,
	SOUND_FRIEND_GOT_ORB = 85,
	SOUND_OPPONENT_GOT_ORB = 86,
	SOUND_OPPONENT_HAS_SCORED = 87,
	/* endif */
};

// I think it would be nice to have a scrape sound...
//#define SOUND_PLAYER_SCRAPE_WALL                72

extern std::array<uint8_t, ::d2x::MAX_SOUNDS> Sounds, AltSounds;
}

constexpr std::integral_constant<int, -1> sound_none{};
#endif

#endif /* _SOUNDS_H */
