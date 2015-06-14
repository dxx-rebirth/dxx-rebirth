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
 * Multiplayer code for network play.
 *
 */

#include <bitset>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "u_mem.h"
#include "strutil.h"
#include "game.h"
#include "multi.h"
#include "multiinternal.h"
#include "object.h"
#include "laser.h"
#include "fuelcen.h"
#include "scores.h"
#include "gauges.h"
#include "gameseg.h"
#include "collide.h"
#include "dxxerror.h"
#include "fireball.h"
#include "newmenu.h"
#include "console.h"
#include "wall.h"
#include "cntrlcen.h"
#include "powerup.h"
#include "polyobj.h"
#include "bm.h"
#include "endlevel.h"
#include "key.h"
#include "playsave.h"
#include "timer.h"
#include "digi.h"
#include "sounds.h"
#include "kconfig.h"
#include "hudmsg.h"
#include "newdemo.h"
#include "text.h"
#include "kmatrix.h"
#include "multibot.h"
#include "gameseq.h"
#include "physics.h"
#include "config.h"
#include "ai.h"
#include "switch.h"
#include "textures.h"
#include "byteutil.h"
#include "sounds.h"
#include "args.h"
#include "effects.h"
#include "iff.h"
#include "state.h"
#include "automap.h"
#ifdef USE_UDP
#include "net_udp.h"
#endif

#include "compiler-begin.h"
#include "compiler-exchange.h"
#include "partial_range.h"
#include "highest_valid.h"

constexpr tt::integral_constant<int8_t, -1> owner_none{};

static void multi_reset_object_texture(const vobjptr_t objp);
static void multi_add_lifetime_killed();
static void multi_send_heartbeat();
static void multi_powcap_adjust_remote_cap(const playernum_t pnum);
#if defined(DXX_BUILD_DESCENT_II)
static std::size_t find_goal_texture(ubyte t);
static tmap_info &find_required_goal_texture(ubyte t);
static void multi_do_capture_bonus(const playernum_t pnum);
static void multi_do_orb_bonus(const playernum_t pnum, const ubyte *buf);
static void multi_send_drop_flag(objnum_t objnum,int seed);
#endif
static void multi_send_ranking();
static void multi_new_bounty_target( playernum_t pnum );
static void multi_save_game(ubyte slot, uint id, char *desc);
static void multi_restore_game(ubyte slot, uint id);
static void multi_send_gmode_update();
static void multi_send_quit();
static void multi_process_data(playernum_t pnum, const ubyte *dat, uint_fast32_t type);

//
// Global variables
//

int multi_protocol=0; // set and determinate used protocol
int imulti_new_game=0; // to prep stuff for level only when starting new game

//do we draw the kill list on the HUD?
int Show_kill_list = 1;
int Show_reticle_name = 1;
fix Show_kill_list_timer = 0;

#if defined(DXX_BUILD_DESCENT_II)
int PhallicLimit=0;
int PhallicMan=-1;

char Multi_is_guided=0;
#endif
int Bounty_target = 0;


array<msgsend_state_t, MAX_PLAYERS> multi_sending_message;
int multi_defining_message = 0;
int multi_message_index = 0;

ubyte multibuf[MAX_MULTI_MESSAGE_LEN+4];            // This is where multiplayer message are built

static array<array<objnum_t, MAX_OBJECTS>, MAX_PLAYERS> remote_to_local;  // Remote object number for each local object
static array<uint16_t, MAX_OBJECTS> local_to_remote;
array<sbyte, MAX_OBJECTS> object_owner;   // Who created each object in my universe, -1 = loaded at start

unsigned   Net_create_loc;       // pointer into previous array
array<objnum_t, MAX_NET_CREATE_OBJECTS>   Net_create_objnums; // For tracking object creation that will be sent to remote
int   Network_status = 0;
ntstring<MAX_MESSAGE_LEN - 1> Network_message;
int   Network_message_reciever=-1;
static array<unsigned, MAX_PLAYERS>   sorted_kills;
int   multi_goto_secret = 0;
array<array<uint16_t, MAX_PLAYERS>, MAX_PLAYERS> kill_matrix;
array<uint16_t, 2> team_kills;
int   multi_quit_game = 0;
const char GMNames[MULTI_GAME_TYPE_COUNT][MULTI_GAME_NAME_LENGTH]={
	"Anarchy",
	"Team Anarchy",
	"Robo Anarchy",
	"Cooperative",
#if defined(DXX_BUILD_DESCENT_I)
	"Unknown",
	"Unknown",
	"Unknown",
#elif defined(DXX_BUILD_DESCENT_II)
	"Capture the Flag",
	"Hoard",
	"Team Hoard",
#endif
	"Bounty"
};
const char GMNamesShrt[MULTI_GAME_TYPE_COUNT][8]={
	"ANRCHY",
	"TEAM",
	"ROBO",
	"COOP",
#if defined(DXX_BUILD_DESCENT_I)
	"UNKNOWN",
	"UNKNOWN",
	"UNKNOWN",
#elif defined(DXX_BUILD_DESCENT_II)
	"FLAG",
	"HOARD",
	"TMHOARD",
#endif
	"BOUNTY"
};

// For rejoin object syncing (used here and all protocols - globally)

int	Network_send_objects = 0;  // Are we in the process of sending objects to a player?
int	Network_send_object_mode = 0; // What type of objects are we sending, static or dynamic?
int 	Network_send_objnum = -1;   // What object are we sending next?
int     Network_rejoined = 0;       // Did WE rejoin this game?
int     Network_sending_extras=0;
int     VerifyPlayerJoined=-1;      // Player (num) to enter game before any ingame/extra stuff is being sent
int     Player_joining_extras=-1;  // This is so we know who to send 'latecomer' packets to.
int     Network_player_added = 0;   // Is this a new player or a returning player?

ushort          my_segments_checksum = 0;

netgame_info Netgame;

array<array<bitmap_index, N_PLAYER_SHIP_TEXTURES>, MAX_PLAYERS> multi_player_textures;

// Globals for protocol-bound Refuse-functions
char RefuseThisPlayer=0,WaitForRefuseAnswer=0,RefuseTeam,RefusePlayerName[12];
fix64 RefuseTimeLimit=0;

static const int message_length[] = {
#define define_message_length(NAME,SIZE)	(SIZE),
	for_each_multiplayer_command(define_message_length)
};

powerup_cap_state PowerupCaps;

const array<char[16], 10> RankStrings{{"(unpatched) ","Cadet ","Ensign ","Lieutenant ","Lt.Commander ",
                     "Commander ","Captain ","Vice Admiral ","Admiral ","Demigod "}};

const char multi_allow_powerup_text[MULTI_ALLOW_POWERUP_MAX][MULTI_ALLOW_POWERUP_TEXT_LENGTH] =
{
#define define_netflag_string(NAME,STR)	STR,
	for_each_netflag_value(define_netflag_string)
};

void powerup_cap_state::cap_laser_powerup_level(stored_laser_level &player_level, const powerup_type_t type, const uint_fast32_t level_bias) const
{
	const uint_fast32_t current_in_mine = get_current(type);
	const uint_fast32_t maximum_allowed = get_max(type);
	const uint_fast32_t player_would_drop = player_level - level_bias;
	if (player_would_drop + current_in_mine <= maximum_allowed)
		return;
	const auto capped = (current_in_mine < maximum_allowed) ? stored_laser_level(maximum_allowed - current_in_mine + level_bias) : stored_laser_level(LASER_LEVEL_1);
	player_level = capped;
	con_printf(CON_VERBOSE, "Capping laser due to powerup cap: current=%u max=%u was=%u capped=%u", get_current(type), get_max(type), static_cast<unsigned>(player_would_drop), static_cast<laser_level_t>(capped));
}

void powerup_cap_state::cap_laser_level(stored_laser_level &player_level) const
{
#if defined(DXX_BUILD_DESCENT_II)
	if (player_level > MAX_LASER_LEVEL)
	{
		cap_laser_powerup_level(player_level, POW_SUPER_LASER, MAX_LASER_LEVEL);
		return;
	}
#endif
	cap_laser_powerup_level(player_level, POW_LASER, 0);
}

void powerup_cap_state::cap_secondary_ammo(powerup_type_t type, uint16_t &player_ammo) const
{
	const auto idx = map_powerup_type_to_index(type);
	const uint_fast32_t current_on_player = player_ammo;
	const uint_fast32_t current_in_mine = get_current(idx);
	const uint_fast32_t maximum_allowed = get_max(idx);
	if (current_on_player + current_in_mine <= maximum_allowed)
		return;
	const auto capped = (current_in_mine < maximum_allowed) ? static_cast<uint16_t>(maximum_allowed - current_in_mine) : 0;
	player_ammo = capped;
	con_printf(CON_VERBOSE, "Capping secondary %u due to powerup cap: current=%u max=%u was=%u now=%hu", idx, get_current(idx), get_max(idx), static_cast<unsigned>(current_on_player), capped);
}

void powerup_cap_state::cap_flag(uint32_t &player_flags, const uint32_t powerup_flag, const powerup_type_t idx) const
{
	if (player_flags & powerup_flag)
		if (!can_add_mapped_powerup(idx))
		{
			player_flags &= ~powerup_flag;
			con_printf(CON_VERBOSE, "Capping accessory %u due to powerup cap: current=%u max=%u", idx, get_current(idx), get_max(idx));
		}
}

void powerup_cap_state::reset_powerup_both(powerup_type_t type)
{
	const auto idx = map_powerup_type_to_index(type);
	m_powerups[idx] = 0;
	m_max[idx] = 0;
}

void powerup_cap_state::clear()
{
	m_powerups = {};
	m_max = {};
}

int GetMyNetRanking()
{
	int rank, eff;

	if (PlayerCfg.NetlifeKills+PlayerCfg.NetlifeKilled==0)
		return (1);

	rank=(int) (((float)PlayerCfg.NetlifeKills/3000.0)*8.0);

	eff=(int)((float)((float)PlayerCfg.NetlifeKills/((float)PlayerCfg.NetlifeKilled+(float)PlayerCfg.NetlifeKills))*100.0);

	if (rank>8)
		rank=8;

	if (eff<0)
		eff=0;

	if (eff<60)
		rank-=((59-eff)/10);

	if (rank<0)
		rank=0;
	if (rank>8)
		rank=8;

	return (rank+1);
}

void ClipRank (ubyte *rank)
{
	// This function insures no crashes when dealing with D2 1.0
	if (*rank > 9)
		*rank = 0;
}

//
//  Functions that replace what used to be macros
//

objnum_t objnum_remote_to_local(uint16_t remote_objnum, int8_t owner)
{
	if (owner == owner_none)
		return(remote_objnum);
	// Map a remote object number from owner to a local object number
	if ((owner >= N_players) || (owner < -1)) {
		Int3(); // Illegal!
		return(remote_objnum);
	}

	if (remote_objnum >= MAX_OBJECTS)
		return(object_none);

	auto result = remote_to_local[owner][remote_objnum];
	return(result);
}

owned_remote_objnum objnum_local_to_remote(objnum_t local_objnum)
{
	// Map a local object number to a remote + owner
	if (local_objnum > Highest_object_index)
	{
		return {owner_none, 0xffff};
	}
	auto owner = object_owner[local_objnum];
	if (owner == owner_none)
		return {owner, local_objnum};
	if (owner >= N_players || owner < -1)
		throw std::runtime_error("illegal object owner");
	auto result = local_to_remote[local_objnum];
	if (result >= MAX_OBJECTS)
		throw std::runtime_error("illegal object remote number");	// See Rob, object has no remote number!
	return {owner, result};
}

uint16_t objnum_local_to_remote(objnum_t local_objnum, int8_t *owner)
{
	auto r = objnum_local_to_remote(local_objnum);
	*owner = r.owner;
	return r.objnum;
}

void
map_objnum_local_to_remote(int local_objnum, int remote_objnum, int owner)
{
	// Add a mapping from a network remote object number to a local one

	Assert(local_objnum > -1);
	Assert(local_objnum < MAX_OBJECTS);
	Assert(remote_objnum > -1);
	Assert(remote_objnum < MAX_OBJECTS);
	Assert(owner > -1);
	Assert(owner != Player_num);

	object_owner[local_objnum] = owner;

	remote_to_local[owner][remote_objnum] = local_objnum;
	local_to_remote[local_objnum] = remote_objnum;

	return;
}

void map_objnum_local_to_local(objnum_t local_objnum)
{
	// Add a mapping for our locally created objects
	Assert(local_objnum < MAX_OBJECTS);

	object_owner[local_objnum] = Player_num;
	remote_to_local[Player_num][local_objnum] = local_objnum;
	local_to_remote[local_objnum] = local_objnum;

	return;
}

void reset_network_objects()
{
	local_to_remote.fill(-1);
	range_for (auto &i, remote_to_local)
		i.fill(object_none);
	object_owner.fill(-1);
}

int multi_objnum_is_past(objnum_t objnum)
{
	switch (multi_protocol)
	{
		case MULTI_PROTO_UDP:
#ifdef USE_UDP
			return net_udp_objnum_is_past(objnum);
			break;
#endif
		default:
			Error("Protocol handling missing in multi_objnum_is_past\n");
			break;
	}
}

//
// Part 1 : functions whose main purpose in life is to divert the flow
//          of execution to either network  specific code based
//          on the curretn Game_mode value.
//

// Show a score list to end of net players
kmatrix_result multi_endlevel_score()
{
	int old_connect=0, game_wind_visible = 0;

	// If there still is a Game_wind and it's suspended (usually both should be the case), bring it up again so host can still take actions of the game
	if (Game_wind)
	{
		if (!window_is_visible(Game_wind))
		{
			game_wind_visible = 1;
			window_set_visible(Game_wind, 1);
		}
	}
	// Save connect state and change to new connect state
	if (Game_mode & GM_NETWORK)
	{
		old_connect = Players[Player_num].connected;
		if (Players[Player_num].connected!=CONNECT_DIED_IN_MINE)
			Players[Player_num].connected = CONNECT_END_MENU;
		Network_status = NETSTAT_ENDLEVEL;
	}

	// Do the actual screen we wish to show
	const auto rval = kmatrix_view(Game_mode & GM_NETWORK);

	// Restore connect state

	if (Game_mode & GM_NETWORK)
	{
		Players[Player_num].connected = old_connect;
	}

	if (Game_mode & GM_MULTI_COOP)
	{
		range_for (auto &i, partial_range(Players, Netgame.max_numplayers))
			// Reset keys
			i.flags &= ~(PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_GOLD_KEY);
	}

#if defined(DXX_BUILD_DESCENT_II)
	range_for (auto &i, partial_range(Players, Netgame.max_numplayers))
		i.flags &= ~(PLAYER_FLAGS_FLAG);  // Clear capture flag
#endif

	range_for (auto &i, Players)
		i.KillGoalCount=0;

	PowerupCaps.clear();

	// hide Game_wind again if we brought it up
	if (Game_wind && game_wind_visible)
		window_set_visible(Game_wind, 0);

        return rval;
}

int get_team(const playernum_t pnum)
{
	if (Netgame.team_vector & (1 << pnum))
		return 1;
	else
		return 0;
}

void
multi_new_game(void)
{
	// Reset variables for a new net game

	for (uint_fast32_t i = 0; i < MAX_PLAYERS; i++)
		init_player_stats_game(i);

	kill_matrix = {}; // Clear kill matrix

	for (uint_fast32_t i = 0; i < MAX_PLAYERS; i++)
	{
		sorted_kills[i] = i;
		Players[i].connected = CONNECT_DISCONNECTED;
		Players[i].net_killed_total = 0;
		Players[i].net_kills_total = 0;
		Players[i].flags = 0;
		Players[i].KillGoalCount=0;
		multi_sending_message[i] = msgsend_none;
	}

	for (uint_fast32_t i = 0; i < MAX_ROBOTS_CONTROLLED; i++)
	{
		robot_controlled[i] = -1;
		robot_agitation[i] = 0;
		robot_fired[i] = 0;
	}

	PowerupCaps.clear();

	team_kills[0] = team_kills[1] = 0;
	imulti_new_game=1;
	multi_quit_game = 0;
	Show_kill_list = 1;
	game_disable_cheats();
}

void multi_make_player_ghost(const playernum_t playernum)
{
	if (playernum == Player_num || playernum >= MAX_PLAYERS)
	{
		Int3(); // Non-terminal, see Rob
		return;
	}
	auto obj = vobjptridx(Players[playernum].objnum);
	obj->type = OBJ_GHOST;
	obj->render_type = RT_NONE;
	obj->movement_type = MT_NONE;
	multi_reset_player_object(obj);
	multi_strip_robots(playernum);
}

void multi_make_ghost_player(const playernum_t playernum)
{
	if ((playernum == Player_num) || (playernum >= MAX_PLAYERS))
	{
		Int3(); // Non-terminal, see rob
		return;
	}
	auto obj = vobjptridx(Players[playernum].objnum);
	obj->type = OBJ_PLAYER;
	obj->movement_type = MT_PHYSICS;
	multi_reset_player_object(obj);
	if (playernum != Player_num)
		init_player_stats_new_ship(playernum);
}

int multi_get_kill_list(playernum_array_t &plist)
{
	// Returns the number of active net players and their
	// sorted order of kills
	int n = 0;

	range_for (const auto i, partial_range(sorted_kills, N_players))
		//if (Players[sorted_kills[i]].connected)
		plist[n++] = i;

	if (n == 0)
		Int3(); // SEE ROB OR MATT

	//memcpy(plist, sorted_kills, N_players*sizeof(int));

	return(n);
}

void
multi_sort_kill_list(void)
{
	// Sort the kills list each time a new kill is added
	int kills[MAX_PLAYERS];
	for (uint_fast32_t i = 0; i < MAX_PLAYERS; i++)
	{
		if (Game_mode & GM_MULTI_COOP)
			kills[i] = Players[i].score;
#if defined(DXX_BUILD_DESCENT_II)
		else
		if (Show_kill_list==2)
		{
			if (Players[i].net_killed_total+Players[i].net_kills_total==0)
				kills[i]=-1;  // always draw the ones without any ratio last
			else
				kills[i]=(int)((float)((float)Players[i].net_kills_total/((float)Players[i].net_killed_total+(float)Players[i].net_kills_total))*100.0);
		}
#endif
		else
			kills[i] = Players[i].net_kills_total;
	}

	const auto predicate = [&](unsigned a, unsigned b) {
		return kills[a] > kills[b];
	};
	const auto &range = partial_range(sorted_kills, N_players);
	std::sort(range.begin(), range.end(), predicate);
}

static const char *prepare_kill_name(const playernum_t pnum, char (&buf)[(CALLSIGN_LEN*2)+4])
{
	if (Game_mode & GM_TEAM)
	{
		snprintf(buf, sizeof(buf), "%s (%s)", static_cast<const char *>(Players[pnum].callsign), static_cast<const char *>(Netgame.team_name[get_team(pnum)]));
		return buf;
	}
	else
		return static_cast<const char *>(Players[pnum].callsign);
}

static void multi_compute_kill(const objptridx_t killer, const vobjptridx_t killed)
{
	// Figure out the results of a network kills and add it to the
	// appropriate player's tally.

	playernum_t killed_pnum, killer_pnum;
	int TheGoal;

	// Both object numbers are localized already!

	const auto killed_type = killed->type;
	if ((killed_type != OBJ_PLAYER) && (killed_type != OBJ_GHOST))
	{
		Int3(); // compute_kill passed non-player object!
		return;
	}

	killed_pnum = get_player_id(killed);

	Assert (killed_pnum < N_players);

	char killed_buf[(CALLSIGN_LEN*2)+4];
	const char *killed_name = prepare_kill_name(killed_pnum, killed_buf);

	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_multi_death(killed_pnum);

	digi_play_sample( SOUND_HUD_KILL, F3_0 );

#if defined(DXX_BUILD_DESCENT_II)
	if (Control_center_destroyed)
		Players[killed_pnum].connected=CONNECT_DIED_IN_MINE;
#endif

	if (killer == object_none)
		return;
	const auto killer_type = killer->type;
	if (killer_type == OBJ_CNTRLCEN)
	{
		Players[killed_pnum].net_killed_total++;
		Players[killed_pnum].net_kills_total--;

		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_multi_kill(killed_pnum, -1);

		if (killed_pnum == Player_num)
		{
			HUD_init_message(HM_MULTI, "%s %s.", TXT_YOU_WERE, TXT_KILLED_BY_NONPLAY);
			multi_add_lifetime_killed ();
		}
		else
			HUD_init_message(HM_MULTI, "%s %s %s.", killed_name, TXT_WAS, TXT_KILLED_BY_NONPLAY );
		return;
	}

	else if ((killer_type != OBJ_PLAYER) && (killer_type != OBJ_GHOST))
	{
#if defined(DXX_BUILD_DESCENT_II)
		const auto killer_id = killer->id;
		if (killer_id==PMINE_ID && killer_type!=OBJ_ROBOT)
		{
			if (killed_pnum == Player_num)
				HUD_init_message_literal(HM_MULTI, "You were killed by a mine!");
			else
				HUD_init_message(HM_MULTI, "%s was killed by a mine!",killed_name);
		}
		else
#endif
		{
			if (killed_pnum == Player_num)
			{
				HUD_init_message(HM_MULTI, "%s %s.", TXT_YOU_WERE, TXT_KILLED_BY_ROBOT);
				multi_add_lifetime_killed();
			}
			else
				HUD_init_message(HM_MULTI, "%s %s %s.", killed_name, TXT_WAS, TXT_KILLED_BY_ROBOT );
		}
		Players[killed_pnum].net_killed_total++;
		return;
	}

	killer_pnum = get_player_id(killer);

	char killer_buf[(CALLSIGN_LEN*2)+4];
	const char *killer_name = prepare_kill_name(killer_pnum, killer_buf);

	// Beyond this point, it was definitely a player-player kill situation

	if (killer_pnum >= N_players)
		Int3(); // See rob, tracking down bug with kill HUD messages
	if (killed_pnum >= N_players)
		Int3(); // See rob, tracking down bug with kill HUD messages

	if (killer_pnum == killed_pnum)
	{
#if defined(DXX_BUILD_DESCENT_II)
		if (!game_mode_hoard())
#endif
		{
			if (Game_mode & GM_TEAM)
			{
				team_kills[get_team(killed_pnum)] -= 1;
			}

			Players[killed_pnum].net_killed_total += 1;
			Players[killed_pnum].net_kills_total -= 1;

			if (Newdemo_state == ND_STATE_RECORDING)
				newdemo_record_multi_kill(killed_pnum, -1);
		}
		kill_matrix[killed_pnum][killed_pnum] += 1; // # of suicides

		if (killer_pnum == Player_num)
		{
			HUD_init_message(HM_MULTI, "%s %s %s!", TXT_YOU, TXT_KILLED, TXT_YOURSELF );
			multi_add_lifetime_killed();
		}
		else
			HUD_init_message(HM_MULTI, "%s %s", killed_name, TXT_SUICIDE);

		/* Bounty mode needs some lovin' */
		if( Game_mode & GM_BOUNTY && killed_pnum == Bounty_target && multi_i_am_master() )
		{
			/* Select a random number */
			int n = d_rand() % MAX_PLAYERS;
			
			/* Make sure they're valid: Don't check against kill flags,
			* just in case everyone's dead! */
			while( !Players[n].connected )
				n = d_rand() % MAX_PLAYERS;
			
			/* Select new target  - it will be sent later when we're done with this function */
			multi_new_bounty_target( n );
		}
	}

	else
	{
#if defined(DXX_BUILD_DESCENT_II)
		if (!game_mode_hoard())
#endif
		{
			if (Game_mode & GM_TEAM)
			{
				if (get_team(killed_pnum) == get_team(killer_pnum))
				{
					team_kills[get_team(killed_pnum)] -= 1;
					Players[killer_pnum].net_kills_total -= 1;
				}
				else
				{
					team_kills[get_team(killer_pnum)] += 1;
					Players[killer_pnum].net_kills_total += 1;
					Players[killer_pnum].KillGoalCount +=1;
				}
			}
			else if( Game_mode & GM_BOUNTY )
			{
				/* Did the target die?  Did the target get a kill? */
				if( killed_pnum == Bounty_target || killer_pnum == Bounty_target )
				{
					/* Increment kill counts */
					Players[killer_pnum].net_kills_total++;
					Players[killer_pnum].KillGoalCount++;
					
					/* Record the kill in a demo */
					if( Newdemo_state == ND_STATE_RECORDING )
						newdemo_record_multi_kill( killer_pnum, 1 );
					
					/* If the target died, the new one is set! */
					if( killed_pnum == Bounty_target )
						multi_new_bounty_target( killer_pnum );
				}
			}
			else
			{
				Players[killer_pnum].net_kills_total += 1;
				Players[killer_pnum].KillGoalCount+=1;
			}
			
			if (Newdemo_state == ND_STATE_RECORDING && !( Game_mode & GM_BOUNTY ) )
				newdemo_record_multi_kill(killer_pnum, 1);
		}

		kill_matrix[killer_pnum][killed_pnum] += 1;
		Players[killed_pnum].net_killed_total += 1;
		if (killer_pnum == Player_num) {
			HUD_init_message(HM_MULTI, "%s %s %s!", TXT_YOU, TXT_KILLED, killed_name);
			multi_add_lifetime_kills();
			if ((Game_mode & GM_MULTI_COOP) && (Players[Player_num].score >= 1000))
				add_points_to_score(-1000);
		}
		else if (killed_pnum == Player_num)
		{
			HUD_init_message(HM_MULTI, "%s %s %s!", killer_name, TXT_KILLED, TXT_YOU);
			multi_add_lifetime_killed();
		}
		else
			HUD_init_message(HM_MULTI, "%s %s %s!", killer_name, TXT_KILLED, killed_name);
	}

	TheGoal=Netgame.KillGoal*5;

	if (Netgame.KillGoal>0)
	{
		if (Players[killer_pnum].KillGoalCount>=TheGoal)
		{
			if (killer_pnum==Player_num)
			{
				HUD_init_message_literal(HM_MULTI, "You reached the kill goal!");
				Players[Player_num].shields=i2f(200);
			}
			else
				HUD_init_message(HM_MULTI, "%s has reached the kill goal!", static_cast<const char *>(Players[killer_pnum].callsign));

			HUD_init_message_literal(HM_MULTI, "The control center has been destroyed!");
			net_destroy_controlcen (obj_find_first_of_type (OBJ_CNTRLCEN));
		}
	}

	multi_sort_kill_list();
	multi_show_player_list();
#if defined(DXX_BUILD_DESCENT_II)
	Players[killed_pnum].flags&=(~(PLAYER_FLAGS_HEADLIGHT_ON));  // clear the killed guys flags/headlights
#endif
}

