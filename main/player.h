/* $Id: player.h,v 1.3 2003-03-01 03:56:55 btb Exp $ */
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

#ifndef _PLAYER_H
#define _PLAYER_H

#include "inferno.h"
#include "fix.h"
#include "vecmat.h"
#include "weapon.h"

#define MAX_PLAYERS 8
#define MAX_MULTI_PLAYERS MAX_PLAYERS+3

// Initial player stat values
#define INITIAL_ENERGY  i2f(100)    //100% energy to start
#define INITIAL_SHIELDS i2f(100)    //100% shields to start

#define MAX_ENERGY      i2f(200)    //go up to 200
#define MAX_SHIELDS     i2f(200)

#define INITIAL_LIVES               3   //start off with 3 lives

// Values for special flags
#define PLAYER_FLAGS_INVULNERABLE   1   // Player is invincible
#define PLAYER_FLAGS_BLUE_KEY       2   // Player has blue key
#define PLAYER_FLAGS_RED_KEY        4   // Player has red key
#define PLAYER_FLAGS_GOLD_KEY       8   // Player has gold key
#define PLAYER_FLAGS_FLAG           16  // Player has his team's flag
#define PLAYER_FLAGS_UNUSED         32  //
#define PLAYER_FLAGS_MAP_ALL        64  // Player can see unvisited areas on map
#define PLAYER_FLAGS_AMMO_RACK      128 // Player has ammo rack
#define PLAYER_FLAGS_CONVERTER      256 // Player has energy->shield converter
#define PLAYER_FLAGS_MAP_ALL_CHEAT  512 // Player can see unvisited areas on map normally
#define PLAYER_FLAGS_QUAD_LASERS    1024    // Player shoots 4 at once
#define PLAYER_FLAGS_CLOAKED        2048    // Player is cloaked for awhile
#define PLAYER_FLAGS_AFTERBURNER    4096    // Player's afterburner is engaged
#define PLAYER_FLAGS_HEADLIGHT      8192    // Player has headlight boost
#define PLAYER_FLAGS_HEADLIGHT_ON   16384   // is headlight on or off?

#define AFTERBURNER_MAX_TIME    (F1_0*5)    // Max time afterburner can be on.
#define CALLSIGN_LEN                8       // so can use as filename (was: 12)

// Amount of time player is cloaked.
#define CLOAK_TIME_MAX          (F1_0*30)
#define INVULNERABLE_TIME_MAX   (F1_0*30)

#define PLAYER_STRUCT_VERSION   17        //increment this every time player struct changes

//defines for teams
#define TEAM_BLUE   0
#define TEAM_RED    1

//When this structure changes, increment the constant SAVE_FILE_VERSION
//in playsave.c
typedef struct player {
	// Who am I data
	char    callsign[CALLSIGN_LEN+1];   // The callsign of this player, for net purposes.
	ubyte   net_address[6];         // The network address of the player.
	byte    connected;              // Is the player connected or not?
	int     objnum;                 // What object number this player is. (made an int by mk because it's very often referenced)
	int     n_packets_got;          // How many packets we got from them
	int     n_packets_sent;         // How many packets we sent to them

	//  -- make sure you're 4 byte aligned now!

	// Game data
	uint    flags;                  // Powerup flags, see below...
	fix     energy;                 // Amount of energy remaining.
	fix     shields;                // shields remaining (protection)
	ubyte   lives;                  // Lives remaining, 0 = game over.
	byte    level;                  // Current level player is playing. (must be signed for secret levels)
	ubyte   laser_level;            // Current level of the laser.
	byte    starting_level;         // What level the player started on.
	short   killer_objnum;          // Who killed me.... (-1 if no one)
	ushort  primary_weapon_flags;   // bit set indicates the player has this weapon.
	ushort  secondary_weapon_flags; // bit set indicates the player has this weapon.
	ushort  primary_ammo[MAX_PRIMARY_WEAPONS];  // How much ammo of each type.
	ushort  secondary_ammo[MAX_SECONDARY_WEAPONS]; // How much ammo of each type.

	ushort  pad; // Pad because increased weapon_flags from byte to short -YW 3/22/95

	//  -- make sure you're 4 byte aligned now

	// Statistics...
	int     last_score;             // Score at beginning of current level.
	int     score;                  // Current score.
	fix     time_level;             // Level time played
	fix     time_total;             // Game time played (high word = seconds)

	fix     cloak_time;             // Time cloaked
	fix     invulnerable_time;      // Time invulnerable

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
	byte    hours_level;            // Hours played (since time_total can only go up to 9 hours)
	byte    hours_total;            // Hours played (since time_total can only go up to 9 hours)
} __pack__ player;

#define N_PLAYER_GUNS 8

typedef struct player_ship {
	int         model_num;
	int     expl_vclip_num;
	fix     mass,drag;
	fix     max_thrust,reverse_thrust,brakes;       //low_thrust
	fix     wiggle;
	fix     max_rotthrust;
	vms_vector gun_points[N_PLAYER_GUNS];
} player_ship;

extern int N_players;   // Number of players ( >1 means a net game, eh?)
extern int Player_num;  // The player number who is on the console.

extern player Players[MAX_PLAYERS+4];   // Misc player info
extern player_ship *Player_ship;


//version 16 structure

#define MAX_PRIMARY_WEAPONS16   5
#define MAX_SECONDARY_WEAPONS16 5

typedef struct player16 {
	// Who am I data
	char    callsign[CALLSIGN_LEN+1];   // The callsign of this player, for net purposes.
	ubyte   net_address[6];         // The network address of the player.
	byte    connected;              // Is the player connected or not?
	int     objnum;                 // What object number this player is. (made an int by mk because it's very often referenced)
	int     n_packets_got;          // How many packets we got from them
	int     n_packets_sent;         // How many packets we sent to them

	//  -- make sure you're 4 byte aligned now!

	// Game data
	uint    flags;                  // Powerup flags, see below...
	fix     energy;                 // Amount of energy remaining.
	fix     shields;                // shields remaining (protection)
	ubyte   lives;                  // Lives remaining, 0 = game over.
	byte    level;                  // Current level player is playing. (must be signed for secret levels)
	ubyte   laser_level;            // Current level of the laser.
	byte    starting_level;         // What level the player started on.
	short   killer_objnum;          // Who killed me.... (-1 if no one)
	ubyte   primary_weapon_flags;   // bit set indicates the player has this weapon.
	ubyte   secondary_weapon_flags; // bit set indicates the player has this weapon.
	ushort  primary_ammo[MAX_PRIMARY_WEAPONS16];    // How much ammo of each type.
	ushort  secondary_ammo[MAX_SECONDARY_WEAPONS16];// How much ammo of each type.

	//  -- make sure you're 4 byte aligned now

	// Statistics...
	int     last_score;             // Score at beginning of current level.
	int     score;                  // Current score.
	fix     time_level;             // Level time played
	fix     time_total;             // Game time played (high word = seconds)

	fix     cloak_time;             // Time cloaked
	fix     invulnerable_time;      // Time invulnerable

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
	byte    hours_level;            // Hours played (since time_total can only go up to 9 hours)
	byte    hours_total;            // Hours played (since time_total can only go up to 9 hours)
} __pack__ player16;

/*
 * reads a player_ship structure from a CFILE
 */
void player_ship_read(player_ship *ps, CFILE *fp);

#endif
