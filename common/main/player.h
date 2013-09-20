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
 * Structure information for the player
 *
 */

#ifndef _PLAYER_H
#define _PLAYER_H

#include <physfs.h>
#include "inferno.h"
#include "maths.h"
#include "vecmat.h"
#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
#include "weapon.h"
#endif

#define MAX_PLAYERS 8
#define MAX_MULTI_PLAYERS MAX_PLAYERS+3

// Initial player stat values
#define INITIAL_ENERGY  i2f(100)    // 100% energy to start
#define INITIAL_SHIELDS i2f(100)    // 100% shields to start

#define MAX_ENERGY      i2f(200)    // go up to 200
#define MAX_SHIELDS     i2f(200)

#define INITIAL_LIVES               3   // start off with 3 lives

// Values for special flags
#define PLAYER_FLAGS_INVULNERABLE   1       // Player is invincible
#define PLAYER_FLAGS_BLUE_KEY       2       // Player has blue key
#define PLAYER_FLAGS_RED_KEY        4       // Player has red key
#define PLAYER_FLAGS_GOLD_KEY       8       // Player has gold key
#if defined(DXX_BUILD_DESCENT_II)
#define PLAYER_FLAGS_FLAG           16      // Player has his team's flag
#endif
#define PLAYER_FLAGS_MAP_ALL        64      // Player can see unvisited areas on map
#if defined(DXX_BUILD_DESCENT_II)
#define PLAYER_FLAGS_AMMO_RACK      128     // Player has ammo rack
#define PLAYER_FLAGS_CONVERTER      256     // Player has energy->shield converter
#endif
#define PLAYER_FLAGS_QUAD_LASERS    1024    // Player shoots 4 at once
#define PLAYER_FLAGS_CLOAKED        2048    // Player is cloaked for awhile
#if defined(DXX_BUILD_DESCENT_II)
#define PLAYER_FLAGS_AFTERBURNER    4096    // Player's afterburner is engaged
#define PLAYER_FLAGS_HEADLIGHT      8192    // Player has headlight boost
#define PLAYER_FLAGS_HEADLIGHT_ON   16384   // is headlight on or off?

#define AFTERBURNER_MAX_TIME    (F1_0*5)    // Max time afterburner can be on.
#endif
#define CALLSIGN_LEN                8       // so can use as filename (was: 12)

// Amount of time player is cloaked.
#define CLOAK_TIME_MAX          (F1_0*30)
#define INVULNERABLE_TIME_MAX   (F1_0*30)

#if defined(DXX_BUILD_DESCENT_I)
#define PLAYER_STRUCT_VERSION 	16		//increment this every time player struct changes
#elif defined(DXX_BUILD_DESCENT_II)
#define PLAYER_STRUCT_VERSION   17  // increment this every time player struct changes

// defines for teams
#define TEAM_BLUE   0
#define TEAM_RED    1
#endif

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
// When this structure changes, increment the constant
// SAVE_FILE_VERSION in playsave.c
typedef struct player {
	// Who am I data
	char    callsign[CALLSIGN_LEN+1];   // The callsign of this player, for net purposes.
	sbyte   connected;              // Is the player connected or not?
	int     objnum;                 // What object number this player is. (made an int by mk because it's very often referenced)

	//  -- make sure you're 4 byte aligned now!

	// Game data
	uint    flags;                  // Powerup flags, see below...
	fix     energy;                 // Amount of energy remaining.
	fix     shields;                // shields remaining (protection)
	ubyte   lives;                  // Lives remaining, 0 = game over.
	sbyte   level;                  // Current level player is playing. (must be signed for secret levels)
	ubyte   laser_level;            // Current level of the laser.
	sbyte   starting_level;         // What level the player started on.
	short   killer_objnum;          // Who killed me.... (-1 if no one)
#if defined(DXX_BUILD_DESCENT_I)
	ubyte		primary_weapon_flags;					//	bit set indicates the player has this weapon.
	ubyte		secondary_weapon_flags;					//	bit set indicates the player has this weapon.
#elif defined(DXX_BUILD_DESCENT_II)
	ushort  primary_weapon_flags;   // bit set indicates the player has this weapon.
	ushort  secondary_weapon_flags; // bit set indicates the player has this weapon.
#endif
	ushort  vulcan_ammo;
	ushort  secondary_ammo[MAX_SECONDARY_WEAPONS]; // How much ammo of each type.

	// Statistics...
	int     last_score;             // Score at beginning of current level.
	int     score;                  // Current score.
	fix     time_level;             // Level time played
	fix     time_total;             // Game time played (high word = seconds)

	fix64   cloak_time;             // Time cloaked
	fix64   invulnerable_time;      // Time invulnerable

	short   KillGoalCount;          // Num of players killed this level
	short   net_killed_total;       // Number of times killed total
	short   net_kills_total;        // Number of net kills total
	short   num_kills_level;        // Number of kills this level
	short   num_kills_total;        // Number of kills total
	short   num_robots_level;       // Number of initial robots this level
	short   num_robots_total;       // Number of robots total
	ushort  hostages_rescued_total; // Total number of hostages rescued.
	ushort  hostages_total;         // Total number of hostages.
	ubyte   hostages_on_board;      // Number of hostages on ship.
	ubyte   hostages_level;         // Number of hostages on this level.
	fix     homing_object_dist;     // Distance of nearest homing object.
	sbyte   hours_level;            // Hours played (since time_total can only go up to 9 hours)
	sbyte   hours_total;            // Hours played (since time_total can only go up to 9 hours)
} __pack__ player;