void multi_do_protocol_frame(int force, int listen)
{
	switch (multi_protocol)
	{
#ifdef USE_UDP
		case MULTI_PROTO_UDP:
			net_udp_do_frame(force, listen);
			break;
#endif
		default:
			Error("Protocol handling missing in multi_do_protocol_frame\n");
			break;
	}
}

void multi_do_frame(void)
{
	static int lasttime=0;
	static fix64 last_update_time = 0;
	int i;

	if (!(Game_mode & GM_MULTI) || Newdemo_state == ND_STATE_PLAYBACK)
	{
		Int3();
		return;
	}

	if ((Game_mode & GM_NETWORK) && Netgame.PlayTimeAllowed && lasttime!=f2i (ThisLevelTime))
	{
		for (i=0;i<N_players;i++)
			if (Players[i].connected)
			{
				if (i==Player_num)
				{
					multi_send_heartbeat();
					lasttime=f2i(ThisLevelTime);
				}
				break;
			}
	}

	// Send update about our game mode-specific variables every 2 secs (to keep in sync since delayed kills can invalidate these infos on Clients)
	if (multi_i_am_master() && timer_query() >= last_update_time + (F1_0*2))
	{
		multi_send_gmode_update();
		last_update_time = timer_query();
	}

	multi_send_message(); // Send any waiting messages

	if (Game_mode & GM_MULTI_ROBOTS)
	{
		multi_check_robot_timeout();
	}

	multi_do_protocol_frame(0, 1);

	if (multi_quit_game)
	{
		multi_quit_game = 0;
		if (Game_wind)
			window_close(Game_wind);
	}
}

void _multi_send_data(const ubyte *buf, unsigned len, int priority)
{
	if (Game_mode & GM_NETWORK)
	{
		switch (multi_protocol)
		{
#ifdef USE_UDP
			case MULTI_PROTO_UDP:
				net_udp_send_data(buf, len, priority);
				break;
#endif
			default:
				Error("Protocol handling missing in multi_send_data\n");
				break;
		}
	}
}

static void _multi_send_data_direct(const ubyte *buf, unsigned len, const playernum_t pnum, int priority)
{
	if (pnum >= MAX_PLAYERS)
		Error("multi_send_data_direct: Illegal player num: %u\n", pnum);

	switch (multi_protocol)
	{
#ifdef USE_UDP
		case MULTI_PROTO_UDP:
			net_udp_send_mdata_direct(buf, len, pnum, priority);
			break;
#endif
		default:
			Error("Protocol handling missing in multi_send_data_direct\n");
			break;
	}
}

template <multiplayer_command_t C>
static void multi_send_data_direct(ubyte *buf, unsigned len, const playernum_t pnum, int priority)
{
	buf[0] = C;
	unsigned expected = command_length<C>::value;
	if (len != expected)
	{
#ifdef DXX_HAVE_BUILTIN_CONSTANT_P
		if (__builtin_constant_p(len) && __builtin_constant_p(len != expected))
			DXX_ALWAYS_ERROR_FUNCTION(dxx_trap_multi_send_data, "wrong packet size");
#endif
		Error("multi_send_data_direct: Packet type %i length: %i, expected: %i\n", C, len, expected);
	}
	_multi_send_data_direct(buf, len, pnum, priority);
}

void
multi_leave_game(void)
{

	if (!(Game_mode & GM_MULTI))
		return;

	if (Game_mode & GM_NETWORK)
	{
		Net_create_loc = 0;
		const auto cobjp = vobjptridx(Players[Player_num].objnum);
		multi_send_position(cobjp);
		multi_powcap_cap_objects();
		if (!Player_eggs_dropped)
		{
			drop_player_eggs(cobjp);
			Player_eggs_dropped = 1;
		}
		multi_send_player_deres(deres_drop);
	}

	multi_send_quit();

	if (Game_mode & GM_NETWORK)
	{
		switch (multi_protocol)
		{
#ifdef USE_UDP
			case MULTI_PROTO_UDP:
				net_udp_leave_game();
				break;
#endif
			default:
				Error("Protocol handling missing in multi_leave_game\n");
				break;
		}
	}
#if defined(DXX_BUILD_DESCENT_I)
	plyr_save_stats();
#endif
}

void
multi_show_player_list()
{
	if (!(Game_mode & GM_MULTI) || (Game_mode & GM_MULTI_COOP))
		return;

	if (Show_kill_list)
		return;

	Show_kill_list_timer = F1_0*5; // 5 second timer
	Show_kill_list = 1;
}

int
multi_endlevel(int *secret)
{
	int result = 0;

	switch (multi_protocol)
	{
#ifdef USE_UDP
		case MULTI_PROTO_UDP:
			result = net_udp_endlevel(secret);
			break;
#endif
		default:
			Error("Protocol handling missing in multi_endlevel\n");
			break;
	}

	return(result);
}

int multi_endlevel_poll1( newmenu *menu,const d_event &event, const unused_newmenu_userdata_t *userdata)
{
	switch (multi_protocol)
	{
#ifdef USE_UDP
		case MULTI_PROTO_UDP:
			return net_udp_kmatrix_poll1( menu, event, userdata );
			break;
#endif
		default:
			Error("Protocol handling missing in multi_endlevel_poll1\n");
			break;
	}
	
	return 0;	// kill warning
}

int multi_endlevel_poll2( newmenu *menu,const d_event &event, const unused_newmenu_userdata_t *userdata)
{
	switch (multi_protocol)
	{
#ifdef USE_UDP
		case MULTI_PROTO_UDP:
			return net_udp_kmatrix_poll2( menu, event, userdata );
			break;
#endif
		default:
			Error("Protocol handling missing in multi_endlevel_poll2\n");
			break;
	}
	
	return 0;
}

void multi_send_endlevel_packet()
{
	switch (multi_protocol)
	{
#ifdef USE_UDP
		case MULTI_PROTO_UDP:
			net_udp_send_endlevel_packet();
			break;
#endif
		default:
			Error("Protocol handling missing in multi_send_endlevel_packet\n");
			break;
	}
}

//
// Part 2 : functions that act on network messages and change the
//          the state of the game in some way.
//

void
multi_define_macro(int key)
{
	if (!(Game_mode & GM_MULTI))
		return;

	key &= (~KEY_SHIFTED);

	switch(key)
	{
		case KEY_F9:
			multi_defining_message = 1; break;
		case KEY_F10:
			multi_defining_message = 2; break;
		case KEY_F11:
			multi_defining_message = 3; break;
		case KEY_F12:
			multi_defining_message = 4; break;
		default:
			Int3();
	}

	if (multi_defining_message)     {
		key_toggle_repeat(1);
		multi_message_index = 0;
		Network_message[multi_message_index] = 0;
	}

}

static void multi_message_feedback(void)
{
	char *colon;
	int found = 0;
	int i;
	char feedback_result[200];

	if (!(!(colon = strstr(Network_message.data(), ": ")) || colon == Network_message.data() || colon - Network_message.data() > CALLSIGN_LEN))
	{
		std::size_t feedlen = snprintf(feedback_result, sizeof(feedback_result), "%s ", TXT_MESSAGE_SENT_TO);
		if ((Game_mode & GM_TEAM) && (Network_message[0] == '1' || Network_message[0] == '2'))
		{
			snprintf(feedback_result + feedlen, sizeof(feedback_result) - feedlen, "%s '%s'", TXT_TEAM, static_cast<const char *>(Netgame.team_name[Network_message[0] - '1']));
			found = 1;
		}
		if (Game_mode & GM_TEAM)
		{
			for (i = 0; i < N_players; i++)
			{
				if (!d_strnicmp(Netgame.team_name[i], Network_message.data(), colon - Network_message.data()))
				{
					const char *comma = found ? ", " : "";
					found++;
					const char *newline = (!(found % 4)) ? "\n" : "";
					size_t l = strlen(feedback_result);
					snprintf(feedback_result + l, sizeof(feedback_result) - l, "%s%s%s '%s'", comma, newline, TXT_TEAM, static_cast<const char *>(Netgame.team_name[i]));
				}
			}
		}
		range_for (auto &i, partial_range(Players, N_players))
		{
			if (&i != &Players[Player_num] && i.connected && !d_strnicmp(static_cast<const char *>(i.callsign), Network_message.data(), colon - Network_message.data()))
			{
				const char *comma = found ? ", " : "";
				found++;
				const char *newline = (!(found % 4)) ? "\n" : "";
				size_t l = strlen(feedback_result);
				snprintf(feedback_result + l, sizeof(feedback_result) - l, "%s%s%s", comma, newline, static_cast<const char *>(i.callsign));
			}
		}
		if (!found)
			strcat(feedback_result, TXT_NOBODY);
		else
			strcat(feedback_result, ".");

		digi_play_sample(SOUND_HUD_MESSAGE, F1_0);

		Assert(strlen(feedback_result) < 200);

		HUD_init_message_literal(HM_MULTI, feedback_result);
	}
}

void
multi_send_macro(int key)
{
	if (! (Game_mode & GM_MULTI) )
		return;

	switch(key)
	{
		case KEY_F9:
			key = 0; break;
		case KEY_F10:
			key = 1; break;
		case KEY_F11:
			key = 2; break;
		case KEY_F12:
			key = 3; break;
		default:
			Int3();
	}

	if (!PlayerCfg.NetworkMessageMacro[key][0])
	{
		HUD_init_message_literal(HM_MULTI, TXT_NO_MACRO);
		return;
	}

	Network_message = PlayerCfg.NetworkMessageMacro[key];
	Network_message_reciever = 100;

	HUD_init_message(HM_MULTI, "%s '%s'", TXT_SENDING, Network_message.data());
	multi_message_feedback();
}


void
multi_send_message_start()
{
	if (Game_mode&GM_MULTI) {
		multi_sending_message[Player_num] = msgsend_typing;
		multi_send_msgsend_state(msgsend_typing);
		multi_message_index = 0;
		Network_message[multi_message_index] = 0;
		key_toggle_repeat(1);
	}
}

static void kick_player(player &plr, netplayer_info &nplr)
{
	switch (multi_protocol)
	{
#ifdef USE_UDP
		case MULTI_PROTO_UDP:
			net_udp_dump_player(nplr.protocol.udp.addr, DUMP_KICKED);
			break;
#endif
		default:
			Error("Protocol handling missing in multi_send_message_end\n");
			break;
	}

	HUD_init_message(HM_MULTI, "Dumping %s...", static_cast<const char *>(plr.callsign));
	multi_message_index = 0;
	multi_sending_message[Player_num] = msgsend_none;
#if defined(DXX_BUILD_DESCENT_II)
	multi_send_msgsend_state(msgsend_none);
#endif
}

static void multi_send_message_end()
{
	char *mytempbuf;
	int i;

#if defined(DXX_BUILD_DESCENT_I)
	multi_message_index = 0;
	multi_sending_message[Player_num] = msgsend_none;
	multi_send_msgsend_state(msgsend_none);
	key_toggle_repeat(0);
#elif defined(DXX_BUILD_DESCENT_II)
	Network_message_reciever = 100;
#endif

	if (!d_strnicmp(Network_message.data(), "/Handicap: "))
	{
		mytempbuf=&Network_message[11];
		StartingShields=atol (mytempbuf);
		if (StartingShields<10)
			StartingShields=10;
		if (StartingShields>100)
		{
			snprintf(Network_message.data(), Network_message.size(), "%s has tried to cheat!",static_cast<const char *>(Players[Player_num].callsign));
			StartingShields=100;
		}
		else
			snprintf(Network_message.data(), Network_message.size(), "%s handicap is now %d",static_cast<const char *>(Players[Player_num].callsign), StartingShields);
		HUD_init_message(HM_MULTI, "Telling others of your handicap of %d!",StartingShields);
		StartingShields=i2f(StartingShields);
	}
	else if (!d_strnicmp(Network_message.data(), "/move: "))
	{
		if ((Game_mode & GM_NETWORK) && (Game_mode & GM_TEAM))
		{
			unsigned name_index=7;
			if (strlen(Network_message.data()) > 7)
				while (Network_message[name_index] == ' ')
					name_index++;

			if (!multi_i_am_master())
			{
				HUD_init_message(HM_MULTI, "Only %s can move players!",static_cast<const char *>(Players[multi_who_is_master()].callsign));
				return;
			}

			if (strlen(Network_message.data()) <= name_index)
			{
				HUD_init_message_literal(HM_MULTI, "You must specify a name to move");
				return;
			}

			for (i = 0; i < N_players; i++)
				if (Players[i].connected && !d_strnicmp(static_cast<const char *>(Players[i].callsign), &Network_message[name_index], strlen(Network_message.data()) - name_index))
				{
#if defined(DXX_BUILD_DESCENT_II)
					if (game_mode_capture_flag() && (Players[i].flags & PLAYER_FLAGS_FLAG))
					{
						HUD_init_message_literal(HM_MULTI, "Can't move player because s/he has a flag!");
						return;
					}
#endif
					if (Netgame.team_vector & (1<<i))
						Netgame.team_vector&=(~(1<<i));
					else
						Netgame.team_vector|=(1<<i);

					range_for (auto &t, partial_range(Players, N_players))
						if (t.connected)
							multi_reset_object_texture (&Objects[t.objnum]);
					reset_cockpit();

					multi_send_gmode_update();

					snprintf(Network_message.data(), Network_message.size(), "%s has changed teams!", static_cast<const char *>(Players[i].callsign));
					if (i==Player_num)
					{
						HUD_init_message_literal(HM_MULTI, "You have changed teams!");
						reset_cockpit();
					}
					else
						HUD_init_message(HM_MULTI, "Moving %s to other team.", static_cast<const char *>(Players[i].callsign));
					break;
				}
		}
	}

	else if (!d_strnicmp(Network_message.data(), "/kick: ") && (Game_mode & GM_NETWORK))
	{
		unsigned name_index=7;
		if (strlen(Network_message.data()) > 7)
			while (Network_message[name_index] == ' ')
				name_index++;

		if (!multi_i_am_master())
		{
			HUD_init_message(HM_MULTI, "Only %s can kick others out!", static_cast<const char *>(Players[multi_who_is_master()].callsign));
			multi_message_index = 0;
			multi_sending_message[Player_num] = msgsend_none;
#if defined(DXX_BUILD_DESCENT_II)
			multi_send_msgsend_state(msgsend_none);
#endif
			return;
		}
		if (strlen(Network_message.data()) <= name_index)
		{
			HUD_init_message_literal(HM_MULTI, "You must specify a name to kick");
			multi_message_index = 0;
			multi_sending_message[Player_num] = msgsend_none;
#if defined(DXX_BUILD_DESCENT_II)
			multi_send_msgsend_state(msgsend_none);
#endif
			return;
		}

		if (Network_message[name_index] == '#' && isdigit(Network_message[name_index+1])) {
			playernum_array_t players;
			int listpos = Network_message[name_index+1] - '0';

			if (Show_kill_list==1 || Show_kill_list==2) {
				if (listpos == 0 || listpos >= N_players) {
					HUD_init_message_literal(HM_MULTI, "Invalid player number for kick.");
					multi_message_index = 0;
					multi_sending_message[Player_num] = msgsend_none;
#if defined(DXX_BUILD_DESCENT_II)
					multi_send_msgsend_state(msgsend_none);
#endif
					return;
				}
				multi_get_kill_list(players);
				i = players[listpos];
				if ((i != Player_num) && (Players[i].connected))
				{
					kick_player(Players[i], Netgame.players[i]);
					return;
				}
			}
			else HUD_init_message_literal(HM_MULTI, "You cannot use # kicking with in team display.");


		    multi_message_index = 0;
		    multi_sending_message[Player_num] = msgsend_none;
#if defined(DXX_BUILD_DESCENT_II)
		    multi_send_msgsend_state(msgsend_none);
#endif
			return;
		}


		for (i = 0; i < N_players; i++)
			if (i != Player_num && Players[i].connected && !d_strnicmp(static_cast<const char *>(Players[i].callsign), &Network_message[name_index], strlen(Network_message.data()) - name_index))
			{
				kick_player(Players[i], Netgame.players[i]);
				return;
			}
	}
	
	else if (!d_stricmp (Network_message.data(), "/killreactor") && (Game_mode & GM_NETWORK) && !Control_center_destroyed)
	{
		if (!multi_i_am_master())
			HUD_init_message(HM_MULTI, "Only %s can kill the reactor this way!", static_cast<const char *>(Players[multi_who_is_master()].callsign));
		else
		{
			net_destroy_controlcen(object_none);
			multi_send_destroy_controlcen(object_none,Player_num);
		}
		multi_message_index = 0;
		multi_sending_message[Player_num] = msgsend_none;
#if defined(DXX_BUILD_DESCENT_II)
		multi_send_msgsend_state(msgsend_none);
#endif
		return;
	}

#if defined(DXX_BUILD_DESCENT_II)
	else
#endif
		HUD_init_message(HM_MULTI, "%s '%s'", TXT_SENDING, Network_message.data());

	multi_send_message();
	multi_message_feedback();

#if defined(DXX_BUILD_DESCENT_I)
	Network_message_reciever = 100;
#elif defined(DXX_BUILD_DESCENT_II)
	multi_message_index = 0;
	multi_sending_message[Player_num] = msgsend_none;
	multi_send_msgsend_state(msgsend_none);
	key_toggle_repeat(0);
#endif
	game_flush_inputs();
}

static void multi_define_macro_end()
{
	Assert( multi_defining_message > 0 );

	PlayerCfg.NetworkMessageMacro[multi_defining_message-1] = Network_message;
	write_player_file();

	multi_message_index = 0;
	multi_defining_message = 0;
	key_toggle_repeat(0);
	game_flush_inputs();
}

window_event_result multi_message_input_sub(int key)
{
	switch( key )
	{
		case KEY_F8:
		case KEY_ESC:
			multi_sending_message[Player_num] = msgsend_none;
			multi_send_msgsend_state(msgsend_none);
			multi_defining_message = 0;
			key_toggle_repeat(0);
			game_flush_inputs();
			return window_event_result::handled;
		case KEY_LEFT:
		case KEY_BACKSP:
		case KEY_PAD4:
			if (multi_message_index > 0)
				multi_message_index--;
			Network_message[multi_message_index] = 0;
			return window_event_result::handled;
		case KEY_ENTER:
			if ( multi_sending_message[Player_num] )
				multi_send_message_end();
			else if ( multi_defining_message )
				multi_define_macro_end();
			game_flush_inputs();
			return window_event_result::handled;
		default:
		{
			int ascii = key_ascii();
			if ( ascii < 255 )     {
				if (multi_message_index < MAX_MESSAGE_LEN-2 )   {
					Network_message[multi_message_index++] = ascii;
					Network_message[multi_message_index] = 0;
				} else if ( multi_sending_message[Player_num] )     {
					int i;
					char * ptext;
					ptext = NULL;
					Network_message[multi_message_index++] = ascii;
					Network_message[multi_message_index] = 0;
					for (i=multi_message_index-1; i>=0; i-- )       {
						if ( Network_message[i]==32 )   {
							ptext = &Network_message[i+1];
							Network_message[i] = 0;
							break;
						}
					}
					multi_send_message_end();
					if ( ptext )    {
						multi_sending_message[Player_num] = msgsend_typing;
						multi_send_msgsend_state(msgsend_typing);
						auto pcolon = strstr(Network_message.data(), ": " );
						if ( pcolon )
							strcpy( pcolon+1, ptext );
						else
							strcpy(Network_message.data(), ptext);
						multi_message_index = strlen( Network_message );
					}
				}
			}
		}
	}
	return window_event_result::ignored;
}

void multi_do_death(int)
{
	// Do any miscellaneous stuff for a new network player after death
	if (!(Game_mode & GM_MULTI_COOP))
	{
		Players[Player_num].flags |= (PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_GOLD_KEY);
	}
}

static void multi_do_fire(const playernum_t pnum, const ubyte *buf)
{
	ubyte weapon;
	sbyte flags;
	vms_vector shot_orientation;

	// Act out the actual shooting
	weapon = (int)buf[2];

	flags = buf[4];
	if (buf[0] == MULTI_FIRE_TRACK)
	{
		Network_laser_track = GET_INTEL_SHORT(buf + 18);
		Network_laser_track = objnum_remote_to_local(Network_laser_track, buf[20]);
	}

	shot_orientation.x = (fix) GET_INTEL_INT(buf + 6); 
	shot_orientation.y = (fix) GET_INTEL_INT(buf + 10); 
	shot_orientation.z = (fix) GET_INTEL_INT(buf + 14); 

	Assert (pnum < N_players);

	const auto obj = vobjptridx(Players[pnum].objnum);
	if (obj->type == OBJ_GHOST)
		multi_make_ghost_player(pnum);

	if (weapon == FLARE_ADJUST)
		Laser_player_fire( obj, FLARE_ID, 6, 1, shot_orientation);
	else
	if (weapon >= MISSILE_ADJUST) {
		int weapon_gun,remote_objnum;
		enum weapon_type_t weapon_id = (enum weapon_type_t) Secondary_weapon_to_weapon_info[weapon-MISSILE_ADJUST];
		weapon_gun = Secondary_weapon_to_gun_num[weapon-MISSILE_ADJUST] + (flags & 1);

#if defined(DXX_BUILD_DESCENT_II)
		if (weapon-MISSILE_ADJUST==GUIDED_INDEX)
		{
			Multi_is_guided=1;
		}
#endif

		auto objnum = Laser_player_fire( obj, weapon_id, weapon_gun, 1, shot_orientation );
		if (buf[0] == MULTI_FIRE_BOMB)
		{
			remote_objnum = GET_INTEL_SHORT(buf + 18);
			map_objnum_local_to_remote(objnum, remote_objnum, pnum);
		}
	}
	else {
		fix save_charge = Fusion_charge;
		if (weapon == FUSION_INDEX) {
			Fusion_charge = flags << 12;
		}
		if (weapon == LASER_ID) {
			if (flags & LASER_QUAD)
				Players[pnum].flags |= PLAYER_FLAGS_QUAD_LASERS;
			else
				Players[pnum].flags &= ~PLAYER_FLAGS_QUAD_LASERS;
		}

		do_laser_firing(vobjptridx(Players[pnum].objnum), weapon, (int)buf[3], flags, (int)buf[5], shot_orientation);

		if (weapon == FUSION_INDEX)
			Fusion_charge = save_charge;
	}
}

static void multi_do_message(const uint8_t *const cbuf)
{
	const auto buf = reinterpret_cast<const char *>(cbuf);
	const char *colon;
	const char *msgstart;
	int loc = 2;

	if (((colon = strstr(buf+loc, ": ")) == NULL) || (colon-(buf+loc) < 1) || (colon-(buf+loc) > CALLSIGN_LEN))
	{
		msgstart = buf + 2;
	}
	else
	{
		if ( (!d_strnicmp(static_cast<const char *>(Players[Player_num].callsign), buf+loc, colon-(buf+loc))) ||
			 ((Game_mode & GM_TEAM) && ( (get_team(Player_num) == atoi(buf+loc)-1) || !d_strnicmp(Netgame.team_name[get_team(Player_num)], buf+loc, colon-(buf+loc)))) )
		{
			msgstart = colon + 2;
		}
		else
			return;
	}
	const auto color = get_player_or_team_color(buf[1]);
	char xrgb = BM_XRGB(player_rgb[color].r,player_rgb[color].g,player_rgb[color].b);
	digi_play_sample(SOUND_HUD_MESSAGE, F1_0);
	HUD_init_message(HM_MULTI, "%c%c%s:%c%c %s", CC_COLOR, xrgb, static_cast<const char *>(Players[(int)buf[1]].callsign), CC_COLOR, BM_XRGB(0, 31, 0), msgstart);
	multi_sending_message[(int)buf[1]] = msgsend_none;
}

static void multi_do_position(const playernum_t pnum, const ubyte *buf)
{
	const auto obj = vobjptridx(Players[pnum].objnum);
	if (words_bigendian)
	{
		shortpos sp;
		memcpy(sp.bytemat, &buf[2], 9);
		memcpy(&sp.xo, &buf[11], 14);
		extract_shortpos_little(obj, &sp);
	}
	else
		extract_shortpos_little(obj, reinterpret_cast<const shortpos *>(&buf[2]));

	if (obj->movement_type == MT_PHYSICS)
		set_thrust_from_velocity(obj);
}

