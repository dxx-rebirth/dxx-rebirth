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

#pragma once

#include <physfs.h>
#include "fwd-object.h"

#define MAX_MULTI_PLAYERS MAX_PLAYERS+3
#define MULTI_PNUM_UNDEF 0xcc

// Initial player stat values
#define INITIAL_ENERGY  i2f(100)    // 100% energy to start
#define INITIAL_SHIELDS i2f(100)    // 100% shields to start

#define MAX_ENERGY      i2f(200)    // go up to 200
#define MAX_SHIELDS     i2f(200)

#define INITIAL_LIVES               3   // start off with 3 lives

// Values for special flags
#if defined(DXX_BUILD_DESCENT_II)
#define PLAYER_MAX_AMMO(powerup_flags,BASE)	((powerup_flags & PLAYER_FLAGS_AMMO_RACK) ? BASE * 2 : BASE)

#define AFTERBURNER_MAX_TIME    (F1_0*5)    // Max time afterburner can be on.
#endif
#define CALLSIGN_LEN                8       // so can use as filename (was: 12)

// Amount of time player is cloaked.
#define CLOAK_TIME_MAX          (F1_0*30)
#define INVULNERABLE_TIME_MAX   (F1_0*30)

#if defined(DXX_BUILD_DESCENT_I)
#define PLAYER_STRUCT_VERSION 	16		//increment this every time player struct changes
#define PLAYER_MAX_AMMO(powerup_flags,BASE)	(static_cast<void>(powerup_flags), BASE)
#elif defined(DXX_BUILD_DESCENT_II)
#define PLAYER_STRUCT_VERSION   17  // increment this every time player struct changes

// defines for teams
#define TEAM_BLUE   0
#define TEAM_RED    1
#endif

#ifdef __cplusplus
#include "dxxsconf.h"
#include "dsx-ns.h"
#include "compiler-array.h"
struct callsign_t;

#define N_PLAYER_GUNS 8
#define N_PLAYER_SHIP_TEXTURES 32

namespace dcx {
struct player_ship;

struct player;
using playernum_t = uint32_t;
constexpr unsigned MAX_PLAYERS = 8;
typedef array<playernum_t, MAX_PLAYERS> playernum_array_t;

extern unsigned N_players;   // Number of players ( >1 means a net game, eh?)
extern playernum_t Player_num;  // The player number who is on the console.
}

#ifdef dsx
DXX_VALPTRIDX_DECLARE_SUBTYPE(dcx::, player, playernum_t, MAX_PLAYERS);
namespace dsx {
struct player_rw;
struct player_info;
void player_rw_swap(player_rw *p, int swap);
int allowed_to_fire_flare(player_info &);
int allowed_to_fire_missile(const player_info &);
#if defined(DXX_BUILD_DESCENT_II)
extern array<object *, MAX_PLAYERS> Guided_missile;
extern array<object_signature_t, MAX_PLAYERS> Guided_missile_sig;
fix get_omega_energy_consumption(fix delta_charge);
void omega_charge_frame(player_info &);
#endif
}
#endif

/*
 * reads a player_ship structure from a PHYSFS_File
 */
void player_ship_read(player_ship *ps, PHYSFS_File *fp);

#endif