// Same as above but structure how Savegames expect
typedef struct player_rw {
	// Who am I data
	char    callsign[CALLSIGN_LEN+1];   // The callsign of this player, for net purposes.
	ubyte   net_address[6];         // The network address of the player.
	sbyte   connected;              // Is the player connected or not?
	int     objnum;                 // What object number this player is. (made an int by mk because it's very often referenced)
	int     n_packets_got;          // How many packets we got from them
	int     n_packets_sent;         // How many packets we sent to them

	//  -- make sure you're 4 byte aligned now!

	// Game data
	uint    flags;                  // Powerup flags, see below...
	fix     energy;                 // Amount of energy remaining.
	fix     shields;                // shields remaining (protection)
	ubyte   lives;                  // Lives remaining, 0 = game over.
	sbyte   level;                  // Current level player is playing. (must be signed for secret levels)
	ubyte   laser_level;            // Current level of the laser.
	sbyte   starting_level;         // What level the player started on.
	short   killer_objnum;          // Who killed me.... (-1 if no one)
#if defined(DXX_BUILD_DESCENT_I)
	ubyte		primary_weapon_flags;					//	bit set indicates the player has this weapon.
	ubyte		secondary_weapon_flags;					//	bit set indicates the player has this weapon.
#elif defined(DXX_BUILD_DESCENT_II)
	ushort  primary_weapon_flags;   // bit set indicates the player has this weapon.
	ushort  secondary_weapon_flags; // bit set indicates the player has this weapon.
#endif
	ushort	laser_ammo;
	ushort  vulcan_ammo;
	ushort  spreadfire_ammo, plasma_ammo, fusion_ammo;
#if defined(DXX_BUILD_DESCENT_II)
	ushort  obsolete_primary_ammo[MAX_PRIMARY_WEAPONS - 5];
#endif
	ushort  secondary_ammo[MAX_SECONDARY_WEAPONS]; // How much ammo of each type.

#if defined(DXX_BUILD_DESCENT_II)
	ushort  pad; // Pad because increased weapon_flags from byte to short -YW 3/22/95
#endif
	//  -- make sure you're 4 byte aligned now

	// Statistics...
	int     last_score;             // Score at beginning of current level.
	int     score;                  // Current score.
	fix     time_level;             // Level time played
	fix     time_total;             // Game time played (high word = seconds)

	fix     cloak_time;             // Time cloaked
	fix     invulnerable_time;      // Time invulnerable

#if defined(DXX_BUILD_DESCENT_II)
	short   KillGoalCount;          // Num of players killed this level
#endif
	short   net_killed_total;       // Number of times killed total
	short   net_kills_total;        // Number of net kills total
	short   num_kills_level;        // Number of kills this level
	short   num_kills_total;        // Number of kills total
	short   num_robots_level;       // Number of initial robots this level
	short   num_robots_total;       // Number of robots total
	ushort  hostages_rescued_total; // Total number of hostages rescued.
	ushort  hostages_total;         // Total number of hostages.
	ubyte   hostages_on_board;      // Number of hostages on ship.
	ubyte   hostages_level;         // Number of hostages on this level.
	fix     homing_object_dist;     // Distance of nearest homing object.
	sbyte   hours_level;            // Hours played (since time_total can only go up to 9 hours)
	sbyte   hours_total;            // Hours played (since time_total can only go up to 9 hours)
} __pack__ player_rw;
#if defined(DXX_BUILD_DESCENT_I)
typedef char player_rw_padding_check[sizeof(player_rw) == 116 ? 1 : -1];
#elif defined(DXX_BUILD_DESCENT_II)
typedef char player_rw_padding_check[sizeof(player_rw) == 142 ? 1 : -1];
#endif
#endif

#define N_PLAYER_GUNS 8
#define N_PLAYER_SHIP_TEXTURES 32

typedef struct player_ship {
	int     model_num;
	int     expl_vclip_num;
	fix     mass,drag;
	fix     max_thrust,reverse_thrust,brakes;       //low_thrust
	fix     wiggle;
	fix     max_rotthrust;
	vms_vector gun_points[N_PLAYER_GUNS];
}
#if defined(DXX_BUILD_DESCENT_I)
__pack__
#endif
player_ship;

extern int N_players;   // Number of players ( >1 means a net game, eh?)
extern int Player_num;  // The player number who is on the console.

#if defined(DXX_BUILD_DESCENT_I)
#define DXX_PLAYER_HEADER_ADD_EXTRA_PLAYERS	0
#elif defined(DXX_BUILD_DESCENT_II)
#define DXX_PLAYER_HEADER_ADD_EXTRA_PLAYERS	4
#endif
#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
extern player Players[MAX_PLAYERS + DXX_PLAYER_HEADER_ADD_EXTRA_PLAYERS];   // Misc player info
void player_rw_swap(player_rw *p, int swap);
#endif
extern player_ship *Player_ship;

extern struct object *Guided_missile[MAX_PLAYERS];
extern int Guided_missile_sig[MAX_PLAYERS];

/*
 * reads a player_ship structure from a PHYSFS_file
 */
void player_ship_read(player_ship *ps, PHYSFS_file *fp);

#endif