static void multi_do_reappear(const playernum_t pnum, const ubyte *buf)
{
	short objnum;

	objnum = GET_INTEL_SHORT(buf + 2);

	Assert(objnum >= 0);
	const auto obj = vobjptridx(objnum);
	if (pnum != get_player_id(obj))
		return;

	multi_make_ghost_player(get_player_id(obj));
	create_player_appearance_effect(obj);
}

static void multi_do_player_deres(const playernum_t pnum, const ubyte *buf)
{
	// Only call this for players, not robots.  pnum is player number, not
	// Object number.

	int count;
	char remote_created;

#ifdef NDEBUG
	if (pnum >= N_players)
		return;
#else
	Assert(pnum < N_players);
#endif

	// If we are in the process of sending objects to a new player, reset that process
	if (Network_send_objects)
	{
		Network_send_objnum = -1;
	}

	// Stuff the Players structure to prepare for the explosion

	count = 3;
#if defined(DXX_BUILD_DESCENT_I)
#define GET_WEAPON_FLAGS(buf,count)	buf[count++]
#elif defined(DXX_BUILD_DESCENT_II)
#define GET_WEAPON_FLAGS(buf,count)	(count += sizeof(uint16_t), GET_INTEL_SHORT(buf + (count - sizeof(uint16_t))))
#endif
	Players[pnum].primary_weapon_flags = GET_WEAPON_FLAGS(buf,count);
	Players[pnum].secondary_weapon_flags = GET_WEAPON_FLAGS(buf,count);
	Players[pnum].laser_level = stored_laser_level(buf[count]);                                                 count++;
	Players[pnum].secondary_ammo[HOMING_INDEX] = buf[count];                count++;
	Players[pnum].secondary_ammo[CONCUSSION_INDEX] = buf[count];count++;
	Players[pnum].secondary_ammo[SMART_INDEX] = buf[count];         count++;
	Players[pnum].secondary_ammo[MEGA_INDEX] = buf[count];          count++;
	Players[pnum].secondary_ammo[PROXIMITY_INDEX] = buf[count]; count++;

#if defined(DXX_BUILD_DESCENT_II)
	Players[pnum].secondary_ammo[SMISSILE1_INDEX] = buf[count]; count++;
	Players[pnum].secondary_ammo[GUIDED_INDEX]    = buf[count]; count++;
	Players[pnum].secondary_ammo[SMART_MINE_INDEX]= buf[count]; count++;
	Players[pnum].secondary_ammo[SMISSILE4_INDEX] = buf[count]; count++;
	Players[pnum].secondary_ammo[SMISSILE5_INDEX] = buf[count]; count++;
#endif

	Players[pnum].vulcan_ammo = GET_INTEL_SHORT(buf + count); count += 2;
	Players[pnum].flags = GET_INTEL_INT(buf + count);               count += 4;

	multi_powcap_adjust_remote_cap (pnum);

	const auto objp = vobjptridx(Players[pnum].objnum);

	//      objp->phys_info.velocity = *(vms_vector *)(buf+16); // 12 bytes
	//      objp->pos = *(vms_vector *)(buf+28);                // 12 bytes

	remote_created = buf[count++]; // How many did the other guy create?

	Net_create_loc = 0;
	drop_player_eggs(objp);

	// Create mapping from remote to local numbering system

	// We now handle this situation gracefully, Int3 not required
	//      if (Net_create_loc != remote_created)
	//              Int3(); // Probably out of object array space, see Rob

	range_for (const auto i, partial_range(Net_create_objnums, std::min(Net_create_loc, static_cast<unsigned>(remote_created))))
	{
		short s;

		s = GET_INTEL_SHORT(buf + count);

		if ((s > 0) && (i > 0))
			map_objnum_local_to_remote((short)i, s, pnum);
		count += 2;
	}
	range_for (const auto i, partial_range(Net_create_objnums, remote_created, Net_create_loc))
	{
		Objects[i].flags |= OF_SHOULD_BE_DEAD;
	}

	if (buf[2] == deres_explode)
	{
		explode_badass_player(objp);

		objp->flags &= ~OF_SHOULD_BE_DEAD;              //don't really kill player
		multi_make_player_ghost(pnum);
	}
	else
	{
		create_player_appearance_effect(objp);
	}

	Players[pnum].flags &= ~(PLAYER_FLAGS_CLOAKED | PLAYER_FLAGS_INVULNERABLE);
#if defined(DXX_BUILD_DESCENT_II)
	Players[pnum].flags &= ~PLAYER_FLAGS_FLAG;
#endif
	Players[pnum].cloak_time = 0;
}

/*
 * Process can compute a kill. If I am a Client this might be my own one (see multi_send_kill()) but with more specific data so I can compute my kill correctly.
 */
static void multi_do_kill(playernum_t, const ubyte *buf)
{
	int count = 1;
	int type = (int)(buf[0]);

	if (multi_i_am_master() && type != MULTI_KILL_CLIENT)
		return;
	if (!multi_i_am_master() && type != MULTI_KILL_HOST)
		return;

	const playernum_t pnum = buf[1];
	if (pnum >= N_players)
	{
		Int3(); // Invalid player number killed
		return;
	}

	// I am host, I know what's going on so take this packet, add game_mode related info which might be necessary for kill computation and send it to everyone so they can compute their kills correctly
	if (multi_i_am_master())
	{
		memcpy(multibuf, buf, 5);
		multibuf[5] = Netgame.team_vector;
		multibuf[6] = Bounty_target;
		
		multi_send_data<MULTI_KILL_HOST>(multibuf, 7, 2);
	}

	objnum_t killer, killed;
	killed = Players[pnum].objnum;
	count += 1;
	killer = GET_INTEL_SHORT(buf + count);
	if (killer > 0)
		killer = objnum_remote_to_local(killer, buf[count+2]);
	if (!multi_i_am_master())
	{
		Netgame.team_vector = buf[5];
		Bounty_target = buf[6];
	}

	multi_compute_kill(killer, killed);

	if (Game_mode & GM_BOUNTY && multi_i_am_master()) // update in case if needed... we could attach this to this packet but... meh...
		multi_send_bounty();
}


//      Changed by MK on 10/20/94 to send NULL as object to net_destroy_controlcen if it got -1
// which means not a controlcen object, but contained in another object
static void multi_do_controlcen_destroy(const ubyte *buf)
{
	sbyte who;
	objnum_t objnum = GET_INTEL_SHORT(buf + 1);
	who = buf[3];

	if (Control_center_destroyed != 1)
	{
		if ((who < N_players) && (who != Player_num)) {
			HUD_init_message(HM_MULTI, "%s %s", static_cast<const char *>(Players[who].callsign), TXT_HAS_DEST_CONTROL);
		}
		else if (who == Player_num)
			HUD_init_message_literal(HM_MULTI, TXT_YOU_DEST_CONTROL);
		else
			HUD_init_message_literal(HM_MULTI, TXT_CONTROL_DESTROYED);

		if (objnum != object_none)
			net_destroy_controlcen(objptridx(objnum));
		else
			net_destroy_controlcen(object_none);
	}
}

void
static multi_do_escape(const ubyte *buf)
{
	const auto objnum = vobjptridx(Players[buf[1]].objnum);
	digi_play_sample(SOUND_HUD_MESSAGE, F1_0);
#if defined(DXX_BUILD_DESCENT_II)
	digi_kill_sound_linked_to_object (objnum);
#endif

	if (buf[2] == 0)
	{
		HUD_init_message(HM_MULTI, "%s %s", static_cast<const char *>(Players[(int)buf[1]].callsign), TXT_HAS_ESCAPED);
		if (Game_mode & GM_NETWORK)
			Players[(int)buf[1]].connected = CONNECT_ESCAPE_TUNNEL;
		if (!multi_goto_secret)
			multi_goto_secret = 2;
	}
	else if (buf[2] == 1)
	{
		HUD_init_message(HM_MULTI, "%s %s", static_cast<const char *>(Players[(int)buf[1]].callsign), TXT_HAS_FOUND_SECRET);
		if (Game_mode & GM_NETWORK)
			Players[(int)buf[1]].connected = CONNECT_FOUND_SECRET;
		if (!multi_goto_secret)
			multi_goto_secret = 1;
	}
	create_player_appearance_effect(objnum);
	multi_make_player_ghost(buf[1]);
}

void
static multi_do_remobj(const ubyte *buf)
{
	short objnum; // which object to remove

	objnum = GET_INTEL_SHORT(buf + 1);
	// which remote list is it entered in
	auto obj_owner = buf[3];

	Assert(objnum >= 0);

	if (objnum < 1)
		return;

	auto local_objnum = objnum_remote_to_local(objnum, obj_owner); // translate to local objnum

	if (local_objnum == object_none)
	{
		return;
	}

	const auto &obj = vobjptr(local_objnum);
	if ((Objects[local_objnum].type != OBJ_POWERUP) && (Objects[local_objnum].type != OBJ_HOSTAGE))
	{
		return;
	}

	if (Network_send_objects && multi_objnum_is_past(local_objnum))
	{
		Network_send_objnum = -1;
	}
	if (Objects[local_objnum].type==OBJ_POWERUP)
		if (Game_mode & GM_NETWORK)
		{
			PowerupCaps.dec_powerup_current(get_powerup_id(obj));
		}

	Objects[local_objnum].flags |= OF_SHOULD_BE_DEAD; // quick and painless

}

void multi_disconnect_player(const playernum_t pnum)
{
	int n = 0;

	if (!(Game_mode & GM_NETWORK))
		return;
	if (Players[pnum].connected == CONNECT_DISCONNECTED)
		return;

	if (Players[pnum].connected == CONNECT_PLAYING)
	{
		digi_play_sample( SOUND_HUD_MESSAGE, F1_0 );
		HUD_init_message(HM_MULTI,  "%s %s", static_cast<const char *>(Players[pnum].callsign), TXT_HAS_LEFT_THE_GAME);

		multi_sending_message[pnum] = msgsend_none;

		if (Network_status == NETSTAT_PLAYING)
		{
			multi_make_player_ghost(pnum);
			multi_strip_robots(pnum);
		}

		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_multi_disconnect(pnum);

		// Bounty target left - select a new one
		if( Game_mode & GM_BOUNTY && pnum == Bounty_target && multi_i_am_master() )
		{
			/* Select a random number */
			int n = d_rand() % MAX_PLAYERS;
			
			/* Make sure they're valid: Don't check against kill flags,
				* just in case everyone's dead! */
			while( !Players[n].connected )
				n = d_rand() % MAX_PLAYERS;
			
			/* Select new target */
			multi_new_bounty_target( n );
			
			/* Send this new data */
			multi_send_bounty();
		}
	}

	Players[pnum].connected = CONNECT_DISCONNECTED;
	Netgame.players[pnum].connected = CONNECT_DISCONNECTED;

	switch (multi_protocol)
	{
#ifdef USE_UDP
		case MULTI_PROTO_UDP:
			net_udp_disconnect_player(pnum);
			break;
#endif
		default:
			Error("Protocol handling missing in multi_disconnect_player\n");
			break;
	}

	if (pnum == multi_who_is_master()) // Host has left - Quit game!
	{
		if (Network_status==NETSTAT_PLAYING)
			multi_leave_game();
		if (Game_wind)
			window_set_visible(Game_wind, 0);
		nm_messagebox(NULL, 1, TXT_OK, "Host left the game!");
		if (Game_wind)
			window_set_visible(Game_wind, 1);
		multi_quit_game = 1;
		game_leave_menus();
		multi_reset_stuff();
		return;
	}

	range_for (auto &i, partial_range(Players, N_players))
		if (i.connected)
			if (++n > 1)
				break;
	if (n == 1)
	{
		HUD_init_message_literal(HM_MULTI, "You are the only person remaining in this netgame");
	}
}

void
static multi_do_quit(const ubyte *buf)
{

	if (!(Game_mode & GM_NETWORK))
		return;
	multi_disconnect_player((int)buf[1]);
}

static void multi_do_cloak(const playernum_t pnum)
{
	Assert(pnum < N_players);

	Players[pnum].flags |= PLAYER_FLAGS_CLOAKED;
	Players[pnum].cloak_time = GameTime64;
	ai_do_cloak_stuff();

	multi_strip_robots(pnum);

	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_multi_cloak(pnum);
}

static void multi_do_decloak(const playernum_t pnum)
{
	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_multi_decloak(pnum);

}

void
static multi_do_door_open(const ubyte *buf)
{
	segnum_t segnum;
	ubyte side;
	wall *w;

	segnum = GET_INTEL_SHORT(buf + 1);
	side = buf[3];
#if defined(DXX_BUILD_DESCENT_II)
	ubyte flag= buf[4];
#endif

	if (side > 5)
	{
		Int3();
		return;
	}

	const auto &&seg = vsegptridx(segnum);

	if (seg->sides[side].wall_num == wall_none) {  //Opening door on illegal wall
		Int3();
		return;
	}

	w = &Walls[seg->sides[side].wall_num];

	if (w->type == WALL_BLASTABLE)
	{
		if (!(w->flags & WALL_BLASTED))
		{
			wall_destroy(seg, side);
		}
		return;
	}
	else if (w->state != WALL_DOOR_OPENING)
	{
		wall_open_door(seg, side);
	}
#if defined(DXX_BUILD_DESCENT_II)
	w->flags=flag;
#endif

}

static void multi_do_create_explosion(const playernum_t pnum)
{
	create_small_fireball_on_object(vobjptridx(Players[pnum].objnum), F1_0, 1);
}

void
static multi_do_controlcen_fire(const ubyte *buf)
{
	vms_vector to_target;
	int gun_num;
	objnum_t objnum;
	int count = 1;

	memcpy(&to_target, buf+count, 12);          count += 12;
	if (words_bigendian)// swap the vector to_target
	{
	to_target.x = (fix)INTEL_INT((int)to_target.x);
	to_target.y = (fix)INTEL_INT((int)to_target.y);
	to_target.z = (fix)INTEL_INT((int)to_target.z);
	}
	gun_num = buf[count];                       count += 1;
	objnum = GET_INTEL_SHORT(buf + count);      count += 2;

	auto objp = vobjptridx(objnum);
	Laser_create_new_easy(to_target, objp->ctype.reactor_info.gun_pos[gun_num], objp, CONTROLCEN_WEAPON_NUM, 1);
}

static void multi_do_create_powerup(const playernum_t pnum, const ubyte *buf)
{
	int count = 1;
	vms_vector new_pos;
	char powerup_type;

	if ((Network_status == NETSTAT_ENDLEVEL) || Control_center_destroyed)
		return;

	count++;
	powerup_type = buf[count++];
	const auto &&segnum = vsegptridx(GET_INTEL_SHORT(&buf[count]));
	count += 2;
	objnum_t objnum = GET_INTEL_SHORT(buf + count); count += 2;
	memcpy(&new_pos, buf+count, sizeof(vms_vector)); count+=sizeof(vms_vector);
	if (words_bigendian)
	{
	new_pos.x = (fix)SWAPINT((int)new_pos.x);
	new_pos.y = (fix)SWAPINT((int)new_pos.y);
	new_pos.z = (fix)SWAPINT((int)new_pos.z);
	}

	Net_create_loc = 0;
	auto my_objnum = call_object_create_egg(&Objects[Players[pnum].objnum], 1, OBJ_POWERUP, powerup_type);

	if (my_objnum == object_none) {
		return;
	}

	if (Network_send_objects && multi_objnum_is_past(my_objnum))
	{
		Network_send_objnum = -1;
	}

	my_objnum->pos = new_pos;

	vm_vec_zero(my_objnum->mtype.phys_info.velocity);

	obj_relink(my_objnum, segnum);

	map_objnum_local_to_remote(my_objnum, objnum, pnum);

	object_create_explosion(segnum, new_pos, i2f(5), VCLIP_POWERUP_DISAPPEARANCE);

	if (Game_mode & GM_NETWORK)
	{
		PowerupCaps.inc_powerup_current(get_powerup_id(my_objnum));
	}
}

static void multi_do_play_sound(const playernum_t pnum, const ubyte *buf)
{
	int sound_num = buf[2];
	fix volume = buf[3] << 12;

	if (!Players[pnum].connected)
		return;

	Assert(Players[pnum].objnum <= Highest_object_index);
	digi_link_sound_to_object( sound_num, vcobjptridx(Players[pnum].objnum), 0, volume);
}

static void multi_do_score(const playernum_t pnum, const ubyte *buf)
{
	if (pnum >= N_players)
	{
		Int3(); // Non-terminal, see rob
		return;
	}

	if (Newdemo_state == ND_STATE_RECORDING) {
		int score;
		score = GET_INTEL_INT(buf + 2);
		newdemo_record_multi_score(pnum, score);
	}
	Players[pnum].score = GET_INTEL_INT(buf + 2);
	multi_sort_kill_list();
}

static void multi_do_trigger(const playernum_t pnum, const ubyte *buf)
{
	int trigger = buf[2];
	if (pnum >= N_players || pnum == Player_num)
	{
		Int3(); // Got trigger from illegal playernum
		return;
	}
	if ((trigger < 0) || (trigger >= Num_triggers))
	{
		Int3(); // Illegal trigger number in multiplayer
		return;
	}
	check_trigger_sub(trigger, pnum,0);
}

#if defined(DXX_BUILD_DESCENT_II)
static void multi_do_effect_blowup(const playernum_t pnum, const ubyte *buf)
{
	int side;
	vms_vector hitpnt;

	if (pnum >= N_players || pnum == Player_num)
		return;

	multi_do_protocol_frame(1, 0); // force packets to be sent, ensuring this packet will be attached to following MULTI_TRIGGER

	segnum_t segnum = GET_INTEL_SHORT(buf + 2); 
	side = buf[4];
	hitpnt.x = GET_INTEL_INT(buf + 5);
	hitpnt.y = GET_INTEL_INT(buf + 9);
	hitpnt.z = GET_INTEL_INT(buf + 13);

	//create a dummy object which will be the weapon that hits
	//the monitor. the blowup code wants to know who the parent of the
	//laser is, so create a laser whose parent is the player
	laser_info laser;
	laser.parent_type = OBJ_PLAYER;
	laser.parent_num = pnum;

	check_effect_blowup(vsegptridx(segnum), side, hitpnt, laser, 0, 1);
}

static void multi_do_drop_marker (const playernum_t pnum, const ubyte *buf)
{
	int i;
	int mesnum=buf[2];
	vms_vector position;

	if (pnum==Player_num)  // my marker? don't set it down cuz it might screw up the orientation
		return;

	position.x = GET_INTEL_INT(buf + 3);
	position.y = GET_INTEL_INT(buf + 7);
	position.z = GET_INTEL_INT(buf + 11);

	for (i=0;i<40;i++)
		MarkerMessage[(pnum*2)+mesnum][i]=buf[15+i];

	if (MarkerObject[(pnum*2)+mesnum] !=object_none && Objects[MarkerObject[(pnum*2)+mesnum]].type!=OBJ_NONE && MarkerObject[(pnum*2)+mesnum] !=0)
		obj_delete(MarkerObject[(pnum*2)+mesnum]);

	MarkerObject[(pnum*2)+mesnum] = drop_marker_object(position,Objects[Players[pnum].objnum].segnum,Objects[Players[pnum].objnum].orient,(pnum*2)+mesnum);
}
#endif

static void multi_do_hostage_door_status(const ubyte *buf)
{
	// Update hit point status of a door

	int count = 1;
	int wallnum;
	fix hps;

	wallnum = GET_INTEL_SHORT(buf + count);     count += 2;
	hps = GET_INTEL_INT(buf + count);           count += 4;

	if ((wallnum < 0) || (wallnum > Num_walls) || (hps < 0) || (Walls[wallnum].type != WALL_BLASTABLE))
	{
		Int3(); // Non-terminal, see Rob
		return;
	}

	if (hps < Walls[wallnum].hps)
		wall_damage(&Segments[Walls[wallnum].segnum], Walls[wallnum].sidenum, Walls[wallnum].hps - hps);
}

void
multi_reset_stuff(void)
{
	// A generic, emergency function to solve problems that crop up
	// when a player exits quick-out from the game because of a
	// connection loss.  Fixes several weird bugs!

	dead_player_end();
	Players[Player_num].homing_object_dist = -F1_0; // Turn off homing sound.
	reset_rear_view();
}

void multi_reset_player_object(const vobjptridx_t objp)
{
	//Init physics for a non-console player
	Assert((objp->type == OBJ_PLAYER) || (objp->type == OBJ_GHOST));

	vm_vec_zero(objp->mtype.phys_info.velocity);
	vm_vec_zero(objp->mtype.phys_info.thrust);
	vm_vec_zero(objp->mtype.phys_info.rotvel);
	vm_vec_zero(objp->mtype.phys_info.rotthrust);
	objp->mtype.phys_info.turnroll = 0;
	objp->mtype.phys_info.mass = Player_ship->mass;
	objp->mtype.phys_info.drag = Player_ship->drag;
	if (objp->type == OBJ_PLAYER)
		objp->mtype.phys_info.flags |= PF_TURNROLL | PF_WIGGLE;
	else
		objp->mtype.phys_info.flags &= ~(PF_TURNROLL | PF_LEVELLING | PF_WIGGLE);

	//Init render info

	objp->render_type = RT_POLYOBJ;
	objp->rtype.pobj_info.model_num = Player_ship->model_num;               //what model is this?
	objp->rtype.pobj_info.subobj_flags = 0;         //zero the flags
	objp->rtype.pobj_info.anim_angles = {};

	// Clear misc

	objp->flags = 0;

	if (objp->type == OBJ_GHOST)
		objp->render_type = RT_NONE;
	//reset textures for this, if not player 0
	multi_reset_object_texture (objp);
}

void multi_reset_object_texture (const vobjptr_t objp)
{
	int id,i;

	if (Game_mode & GM_TEAM)
		id = get_team(get_player_id(objp));
	else
		id = get_player_id(objp);

	if (id == 0)
		objp->rtype.pobj_info.alt_textures=0;
	else {
		if (N_PLAYER_SHIP_TEXTURES < Polygon_models[objp->rtype.pobj_info.model_num].n_textures)
			Error("Too many player ship textures!\n");

		for (i=0;i<Polygon_models[objp->rtype.pobj_info.model_num].n_textures;i++)
			multi_player_textures[id-1][i] = ObjBitmaps[ObjBitmapPtrs[Polygon_models[objp->rtype.pobj_info.model_num].first_texture+i]];

		multi_player_textures[id-1][4] = ObjBitmaps[ObjBitmapPtrs[First_multi_bitmap_num+(id-1)*2]];
		multi_player_textures[id-1][5] = ObjBitmaps[ObjBitmapPtrs[First_multi_bitmap_num+(id-1)*2+1]];

		objp->rtype.pobj_info.alt_textures = id;
	}
}

void multi_process_bigdata(const playernum_t pnum, const ubyte *buf, uint_fast32_t len)
{
	// Takes a bunch of messages, check them for validity,
	// and pass them to multi_process_data.

	uint_fast32_t bytes_processed = 0;

	while( bytes_processed < len )  {
		uint_fast32_t type = buf[bytes_processed];

		if ( (type>= sizeof(message_length)/sizeof(message_length[0])))
		{
			con_printf( CON_DEBUG,"multi_process_bigdata: Invalid packet type %lu!", static_cast<unsigned long>(type));
			return;
		}
		uint_fast32_t sub_len = message_length[type];

		Assert(sub_len > 0);

		if ( (bytes_processed+sub_len) > len )  {
			con_printf(CON_DEBUG, "multi_process_bigdata: packet type %u too short (%lu>%lu)!", static_cast<unsigned>(type), static_cast<unsigned long>(bytes_processed+sub_len), static_cast<unsigned long>(len));
			Int3();
			return;
		}

		multi_process_data(pnum, &buf[bytes_processed], type);
		bytes_processed += sub_len;
	}
}

//
// Part 2 : Functions that send communication messages to inform the other
//          players of something we did.
//

void multi_send_fire(int laser_gun, int laser_level, int laser_flags, int laser_fired, objnum_t laser_track, const objptridx_t is_bomb_objnum)
{
	object* ownship = &Objects[Players[Player_num].objnum];
	static fix64 last_fireup_time = 0;

	// provoke positional update if possible (20 times per second max. matches vulcan, the fastest firing weapon)
	if (timer_query() >= (last_fireup_time+(F1_0/20)))
	{
		multi_do_protocol_frame(1, 0);
		last_fireup_time = timer_query();
	}

	multibuf[0] = (char)MULTI_FIRE;
	if (is_bomb_objnum != object_none)
	{
		if (is_proximity_bomb_or_smart_mine(get_weapon_id(is_bomb_objnum)))
			multibuf[0] = (char)MULTI_FIRE_BOMB;
	}
	else if (laser_track != object_none)
	{
		multibuf[0] = (char)MULTI_FIRE_TRACK;
	}
	multibuf[1] = (char)Player_num;
	multibuf[2] = (char)laser_gun;
	multibuf[3] = (char)laser_level;
	multibuf[4] = (char)laser_flags;
	multibuf[5] = (char)laser_fired;

	PUT_INTEL_INT(multibuf+6 , ownship->orient.fvec.x);
	PUT_INTEL_INT(multibuf+10, ownship->orient.fvec.y);
	PUT_INTEL_INT(multibuf+14, ownship->orient.fvec.z);

	/*
	 * If we fire a bomb, it's persistent. Let others know of it's objnum so host can track it's behaviour over clients (host-authority functions, D2 chaff ability).
	 * If we fire a tracking projectile, we should others let know abotu what we track but we have to pay attention it's mapped correctly.
	 * If we fire something else, we make the packet as small as possible.
	 */
	if (multibuf[0] == MULTI_FIRE_BOMB)
	{
		map_objnum_local_to_local(is_bomb_objnum);
		PUT_INTEL_SHORT(&multibuf[18], static_cast<objnum_t>(is_bomb_objnum));
		multi_send_data<MULTI_FIRE_BOMB>(multibuf, 20, 1);
	}
	else if (multibuf[0] == MULTI_FIRE_TRACK)
	{
		sbyte remote_owner;
		short remote_laser_track = -1;

		remote_laser_track = objnum_local_to_remote(laser_track, &remote_owner);
		PUT_INTEL_SHORT(multibuf+18, remote_laser_track);
		multibuf[20] = remote_owner;
		multi_send_data<MULTI_FIRE_TRACK>(multibuf, 21, 1);
	}
	else
		multi_send_data<MULTI_FIRE>(multibuf, 18, 1);
}

void multi_send_destroy_controlcen(objnum_t objnum, int player)
{
	if (player == Player_num)
		HUD_init_message_literal(HM_MULTI, TXT_YOU_DEST_CONTROL);
	else if ((player > 0) && (player < N_players))
		HUD_init_message(HM_MULTI, "%s %s", static_cast<const char *>(Players[player].callsign), TXT_HAS_DEST_CONTROL);
	else
		HUD_init_message_literal(HM_MULTI, TXT_CONTROL_DESTROYED);

	PUT_INTEL_SHORT(multibuf+1, objnum);
	multibuf[3] = player;
	multi_send_data<MULTI_CONTROLCEN>(multibuf, 4, 2);
}

#if defined(DXX_BUILD_DESCENT_II)
void multi_send_drop_marker (int player,const vms_vector &position,char messagenum,const marker_message_text_t &text)
{
	if (player<N_players)
	{
		multibuf[1]=(char)player;
		multibuf[2]=messagenum;
		PUT_INTEL_INT(multibuf+3, position.x);
		PUT_INTEL_INT(multibuf+7, position.y);
		PUT_INTEL_INT(multibuf+11, position.z);
		for (unsigned i = 0; i < text.size(); i++)
			multibuf[15+i]=text[i];
		multi_send_data<MULTI_MARKER>(multibuf, 15 + text.size(), 2);
	}
}

void multi_send_markers()
{
	// send marker positions/text to new player
	int i;

	for (i = 0; i < N_players; i++)
	{
		objnum_t mo;
		mo = MarkerObject[(i*2)];
		if (mo!=object_none)
			multi_send_drop_marker (i,Objects[mo].pos,0,MarkerMessage[i*2]);
		mo = MarkerObject[(i*2)+1];
		if (mo!=object_none)
			multi_send_drop_marker (i,Objects[mo].pos,1,MarkerMessage[(i*2)+1]);
	}
}
#endif

void
multi_send_endlevel_start(int secret)
{
	multibuf[1] = Player_num;
	multibuf[2] = (char)secret;

	if ((secret) && !multi_goto_secret)
		multi_goto_secret = 1;
	else if (!multi_goto_secret)
		multi_goto_secret = 2;

	multi_send_data<MULTI_ENDLEVEL_START>(multibuf, 3, 2);
	if (Game_mode & GM_NETWORK)
	{
		Players[Player_num].connected = CONNECT_ESCAPE_TUNNEL;
		switch (multi_protocol)
		{
#ifdef USE_UDP
			case MULTI_PROTO_UDP:
				net_udp_send_endlevel_packet();
				break;
#endif
			default:
				Error("Protocol handling missing in multi_send_endlevel_start\n");
				break;
		}
	}
}

void multi_send_player_deres(deres_type_t type)
{
	int count = 0;
	if (Network_send_objects)
	{
		Network_send_objnum = -1;
	}

	multi_send_position(Players[Player_num].objnum);

	count++;
	multibuf[count++] = Player_num;
	multibuf[count++] = type;

#if defined(DXX_BUILD_DESCENT_I)
#define PUT_WEAPON_FLAGS(buf,count,value)	(buf[count] = value, ++count)
#elif defined(DXX_BUILD_DESCENT_II)
#define PUT_WEAPON_FLAGS(buf,count,value)	((PUT_INTEL_SHORT(buf+count, value)), count+=sizeof(uint16_t))
#endif
	PUT_WEAPON_FLAGS(multibuf, count, Players[Player_num].primary_weapon_flags);
	PUT_WEAPON_FLAGS(multibuf, count, Players[Player_num].secondary_weapon_flags);
	multibuf[count++] = (char)Players[Player_num].laser_level;

	multibuf[count++] = (char)Players[Player_num].secondary_ammo[HOMING_INDEX];
	multibuf[count++] = (char)Players[Player_num].secondary_ammo[CONCUSSION_INDEX];
	multibuf[count++] = (char)Players[Player_num].secondary_ammo[SMART_INDEX];
	multibuf[count++] = (char)Players[Player_num].secondary_ammo[MEGA_INDEX];
	multibuf[count++] = (char)Players[Player_num].secondary_ammo[PROXIMITY_INDEX];

#if defined(DXX_BUILD_DESCENT_II)
	multibuf[count++] = (char)Players[Player_num].secondary_ammo[SMISSILE1_INDEX];
	multibuf[count++] = (char)Players[Player_num].secondary_ammo[GUIDED_INDEX];
	multibuf[count++] = (char)Players[Player_num].secondary_ammo[SMART_MINE_INDEX];
	multibuf[count++] = (char)Players[Player_num].secondary_ammo[SMISSILE4_INDEX];
	multibuf[count++] = (char)Players[Player_num].secondary_ammo[SMISSILE5_INDEX];
#endif

	PUT_INTEL_SHORT(multibuf+count, Players[Player_num].vulcan_ammo );
	count += 2;
	PUT_INTEL_INT(multibuf+count, Players[Player_num].flags );
	count += 4;

	multibuf[count++] = Net_create_loc;

	Assert(Net_create_loc <= MAX_NET_CREATE_OBJECTS);

	memset(multibuf+count, -1, MAX_NET_CREATE_OBJECTS*sizeof(short));

	range_for (const auto i, partial_range(Net_create_objnums, Net_create_loc))
	{
		if (i <= 0) {
			Int3(); // Illegal value in created egg object numbers
			count +=2;
			continue;
		}

		PUT_INTEL_SHORT(multibuf+count, i); count += 2;

		// We created these objs so our local number = the network number
		map_objnum_local_to_local(i);
	}

	Net_create_loc = 0;

	if (count > message_length[MULTI_PLAYER_DERES])
	{
		Int3(); // See Rob
	}

	multi_send_data<MULTI_PLAYER_DERES>(multibuf, command_length<MULTI_PLAYER_DERES>::value, 2);
	if (Players[Player_num].flags & PLAYER_FLAGS_CLOAKED)
		multi_send_decloak();
	multi_strip_robots(Player_num);
}

/*
 * Powerup capping: Keep track of how many powerups are in level and kill these which would exceed initial limit.
 * NOTE: code encapsuled by OLDPOWCAP define is original and buggy Descent2 code. 
 */

// Count the initial amount of Powerups in the level
void multi_powcap_count_powerups_in_mine(void)
{
	PowerupCaps.recount();
}

void powerup_cap_state::recount()
{
	m_powerups = {};
	range_for (const auto i, highest_valid(Objects))
	{
		const auto &obj = vobjptr(static_cast<objnum_t>(i));
		if (obj->type == OBJ_POWERUP)
		{
			inc_powerup_current(get_powerup_id(obj));
		}
	}
}

// We want to drop something. Kill every Powerup which exceeds the level limit
void multi_powcap_cap_objects()
{
	int index;

	if (!(Game_mode & GM_NETWORK) || (Game_mode & GM_MULTI_COOP))
		return;

	auto &plr = Players[Player_num];
	if (!game_mode_hoard())
	  	Players[Player_num].secondary_ammo[PROXIMITY_INDEX]+=Proximity_dropped;
	Proximity_dropped=0;
#if defined(DXX_BUILD_DESCENT_II)
	Players[Player_num].secondary_ammo[SMART_MINE_INDEX]+=Smartmines_dropped;
	Smartmines_dropped=0;
#endif

	for (index=0;index<MAX_PRIMARY_WEAPONS;index++)
	{
		const auto type = Primary_weapon_to_powerup[index];
			if(Players[Player_num].primary_weapon_flags & HAS_PRIMARY_FLAG(index))
				if (!PowerupCaps.can_add_powerup(type))
			{
				con_printf(CON_VERBOSE, "Removing primary %u due to powerup cap: current=%u max=%u", index, PowerupCaps.get_current(type), PowerupCaps.get_max(type));
				Players[Player_num].primary_weapon_flags&=(~HAS_PRIMARY_FLAG(index));
			}
	}


	// Don't do the adjustment stuff for Hoard mode
	if (!game_mode_hoard())
		Players[Player_num].secondary_ammo[2]/=4;

#if defined(DXX_BUILD_DESCENT_II)
	Players[Player_num].secondary_ammo[7]/=4;
#endif

	for (index=0;index<MAX_SECONDARY_WEAPONS;index++)
	{
		if (game_mode_hoard() && index==PROXIMITY_INDEX)
			continue;

		const auto type = Secondary_weapon_to_powerup[index];

		PowerupCaps.cap_secondary_ammo(type, plr.secondary_ammo[index]);
	}

	if (!game_mode_hoard())
		Players[Player_num].secondary_ammo[2]*=4;
#if defined(DXX_BUILD_DESCENT_II)
	Players[Player_num].secondary_ammo[7]*=4;
#endif
	PowerupCaps.cap_laser_level(plr.laser_level);
	PowerupCaps.cap_flag(plr.flags, PLAYER_FLAGS_QUAD_LASERS, POW_QUAD_FIRE);
	PowerupCaps.cap_flag(plr.flags, PLAYER_FLAGS_CLOAKED, POW_CLOAK);
#if defined(DXX_BUILD_DESCENT_II)
	PowerupCaps.cap_flag(plr.flags, PLAYER_FLAGS_MAP_ALL, POW_FULL_MAP);
	PowerupCaps.cap_flag(plr.flags, PLAYER_FLAGS_AFTERBURNER, POW_AFTERBURNER);
	PowerupCaps.cap_flag(plr.flags, PLAYER_FLAGS_AMMO_RACK, POW_AMMO_RACK);
	PowerupCaps.cap_flag(plr.flags, PLAYER_FLAGS_CONVERTER, POW_CONVERTER);
	PowerupCaps.cap_flag(plr.flags, PLAYER_FLAGS_HEADLIGHT, POW_HEADLIGHT);
	if (game_mode_capture_flag())
	{
		PowerupCaps.cap_flag(plr.flags, PLAYER_FLAGS_FLAG, get_team(Player_num) == TEAM_RED ? POW_FLAG_BLUE : POW_FLAG_RED);
	}
#endif

}

// Adds players inventory to multi cap
static void multi_powcap_adjust_cap_for_player(const playernum_t pnum)
{
	int index;

	if (!(Game_mode & GM_NETWORK) || (Game_mode & GM_MULTI_COOP))
		return;

	auto &plr = Players[pnum];
	for (index=0;index<MAX_PRIMARY_WEAPONS;index++)
	{
		const auto type = Primary_weapon_to_powerup[index];
		if (Players[pnum].primary_weapon_flags & HAS_PRIMARY_FLAG(index))
			PowerupCaps.inc_mapped_powerup_max(type);
	}

	for (index=0;index<MAX_SECONDARY_WEAPONS;index++)
	{
		const auto type = Secondary_weapon_to_powerup[index];
		PowerupCaps.add_mapped_powerup_max(type, Players[pnum].secondary_ammo[index]);
	}

	PowerupCaps.inc_flag_max(plr.flags, PLAYER_FLAGS_QUAD_LASERS, POW_QUAD_FIRE);
	PowerupCaps.inc_flag_max(plr.flags, PLAYER_FLAGS_CLOAKED, POW_CLOAK);
#if defined(DXX_BUILD_DESCENT_II)
	PowerupCaps.inc_flag_max(plr.flags, PLAYER_FLAGS_MAP_ALL, POW_FULL_MAP);
	PowerupCaps.inc_flag_max(plr.flags, PLAYER_FLAGS_AFTERBURNER, POW_AFTERBURNER);
	PowerupCaps.inc_flag_max(plr.flags, PLAYER_FLAGS_AMMO_RACK, POW_AMMO_RACK);
	PowerupCaps.inc_flag_max(plr.flags, PLAYER_FLAGS_CONVERTER, POW_CONVERTER);
	PowerupCaps.inc_flag_max(plr.flags, PLAYER_FLAGS_HEADLIGHT, POW_HEADLIGHT);
	if (plr.laser_level > MAX_LASER_LEVEL)
		PowerupCaps.add_mapped_powerup_max(POW_SUPER_LASER, plr.laser_level - MAX_LASER_LEVEL);
	else
#endif
		PowerupCaps.add_mapped_powerup_max(POW_LASER, plr.laser_level);
}

void multi_powcap_adjust_remote_cap(const playernum_t pnum)
{
	int index;

	if (!(Game_mode & GM_NETWORK) || (Game_mode & GM_MULTI_COOP))
		return;

	const auto &plr = Players[pnum];
	for (index=0;index<MAX_PRIMARY_WEAPONS;index++)
	{
		const auto type = Primary_weapon_to_powerup[index];
		if (Players[pnum].primary_weapon_flags & HAS_PRIMARY_FLAG(index))
			PowerupCaps.inc_mapped_powerup_current(type);
	}

	for (index=0;index<MAX_SECONDARY_WEAPONS;index++)
	{
		const auto type = Secondary_weapon_to_powerup[index];

		if (game_mode_hoard() && index==2)
			continue;

		const auto is_always_4pack = (index == secondary_weapon_index_t::PROXIMITY_INDEX // PROX? Those bastards...
#if defined(DXX_BUILD_DESCENT_II)
			|| index == secondary_weapon_index_t::SMART_MINE_INDEX // PROX or SMARTMINES? Those bastards...
#endif
		);
		const auto secondary_ammo = plr.secondary_ammo[index];
		PowerupCaps.add_mapped_powerup_current(type, is_always_4pack ? secondary_ammo / 4 : secondary_ammo);
	}

	const auto player_flags = plr.flags;
	PowerupCaps.inc_flag_current(player_flags, PLAYER_FLAGS_QUAD_LASERS, POW_QUAD_FIRE);
	PowerupCaps.inc_flag_current(player_flags, PLAYER_FLAGS_CLOAKED, POW_CLOAK);
#if defined(DXX_BUILD_DESCENT_II)
	PowerupCaps.inc_flag_current(player_flags, PLAYER_FLAGS_MAP_ALL, POW_FULL_MAP);
	PowerupCaps.inc_flag_current(player_flags, PLAYER_FLAGS_AFTERBURNER, POW_AFTERBURNER);
	PowerupCaps.inc_flag_current(player_flags, PLAYER_FLAGS_AMMO_RACK, POW_AMMO_RACK);
	PowerupCaps.inc_flag_current(player_flags, PLAYER_FLAGS_CONVERTER, POW_CONVERTER);
	PowerupCaps.inc_flag_current(player_flags, PLAYER_FLAGS_HEADLIGHT, POW_HEADLIGHT);
	if (plr.laser_level > MAX_LASER_LEVEL)
		PowerupCaps.add_mapped_powerup_current(POW_SUPER_LASER, plr.laser_level - MAX_LASER_LEVEL);
	else
#endif
		PowerupCaps.add_mapped_powerup_current(POW_LASER, plr.laser_level);
}

void
multi_send_message(void)
{
	int loc = 0;
	if (Network_message_reciever != -1)
	{
		loc += 1;
		multibuf[loc] = (char)Player_num;                       loc += 1;
		strncpy((char *)(multibuf+loc), Network_message, MAX_MESSAGE_LEN); loc += MAX_MESSAGE_LEN;
		multibuf[loc-1] = '\0';
		multi_send_data<MULTI_MESSAGE>(multibuf, loc, 0);
		Network_message_reciever = -1;
	}
}

void
multi_send_reappear()
{
	multi_send_position(Players[Player_num].objnum);
	multibuf[1] = (char)Player_num;
	PUT_INTEL_SHORT(multibuf+2, Players[Player_num].objnum);

	multi_send_data<MULTI_REAPPEAR>(multibuf, 4, 2);
}

void multi_send_position(const vobjptridx_t obj)
{
	int count=0;

	count++;
	multibuf[count++] = (char)Player_num;
	if (words_bigendian)
	{
		shortpos sp;
		create_shortpos_little(&sp, obj);
		memcpy(&multibuf[count], sp.bytemat, 9);
		count += 9;
		memcpy(&multibuf[count], &sp.xo, 14);
		count += 14;
	}
	else
	{
		create_shortpos_little(reinterpret_cast<shortpos *>(&multibuf[count]), obj);
		count += sizeof(shortpos);
	}
	// send twice while first has priority so the next one will be attached to the next bigdata packet
	multi_send_data<MULTI_POSITION>(multibuf, count, 2);
	multi_send_data<MULTI_POSITION>(multibuf, count, 0);
}

/* 
 * I was killed. If I am host, send this info to everyone and compute kill. If I am just a Client I'll only send the kill but not compute it for me. I (Client) will wait for Host to send me my kill back together with updated game_mode related variables which are important for me to compute consistent kill.
 */
void multi_send_kill(const vobjptridx_t objnum)
{
	// I died, tell the world.

	int count = 0;

	Assert(get_player_id(objnum) == Player_num);
	auto killer_objnum = Players[Player_num].killer_objnum;

							count += 1;
	multibuf[count] = Player_num;			count += 1;

	if (killer_objnum != object_none)
	{
		short s = (short)objnum_local_to_remote(killer_objnum, (sbyte *)&multibuf[count+2]); // do it with variable since INTEL_SHORT won't work on return val from function.
		PUT_INTEL_SHORT(multibuf+count, s);
	}
	else
	{
		PUT_INTEL_SHORT(multibuf+count, static_cast<int16_t>(-1));
		multibuf[count+2] = (char)-1;
	}
	count += 3;
	// I am host - I know what's going on so attach game_mode related info which might be vital for correct kill computation
	if (multi_i_am_master())
	{
		multibuf[count] = Netgame.team_vector;	count += 1;
		multibuf[count] = Bounty_target;	count += 1;
		multi_compute_kill(killer_objnum, objnum);
		multi_send_data<MULTI_KILL_HOST>(multibuf, count, 2);
	}
	else
		multi_send_data_direct<MULTI_KILL_CLIENT>(multibuf, count, multi_who_is_master(), 2); // I am just a client so I'll only send my kill but not compute it, yet. I'll get response from host so I can compute it correctly

	multi_strip_robots(Player_num);

	if (Game_mode & GM_BOUNTY && multi_i_am_master()) // update in case if needed... we could attach this to this packet but... meh...
		multi_send_bounty();
}

void multi_send_remobj(const vobjptridx_t objnum)
{
	// Tell the other guy to remove an object from his list

	sbyte obj_owner;
	short remote_objnum;

	if (objnum->type==OBJ_POWERUP && (Game_mode & GM_NETWORK))
	{
		PowerupCaps.dec_powerup_current(get_powerup_id(objnum));
	}

	remote_objnum = objnum_local_to_remote(objnum, &obj_owner);

	PUT_INTEL_SHORT(multibuf+1, remote_objnum); // Map to network objnums

	multibuf[3] = obj_owner;

	multi_send_data<MULTI_REMOVE_OBJECT>(multibuf, 4, 2);

	if (Network_send_objects && multi_objnum_is_past(objnum))
	{
		Network_send_objnum = -1;
	}
}

void
multi_send_quit()
{
	// I am quitting the game, tell the other guy the bad news.

	multibuf[1] = Player_num;
	multi_send_data<MULTI_QUIT>(multibuf, 2, 2);

}

void
multi_send_cloak(void)
{
	// Broadcast a change in our pflags (made to support cloaking)

	multibuf[1] = (char)Player_num;

	multi_send_data<MULTI_CLOAK>(multibuf, 2, 2);

	multi_strip_robots(Player_num);
}

void
multi_send_decloak(void)
{
	// Broadcast a change in our pflags (made to support cloaking)

	multibuf[1] = (char)Player_num;

	multi_send_data<MULTI_DECLOAK>(multibuf, 2, 2);
}

void multi_send_door_open(segnum_t segnum, int side,ubyte flag)
{
	// When we open a door make sure everyone else opens that door
	PUT_INTEL_SHORT(multibuf+1, segnum );
	multibuf[3] = (sbyte)side;
#if defined(DXX_BUILD_DESCENT_I)
	(void)flag;
#elif defined(DXX_BUILD_DESCENT_II)
	multibuf[4] = flag;
#endif
	multi_send_data<MULTI_DOOR_OPEN>(multibuf, DXX_MP_SIZE_DOOR_OPEN, 2);
}

#if defined(DXX_BUILD_DESCENT_II)
void multi_send_door_open_specific(const playernum_t pnum,segnum_t segnum, int side,ubyte flag)
{
	// For sending doors only to a specific person (usually when they're joining)

	Assert (Game_mode & GM_NETWORK);
	//   Assert (pnum>-1 && pnum<N_players);

	PUT_INTEL_SHORT(multibuf+1, segnum);
	multibuf[3] = (sbyte)side;
	multibuf[4] = flag;

	multi_send_data_direct<MULTI_DOOR_OPEN>(multibuf, DXX_MP_SIZE_DOOR_OPEN, pnum, 2);
}
#endif

//
// Part 3 : Functions that change or prepare the game for multiplayer use.
//          Not including functions needed to syncronize or start the
//          particular type of multiplayer game.  Includes preparing the
//                      mines, player structures, etc.

void multi_send_create_explosion(const playernum_t pnum)
{
	// Send all data needed to create a remote explosion

	int count = 0;

	count += 1;
	multibuf[count] = (sbyte)pnum;                  count += 1;
	//                                                                                                      -----------
	//                                                                                                      Total size = 2

	multi_send_data<MULTI_CREATE_EXPLOSION>(multibuf, count, 0);
}

void multi_send_controlcen_fire(const vms_vector &to_goal, int best_gun_num, objnum_t objnum)
{
	int count = 0;

	count +=  1;
	if (words_bigendian)
	{
		vms_vector swapped_vec;
	swapped_vec.x = (fix)INTEL_INT( (int)to_goal.x );
	swapped_vec.y = (fix)INTEL_INT( (int)to_goal.y );
	swapped_vec.z = (fix)INTEL_INT( (int)to_goal.z );
		memcpy(&multibuf[count], &swapped_vec, 12);
	}
	else
	{
		memcpy(&multibuf[count], &to_goal, 12);
	}
	count += 12;
	multibuf[count] = (char)best_gun_num;                   count +=  1;
	PUT_INTEL_SHORT(multibuf+count, objnum );     count +=  2;
	//                                                                                                                      ------------
	//                                                                                                                      Total  = 16
	multi_send_data<MULTI_CONTROLCEN_FIRE>(multibuf, count, 0);
}

void multi_send_create_powerup(powerup_type_t powerup_type, segnum_t segnum, objnum_t objnum, const vms_vector &pos)
{
	// Create a powerup on a remote machine, used for remote
	// placement of used powerups like missiles and cloaking
	// powerups.

	int count = 0;

	multi_send_position(Players[Player_num].objnum);

	if (Game_mode & GM_NETWORK)
	{
		PowerupCaps.inc_powerup_current(powerup_type);
	}

	count += 1;
	multibuf[count] = Player_num;                                      count += 1;
	multibuf[count] = powerup_type;                                 count += 1;
	PUT_INTEL_SHORT(multibuf+count, segnum );     count += 2;
	PUT_INTEL_SHORT(multibuf+count, objnum );     count += 2;
	if (words_bigendian)
	{
		vms_vector swapped_vec;
	swapped_vec.x = (fix)INTEL_INT( (int)pos.x );
	swapped_vec.y = (fix)INTEL_INT( (int)pos.y );
	swapped_vec.z = (fix)INTEL_INT( (int)pos.z );
		memcpy(&multibuf[count], &swapped_vec, 12);
		count += 12;
	}
	else
	{
		memcpy(&multibuf[count], &pos, sizeof(vms_vector));
		count += sizeof(vms_vector);
	}
	//                                                                                                            -----------
	//                                                                                                            Total =  19
	multi_send_data<MULTI_CREATE_POWERUP>(multibuf, count, 2);

	if (Network_send_objects && multi_objnum_is_past(objnum))
	{
		Network_send_objnum = -1;
	}

	map_objnum_local_to_local(objnum);
}

void
multi_send_play_sound(int sound_num, fix volume)
{
	int count = 0;
	count += 1;
	multibuf[count] = Player_num;                                   count += 1;
	multibuf[count] = (char)sound_num;                      count += 1;
	multibuf[count] = (char)(volume >> 12); count += 1;
	//                                                                                                         -----------
	//                                                                                                         Total = 4
	multi_send_data<MULTI_PLAY_SOUND>(multibuf, count, 0);
}

void
multi_send_score(void)
{
	// Send my current score to all other players so it will remain
	// synced.
	int count = 0;

	if (Game_mode & GM_MULTI_COOP) {
		multi_sort_kill_list();
		count += 1;
		multibuf[count] = Player_num;                           count += 1;
		PUT_INTEL_INT(multibuf+count, Players[Player_num].score);  count += 4;
		multi_send_data<MULTI_SCORE>(multibuf, count, 0);
	}
}

void
multi_send_trigger(int triggernum)
{
	// Send an event to trigger something in the mine

	int count = 0;

	count += 1;
	multibuf[count] = Player_num;                                   count += 1;
	multibuf[count] = (ubyte)triggernum;            count += 1;

	multi_send_data<MULTI_TRIGGER>(multibuf, count, 2);
}

#if defined(DXX_BUILD_DESCENT_II)
void multi_send_effect_blowup(segnum_t segnum, int side, const vms_vector &pnt)
{
	// We blew up something connected to a trigger. Send this blowup result to other players shortly before MULTI_TRIGGER.
	// NOTE: The reason this is now a separate packet is to make sure trigger-connected switches/monitors are in sync with MULTI_TRIGGER.
	//       If a fire packet is late it might blow up a switch for some clients without the shooter actually registering this hit,
	//       not sending MULTI_TRIGGER and making puzzles or progress impossible.
	int count = 0;

	multi_do_protocol_frame(1, 0); // force packets to be sent, ensuring this packet will be attached to following MULTI_TRIGGER
	
	count += 1;
	multibuf[count] = Player_num;                                   count += 1;
	PUT_INTEL_SHORT(multibuf+count, segnum);                        count += 2;
	multibuf[count] = (sbyte)side;                                  count += 1;
	PUT_INTEL_INT(multibuf+count, pnt.x);                          count += 4;
	PUT_INTEL_INT(multibuf+count, pnt.y);                          count += 4;
	PUT_INTEL_INT(multibuf+count, pnt.z);                          count += 4;

	multi_send_data<MULTI_EFFECT_BLOWUP>(multibuf, count, 0);
}
#endif

void multi_send_hostage_door_status(uint16_t wallnum)
{
	// Tell the other player what the hit point status of a hostage door
	// should be

	int count = 0;

	Assert(Walls[wallnum].type == WALL_BLASTABLE);

	count += 1;
	PUT_INTEL_SHORT(multibuf+count, wallnum );           count += 2;
	PUT_INTEL_INT(multibuf+count, Walls[wallnum].hps );  count += 4;

	multi_send_data<MULTI_HOSTAGE_DOOR>(multibuf, count, 0);
}

void multi_consistency_error(int reset)
{
	static int count = 0;

	if (reset)
		count = 0;

	if (++count < 10)
		return;

	if (Game_wind)
		window_set_visible(Game_wind, 0);
	nm_messagebox(NULL, 1, TXT_OK, TXT_CONSISTENCY_ERROR);
	if (Game_wind)
		window_set_visible(Game_wind, 1);
	count = 0;
	multi_quit_game = 1;
	game_leave_menus();
	multi_reset_stuff();
}

static constexpr unsigned grant_shift_helper(const packed_spawn_granted_items p, int s)
{
	return s > 0 ? p.mask >> s : p.mask << -s;
}

uint_fast32_t map_granted_flags_to_player_flags(const packed_spawn_granted_items p)
{
	auto &grant = p.mask;
	return ((grant & NETGRANT_QUAD) ? PLAYER_FLAGS_QUAD_LASERS : 0)
#if defined(DXX_BUILD_DESCENT_II)
		| ((grant & NETGRANT_AFTERBURNER) ? PLAYER_FLAGS_AFTERBURNER : 0)
		| ((grant & NETGRANT_AMMORACK) ? PLAYER_FLAGS_AMMO_RACK : 0)
		| ((grant & NETGRANT_CONVERTER) ? PLAYER_FLAGS_CONVERTER : 0)
		| ((grant & NETGRANT_HEADLIGHT) ? PLAYER_FLAGS_HEADLIGHT : 0)
#endif
		;
}

uint_fast32_t map_granted_flags_to_primary_weapon_flags(const packed_spawn_granted_items p)
{
	auto &grant = p.mask;
	return ((grant & NETGRANT_VULCAN) ? HAS_VULCAN_FLAG : 0)
		| ((grant & NETGRANT_SPREAD) ? HAS_SPREADFIRE_FLAG : 0)
		| ((grant & NETGRANT_PLASMA) ? HAS_PLASMA_FLAG : 0)
		| ((grant & NETGRANT_FUSION) ? HAS_FUSION_FLAG : 0)
#if defined(DXX_BUILD_DESCENT_II)
		| ((grant & NETGRANT_GAUSS) ? HAS_GAUSS_FLAG : 0)
		| ((grant & NETGRANT_HELIX) ? HAS_HELIX_FLAG : 0)
		| ((grant & NETGRANT_PHOENIX) ? HAS_PHOENIX_FLAG : 0)
		| ((grant & NETGRANT_OMEGA) ? HAS_OMEGA_FLAG : 0)
#endif
		;
}

uint16_t map_granted_flags_to_vulcan_ammo(const packed_spawn_granted_items p)
{
	auto &grant = p.mask;
	const auto amount = VULCAN_WEAPON_AMMO_AMOUNT;
	return
#if defined(DXX_BUILD_DESCENT_II)
		(grant & NETGRANT_GAUSS ? amount : 0) +
#endif
		(grant & NETGRANT_VULCAN ? amount : 0);
}

static constexpr unsigned map_granted_flags_to_netflag(const packed_spawn_granted_items grant)
{
	return (grant_shift_helper(grant, BIT_NETGRANT_QUAD - BIT_NETFLAG_DOQUAD) & (NETFLAG_DOQUAD | NETFLAG_DOVULCAN | NETFLAG_DOSPREAD | NETFLAG_DOPLASMA | NETFLAG_DOFUSION))
#if defined(DXX_BUILD_DESCENT_II)
		| (grant_shift_helper(grant, BIT_NETGRANT_GAUSS - BIT_NETFLAG_DOGAUSS) & (NETFLAG_DOGAUSS | NETFLAG_DOHELIX | NETFLAG_DOPHOENIX | NETFLAG_DOOMEGA))
		| (grant_shift_helper(grant, BIT_NETGRANT_AFTERBURNER - BIT_NETFLAG_DOAFTERBURNER) & (NETFLAG_DOAFTERBURNER | NETFLAG_DOAMMORACK | NETFLAG_DOCONVERTER | NETFLAG_DOHEADLIGHT))
#endif
		;
}

assert_equal(0, 0, "zero");
assert_equal(map_granted_flags_to_netflag(NETGRANT_QUAD), NETFLAG_DOQUAD, "QUAD");
assert_equal(map_granted_flags_to_netflag(NETGRANT_QUAD | NETGRANT_PLASMA), NETFLAG_DOQUAD | NETFLAG_DOPLASMA, "QUAD | PLASMA");
#if defined(DXX_BUILD_DESCENT_II)
assert_equal(map_granted_flags_to_netflag(NETGRANT_GAUSS), NETFLAG_DOGAUSS, "GAUSS");
assert_equal(map_granted_flags_to_netflag(NETGRANT_GAUSS | NETGRANT_PLASMA), NETFLAG_DOGAUSS | NETFLAG_DOPLASMA, "GAUSS | PLASMA");
assert_equal(map_granted_flags_to_netflag(NETGRANT_GAUSS | NETGRANT_AFTERBURNER), NETFLAG_DOGAUSS | NETFLAG_DOAFTERBURNER, "GAUSS | AFTERBURNER");
assert_equal(map_granted_flags_to_netflag(NETGRANT_GAUSS | NETGRANT_PLASMA | NETGRANT_AFTERBURNER), NETFLAG_DOGAUSS | NETFLAG_DOPLASMA | NETFLAG_DOAFTERBURNER, "GAUSS | PLASMA | AFTERBURNER");
assert_equal(map_granted_flags_to_netflag(NETGRANT_PLASMA | NETGRANT_AFTERBURNER), NETFLAG_DOPLASMA | NETFLAG_DOAFTERBURNER, "PLASMA | AFTERBURNER");
assert_equal(map_granted_flags_to_netflag(NETGRANT_AFTERBURNER), NETFLAG_DOAFTERBURNER, "AFTERBURNER");
assert_equal(map_granted_flags_to_netflag(NETGRANT_HEADLIGHT), NETFLAG_DOHEADLIGHT, "HEADLIGHT");
#endif

namespace {

class update_item_state
{
	std::bitset<MAX_OBJECTS> m_modified;
public:
	bool must_skip(const vcobjptridx_t i) const
	{
		return m_modified.test(i);
	}
	void process_powerup(vcobjptridx_t, powerup_type_t);
};

void update_item_state::process_powerup(const vcobjptridx_t o, const powerup_type_t id)
{
	uint_fast32_t count;
	switch (id)
	{
		case POW_LASER:
		case POW_QUAD_FIRE:
		case POW_VULCAN_WEAPON:
		case POW_VULCAN_AMMO:
		case POW_SPREADFIRE_WEAPON:
		case POW_PLASMA_WEAPON:
		case POW_FUSION_WEAPON:
#if defined(DXX_BUILD_DESCENT_II)
		case POW_SUPER_LASER:
		case POW_GAUSS_WEAPON:
		case POW_HELIX_WEAPON:
		case POW_PHOENIX_WEAPON:
		case POW_OMEGA_WEAPON:
#endif
			count = Netgame.DuplicatePowerups.get_primary_count();
			break;
		case POW_MISSILE_1:
		case POW_MISSILE_4:
		case POW_HOMING_AMMO_1:
		case POW_HOMING_AMMO_4:
		case POW_PROXIMITY_WEAPON:
		case POW_SMARTBOMB_WEAPON:
		case POW_MEGA_WEAPON:
#if defined(DXX_BUILD_DESCENT_II)
		case POW_SMISSILE1_1:
		case POW_SMISSILE1_4:
		case POW_GUIDED_MISSILE_1:
		case POW_GUIDED_MISSILE_4:
		case POW_SMART_MINE:
		case POW_MERCURY_MISSILE_1:
		case POW_MERCURY_MISSILE_4:
		case POW_EARTHSHAKER_MISSILE:
#endif
			count = Netgame.DuplicatePowerups.get_secondary_count();
			break;
#if defined(DXX_BUILD_DESCENT_II)
		case POW_FULL_MAP:
		case POW_CONVERTER:
		case POW_AMMO_RACK:
		case POW_AFTERBURNER:
		case POW_HEADLIGHT:
			count = Netgame.DuplicatePowerups.get_accessory_count();
			break;
#endif
		default:
			return;
	}
	if (!count)
		return;
	const auto &vc = Vclip[o->rtype.vclip_info.vclip_num];
	for (uint_fast32_t i = count++; i; --i)
	{
		const auto &seg = vsegptr(o->segnum);
		const auto no = obj_create(OBJ_POWERUP, id, o->segnum, vm_vec_avg(o->pos, Vertices[seg->verts[i % seg->verts.size()]]), &vmd_identity_matrix, o->size, CT_POWERUP, MT_PHYSICS, RT_POWERUP);
		if (no == object_none)
			return;
		m_modified.set(no);
		PowerupCaps.inc_powerup_both(id);
		no->mtype.phys_info = o->mtype.phys_info;
		no->rtype.vclip_info = o->rtype.vclip_info;
		no->rtype.vclip_info.framenum = (o->rtype.vclip_info.framenum + (i * vc.num_frames) / count) % vc.num_frames;
		no->ctype.powerup_info = o->ctype.powerup_info;
	}
}

}

void multi_prep_level(void)
{
	// Do any special stuff to the level required for games
	// before we begin playing in it.

	// Player_num MUST be set before calling this procedure.

	// This function must be called before checksuming the Object array,
	// since the resulting checksum with depend on the value of Player_num
	// at the time this is called.

	int i;

	Assert(Game_mode & GM_MULTI);

	Assert(NumNetPlayerPositions > 0);

#if defined(DXX_BUILD_DESCENT_II)
	PhallicLimit=0;
	PhallicMan=-1;
	Drop_afterburner_blob_flag=0;
#endif
	Bounty_target = 0;

	multi_consistency_error(1);

	for (i=0;i<MAX_PLAYERS;i++)
	{
		multi_sending_message[i] = msgsend_none;
		if (imulti_new_game)
			init_player_stats_new_ship(i);
	}

	for (i = 0; i < NumNetPlayerPositions; i++)
	{
		const auto objp = vobjptridx(Players[i].objnum);
		if (i != Player_num)
			objp->control_type = CT_REMOTE;
		objp->movement_type = MT_PHYSICS;
		multi_reset_player_object(objp);
		Netgame.players[i].LastPacketTime = 0;
	}

	for (i = 0; i < MAX_ROBOTS_CONTROLLED; i++)
	{
		robot_controlled[i] = -1;
		robot_agitation[i] = 0;
		robot_fired[i] = 0;
	}

	Viewer = ConsoleObject = &Objects[Players[Player_num].objnum];

	if (!(Game_mode & GM_MULTI_COOP))
	{
		multi_delete_extra_objects(); // Removes monsters from level
	}

	if ((Game_mode & GM_NETWORK) && !(Game_mode & GM_MULTI_COOP))
	{
		multi_powcap_adjust_cap_for_player(Player_num);
		multi_send_powcap_update();
	}

	constexpr unsigned MAX_ALLOWED_INVULNERABILITY = 3;
	constexpr unsigned MAX_ALLOWED_CLOAK = 3;
	const auto AllowedItems = Netgame.AllowedItems;
	const auto SpawnGrantedItems = map_granted_flags_to_netflag(Netgame.SpawnGrantedItems);
	unsigned inv_remaining = (AllowedItems & NETFLAG_DOINVUL) ? MAX_ALLOWED_INVULNERABILITY : 0;
	unsigned cloak_remaining = (AllowedItems & NETFLAG_DOCLOAK) ? MAX_ALLOWED_CLOAK : 0;
	update_item_state duplicates;
	range_for (const auto i, highest_valid(Objects))
	{
		const auto o = vobjptridx(i);
		if ((o->type == OBJ_HOSTAGE) && !(Game_mode & GM_MULTI_COOP))
		{
			const auto objnum = obj_create(OBJ_POWERUP, POW_SHIELD_BOOST, o->segnum, o->pos, &vmd_identity_matrix, Powerup_info[POW_SHIELD_BOOST].size, CT_POWERUP, MT_PHYSICS, RT_POWERUP);
			obj_delete(o);
			if (objnum != object_none)
			{
				objnum->rtype.vclip_info.vclip_num = Powerup_info[POW_SHIELD_BOOST].vclip_num;
				objnum->rtype.vclip_info.frametime = Vclip[objnum->rtype.vclip_info.vclip_num].frame_time;
				objnum->rtype.vclip_info.framenum = 0;
				objnum->mtype.phys_info.drag = 512;     //1024;
				objnum->mtype.phys_info.mass = F1_0;
				vm_vec_zero(objnum->mtype.phys_info.velocity);
			}
			continue;
		}

		if (o->type == OBJ_POWERUP && !duplicates.must_skip(o))
		{
			switch (const auto id = get_powerup_id(o))
			{
				case POW_EXTRA_LIFE:
					set_powerup_id(o, POW_INVULNERABILITY);
					/* fall through */
				case POW_INVULNERABILITY:
					if (inv_remaining)
						-- inv_remaining;
					else
						set_powerup_id(o, POW_SHIELD_BOOST);
					continue;
				case POW_CLOAK:
					if (cloak_remaining)
						-- cloak_remaining;
					else
						set_powerup_id(o, POW_SHIELD_BOOST);
					continue;
				default:
					if (!multi_powerup_is_allowed(id, AllowedItems, SpawnGrantedItems))
						bash_to_shield(o);
					else
						duplicates.process_powerup(o, id);
					continue;
			}
		}
	}

#if defined(DXX_BUILD_DESCENT_II)
	if (game_mode_hoard())
		init_hoard_data();

	if (game_mode_capture_flag() || game_mode_hoard())
		multi_apply_goal_textures();
#endif

	multi_sort_kill_list();

	multi_show_player_list();

	ConsoleObject->control_type = CT_FLYING;

	reset_player_object();

	imulti_new_game=0;
}

int multi_level_sync(void)
{
	switch (multi_protocol)
	{
#ifdef USE_UDP
		case MULTI_PROTO_UDP:
			return net_udp_level_sync();
			break;
#endif
		default:
			Error("Protocol handling missing in multi_level_sync\n");
			break;
	}
}

#if defined(DXX_BUILD_DESCENT_II)
static void apply_segment_goal_texture(const vsegptr_t seg, ubyte team_mask)
{
	seg->static_light = i2f(100);	//make static light bright
	std::size_t tex = find_goal_texture(game_mode_hoard() ? TMI_GOAL_HOARD : team_mask);
	if (tex < TmapInfo.size())
		range_for (auto &s, seg->sides)
		{
			s.tmap_num = tex;
			range_for (auto &uvl, s.uvls)
				uvl.l = i2f(100);		//max out
		}
}

void multi_apply_goal_textures()
{
	range_for (const auto i, highest_valid(Segments))
	{
		const auto &&seg = vsegptr(static_cast<segnum_t>(i));
		if (seg->special==SEGMENT_IS_GOAL_BLUE)
		{
			apply_segment_goal_texture(seg, TMI_GOAL_BLUE);
		}
		else if (seg->special==SEGMENT_IS_GOAL_RED)
		{
			// Make both textures the same if Hoard mode
			apply_segment_goal_texture(seg, TMI_GOAL_RED);
		}
	}
}

std::size_t find_goal_texture (ubyte t)
{
	auto r = partial_range(TmapInfo, NumTextures);
	return std::distance(r.begin(), std::find_if(r.begin(), r.end(), [t](const tmap_info &i) { return (i.flags & t); }));
}

tmap_info &find_required_goal_texture(ubyte t)
{
	std::size_t r = find_goal_texture(t);
	if (r < TmapInfo.size())
		return TmapInfo[r];
	Int3(); // Hey, there is no goal texture for this PIG!!!!
	// Edit bitmaps.tbl and designate two textures to be RED and BLUE
	// goal textures
	throw std::runtime_error("PIG missing goal texture");
}
#endif

static inline int object_allowed_in_anarchy(const vobjptr_t objp)
{
	if ((objp->type==OBJ_NONE) ||
		(objp->type==OBJ_PLAYER) ||
		(objp->type==OBJ_POWERUP) ||
		(objp->type==OBJ_CNTRLCEN) ||
		(objp->type==OBJ_HOSTAGE))
		return 1;
#if defined(DXX_BUILD_DESCENT_II)
	if (objp->type==OBJ_WEAPON && get_weapon_id(objp)==PMINE_ID)
		return 1;
#endif
	return 0;
}

int multi_delete_extra_objects()
{
	int nnp=0;

	// Go through the object list and remove any objects not used in
	// 'Anarchy!' games.

	// This function also prints the total number of available multiplayer
	// positions in this level, even though this should always be 8 or more!

	range_for (const auto i, highest_valid(Objects))
	{
		const auto objp = vobjptridx(i);
		if ((objp->type==OBJ_PLAYER) || (objp->type==OBJ_GHOST))
			nnp++;
		else if ((objp->type==OBJ_ROBOT) && (Game_mode & GM_MULTI_ROBOTS))
			;
		else if (!object_allowed_in_anarchy(objp) ) {
#if defined(DXX_BUILD_DESCENT_II)
			// Before deleting object, if it's a robot, drop it's special powerup, if any
			if (objp->type == OBJ_ROBOT)
				if (objp->contains_count && (objp->contains_type == OBJ_POWERUP))
					object_create_robot_egg(objp);
#endif
			obj_delete(objp);
		}
	}

	return nnp;
}

// Returns 1 if player is Master/Host of this game
int multi_i_am_master(void)
{
	return (Player_num == 0);
}

// Returns the Player_num of Master/Host of this game
playernum_t multi_who_is_master()
{
	return 0;
}

void change_playernum_to( int new_Player_num )
{
	if (Player_num < Players.size())
	{
		Players[new_Player_num].callsign = Players[Player_num].callsign;
	}

	Player_num = new_Player_num;
}

#if defined(DXX_BUILD_DESCENT_I)
static
#endif
int multi_all_players_alive()
{
	range_for (auto &i, partial_range(Players, N_players))
	{
		if ((i.connected == CONNECT_PLAYING) && (Objects[i.objnum].type == OBJ_GHOST)) // player alive?
			return (0);
		if ((i.connected != CONNECT_DISCONNECTED) && (i.connected != CONNECT_PLAYING)) // ... and actually playing?
			return (0);
	}
	return (1);
}

#if defined(DXX_BUILD_DESCENT_II)
void multi_send_drop_weapon(objnum_t objnum, int seed)
{
	int count=0;
	int ammo_count;

	multi_send_position(Players[Player_num].objnum);
	auto objp = &Objects[objnum];
	ammo_count = objp->ctype.powerup_info.count;

	if (get_powerup_id(objp) == POW_OMEGA_WEAPON && ammo_count == F1_0)
		ammo_count = F1_0 - 1; //make fit in short

	Assert(ammo_count < F1_0); //make sure fits in short

	count++;
	multibuf[count++]=(char)get_powerup_id(objp);
	PUT_INTEL_SHORT(multibuf+count, objnum); count += 2;
	PUT_INTEL_SHORT(multibuf+count, static_cast<uint16_t>(ammo_count)); count += 2;
	PUT_INTEL_INT(multibuf+count, seed);
	count += 4;

	map_objnum_local_to_local(objnum);

	if (Game_mode & GM_NETWORK)
	{
		PowerupCaps.inc_powerup_current(get_powerup_id(objp));
	}
	multi_send_data<MULTI_DROP_WEAPON>(multibuf, count, 2);
}

static void multi_do_drop_weapon (const playernum_t pnum, const ubyte *buf)
{
	int ammo,remote_objnum,seed;
	const auto powerup_id = static_cast<powerup_type_t>(buf[1]);
	remote_objnum = GET_INTEL_SHORT(buf + 2);
	ammo = GET_INTEL_SHORT(buf + 4);
	seed = GET_INTEL_INT(buf + 6);
	auto objp = &Objects[Players[pnum].objnum];
	auto objnum = spit_powerup(objp, powerup_id, seed);

	map_objnum_local_to_remote(objnum, remote_objnum, pnum);

	if (objnum!=object_none)
		objnum->ctype.powerup_info.count = ammo;

	if (Game_mode & GM_NETWORK)
	{
		PowerupCaps.inc_powerup_current(powerup_id);
	}
}

void multi_send_guided_info (const vobjptr_t miss,char done)
{
	int count=0;

	count++;
	multibuf[count++]=(char)Player_num;
	multibuf[count++]=done;

	if (words_bigendian)
	{
		shortpos sp;
		create_shortpos_little(&sp, miss);
		memcpy(&multibuf[count], sp.bytemat, 9);
	count += 9;
		memcpy(&multibuf[count], &sp.xo, 14);
	count += 14;
	}
	else
	{
		create_shortpos_little(reinterpret_cast<shortpos *>(&multibuf[count]), miss);
		count += sizeof(shortpos);
	}
	multi_send_data<MULTI_GUIDED>(multibuf, count, 0);
}

static void multi_do_guided (const playernum_t pnum, const ubyte *buf)
{
	int count=3;

	if (Guided_missile[(int)pnum]==NULL)
	{
		return;
	}

	if (buf[2])
	{
		release_guided_missile(pnum);
		return;
	}

	if (words_bigendian)
	{
		shortpos sp;
		memcpy(sp.bytemat, &buf[count], 9);
		memcpy(&sp.xo, &buf[count + 9], 14);
		extract_shortpos_little(Guided_missile[(int)pnum], &sp);
	}
	else
	{
		extract_shortpos_little(Guided_missile[(int)pnum], reinterpret_cast<const shortpos *>(&buf[count]));
	}

	count+=sizeof (shortpos);

	update_object_seg(Guided_missile[(int)pnum]);
}

void multi_send_stolen_items ()
{
	int i,count=1;

	for (i=0;i<MAX_STOLEN_ITEMS;i++)
	{
		multibuf[i+1]=Stolen_items[i];
		count++;      // So I like to break my stuff into smaller chunks, so what?
	}
	multi_send_data<MULTI_STOLEN_ITEMS>(multibuf, count, 2);
}

static void multi_do_stolen_items (const ubyte *buf)
{
	int i;

	for (i=0;i<MAX_STOLEN_ITEMS;i++)
	{
		Stolen_items[i]=buf[i+1];
	}
}

void multi_send_wall_status_specific(const playernum_t pnum,uint16_t wallnum,ubyte type,ubyte flags,ubyte state)
{
	// Send wall states a specific rejoining player

	int count=0;

	Assert (Game_mode & GM_NETWORK);
	//Assert (pnum>-1 && pnum<N_players);

	count++;
	PUT_INTEL_SHORT(multibuf+count, wallnum);  count+=2;
	multibuf[count]=type;                 count++;
	multibuf[count]=flags;                count++;
	multibuf[count]=state;                count++;

	multi_send_data_direct<MULTI_WALL_STATUS>(multibuf, count, pnum, 2);
}

static void multi_do_wall_status (const ubyte *buf)
{
	short wallnum;
	ubyte flag,type,state;

	wallnum = GET_INTEL_SHORT(buf + 1);
	type=buf[3];
	flag=buf[4];
	state=buf[5];

	Assert (wallnum>=0);
	Walls[wallnum].type=type;
	Walls[wallnum].flags=flag;
	//Assert(state <= 4);
	Walls[wallnum].state=state;

	if (Walls[wallnum].type==WALL_OPEN)
	{
		digi_kill_sound_linked_to_segment(Walls[wallnum].segnum,Walls[wallnum].sidenum,SOUND_FORCEFIELD_HUM);
		//digi_kill_sound_linked_to_segment(csegp-Segments,cside,SOUND_FORCEFIELD_HUM);
	}
}
#endif

void multi_send_kill_goal_counts()
{
	int i,count=1;

	for (i=0;i<MAX_PLAYERS;i++)
	{
		*(char *)(multibuf+count)=(char)Players[i].KillGoalCount;
		count++;
	}

	multi_send_data<MULTI_KILLGOALS>(multibuf, count, 2);
}

static void multi_do_kill_goal_counts(const ubyte *buf)
{
	int i,count=1;

	for (i=0;i<MAX_PLAYERS;i++)
	{
		Players[i].KillGoalCount = buf[count];
		count++;
	}

}

void multi_send_heartbeat ()
{
	if (!Netgame.PlayTimeAllowed)
		return;

	PUT_INTEL_INT(multibuf+1, ThisLevelTime);
	multi_send_data<MULTI_HEARTBEAT>(multibuf, 5, 0);
}

static void multi_do_heartbeat (const ubyte *buf)
{
	fix num;

	num = GET_INTEL_INT(buf + 1);

	ThisLevelTime=num;
}

void multi_check_for_killgoal_winner ()
{
	int i,best=0,bestnum=0;
	if (Control_center_destroyed)
		return;

	for (i=0;i<N_players;i++)
	{
		if (Players[i].KillGoalCount>best)
		{
			best=Players[i].KillGoalCount;
			bestnum=i;
		}
	}

	if (bestnum==Player_num)
	{
		HUD_init_message(HM_MULTI, "You have the best score at %d kills!",best);
		//Players[Player_num].shields=i2f(200);
	}
	else

		HUD_init_message(HM_MULTI, "%s has the best score with %d kills!",static_cast<const char *>(Players[bestnum].callsign),best);

	HUD_init_message_literal(HM_MULTI, "The control center has been destroyed!");
	net_destroy_controlcen (obj_find_first_of_type (OBJ_CNTRLCEN));
}

#if defined(DXX_BUILD_DESCENT_II)
// Sync our seismic time with other players
void multi_send_seismic(fix duration)
{
	int count=1;
	PUT_INTEL_INT(&multibuf[count], duration); count += sizeof(duration);
	multi_send_data<MULTI_SEISMIC>(multibuf, count, 2);
}

static void multi_do_seismic (const ubyte *buf)
{
	const fix duration = GET_INTEL_INT(&buf[1]);
	Seismic_disturbance_end_time = GameTime64 + duration;
	digi_play_sample (SOUND_SEISMIC_DISTURBANCE_START, F1_0);
}

void multi_send_light_specific (const playernum_t pnum,segnum_t segnum,ubyte val)
{
	int count=1;

	Assert (Game_mode & GM_NETWORK);
	//  Assert (pnum>-1 && pnum<N_players);

	PUT_INTEL_INT(multibuf+count, segnum); count+=(sizeof(int));
	*(char *)(multibuf+count)=val; count++;

	range_for (auto &i, Segments[segnum].sides)
	{
		PUT_INTEL_SHORT(multibuf+count, i.tmap_num2); count+=2;
	}

	multi_send_data_direct<MULTI_LIGHT>(multibuf, count, pnum, 2);
}

static void multi_do_light (const ubyte *buf)
{
	int i;
	const auto sides = buf[5];

	segnum_t seg;
	seg = GET_INTEL_INT(buf + 1);
	auto &side_array = vsegptr(seg)->sides;
	for (i=0;i<6;i++)
	{
		if ((sides & (1<<i)))
		{
			subtract_light (seg,i);
			side_array[i].tmap_num2 = GET_INTEL_SHORT(&buf[6 + (2 * i)]);
		}
	}
}

static void multi_do_flags (const playernum_t pnum, const ubyte *buf)
{
	uint flags;

	flags = GET_INTEL_INT(buf + 2);
	if (pnum!=Player_num)
		Players[(int)pnum].flags=flags;
}

void multi_send_flags (const playernum_t pnum)
{
	multibuf[1]=pnum;
	PUT_INTEL_INT(multibuf+2, Players[(int)pnum].flags);
 
	multi_send_data<MULTI_FLAGS>(multibuf, 6, 2);
}

void multi_send_drop_blobs (const playernum_t pnum)
{
	multibuf[1]=pnum;

	multi_send_data<MULTI_DROP_BLOB>(multibuf, 2, 0);
}

static void multi_do_drop_blob (const playernum_t pnum)
{
	drop_afterburner_blobs (&Objects[Players[(int)pnum].objnum], 2, i2f(5)/2, -1);
}
#endif

void multi_send_powcap_update ()
{
	if (!(Game_mode & GM_NETWORK) || (Game_mode & GM_MULTI_COOP))
		return;

	for (unsigned i=0;i<MAX_POWERUP_TYPES;i++)
		multibuf[i+1] = PowerupCaps.get_max(static_cast<powerup_type_t>(i));

	multi_send_data<MULTI_POWCAP_UPDATE>(multibuf, MAX_POWERUP_TYPES+1, 2);
}

static void multi_do_powcap_update (const ubyte *buf)
{
	if (!(Game_mode & GM_NETWORK) || (Game_mode & GM_MULTI_COOP))
		return;

	for (unsigned i=0;i<MAX_POWERUP_TYPES;i++)
	{
		const auto p = static_cast<powerup_type_t>(i);
		if (PowerupCaps.get_max(p) < buf[i+1])
			PowerupCaps.set_max(p, buf[i+1]);
	}
}

#if defined(DXX_BUILD_DESCENT_II)
void multi_send_sound_function (char whichfunc, char sound)
{
	int count=0;

	count++;
	multibuf[1]=Player_num;             count++;
	multibuf[2]=whichfunc;              count++;
	if (!words_bigendian)
	{
	*(uint *)(multibuf+count)=sound;    count++;
	}
	else
	{
	multibuf[3] = sound; count++;       // this would probably work on the PC as well.  Jason?
	}
	multi_send_data<MULTI_SOUND_FUNCTION>(multibuf,4,2);
}

#define AFTERBURNER_LOOP_START  20098
#define AFTERBURNER_LOOP_END    25776

static void multi_do_sound_function (const playernum_t pnum, const ubyte *buf)
{
	// for afterburner

	char whichfunc;
	int sound;

	if (Players[Player_num].connected!=CONNECT_PLAYING)
		return;

	whichfunc=buf[2];
	sound=buf[3];

	const auto plobj = vcobjptridx(Players[pnum].objnum);
	if (whichfunc==0)
		digi_kill_sound_linked_to_object(plobj);
	else if (whichfunc==3)
		digi_link_sound_to_object3(sound, plobj, 1,F1_0, vm_distance{i2f(256)}, AFTERBURNER_LOOP_START, AFTERBURNER_LOOP_END);
}

void multi_send_capture_bonus (const playernum_t pnum)
{
	Assert (game_mode_capture_flag());

	multibuf[1]=pnum;

	multi_send_data<MULTI_CAPTURE_BONUS>(multibuf,2,2);
	multi_do_capture_bonus (pnum);
}

void multi_send_orb_bonus (const playernum_t pnum)
{
	Assert (game_mode_hoard());

	multibuf[1]=pnum;
	multibuf[2]=Players[Player_num].secondary_ammo[PROXIMITY_INDEX];

	multi_send_data<MULTI_ORB_BONUS>(multibuf,3,2);
	multi_do_orb_bonus (pnum, multibuf);
}

void multi_do_capture_bonus(const playernum_t pnum)
{
	// Figure out the results of a network kills and add it to the
	// appropriate player's tally.

	int TheGoal;

	if (pnum==Player_num)
		HUD_init_message_literal(HM_MULTI, "You have Scored!");
	else
		HUD_init_message(HM_MULTI, "%s has Scored!", static_cast<const char *>(Players[pnum].callsign));

	if (pnum==Player_num)
		digi_play_sample (SOUND_HUD_YOU_GOT_GOAL,F1_0*2);
	else if (get_team(pnum)==TEAM_RED)
		digi_play_sample (SOUND_HUD_RED_GOT_GOAL,F1_0*2);
	else
		digi_play_sample (SOUND_HUD_BLUE_GOT_GOAL,F1_0*2);

	Players[(int)pnum].flags &= ~(PLAYER_FLAGS_FLAG);  // Clear capture flag

	team_kills[get_team(pnum)] += 5;
	Players[(int)pnum].net_kills_total += 5;
	Players[(int)pnum].KillGoalCount+=5;

	if (Netgame.KillGoal>0)
	{
		TheGoal=Netgame.KillGoal*5;

		if (Players[(int)pnum].KillGoalCount>=TheGoal)
		{
			if (pnum==Player_num)
			{
				HUD_init_message_literal(HM_MULTI, "You reached the kill goal!");
				Players[Player_num].shields=i2f(200);
			}
			else
				HUD_init_message(HM_MULTI, "%s has reached the kill goal!",static_cast<const char *>(Players[pnum].callsign));

			HUD_init_message_literal(HM_MULTI, "The control center has been destroyed!");
			net_destroy_controlcen (obj_find_first_of_type (OBJ_CNTRLCEN));
		}
	}

	multi_sort_kill_list();
	multi_show_player_list();
}

static int GetOrbBonus (char num)
{
	int bonus;

	bonus=num*(num+1)/2;
	return (bonus);
}

void multi_do_orb_bonus(const playernum_t pnum, const ubyte *buf)
{
	// Figure out the results of a network kills and add it to the
	// appropriate player's tally.

	int TheGoal;
	int bonus=GetOrbBonus (buf[2]);

	if (pnum==Player_num)
		HUD_init_message(HM_MULTI, "You have scored %d points!",bonus);
	else
		HUD_init_message(HM_MULTI, "%s has scored with %d orbs!",static_cast<const char *>(Players[pnum].callsign), buf[2]);

	if (pnum==Player_num)
		digi_start_sound_queued (SOUND_HUD_YOU_GOT_GOAL,F1_0*2);
	else if (Game_mode & GM_TEAM)
	{
		if (get_team(pnum)==TEAM_RED)
			digi_play_sample (SOUND_HUD_RED_GOT_GOAL,F1_0*2);
		else
			digi_play_sample (SOUND_HUD_BLUE_GOT_GOAL,F1_0*2);
	}
	else
		digi_play_sample (SOUND_OPPONENT_HAS_SCORED,F1_0*2);

	if (bonus>PhallicLimit)
	{
		if (pnum==Player_num)
			HUD_init_message(HM_MULTI, "You have the record with %d points!",bonus);
		else
			HUD_init_message(HM_MULTI, "%s has the record with %d points!",static_cast<const char *>(Players[pnum].callsign),bonus);
		digi_play_sample (SOUND_BUDDY_MET_GOAL,F1_0*2);
		PhallicMan=pnum;
		PhallicLimit=bonus;
	}

	Players[(int)pnum].flags &= ~(PLAYER_FLAGS_FLAG);  // Clear orb flag

	team_kills[get_team(pnum)] += bonus;
	Players[(int)pnum].net_kills_total += bonus;
	Players[(int)pnum].KillGoalCount+=bonus;

	team_kills[get_team(pnum)]%=1000;
	Players[(int)pnum].net_kills_total%=1000;
	Players[(int)pnum].KillGoalCount%=1000;

	if (Netgame.KillGoal>0)
	{
		TheGoal=Netgame.KillGoal*5;

		if (Players[(int)pnum].KillGoalCount>=TheGoal)
		{
			if (pnum==Player_num)
			{
				HUD_init_message_literal(HM_MULTI, "You reached the kill goal!");
				Players[Player_num].shields=i2f(200);
			}
			else
				HUD_init_message(HM_MULTI, "%s has reached the kill goal!",static_cast<const char *>(Players[pnum].callsign));

			HUD_init_message_literal(HM_MULTI, "The control center has been destroyed!");
			net_destroy_controlcen (obj_find_first_of_type (OBJ_CNTRLCEN));
		}
	}
	multi_sort_kill_list();
	multi_show_player_list();
}

void multi_send_got_flag (const playernum_t pnum)
{
	multibuf[1]=pnum;

	digi_start_sound_queued (SOUND_HUD_YOU_GOT_FLAG,F1_0*2);

	multi_send_data<MULTI_GOT_FLAG>(multibuf,2,2);
	multi_send_flags (Player_num);
}

void multi_send_got_orb (const playernum_t pnum)
{
	multibuf[1]=pnum;

	digi_play_sample (SOUND_YOU_GOT_ORB,F1_0*2);

	multi_send_data<MULTI_GOT_ORB>(multibuf,2,2);
	multi_send_flags (Player_num);
}

static void multi_do_got_flag (const playernum_t pnum)
{
	if (pnum==Player_num)
		digi_start_sound_queued (SOUND_HUD_YOU_GOT_FLAG,F1_0*2);
	else if (get_team(pnum)==TEAM_RED)
		digi_start_sound_queued (SOUND_HUD_RED_GOT_FLAG,F1_0*2);
	else
		digi_start_sound_queued (SOUND_HUD_BLUE_GOT_FLAG,F1_0*2);
	Players[(int)pnum].flags|=PLAYER_FLAGS_FLAG;
	HUD_init_message(HM_MULTI, "%s picked up a flag!",static_cast<const char *>(Players[pnum].callsign));
}
static void multi_do_got_orb (const playernum_t pnum)
{
	Assert (game_mode_hoard());

	if (Game_mode & GM_TEAM)
	{
		if (get_team(pnum)==get_team(Player_num))
			digi_play_sample (SOUND_FRIEND_GOT_ORB,F1_0*2);
		else
			digi_play_sample (SOUND_OPPONENT_GOT_ORB,F1_0*2);
    }
	else
		digi_play_sample (SOUND_OPPONENT_GOT_ORB,F1_0*2);

	Players[(int)pnum].flags|=PLAYER_FLAGS_FLAG;
	HUD_init_message(HM_MULTI, "%s picked up an orb!",static_cast<const char *>(Players[pnum].callsign));
}


static void DropOrb ()
{
	int seed;

	if (!game_mode_hoard())
		Int3(); // How did we get here? Get Leighton!

	if (!Players[Player_num].secondary_ammo[PROXIMITY_INDEX])
	{
		HUD_init_message_literal(HM_MULTI, "No orbs to drop!");
		return;
	}

	seed = d_rand();

	auto objnum = spit_powerup(ConsoleObject,POW_HOARD_ORB,seed);

	if (objnum == object_none)
		return;

	HUD_init_message_literal(HM_MULTI, "Orb dropped!");
	digi_play_sample (SOUND_DROP_WEAPON,F1_0);

	if (game_mode_hoard())
		multi_send_drop_flag(objnum,seed);

	Players[Player_num].secondary_ammo[PROXIMITY_INDEX]--;

	// If empty, tell everyone to stop drawing the box around me
	if (Players[Player_num].secondary_ammo[PROXIMITY_INDEX]==0)
	{
		Players[Player_num].flags &=~(PLAYER_FLAGS_FLAG);
		multi_send_flags (Player_num);
	}
}

void DropFlag ()
{
	int seed;

	if (!game_mode_capture_flag() && !game_mode_hoard())
		return;
	if (game_mode_hoard())
	{
		DropOrb();
		return;
	}

	if (!(Players[Player_num].flags & PLAYER_FLAGS_FLAG))
	{
		HUD_init_message_literal(HM_MULTI, "No flag to drop!");
		return;
	}
	seed = d_rand();
	const auto &&objnum = spit_powerup(ConsoleObject, get_team(Player_num) == TEAM_RED ? POW_FLAG_BLUE : POW_FLAG_RED, seed);
	if (objnum == object_none)
	{
		HUD_init_message_literal(HM_MULTI, "Failed to drop flag!");
		return;
	}

	HUD_init_message_literal(HM_MULTI, "Flag dropped!");
	digi_play_sample (SOUND_DROP_WEAPON,F1_0);

	if (game_mode_capture_flag())
		multi_send_drop_flag(objnum,seed);

	Players[Player_num].flags &=~(PLAYER_FLAGS_FLAG);
}


void multi_send_drop_flag(objnum_t objnum,int seed)
{
	int count=0;
	auto objp = &Objects[objnum];
	count++;
	multibuf[count++]=(char)get_powerup_id(objp);

	PUT_INTEL_SHORT(multibuf+count, objnum); count += 2;
	PUT_INTEL_INT(multibuf+count, seed);

	map_objnum_local_to_local(objnum);

	if (!game_mode_hoard())
		if (Game_mode & GM_NETWORK)
			PowerupCaps.inc_mapped_powerup_current(get_powerup_id(objp));

	multi_send_data<MULTI_DROP_FLAG>(multibuf, 8, 2);
}

static void multi_do_drop_flag (const playernum_t pnum, const ubyte *buf)
{
	int remote_objnum,seed;
	const auto powerup_id = static_cast<powerup_type_t>(buf[1]);
	remote_objnum = GET_INTEL_SHORT(buf + 2);
	seed = GET_INTEL_INT(buf + 6);

	auto objp = &Objects[Players[pnum].objnum];

	auto objnum = spit_powerup(objp, powerup_id, seed);

	map_objnum_local_to_remote(objnum, remote_objnum, pnum);
	if (!game_mode_hoard())
	{
		if (Game_mode & GM_NETWORK)
			PowerupCaps.inc_mapped_powerup_current(powerup_id);
		Players[pnum].flags &= ~(PLAYER_FLAGS_FLAG);
	}
}
#endif

template <powerup_cap_state::direction d>
void powerup_cap_state::modify_count(powerup_count_type &c, const powerup_count_type amount)
{
	if (d == direction::plus)
	{
		const unsigned n = c + amount;
		const auto cn = static_cast<powerup_count_type>(n);
		c = likely(cn == n) ? cn : ~0;
	}
	else
		c = likely(c >= amount) ? c - amount : 0;
}

template <powerup_cap_state::which w, powerup_cap_state::direction d>
void powerup_cap_state::modify_counts(const powerup_type_t id, const powerup_count_type amount)
{
	if (w != which::max)
		modify_count<d>(m_powerups[id], amount);
	if (w != which::current)
		modify_count<d>(m_max[id], amount);
}

template <powerup_cap_state::which w, powerup_cap_state::direction d, powerup_cap_state::mapped m>
void powerup_cap_state::modify_counts(const powerup_type_t id)
{
	if (m == mapped::yes || !powerup_is_4pack(id))
		modify_counts<w, d>(id, 1);
	else
		modify_counts<w, d>(map_powerup_4pack(id), 4);
}

void powerup_cap_state::add_mapped_powerup_current(const powerup_type_t id, const uint_fast32_t amount)
{
	modify_counts<which::current, direction::plus>(id, amount);
}

void powerup_cap_state::inc_powerup_current(const powerup_type_t id)
{
	modify_counts<which::current, direction::plus, mapped::no>(id);
}

void powerup_cap_state::dec_powerup_current(const powerup_type_t id)
{
	modify_counts<which::current, direction::minus, mapped::no>(id);
}

void powerup_cap_state::add_mapped_powerup_both(const powerup_type_t id, uint_fast32_t amount)
{
	modify_counts<which::both, direction::plus>(id, amount);
}

void powerup_cap_state::add_mapped_powerup_max(const powerup_type_t id, uint_fast32_t amount)
{
	modify_counts<which::max, direction::plus>(id, amount);
}

void powerup_cap_state::inc_mapped_powerup_current(const powerup_type_t id)
{
	modify_counts<which::current, direction::plus, mapped::yes>(id);
}

void powerup_cap_state::inc_mapped_powerup_max(const powerup_type_t id)
{
	modify_counts<which::max, direction::plus, mapped::yes>(id);
}

void powerup_cap_state::inc_mapped_powerup_both(const powerup_type_t id)
{
	modify_counts<which::both, direction::plus, mapped::yes>(id);
}

void powerup_cap_state::inc_powerup_both(const powerup_type_t id)
{
	modify_counts<which::both, direction::plus, mapped::no>(id);
}

void powerup_cap_state::inc_powerup_max(const powerup_type_t id)
{
	modify_counts<which::max, direction::plus, mapped::no>(id);
}

static const int PowerupAdjustMapping[]={11,19
#if defined(DXX_BUILD_DESCENT_II)
	,39,41,44
#endif
};

bool powerup_cap_state::powerup_is_4pack(const powerup_type_t id)
{
	const auto e = end(PowerupAdjustMapping);
	return std::find(begin(PowerupAdjustMapping), e, id) != e;
}

powerup_type_t powerup_cap_state::map_powerup_4pack(const powerup_type_t id)
{
	assert(powerup_is_4pack(id));
	return static_cast<powerup_type_t>(id - 1);
}

powerup_type_t powerup_cap_state::map_powerup_type_to_index(const powerup_type_t id)
{
	if (powerup_is_4pack(id))
		return map_powerup_4pack(id);
	return id;
}

uint_fast32_t multi_powerup_is_allowed(const unsigned id, const unsigned AllowedItems)
{
	return multi_powerup_is_allowed(id, AllowedItems, map_granted_flags_to_netflag(Netgame.SpawnGrantedItems));
}

uint_fast32_t multi_powerup_is_allowed(const unsigned id, const unsigned BaseAllowedItems, const unsigned SpawnGrantedItems)
{
	const auto AllowedItems = BaseAllowedItems & ~SpawnGrantedItems;
	switch (id)
	{
		case POW_KEY_BLUE:
		case POW_KEY_GOLD:
		case POW_KEY_RED:
			return Game_mode & GM_MULTI_COOP;
		case POW_INVULNERABILITY:
			return AllowedItems & NETFLAG_DOINVUL;
		case POW_CLOAK:
			return AllowedItems & NETFLAG_DOCLOAK;
		case POW_LASER:
			if (map_granted_flags_to_laser_level(Netgame.SpawnGrantedItems) >= MAX_LASER_LEVEL)
				return 0;
			return AllowedItems & NETFLAG_DOLASER;
		case POW_QUAD_FIRE:
			return AllowedItems & NETFLAG_DOQUAD;
		case POW_VULCAN_WEAPON:
			return AllowedItems & NETFLAG_DOVULCAN;
		case POW_SPREADFIRE_WEAPON:
			return AllowedItems & NETFLAG_DOSPREAD;
		case POW_PLASMA_WEAPON:
			return AllowedItems & NETFLAG_DOPLASMA;
		case POW_FUSION_WEAPON:
			return AllowedItems & NETFLAG_DOFUSION;
		case POW_HOMING_AMMO_1:
		case POW_HOMING_AMMO_4:
			return AllowedItems & NETFLAG_DOHOMING;
		case POW_PROXIMITY_WEAPON:
			// Special: Make all proximity bombs into shields if in
			// hoard mode because we use the proximity slot in the
			// player struct to signify how many orbs the player has.
			if (game_mode_hoard())
				return 0;
			return AllowedItems & NETFLAG_DOPROXIM;
		case POW_SMARTBOMB_WEAPON:
			return AllowedItems & NETFLAG_DOSMART;
		case POW_MEGA_WEAPON:
			return AllowedItems & NETFLAG_DOMEGA;
		case POW_VULCAN_AMMO:
#if defined(DXX_BUILD_DESCENT_I)
			return BaseAllowedItems & NETFLAG_DOVULCAN;
#elif defined(DXX_BUILD_DESCENT_II)
			return BaseAllowedItems & (NETFLAG_DOVULCAN | NETFLAG_DOGAUSS);
#endif
#if defined(DXX_BUILD_DESCENT_II)
		case POW_SUPER_LASER:
			if (map_granted_flags_to_laser_level(Netgame.SpawnGrantedItems) >= MAX_SUPER_LASER_LEVEL)
				return 0;
			return AllowedItems & NETFLAG_DOSUPERLASER;
		case POW_GAUSS_WEAPON:
			return AllowedItems & NETFLAG_DOGAUSS;
		case POW_HELIX_WEAPON:
			return AllowedItems & NETFLAG_DOHELIX;
		case POW_PHOENIX_WEAPON:
			return AllowedItems & NETFLAG_DOPHOENIX;
		case POW_OMEGA_WEAPON:
			return AllowedItems & NETFLAG_DOOMEGA;
		case POW_SMISSILE1_1:
		case POW_SMISSILE1_4:
			return AllowedItems & NETFLAG_DOFLASH;
		case POW_GUIDED_MISSILE_1:
		case POW_GUIDED_MISSILE_4:
			return AllowedItems & NETFLAG_DOGUIDED;
		case POW_SMART_MINE:
			return AllowedItems & NETFLAG_DOSMARTMINE;
		case POW_MERCURY_MISSILE_1:
		case POW_MERCURY_MISSILE_4:
			return AllowedItems & NETFLAG_DOMERCURY;
		case POW_EARTHSHAKER_MISSILE:
			return AllowedItems & NETFLAG_DOSHAKER;
		case POW_AFTERBURNER:
			return AllowedItems & NETFLAG_DOAFTERBURNER;
		case POW_CONVERTER:
			return AllowedItems & NETFLAG_DOCONVERTER;
		case POW_AMMO_RACK:
			return AllowedItems & NETFLAG_DOAMMORACK;
		case POW_HEADLIGHT:
			return AllowedItems & NETFLAG_DOHEADLIGHT;
		case POW_FLAG_BLUE:
		case POW_FLAG_RED:
			return game_mode_capture_flag();
#endif
		default:
			return 1;
	}
}

#if defined(DXX_BUILD_DESCENT_II)
void multi_send_finish_game ()
{
	multibuf[1]=Player_num;

	multi_send_data<MULTI_FINISH_GAME>(multibuf,2,2);
}

static void multi_do_finish_game (const ubyte *buf)
{
	if (buf[0]!=MULTI_FINISH_GAME)
		return;

	if (Current_level_num!=Last_level)
		return;

	do_final_boss_hacks();
}

void multi_send_trigger_specific (const playernum_t pnum,char trig)
{
	multibuf[1] = trig;

	multi_send_data_direct<MULTI_START_TRIGGER>(multibuf, 2, pnum, 2);
}
static void multi_do_start_trigger (const ubyte *buf)
{
	Triggers[(int)buf[1]].flags |=TF_DISABLED;
}
#endif

void multi_add_lifetime_kills ()
{
	// This function adds a kill to lifetime stats of this player, and possibly
	// gives a promotion.  If so, it will tell everyone else

	int oldrank;

	if (!(Game_mode & GM_NETWORK))
		return;

	oldrank=GetMyNetRanking();

	PlayerCfg.NetlifeKills++;

	if (oldrank!=GetMyNetRanking())
	{
		multi_send_ranking();
		if (!PlayerCfg.NoRankings)
		{
			HUD_init_message(HM_MULTI, "You have been promoted to %s!",RankStrings[GetMyNetRanking()]);
#if defined(DXX_BUILD_DESCENT_I)
			digi_play_sample (SOUND_CONTROL_CENTER_WARNING_SIREN,F1_0*2);
#elif defined(DXX_BUILD_DESCENT_II)
			digi_play_sample (SOUND_BUDDY_MET_GOAL,F1_0*2);
#endif
			Netgame.players[Player_num].rank=GetMyNetRanking();
		}
	}
}

void multi_add_lifetime_killed ()
{
	// This function adds a "killed" to lifetime stats of this player, and possibly
	// gives a demotion.  If so, it will tell everyone else

	int oldrank;

	if (!(Game_mode & GM_NETWORK))
		return;

	oldrank=GetMyNetRanking();

	PlayerCfg.NetlifeKilled++;

	if (oldrank!=GetMyNetRanking())
	{
		multi_send_ranking();
		Netgame.players[Player_num].rank=GetMyNetRanking();

		if (!PlayerCfg.NoRankings)
			HUD_init_message(HM_MULTI, "You have been demoted to %s!",RankStrings[GetMyNetRanking()]);

	}
}

void multi_send_ranking ()
{
	multibuf[1]=(char)Player_num;
	multibuf[2]=(char)GetMyNetRanking();

	multi_send_data<MULTI_RANK>(multibuf,3,2);
}

#if defined(DXX_BUILD_DESCENT_II)
static void multi_do_ranking (const playernum_t pnum, const ubyte *buf)
{
	char rankstr[20];
	char rank=buf[2];

	if (Netgame.players[(int)pnum].rank<rank)
		strcpy (rankstr,"promoted");
	else if (Netgame.players[(int)pnum].rank>rank)
		strcpy (rankstr,"demoted");
	else
		return;

	Netgame.players[(int)pnum].rank=rank;

	if (!PlayerCfg.NoRankings)
		HUD_init_message(HM_MULTI, "%s has been %s to %s!",static_cast<const char *>(Players[pnum].callsign),rankstr,RankStrings[(int)rank]);
}
#endif

// Decide if fire from "killer" is friendly. If yes return 1 (no harm to me) otherwise 0 (damage me)
int multi_maybe_disable_friendly_fire(const cobjptridx_t killer)
{
	if (!(Game_mode & GM_NETWORK)) // no Multiplayer game -> always harm me!
		return 0;
	if (!Netgame.NoFriendlyFire) // friendly fire is activated -> harm me!
		return 0;
	if (killer == object_none) // no actual killer -> harm me!
		return 0;
	if (killer->type != OBJ_PLAYER) // not a player -> harm me!
		return 0;
	if (Game_mode & GM_MULTI_COOP) // coop mode -> don't harm me!
		return 1;
	else if (Game_mode & GM_TEAM) // team mode - find out if killer is in my team
	{
		if (get_team(Player_num) == get_team(get_player_id(killer))) // in my team -> don't harm me!
			return 1;
		else // opposite team -> harm me!
			return 0;
	}
	return 0; // all other cases -> harm me!
}

/* Bounty packer sender and handler */
void multi_send_bounty( void )
{
	/* Test game mode */
	if( !( Game_mode & GM_BOUNTY ) )
		return;
	if ( !multi_i_am_master() )
		return;
	
	/* Add opcode, target ID and how often we re-assigned */
	multibuf[1] = (char)Bounty_target;
	
	/* Send data */
	multi_send_data<MULTI_DO_BOUNTY>( multibuf, 2, 2 );
}

static void multi_do_bounty( const ubyte *buf )
{
	if ( multi_i_am_master() )
		return;
	
	multi_new_bounty_target( buf[1] );
}

void multi_new_bounty_target(const playernum_t pnum )
{
	/* If it's already the same, don't do it */
	if( Bounty_target == pnum )
		return;
	
	/* Set the target */
	Bounty_target = pnum;
	
	/* Send a message */
	HUD_init_message( HM_MULTI, "%c%c%s is the new target!", CC_COLOR,
		BM_XRGB( player_rgb[Bounty_target].r, player_rgb[Bounty_target].g, player_rgb[Bounty_target].b ),
		static_cast<const char *>(Players[Bounty_target].callsign));

#if defined(DXX_BUILD_DESCENT_I)
	digi_play_sample( SOUND_CONTROL_CENTER_WARNING_SIREN, F1_0 * 3 );
#elif defined(DXX_BUILD_DESCENT_II)
	digi_play_sample( SOUND_BUDDY_MET_GOAL, F1_0 * 2 );
#endif
}

static void multi_do_save_game(const ubyte *buf)
{
	int count = 1;
	ubyte slot;
	uint id;
	char desc[25];

	slot = buf[count];			count += 1;
	id = GET_INTEL_INT(buf+count);			count += 4;
	memcpy( desc, &buf[count], 20 );		count += 20;

	multi_save_game( slot, id, desc );
}

static void multi_do_restore_game(const ubyte *buf)
{
	int count = 1;
	ubyte slot;
	uint id;

	slot = buf[count];			count += 1;
	id = GET_INTEL_INT(buf+count);			count += 4;

	multi_restore_game( slot, id );
}

static void multi_send_save_game(ubyte slot, uint id, char * desc)
{
	int count = 0;
	
	count += 1;
	multibuf[count] = slot;				count += 1; // Save slot=0
	PUT_INTEL_INT( multibuf+count, id );		count += 4; // Save id
	memcpy( &multibuf[count], desc, 20 );		count += 20;

	multi_send_data<MULTI_SAVE_GAME>(multibuf, count, 2);
}

static void multi_send_restore_game(ubyte slot, uint id)
{
	int count = 0;
	
	count += 1;
	multibuf[count] = slot;				count += 1; // Save slot=0
	PUT_INTEL_INT( multibuf+count, id );		count += 4; // Save id

	multi_send_data<MULTI_RESTORE_GAME>(multibuf, count, 2);
}

void multi_initiate_save_game()
{
	fix game_id = 0;
	int slot;
	char filename[PATH_MAX];
	char desc[24];

	if ((Network_status == NETSTAT_ENDLEVEL) || (Control_center_destroyed))
		return;

	if (!multi_i_am_master())
	{
		HUD_init_message_literal(HM_MULTI, "Only host is allowed to save a game!");
		return;
	}
	if (!multi_all_players_alive())
	{
		HUD_init_message_literal(HM_MULTI, "Can't save! All players must be alive and playing!");
		return;
	}
	{
		auto range = partial_range(Players, N_players);
	for (auto i = range.begin(); i != range.end(); ++i)
	{
		for (auto j = i + 1; j != range.end(); ++j)
		{
			if (i->callsign == j->callsign)
			{
				HUD_init_message_literal(HM_MULTI, "Can't save! Multiple players with same callsign!");
				return;
			}
		}
	}
	}

	memset(&filename, '\0', PATH_MAX);
	memset(&desc, '\0', 24);
	slot = state_get_save_file(filename, desc, blind_save::no);
	if (!slot)
		return;
	slot--;

	// Make a unique game id
	game_id = ((fix)timer_query());
	game_id ^= N_players<<4;
	range_for (auto &i, partial_range(Players, N_players))
	{
		fix call2i;
		memcpy(&call2i, static_cast<const char *>(i.callsign), sizeof(fix));
		game_id ^= call2i;
	}
	if ( game_id == 0 )
		game_id = 1; // 0 is invalid

	// Execute "alive" and "duplicate callsign" checks again in case things changed while host decided upon the savegame.
	if (!multi_all_players_alive())
	{
		HUD_init_message_literal(HM_MULTI, "Can't save! All players must be alive and playing!");
		return;
	}
	{
		auto range = partial_range(Players, N_players);
	for (auto i = range.begin(); i != range.end(); ++i)
	{
		for (auto j = i + 1; j != range.end(); ++j)
		{
			if (i->callsign == j->callsign)
			{
				HUD_init_message_literal(HM_MULTI, "Can't save! Multiple players with same callsign!");
				return;
			}
		}
	}
	}

	multi_send_save_game( slot, game_id, desc );
	multi_do_frame();
	multi_save_game( slot,game_id, desc );
}

void multi_initiate_restore_game()
{
	int slot;
	char filename[PATH_MAX];

	if ((Network_status == NETSTAT_ENDLEVEL) || (Control_center_destroyed))
		return;

	if (!multi_i_am_master())
	{
		HUD_init_message_literal(HM_MULTI, "Only host is allowed to load a game!");
		return;
	}
	if (!multi_all_players_alive())
	{
		HUD_init_message_literal(HM_MULTI, "Can't load! All players must be alive and playing!");
		return;
	}
	{
		auto range = partial_range(Players, N_players);
	for (auto i = range.begin(); i != range.end(); ++i)
	{
		for (auto j = i + 1; j != range.end(); ++j)
		{
			if (i->callsign == j->callsign)
			{
				HUD_init_message_literal(HM_MULTI, "Can't load! Multiple players with same callsign!");
				return;
			}
		}
	}
	}
	slot = state_get_restore_file(filename, blind_save::no);
	if (!slot)
		return;
	state_game_id = state_get_game_id(filename);
	if (!state_game_id)
		return;
	slot--;
	multi_send_restore_game(slot,state_game_id);
	multi_do_frame();
	multi_restore_game(slot,state_game_id);
}

void multi_save_game(ubyte slot, uint id, char *desc)
{
	char filename[PATH_MAX];

	if ((Network_status == NETSTAT_ENDLEVEL) || (Control_center_destroyed))
		return;

	snprintf(filename, sizeof(filename), PLAYER_DIRECTORY_STRING("%s.mg%d"), static_cast<const char *>(Players[Player_num].callsign), slot);
	HUD_init_message(HM_MULTI,  "Saving game #%d, '%s'", slot, desc);
	stop_time();
	state_game_id = id;
	state_save_all_sub(filename, desc );
}

void multi_restore_game(ubyte slot, uint id)
{
	char filename[PATH_MAX];
	int i;
	int thisid;

	if ((Network_status == NETSTAT_ENDLEVEL) || (Control_center_destroyed))
		return;

	snprintf(filename, sizeof(filename), PLAYER_DIRECTORY_STRING("%s.mg%d"), static_cast<const char *>(Players[Player_num].callsign), slot);
   
	for (i = 0; i < N_players; i++)
		multi_strip_robots(i);
	if (multi_i_am_master()) // put all players to wait-state again so we can sync up properly
		range_for (auto &i, Players)
			if (i.connected == CONNECT_PLAYING && &i != &Players[Player_num])
				i.connected = CONNECT_WAITING;
   
	thisid=state_get_game_id(filename);
	if (thisid!=id)
	{
		nm_messagebox(NULL, 1, TXT_OK, "A multi-save game was restored\nthat you are missing or does not\nmatch that of the others.\nYou must rejoin if you wish to\ncontinue.");
		return;
	}
  
	state_restore_all_sub(filename, secret_restore::none);
	multi_send_score(); // send my restored scores. I sent 0 when I loaded the level anyways...
}

static void multi_do_msgsend_state(const ubyte *buf)
{
	multi_sending_message[(int)buf[1]] = (msgsend_state_t)buf[2];
}

void multi_send_msgsend_state(msgsend_state_t state)
{
	multibuf[1] = Player_num;
	multibuf[2] = (char)state;
	
	multi_send_data<MULTI_TYPING_STATE>(multibuf, 3, 2);
}

// Specific variables related to our game mode we want the clients to know about
void multi_send_gmode_update()
{
	if (!multi_i_am_master())
		return;
	if (!(Game_mode & GM_TEAM || Game_mode & GM_BOUNTY)) // expand if necessary
		return;
	multibuf[1] = Netgame.team_vector;
	multibuf[2] = Bounty_target;
	
	multi_send_data<MULTI_GMODE_UPDATE>(multibuf, 3, 0);
}

static void multi_do_gmode_update(const ubyte *buf)
{
	if (multi_i_am_master())
		return;
	if (Game_mode & GM_TEAM)
	{
		if (buf[1] != Netgame.team_vector)
		{
			Netgame.team_vector = buf[1];
			range_for (auto &t, partial_range(Players, N_players))
				if (t.connected)
					multi_reset_object_texture (&Objects[t.objnum]);
			reset_cockpit();
		}
	}
	if (Game_mode & GM_BOUNTY)
	{
		Bounty_target = buf[2]; // accept silently - message about change we SHOULD have gotten due to kill computation
	}
}

#if defined(DXX_BUILD_DESCENT_II)
///
/// CODE TO LOAD HOARD DATA
///

namespace {

class hoard_resources_type
{
	static constexpr auto invalid_bm_idx = tt::integral_constant<int, -1>{};
	static constexpr auto invalid_snd_idx = tt::integral_constant<unsigned, ~0u>{};
public:
	int bm_idx;
	unsigned snd_idx;
	void reset();
	constexpr hoard_resources_type() :
		bm_idx(invalid_bm_idx), snd_idx(invalid_snd_idx)
	{
	}
	~hoard_resources_type()
	{
		reset();
	}
};

constexpr tt::integral_constant<int, -1> hoard_resources_type::invalid_bm_idx;
constexpr tt::integral_constant<unsigned, ~0u> hoard_resources_type::invalid_snd_idx;

}

static hoard_resources_type hoard_resources;

int HoardEquipped()
{
	static int checked=-1;

	if (unlikely(checked == -1))
	{
		checked = PHYSFSX_exists("hoard.ham",1);
	}
	return (checked);
}

array<grs_main_bitmap, 2> Orb_icons;

void hoard_resources_type::reset()
{
	if (bm_idx != invalid_bm_idx)
		d_free(GameBitmaps[exchange(bm_idx, invalid_bm_idx)].bm_mdata);
	if (snd_idx != invalid_snd_idx)
	{
		const auto idx = exchange(snd_idx, invalid_snd_idx);
		range_for (auto &i, partial_range(GameSounds, idx, idx + 4))
			d_free(i.data);
	}
	range_for (auto &i, Orb_icons)
		i.reset();
}

void init_hoard_data()
{
	hoard_resources.reset();
	static int orb_vclip;
	unsigned n_orb_frames,n_goal_frames;
	int orb_w,orb_h;
	int icon_w,icon_h;
	palette_array_t palette;
	ubyte *bitmap_data1;
	int i,save_pos;
	int bitmap_num = hoard_resources.bm_idx = Num_bitmap_files;

	auto ifile = PHYSFSX_openReadBuffered("hoard.ham");
	if (!ifile)
		Error("can't open <hoard.ham>");

	n_orb_frames = PHYSFSX_readShort(ifile);
	orb_w = PHYSFSX_readShort(ifile);
	orb_h = PHYSFSX_readShort(ifile);
	save_pos = PHYSFS_tell(ifile);
	PHYSFSX_fseek(ifile,sizeof(palette)+n_orb_frames*orb_w*orb_h,SEEK_CUR);
	n_goal_frames = PHYSFSX_readShort(ifile);
	PHYSFSX_fseek(ifile,save_pos,SEEK_SET);

	//Allocate memory for bitmaps
	MALLOC( bitmap_data1, ubyte, n_orb_frames*orb_w*orb_h + n_goal_frames*64*64 );

	//Create orb vclip
	orb_vclip = Num_vclips++;
	Assert(Num_vclips <= VCLIP_MAXNUM);
	Vclip[orb_vclip].play_time = F1_0/2;
	Vclip[orb_vclip].num_frames = n_orb_frames;
	Vclip[orb_vclip].frame_time = Vclip[orb_vclip].play_time / Vclip[orb_vclip].num_frames;
	Vclip[orb_vclip].flags = 0;
	Vclip[orb_vclip].sound_num = -1;
	Vclip[orb_vclip].light_value = F1_0;
	range_for (auto &i, partial_range(Vclip[orb_vclip].frames, n_orb_frames))
	{
		i.index = bitmap_num;
		gr_init_bitmap(GameBitmaps[bitmap_num],BM_LINEAR,0,0,orb_w,orb_h,orb_w,bitmap_data1);
		gr_set_transparent(GameBitmaps[bitmap_num], 1);
		bitmap_data1 += orb_w*orb_h;
		bitmap_num++;
		Assert(bitmap_num < MAX_BITMAP_FILES);
	}

	//Create obj powerup
	Powerup_info[POW_HOARD_ORB].vclip_num = orb_vclip;
	Powerup_info[POW_HOARD_ORB].hit_sound = -1; //Powerup_info[POW_SHIELD_BOOST].hit_sound;
	Powerup_info[POW_HOARD_ORB].size = Powerup_info[POW_SHIELD_BOOST].size;
	Powerup_info[POW_HOARD_ORB].light = Powerup_info[POW_SHIELD_BOOST].light;

	//Create orb goal wall effect
	const auto goal_eclip = Num_effects++;
	Assert(Num_effects < MAX_EFFECTS);
	Effects[goal_eclip] = Effects[94];        //copy from blue goal
	Effects[goal_eclip].changing_wall_texture = NumTextures;
	Effects[goal_eclip].vc.num_frames=n_goal_frames;

	TmapInfo[NumTextures] = find_required_goal_texture(TMI_GOAL_BLUE);
	TmapInfo[NumTextures].eclip_num = goal_eclip;
	TmapInfo[NumTextures].flags = TMI_GOAL_HOARD;
	NumTextures++;
	Assert(NumTextures < MAX_TEXTURES);
	range_for (auto &i, partial_range(Effects[goal_eclip].vc.frames, n_goal_frames))
	{
		i.index = bitmap_num;
		gr_init_bitmap(GameBitmaps[bitmap_num],BM_LINEAR,0,0,64,64,64,bitmap_data1);
		bitmap_data1 += 64*64;
		bitmap_num++;
		Assert(bitmap_num < MAX_BITMAP_FILES);
	}

	//Load and remap bitmap data for orb
	PHYSFS_read(ifile,&palette[0],sizeof(palette[0]),palette.size());
	range_for (auto &i, partial_range(Vclip[orb_vclip].frames, n_orb_frames))
	{
		grs_bitmap *bm = &GameBitmaps[i.index];
		PHYSFS_read(ifile,bm->get_bitmap_data(),1,orb_w*orb_h);
		gr_remap_bitmap_good(*bm, palette, 255, -1);
	}

	//Load and remap bitmap data for goal texture
	PHYSFSX_readShort(ifile);        //skip frame count
	PHYSFS_read(ifile,&palette[0],sizeof(palette[0]),palette.size());
	range_for (auto &i, partial_range(Effects[goal_eclip].vc.frames, n_goal_frames))
	{
		grs_bitmap *bm = &GameBitmaps[i.index];
		PHYSFS_read(ifile,bm->get_bitmap_data(),1,64*64);
		gr_remap_bitmap_good(*bm, palette, 255, -1);
	}

	//Load and remap bitmap data for HUD icons
	range_for (auto &i, Orb_icons)
	{
		ubyte *bitmap_data2;
		icon_w = PHYSFSX_readShort(ifile);
		icon_h = PHYSFSX_readShort(ifile);
		MALLOC( bitmap_data2, ubyte, icon_w*icon_h );
		gr_init_bitmap(i,BM_LINEAR,0,0,icon_w,icon_h,icon_w,bitmap_data2);
		gr_set_transparent(i, 1);
		PHYSFS_read(ifile,&palette[0],sizeof(palette[0]),palette.size());
		PHYSFS_read(ifile,i.get_bitmap_data(),1,icon_w*icon_h);
		gr_remap_bitmap_good(i, palette, 255, -1);
	}

	//Load sounds for orb game
	hoard_resources.snd_idx = Num_sound_files;
	for (i=0;i<4;i++) {
		int len;

		len = PHYSFSX_readInt(ifile);        //get 11k len

		if (GameArg.SndDigiSampleRate == SAMPLE_RATE_22K) {
			PHYSFSX_fseek(ifile,len,SEEK_CUR);     //skip over 11k sample
			len = PHYSFSX_readInt(ifile);    //get 22k len
		}

		GameSounds[Num_sound_files+i].length = len;
		MALLOC(GameSounds[Num_sound_files+i].data, ubyte, len);
		PHYSFS_read(ifile,GameSounds[Num_sound_files+i].data,1,len);

		if (GameArg.SndDigiSampleRate == SAMPLE_RATE_11K) {
			len = PHYSFSX_readInt(ifile);    //get 22k len
			PHYSFSX_fseek(ifile,len,SEEK_CUR);     //skip over 22k sample
		}

		Sounds[SOUND_YOU_GOT_ORB+i] = Num_sound_files+i;
		AltSounds[SOUND_YOU_GOT_ORB+i] = Sounds[SOUND_YOU_GOT_ORB+i];
	}
}

#ifdef EDITOR
void save_hoard_data(void)
{
	grs_bitmap icon;
	unsigned nframes;
	palette_array_t palette;
	int iff_error;
	unsigned i;
	static const char sounds[][13] = {"selforb.raw","selforb.r22",          //SOUND_YOU_GOT_ORB
				"teamorb.raw","teamorb.r22",    //SOUND_FRIEND_GOT_ORB
				"enemyorb.raw","enemyorb.r22",  //SOUND_OPPONENT_GOT_ORB
				"OPSCORE1.raw","OPSCORE1.r22"}; //SOUND_OPPONENT_HAS_SCORED
		
	auto ofile = PHYSFSX_openWriteBuffered("hoard.ham");

	array<std::unique_ptr<grs_main_bitmap>, MAX_BITMAPS_PER_BRUSH> bm;
	iff_error = iff_read_animbrush("orb.abm",bm,&nframes,palette);
	Assert(iff_error == IFF_NO_ERROR);
	PHYSFS_writeULE16(ofile, nframes);
	PHYSFS_writeULE16(ofile, bm[0]->bm_w);
	PHYSFS_writeULE16(ofile, bm[0]->bm_h);
	PHYSFS_write(ofile, &palette[0], sizeof(palette[0]), palette.size());
	range_for (auto &i, partial_range(bm, nframes))
		PHYSFS_write(ofile, i->bm_data, i->bm_w * i->bm_h, 1);

	iff_error = iff_read_animbrush("orbgoal.abm",bm,&nframes,palette);
	Assert(iff_error == IFF_NO_ERROR);
	Assert(bm[0]->bm_w == 64 && bm[0]->bm_h == 64);
	PHYSFS_writeULE16(ofile, nframes);
	PHYSFS_write(ofile, &palette[0], sizeof(palette[0]), palette.size());
	range_for (auto &i, partial_range(bm, nframes))
		PHYSFS_write(ofile, i->bm_data, i->bm_w * i->bm_h, 1);

	for (i=0;i<2;i++)
	{
		iff_error = iff_read_bitmap(i?"orbb.bbm":"orb.bbm",icon,BM_LINEAR,&palette);
		Assert(iff_error == IFF_NO_ERROR);
		PHYSFS_writeULE16(ofile, icon.bm_w);
		PHYSFS_writeULE16(ofile, icon.bm_h);
		PHYSFS_write(ofile, &palette[0], sizeof(palette[0]), palette.size());
		PHYSFS_write(ofile, icon.bm_data, icon.bm_w*icon.bm_h, 1);
	}
	(void)iff_error;
		
	range_for (auto &i, sounds)
		if (RAIIPHYSFS_File ifile{PHYSFS_openRead(i)})
	{
		int size;
		size = PHYSFS_fileLength(ifile);
		RAIIdmem<uint8_t[]> buf;
		MALLOC(buf, uint8_t[], size);
		PHYSFS_read(ifile, buf, size, 1);
		PHYSFS_writeULE32(ofile, size);
		PHYSFS_write(ofile, buf, size, 1);
	}
}
#endif
#endif

static void multi_process_data(const playernum_t pnum, const ubyte *buf, const uint_fast32_t type)
{
	// Take an entire message (that has already been checked for validity,
	// if necessary) and act on it.
	switch(type)
	{
		case MULTI_POSITION:
			multi_do_position(pnum, buf); break;
		case MULTI_REAPPEAR:
			multi_do_reappear(pnum, buf); break;
		case MULTI_FIRE:
		case MULTI_FIRE_TRACK:
		case MULTI_FIRE_BOMB:
			multi_do_fire(pnum, buf); break;
		case MULTI_KILL:
			multi_do_kill(pnum, buf); break;
		case MULTI_REMOVE_OBJECT:
			multi_do_remobj(buf); break;
		case MULTI_PLAYER_DERES:
			multi_do_player_deres(pnum, buf); break;
		case MULTI_MESSAGE:
			multi_do_message(buf); break;
		case MULTI_QUIT:
			multi_do_quit(buf); break;
		case MULTI_CONTROLCEN:
			multi_do_controlcen_destroy(buf); break;
#if defined(DXX_BUILD_DESCENT_II)
		case MULTI_SOUND_FUNCTION:
			multi_do_sound_function(pnum, buf); break;
		case MULTI_MARKER:
			multi_do_drop_marker (pnum, buf); break;
		case MULTI_DROP_WEAPON:
			multi_do_drop_weapon(pnum, buf); break;
		case MULTI_DROP_FLAG:
			multi_do_drop_flag(pnum, buf); break;
		case MULTI_GUIDED:
			multi_do_guided (pnum, buf); break;
		case MULTI_STOLEN_ITEMS:
			multi_do_stolen_items(buf); break;
		case MULTI_WALL_STATUS:
			multi_do_wall_status(buf); break;
		case MULTI_SEISMIC:
			multi_do_seismic (buf); break;
		case MULTI_LIGHT:
			multi_do_light (buf); break;
#endif
		case MULTI_ENDLEVEL_START:
			multi_do_escape(buf); break;
		case MULTI_CLOAK:
			multi_do_cloak(pnum); break;
		case MULTI_DECLOAK:
			multi_do_decloak(pnum); break;
		case MULTI_DOOR_OPEN:
			multi_do_door_open(buf); break;
		case MULTI_CREATE_EXPLOSION:
			multi_do_create_explosion(pnum); break;
		case MULTI_CONTROLCEN_FIRE:
			multi_do_controlcen_fire(buf); break;
		case MULTI_CREATE_POWERUP:
			multi_do_create_powerup(pnum, buf); break;
		case MULTI_PLAY_SOUND:
			multi_do_play_sound(pnum, buf); break;
#if defined(DXX_BUILD_DESCENT_II)
		case MULTI_CAPTURE_BONUS:
			multi_do_capture_bonus(pnum); break;
		case MULTI_ORB_BONUS:
			multi_do_orb_bonus(pnum, buf); break;
		case MULTI_GOT_FLAG:
			multi_do_got_flag(pnum); break;
		case MULTI_GOT_ORB:
			multi_do_got_orb(pnum); break;
		case MULTI_RANK:
			multi_do_ranking (pnum, buf); break;
		case MULTI_FINISH_GAME:
			multi_do_finish_game(buf); break;  // do this one regardless of endsequence
#endif
		case MULTI_ROBOT_CLAIM:
			multi_do_claim_robot(pnum, buf); break;
		case MULTI_ROBOT_POSITION:
			multi_do_robot_position(pnum, buf); break;
		case MULTI_ROBOT_EXPLODE:
			multi_do_robot_explode(buf); break;
		case MULTI_ROBOT_RELEASE:
			multi_do_release_robot(pnum, buf); break;
		case MULTI_ROBOT_FIRE:
			multi_do_robot_fire(buf); break;
		case MULTI_SCORE:
			multi_do_score(pnum, buf); break;
		case MULTI_CREATE_ROBOT:
			multi_do_create_robot(pnum, buf); break;
		case MULTI_TRIGGER:
			multi_do_trigger(pnum, buf); break;
#if defined(DXX_BUILD_DESCENT_II)
		case MULTI_START_TRIGGER:
			multi_do_start_trigger(buf); break;
		case MULTI_EFFECT_BLOWUP:
			multi_do_effect_blowup(pnum, buf); break;
		case MULTI_FLAGS:
			multi_do_flags(pnum, buf); break;
		case MULTI_DROP_BLOB:
			multi_do_drop_blob(pnum); break;
#endif
		case MULTI_BOSS_TELEPORT:
			multi_do_boss_teleport(pnum, buf); break;
		case MULTI_BOSS_CLOAK:
			multi_do_boss_cloak(buf); break;
		case MULTI_BOSS_START_GATE:
			multi_do_boss_start_gate(buf); break;
		case MULTI_BOSS_STOP_GATE:
			multi_do_boss_stop_gate(buf); break;
		case MULTI_BOSS_CREATE_ROBOT:
			multi_do_boss_create_robot(pnum, buf); break;
		case MULTI_CREATE_ROBOT_POWERUPS:
			multi_do_create_robot_powerups(pnum, buf); break;
		case MULTI_HOSTAGE_DOOR:
			multi_do_hostage_door_status(buf); break;
		case MULTI_SAVE_GAME:
			multi_do_save_game(buf); break;
		case MULTI_RESTORE_GAME:
			multi_do_restore_game(buf); break;
		case MULTI_HEARTBEAT:
			multi_do_heartbeat (buf); break;
		case MULTI_KILLGOALS:
			multi_do_kill_goal_counts (buf); break;
		case MULTI_POWCAP_UPDATE:
			multi_do_powcap_update(buf); break;
		case MULTI_DO_BOUNTY:
			multi_do_bounty( buf ); break;
		case MULTI_TYPING_STATE:
			multi_do_msgsend_state( buf ); break;
		case MULTI_GMODE_UPDATE:
			multi_do_gmode_update( buf ); break;
		case MULTI_KILL_HOST:
			multi_do_kill(pnum, buf); break;
		case MULTI_KILL_CLIENT:
			multi_do_kill(pnum, buf); break;
		default:
			throw std::runtime_error("invalid message type");
	}
}

// Following functions convert object to object_rw and back.
// turn object to object_rw for sending
void multi_object_to_object_rw(const vobjptr_t obj, object_rw *obj_rw)
{
	obj_rw->signature     = obj->signature.get();
	obj_rw->type          = obj->type;
	obj_rw->id            = obj->id;
	obj_rw->next          = obj->next;
	obj_rw->prev          = obj->prev;
	obj_rw->control_type  = obj->control_type;
	obj_rw->movement_type = obj->movement_type;
	obj_rw->render_type   = obj->render_type;
	obj_rw->flags         = obj->flags;
	obj_rw->segnum        = obj->segnum;
	obj_rw->attached_obj  = obj->attached_obj;
	obj_rw->pos.x         = obj->pos.x;
	obj_rw->pos.y         = obj->pos.y;
	obj_rw->pos.z         = obj->pos.z;
	obj_rw->orient.rvec.x = obj->orient.rvec.x;
	obj_rw->orient.rvec.y = obj->orient.rvec.y;
	obj_rw->orient.rvec.z = obj->orient.rvec.z;
	obj_rw->orient.fvec.x = obj->orient.fvec.x;
	obj_rw->orient.fvec.y = obj->orient.fvec.y;
	obj_rw->orient.fvec.z = obj->orient.fvec.z;
	obj_rw->orient.uvec.x = obj->orient.uvec.x;
	obj_rw->orient.uvec.y = obj->orient.uvec.y;
	obj_rw->orient.uvec.z = obj->orient.uvec.z;
	obj_rw->size          = obj->size;
	obj_rw->shields       = obj->shields;
	obj_rw->last_pos.x    = obj->last_pos.x;
	obj_rw->last_pos.y    = obj->last_pos.y;
	obj_rw->last_pos.z    = obj->last_pos.z;
	obj_rw->contains_type = obj->contains_type;
	obj_rw->contains_id   = obj->contains_id;
	obj_rw->contains_count= obj->contains_count;
	obj_rw->matcen_creator= obj->matcen_creator;
	obj_rw->lifeleft      = obj->lifeleft;
	
	switch (obj_rw->movement_type)
	{
		case MT_PHYSICS:
			obj_rw->mtype.phys_info.velocity.x  = obj->mtype.phys_info.velocity.x;
			obj_rw->mtype.phys_info.velocity.y  = obj->mtype.phys_info.velocity.y;
			obj_rw->mtype.phys_info.velocity.z  = obj->mtype.phys_info.velocity.z;
			obj_rw->mtype.phys_info.thrust.x    = obj->mtype.phys_info.thrust.x;
			obj_rw->mtype.phys_info.thrust.y    = obj->mtype.phys_info.thrust.y;
			obj_rw->mtype.phys_info.thrust.z    = obj->mtype.phys_info.thrust.z;
			obj_rw->mtype.phys_info.mass        = obj->mtype.phys_info.mass;
			obj_rw->mtype.phys_info.drag        = obj->mtype.phys_info.drag;
			obj_rw->mtype.phys_info.rotvel.x    = obj->mtype.phys_info.rotvel.x;
			obj_rw->mtype.phys_info.rotvel.y    = obj->mtype.phys_info.rotvel.y;
			obj_rw->mtype.phys_info.rotvel.z    = obj->mtype.phys_info.rotvel.z;
			obj_rw->mtype.phys_info.rotthrust.x = obj->mtype.phys_info.rotthrust.x;
			obj_rw->mtype.phys_info.rotthrust.y = obj->mtype.phys_info.rotthrust.y;
			obj_rw->mtype.phys_info.rotthrust.z = obj->mtype.phys_info.rotthrust.z;
			obj_rw->mtype.phys_info.turnroll    = obj->mtype.phys_info.turnroll;
			obj_rw->mtype.phys_info.flags       = obj->mtype.phys_info.flags;
			break;
			
		case MT_SPINNING:
			obj_rw->mtype.spin_rate.x = obj->mtype.spin_rate.x;
			obj_rw->mtype.spin_rate.y = obj->mtype.spin_rate.y;
			obj_rw->mtype.spin_rate.z = obj->mtype.spin_rate.z;
			break;
	}
	
	switch (obj_rw->control_type)
	{
		case CT_WEAPON:
			obj_rw->ctype.laser_info.parent_type      = obj->ctype.laser_info.parent_type;
			obj_rw->ctype.laser_info.parent_num       = obj->ctype.laser_info.parent_num;
			obj_rw->ctype.laser_info.parent_signature = obj->ctype.laser_info.parent_signature.get();
			if (obj->ctype.laser_info.creation_time - GameTime64 < F1_0*(-18000))
				obj_rw->ctype.laser_info.creation_time = F1_0*(-18000);
			else
				obj_rw->ctype.laser_info.creation_time = obj->ctype.laser_info.creation_time - GameTime64;
			obj_rw->ctype.laser_info.last_hitobj      = obj->ctype.laser_info.last_hitobj;
			obj_rw->ctype.laser_info.track_goal       = obj->ctype.laser_info.track_goal;
			obj_rw->ctype.laser_info.multiplier       = obj->ctype.laser_info.multiplier;
			break;
			
		case CT_EXPLOSION:
			obj_rw->ctype.expl_info.spawn_time    = obj->ctype.expl_info.spawn_time;
			obj_rw->ctype.expl_info.delete_time   = obj->ctype.expl_info.delete_time;
			obj_rw->ctype.expl_info.delete_objnum = obj->ctype.expl_info.delete_objnum;
			obj_rw->ctype.expl_info.attach_parent = obj->ctype.expl_info.attach_parent;
			obj_rw->ctype.expl_info.prev_attach   = obj->ctype.expl_info.prev_attach;
			obj_rw->ctype.expl_info.next_attach   = obj->ctype.expl_info.next_attach;
			break;
			
		case CT_AI:
		{
			int i;
			obj_rw->ctype.ai_info.behavior               = static_cast<uint8_t>(obj->ctype.ai_info.behavior);
			for (i = 0; i < MAX_AI_FLAGS; i++)
				obj_rw->ctype.ai_info.flags[i]       = obj->ctype.ai_info.flags[i]; 
			obj_rw->ctype.ai_info.hide_segment           = obj->ctype.ai_info.hide_segment;
			obj_rw->ctype.ai_info.hide_index             = obj->ctype.ai_info.hide_index;
			obj_rw->ctype.ai_info.path_length            = obj->ctype.ai_info.path_length;
			obj_rw->ctype.ai_info.cur_path_index         = obj->ctype.ai_info.cur_path_index;
			obj_rw->ctype.ai_info.danger_laser_num       = obj->ctype.ai_info.danger_laser_num;
			obj_rw->ctype.ai_info.danger_laser_signature = obj->ctype.ai_info.danger_laser_signature.get();
#if defined(DXX_BUILD_DESCENT_I)
			obj_rw->ctype.ai_info.follow_path_start_seg  = segment_none;
			obj_rw->ctype.ai_info.follow_path_end_seg    = segment_none;
#elif defined(DXX_BUILD_DESCENT_II)
			obj_rw->ctype.ai_info.dying_sound_playing    = obj->ctype.ai_info.dying_sound_playing;
			if (obj->ctype.ai_info.dying_start_time == 0) // if bot not dead, anything but 0 will kill it
				obj_rw->ctype.ai_info.dying_start_time = 0;
			else
				obj_rw->ctype.ai_info.dying_start_time = obj->ctype.ai_info.dying_start_time - GameTime64;
#endif
			break;
		}
			
		case CT_LIGHT:
			obj_rw->ctype.light_info.intensity = obj->ctype.light_info.intensity;
			break;
			
		case CT_POWERUP:
			obj_rw->ctype.powerup_info.count         = obj->ctype.powerup_info.count;
#if defined(DXX_BUILD_DESCENT_II)
			if (obj->ctype.powerup_info.creation_time - GameTime64 < F1_0*(-18000))
				obj_rw->ctype.powerup_info.creation_time = F1_0*(-18000);
			else
				obj_rw->ctype.powerup_info.creation_time = obj->ctype.powerup_info.creation_time - GameTime64;
			obj_rw->ctype.powerup_info.flags         = obj->ctype.powerup_info.flags;
#endif
			break;
	}
	
	switch (obj_rw->render_type)
	{
		case RT_MORPH:
		case RT_POLYOBJ:
		case RT_NONE: // HACK below
		{
			int i;
			if (obj->render_type == RT_NONE && obj->type != OBJ_GHOST) // HACK: when a player is dead or not connected yet, clients still expect to get polyobj data - even if render_type == RT_NONE at this time.
				break;
			obj_rw->rtype.pobj_info.model_num                = obj->rtype.pobj_info.model_num;
			for (i=0;i<MAX_SUBMODELS;i++)
			{
				obj_rw->rtype.pobj_info.anim_angles[i].p = obj->rtype.pobj_info.anim_angles[i].p;
				obj_rw->rtype.pobj_info.anim_angles[i].b = obj->rtype.pobj_info.anim_angles[i].b;
				obj_rw->rtype.pobj_info.anim_angles[i].h = obj->rtype.pobj_info.anim_angles[i].h;
			}
			obj_rw->rtype.pobj_info.subobj_flags             = obj->rtype.pobj_info.subobj_flags;
			obj_rw->rtype.pobj_info.tmap_override            = obj->rtype.pobj_info.tmap_override;
			obj_rw->rtype.pobj_info.alt_textures             = obj->rtype.pobj_info.alt_textures;
			break;
		}
			
		case RT_WEAPON_VCLIP:
		case RT_HOSTAGE:
		case RT_POWERUP:
		case RT_FIREBALL:
			obj_rw->rtype.vclip_info.vclip_num = obj->rtype.vclip_info.vclip_num;
			obj_rw->rtype.vclip_info.frametime = obj->rtype.vclip_info.frametime;
			obj_rw->rtype.vclip_info.framenum  = obj->rtype.vclip_info.framenum;
			break;
			
		case RT_LASER:
			break;
			
	}
}

// turn object_rw to object after receiving
void multi_object_rw_to_object(object_rw *obj_rw, const vobjptr_t obj)
{
	obj->signature     = object_signature_t{static_cast<uint16_t>(obj_rw->signature)};
	obj->type          = obj_rw->type;
	obj->id            = obj_rw->id;
	obj->next          = obj_rw->next;
	obj->prev          = obj_rw->prev;
	obj->control_type  = obj_rw->control_type;
	obj->movement_type = obj_rw->movement_type;
	obj->render_type   = obj_rw->render_type;
	obj->flags         = obj_rw->flags;
	obj->segnum        = obj_rw->segnum;
	obj->attached_obj  = obj_rw->attached_obj;
	obj->pos.x         = obj_rw->pos.x;
	obj->pos.y         = obj_rw->pos.y;
	obj->pos.z         = obj_rw->pos.z;
	obj->orient.rvec.x = obj_rw->orient.rvec.x;
	obj->orient.rvec.y = obj_rw->orient.rvec.y;
	obj->orient.rvec.z = obj_rw->orient.rvec.z;
	obj->orient.fvec.x = obj_rw->orient.fvec.x;
	obj->orient.fvec.y = obj_rw->orient.fvec.y;
	obj->orient.fvec.z = obj_rw->orient.fvec.z;
	obj->orient.uvec.x = obj_rw->orient.uvec.x;
	obj->orient.uvec.y = obj_rw->orient.uvec.y;
	obj->orient.uvec.z = obj_rw->orient.uvec.z;
	obj->size          = obj_rw->size;
	obj->shields       = obj_rw->shields;
	obj->last_pos.x    = obj_rw->last_pos.x;
	obj->last_pos.y    = obj_rw->last_pos.y;
	obj->last_pos.z    = obj_rw->last_pos.z;
	obj->contains_type = obj_rw->contains_type;
	obj->contains_id   = obj_rw->contains_id;
	obj->contains_count= obj_rw->contains_count;
	obj->matcen_creator= obj_rw->matcen_creator;
	obj->lifeleft      = obj_rw->lifeleft;
	
	switch (obj->movement_type)
	{
		case MT_PHYSICS:
			obj->mtype.phys_info.velocity.x  = obj_rw->mtype.phys_info.velocity.x;
			obj->mtype.phys_info.velocity.y  = obj_rw->mtype.phys_info.velocity.y;
			obj->mtype.phys_info.velocity.z  = obj_rw->mtype.phys_info.velocity.z;
			obj->mtype.phys_info.thrust.x    = obj_rw->mtype.phys_info.thrust.x;
			obj->mtype.phys_info.thrust.y    = obj_rw->mtype.phys_info.thrust.y;
			obj->mtype.phys_info.thrust.z    = obj_rw->mtype.phys_info.thrust.z;
			obj->mtype.phys_info.mass        = obj_rw->mtype.phys_info.mass;
			obj->mtype.phys_info.drag        = obj_rw->mtype.phys_info.drag;
			obj->mtype.phys_info.rotvel.x    = obj_rw->mtype.phys_info.rotvel.x;
			obj->mtype.phys_info.rotvel.y    = obj_rw->mtype.phys_info.rotvel.y;
			obj->mtype.phys_info.rotvel.z    = obj_rw->mtype.phys_info.rotvel.z;
			obj->mtype.phys_info.rotthrust.x = obj_rw->mtype.phys_info.rotthrust.x;
			obj->mtype.phys_info.rotthrust.y = obj_rw->mtype.phys_info.rotthrust.y;
			obj->mtype.phys_info.rotthrust.z = obj_rw->mtype.phys_info.rotthrust.z;
			obj->mtype.phys_info.turnroll    = obj_rw->mtype.phys_info.turnroll;
			obj->mtype.phys_info.flags       = obj_rw->mtype.phys_info.flags;
			break;
			
		case MT_SPINNING:
			obj->mtype.spin_rate.x = obj_rw->mtype.spin_rate.x;
			obj->mtype.spin_rate.y = obj_rw->mtype.spin_rate.y;
			obj->mtype.spin_rate.z = obj_rw->mtype.spin_rate.z;
			break;
	}
	
	switch (obj->control_type)
	{
		case CT_WEAPON:
			obj->ctype.laser_info.parent_type      = obj_rw->ctype.laser_info.parent_type;
			obj->ctype.laser_info.parent_num       = obj_rw->ctype.laser_info.parent_num;
			obj->ctype.laser_info.parent_signature = object_signature_t{static_cast<uint16_t>(obj_rw->ctype.laser_info.parent_signature)};
			obj->ctype.laser_info.creation_time    = obj_rw->ctype.laser_info.creation_time;
			obj->ctype.laser_info.last_hitobj      = obj_rw->ctype.laser_info.last_hitobj;
			obj->ctype.laser_info.track_goal       = obj_rw->ctype.laser_info.track_goal;
			obj->ctype.laser_info.multiplier       = obj_rw->ctype.laser_info.multiplier;
#if defined(DXX_BUILD_DESCENT_II)
			obj->ctype.laser_info.last_afterburner_time = 0;
#endif
			break;
			
		case CT_EXPLOSION:
			obj->ctype.expl_info.spawn_time    = obj_rw->ctype.expl_info.spawn_time;
			obj->ctype.expl_info.delete_time   = obj_rw->ctype.expl_info.delete_time;
			obj->ctype.expl_info.delete_objnum = obj_rw->ctype.expl_info.delete_objnum;
			obj->ctype.expl_info.attach_parent = obj_rw->ctype.expl_info.attach_parent;
			obj->ctype.expl_info.prev_attach   = obj_rw->ctype.expl_info.prev_attach;
			obj->ctype.expl_info.next_attach   = obj_rw->ctype.expl_info.next_attach;
			break;
			
		case CT_AI:
		{
			int i;
			obj->ctype.ai_info.behavior               = static_cast<ai_behavior>(obj_rw->ctype.ai_info.behavior);
			for (i = 0; i < MAX_AI_FLAGS; i++)
				obj->ctype.ai_info.flags[i]       = obj_rw->ctype.ai_info.flags[i]; 
			obj->ctype.ai_info.hide_segment           = obj_rw->ctype.ai_info.hide_segment;
			obj->ctype.ai_info.hide_index             = obj_rw->ctype.ai_info.hide_index;
			obj->ctype.ai_info.path_length            = obj_rw->ctype.ai_info.path_length;
			obj->ctype.ai_info.cur_path_index         = obj_rw->ctype.ai_info.cur_path_index;
			obj->ctype.ai_info.danger_laser_num       = obj_rw->ctype.ai_info.danger_laser_num;
			obj->ctype.ai_info.danger_laser_signature = object_signature_t{static_cast<uint16_t>(obj_rw->ctype.ai_info.danger_laser_signature)};
#if defined(DXX_BUILD_DESCENT_I)
#elif defined(DXX_BUILD_DESCENT_II)
			obj->ctype.ai_info.dying_sound_playing    = obj_rw->ctype.ai_info.dying_sound_playing;
			obj->ctype.ai_info.dying_start_time       = obj_rw->ctype.ai_info.dying_start_time;
#endif
			break;
		}
			
		case CT_LIGHT:
			obj->ctype.light_info.intensity = obj_rw->ctype.light_info.intensity;
			break;
			
		case CT_POWERUP:
			obj->ctype.powerup_info.count         = obj_rw->ctype.powerup_info.count;
#if defined(DXX_BUILD_DESCENT_II)
			obj->ctype.powerup_info.creation_time = obj_rw->ctype.powerup_info.creation_time;
			obj->ctype.powerup_info.flags         = obj_rw->ctype.powerup_info.flags;
#endif
			break;
		case CT_CNTRLCEN:
		{
			// gun points of reactor now part of the object but of course not saved in object_rw. Let's just recompute them.
			int i = 0;
			reactor *reactor = get_reactor_definition(get_reactor_id(obj));
			for (i=0; i<reactor->n_guns; i++)
				calc_controlcen_gun_point(reactor, obj, i);
			break;
		}
	}
	
	switch (obj->render_type)
	{
		case RT_MORPH:
		case RT_POLYOBJ:
		case RT_NONE: // HACK below
		{
			int i;
			if (obj->render_type == RT_NONE && obj->type != OBJ_GHOST) // HACK: when a player is dead or not connected yet, clients still expect to get polyobj data - even if render_type == RT_NONE at this time.
				break;
			obj->rtype.pobj_info.model_num                = obj_rw->rtype.pobj_info.model_num;
			for (i=0;i<MAX_SUBMODELS;i++)
			{
				obj->rtype.pobj_info.anim_angles[i].p = obj_rw->rtype.pobj_info.anim_angles[i].p;
				obj->rtype.pobj_info.anim_angles[i].b = obj_rw->rtype.pobj_info.anim_angles[i].b;
				obj->rtype.pobj_info.anim_angles[i].h = obj_rw->rtype.pobj_info.anim_angles[i].h;
			}
			obj->rtype.pobj_info.subobj_flags             = obj_rw->rtype.pobj_info.subobj_flags;
			obj->rtype.pobj_info.tmap_override            = obj_rw->rtype.pobj_info.tmap_override;
			obj->rtype.pobj_info.alt_textures             = obj_rw->rtype.pobj_info.alt_textures;
			break;
		}
			
		case RT_WEAPON_VCLIP:
		case RT_HOSTAGE:
		case RT_POWERUP:
		case RT_FIREBALL:
			obj->rtype.vclip_info.vclip_num = obj_rw->rtype.vclip_info.vclip_num;
			obj->rtype.vclip_info.frametime = obj_rw->rtype.vclip_info.frametime;
			obj->rtype.vclip_info.framenum  = obj_rw->rtype.vclip_info.framenum;
			break;
			
		case RT_LASER:
			break;
			
	}
}

