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

#include "dxxsconf.h"
#include <bitset>
#include <new>
#include <stdexcept>
#include <random>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <inttypes.h>
#include <ranges>

#include "u_mem.h"
#include "strutil.h"
#include "game.h"
#include "multi.h"
#include "multiinternal.h"
#include "object.h"
#include "player.h"
#include "laser.h"
#include "gauges.h"
#include "gameseg.h"
#include "weapon.h"
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
#include "switch.h"
#include "textures.h"
#include "sounds.h"
#include "args.h"
#include "effects.h"
#include "iff.h"
#include "state.h"
#include "automap.h"
#include "event.h"
#include "d_array.h"
#include "d_enumerate.h"
#include "d_levelstate.h"
#include "d_range.h"
#include "d_underlying_value.h"
#include "d_zip.h"

#include "partial_range.h"
#include <utility>

#define array_snprintf(array,fmt,arg1,...)	std::snprintf(array.data(), array.size(), fmt, arg1, ## __VA_ARGS__)

constexpr std::integral_constant<int8_t, -1> owner_none{};

namespace dsx {

namespace {

static void MultiLevelInv_Repopulate(fix frequency);
void multi_new_bounty_target_with_sound(playernum_t, const char *callsign);
static void multi_reset_object_texture(object_base &objp);
static void multi_process_data(const d_level_shared_robot_info_state &LevelSharedRobotInfoState, playernum_t pnum, std::span<const uint8_t> data, multiplayer_command_t type);
static void multi_update_objects_for_non_cooperative();
static void multi_restore_game(unsigned slot, unsigned id);
static void multi_save_game(unsigned slot, unsigned id, const d_game_unique_state::savegame_description &desc);
static void multi_add_lifetime_killed();
#if !(!defined(RELEASE) && defined(DXX_BUILD_DESCENT_II))
static void multi_add_lifetime_kills(int count);
#endif

static constexpr netflag_flag operator~(const netflag_flag a)
{
	return static_cast<netflag_flag>(~static_cast<uint32_t>(a));
}

static constexpr netflag_flag operator|(const netflag_flag a, const netflag_flag b)
{
	return static_cast<netflag_flag>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

static constexpr netflag_flag operator&(const netflag_flag a, const netflag_flag b)
{
	return static_cast<netflag_flag>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

static constexpr netgrant_flag operator|(const netgrant_flag a, const netgrant_flag b)
{
	return static_cast<netgrant_flag>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

}

}

namespace {

static void multi_send_heartbeat();
static void multi_send_ranking(netplayer_info::player_rank);
static void multi_send_gmode_update();

}

#if defined(DXX_BUILD_DESCENT_II)
namespace dsx {

namespace {

static char hoard_ham_basename[]{"hoard.ham"};

static void multi_do_capture_bonus(const playernum_t pnum);
static void multi_do_orb_bonus(const playernum_t pnum, const multiplayer_rspan<multiplayer_command_t::MULTI_ORB_BONUS> buf);
static void multi_send_drop_flag(vmobjptridx_t objnum,int seed);

}

}
#endif
namespace dcx {
namespace {
static int imulti_new_game; // to prep stuff for level only when starting new game
static std::size_t multi_message_index;
static per_player_array<std::array<objnum_t, MAX_OBJECTS>> remote_to_local;  // Remote object number for each local object
static std::array<uint16_t, MAX_OBJECTS> local_to_remote;
static per_player_array<unsigned> sorted_kills;
static void multi_send_quit();
void multi_new_bounty_target(playernum_t, const char *callsign);

using kill_name_storage = std::array<char, (CALLSIGN_LEN * 2) + 4>;

[[nodiscard]]
static const char *prepare_kill_name(fvcplayerptr &vcplayerptr, const game_mode_flags Game_mode, const netgame_info &Netgame, const vcplayeridx_t pnum, kill_name_storage &buf)
{
	const char *const callsign = vcplayerptr(pnum)->callsign;
	if (Game_mode & GM_TEAM)
	{
		const auto r = std::data(buf);
		snprintf(r, std::size(buf), "%s (%s)", callsign, Netgame.team_name[get_team(pnum)].operator const char *());
		return r;
	}
	else
		return callsign;
}

constexpr vms_angvec build_native_endian_angvec_from_little_endian(const vms_angvec &v)
{
	return vms_angvec{
		.p = INTEL_SHORT(v.p),
		.b = INTEL_SHORT(v.b),
		.h = INTEL_SHORT(v.h),
	};
}

constexpr vms_vector build_native_endian_vector_from_little_endian(const vms_vector &v)
{
	return vms_vector{
		.x = INTEL_INT(v.x),
		.y = INTEL_INT(v.y),
		.z = INTEL_INT(v.z),
	};
}

constexpr vms_matrix build_native_endian_matrix_from_little_endian(const vms_matrix &v)
{
	return vms_matrix{
		.rvec = build_native_endian_vector_from_little_endian(v.rvec),
		.uvec = build_native_endian_vector_from_little_endian(v.uvec),
		.fvec = build_native_endian_vector_from_little_endian(v.fvec),
	};
}

#define build_little_endian_angvec_from_native_endian build_native_endian_angvec_from_little_endian
#define build_little_endian_matrix_from_native_endian build_native_endian_matrix_from_little_endian
#define build_little_endian_vector_from_native_endian build_native_endian_vector_from_little_endian

}
DEFINE_SERIAL_UDT_TO_MESSAGE(shortpos, s, (s.bytemat, s.xo, s.yo, s.zo, s.segment, s.velx, s.vely, s.velz));

std::optional<network_state> build_network_state_from_untrusted(const uint8_t untrusted)
{
	switch (untrusted)
	{
		case static_cast<uint8_t>(network_state::menu):
		case static_cast<uint8_t>(network_state::playing):
		case static_cast<uint8_t>(network_state::browsing):
		case static_cast<uint8_t>(network_state::waiting):
		case static_cast<uint8_t>(network_state::starting):
		case static_cast<uint8_t>(network_state::endlevel):
			return network_state{untrusted};
		default:
			return std::nullopt;
	}
}

vms_vector multi_get_vector(const std::span<const uint8_t, 12> buf)
{
	return vms_vector{
		GET_INTEL_INT<int32_t>(&buf[0]),
		GET_INTEL_INT<int32_t>(&buf[4]),
		GET_INTEL_INT<int32_t>(&buf[8]),
	};
}

void multi_put_vector(uint8_t *const buf, const vms_vector &v)
{
	/* Copy the vms_vector into a local so that the compiler can prove that
	 * `buf` does not alias the bytes of the vector.  Optimizing builds will
	 * eliminate this needless copy.
	 *
	 * With no aliasing, the compiler can use word-sized load/store
	 * instructions.
	 *
	 * With possible aliasing, the compiler uses `fix` sized load/store, since
	 * a store to `buf` may change the result of a subsequent load from an
	 * aliased vms_vector.
	 */
	const auto lv = v;
	PUT_INTEL_INT(&buf[0], lv.x);
	PUT_INTEL_INT(&buf[4], lv.y);
	PUT_INTEL_INT(&buf[8], lv.z);
}

}
namespace {
static playernum_t multi_who_is_master();
static void multi_show_player_list();
static void multi_send_message();
}

//
// Global variables
//

namespace dcx {

//do we draw the kill list on the HUD?
show_kill_list_mode Show_kill_list = show_kill_list_mode::_1;
bool Show_reticle_name{true};
fix Show_kill_list_timer = 0;

}

#if defined(DXX_BUILD_DESCENT_II)
namespace dsx {
hoard_highest_record hoard_highest_record_stats;
}
#endif

namespace dcx {

network_state Network_status;
playernum_t Bounty_target;


per_player_array<msgsend_state> multi_sending_message;
multi_macro_message_index multi_defining_message{multi_macro_message_index::None};

std::array<sbyte, MAX_OBJECTS> object_owner;   // Who created each object in my universe, -1 = loaded at start

unsigned   Net_create_loc;       // pointer into previous array
std::array<objnum_t, MAX_NET_CREATE_OBJECTS>   Net_create_objnums; // For tracking object creation that will be sent to remote
ntstring<MAX_MESSAGE_LEN - 1> Network_message;
int   Network_message_reciever=-1;
per_player_array<per_player_array<uint16_t>> kill_matrix;
per_team_array<int16_t> team_kills;
int   multi_quit_game = 0;

}

namespace dsx {

const GMNames_array GMNames = {{
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
}};
const std::array<char[8], MULTI_GAME_TYPE_COUNT> GMNamesShrt = {{
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
}};

}

namespace dcx {

// For rejoin object syncing (used here and all protocols - globally)

int	Network_send_objects = 0;  // Are we in the process of sending objects to a player?
int	Network_send_object_mode = 0; // What type of objects are we sending, static or dynamic?
int 	Network_send_objnum = -1;   // What object are we sending next?
int     Network_rejoined = 0;       // Did WE rejoin this game?
int     Network_sending_extras=0;
int     VerifyPlayerJoined=-1;      // Player (num) to enter game before any ingame/extra stuff is being sent
int     Player_joining_extras=-1;  // This is so we know who to send 'latecomer' packets to.

ushort          my_segments_checksum = 0;


per_player_array<std::array<bitmap_index, N_PLAYER_SHIP_TEXTURES>> multi_player_textures;

// Globals for protocol-bound Refuse-functions
char RefuseThisPlayer=0,WaitForRefuseAnswer=0,RefuseTeam,RefusePlayerName[12];
fix64 RefuseTimeLimit=0;

namespace {
constexpr int message_length[] = {
#define define_message_length(NAME,SIZE)	(SIZE),
	for_each_multiplayer_command(define_message_length)
};

}

}

namespace dsx {

netgame_info Netgame;
multi_level_inv MultiLevelInv;

}

namespace dcx {
const enumerated_array<char[16], 10, netplayer_info::player_rank> RankStrings{{{
	"(unpatched)",
	"Cadet",
	"Ensign",
	"Lieutenant",
	"Lt.Commander",
	"Commander",
	"Captain",
	"Vice Admiral",
	"Admiral",
	"Demigod"
}}};

netplayer_info::player_rank build_rank_from_untrusted(const uint8_t untrusted)
{
	switch (untrusted)
	{
		case static_cast<uint8_t>(netplayer_info::player_rank::None):
		case static_cast<uint8_t>(netplayer_info::player_rank::Cadet):
		case static_cast<uint8_t>(netplayer_info::player_rank::Ensign):
		case static_cast<uint8_t>(netplayer_info::player_rank::Lieutenant):
		case static_cast<uint8_t>(netplayer_info::player_rank::LtCommander):
		case static_cast<uint8_t>(netplayer_info::player_rank::Commander):
		case static_cast<uint8_t>(netplayer_info::player_rank::Captain):
		case static_cast<uint8_t>(netplayer_info::player_rank::ViceAdmiral):
		case static_cast<uint8_t>(netplayer_info::player_rank::Admiral):
		case static_cast<uint8_t>(netplayer_info::player_rank::Demigod):
			return netplayer_info::player_rank{untrusted};
		default:
			return netplayer_info::player_rank::None;
	}
}
}

namespace dsx {
const multi_allow_powerup_text_array multi_allow_powerup_text = {{
#define define_netflag_string(NAME,STR)	STR,
	for_each_netflag_value(define_netflag_string)
}};
}

netplayer_info::player_rank GetMyNetRanking()
{
	int rank, eff;

	if (PlayerCfg.NetlifeKills + PlayerCfg.NetlifeKilled <= 0)
		return netplayer_info::player_rank::Cadet;

	rank=static_cast<int>((static_cast<float>(PlayerCfg.NetlifeKills)/3000.0)*8.0);

	eff = static_cast<int>(
		(
			static_cast<float>(PlayerCfg.NetlifeKills) / (
				static_cast<float>(PlayerCfg.NetlifeKilled) + static_cast<float>(PlayerCfg.NetlifeKills)
			)
		) * 100.0
	);

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

	return static_cast<netplayer_info::player_rank>(rank + 1);
}

//
//  Functions that replace what used to be macros
//

objnum_t objnum_remote_to_local(const uint16_t remote_objnum, const int8_t owner)
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
	auto &Objects = LevelUniqueObjectState.Objects;
	// Map a local object number to a remote + owner
	if (local_objnum > Highest_object_index)
	{
		return {owner_none, 0xffff};
	}
	auto owner = object_owner[local_objnum];
	if (owner == owner_none)
		return {owner, local_objnum};
	auto result = local_to_remote[local_objnum];
	const char *emsg;
	if (
		((owner >= N_players || owner < -1) && (emsg = "illegal object owner", true)) ||
		(result >= MAX_OBJECTS && (emsg = "illegal object remote number", true))	// See Rob, object has no remote number!
	)
		throw std::runtime_error(emsg);
	return {owner, result};
}

void map_objnum_local_to_remote(const objnum_t local_objnum, const int remote_objnum, const int owner)
{
	// Add a mapping from a network remote object number to a local one
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

namespace dsx {

namespace {

void update_bounty_target()
{
	std::array<std::pair<playernum_t, const char *>, std::size(Players)> candidates{};
	const auto b = candidates.begin();
	auto iter = b;
	for (auto &&[idx, plr] : enumerate(Players))
		if (plr.connected != player_connection_status::disconnected)
			*iter++ = {idx, plr.callsign};
	const auto n = std::distance(b, iter);
	if (!n)
		return;
	auto &choice = candidates[d_rand() % n];
	multi_new_bounty_target_with_sound(choice.first, choice.second);
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
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	player_connection_status old_connect{};

	// Save connect state and change to new connect state
	if (Game_mode & GM_NETWORK)
	{
		auto &plr = get_local_player();
		old_connect = plr.connected;
		if (plr.connected != player_connection_status::died_in_mine)
			plr.connected = player_connection_status::end_menu;
		Network_status = network_state::endlevel;
	}

	// Do the actual screen we wish to show
	const auto rval = kmatrix_view(static_cast<kmatrix_network>(Game_mode & GM_NETWORK), Controls);

	// Restore connect state

	if (Game_mode & GM_NETWORK)
	{
		get_local_player().connected = old_connect;
	}

	/* Door key flags should only be cleared in cooperative games, not
	 * in other games.
	 *
	 * The capture-the-flag marker can be cleared unconditionally, but
	 * would never have been set in a cooperative game.
	 *
	 * The kill goal count can be cleared unconditionally.
	 *
	 * For Descent 1, the only flags to clear are the door key flags.
	 * Use a no-op mask in non-cooperative games, since there are no
	 * flags to clear there.
	 *
	 * For Descent 2, clear door key flags or the capture-the-flag
	 * flag, depending on game type.  This version has the advantage of
	 * making only one pass when in cooperative mode, where the previous
	 * version would make one pass if in a cooperative game, then make
	 * an unconditional pass to try to clear PLAYER_FLAGS_FLAG.
	 */
	const auto clear_flags = (Game_mode & GM_MULTI_COOP)
		// Reset keys
		? ~player_flags(PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_GOLD_KEY | PLAYER_FLAGS_RED_KEY)
		:
#if defined(DXX_BUILD_DESCENT_I)
		/* Nothing to clear.  Set a mask that has no effect when
		 * applied, so that the loop does not need to retest the
		 * conditional on each pass.
		 */
		player_flags(~0u)
#elif defined(DXX_BUILD_DESCENT_II)
		// Clear capture flag
		~player_flags(PLAYER_FLAGS_FLAG)
#endif
		;
	range_for (auto &i, partial_const_range(Players, Netgame.max_numplayers))
	{
		auto &obj = *vmobjptr(i.objnum);
		auto &player_info = obj.ctype.player_info;
		player_info.powerup_flags &= clear_flags;
		player_info.KillGoalCount = 0;
	}
	return rval;
}

}

team_number get_team(const playernum_t pnum)
{
	if (Netgame.team_vector & (1 << pnum))
		return team_number::red;
	else
		return team_number::blue;
}

void multi_new_game()
{
	// Reset variables for a new net game
	reset_globals_for_new_game();

	LevelUniqueObjectState.accumulated_robots = 0;
	LevelUniqueObjectState.total_hostages = 0;
	GameUniqueState.accumulated_robots = 0;
	GameUniqueState.total_hostages = 0;
	for (uint_fast32_t i = 0; i < MAX_PLAYERS; i++)
		init_player_stats_game(i);

	kill_matrix = {}; // Clear kill matrix

	for (playernum_t i = 0; i < MAX_PLAYERS; ++i)
	{
		sorted_kills[i] = i;
		vmplayerptr(i)->connected = player_connection_status::disconnected;
	}
	multi_sending_message.fill(msgsend_state::none);

	robot_controlled.fill(-1);
	robot_agitation = {};
	robot_fired = {};

	team_kills = {};
	imulti_new_game=1;
	multi_quit_game = 0;
	Show_kill_list = show_kill_list_mode::_1;
	game_disable_cheats();
}

namespace dsx {

void multi_make_player_ghost(const playernum_t playernum)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	if (playernum == Player_num || playernum >= MAX_PLAYERS)
	{
		Int3(); // Non-terminal, see Rob
		return;
	}
	const auto &&obj = vmobjptridx(vcplayerptr(playernum)->objnum);
	obj->type = OBJ_GHOST;
	obj->render_type = render_type::RT_NONE;
	obj->movement_source = object::movement_type::None;
	multi_reset_player_object(obj);
	multi_strip_robots(playernum);
}

void multi_make_ghost_player(const playernum_t playernum)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	if ((playernum == Player_num) || (playernum >= MAX_PLAYERS))
	{
		Int3(); // Non-terminal, see rob
		return;
	}
	const auto &&obj = vmobjptridx(vcplayerptr(playernum)->objnum);
	obj->type = OBJ_PLAYER;
	obj->movement_source = object::movement_type::physics;
	multi_reset_player_object(obj);
	if (playernum != Player_num)
		init_player_stats_new_ship(playernum);
}

}

int multi_get_kill_list(playernum_array_t &plist)
{
	// Returns the number of active net players and their
	// sorted order of kills
	int n = 0;

	range_for (const auto i, partial_const_range(sorted_kills, N_players))
		//if (Players[sorted_kills[i]].connected)
		plist[n++] = i;

	if (n == 0)
		Int3(); // SEE ROB OR MATT

	//memcpy(plist, sorted_kills, N_players*sizeof(int));

	return(n);
}

namespace dsx {

void multi_sort_kill_list()
{
	const auto n_players{N_players};
	if (n_players > std::size(sorted_kills))
		return;
	auto &Objects = LevelUniqueObjectState.Objects;
	// Sort the kills list each time a new kill is added
	const auto build_kill_list_sort_key{[](fvcobjptr &vcobjptr, const game_mode_flags game_mode,
#if defined(DXX_BUILD_DESCENT_II)
		const show_kill_list_mode show_kill_list,
#endif
		const player &plr) -> int {
		const auto o{plr.objnum};
		if (o == object_none)
			return -1;
		auto &player_info = vcobjptr(o)->ctype.player_info;
		if (game_mode & GM_MULTI_COOP)
			return {player_info.mission.score};
		const auto net_kills_total{player_info.net_kills_total};
#if defined(DXX_BUILD_DESCENT_II)
		if (show_kill_list == show_kill_list_mode::efficiency)
		{
			const auto kk{player_info.net_killed_total + net_kills_total};
			if (kk <= 0)
				// always draw the ones without any ratio last
				return kk - 1;
			const float net_kills_total_f = net_kills_total;
			return static_cast<int>(net_kills_total_f / (
					static_cast<float>(player_info.net_killed_total) + net_kills_total_f
			) * 100.0f);
		}
#endif
		return {net_kills_total};
	}};
	const auto build_kill_list_sort_keys{[build_kill_list_sort_key](fvcobjptr &vcobjptr, const game_mode_flags game_mode,
#if defined(DXX_BUILD_DESCENT_II)
		const show_kill_list_mode show_kill_list,
#endif
		fvcplayerptr &vcplayerptr) -> per_player_array<int> {
		per_player_array<int> kills;
		for (auto &&[k, plr] : zip(kills, vcplayerptr))
			k = build_kill_list_sort_key(vcobjptr, game_mode,
#if defined(DXX_BUILD_DESCENT_II)
				show_kill_list,
#endif
				plr);
		return kills;
	}};
	const auto projection{[keys = build_kill_list_sort_keys(Objects.vcptr, Game_mode,
#if defined(DXX_BUILD_DESCENT_II)
		Show_kill_list,
#endif
		vcplayerptr)](const unsigned sk) {
		return keys[sk];
	}};
	std::ranges::sort(std::span(sorted_kills).first(n_players), std::ranges::greater{}, std::ref(projection));
}

namespace {

static void print_kill_goal_tables(fvcobjptr &vcobjptr)
{
	const auto &local_player = get_local_player();
	const auto pnum = Player_num;
	con_printf(CON_NORMAL, "Kill goal statistics: player #%u \"%s\"", pnum, static_cast<const char *>(local_player.callsign));
	for (auto &&[idx, i] : enumerate(Players))
	{
		if (i.connected == player_connection_status::disconnected)
			continue;
		auto &plrobj = *vcobjptr(i.objnum);
		auto &player_info = plrobj.ctype.player_info;
		con_printf(CON_NORMAL, "\t#%" DXX_PRI_size_type " \"%s\"\tdeaths=%i\tkills=%i\tmatrix=%hu/%hu", idx, static_cast<const char *>(i.callsign), player_info.net_killed_total, player_info.net_kills_total, kill_matrix[pnum][idx], kill_matrix[idx][pnum]);
	}
}

static void net_destroy_controlcen(object_array &Objects, const d_robot_info_array &Robot_info)
{
	print_kill_goal_tables(Objects.vcptr);
	HUD_init_message_literal(HM_MULTI, "The control center has been destroyed!");
	net_destroy_controlcen_object(Robot_info, obj_find_first_of_type(Objects.vmptridx, OBJ_CNTRLCEN));
}

static void multi_compute_kill(const d_robot_info_array &Robot_info, const imobjptridx_t killer, object &killed)
{
#if defined(DXX_BUILD_DESCENT_II)
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
#endif
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	// Figure out the results of a network kills and add it to the
	// appropriate player's tally.

	playernum_t killed_pnum, killer_pnum;

	// Both object numbers are localized already!

	const auto killed_type = killed.type;
	if ((killed_type != OBJ_PLAYER) && (killed_type != OBJ_GHOST))
	{
		Int3(); // compute_kill passed non-player object!
		return;
	}

	killed_pnum = get_player_id(killed);

	Assert (killed_pnum < N_players);

	kill_name_storage killed_buf;
	const auto killed_name = prepare_kill_name(vcplayerptr, Game_mode, Netgame, killed_pnum, killed_buf);

	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_multi_death(killed_pnum);

	digi_play_sample( SOUND_HUD_KILL, F3_0 );

#if defined(DXX_BUILD_DESCENT_II)
	if (LevelUniqueControlCenterState.Control_center_destroyed)
		vmplayerptr(killed_pnum)->connected = player_connection_status::died_in_mine;
#endif

	if (killer == object_none)
		return;
	const auto killer_type = killer->type;
	if (killer_type == OBJ_CNTRLCEN)
	{
		if (Game_mode & GM_TEAM)
			-- team_kills[get_team(killed_pnum)];
		++ killed.ctype.player_info.net_killed_total;
		-- killed.ctype.player_info.net_kills_total;
		-- killed.ctype.player_info.KillGoalCount;

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
		if (killer_type == OBJ_WEAPON && get_weapon_id(killer) == weapon_id_type::PMINE_ID)
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
		++ killed.ctype.player_info.net_killed_total;
		return;
	}

	killer_pnum = get_player_id(killer);

	kill_name_storage killer_buf;
	const auto killer_name = prepare_kill_name(vcplayerptr, Game_mode, Netgame, killer_pnum, killer_buf);

	// Beyond this point, it was definitely a player-player kill situation

	if (killer_pnum >= N_players)
		Int3(); // See rob, tracking down bug with kill HUD messages
	if (killed_pnum >= N_players)
		Int3(); // See rob, tracking down bug with kill HUD messages

	kill_matrix[killer_pnum][killed_pnum] += 1;
	if (killer_pnum == killed_pnum)
	{
		if (!game_mode_hoard(Game_mode))
		{
			if (Game_mode & GM_TEAM)
			{
				team_kills[get_team(killed_pnum)] -= 1;
			}

			++ killed.ctype.player_info.net_killed_total;
			-- killed.ctype.player_info.net_kills_total;
			-- killed.ctype.player_info.KillGoalCount;

			if (Newdemo_state == ND_STATE_RECORDING)
				newdemo_record_multi_kill(killed_pnum, -1);
		}
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
			update_bounty_target();
		}
	}

	else
	{
		short adjust = 1;
		/* Dead stores to prevent bogus -Wmaybe-uninitialized warnings
		 * when the optimization level prevents the compiler from
		 * recognizing that these are written in a team game and only
		 * read in a team game.
		 */
		team_number killed_team{}, killer_team{};
		DXX_MAKE_VAR_UNDEFINED(killed_team);
		DXX_MAKE_VAR_UNDEFINED(killer_team);
		const auto is_team_game = Game_mode & GM_TEAM;
		if (is_team_game)
		{
			killed_team = get_team(killed_pnum);
			killer_team = get_team(killer_pnum);
			if (killed_team == killer_team)
				adjust = -1;
		}
		if (!game_mode_hoard(Game_mode))
		{
			if (is_team_game)
			{
				team_kills[killer_team] += adjust;
				killer->ctype.player_info.net_kills_total += adjust;
				killer->ctype.player_info.KillGoalCount += adjust;
			}
			else if( Game_mode & GM_BOUNTY )
			{
				/* Did the target die?  Did the target get a kill? */
				if( killed_pnum == Bounty_target || killer_pnum == Bounty_target )
				{
					/* Increment kill counts */
					++ killer->ctype.player_info.net_kills_total;
					++ killer->ctype.player_info.KillGoalCount;
					
					/* If the target died, the new one is set! */
					if( killed_pnum == Bounty_target )
						multi_new_bounty_target_with_sound(killer_pnum, vcplayerptr(get_player_id(killer))->callsign);
				}
			}
			else
			{
				++ killer->ctype.player_info.net_kills_total;
				++ killer->ctype.player_info.KillGoalCount;
			}
			
			if (Newdemo_state == ND_STATE_RECORDING)
				newdemo_record_multi_kill(killer_pnum, 1);
		}

		++ killed.ctype.player_info.net_killed_total;
		const char *name0, *name1;
		if (killer_pnum == Player_num) {
			if (Game_mode & GM_MULTI_COOP)
			{
				auto &player_info = get_local_plrobj().ctype.player_info;
				const auto local_player_score = player_info.mission.score;
				add_points_to_score(player_info, local_player_score >= 1000 ? -1000 : -local_player_score, Game_mode);
			}
			else
				multi_add_lifetime_kills(adjust);
			name0 = TXT_YOU;
			name1 = killed_name;
		}
		else if (name0 = killer_name, killed_pnum == Player_num)
		{
			multi_add_lifetime_killed();
			name1 = TXT_YOU;
		}
		else
			name1 = killed_name;
		HUD_init_message(HM_MULTI, "%s %s %s!", name0, TXT_KILLED, name1);
	}

	if (Netgame.KillGoal>0)
	{
		const auto TheGoal = Netgame.KillGoal * 5;
		if (((Game_mode & GM_TEAM)
				? team_kills[get_team(killer_pnum)]
				: killer->ctype.player_info.KillGoalCount
			) >= TheGoal)
		{
			if (killer_pnum==Player_num)
			{
				HUD_init_message_literal(HM_MULTI, "You reached the kill goal!");
				get_local_plrobj().shields = i2f(200);
			}
			else
			{
				kill_name_storage buf;
				HUD_init_message(HM_MULTI, "%s has reached the kill goal!", prepare_kill_name(vcplayerptr, Game_mode, Netgame, killer_pnum, buf));
			}
			net_destroy_controlcen(Objects, Robot_info);
		}
	}

	multi_sort_kill_list();
	multi_show_player_list();
#if defined(DXX_BUILD_DESCENT_II)
	// clear the killed guys flags/headlights
	killed.ctype.player_info.powerup_flags &= ~(PLAYER_FLAGS_HEADLIGHT_ON); 
#endif
}

}

}

window_event_result multi_do_frame()
{
	static d_time_fix lasttime;
	static fix64 last_gmode_time = 0, last_inventory_time = 0, last_repo_time = 0;

	if (!(Game_mode & GM_MULTI) || Newdemo_state == ND_STATE_PLAYBACK)
	{
		Int3();
		return window_event_result::ignored;
	}

	if ((Game_mode & GM_NETWORK) && Netgame.PlayTimeAllowed.count() && lasttime != ThisLevelTime)
	{
		for (unsigned i = 0; i < N_players; ++i)
			if (vcplayerptr(i)->connected != player_connection_status::disconnected)
			{
				if (i==Player_num)
				{
					multi_send_heartbeat();
					lasttime = ThisLevelTime;
				}
				break;
			}
	}

	// Send update about our game mode-specific variables every 2 secs (to keep in sync since delayed kills can invalidate these infos on Clients)
	if (multi_i_am_master() && timer_query() >= last_gmode_time + (F1_0*2))
	{
		multi_send_gmode_update();
		last_gmode_time = timer_query();
	}

	if (Network_status == network_state::playing)
	{
		// Send out inventory three times per second
		if (timer_query() >= last_inventory_time + (F1_0/3))
		{
			multi_send_player_inventory(multiplayer_data_priority::_0);
			last_inventory_time = timer_query();
		}
		// Repopulate the level if necessary
		if (timer_query() >= last_repo_time + (F1_0/2))
		{
			MultiLevelInv_Repopulate((F1_0/2));
			last_repo_time = timer_query();
		}
	}

	multi_send_message(); // Send any waiting messages

	if (Game_mode & GM_MULTI_ROBOTS)
	{
		multi_check_robot_timeout();
	}

	multi::dispatch->do_protocol_frame(0, 1);

	return multi_quit_game ? window_event_result::close : window_event_result::handled;
}

namespace {

template <multiplayer_command_t C>
#ifndef __clang__
/* udp::dispatch_table::send_data_direct copies `buf` into a buffer sized from
 * `UDP_mdata_info`.  Require that no overflow will occur.
 *
 * Guard this with `#ifndef __clang__` because clang-14 rejects this constraint
 * with the error:

similar/main/multi.cpp:1068:22: note: because '(std::size(buf) + 6 <= sizeof(UDP_mdata_info))' would be invalid: constraint variable 'buf' cannot be used in an evaluated context

 * gcc accepts this requires() constraint and enforces it as intended.  Raising
 * the `6` to `6000` correctly provokes a rejection.
 */
requires(
	requires(multi_command<C> buf) {
		requires(std::size(buf) + 6 <= sizeof(UDP_mdata_info));
	}
)
#endif
static inline void multi_send_data_direct(const multi_command<C> &buf, const playernum_t pnum, const int priority)
{
	multi::dispatch->send_data_direct(buf, pnum, priority);
}

}

namespace dsx {

void multi_leave_game()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;

	if (!(Game_mode & GM_MULTI))
		return;

	if (!multi_i_am_master())
	{
		/* The host does not need to send item drops, because the game will end
		 * when the host quits.  The multi_send_quit after this block will
		 * cause all guests to be ejected from the game, so no players would
		 * have time to pick up any items dropped by this block.
		 */
		Net_create_loc = 0;
		const auto cobjp = vmobjptridx(get_local_player().objnum);
		multi_send_position(cobjp);
		auto &player_info = cobjp->ctype.player_info;
		if (!player_info.Player_eggs_dropped)
		{
			player_info.Player_eggs_dropped = true;
			drop_player_eggs(cobjp);
		}
		multi_send_player_deres(deres_drop);
	}

	multi_send_quit();
	multi::dispatch->leave_game();

#if defined(DXX_BUILD_DESCENT_I)
	plyr_save_stats(InterfaceUniqueState.PilotName, PlayerCfg.NetlifeKills, PlayerCfg.NetlifeKilled);
#endif

	multi_quit_game = 0;	// quit complete
}

}

namespace {

void multi_show_player_list()
{
	if (!(Game_mode & GM_MULTI) || (Game_mode & GM_MULTI_COOP))
		return;

	if (Show_kill_list != show_kill_list_mode::None)
		return;

	Show_kill_list_timer = F1_0*5; // 5 second timer
	Show_kill_list = show_kill_list_mode::_1;
}

}

//
// Part 2 : functions that act on network messages and change the
//          the state of the game in some way.
//

void multi_define_macro(const int key)
{
	if (!(Game_mode & GM_MULTI))
		return;

	switch (key & ~KEY_SHIFTED)
	{
		case KEY_F9:
			multi_defining_message = multi_macro_message_index::_0;
			break;
		case KEY_F10:
			multi_defining_message = multi_macro_message_index::_1;
			break;
		case KEY_F11:
			multi_defining_message = multi_macro_message_index::_2;
			break;
		case KEY_F12:
			multi_defining_message = multi_macro_message_index::_3;
			break;
		default:
			Int3();
			return;
	}
	{
		key_toggle_repeat(1);
		multi_message_index = 0;
		Network_message = {};
	}
}

namespace {

static void multi_message_feedback(void)
{
	char *colon;
	int found = 0;
	char feedback_result[200];

	if (!(!(colon = strstr(Network_message.data(), ": ")) || colon == Network_message.data() || colon - Network_message.data() > CALLSIGN_LEN))
	{
		std::size_t feedlen = snprintf(feedback_result, sizeof(feedback_result), "%s ", TXT_MESSAGE_SENT_TO);
		if (Game_mode & GM_TEAM)
		{
			if (const auto o = Netgame.team_name.valid_index(Network_message[0u] - '1'))
			{
				snprintf(feedback_result + feedlen, sizeof(feedback_result) - feedlen, "%s '%s'", TXT_TEAM, Netgame.team_name[*o].operator const char *());
				found = 1;
			}
			range_for (auto &i, Netgame.team_name)
			{
				if (!d_strnicmp(i, Network_message.data(), colon - Network_message.data()))
				{
					const char *comma = found ? ", " : "";
					found++;
					const char *newline = (!(found % 4)) ? "\n" : "";
					size_t l = strlen(feedback_result);
					snprintf(feedback_result + l, sizeof(feedback_result) - l, "%s%s%s '%s'", comma, newline, TXT_TEAM, static_cast<const char *>(i));
				}
			}
		}
		const player *const local_player = &get_local_player();
		range_for (auto &i, partial_const_range(Players, N_players))
		{
			if (&i != local_player && i.connected != player_connection_status::disconnected && !d_strnicmp(static_cast<const char *>(i.callsign), Network_message.data(), colon - Network_message.data()))
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

}

void multi_send_macro(const int fkey)
{
	if (! (Game_mode & GM_MULTI) )
		return;

	multi_macro_message_index key;
	switch (fkey)
	{
		case KEY_F9:
			key = multi_macro_message_index::_0;
			break;
		case KEY_F10:
			key = multi_macro_message_index::_1;
			break;
		case KEY_F11:
			key = multi_macro_message_index::_2;
			break;
		case KEY_F12:
			key = multi_macro_message_index::_3;
			break;
		default:
			Int3();
			return;
	}

	if (!PlayerCfg.NetworkMessageMacro[key][0u])
	{
		const auto &&m = TXT_NO_MACRO;
		HUD_init_message_literal(HM_MULTI, {m, strlen(m)});
		return;
	}

	Network_message = PlayerCfg.NetworkMessageMacro[key];
	Network_message_reciever = 100;

	HUD_init_message(HM_MULTI, "%s '%s'", TXT_SENDING, Network_message.data());
	multi_message_feedback();
}

void multi_send_message_start()
{
	if (Game_mode&GM_MULTI) {
		multi_sending_message[Player_num] = msgsend_state::typing;
		multi_send_msgsend_state(msgsend_state::typing);
		multi_message_index = 0;
		Network_message = {};
		key_toggle_repeat(1);
	}
}

namespace dsx {

namespace {

static void kick_player(const player &plr, netplayer_info &nplr)
{
	multi::dispatch->kick_player(nplr.protocol.udp.addr, kick_player_reason::kicked);
	HUD_init_message(HM_MULTI, "Dumping %s...", static_cast<const char *>(plr.callsign));
	multi_message_index = 0;
	multi_sending_message[Player_num] = msgsend_state::none;
#if defined(DXX_BUILD_DESCENT_II)
	multi_send_msgsend_state(msgsend_state::none);
#endif
}

static void multi_send_message_end(const d_robot_info_array &Robot_info, fvmobjptr &vmobjptr, control_info &Controls)
{
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
#if defined(DXX_BUILD_DESCENT_I)
	multi_message_index = 0;
	multi_sending_message[Player_num] = msgsend_state::none;
	multi_send_msgsend_state(msgsend_state::none);
	key_toggle_repeat(0);
#elif defined(DXX_BUILD_DESCENT_II)
	Network_message_reciever = 100;
#endif

	if (!d_strnicmp(Network_message.data(), "/Handicap: "))
	{
		constexpr auto minimum_allowed_shields = 10ul;
		constexpr auto maximum_allowed_shields = 100ul;
		auto &plr = get_local_player();
		const char *const callsign = plr.callsign;
		const auto requested_handicap = strtoul(&Network_message[11u], 0, 10);
		const auto clamped_handicap_max = std::max(minimum_allowed_shields, requested_handicap);
		const auto clamped_handicap_min = std::min(maximum_allowed_shields, clamped_handicap_max);
		StartingShields = i2f(clamped_handicap_min);
		if (clamped_handicap_min != clamped_handicap_max)
			snprintf(Network_message.data(), Network_message.size(), "%s has tried to cheat!", callsign);
		else
			snprintf(Network_message.data(), Network_message.size(), "%s handicap is now %lu", callsign, clamped_handicap_min);
		HUD_init_message(HM_MULTI, "Telling others of your handicap of %lu!",clamped_handicap_min);
	}
	else if (!d_strnicmp(Network_message.data(), "/move: "))
	{
		if ((Game_mode & GM_NETWORK) && (Game_mode & GM_TEAM))
		{
			if (!multi_i_am_master())
			{
				HUD_init_message(HM_MULTI, "Only %s can move players!",static_cast<const char *>(Players[multi_who_is_master()].callsign));
				return;
			}

			unsigned name_index=7;
			const auto nlen = strlen(Network_message.data());
			if (nlen > 7)
				while (Network_message[name_index] == ' ')
					name_index++;

			if (nlen <= name_index)
			{
				HUD_init_message_literal(HM_MULTI, "You must specify a name to move");
				return;
			}

			for (unsigned i = 0; i < N_players; ++i)
				if (vcplayerptr(i)->connected != player_connection_status::disconnected && !d_strnicmp(static_cast<const char *>(vcplayerptr(i)->callsign), &Network_message[name_index], nlen - name_index))
				{
#if defined(DXX_BUILD_DESCENT_II)
					if (game_mode_capture_flag(Game_mode) && (vmobjptr(vcplayerptr(i)->objnum)->ctype.player_info.powerup_flags & PLAYER_FLAGS_FLAG))
					{
						HUD_init_message_literal(HM_MULTI, "Can't move player because s/he has a flag!");
						return;
					}
#endif
					if (Netgame.team_vector & (1<<i))
						Netgame.team_vector&=(~(1<<i));
					else
						Netgame.team_vector|=(1<<i);

					range_for (auto &t, partial_const_range(Players, N_players))
						if (t.connected != player_connection_status::disconnected)
							multi_reset_object_texture(vmobjptr(t.objnum));
					reset_cockpit();

					multi_send_gmode_update();

					snprintf(Network_message.data(), Network_message.size(), "%s has changed teams!", static_cast<const char *>(vcplayerptr(i)->callsign));
					if (i==Player_num)
					{
						HUD_init_message_literal(HM_MULTI, "You have changed teams!");
						reset_cockpit();
					}
					else
						HUD_init_message(HM_MULTI, "Moving %s to other team.", static_cast<const char *>(vcplayerptr(i)->callsign));
					break;
				}
		}
	}

	else if (!d_strnicmp(Network_message.data(), "/kick: ") && (Game_mode & GM_NETWORK))
	{
		if (!multi_i_am_master())
		{
			HUD_init_message(HM_MULTI, "Only %s can kick others out!", static_cast<const char *>(Players[multi_who_is_master()].callsign));
			multi_message_index = 0;
			multi_sending_message[Player_num] = msgsend_state::none;
#if defined(DXX_BUILD_DESCENT_II)
			multi_send_msgsend_state(msgsend_state::none);
#endif
			return;
		}
		unsigned name_index=7;
		const auto nlen = strlen(Network_message.data());
		if (nlen > 7)
			while (Network_message[name_index] == ' ')
				name_index++;

		if (nlen <= name_index)
		{
			HUD_init_message_literal(HM_MULTI, "You must specify a name to kick");
			multi_message_index = 0;
			multi_sending_message[Player_num] = msgsend_state::none;
#if defined(DXX_BUILD_DESCENT_II)
			multi_send_msgsend_state(msgsend_state::none);
#endif
			return;
		}

		if (Network_message[name_index] == '#' && isdigit(Network_message[name_index+1])) {
			playernum_array_t players;
			int listpos = Network_message[name_index+1] - '0';

			if (Show_kill_list == show_kill_list_mode::_1 || Show_kill_list == show_kill_list_mode::efficiency)
			{
				if (listpos == 0 || listpos >= N_players) {
					HUD_init_message_literal(HM_MULTI, "Invalid player number for kick.");
					multi_message_index = 0;
					multi_sending_message[Player_num] = msgsend_state::none;
#if defined(DXX_BUILD_DESCENT_II)
					multi_send_msgsend_state(msgsend_state::none);
#endif
					return;
				}
				multi_get_kill_list(players);
				const auto i = players[listpos];
				if (i != Player_num && vcplayerptr(i)->connected != player_connection_status::disconnected)
				{
					kick_player(*vcplayerptr(i), Netgame.players[i]);
					return;
				}
			}
			else HUD_init_message_literal(HM_MULTI, "You cannot use # kicking with in team display.");


		    multi_message_index = 0;
		    multi_sending_message[Player_num] = msgsend_state::none;
#if defined(DXX_BUILD_DESCENT_II)
		    multi_send_msgsend_state(msgsend_state::none);
#endif
			return;
		}

		for (unsigned i = 0; i < N_players; i++)
			if (i != Player_num && vcplayerptr(i)->connected != player_connection_status::disconnected && !d_strnicmp(static_cast<const char *>(vcplayerptr(i)->callsign), &Network_message[name_index], nlen - name_index))
			{
				kick_player(*vcplayerptr(i), Netgame.players[i]);
				return;
			}
	}
	else if (!d_stricmp (Network_message.data(), "/killreactor") && (Game_mode & GM_NETWORK) && !LevelUniqueControlCenterState.Control_center_destroyed)
	{
		if (!multi_i_am_master())
			HUD_init_message(HM_MULTI, "Only %s can kill the reactor this way!", static_cast<const char *>(Players[multi_who_is_master()].callsign));
		else
		{
			net_destroy_controlcen_object(Robot_info, object_none);
			multi_send_destroy_controlcen(object_none,Player_num);
		}
		multi_message_index = 0;
		multi_sending_message[Player_num] = msgsend_state::none;
#if defined(DXX_BUILD_DESCENT_II)
		multi_send_msgsend_state(msgsend_state::none);
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
	multi_sending_message[Player_num] = msgsend_state::none;
	multi_send_msgsend_state(msgsend_state::none);
	key_toggle_repeat(0);
#endif
	game_flush_inputs(Controls);
}

static void multi_define_macro_end(control_info &Controls)
{
	const auto defining = multi_defining_message;
	if (!PlayerCfg.NetworkMessageMacro.valid_index(defining))
		return;
	multi_defining_message = multi_macro_message_index::None;
	PlayerCfg.NetworkMessageMacro[defining] = Network_message;
	write_player_file();

	multi_message_index = 0;
	key_toggle_repeat(0);
	game_flush_inputs(Controls);
}

}

window_event_result multi_message_input_sub(const d_robot_info_array &Robot_info, const int key, control_info &Controls)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	switch( key )
	{
		case KEY_F8:
		case KEY_ESC:
			multi_sending_message[Player_num] = msgsend_state::none;
			multi_send_msgsend_state(msgsend_state::none);
			multi_defining_message = multi_macro_message_index::None;
			key_toggle_repeat(0);
			game_flush_inputs(Controls);
			return window_event_result::handled;
		case KEY_LEFT:
		case KEY_BACKSP:
		case KEY_PAD4:
			if (multi_message_index > 0)
				multi_message_index--;
			Network_message[multi_message_index] = 0;
			return window_event_result::handled;
		case KEY_ENTER:
			if (multi_sending_message[Player_num] != msgsend_state::none)
				multi_send_message_end(Robot_info, vmobjptr, Controls);
			else if (multi_defining_message != multi_macro_message_index::None)
				multi_define_macro_end(Controls);
			game_flush_inputs(Controls);
			return window_event_result::handled;
		default:
		{
			int ascii = key_ascii();
			if ( ascii < 255 )     {
				if (multi_message_index < MAX_MESSAGE_LEN-2 )   {
					Network_message[multi_message_index++] = ascii;
					Network_message[multi_message_index] = 0;
				}
				else if (auto &sending = multi_sending_message[Player_num]; sending != msgsend_state::none)
				{
					const char *ptext = nullptr;
					Network_message[std::size(Network_message) - 2] = ascii;
					Network_message.back() = 0;
					const auto nmb = Network_message.begin();
					/* Start at the end of the message and scan backward for
					 * the first space.  Start the scan at the second-to-last
					 * element, since the last element was set to null above,
					 * and so is guaranteed not to be a space.
					 */
					for (auto nmi = std::prev(Network_message.end());;)
					{
						if (auto &c = * -- nmi; c == ' ')
						{
							/* Found a space.  Replace it with a null, and save
							 * a pointer to the first non-space after the
							 * space.
							 */
							c = 0;
							ptext = std::next(nmi);
							break;
						}
						/* The scan reached the beginning of the buffer without
						 * finding a space.  Break anyway to avoid scanning
						 * before the beginning of the buffer.
						 */
						if (nmi == nmb)
							break;
					}
					multi_send_message_end(Robot_info, vmobjptr, Controls);
					if ( ptext )    {
						sending = msgsend_state::typing;
						multi_send_msgsend_state(msgsend_state::typing);
						auto pcolon = strstr(Network_message.data(), ": " );
						std::move(ptext, std::cend(Network_message), pcolon ? std::next(pcolon, 2) : Network_message.data());
						multi_message_index = strlen( Network_message );
					}
				}
			}
		}
	}
	return window_event_result::ignored;
}

namespace {

static void multi_do_fire(fvmobjptridx &vmobjptridx, const playernum_t pnum, const multiplayer_rspan<multiplayer_command_t::MULTI_FIRE> buf, const icobjidx_t Network_laser_track, const std::optional<uint16_t> remote_objnum)
{
	// Act out the actual shooting
	const uint8_t untrusted_raw_weapon = buf[2];

	const auto flags{buf[4]};

	const auto shot_orientation = multi_get_vector(buf.subspan<5, 12>());

	Assert (pnum < N_players);

	const auto &&obj = vmobjptridx(vcplayerptr(pnum)->objnum);
	if (obj->type == OBJ_GHOST)
		multi_make_ghost_player(pnum);

	if (untrusted_raw_weapon == FLARE_ADJUST)
		Laser_player_fire(LevelSharedRobotInfoState.Robot_info, obj, weapon_id_type::FLARE_ID, gun_num_t::center, weapon_sound_flag::audible, shot_orientation, object_none);
	else if (const uint8_t untrusted_missile_adjusted_weapon = untrusted_raw_weapon - MISSILE_ADJUST; untrusted_missile_adjusted_weapon < MAX_SECONDARY_WEAPONS)
	{
		const auto weapon = secondary_weapon_index_t{untrusted_missile_adjusted_weapon};
		const auto weapon_id = Secondary_weapon_to_weapon_info[weapon];
		const auto base_weapon_gun = Secondary_weapon_to_gun_num[weapon];
		const auto weapon_gun = (base_weapon_gun == gun_num_t::_4)
			? static_cast<gun_num_t>(static_cast<uint8_t>(base_weapon_gun) + (flags & 1))
			: base_weapon_gun;

		const auto &&objnum = Laser_player_fire(LevelSharedRobotInfoState.Robot_info, obj, weapon_id, weapon_gun, weapon_sound_flag::audible, shot_orientation, Network_laser_track);
		if (remote_objnum)
			map_objnum_local_to_remote(objnum, *remote_objnum, pnum);
	}
	else if (const uint8_t untrusted_weapon = untrusted_raw_weapon; untrusted_weapon < MAX_PRIMARY_WEAPONS)
	{
		const auto weapon = primary_weapon_index_t{untrusted_weapon};
		if (weapon == primary_weapon_index_t::FUSION_INDEX) {
			obj->ctype.player_info.Fusion_charge = flags << 12;
		}
		if (weapon == primary_weapon_index_t::LASER_INDEX)
		{
			auto &powerup_flags = obj->ctype.player_info.powerup_flags;
			if (flags & LASER_QUAD)
				powerup_flags |= PLAYER_FLAGS_QUAD_LASERS;
			else
				powerup_flags &= ~PLAYER_FLAGS_QUAD_LASERS;
		}

		do_laser_firing(obj, weapon, laser_level{buf[3]}, flags, shot_orientation, Network_laser_track);
	}
}

}

}

namespace {

static void multi_do_message(const playernum_t pnum, const multiplayer_rspan<multiplayer_command_t::MULTI_MESSAGE> cbuf)
{
	const auto buf = reinterpret_cast<const char *>(cbuf.data());
	const char *colon;
	const char *msgstart;
	int loc = 2;

	if (((colon = strstr(buf+loc, ": ")) == NULL) || (colon-(buf+loc) < 1) || (colon-(buf+loc) > CALLSIGN_LEN))
	{
		msgstart = buf + 2;
	}
	else
	{
		/* if
		 * - it is addressed to me by name OR
		 * - team mode is active AND one of:
		 *	- it is addressed to the number of my team OR
		 *	- it is addressed to the name of my team
		 *
		 * then show it.  Otherwise, hide it.
		 */
		if ( (!d_strnicmp(static_cast<const char *>(get_local_player().callsign), buf+loc, colon-(buf+loc))) ||
			((Game_mode & GM_TEAM) && ({
				const auto local_player_team = get_team(Player_num);
				(buf + loc + 1 == colon && ({
					/* The length is correct, so this might or might not be a team number. */
					const auto o = Netgame.team_name.valid_index(buf[loc] - '1');
					o && local_player_team == *o;
					})
				) ||
				!d_strnicmp(Netgame.team_name[local_player_team], buf+loc, colon-(buf+loc));
			})))
		{
			msgstart = colon + 2;
		}
		else
			return;
	}
	const auto color = get_player_or_team_color(pnum);
	char xrgb = BM_XRGB(player_rgb[color].r,player_rgb[color].g,player_rgb[color].b);
	digi_play_sample(SOUND_HUD_MESSAGE, F1_0);
	HUD_init_message(HM_MULTI, "%c%c%s:%c%c %s", CC_COLOR, xrgb, static_cast<const char *>(vcplayerptr(pnum)->callsign), CC_COLOR, BM_XRGB(0, 31, 0), msgstart);
	multi_sending_message[pnum] = msgsend_state::none;
}

}

namespace dsx {

namespace {

static void multi_do_position(fvmobjptridx &vmobjptridx, const playernum_t pnum, const multiplayer_rspan<multiplayer_command_t::MULTI_POSITION> buf)
{
	const auto &&obj = vmobjptridx(vcplayerptr(pnum)->objnum);
        int count = 1;

        quaternionpos qpp{};
	qpp.orient.w = GET_INTEL_SHORT(&buf[count]);					count += 2;
	qpp.orient.x = GET_INTEL_SHORT(&buf[count]);					count += 2;
	qpp.orient.y = GET_INTEL_SHORT(&buf[count]);					count += 2;
	qpp.orient.z = GET_INTEL_SHORT(&buf[count]);					count += 2;
	qpp.pos = multi_get_vector(buf.subspan<9, 12>());
	count += 12;
	if (const auto s{vmsegidx_t::check_nothrow_index(GET_INTEL_SHORT(&buf[count]))})
	{
		qpp.segment = *s;
		count += 2;
	}
	else
		return;
	qpp.vel = multi_get_vector(buf.subspan<9 + 12 + 2, 12>());
	count += 12;
	qpp.rotvel = multi_get_vector(buf.subspan<9 + 12 + 2 + 12, 12>());
	count += 12;
	extract_quaternionpos(obj, qpp);

	if (obj->movement_source == object::movement_type::physics)
		set_thrust_from_velocity(obj);
}

static void multi_do_reappear(const playernum_t pnum, const multiplayer_rspan<multiplayer_command_t::MULTI_REAPPEAR> buf)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	const objnum_t objnum = GET_INTEL_SHORT(&buf[2]);

	const auto &&uobj = vcobjptr.check_untrusted(objnum);
	if (!uobj)
		return;
	auto &obj = **uobj;
	if (obj.type != OBJ_PLAYER && obj.type != OBJ_GHOST)
	{
		con_printf(CON_URGENT, "%s:%u: BUG: object %hu has type %u, expected %u or %u", __FILE__, __LINE__, objnum, obj.type, OBJ_PLAYER, OBJ_GHOST);
		return;
	}
	/* Override macro, call only the getter.
	 *
	 * This message is overloaded to be used on both players and ghosts,
	 * so the standard check cannot be used.  Instead, the correct check
	 * is open-coded above.
	 */
	if (pnum != (get_player_id)(obj))
		return;

	multi_make_ghost_player(pnum);
	create_player_appearance_effect(Vclip, obj);
}

static void multi_do_player_deres(const d_robot_info_array &Robot_info, object_array &Objects, const playernum_t pnum, const multiplayer_rspan<multiplayer_command_t::MULTI_PLAYER_DERES> buf)
{
	auto &vmobjptridx = Objects.vmptridx;
	auto &vmobjptr = Objects.vmptr;
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
#define GET_WEAPON_FLAGS(buf,count)	(count += sizeof(uint16_t), GET_INTEL_SHORT(&buf[(count - sizeof(uint16_t))]))
#endif
	const auto &&objp = vmobjptridx(vcplayerptr(pnum)->objnum);
	auto &player_info = objp->ctype.player_info;
	player_info.primary_weapon_flags = GET_WEAPON_FLAGS(buf, count);
	player_info.laser_level = laser_level{buf[count]};
	count++;
	if (game_mode_hoard(Game_mode))
		player_info.hoard.orbs = buf[count];
	count++;

	auto &secondary_ammo = player_info.secondary_ammo;
	secondary_ammo[secondary_weapon_index_t::HOMING_INDEX] = buf[count];                count++;
	secondary_ammo[secondary_weapon_index_t::CONCUSSION_INDEX] = buf[count];count++;
	secondary_ammo[secondary_weapon_index_t::SMART_INDEX] = buf[count];         count++;
	secondary_ammo[secondary_weapon_index_t::MEGA_INDEX] = buf[count];          count++;
	secondary_ammo[secondary_weapon_index_t::PROXIMITY_INDEX] = buf[count]; count++;

#if defined(DXX_BUILD_DESCENT_II)
	secondary_ammo[secondary_weapon_index_t::SMISSILE1_INDEX] = buf[count]; count++;
	secondary_ammo[secondary_weapon_index_t::GUIDED_INDEX]    = buf[count]; count++;
	secondary_ammo[secondary_weapon_index_t::SMART_MINE_INDEX]= buf[count]; count++;
	secondary_ammo[secondary_weapon_index_t::SMISSILE4_INDEX] = buf[count]; count++;
	secondary_ammo[secondary_weapon_index_t::SMISSILE5_INDEX] = buf[count]; count++;
#endif

	player_info.vulcan_ammo = GET_INTEL_SHORT(&buf[count]); count += 2;
	player_info.powerup_flags = player_flags(GET_INTEL_INT(&buf[count]));    count += 4;

	//      objp->phys_info.velocity = *reinterpret_cast<vms_vector *>(buf+16); // 12 bytes
	//      objp->pos = *reinterpret_cast<vms_vector *>(buf+28);                // 12 bytes

	remote_created = buf[count++]; // How many did the other guy create?

	Net_create_loc = 0;
	drop_player_eggs(objp);

	// Create mapping from remote to local numbering system

	// We now handle this situation gracefully, Int3 not required
	//      if (Net_create_loc != remote_created)
	//              Int3(); // Probably out of object array space, see Rob

	range_for (const auto i, partial_const_range(Net_create_objnums, std::min(Net_create_loc, static_cast<unsigned>(remote_created))))
	{
		short s;

		s = GET_INTEL_SHORT(&buf[count]);

		if ((s > 0) && (i > 0))
			map_objnum_local_to_remote(static_cast<short>(i), s, pnum);
		count += 2;
	}
	range_for (const auto i, partial_const_range(Net_create_objnums, remote_created, Net_create_loc))
	{
		vmobjptr(i)->flags |= OF_SHOULD_BE_DEAD;
	}

	if (buf[2] == deres_explode)
	{
		explode_badass_player(Robot_info, objp);

		objp->flags &= ~OF_SHOULD_BE_DEAD;              //don't really kill player
		multi_make_player_ghost(pnum);
	}
	else
	{
		create_player_appearance_effect(Vclip, objp);
	}

	player_info.powerup_flags &= ~(PLAYER_FLAGS_CLOAKED | PLAYER_FLAGS_INVULNERABLE);
#if defined(DXX_BUILD_DESCENT_II)
	player_info.powerup_flags &= ~PLAYER_FLAGS_FLAG;
#endif
	DXX_MAKE_VAR_UNDEFINED(player_info.cloak_time);
}

/*
 * Process can compute a kill. If I am a Client this might be my own one (see multi_send_kill()) but with more specific data so I can compute my kill correctly.
 */
static void multi_do_kill_host(object_array &Objects, const playernum_t pnum, const multiplayer_rspan<multiplayer_command_t::MULTI_KILL_HOST> buf)
{
	int count = 1;

	if (multi_i_am_master())
		return;
	const auto killed = vcplayerptr(pnum)->objnum;
	count += 1;
	objnum_t killer = GET_INTEL_SHORT(&buf[count]);
	if (killer > 0)
		killer = objnum_remote_to_local(killer, buf[count+2]);
	Netgame.team_vector = buf[5];
	Bounty_target = buf[6];

	multi_compute_kill(LevelSharedRobotInfoState.Robot_info, Objects.imptridx(killer), Objects.vmptridx(killed));
}

static void multi_do_kill_client(object_array &Objects, const playernum_t pnum, const multiplayer_rspan<multiplayer_command_t::MULTI_KILL_CLIENT> buf)
{
	if (!multi_i_am_master())
		return;
	int count = 1;
	// I am host, I know what's going on so take this packet, add game_mode related info which might be necessary for kill computation and send it to everyone so they can compute their kills correctly
	{
		multi_command<multiplayer_command_t::MULTI_KILL_HOST> multibuf;
		std::memcpy(std::next(multibuf.data()), std::next(buf.data()), 4);
		multibuf[5] = Netgame.team_vector;
		multibuf[6] = Bounty_target;
		
		multi_send_data(multibuf, multiplayer_data_priority::_2);
	}

	const auto killed = vcplayerptr(pnum)->objnum;
	count += 1;
	objnum_t killer = GET_INTEL_SHORT(&buf[count]);
	if (killer > 0)
		killer = objnum_remote_to_local(killer, buf[count+2]);

	multi_compute_kill(LevelSharedRobotInfoState.Robot_info, Objects.imptridx(killer), Objects.vmptridx(killed));

	if (Game_mode & GM_BOUNTY) // update in case if needed... we could attach this to this packet but... meh...
		multi_send_bounty();
}

//      Changed by MK on 10/20/94 to send NULL as object to net_destroy_controlcen if it got -1
// which means not a controlcen object, but contained in another object
static void multi_do_controlcen_destroy(const d_robot_info_array &Robot_info, fimobjptridx &imobjptridx, const multiplayer_rspan<multiplayer_command_t::MULTI_CONTROLCEN> buf)
{
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	objnum_t objnum = GET_INTEL_SHORT(&buf[1]);
	const playernum_t who = buf[3];

	if (LevelUniqueControlCenterState.Control_center_destroyed != 1)
	{
		if ((who < N_players) && (who != Player_num)) {
			HUD_init_message(HM_MULTI, "%s %s", static_cast<const char *>(vcplayerptr(who)->callsign), TXT_HAS_DEST_CONTROL);
		}
		else
			HUD_init_message_literal(HM_MULTI, who == Player_num ? ( { const auto &&m = TXT_YOU_DEST_CONTROL; std::span<const char>(m, strlen(m)); }) : ( { const auto &&m = TXT_CONTROL_DESTROYED; std::span<const char>(m, strlen(m)); }));

		net_destroy_controlcen_object(Robot_info, objnum == object_none ? object_none : imobjptridx(objnum));
	}
}

static void multi_do_escape(fvmobjptridx &vmobjptridx, const playernum_t pnum, const multiplayer_rspan<multiplayer_command_t::MULTI_ENDLEVEL_START> buf)
{
	digi_play_sample(SOUND_HUD_MESSAGE, F1_0);
	auto &plr = *vmplayerptr(pnum);
	const auto &&objnum = vmobjptridx(plr.objnum);
#if defined(DXX_BUILD_DESCENT_II)
	digi_kill_sound_linked_to_object (objnum);
#endif

	const char *txt;
	player_connection_status connected;
#if defined(DXX_BUILD_DESCENT_I)
	if (buf[2] == static_cast<uint8_t>(multi_endlevel_type::secret))
	{
		txt = TXT_HAS_FOUND_SECRET;
		connected = player_connection_status::found_secret;
	}
	else
#endif
	{
		txt = TXT_HAS_ESCAPED;
		connected = player_connection_status::escape_tunnel;
	}
	HUD_init_message(HM_MULTI, "%s %s", static_cast<const char *>(plr.callsign), txt);
	if (Game_mode & GM_NETWORK)
		plr.connected = connected;
	create_player_appearance_effect(Vclip, objnum);
	multi_make_player_ghost(buf[1]);
}

static void multi_do_remobj(fvmobjptr &vmobjptr, const multiplayer_rspan<multiplayer_command_t::MULTI_REMOVE_OBJECT> buf)
{
	// which object to remove
	const objnum_t objnum = GET_INTEL_SHORT(&buf[1]);
	// which remote list is it entered in
	auto obj_owner = buf[3];

	if (objnum < 1)
		return;

	const auto local_objnum = objnum_remote_to_local(objnum, obj_owner); // translate to local objnum
	if (local_objnum == object_none)
	{
		return;
	}

	auto &obj = *vmobjptr(local_objnum);
	if (obj.type != OBJ_POWERUP && obj.type != OBJ_HOSTAGE)
	{
		return;
	}

	if (Network_send_objects && multi::dispatch->objnum_is_past(local_objnum))
	{
		Network_send_objnum = -1;
	}

	obj.flags |= OF_SHOULD_BE_DEAD; // quick and painless
}

}

}

void multi_disconnect_player(const playernum_t pnum)
{
	if (!(Game_mode & GM_NETWORK))
		return;
	if (vcplayerptr(pnum)->connected == player_connection_status::disconnected)
		return;

	if (vcplayerptr(pnum)->connected == player_connection_status::playing)
	{
		digi_play_sample( SOUND_HUD_MESSAGE, F1_0 );
		HUD_init_message(HM_MULTI,  "%s %s", static_cast<const char *>(vcplayerptr(pnum)->callsign), TXT_HAS_LEFT_THE_GAME);

		multi_sending_message[pnum] = msgsend_state::none;

		if (Network_status == network_state::playing)
		{
			multi_make_player_ghost(pnum);
			multi_strip_robots(pnum);
		}

		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_multi_disconnect(pnum);

		// Bounty target left - select a new one
		if( Game_mode & GM_BOUNTY && pnum == Bounty_target && multi_i_am_master() )
		{
			update_bounty_target();
			/* Send this new data */
			multi_send_bounty();
		}
	}

	vmplayerptr(pnum)->connected = player_connection_status::disconnected;
	Netgame.players[pnum].connected = player_connection_status::disconnected;

	multi::dispatch->disconnect_player(pnum);

	if (pnum == multi_who_is_master()) // Host has left - Quit game!
	{
		const auto g = Game_wind;
		if (g)
			g->set_visible(0);
		struct host_left_game : passive_messagebox
		{
			host_left_game() :
				passive_messagebox(menu_title{nullptr}, menu_subtitle{"Host left the game!"}, TXT_OK, grd_curscreen->sc_canvas)
				{
				}
		};
		run_blocking_newmenu<host_left_game>();
		if (g)
			g->set_visible(1);
		multi_quit_game = 1;
		game_leave_menus();
		return;
	}

	int n = 0;
	range_for (auto &i, partial_const_range(Players, N_players))
		if (i.connected != player_connection_status::disconnected)
			if (++n > 1)
				break;
	if (n == 1)
	{
		HUD_init_message_literal(HM_MULTI, "You are the only person remaining in this netgame");
	}
}

namespace {

static void multi_do_quit(const playernum_t pnum)
{
	multi_disconnect_player(pnum);
}

}

namespace dsx {

namespace {

static void multi_do_cloak(fvmobjptr &vmobjptr, const playernum_t pnum)
{
	Assert(pnum < N_players);

	const auto &&objp = vmobjptr(vcplayerptr(pnum)->objnum);
	auto &player_info = objp->ctype.player_info;
	player_info.powerup_flags |= PLAYER_FLAGS_CLOAKED;
	player_info.cloak_time = {GameTime64};
	ai_do_cloak_stuff();

	multi_strip_robots(pnum);

	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_multi_cloak(pnum);
}

}

}

namespace dcx {

namespace {

static const char *deny_multi_save_game_duplicate_callsign(const std::ranges::subrange<const player *> range)
{
	const auto e = range.end();
	for (auto i = range.begin(); i != e; ++i)
		for (auto j = std::next(i); j != e; ++j)
		{
			if (i->callsign == j->callsign)
				return "Multiple players with same callsign!";
		}
	return nullptr;
}

static void multi_do_decloak(const playernum_t pnum)
{
	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_multi_decloak(pnum);
}

}

}

namespace dsx {

namespace {

static void multi_do_door_open(fvmwallptr &vmwallptr, const multiplayer_rspan<multiplayer_command_t::MULTI_DOOR_OPEN> buf)
{
	const auto uside = build_sidenum_from_untrusted(buf[3]);
	if (!uside)
		return;
	const auto side = *uside;
#if defined(DXX_BUILD_DESCENT_II)
	ubyte flag= buf[4];
#endif

	const auto &&useg = vmsegptridx.check_untrusted(segnum_t{GET_INTEL_SHORT(&buf[1])});
	if (!useg)
		return;
	const auto &&seg = *useg;
	const auto wall_num = seg->shared_segment::sides[side].wall_num;
	if (wall_num == wall_none) {  //Opening door on illegal wall
		Int3();
		return;
	}

	auto &w = *vmwallptr(wall_num);

	if (w.type == WALL_BLASTABLE)
	{
		if (!(w.flags & wall_flag::blasted))
		{
			wall_destroy(seg, side);
		}
		return;
	}
	else if (w.state != wall_state::opening)
	{
		wall_open_door(seg, side);
	}
#if defined(DXX_BUILD_DESCENT_II)
	w.flags = wall_flags{flag};
#endif

}

static void multi_do_create_explosion(fvmobjptridx &vmobjptridx, const playernum_t pnum)
{
	create_small_fireball_on_object(vmobjptridx(vcplayerptr(pnum)->objnum), F1_0, 1);
}

static void multi_do_controlcen_fire(const multiplayer_rspan<multiplayer_command_t::MULTI_CONTROLCEN_FIRE> buf)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	int gun_num;
	objnum_t objnum;
	int count = 1;

	const auto to_target = multi_get_vector(buf.subspan<1, 12>());
	count += 12;
	gun_num = buf[count];                       count += 1;
	objnum = GET_INTEL_SHORT(&buf[count]);      count += 2;

	const auto &&uobj = vmobjptridx.check_untrusted(objnum);
	if (!uobj)
		return;
	const auto &&objp = *uobj;
	Laser_create_new_easy(LevelSharedRobotInfoState.Robot_info, to_target, objp->ctype.reactor_info.gun_pos[gun_num], objp, weapon_id_type::CONTROLCEN_WEAPON_NUM, weapon_sound_flag::audible);
}

static void multi_do_create_powerup(fvmsegptridx &vmsegptridx, const playernum_t pnum, const multiplayer_rspan<multiplayer_command_t::MULTI_CREATE_POWERUP> buf)
{
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	int count = 1;
	if (Network_status == network_state::endlevel || LevelUniqueControlCenterState.Control_center_destroyed)
		return;

	count++;
	const uint8_t powerup_type{buf[count++]};
	if (powerup_type >= MAX_POWERUP_TYPES)
		return;
	/* Casting the untrusted network input to segnum_t is safe here, since it
	 * is immediately passed to `check_untrusted`, which validates that the
	 * index is reasonable.
	 */
	const auto &&useg = vmsegptridx.check_untrusted(segnum_t{GET_INTEL_SHORT(&buf[count])});
	if (!useg)
		return;
	const auto &&segnum = *useg;
	count += 2;
	objnum_t objnum = GET_INTEL_SHORT(&buf[count]); count += 2;
	const auto new_pos = multi_get_vector(buf.subspan<1 + 1 + 1 + 2 + 2, 12>());
	count+=sizeof(vms_vector);
	const auto &&my_objnum{drop_powerup(LevelUniqueObjectState, LevelSharedSegmentState, LevelUniqueSegmentState, Vclip, powerup_type_t{powerup_type}, vmd_zero_vector, new_pos, segnum, true)};
	if (my_objnum == object_none)
		return;

	if (Network_send_objects && multi::dispatch->objnum_is_past(my_objnum))
	{
		Network_send_objnum = -1;
	}

	map_objnum_local_to_remote(my_objnum, objnum, pnum);

	object_create_explosion_without_damage(Vclip, segnum, new_pos, i2f(5), vclip_index::powerup_disappearance);
}

static void multi_do_play_sound(object_array &Objects, const playernum_t pnum, const multiplayer_rspan<multiplayer_command_t::MULTI_PLAY_SOUND> buf)
{
	auto &vcobjptridx = Objects.vcptridx;
	const auto &plr = *vcplayerptr(pnum);
	if (plr.connected == player_connection_status::disconnected)
		return;

	const unsigned sound_num = buf[2];
	const uint8_t once = buf[3];
	const fix volume{GET_INTEL_INT<int32_t>(&buf[4])};

	const auto objnum = plr.objnum;
	digi_link_sound_to_object(sound_num, vcobjptridx(objnum), 0, volume, static_cast<sound_stack>(once));
}

}

}

namespace {

static void multi_do_score(fvmobjptr &vmobjptr, const playernum_t pnum, const multiplayer_rspan<multiplayer_command_t::MULTI_SCORE> buf)
{
	if (pnum >= N_players)
	{
		Int3(); // Non-terminal, see rob
		return;
	}

	const auto score{GET_INTEL_INT<int32_t>(&buf[2])};
	if (Newdemo_state == ND_STATE_RECORDING) {
		newdemo_record_multi_score(pnum, score);
	}
	auto &player_info = vmobjptr(vcplayerptr(pnum)->objnum)->ctype.player_info;
	player_info.mission.score = score;
	multi_sort_kill_list();
}

static void multi_do_trigger(const playernum_t pnum, const multiplayer_rspan<multiplayer_command_t::MULTI_TRIGGER> buf)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	const std::underlying_type<trgnum_t>::type trigger = buf[2];
	if (pnum >= N_players || pnum == Player_num)
	{
		Int3(); // Got trigger from illegal playernum
		return;
	}
	auto &Triggers = LevelUniqueWallSubsystemState.Triggers;
	if (trigger >= Triggers.get_count())
	{
		Int3(); // Illegal trigger number in multiplayer
		return;
	}
	check_trigger_sub(get_local_plrobj(), static_cast<trgnum_t>(trigger), pnum, 0);
}

}

#if defined(DXX_BUILD_DESCENT_II)
namespace dsx {

namespace {

static void multi_do_effect_blowup(const playernum_t pnum, const multiplayer_rspan<multiplayer_command_t::MULTI_EFFECT_BLOWUP> buf)
{
	if (pnum >= N_players || pnum == Player_num)
		return;

	multi::dispatch->do_protocol_frame(1, 0); // force packets to be sent, ensuring this packet will be attached to following MULTI_TRIGGER

	const auto &&useg = vmsegptridx.check_untrusted(segnum_t{GET_INTEL_SHORT(&buf[2])});
	if (!useg)
		return;
	const auto uside = build_sidenum_from_untrusted(buf[4]);
	if (!uside)
		return;
	const auto side = *uside;
	const auto hitpnt = multi_get_vector(buf.subspan<5, 12>());

	//create a dummy object which will be the weapon that hits
	//the monitor. the blowup code wants to know who the parent of the
	//laser is, so create a laser whose parent is the player
	laser_parent laser;
	laser.parent_type = OBJ_PLAYER;
	laser.parent_num = pnum;

	auto &LevelSharedDestructibleLightState = LevelSharedSegmentState.DestructibleLights;
	check_effect_blowup(LevelSharedDestructibleLightState, Vclip, *useg, side, hitpnt, laser, 0, 1);
}

static void multi_do_drop_marker(object_array &Objects, fvmsegptridx &vmsegptridx, const playernum_t pnum, const multiplayer_rspan<multiplayer_command_t::MULTI_MARKER> buf)
{
	if (pnum==Player_num)  // my marker? don't set it down cuz it might screw up the orientation
		return;

	const auto mesnum = buf[2];
	const auto game_mode = Game_mode;
	const auto max_numplayers = Netgame.max_numplayers;
	if (mesnum >= MarkerState.get_markers_per_player(game_mode, max_numplayers))
		return;

	const auto position = multi_get_vector(buf.subspan<3, 12>());

	const auto gmi = convert_player_marker_index_to_game_marker_index(game_mode, max_numplayers, pnum, player_marker_index{mesnum});
	auto &marker_message = MarkerState.message[gmi];
	marker_message = {};
	for (const auto i : xrange(marker_message.size()))
	{
		const auto c = marker_message[i] = buf[15 + i];
		if (!c)
			break;
	}

	auto &mo = MarkerState.imobjidx[gmi];
	if (mo != object_none)
		obj_delete(LevelUniqueObjectState, Segments, Objects.vmptridx(mo));

	const auto &&plr_objp = Objects.vcptr(vcplayerptr(pnum)->objnum);
	mo = drop_marker_object(position, vmsegptridx(plr_objp->segnum), plr_objp->orient, gmi);
}

}

}
#endif

namespace {

static void multi_do_hostage_door_status(fvmsegptridx &vmsegptridx, wall_array &Walls, const multiplayer_rspan<multiplayer_command_t::MULTI_HOSTAGE_DOOR> buf)
{
	// Update hit point status of a door

	int count = 1;

	const wallnum_t wallnum{GET_INTEL_SHORT(&buf[count])};
	count += 2;
	const fix hps{GET_INTEL_INT<int32_t>(&buf[count])};           count += 4;

	auto &vmwallptr = Walls.vmptr;
	auto &w = *vmwallptr(wallnum);
	if (hps < 0 || w.type != WALL_BLASTABLE)
	{
		Int3(); // Non-terminal, see Rob
		return;
	}

	if (hps < w.hps)
		wall_damage(vmsegptridx(w.segnum), w.sidenum, w.hps - hps);
}

}

void multi_reset_stuff()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	// A generic, emergency function to solve problems that crop up
	// when a player exits quick-out from the game because of a
	// connection loss.  Fixes several weird bugs!

	dead_player_end();
	get_local_plrobj().ctype.player_info.homing_object_dist = -1; // Turn off homing sound.
	reset_rear_view();
}

namespace dsx {

void multi_reset_player_object(object &objp)
{
	//Init physics for a non-console player
	Assert((objp.type == OBJ_PLAYER) || (objp.type == OBJ_GHOST));

	objp.mtype.phys_info.velocity = {};
	objp.mtype.phys_info.thrust = {};
	objp.mtype.phys_info.rotvel = {};
	objp.mtype.phys_info.rotthrust = {};
	objp.mtype.phys_info.turnroll = 0;
	objp.mtype.phys_info.mass = Player_ship->mass;
	objp.mtype.phys_info.drag = Player_ship->drag;
	if (objp.type == OBJ_PLAYER)
		objp.mtype.phys_info.flags = PF_TURNROLL | PF_WIGGLE | PF_USES_THRUST;
	else
		objp.mtype.phys_info.flags &= ~(PF_TURNROLL | PF_LEVELLING | PF_WIGGLE);

	//Init render info

	objp.render_type = render_type::RT_POLYOBJ;
	objp.rtype.pobj_info.model_num = Player_ship->model_num;               //what model is this?
	objp.rtype.pobj_info.subobj_flags = 0;         //zero the flags
	objp.rtype.pobj_info.anim_angles = {};

	// Clear misc

	objp.flags = 0;

	if (objp.type == OBJ_GHOST)
		objp.render_type = render_type::RT_NONE;
	//reset textures for this, if not player 0
	multi_reset_object_texture (objp);
}

namespace {

static void multi_reset_object_texture(object_base &objp)
{
	if (objp.type == OBJ_GHOST)
                return;

	const auto player_id = get_player_id(objp);
	const auto id = (Game_mode & GM_TEAM)
		? static_cast<unsigned>(get_team(player_id))
		: player_id;

	auto &pobj_info = objp.rtype.pobj_info;
	pobj_info.alt_textures = id;
	if (id)
	{
		auto &Polygon_models = LevelSharedPolygonModelState.Polygon_models;
		auto &model = Polygon_models[pobj_info.model_num];
		const unsigned n_textures = model.n_textures;
		if (N_PLAYER_SHIP_TEXTURES < n_textures)
			Error("Too many player ship textures!\n");

		const unsigned first_texture = model.first_texture;
		for (unsigned i = 0; i < n_textures; ++i)
			multi_player_textures[id - 1][i] = ObjBitmaps[ObjBitmapPtrs[first_texture + i]];

		multi_player_textures[id-1][4] = ObjBitmaps[ObjBitmapPtrs[First_multi_bitmap_num+(id-1)*2]];
		multi_player_textures[id-1][5] = ObjBitmaps[ObjBitmapPtrs[First_multi_bitmap_num+(id-1)*2+1]];
	}
}

static std::optional<multiplayer_command_t> build_multiplayer_command_from_untrusted(const uint8_t type)
{
	switch (type)
	{
#define multiplayer_case(NAME, LENGTH)	\
		case static_cast<uint8_t>(multiplayer_command_t::NAME):
			for_each_multiplayer_command(multiplayer_case);
			return static_cast<multiplayer_command_t>(type);
#undef multiplayer_case
		default:
			return std::nullopt;
	}
}

}

void multi_process_bigdata(const d_level_shared_robot_info_state &LevelSharedRobotInfoState, const playernum_t pnum, const std::span<const uint8_t> buf)
{
	// Takes a bunch of messages, check them for validity,
	// and pass them to multi_process_data.

	uint_fast32_t bytes_processed = 0;

	while (bytes_processed < buf.size())
	{
		const uint_fast32_t type = buf[bytes_processed];

		if (type >= std::size(message_length))
		{
			con_printf(CON_DEBUG, "multi_process_bigdata: Invalid packet type %" PRIuFAST32 "!", type);
			return;
		}
		const auto mtype = build_multiplayer_command_from_untrusted(type);
		if (!mtype)
			return;
		const uint_fast32_t sub_len = message_length[type];

		Assert(sub_len > 0);

		if (bytes_processed + sub_len > buf.size())
		{
			con_printf(CON_DEBUG, "multi_process_bigdata: packet type %" PRIuFAST32 " too short (%" PRIuFAST32 " > %" DXX_PRI_size_type ")!", type, bytes_processed + sub_len, buf.size());
			Int3();
			return;
		}

		multi_process_data(LevelSharedRobotInfoState, pnum, buf.subspan(bytes_processed, sub_len), *mtype);
		bytes_processed += sub_len;
	}
}

//
// Part 2 : Functions that send communication messages to inform the other
//          players of something we did.
//

void multi_send_fire(const vms_matrix &orient, int laser_gun, const laser_level level, int laser_flags, objnum_t laser_track, const imobjptridx_t is_bomb_objnum)
{
	static fix64 last_fireup_time = 0;

	// provoke positional update if possible (20 times per second max. matches vulcan, the fastest firing weapon)
	if (timer_query() >= (last_fireup_time+(F1_0/20)))
	{
		multi::dispatch->do_protocol_frame(1, 0);
		last_fireup_time = timer_query();
	}

	union mb {
		multi_command<multiplayer_command_t::MULTI_FIRE_BOMB> multibomb;
		multi_command<multiplayer_command_t::MULTI_FIRE_TRACK> multitrack;
		multi_command<multiplayer_command_t::MULTI_FIRE> multifire;
		mb() {}
	} multibuf;
	if (is_bomb_objnum != object_none)
		new(&multibuf.multibomb) multi_command<multiplayer_command_t::MULTI_FIRE_BOMB>();
	else if (laser_track != object_none)
		new(&multibuf.multitrack) multi_command<multiplayer_command_t::MULTI_FIRE_TRACK>();
	else
		new(&multibuf.multifire) multi_command<multiplayer_command_t::MULTI_FIRE>();
	multibuf.multifire[1] = static_cast<char>(Player_num);
	multibuf.multifire[2] = static_cast<char>(laser_gun);
	multibuf.multifire[3] = static_cast<uint8_t>(level);
	multibuf.multifire[4] = static_cast<char>(laser_flags);

	multi_put_vector(&multibuf.multifire[5], orient.fvec);

	/*
	 * If we fire a bomb, it's persistent. Let others know of it's objnum so host can track it's behaviour over clients (host-authority functions, D2 chaff ability).
	 * If we fire a tracking projectile, we should others let know about what we track but we have to pay attention that it is mapped correctly.
	 * If we fire something else, we make the packet as small as possible.
	 */
	if (is_bomb_objnum != object_none)
	{
		map_objnum_local_to_local(is_bomb_objnum);
		PUT_INTEL_SHORT(&multibuf.multibomb[17], is_bomb_objnum.operator objnum_t());
		multi_send_data(multibuf.multibomb, multiplayer_data_priority::_1);
	}
	else if (laser_track != object_none)
	{
		const auto &&[remote_owner, remote_laser_track] = objnum_local_to_remote(laser_track);
		PUT_INTEL_SHORT(&multibuf.multitrack[17], remote_laser_track);
		multibuf.multitrack[19] = remote_owner;
		multi_send_data(multibuf.multitrack, multiplayer_data_priority::_1);
	}
	else
		multi_send_data(multibuf.multifire, multiplayer_data_priority::_1);
}

void multi_send_destroy_controlcen(const objnum_t objnum, const playernum_t player)
{
	if (player == Player_num)
	{
		const auto &&m = TXT_YOU_DEST_CONTROL;
		HUD_init_message_literal(HM_MULTI, {m, strlen(m)});
	}
	else if ((player > 0) && (player < N_players))
		HUD_init_message(HM_MULTI, "%s %s", static_cast<const char *>(vcplayerptr(player)->callsign), TXT_HAS_DEST_CONTROL);
	else
	{
		const auto &&m = TXT_CONTROL_DESTROYED;
		HUD_init_message_literal(HM_MULTI, {m, strlen(m)});
	}

	multi_command<multiplayer_command_t::MULTI_CONTROLCEN> multibuf;
	PUT_INTEL_SHORT(&multibuf[1], objnum);
	multibuf[3] = player;
	multi_send_data(multibuf, multiplayer_data_priority::_2);
}

#if defined(DXX_BUILD_DESCENT_II)
void multi_send_drop_marker(const unsigned player, const vms_vector &position, const player_marker_index messagenum, const marker_message_text_t &text)
{
	multi_command<multiplayer_command_t::MULTI_MARKER> multibuf;
		multibuf[1]=static_cast<char>(player);
		multibuf[2] = static_cast<uint8_t>(messagenum);
	multi_put_vector(&multibuf[3], position);
	std::copy(text.begin(), text.end(), std::next(multibuf.begin(), 15));
	multi_send_data(multibuf, multiplayer_data_priority::_2);
}

void multi_send_markers()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	// send marker positions/text to new player

	const auto game_mode = Game_mode;
	const auto max_numplayers = Netgame.max_numplayers;
	auto &&player_marker_range = get_player_marker_range(MarkerState.get_markers_per_player(game_mode, max_numplayers));
	for (const playernum_t pnum : xrange(max_numplayers))
	{
		for (const auto pmi : player_marker_range)
		{
			const auto gmi = convert_player_marker_index_to_game_marker_index(game_mode, max_numplayers, pnum, pmi);
			const auto mo = MarkerState.imobjidx[gmi];
			if (mo != object_none)
				multi_send_drop_marker(pnum, vcobjptr(mo)->pos, pmi, MarkerState.message[gmi]);
		}
	}
}
#endif

#if defined(DXX_BUILD_DESCENT_I)
void multi_send_endlevel_start(const multi_endlevel_type secret)
#elif defined(DXX_BUILD_DESCENT_II)
void multi_send_endlevel_start()
#endif
{
	multi_command<multiplayer_command_t::MULTI_ENDLEVEL_START> buf;
	/* Obsolete - reclaim player number field on next multiplayer protocol version bump */
	buf[1] = Player_num;
#if defined(DXX_BUILD_DESCENT_I)
	buf[2] = static_cast<uint8_t>(secret);
#endif

	multi_send_data(buf, multiplayer_data_priority::_2);
	if (Game_mode & GM_NETWORK)
	{
		get_local_player().connected = player_connection_status::escape_tunnel;
		multi::dispatch->send_endlevel_packet();
	}
}

void multi_send_player_deres(deres_type_t type)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	auto &vmobjptridx = Objects.vmptridx;
	int count = 0;
	if (Network_send_objects)
	{
		Network_send_objnum = -1;
	}

	multi_send_position(vmobjptridx(get_local_player().objnum));

	multi_command<multiplayer_command_t::MULTI_PLAYER_DERES> multibuf;
	count++;
	multibuf[count++] = Player_num;
	multibuf[count++] = type;

#if defined(DXX_BUILD_DESCENT_I)
#define PUT_WEAPON_FLAGS(buf,count,value)	(buf[count] = value, ++count)
#elif defined(DXX_BUILD_DESCENT_II)
#define PUT_WEAPON_FLAGS(buf,count,value)	((PUT_INTEL_SHORT(&buf[count], value)), count+=sizeof(uint16_t))
#endif
	auto &player_info = get_local_plrobj().ctype.player_info;
	PUT_WEAPON_FLAGS(multibuf, count, player_info.primary_weapon_flags);
	multibuf[count++] = static_cast<char>(player_info.laser_level);
	multibuf[count++] = game_mode_hoard(Game_mode) ? static_cast<char>(player_info.hoard.orbs) : 0;

	auto &secondary_ammo = player_info.secondary_ammo;
	multibuf[count++] = secondary_ammo[secondary_weapon_index_t::HOMING_INDEX];
	multibuf[count++] = secondary_ammo[secondary_weapon_index_t::CONCUSSION_INDEX];
	multibuf[count++] = secondary_ammo[secondary_weapon_index_t::SMART_INDEX];
	multibuf[count++] = secondary_ammo[secondary_weapon_index_t::MEGA_INDEX];
	multibuf[count++] = secondary_ammo[secondary_weapon_index_t::PROXIMITY_INDEX];

#if defined(DXX_BUILD_DESCENT_II)
	multibuf[count++] = secondary_ammo[secondary_weapon_index_t::SMISSILE1_INDEX];
	multibuf[count++] = secondary_ammo[secondary_weapon_index_t::GUIDED_INDEX];
	multibuf[count++] = secondary_ammo[secondary_weapon_index_t::SMART_MINE_INDEX];
	multibuf[count++] = secondary_ammo[secondary_weapon_index_t::SMISSILE4_INDEX];
	multibuf[count++] = secondary_ammo[secondary_weapon_index_t::SMISSILE5_INDEX];
#endif

	PUT_INTEL_SHORT(&multibuf[count], player_info.vulcan_ammo);
	count += 2;
	PUT_INTEL_INT(&multibuf[count], player_info.powerup_flags.get_player_flags());
	count += 4;

	multibuf[count++] = Net_create_loc;

	Assert(Net_create_loc <= MAX_NET_CREATE_OBJECTS);

	memset(&multibuf[count], -1, MAX_NET_CREATE_OBJECTS*sizeof(short));

	range_for (const auto i, partial_const_range(Net_create_objnums, Net_create_loc))
	{
		if (i <= 0) {
			Int3(); // Illegal value in created egg object numbers
			count +=2;
			continue;
		}

		PUT_INTEL_SHORT(&multibuf[count], i); count += 2;

		// We created these objs so our local number = the network number
		map_objnum_local_to_local(i);
	}

	Net_create_loc = 0;

	if (count > command_length<multiplayer_command_t::MULTI_PLAYER_DERES>)
	{
		Int3(); // See Rob
	}

	multi_send_data(multibuf, multiplayer_data_priority::_2);
	if (player_info.powerup_flags & PLAYER_FLAGS_CLOAKED)
		multi_send_decloak();
	multi_strip_robots(Player_num);
}

}

namespace {

void multi_send_message()
{
	if (Network_message_reciever != -1)
	{
		multi_command<multiplayer_command_t::MULTI_MESSAGE> multibuf;
		/* Obsolete - reclaim player number field on next multiplayer protocol version bump */
		multibuf[1] = Player_num;
		std::copy(Network_message.begin(), Network_message.end(), std::next(multibuf.begin(), 2));
		multi_send_data(multibuf, multiplayer_data_priority::_0);
		Network_message_reciever = -1;
	}
}

}

void multi_send_reappear()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	auto &plr = get_local_player();
	multi_send_position(vmobjptridx(plr.objnum));
	multi_command<multiplayer_command_t::MULTI_REAPPEAR> multibuf;
	multibuf[1] = static_cast<char>(Player_num);
	PUT_INTEL_SHORT(&multibuf[2], plr.objnum);

	multi_send_data(multibuf, multiplayer_data_priority::_2);
}

namespace dsx {

void multi_send_position(object &obj)
{
	int count=1;

	const auto qpp{build_quaternionpos(obj)};
	multi_command<multiplayer_command_t::MULTI_POSITION> multibuf;
	PUT_INTEL_SHORT(&multibuf[count], qpp.orient.w);							count += 2;
	PUT_INTEL_SHORT(&multibuf[count], qpp.orient.x);							count += 2;
	PUT_INTEL_SHORT(&multibuf[count], qpp.orient.y);							count += 2;
	PUT_INTEL_SHORT(&multibuf[count], qpp.orient.z);							count += 2;
	multi_put_vector(&multibuf[count], qpp.pos);
	count += 12;
	PUT_INTEL_SEGNUM(&multibuf[count], qpp.segment);					count += 2;
	multi_put_vector(&multibuf[count], qpp.vel);
	count += 12;
	multi_put_vector(&multibuf[count], qpp.rotvel);
	count += 12;
	// 46

	// send twice while first has priority so the next one will be attached to the next bigdata packet
	multi_send_data(multibuf, multiplayer_data_priority::_1);
	multi_send_data(multibuf, multiplayer_data_priority::_0);
}

/* 
 * I was killed. If I am host, send this info to everyone and compute kill. If I am just a Client I'll only send the kill but not compute it for me. I (Client) will wait for Host to send me my kill back together with updated game_mode related variables which are important for me to compute consistent kill.
 */
void multi_send_kill(const vmobjptridx_t objnum)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &imobjptridx = Objects.imptridx;
	auto &vmobjptr = Objects.vmptr;
	// I died, tell the world.
	Assert(get_player_id(objnum) == Player_num);
	const auto killer_objnum = get_local_plrobj().ctype.player_info.killer_objnum;

	// do it with variable since INTEL_SHORT won't work on return val from function.
	const auto &&[remote_owner, remote_objnum] = killer_objnum != object_none
		? objnum_local_to_remote(killer_objnum)
		: owned_remote_objnum{static_cast<int8_t>(-1), static_cast<uint16_t>(0xffff)};
	const auto local_is_host = multi_i_am_master();
	union mb {
		multi_command<multiplayer_command_t::MULTI_KILL_CLIENT> c;
		multi_command<multiplayer_command_t::MULTI_KILL_HOST> h;
		mb() {}
	};
	mb multibuf;
	if (local_is_host)
	{
		new(&multibuf.h) multi_command<multiplayer_command_t::MULTI_KILL_HOST>();
		multibuf.h[5] = Netgame.team_vector;
		multibuf.h[6] = Bounty_target;
	}
	else
	{
		new(&multibuf.c) multi_command<multiplayer_command_t::MULTI_KILL_CLIENT>();
	}
	/* Obsolete - reclaim player number field on next multiplayer protocol version bump */
	multibuf.h[1] = Player_num;
	multibuf.h[4] = remote_owner;
	PUT_INTEL_SHORT(&multibuf.h[2], remote_objnum);
	// I am host - I know what's going on so attach game_mode related info which might be vital for correct kill computation
	if (local_is_host)
	{
		multi_compute_kill(LevelSharedRobotInfoState.Robot_info, imobjptridx(killer_objnum), objnum);
		multi_send_data(multibuf.h, multiplayer_data_priority::_2);
	}
	else
		multi_send_data_direct(multibuf.c, multi_who_is_master(), 2); // I am just a client so I'll only send my kill but not compute it, yet. I'll get response from host so I can compute it correctly

	multi_strip_robots(Player_num);

	if (Game_mode & GM_BOUNTY && multi_i_am_master()) // update in case if needed... we could attach this to this packet but... meh...
		multi_send_bounty();
}

void multi_send_remobj(const vmobjidx_t objnum)
{
	// Tell the other guy to remove an object from his list
	const auto &&[obj_owner, remote_objnum] = objnum_local_to_remote(objnum);
	multi_command<multiplayer_command_t::MULTI_REMOVE_OBJECT> multibuf;
	PUT_INTEL_SHORT(&multibuf[1], remote_objnum); // Map to network objnums

	multibuf[3] = obj_owner;

	multi_send_data(multibuf, multiplayer_data_priority::_2);

	if (Network_send_objects && multi::dispatch->objnum_is_past(objnum))
	{
		Network_send_objnum = -1;
	}
}

}

namespace dcx {

namespace {

void multi_send_quit()
{
	// I am quitting the game, tell the other guy the bad news.

	multi_command<multiplayer_command_t::MULTI_QUIT> multibuf;
	/* Obsolete - reclaim player number field on next multiplayer protocol version bump */
	multibuf[1] = Player_num;
	multi_send_data(multibuf, multiplayer_data_priority::_2);
}

}

void multi_send_cloak()
{
	// Broadcast a change in our pflags (made to support cloaking)

	multi_command<multiplayer_command_t::MULTI_CLOAK> multibuf;
	const auto pnum = Player_num;
	multibuf[1] = pnum;

	multi_send_data(multibuf, multiplayer_data_priority::_2);

	multi_strip_robots(pnum);
}

void multi_send_decloak()
{
	// Broadcast a change in our pflags (made to support cloaking)

	multi_command<multiplayer_command_t::MULTI_DECLOAK> multibuf;
	multibuf[1] = Player_num;

	multi_send_data(multibuf, multiplayer_data_priority::_2);
}

}

namespace dsx {

void multi_send_door_open(const vcsegidx_t segnum, const sidenum_t side, const wall_flags flag)
{
	multi_command<multiplayer_command_t::MULTI_DOOR_OPEN> multibuf;
	// When we open a door make sure everyone else opens that door
	PUT_INTEL_SEGNUM(&multibuf[1], segnum);
	multibuf[3] = underlying_value(side);
#if defined(DXX_BUILD_DESCENT_I)
	(void)flag;
#elif defined(DXX_BUILD_DESCENT_II)
	multibuf[4] = underlying_value(flag);
#endif
	multi_send_data(multibuf, multiplayer_data_priority::_2);
}

#if defined(DXX_BUILD_DESCENT_II)
void multi_send_door_open_specific(const playernum_t pnum, const vcsegidx_t segnum, const sidenum_t side, const wall_flags flag)
{
	// For sending doors only to a specific person (usually when they're joining)

	Assert (Game_mode & GM_NETWORK);
	//   Assert (pnum>-1 && pnum<N_players);

	multi_command<multiplayer_command_t::MULTI_DOOR_OPEN> multibuf;
	PUT_INTEL_SEGNUM(&multibuf[1], segnum);
	multibuf[3] = static_cast<int8_t>(side);
	multibuf[4] = underlying_value(flag);

	multi_send_data_direct(multibuf, pnum, 2);
}
#endif

}

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
	multi_command<multiplayer_command_t::MULTI_CREATE_EXPLOSION> multibuf;
	multibuf[count] = static_cast<int8_t>(pnum);                  count += 1;
	//                                                                                                      -----------
	//                                                                                                      Total size = 2
	multi_send_data(multibuf, multiplayer_data_priority::_0);
}

void multi_send_controlcen_fire(const vms_vector &to_goal, int best_gun_num, objnum_t objnum)
{
	int count = 0;

	count +=  1;
	multi_command<multiplayer_command_t::MULTI_CONTROLCEN_FIRE> multibuf;
	multi_put_vector(&multibuf[count], to_goal);
	count += 12;
	multibuf[count] = static_cast<char>(best_gun_num);                   count +=  1;
	PUT_INTEL_SHORT(&multibuf[count], objnum );     count +=  2;
	//                                                                                                                      ------------
	//                                                                                                                      Total  = 16
	multi_send_data(multibuf, multiplayer_data_priority::_0);
}

namespace dsx {

void multi_send_create_powerup(const powerup_type_t powerup_type, const vcsegidx_t segnum, const vcobjidx_t objnum, const vms_vector &pos)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	// Create a powerup on a remote machine, used for remote
	// placement of used powerups like missiles and cloaking
	// powerups.

	int count = 0;

	multi_send_position(vmobjptridx(get_local_player().objnum));

	count += 1;
	multi_command<multiplayer_command_t::MULTI_CREATE_POWERUP> multibuf;
	multibuf[count] = Player_num;                                      count += 1;
	multibuf[count] = underlying_value(powerup_type);                                 count += 1;
	PUT_INTEL_SEGNUM(&multibuf[count], segnum);     count += 2;
	PUT_INTEL_SHORT(&multibuf[count], objnum );     count += 2;
	multi_put_vector(&multibuf[count], pos);
	count += 12;
	//                                                                                                            -----------
	//                                                                                                            Total =  19
	multi_send_data(multibuf, multiplayer_data_priority::_2);

	if (Network_send_objects && multi::dispatch->objnum_is_past(objnum))
	{
		Network_send_objnum = -1;
	}

	map_objnum_local_to_local(objnum);
}

}

namespace {

static void multi_digi_play_sample(const int soundnum, const fix max_volume, const sound_stack once)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptridx = Objects.vcptridx;
	if (Game_mode & GM_MULTI)
		multi_send_play_sound(soundnum, max_volume, once);
	digi_link_sound_to_object(soundnum, vcobjptridx(Viewer), 0, max_volume, once);
}

}

void multi_digi_play_sample_once(int soundnum, fix max_volume)
{
	multi_digi_play_sample(soundnum, max_volume, sound_stack::cancel_previous);
}

void multi_digi_play_sample(int soundnum, fix max_volume)
{
	multi_digi_play_sample(soundnum, max_volume, sound_stack::allow_stacking);
}

namespace dsx {

void multi_digi_link_sound_to_pos(const int soundnum, const vcsegptridx_t segnum, const sidenum_t sidenum, const vms_vector &pos, const int forever, const fix max_volume)
{
	if (Game_mode & GM_MULTI)
		multi_send_play_sound(soundnum, max_volume, sound_stack::allow_stacking);
	digi_link_sound_to_pos(soundnum, segnum, sidenum, pos, forever, max_volume);
}

}

void multi_send_play_sound(const int sound_num, const fix volume, const sound_stack once)
{
	int count = 0;
	count += 1;
	multi_command<multiplayer_command_t::MULTI_PLAY_SOUND> multibuf;
	multibuf[count] = Player_num;                                   count += 1;
	multibuf[count] = static_cast<char>(sound_num);                      count += 1;
	multibuf[count] = static_cast<uint8_t>(once);                      count += 1;
	PUT_INTEL_INT(&multibuf[count], volume);							count += 4;
	//                                                                                                         -----------
	//                                                                                                         Total = 4
	multi_send_data(multibuf, multiplayer_data_priority::_0);
}

void multi_send_score()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	// Send my current score to all other players so it will remain
	// synced.
	int count = 0;

	if (Game_mode & GM_MULTI_COOP) {
		multi_sort_kill_list();
		count += 1;
		multi_command<multiplayer_command_t::MULTI_SCORE> multibuf;
		multibuf[count] = Player_num;                           count += 1;
		auto &player_info = get_local_plrobj().ctype.player_info;
		PUT_INTEL_INT(&multibuf[count], player_info.mission.score);  count += 4;
		multi_send_data(multibuf, multiplayer_data_priority::_0);
	}
}

void multi_send_trigger(const trgnum_t triggernum)
{
	// Send an event to trigger something in the mine

	int count = 0;

	count += 1;
	multi_command<multiplayer_command_t::MULTI_TRIGGER> multibuf;
	/* Obsolete - reclaim player number field on next multiplayer protocol version bump */
	multibuf[count] = Player_num;                                   count += 1;
	static_assert(sizeof(trgnum_t) == sizeof(uint8_t), "trigger number could be truncated");
	multibuf[count] = static_cast<uint8_t>(triggernum);            count += 1;

	multi_send_data(multibuf, multiplayer_data_priority::_2);
}

namespace dsx {

#if defined(DXX_BUILD_DESCENT_II)
void multi_send_effect_blowup(const vcsegidx_t segnum, const sidenum_t side, const vms_vector &pnt)
{
	// We blew up something connected to a trigger. Send this blowup result to other players shortly before MULTI_TRIGGER.
	// NOTE: The reason this is now a separate packet is to make sure trigger-connected switches/monitors are in sync with MULTI_TRIGGER.
	//       If a fire packet is late it might blow up a switch for some clients without the shooter actually registering this hit,
	//       not sending MULTI_TRIGGER and making puzzles or progress impossible.
	int count = 0;

	multi::dispatch->do_protocol_frame(1, 0); // force packets to be sent, ensuring this packet will be attached to following MULTI_TRIGGER
	
	count += 1;
	multi_command<multiplayer_command_t::MULTI_EFFECT_BLOWUP> multibuf;
	multibuf[count] = Player_num;                                   count += 1;
	PUT_INTEL_SEGNUM(&multibuf[count], segnum);                        count += 2;
	multibuf[count] = static_cast<int8_t>(side);                                  count += 1;
	multi_put_vector(&multibuf[count], pnt);

	multi_send_data(multibuf, multiplayer_data_priority::_0);
}
#endif

void multi_send_hostage_door_status(const vcwallptridx_t w)
{
	// Tell the other player what the hit point status of a hostage door
	// should be

	int count = 0;

	assert(w->type == WALL_BLASTABLE);

	count += 1;
	multi_command<multiplayer_command_t::MULTI_HOSTAGE_DOOR> multibuf;
	PUT_INTEL_SHORT(&multibuf[count], underlying_value(wallnum_t{w}));
	count += 2;
	PUT_INTEL_INT(&multibuf[count], w->hps);  count += 4;

	multi_send_data(multibuf, multiplayer_data_priority::_0);
}

}

void multi_consistency_error(int reset)
{
	static int count = 0;

	if (reset)
		count = 0;

	if (++count < 10)
		return;

	const auto g = Game_wind;
	if (g)
		g->set_visible(0);
	nm_messagebox_str(menu_title{nullptr}, nm_messagebox_tie(TXT_OK), menu_subtitle{TXT_CONSISTENCY_ERROR});
	if (g)
		g->set_visible(1);
	count = 0;
	multi_quit_game = 1;
	game_leave_menus();
	multi_reset_stuff();
}

namespace {

template <netgrant_bit grant, netflag_bit flag, int s = static_cast<int>(grant) - static_cast<int>(flag)>
static constexpr netflag_flag grant_shift_helper(const packed_spawn_granted_items p)
{
	if constexpr (s > 0)
		return static_cast<netflag_flag>(p.mask >> s);
	else
		return static_cast<netflag_flag>(p.mask << -s);
}

}

namespace dsx {

player_flags map_granted_flags_to_player_flags(const packed_spawn_granted_items p)
{
	auto &grant = p.mask;
	const auto None = PLAYER_FLAG::None;
	return player_flags(
		((grant & netgrant_flag::NETGRANT_QUAD) != netgrant_flag::None ? PLAYER_FLAGS_QUAD_LASERS : None)
#if defined(DXX_BUILD_DESCENT_II)
		| ((grant & netgrant_flag::NETGRANT_AFTERBURNER) != netgrant_flag::None ? PLAYER_FLAGS_AFTERBURNER : None)
		| ((grant & netgrant_flag::NETGRANT_AMMORACK) != netgrant_flag::None ? PLAYER_FLAGS_AMMO_RACK : None)
		| ((grant & netgrant_flag::NETGRANT_CONVERTER) != netgrant_flag::None ? PLAYER_FLAGS_CONVERTER : None)
		| ((grant & netgrant_flag::NETGRANT_HEADLIGHT) != netgrant_flag::None ? PLAYER_FLAGS_HEADLIGHT : None)
#endif
	);
}

uint_fast32_t map_granted_flags_to_primary_weapon_flags(const packed_spawn_granted_items p)
{
	auto &grant = p.mask;
	return ((grant & netgrant_flag::NETGRANT_VULCAN) != netgrant_flag::None ? HAS_VULCAN_FLAG : 0)
		| ((grant & netgrant_flag::NETGRANT_SPREAD) != netgrant_flag::None ? HAS_SPREADFIRE_FLAG : 0)
		| ((grant & netgrant_flag::NETGRANT_PLASMA) != netgrant_flag::None ? HAS_PLASMA_FLAG : 0)
		| ((grant & netgrant_flag::NETGRANT_FUSION) != netgrant_flag::None ? HAS_FUSION_FLAG : 0)
#if defined(DXX_BUILD_DESCENT_II)
		| ((grant & netgrant_flag::NETGRANT_GAUSS) != netgrant_flag::None ? HAS_GAUSS_FLAG : 0)
		| ((grant & netgrant_flag::NETGRANT_HELIX) != netgrant_flag::None ? HAS_HELIX_FLAG : 0)
		| ((grant & netgrant_flag::NETGRANT_PHOENIX) != netgrant_flag::None ? HAS_PHOENIX_FLAG : 0)
		| ((grant & netgrant_flag::NETGRANT_OMEGA) != netgrant_flag::None ? HAS_OMEGA_FLAG : 0)
#endif
		;
}

uint16_t map_granted_flags_to_vulcan_ammo(const packed_spawn_granted_items p)
{
	auto &grant = p.mask;
	const auto amount = VULCAN_WEAPON_AMMO_AMOUNT;
	return
#if defined(DXX_BUILD_DESCENT_II)
		((grant & netgrant_flag::NETGRANT_GAUSS) != netgrant_flag::None ? amount : 0) +
#endif
		((grant & netgrant_flag::NETGRANT_VULCAN) != netgrant_flag::None ? amount : 0);
}

namespace {

static constexpr netflag_flag map_granted_flags_to_netflag(const packed_spawn_granted_items grant)
{
	return (grant_shift_helper<BIT_NETGRANT_QUAD, BIT_NETFLAG_DOQUAD>(grant) & (netflag_flag::NETFLAG_DOQUAD | netflag_flag::NETFLAG_DOVULCAN | netflag_flag::NETFLAG_DOSPREAD | netflag_flag::NETFLAG_DOPLASMA | netflag_flag::NETFLAG_DOFUSION))
#if defined(DXX_BUILD_DESCENT_II)
		| (grant_shift_helper<BIT_NETGRANT_GAUSS, BIT_NETFLAG_DOGAUSS>(grant) & (netflag_flag::NETFLAG_DOGAUSS | netflag_flag::NETFLAG_DOHELIX | netflag_flag::NETFLAG_DOPHOENIX | netflag_flag::NETFLAG_DOOMEGA))
		| (grant_shift_helper<BIT_NETGRANT_AFTERBURNER, BIT_NETFLAG_DOAFTERBURNER>(grant) & (netflag_flag::NETFLAG_DOAFTERBURNER | netflag_flag::NETFLAG_DOAMMORACK | netflag_flag::NETFLAG_DOCONVERTER | netflag_flag::NETFLAG_DOHEADLIGHT))
#endif
		;
}

template <netgrant_flag grant, netflag_flag expected_flag, netflag_flag actual_flag = map_granted_flags_to_netflag(grant)>
struct assert_netgrant_map_result : std::true_type
{
	static_assert(actual_flag == expected_flag);
};

static_assert(assert_netgrant_map_result<netgrant_flag::NETGRANT_QUAD, netflag_flag::NETFLAG_DOQUAD>::value, "QUAD");
static_assert(assert_netgrant_map_result<netgrant_flag::NETGRANT_QUAD | netgrant_flag::NETGRANT_PLASMA, netflag_flag::NETFLAG_DOQUAD | netflag_flag::NETFLAG_DOPLASMA>::value, "QUAD | PLASMA");
#if defined(DXX_BUILD_DESCENT_II)
static_assert(assert_netgrant_map_result<netgrant_flag::NETGRANT_GAUSS, netflag_flag::NETFLAG_DOGAUSS>::value, "GAUSS");
static_assert(assert_netgrant_map_result<netgrant_flag::NETGRANT_GAUSS | netgrant_flag::NETGRANT_PLASMA, netflag_flag::NETFLAG_DOGAUSS | netflag_flag::NETFLAG_DOPLASMA>::value, "GAUSS | PLASMA");
static_assert(assert_netgrant_map_result<netgrant_flag::NETGRANT_GAUSS | netgrant_flag::NETGRANT_AFTERBURNER, netflag_flag::NETFLAG_DOGAUSS | netflag_flag::NETFLAG_DOAFTERBURNER>::value, "GAUSS | AFTERBURNER");
static_assert(assert_netgrant_map_result<netgrant_flag::NETGRANT_GAUSS | netgrant_flag::NETGRANT_PLASMA | netgrant_flag::NETGRANT_AFTERBURNER, netflag_flag::NETFLAG_DOGAUSS | netflag_flag::NETFLAG_DOPLASMA | netflag_flag::NETFLAG_DOAFTERBURNER>::value, "GAUSS | PLASMA | AFTERBURNER");
static_assert(assert_netgrant_map_result<netgrant_flag::NETGRANT_PLASMA | netgrant_flag::NETGRANT_AFTERBURNER, netflag_flag::NETFLAG_DOPLASMA | netflag_flag::NETFLAG_DOAFTERBURNER>::value, "PLASMA | AFTERBURNER");
static_assert(assert_netgrant_map_result<netgrant_flag::NETGRANT_AFTERBURNER, netflag_flag::NETFLAG_DOAFTERBURNER>::value, "AFTERBURNER");
static_assert(assert_netgrant_map_result<netgrant_flag::NETGRANT_HEADLIGHT, netflag_flag::NETFLAG_DOHEADLIGHT>::value, "HEADLIGHT");
#endif

class update_item_state
{
	std::bitset<MAX_OBJECTS> m_modified;
public:
	bool must_skip(const vcobjidx_t i) const
	{
		return m_modified.test(i);
	}
	void process_powerup(const d_vclip_array &Vclip, fvmsegptridx &, const object &, powerup_type_t);
};

class powerup_shuffle_state
{
	unsigned count = 0;
	unsigned seed;
	union {
		std::array<vmobjptridx_t, MAX_OBJECTS> ptrs;
	};
public:
	powerup_shuffle_state(const unsigned s) :
		seed(s)
	{
	}
	void record_powerup(vmobjptridx_t);
	void shuffle() const;
};

void update_item_state::process_powerup(const d_vclip_array &Vclip, fvmsegptridx &vmsegptridx, const object &o, const powerup_type_t id)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Vertices = LevelSharedVertexState.get_vertices();
	uint_fast32_t count;
	switch (id)
	{
		case powerup_type_t::POW_LASER:
		case powerup_type_t::POW_QUAD_FIRE:
		case powerup_type_t::POW_VULCAN_WEAPON:
		case powerup_type_t::POW_VULCAN_AMMO:
		case powerup_type_t::POW_SPREADFIRE_WEAPON:
		case powerup_type_t::POW_PLASMA_WEAPON:
		case powerup_type_t::POW_FUSION_WEAPON:
#if defined(DXX_BUILD_DESCENT_II)
		case powerup_type_t::POW_SUPER_LASER:
		case powerup_type_t::POW_GAUSS_WEAPON:
		case powerup_type_t::POW_HELIX_WEAPON:
		case powerup_type_t::POW_PHOENIX_WEAPON:
		case powerup_type_t::POW_OMEGA_WEAPON:
#endif
			count = Netgame.DuplicatePowerups.get_primary_count();
			break;
		case powerup_type_t::POW_MISSILE_1:
		case powerup_type_t::POW_MISSILE_4:
		case powerup_type_t::POW_HOMING_AMMO_1:
		case powerup_type_t::POW_HOMING_AMMO_4:
		case powerup_type_t::POW_PROXIMITY_WEAPON:
		case powerup_type_t::POW_SMARTBOMB_WEAPON:
		case powerup_type_t::POW_MEGA_WEAPON:
#if defined(DXX_BUILD_DESCENT_II)
		case powerup_type_t::POW_SMISSILE1_1:
		case powerup_type_t::POW_SMISSILE1_4:
		case powerup_type_t::POW_GUIDED_MISSILE_1:
		case powerup_type_t::POW_GUIDED_MISSILE_4:
		case powerup_type_t::POW_SMART_MINE:
		case powerup_type_t::POW_MERCURY_MISSILE_1:
		case powerup_type_t::POW_MERCURY_MISSILE_4:
		case powerup_type_t::POW_EARTHSHAKER_MISSILE:
#endif
			count = Netgame.DuplicatePowerups.get_secondary_count();
			break;
#if defined(DXX_BUILD_DESCENT_II)
		case powerup_type_t::POW_FULL_MAP:
		case powerup_type_t::POW_CONVERTER:
		case powerup_type_t::POW_AMMO_RACK:
		case powerup_type_t::POW_AFTERBURNER:
		case powerup_type_t::POW_HEADLIGHT:
			count = Netgame.DuplicatePowerups.get_accessory_count();
			break;
#endif
		default:
			return;
	}
	if (!count)
		return;
	const auto &vc = Vclip[o.rtype.vclip_info.vclip_num];
	const auto vc_num_frames = vc.num_frames;
	const auto &&segp = vmsegptridx(o.segnum);
	const auto &seg_verts = segp->verts;
	auto &vcvertptr = Vertices.vcptr;
	for (uint_fast32_t i = count++; i; --i)
	{
		assert(o.movement_source == object::movement_type::None);
		assert(o.render_type == render_type::RT_POWERUP);
		const auto &&no = obj_create(LevelUniqueObjectState, LevelSharedSegmentState, LevelUniqueSegmentState, OBJ_POWERUP, underlying_value(id), segp, vm_vec_avg(o.pos, vcvertptr(seg_verts[static_cast<segment_relative_vertnum>(i % seg_verts.size())])), &vmd_identity_matrix, o.size, object::control_type::powerup, object::movement_type::None, render_type::RT_POWERUP);
		if (no == object_none)
			return;
		m_modified.set(no);
		no->rtype.vclip_info = o.rtype.vclip_info;
		/* Vary the frame number so that the powerups are not animated
		 * in sync.
		 */
		no->rtype.vclip_info.framenum = (o.rtype.vclip_info.framenum + (i * vc_num_frames) / count) % vc_num_frames;
		no->ctype.powerup_info = o.ctype.powerup_info;
	}
}

class accumulate_object_count
{
protected:
	using array_reference = enumerated_array<uint32_t, MAX_POWERUP_TYPES, powerup_type_t> &;
	array_reference current;
	accumulate_object_count(array_reference a) : current(a)
	{
	}
};

template <typename F, typename M>
class accumulate_flags_count : accumulate_object_count
{
	const F &flags;
public:
	accumulate_flags_count(array_reference current, const F &f) :
		accumulate_object_count{current}, flags(f)
	{
	}
	void process(const M mask, const powerup_type_t id) const
	{
		if (flags & mask)
			++current[id];
	}
};

}

/*
 * The place to do objects operations such as:
 * Robot deletion for non-robot games, Powerup duplication, AllowedItems, Initial powerup counting.
 * MUST be done before multi_level_sync() in case we join a running game and get updated objects there. We want the initial powerup setup for a level here!
 */
void multi_prep_level_objects(const d_powerup_info_array &Powerup_info, const d_vclip_array &Vclip)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
        if (!(Game_mode & GM_MULTI_COOP))
	{
		multi_update_objects_for_non_cooperative(); // Removes monsters from level
	}

	constexpr unsigned MAX_ALLOWED_INVULNERABILITY = 3;
	constexpr unsigned MAX_ALLOWED_CLOAK = 3;
	const auto AllowedItems = Netgame.AllowedItems;
	const auto SpawnGrantedItems = map_granted_flags_to_netflag(Netgame.SpawnGrantedItems);
	unsigned inv_remaining = (AllowedItems & netflag_flag::NETFLAG_DOINVUL) != netflag_flag::None ? MAX_ALLOWED_INVULNERABILITY : 0;
	unsigned cloak_remaining = (AllowedItems & netflag_flag::NETFLAG_DOCLOAK) != netflag_flag::None ? MAX_ALLOWED_CLOAK : 0;
	update_item_state duplicates;
	range_for (const auto &&o, vmobjptridx)
	{
		if ((o->type == OBJ_HOSTAGE) && !(Game_mode & GM_MULTI_COOP))
		{
			/* In non-cooperative multiplayer games, replace hostage powerups
			 * with shield powerups.
			 */
			const auto &&objnum = obj_create(LevelUniqueObjectState, LevelSharedSegmentState, LevelUniqueSegmentState, OBJ_POWERUP, underlying_value(powerup_type_t::POW_SHIELD_BOOST), vmsegptridx(o->segnum), o->pos, &vmd_identity_matrix, Powerup_info[powerup_type_t::POW_SHIELD_BOOST].size, object::control_type::powerup, object::movement_type::physics, render_type::RT_POWERUP);
			obj_delete(LevelUniqueObjectState, Segments, o);
			if (objnum != object_none)
			{
				objnum->rtype.vclip_info.vclip_num = Powerup_info[powerup_type_t::POW_SHIELD_BOOST].vclip_num;
				objnum->rtype.vclip_info.frametime = Vclip[objnum->rtype.vclip_info.vclip_num].frame_time;
				objnum->rtype.vclip_info.framenum = 0;
				objnum->mtype.phys_info.drag = 512;     //1024;
				objnum->mtype.phys_info.mass = F1_0;
				objnum->mtype.phys_info.velocity = {};
			}
			continue;
		}

		if (o->type == OBJ_POWERUP && !duplicates.must_skip(o))
		{
			switch (const auto id = get_powerup_id(o))
			{
				case powerup_type_t::POW_EXTRA_LIFE:
					set_powerup_id(Powerup_info, Vclip, o, powerup_type_t::POW_INVULNERABILITY);
					[[fallthrough]];
				case powerup_type_t::POW_INVULNERABILITY:
					if (inv_remaining)
						-- inv_remaining;
					else
						set_powerup_id(Powerup_info, Vclip, o, powerup_type_t::POW_SHIELD_BOOST);
					continue;
				case powerup_type_t::POW_CLOAK:
					if (cloak_remaining)
						-- cloak_remaining;
					else
						set_powerup_id(Powerup_info, Vclip, o, powerup_type_t::POW_SHIELD_BOOST);
					continue;
				default:
					if (multi_powerup_is_allowed(id, AllowedItems, SpawnGrantedItems) == netflag_flag::None)
						bash_to_shield(Powerup_info, Vclip, o);
					else
						duplicates.process_powerup(Vclip, vmsegptridx, o, id);
					continue;
			}
		}
	}

	// After everything is done, count initial level inventory.
	MultiLevelInv_InitializeCount();
}

void multi_prep_level_player(void)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	// Do any special stuff to the level required for games
	// before we begin playing in it.

	// Player_num MUST be set before calling this procedure.

	// This function must be called before checksuming the Object array,
	// since the resulting checksum with depend on the value of Player_num
	// at the time this is called.

	Assert(Game_mode & GM_MULTI);

	Assert(NumNetPlayerPositions > 0);

#if defined(DXX_BUILD_DESCENT_II)
	hoard_highest_record_stats = {};
	Drop_afterburner_blob_flag=0;
#endif
	Bounty_target = 0;

	multi_consistency_error(1);

	multi_sending_message.fill(msgsend_state::none);
	if (imulti_new_game)
		for (uint_fast32_t i = 0; i != Players.size(); i++)
			init_player_stats_new_ship(i);

	for (unsigned i = 0; i < NumNetPlayerPositions; i++)
	{
		const auto &&objp = vmobjptr(vcplayerptr(i)->objnum);
		if (i != Player_num)
			objp->control_source = object::control_type::remote;
		objp->movement_source = object::movement_type::physics;
		multi_reset_player_object(objp);
		Netgame.players[i].LastPacketTime = 0;
	}

	robot_controlled.fill(-1);
	robot_agitation = {};
	robot_fired = {};

	Viewer = ConsoleObject = &get_local_plrobj();

#if defined(DXX_BUILD_DESCENT_II)
	if (game_mode_hoard(Game_mode))
		init_hoard_data(Vclip);

	if (game_mode_capture_flag(Game_mode) || game_mode_hoard(Game_mode))
		multi_apply_goal_textures();
#endif

	multi_sort_kill_list();

	multi_show_player_list();

	ConsoleObject->control_source = object::control_type::flying;

	reset_player_object(*ConsoleObject);

	imulti_new_game=0;
}

#if defined(DXX_BUILD_DESCENT_II)
namespace {

void apply_segment_goal_texture(const d_level_unique_tmap_info_state &LevelUniqueTmapInfoState, unique_segment &seg, const texture1_value tex)
{
	auto &TmapInfo = LevelUniqueTmapInfoState.TmapInfo;
	const auto bright_light = i2f(100);	//make static light bright
	seg.static_light = bright_light;
	if (static_cast<unsigned>(tex) < TmapInfo.size())
		range_for (auto &s, seg.sides)
		{
			s.tmap_num = tex;
			range_for (auto &uvl, s.uvls)
				uvl.l = bright_light;		//max out
		}
}

texture_index find_goal_texture(const d_level_unique_tmap_info_state &LevelUniqueTmapInfoState, const tmapinfo_flag tmi_flag)
{
	auto &TmapInfo = LevelUniqueTmapInfoState.TmapInfo;
	const auto &&r = partial_const_range(TmapInfo, NumTextures);
	const auto &&predicate = [tmi_flag](const tmap_info &i) {
		return (i.flags & tmi_flag);
	};
	const auto begin = r.begin();
	return std::distance(begin, std::ranges::find_if(begin, r.end(), predicate));
}

const tmap_info &find_required_goal_texture(const d_level_unique_tmap_info_state &LevelUniqueTmapInfoState, const tmapinfo_flag tmi_flag)
{
	auto &TmapInfo = LevelUniqueTmapInfoState.TmapInfo;
	const auto found_index = find_goal_texture(LevelUniqueTmapInfoState, tmi_flag);
	std::size_t r = found_index;
	if (r < TmapInfo.size())
		return TmapInfo[r];
	Int3(); // Hey, there is no goal texture for this PIG!!!!
	// Edit bitmaps.tbl and designate two textures to be RED and BLUE
	// goal textures
	throw std::runtime_error("PIG missing goal texture");
}

}

void multi_apply_goal_textures()
{
	texture1_value tex_blue, tex_red;
	if (game_mode_hoard(Game_mode))
		tex_blue = tex_red = build_texture1_value(find_goal_texture(LevelUniqueTmapInfoState, tmapinfo_flag::goal_hoard));
	else
	{
		tex_blue = build_texture1_value(find_goal_texture(LevelUniqueTmapInfoState, tmapinfo_flag::goal_blue));
		tex_red = build_texture1_value(find_goal_texture(LevelUniqueTmapInfoState, tmapinfo_flag::goal_red));
	}
	for (auto &seg : vmsegptr)
	{
		texture1_value tex;
		if (seg.special == segment_special::goal_blue)
		{
			tex = tex_blue;
		}
		else if (seg.special == segment_special::goal_red)
		{
			// Make both textures the same if Hoard mode
			tex = tex_red;
		}
		else
			continue;
		apply_segment_goal_texture(LevelUniqueTmapInfoState, seg, tex);
	}
}
#endif

namespace {

static int object_allowed_in_anarchy(const object_base &objp)
{
	if (objp.type == OBJ_NONE ||
		objp.type == OBJ_PLAYER ||
		objp.type == OBJ_POWERUP ||
		objp.type == OBJ_CNTRLCEN ||
		objp.type == OBJ_HOSTAGE)
		return 1;
#if defined(DXX_BUILD_DESCENT_II)
	if (objp.type == OBJ_WEAPON && get_weapon_id(objp) == weapon_id_type::PMINE_ID)
		return 1;
#endif
	return 0;
}

void powerup_shuffle_state::record_powerup(const vmobjptridx_t o)
{
	if (!seed)
		return;
	const auto id = get_powerup_id(o);
	switch (id)
	{
		/* record_powerup runs before object conversion or duplication,
		 * so object types that anarchy converts still have their
		 * original type when this switch runs.  Therefore,
		 * powerup_type_t::POW_EXTRA_LIFE and the key powerups must be handled here,
		 * even though they are converted to other objects before play
		 * begins.  If they were not handled, no object could exchange
		 * places with a converted object.
		 */
		case powerup_type_t::POW_EXTRA_LIFE:
		case powerup_type_t::POW_ENERGY:
		case powerup_type_t::POW_SHIELD_BOOST:
		case powerup_type_t::POW_LASER:
		case powerup_type_t::POW_KEY_BLUE:
		case powerup_type_t::POW_KEY_RED:
		case powerup_type_t::POW_KEY_GOLD:
		case powerup_type_t::POW_MISSILE_1:
		case powerup_type_t::POW_MISSILE_4:
		case powerup_type_t::POW_QUAD_FIRE:
		case powerup_type_t::POW_VULCAN_WEAPON:
		case powerup_type_t::POW_SPREADFIRE_WEAPON:
		case powerup_type_t::POW_PLASMA_WEAPON:
		case powerup_type_t::POW_FUSION_WEAPON:
		case powerup_type_t::POW_PROXIMITY_WEAPON:
		case powerup_type_t::POW_HOMING_AMMO_1:
		case powerup_type_t::POW_HOMING_AMMO_4:
		case powerup_type_t::POW_SMARTBOMB_WEAPON:
		case powerup_type_t::POW_MEGA_WEAPON:
		case powerup_type_t::POW_VULCAN_AMMO:
		case powerup_type_t::POW_CLOAK:
		case powerup_type_t::POW_INVULNERABILITY:
#if defined(DXX_BUILD_DESCENT_II)
		case powerup_type_t::POW_GAUSS_WEAPON:
		case powerup_type_t::POW_HELIX_WEAPON:
		case powerup_type_t::POW_PHOENIX_WEAPON:
		case powerup_type_t::POW_OMEGA_WEAPON:

		case powerup_type_t::POW_SUPER_LASER:
		case powerup_type_t::POW_FULL_MAP:
		case powerup_type_t::POW_CONVERTER:
		case powerup_type_t::POW_AMMO_RACK:
		case powerup_type_t::POW_AFTERBURNER:
		case powerup_type_t::POW_HEADLIGHT:

		case powerup_type_t::POW_SMISSILE1_1:
		case powerup_type_t::POW_SMISSILE1_4:
		case powerup_type_t::POW_GUIDED_MISSILE_1:
		case powerup_type_t::POW_GUIDED_MISSILE_4:
		case powerup_type_t::POW_SMART_MINE:
		case powerup_type_t::POW_MERCURY_MISSILE_1:
		case powerup_type_t::POW_MERCURY_MISSILE_4:
		case powerup_type_t::POW_EARTHSHAKER_MISSILE:
#endif
			break;
		default:
			return;
	}
	if (count >= ptrs.size())
		return;
	ptrs[count++] = o;
}

void powerup_shuffle_state::shuffle() const
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	if (!count)
		return;
	std::minstd_rand mr(seed);
	for (unsigned j = count; --j;)
	{
		const auto oi = std::uniform_int_distribution<unsigned>(0u, j)(mr);
		if (oi == j)
			/* Swapping an object with itself is a no-op.  Skip the
			 * work.  Do not re-roll, both to avoid the potential for an
			 * infinite loop on unlucky rolls and to ensure a uniform
			 * distribution of swaps.
			 */
			continue;
		const auto o0 = ptrs[j];
		const auto o1 = ptrs[oi];
		const auto os0 = o0->segnum;
		const auto os1 = o1->segnum;
		/* Disconnect both objects from their original segments.  Swap
		 * their positions.  Link each object to the segment that the
		 * other object previously used.  This is necessary instead of
		 * using std::swap on object::segnum, since the segment's linked
		 * list of objects needs to be updated.
		 */
		obj_unlink(vmobjptr, vmsegptr, *o0);
		obj_unlink(vmobjptr, vmsegptr, *o1);
		std::swap(o0->pos, o1->pos);
		obj_link_unchecked(vmobjptr, o0, vmsegptridx(os1));
		obj_link_unchecked(vmobjptr, o1, vmsegptridx(os0));
	}
}

void multi_update_objects_for_non_cooperative()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	// Go through the object list and remove any objects not used in
	// 'Anarchy!' games.

	const auto game_mode = Game_mode;
	/* Shuffle objects before object duplication runs.  Otherwise,
	 * duplication-eligible items would be duplicated, then scattered,
	 * causing the original site to be a treasure trove of swapped
	 * items.  This way, duplicated items appear with their original.
	 */
	powerup_shuffle_state powerup_shuffle(Netgame.ShufflePowerupSeed);
	range_for (const auto &&objp, vmobjptridx)
	{
		const auto obj_type = objp->type;
		if (obj_type == OBJ_PLAYER || obj_type == OBJ_GHOST)
			continue;
		else if (obj_type == OBJ_ROBOT && (game_mode & GM_MULTI_ROBOTS))
			continue;
		else if (obj_type == OBJ_POWERUP)
		{
			powerup_shuffle.record_powerup(objp);
			continue;
		}
		else if (!object_allowed_in_anarchy(objp) ) {
#if defined(DXX_BUILD_DESCENT_II)
			// Before deleting object, if it's a robot, drop it's special powerup, if any
			if (obj_type == OBJ_ROBOT)
				if (objp->contains.count && objp->contains.type == contained_object_type::powerup)
					object_create_robot_egg(LevelSharedRobotInfoState.Robot_info, objp);
#endif
			obj_delete(LevelUniqueObjectState, Segments, objp);
		}
	}
	powerup_shuffle.shuffle();
}

}

}

namespace {

// Returns the Player_num of Master/Host of this game
playernum_t multi_who_is_master()
{
	return 0;
}

}

void change_playernum_to(const playernum_t new_Player_num)
{
	if (Player_num < Players.size())
	{
		vmplayerptr(new_Player_num)->callsign = get_local_player().callsign;
	}
	Player_num = new_Player_num;
}

namespace dsx {

#if defined(DXX_BUILD_DESCENT_I)
static
#endif
int multi_all_players_alive(const fvcobjptr &vcobjptr, const std::ranges::subrange<const player *> player_range)
{
	range_for (auto &plr, player_range)
	{
		const auto connected = plr.connected;
		if (connected == player_connection_status::playing)
		{
			if (vcobjptr(plr.objnum)->type == OBJ_GHOST) // player alive?
				return 0;
		}
		else if (connected != player_connection_status::disconnected) // ... and actually playing?
			return 0;
	}
	return (1);
}

const char *multi_common_deny_save_game(const fvcobjptr &vcobjptr, const std::ranges::subrange<const player *> player_range)
{
	if (Network_status == network_state::endlevel)
		return "Level is ending";
	if (!multi_all_players_alive(vcobjptr, player_range))
		return "All players must be alive and playing!";
	return deny_multi_save_game_duplicate_callsign(player_range);
}

const char *multi_interactive_deny_save_game(const fvcobjptr &vcobjptr, const std::ranges::subrange<const player *> player_range, const d_level_unique_control_center_state &LevelUniqueControlCenterState)
{
	if (LevelUniqueControlCenterState.Control_center_destroyed)
		return "Countdown in progress";
	return multi_common_deny_save_game(vcobjptr, player_range);
}

void multi_send_drop_weapon(const vmobjptridx_t objp, int seed)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	int count=0;
	int ammo_count;

	multi_send_position(vmobjptridx(get_local_player().objnum));
	ammo_count = objp->ctype.powerup_info.count;

#if defined(DXX_BUILD_DESCENT_II)
	if (get_powerup_id(objp) == powerup_type_t::POW_OMEGA_WEAPON && ammo_count == F1_0)
		ammo_count = F1_0 - 1; //make fit in short
#endif

	Assert(ammo_count < F1_0); //make sure fits in short

	count++;
	multi_command<multiplayer_command_t::MULTI_DROP_WEAPON> multibuf;
	multibuf[count++]=static_cast<char>(get_powerup_id(objp));
	PUT_INTEL_SHORT(&multibuf[count], objp); count += 2;
	PUT_INTEL_SHORT(&multibuf[count], static_cast<uint16_t>(ammo_count)); count += 2;
	PUT_INTEL_INT(&multibuf[count], seed);
	count += 4;

	map_objnum_local_to_local(objp);

	multi_send_data(multibuf, multiplayer_data_priority::_2);
}

namespace {

static void multi_do_drop_weapon(fvmobjptr &vmobjptr, const playernum_t pnum, const multiplayer_rspan<multiplayer_command_t::MULTI_DROP_WEAPON> buf)
{
	const auto powerup_id = static_cast<powerup_type_t>(buf[1]);
	const objnum_t remote_objnum = GET_INTEL_SHORT(&buf[2]);
	const uint16_t ammo = GET_INTEL_SHORT(&buf[4]);
	const auto seed{GET_INTEL_INT(&buf[6])};
	const auto &&objnum = spit_powerup(LevelUniqueObjectState, LevelSharedSegmentState, LevelUniqueSegmentState, Vclip, vmobjptr(vcplayerptr(pnum)->objnum), powerup_id, seed);
	if (objnum == object_none)
		return;
	objnum->ctype.powerup_info.count = ammo;
	map_objnum_local_to_remote(objnum, remote_objnum, pnum);
}

}

// We collected some ammo from a vulcan/gauss cannon powerup. Now we need to let everyone else know about its new ammo count.
void multi_send_vulcan_weapon_ammo_adjust(const vmobjptridx_t objnum)
{
	const auto &&[obj_owner, remote_objnum] = objnum_local_to_remote(objnum);
	multi_command<multiplayer_command_t::MULTI_VULWPN_AMMO_ADJ> multibuf;
	PUT_INTEL_SHORT(&multibuf[1], remote_objnum); // Map to network objnums

	multibuf[3] = obj_owner;

	const uint16_t ammo_count = objnum->ctype.powerup_info.count;
	PUT_INTEL_SHORT(&multibuf[4], ammo_count);

	multi_send_data(multibuf, multiplayer_data_priority::_2);

	if (Network_send_objects && multi::dispatch->objnum_is_past(objnum))
	{
		Network_send_objnum = -1;
	}
}

namespace {

static void multi_do_vulcan_weapon_ammo_adjust(fvmobjptr &vmobjptr, const multiplayer_rspan<multiplayer_command_t::MULTI_VULWPN_AMMO_ADJ> buf)
{
	// which object to update
	const objnum_t objnum = GET_INTEL_SHORT(&buf[1]);
	// which remote list is it entered in
	auto obj_owner = buf[3];

	assert(objnum != object_none);

	if (objnum < 1)
		return;

	auto local_objnum = objnum_remote_to_local(objnum, obj_owner); // translate to local objnum

	if (local_objnum == object_none)
	{
		return;
	}

	const auto &&obj = vmobjptr(local_objnum);
	if (obj->type != OBJ_POWERUP)
	{
		return;
	}

	if (Network_send_objects && multi::dispatch->objnum_is_past(local_objnum))
	{
		Network_send_objnum = -1;
	}

	const auto ammo = GET_INTEL_SHORT(&buf[4]);
		obj->ctype.powerup_info.count = ammo;
}

}

#if defined(DXX_BUILD_DESCENT_II)
namespace {

struct multi_guided_info
{
	uint8_t pnum;
	uint8_t release;
	shortpos sp;
};

DEFINE_MULTIPLAYER_SERIAL_MESSAGE(multiplayer_command_t::MULTI_GUIDED, multi_guided_info, g, (g.pnum, g.release, g.sp));

}

void multi_send_guided_info(const object_base &miss, const char done)
{
	multi_guided_info gi;
	gi.pnum = static_cast<uint8_t>(Player_num);
	gi.release = done;
	create_shortpos_little(LevelSharedSegmentState, gi.sp, miss);
	multi_serialize_write(multiplayer_data_priority::_0, gi);
}

namespace {

static void multi_do_guided(d_level_unique_object_state &LevelUniqueObjectState, const playernum_t pnum, const multiplayer_rspan<multiplayer_command_t::MULTI_GUIDED> buf)
{
	multi_guided_info b;
	multi_serialize_read(buf, b);
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;

	if (b.release)
	{
		release_remote_guided_missile(LevelUniqueObjectState, pnum);
		return;
	}

	const auto &&gimobj = LevelUniqueObjectState.Guided_missile.get_player_active_guided_missile(LevelUniqueObjectState.get_objects().vmptridx, pnum);
	if (gimobj == nullptr)
		return;
	const vmobjptridx_t guided_missile = gimobj;
	extract_shortpos_little(guided_missile, &b.sp);
	update_object_seg(vmobjptr, LevelSharedSegmentState, LevelUniqueSegmentState, guided_missile);
}

}

void multi_send_stolen_items ()
{
	multi_command<multiplayer_command_t::MULTI_STOLEN_ITEMS> multibuf;
	auto &Stolen_items = LevelUniqueObjectState.ThiefState.Stolen_items;
	std::transform(Stolen_items.begin(), Stolen_items.end(), std::next(multibuf.begin()), underlying_value<powerup_type_t>);
	multi_send_data(multibuf, multiplayer_data_priority::_2);
}

namespace {

static void multi_do_stolen_items(d_thief_unique_state &ThiefUniqueState, const multiplayer_rspan<multiplayer_command_t::MULTI_STOLEN_ITEMS> buf)
{
	for (auto &&[untrusted_network_powerup_value, local_stolen_item] : zip(buf.subspan<1>(), ThiefUniqueState.Stolen_items))
	{
		local_stolen_item = (untrusted_network_powerup_value < MAX_POWERUP_TYPES) ? powerup_type_t{untrusted_network_powerup_value} : ThiefUniqueState.stolen_item_type_none;
	}
}

}

void multi_send_wall_status_specific(const playernum_t pnum, wallnum_t wallnum, uint8_t type, const wall_flags flags, const wall_state state)
{
	// Send wall states a specific rejoining player

	int count=0;

	Assert (Game_mode & GM_NETWORK);
	//Assert (pnum>-1 && pnum<N_players);

	count++;
	multi_command<multiplayer_command_t::MULTI_WALL_STATUS> multibuf;
	PUT_INTEL_SHORT(&multibuf[count], static_cast<uint16_t>(wallnum));
	count+=2;
	multibuf[count]=type;                 count++;
	multibuf[count] = underlying_value(flags); count++;
	multibuf[count] = underlying_value(state); count++;

	multi_send_data_direct(multibuf, pnum, 2);
}

namespace {

static void multi_do_wall_status(fvmwallptr &vmwallptr, const multiplayer_rspan<multiplayer_command_t::MULTI_WALL_STATUS> buf)
{
	ubyte flag,type,state;

	const wallnum_t wallnum{GET_INTEL_SHORT(&buf[1])};
	type=buf[3];
	flag=buf[4];
	state=buf[5];

	auto &w = *vmwallptr(wallnum);
	w.type = type;
	w.flags = wall_flags{flag};
	w.state = wall_state{state};

	if (w.type == WALL_OPEN)
		digi_kill_sound_linked_to_segment(w.segnum, w.sidenum, SOUND_FORCEFIELD_HUM);
}

}
#endif

}

void multi_send_kill_goal_counts()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	int count=1;

	multi_command<multiplayer_command_t::MULTI_KILLGOALS> multibuf;
	range_for (auto &i, Players)
	{
		auto &obj = *vcobjptr(i.objnum);
		auto &player_info = obj.ctype.player_info;
		multibuf[count] = player_info.KillGoalCount;
		count++;
	}
	multi_send_data(multibuf, multiplayer_data_priority::_2);
}

namespace {

static void multi_do_kill_goal_counts(fvmobjptr &vmobjptr, const multiplayer_rspan<multiplayer_command_t::MULTI_KILLGOALS> buf)
{
	int count=1;

	range_for (auto &i, Players)
	{
		auto &obj = *vmobjptr(i.objnum);
		auto &player_info = obj.ctype.player_info;
		player_info.KillGoalCount = buf[count];
		count++;
	}
}

void multi_send_heartbeat ()
{
	if (!Netgame.PlayTimeAllowed.count())
		return;

	multi_command<multiplayer_command_t::MULTI_HEARTBEAT> multibuf;
	PUT_INTEL_INT(&multibuf[1], ThisLevelTime.count());
	multi_send_data(multibuf, multiplayer_data_priority::_0);
}

static void multi_do_heartbeat(const multiplayer_rspan<multiplayer_command_t::MULTI_HEARTBEAT> buf)
{
	const fix num{GET_INTEL_INT<int32_t>(&buf[1])};
	ThisLevelTime = d_time_fix(num);
}

}

namespace dsx {

void multi_check_for_killgoal_winner(const d_robot_info_array &Robot_info)
{
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	if (LevelUniqueControlCenterState.Control_center_destroyed)
		return;

	/* For historical compatibility, this routine has some quirks with
	 * scoring:
	 * - When two or more players have the same number of kills, this
	 *   routine always chooses the lowest index player.  No opportunity
	 *   is provided for the players to score a tie-breaking kill, nor
	 *   is any other property (such as remaining shields) considered.
	 * Historical versions had additional quirks relating to
	 * zero/negative kills, but those quirks have been removed.
	 */
	const auto &local_player = get_local_player();
	const player *bestplr = nullptr;
	int highest_kill_goal_count = 0;
	range_for (auto &i, partial_const_range(Players, N_players))
	{
		auto &obj = *vcobjptr(i.objnum);
		auto &player_info = obj.ctype.player_info;
		const auto KillGoalCount = player_info.KillGoalCount;
		if (highest_kill_goal_count < KillGoalCount)
		{
			highest_kill_goal_count = KillGoalCount;
			bestplr = &i;
		}
	}
	if (!bestplr)
	{
		/* No player has at least one kill */
		HUD_init_message_literal(HM_MULTI, "No one has scored any kills!");
	}
	else if (bestplr == &local_player)
	{
		HUD_init_message(HM_MULTI, "You have the best score at %d kills!", highest_kill_goal_count);
	}
	else
		HUD_init_message(HM_MULTI, "%s has the best score with %d kills!", static_cast<const char *>(bestplr->callsign), highest_kill_goal_count);
	net_destroy_controlcen(Objects, Robot_info);
}

#if defined(DXX_BUILD_DESCENT_II)
// Sync our seismic time with other players
void multi_send_seismic(fix duration)
{
	int count=1;
	multi_command<multiplayer_command_t::MULTI_SEISMIC> multibuf;
	PUT_INTEL_INT(&multibuf[count], duration); count += sizeof(duration);
	multi_send_data(multibuf, multiplayer_data_priority::_2);
}

namespace {

static void multi_do_seismic(multiplayer_rspan<multiplayer_command_t::MULTI_SEISMIC> buf)
{
	const fix duration{GET_INTEL_INT<int32_t>(&buf[1])};
	LevelUniqueSeismicState.Seismic_disturbance_end_time = GameTime64 + duration;
	digi_play_sample (SOUND_SEISMIC_DISTURBANCE_START, F1_0);
}

}

void multi_send_light_specific (const playernum_t pnum, const vcsegptridx_t segnum, const sidemask_t val)
{
	int count=1;

	Assert (Game_mode & GM_NETWORK);
	//  Assert (pnum>-1 && pnum<N_players);

	multi_command<multiplayer_command_t::MULTI_LIGHT> multibuf;
	PUT_INTEL_SEGNUM(&multibuf[count], segnum);
	count += sizeof(uint16_t);
	multibuf[count] = underlying_value(val); count++;

	range_for (auto &i, segnum->unique_segment::sides)
	{
		PUT_INTEL_SHORT(&multibuf[count], static_cast<uint16_t>(i.tmap_num2));
		count+=2;
	}
	multi_send_data_direct(multibuf, pnum, 2);
}

namespace {

static void multi_do_light(const multiplayer_rspan<multiplayer_command_t::MULTI_LIGHT> buf)
{
	const auto sides = buf[3];

	const auto &&usegp = vmsegptridx.check_untrusted(segnum_t{GET_INTEL_SHORT(&buf[1])});
	if (!usegp)
		return;
	const auto &&segp = *usegp;
	auto &side_array = segp->unique_segment::sides;
	for (const auto i : MAX_SIDES_PER_SEGMENT)
	{
		if (sides & underlying_value(build_sidemask(i)))
		{
			auto &LevelSharedDestructibleLightState = LevelSharedSegmentState.DestructibleLights;
			subtract_light(LevelSharedDestructibleLightState, segp, i);
			const auto tmap_num2 = texture2_value{GET_INTEL_SHORT(&buf[4 + (2 * underlying_value(i))])};
			if (get_texture_index(tmap_num2) >= Textures.size())
				continue;
			side_array[i].tmap_num2 = tmap_num2;
		}
	}
}

static void multi_do_flags(fvmobjptr &vmobjptr, const playernum_t pnum, const multiplayer_rspan<multiplayer_command_t::MULTI_FLAGS> buf)
{
	if (pnum!=Player_num)
	{
		const auto flags{GET_INTEL_INT(&buf[2])};
		vmobjptr(vcplayerptr(pnum)->objnum)->ctype.player_info.powerup_flags = player_flags(flags);
	}
}

}

void multi_send_flags (const playernum_t pnum)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	multi_command<multiplayer_command_t::MULTI_FLAGS> multibuf;
	multibuf[1]=pnum;
	PUT_INTEL_INT(&multibuf[2], vmobjptr(vcplayerptr(pnum)->objnum)->ctype.player_info.powerup_flags.get_player_flags());
 
	multi_send_data(multibuf, multiplayer_data_priority::_2);
}

void multi_send_drop_blobs (const playernum_t pnum)
{
	multi_command<multiplayer_command_t::MULTI_DROP_BLOB> multibuf;
	multibuf[1]=pnum;

	multi_send_data(multibuf, multiplayer_data_priority::_0);
}

namespace {

static void multi_do_drop_blob(fvmobjptr &vmobjptr, const playernum_t pnum)
{
	drop_afterburner_blobs (vmobjptr(vcplayerptr(pnum)->objnum), 2, i2f(5) / 2, -1);
}

}

void multi_send_sound_function (char whichfunc, char sound)
{
	multi_command<multiplayer_command_t::MULTI_SOUND_FUNCTION> multibuf;
	multibuf[1]=Player_num;
	multibuf[2]=whichfunc;
	multibuf[3] = sound;       // this would probably work on the PC as well.  Jason?
	multi_send_data(multibuf, multiplayer_data_priority::_2);
}

#define AFTERBURNER_LOOP_START  20098
#define AFTERBURNER_LOOP_END    25776

namespace {

static void multi_do_sound_function(const playernum_t pnum, const multiplayer_rspan<multiplayer_command_t::MULTI_SOUND_FUNCTION> buf)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptridx = Objects.vcptridx;
	// for afterburner

	if (get_local_player().connected != player_connection_status::playing)
		return;

	const auto plobj = vcobjptridx(vcplayerptr(pnum)->objnum);
	const auto whichfunc = buf[2];
	if (whichfunc==0)
		digi_kill_sound_linked_to_object(plobj);
	else if (whichfunc==3)
	{
		const int sound = buf[3];
		digi_link_sound_to_object3(sound, plobj, 1, F1_0, sound_stack::allow_stacking, vm_distance{i2f(256)}, AFTERBURNER_LOOP_START, AFTERBURNER_LOOP_END);
	}
}

}

void multi_send_capture_bonus (const playernum_t pnum)
{
	multi_command<multiplayer_command_t::MULTI_CAPTURE_BONUS> multibuf;
	assert(game_mode_capture_flag(Game_mode));

	multibuf[1]=pnum;

	multi_send_data(multibuf, multiplayer_data_priority::_2);
	multi_do_capture_bonus (pnum);
}

void multi_send_orb_bonus (const playernum_t pnum, const uint8_t hoard_orbs)
{
	multi_command<multiplayer_command_t::MULTI_ORB_BONUS> multibuf;
	assert(game_mode_hoard(Game_mode));

	multibuf[1]=pnum;
	multibuf[2] = hoard_orbs;

	multi_send_data(multibuf, multiplayer_data_priority::_2);
	multi_do_orb_bonus(pnum, multibuf);
}

namespace {

void multi_do_capture_bonus(const playernum_t pnum)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	// Figure out the results of a network kills and add it to the
	// appropriate player's tally.

	int TheGoal;

	if (pnum==Player_num)
		HUD_init_message_literal(HM_MULTI, "You have Scored!");
	else
		HUD_init_message(HM_MULTI, "%s has Scored!", static_cast<const char *>(vcplayerptr(pnum)->callsign));

	digi_play_sample(pnum == Player_num
		? SOUND_HUD_YOU_GOT_GOAL
		: (get_team(pnum) == team_number::blue
			? SOUND_HUD_BLUE_GOT_GOAL
			: SOUND_HUD_RED_GOT_GOAL
		), F1_0*2);


	team_kills[get_team(pnum)] += 5;
	auto &plr = *vcplayerptr(pnum);
	auto &player_info = vmobjptr(plr.objnum)->ctype.player_info;
	player_info.powerup_flags &= ~PLAYER_FLAGS_FLAG;  // Clear capture flag
	player_info.net_kills_total += 5;
	player_info.KillGoalCount += 5;

	if (Netgame.KillGoal>0)
	{
		TheGoal=Netgame.KillGoal*5;

		if (player_info.KillGoalCount >= TheGoal)
		{
			if (pnum==Player_num)
			{
				HUD_init_message_literal(HM_MULTI, "You reached the kill goal!");
				get_local_plrobj().shields = i2f(200);
			}
			else
				HUD_init_message(HM_MULTI, "%s has reached the kill goal!",static_cast<const char *>(vcplayerptr(pnum)->callsign));
			net_destroy_controlcen(Objects, LevelSharedRobotInfoState.Robot_info);
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

void multi_do_orb_bonus(const playernum_t pnum, const multiplayer_rspan<multiplayer_command_t::MULTI_ORB_BONUS> buf)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	// Figure out the results of a network kills and add it to the
	// appropriate player's tally.

	int TheGoal;
	int bonus=GetOrbBonus (buf[2]);

	if (pnum==Player_num)
		HUD_init_message(HM_MULTI, "You have scored %d points!",bonus);
	else
		HUD_init_message(HM_MULTI, "%s has scored with %d orbs!",static_cast<const char *>(vcplayerptr(pnum)->callsign), buf[2]);

	if (pnum==Player_num)
		digi_start_sound_queued (SOUND_HUD_YOU_GOT_GOAL,F1_0*2);
	else
		digi_play_sample((Game_mode & GM_TEAM)
			? (get_team(pnum) == team_number::blue
				? SOUND_HUD_BLUE_GOT_GOAL
				: SOUND_HUD_RED_GOT_GOAL
			) : SOUND_OPPONENT_HAS_SCORED, F1_0*2);

	if (bonus > hoard_highest_record_stats.points)
	{
		hoard_highest_record_stats.player = pnum;
		hoard_highest_record_stats.points = bonus;
		if (pnum==Player_num)
			HUD_init_message(HM_MULTI, "You have the record with %d points!",bonus);
		else
			HUD_init_message(HM_MULTI, "%s has the record with %d points!",static_cast<const char *>(vcplayerptr(pnum)->callsign),bonus);
		digi_play_sample (SOUND_BUDDY_MET_GOAL,F1_0*2);
	}


	team_kills[get_team(pnum)] += bonus;
	auto &plr = *vcplayerptr(pnum);
	auto &player_info = vmobjptr(plr.objnum)->ctype.player_info;
	player_info.powerup_flags &= ~PLAYER_FLAGS_FLAG;  // Clear orb flag
	player_info.net_kills_total += bonus;
	player_info.KillGoalCount += bonus;

	team_kills[get_team(pnum)]%=1000;
	player_info.net_kills_total%=1000;
	player_info.KillGoalCount %= 1000;

	if (Netgame.KillGoal>0)
	{
		TheGoal=Netgame.KillGoal*5;

		if (player_info.KillGoalCount >= TheGoal)
		{
			if (pnum==Player_num)
			{
				HUD_init_message_literal(HM_MULTI, "You reached the kill goal!");
				get_local_plrobj().shields = i2f(200);
			}
			else
				HUD_init_message(HM_MULTI, "%s has reached the kill goal!",static_cast<const char *>(vcplayerptr(pnum)->callsign));
			net_destroy_controlcen(Objects, LevelSharedRobotInfoState.Robot_info);
		}
	}
	multi_sort_kill_list();
	multi_show_player_list();
}

}

void multi_send_got_flag (const playernum_t pnum)
{
	multi_command<multiplayer_command_t::MULTI_GOT_FLAG> multibuf;
	multibuf[1]=pnum;

	digi_start_sound_queued (SOUND_HUD_YOU_GOT_FLAG,F1_0*2);

	multi_send_data(multibuf, multiplayer_data_priority::_2);
	multi_send_flags (Player_num);
}

void multi_send_got_orb (const playernum_t pnum)
{
	multi_command<multiplayer_command_t::MULTI_GOT_ORB> multibuf;
	multibuf[1]=pnum;

	digi_play_sample (SOUND_YOU_GOT_ORB,F1_0*2);

	multi_send_data(multibuf, multiplayer_data_priority::_2);
	multi_send_flags (Player_num);
}

namespace {

static void multi_do_got_flag (const playernum_t pnum)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	digi_start_sound_queued(pnum == Player_num
		? SOUND_HUD_YOU_GOT_FLAG
		: (get_team(pnum) == team_number::blue
			? SOUND_HUD_BLUE_GOT_FLAG
			: SOUND_HUD_RED_GOT_FLAG
		), F1_0*2);
	vmobjptr(vcplayerptr(pnum)->objnum)->ctype.player_info.powerup_flags |= PLAYER_FLAGS_FLAG;
	HUD_init_message(HM_MULTI, "%s picked up a flag!",static_cast<const char *>(vcplayerptr(pnum)->callsign));
}

static void multi_do_got_orb (const playernum_t pnum)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	assert(game_mode_hoard(Game_mode));

	digi_play_sample((Game_mode & GM_TEAM) && get_team(pnum) == get_team(Player_num)
		? SOUND_FRIEND_GOT_ORB
		: SOUND_OPPONENT_GOT_ORB, F1_0*2);

	const auto &&objp = vmobjptr(vcplayerptr(pnum)->objnum);
	objp->ctype.player_info.powerup_flags |= PLAYER_FLAGS_FLAG;
	HUD_init_message(HM_MULTI, "%s picked up an orb!",static_cast<const char *>(vcplayerptr(pnum)->callsign));
}

static void DropOrb ()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	int seed;

	if (!game_mode_hoard(Game_mode))
		Int3(); // How did we get here? Get Leighton!

	auto &player_info = get_local_plrobj().ctype.player_info;
	auto &proximity = player_info.hoard.orbs;
	if (!proximity)
	{
		HUD_init_message_literal(HM_MULTI, "No orbs to drop!");
		return;
	}

	seed = d_rand();

	const auto &&objnum = spit_powerup(LevelUniqueObjectState, LevelSharedSegmentState, LevelUniqueSegmentState, Vclip, *ConsoleObject, powerup_type_t::POW_HOARD_ORB, seed);
	if (objnum == object_none)
	{
		HUD_init_message_literal(HM_MULTI, "Failed to drop orb!");
		return;
	}

	HUD_init_message_literal(HM_MULTI, "Orb dropped!");
	digi_play_sample (SOUND_DROP_WEAPON,F1_0);

	multi_send_drop_flag(objnum, seed);
	-- proximity;

	// If empty, tell everyone to stop drawing the box around me
	if (!proximity)
	{
		player_info.powerup_flags &=~(PLAYER_FLAGS_FLAG);
		multi_send_flags (Player_num);
	}
}

}

void DropFlag ()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	int seed;

	if (game_mode_hoard(Game_mode))
	{
		DropOrb();
		return;
	}
	else if (!game_mode_capture_flag(Game_mode))
		return;

	auto &player_info = get_local_plrobj().ctype.player_info;
	if (!(player_info.powerup_flags & PLAYER_FLAGS_FLAG))
	{
		HUD_init_message_literal(HM_MULTI, "No flag to drop!");
		return;
	}
	seed = d_rand();
	const auto &&objnum = spit_powerup(LevelUniqueObjectState, LevelSharedSegmentState, LevelUniqueSegmentState, Vclip, *ConsoleObject, get_team(Player_num) == team_number::blue ? powerup_type_t::POW_FLAG_RED : powerup_type_t::POW_FLAG_BLUE, seed);
	if (objnum == object_none)
	{
		HUD_init_message_literal(HM_MULTI, "Failed to drop flag!");
		return;
	}

	HUD_init_message_literal(HM_MULTI, "Flag dropped!");
	digi_play_sample (SOUND_DROP_WEAPON,F1_0);

	if (game_mode_capture_flag(Game_mode))
		multi_send_drop_flag(objnum,seed);

	player_info.powerup_flags &=~(PLAYER_FLAGS_FLAG);
}

namespace {

void multi_send_drop_flag(const vmobjptridx_t objp, int seed)
{
	multi_command<multiplayer_command_t::MULTI_DROP_FLAG> multibuf;
	int count=0;
	count++;
	multibuf[count++]=static_cast<char>(get_powerup_id(objp));

	PUT_INTEL_SHORT(&multibuf[count], objp.get_unchecked_index());
	count += 2;
	PUT_INTEL_INT(&multibuf[count], seed);

	map_objnum_local_to_local(objp);

	multi_send_data(multibuf, multiplayer_data_priority::_2);
}

static void multi_do_drop_flag(const playernum_t pnum, const multiplayer_rspan<multiplayer_command_t::MULTI_DROP_FLAG> buf)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	const auto powerup_id = static_cast<powerup_type_t>(buf[1]);
	const objnum_t remote_objnum = GET_INTEL_SHORT(&buf[2]);
	const auto seed{GET_INTEL_INT<int32_t>(&buf[6])};

	auto &plrobj{*vmobjptr(vcplayerptr(pnum)->objnum)};

	const imobjidx_t objnum{spit_powerup(LevelUniqueObjectState, LevelSharedSegmentState, LevelUniqueSegmentState, Vclip, plrobj, powerup_id, seed)};
	if (objnum == object_none)
		return;

	map_objnum_local_to_remote(objnum, remote_objnum, pnum);
	if (!game_mode_hoard(Game_mode))
		plrobj.ctype.player_info.powerup_flags &= ~(PLAYER_FLAGS_FLAG);
}

}
#endif

netflag_flag multi_powerup_is_allowed(const powerup_type_t id, const netflag_flag AllowedItems)
{
	return multi_powerup_is_allowed(id, AllowedItems, map_granted_flags_to_netflag(Netgame.SpawnGrantedItems));
}

netflag_flag multi_powerup_is_allowed(const powerup_type_t id, const netflag_flag BaseAllowedItems, const netflag_flag SpawnGrantedItems)
{
	const auto AllowedItems = BaseAllowedItems & ~SpawnGrantedItems;
	switch (id)
	{
		case powerup_type_t::POW_KEY_BLUE:
		case powerup_type_t::POW_KEY_GOLD:
		case powerup_type_t::POW_KEY_RED:
			/* Callers only test whether the result is netflag_flag::None or
			 * not, and do not expect a specific value. */
			return static_cast<netflag_flag>(Game_mode & GM_MULTI_COOP);
		case powerup_type_t::POW_INVULNERABILITY:
			return AllowedItems & netflag_flag::NETFLAG_DOINVUL;
		case powerup_type_t::POW_CLOAK:
			return AllowedItems & netflag_flag::NETFLAG_DOCLOAK;
		case powerup_type_t::POW_LASER:
			if (map_granted_flags_to_laser_level(Netgame.SpawnGrantedItems) >= MAX_LASER_LEVEL)
				/* If players are granted maximum level lasers, then disallow
				 * placing laser powerups.
				 */
				return netflag_flag::None;
			return AllowedItems & netflag_flag::NETFLAG_DOLASER;
		case powerup_type_t::POW_QUAD_FIRE:
			return AllowedItems & netflag_flag::NETFLAG_DOQUAD;
		case powerup_type_t::POW_VULCAN_WEAPON:
			return AllowedItems & netflag_flag::NETFLAG_DOVULCAN;
		case powerup_type_t::POW_SPREADFIRE_WEAPON:
			return AllowedItems & netflag_flag::NETFLAG_DOSPREAD;
		case powerup_type_t::POW_PLASMA_WEAPON:
			return AllowedItems & netflag_flag::NETFLAG_DOPLASMA;
		case powerup_type_t::POW_FUSION_WEAPON:
			return AllowedItems & netflag_flag::NETFLAG_DOFUSION;
		case powerup_type_t::POW_HOMING_AMMO_1:
		case powerup_type_t::POW_HOMING_AMMO_4:
			return AllowedItems & netflag_flag::NETFLAG_DOHOMING;
		case powerup_type_t::POW_PROXIMITY_WEAPON:
			return AllowedItems & netflag_flag::NETFLAG_DOPROXIM;
		case powerup_type_t::POW_SMARTBOMB_WEAPON:
			return AllowedItems & netflag_flag::NETFLAG_DOSMART;
		case powerup_type_t::POW_MEGA_WEAPON:
			return AllowedItems & netflag_flag::NETFLAG_DOMEGA;
		case powerup_type_t::POW_VULCAN_AMMO:
#if defined(DXX_BUILD_DESCENT_I)
			return BaseAllowedItems & netflag_flag::NETFLAG_DOVULCAN;
#elif defined(DXX_BUILD_DESCENT_II)
			return BaseAllowedItems & (netflag_flag::NETFLAG_DOVULCAN | netflag_flag::NETFLAG_DOGAUSS);
#endif
#if defined(DXX_BUILD_DESCENT_II)
		case powerup_type_t::POW_SUPER_LASER:
			if (map_granted_flags_to_laser_level(Netgame.SpawnGrantedItems) >= MAX_SUPER_LASER_LEVEL)
				/* If players are granted maximum level super lasers, then
				 * disallow placing super laser powerups.
				 */
				return netflag_flag::None;
			return AllowedItems & netflag_flag::NETFLAG_DOSUPERLASER;
		case powerup_type_t::POW_GAUSS_WEAPON:
			return AllowedItems & netflag_flag::NETFLAG_DOGAUSS;
		case powerup_type_t::POW_HELIX_WEAPON:
			return AllowedItems & netflag_flag::NETFLAG_DOHELIX;
		case powerup_type_t::POW_PHOENIX_WEAPON:
			return AllowedItems & netflag_flag::NETFLAG_DOPHOENIX;
		case powerup_type_t::POW_OMEGA_WEAPON:
			return AllowedItems & netflag_flag::NETFLAG_DOOMEGA;
		case powerup_type_t::POW_SMISSILE1_1:
		case powerup_type_t::POW_SMISSILE1_4:
			return AllowedItems & netflag_flag::NETFLAG_DOFLASH;
		case powerup_type_t::POW_GUIDED_MISSILE_1:
		case powerup_type_t::POW_GUIDED_MISSILE_4:
			return AllowedItems & netflag_flag::NETFLAG_DOGUIDED;
		case powerup_type_t::POW_SMART_MINE:
			return AllowedItems & netflag_flag::NETFLAG_DOSMARTMINE;
		case powerup_type_t::POW_MERCURY_MISSILE_1:
		case powerup_type_t::POW_MERCURY_MISSILE_4:
			return AllowedItems & netflag_flag::NETFLAG_DOMERCURY;
		case powerup_type_t::POW_EARTHSHAKER_MISSILE:
			return AllowedItems & netflag_flag::NETFLAG_DOSHAKER;
		case powerup_type_t::POW_AFTERBURNER:
			return AllowedItems & netflag_flag::NETFLAG_DOAFTERBURNER;
		case powerup_type_t::POW_CONVERTER:
			return AllowedItems & netflag_flag::NETFLAG_DOCONVERTER;
		case powerup_type_t::POW_AMMO_RACK:
			return AllowedItems & netflag_flag::NETFLAG_DOAMMORACK;
		case powerup_type_t::POW_HEADLIGHT:
			return AllowedItems & netflag_flag::NETFLAG_DOHEADLIGHT;
		case powerup_type_t::POW_FLAG_BLUE:
		case powerup_type_t::POW_FLAG_RED:
			return netflag_flag{game_mode_capture_flag(Game_mode)};
#endif
		default:
			return netflag_flag{1};
	}
}

#if defined(DXX_BUILD_DESCENT_II)
void multi_send_finish_game ()
{
	multi_command<multiplayer_command_t::MULTI_FINISH_GAME> multibuf;
	multibuf[1]=Player_num;

	multi_send_data(multibuf, multiplayer_data_priority::_2);
}

namespace {

static void multi_do_finish_game()
{
	if (Current_level_num != Current_mission->last_level)
		return;

	do_final_boss_hacks();
}

}

void multi_send_trigger_specific(const playernum_t pnum, const trgnum_t trig)
{
	multi_command<multiplayer_command_t::MULTI_START_TRIGGER> multibuf;
	static_assert(sizeof(trgnum_t) == sizeof(uint8_t), "trigger number could be truncated");
	multibuf[1] = static_cast<uint8_t>(trig);

	multi_send_data_direct(multibuf, pnum, 2);
}

namespace {

static void multi_do_start_trigger(const multiplayer_rspan<multiplayer_command_t::MULTI_START_TRIGGER> buf)
{
	auto &Triggers = LevelUniqueWallSubsystemState.Triggers;
	auto &vmtrgptr = Triggers.vmptr;
	const auto &&utrg = vmtrgptr.check_untrusted(static_cast<trgnum_t>(buf[1]));
	if (!utrg)
		return;
	auto &trg = **utrg;
	trg.flags |= trigger_behavior_flags::disabled;
}

}
#endif

namespace {
static void multi_adjust_lifetime_ranking(int &k, const int count)
{
	if (!(Game_mode & GM_NETWORK))
		return;

	const auto oldrank = GetMyNetRanking();
	k += count;
	const auto newrank = GetMyNetRanking();
	if (oldrank != newrank)
	{
		Netgame.players[Player_num].rank = newrank;
		multi_send_ranking(newrank);
		if (!PlayerCfg.NoRankings)
		{
			HUD_init_message(HM_MULTI, "You have been %smoted to %s!", newrank > oldrank ? "pro" : "de", RankStrings[newrank]);
#if defined(DXX_BUILD_DESCENT_I)
			digi_play_sample (SOUND_CONTROL_CENTER_WARNING_SIREN,F1_0*2);
#elif defined(DXX_BUILD_DESCENT_II)
			digi_play_sample (SOUND_BUDDY_MET_GOAL,F1_0*2);
#endif
		}
	}
}
}

#if !(!defined(RELEASE) && defined(DXX_BUILD_DESCENT_II))
namespace {
#endif
void multi_add_lifetime_kills(const int count)
{
	// This function adds a kill to lifetime stats of this player, and possibly
	// gives a promotion.  If so, it will tell everyone else

	multi_adjust_lifetime_ranking(PlayerCfg.NetlifeKills, count);
}
#if !(!defined(RELEASE) && defined(DXX_BUILD_DESCENT_II))
}
#endif

namespace {
void multi_add_lifetime_killed ()
{
	// This function adds a "killed" to lifetime stats of this player, and possibly
	// gives a demotion.  If so, it will tell everyone else

	if (Game_mode & GM_MULTI_COOP)
		return;

	multi_adjust_lifetime_ranking(PlayerCfg.NetlifeKilled, 1);
}
}
}

namespace {

void multi_send_ranking (const netplayer_info::player_rank newrank)
{
	multi_command<multiplayer_command_t::MULTI_RANK> multibuf;
	multibuf[1]=static_cast<char>(Player_num);
	multibuf[2] = underlying_value(newrank);

	multi_send_data(multibuf, multiplayer_data_priority::_2);
}

static void multi_do_ranking(const playernum_t pnum, const multiplayer_rspan<multiplayer_command_t::MULTI_RANK> buf)
{
	const auto rank = build_rank_from_untrusted(buf[2]);
	if (rank == netplayer_info::player_rank::None)
		return;
	if (!RankStrings.valid_index(rank))
		return;

	auto &netrank = Netgame.players[pnum].rank;
	if (netrank == rank)
		return;
	const auto rankstr = (netrank < rank) ? "pro" : "de";
	netrank = rank;

	if (!PlayerCfg.NoRankings)
		HUD_init_message(HM_MULTI, "%s has been %smoted to %s!",static_cast<const char *>(vcplayerptr(pnum)->callsign), rankstr, RankStrings[rank]);
}

}

namespace dcx {

// Decide if fire from "killer" is friendly. If yes return 1 (no harm to me) otherwise 0 (damage me)
int multi_maybe_disable_friendly_fire(const object_base *const killer)
{
	if (!(Game_mode & GM_NETWORK)) // no Multiplayer game -> always harm me!
		return 0;
	if (!Netgame.NoFriendlyFire) // friendly fire is activated -> harm me!
		return 0;
	if (!killer) // no actual killer -> harm me!
		return 0;
	if (killer->type != OBJ_PLAYER) // not a player -> harm me!
		return 0;
	if (auto is_coop = Game_mode & GM_MULTI_COOP) // coop mode -> don't harm me!
		return is_coop;
	else if (Game_mode & GM_TEAM) // team mode - find out if killer is in my team
	{
		if (get_team(Player_num) == get_team(get_player_id(*killer))) // in my team -> don't harm me!
			return 1;
		else // opposite team -> harm me!
			return 0;
	}
	return 0; // all other cases -> harm me!
}

namespace {

void multi_new_bounty_target(playernum_t pnum, const char *const callsign)
{
	/* If it's already the same, don't do it */
	if (Bounty_target == pnum)
		return;
	/* Set the target */
	Bounty_target = pnum;
	/* Send a message */
	HUD_init_message(HM_MULTI, "%c%c%s is the new target!", CC_COLOR, BM_XRGB(player_rgb[pnum].r, player_rgb[pnum].g, player_rgb[pnum].b), callsign);
}

}

}

/* Bounty packer sender and handler */
void multi_send_bounty( void )
{
	/* Test game mode */
	if( !( Game_mode & GM_BOUNTY ) )
		return;
	if ( !multi_i_am_master() )
		return;
	
	multi_command<multiplayer_command_t::MULTI_DO_BOUNTY> multibuf;
	/* Add opcode, target ID and how often we re-assigned */
	multibuf[1] = static_cast<char>(Bounty_target);
	
	/* Send data */
	multi_send_data(multibuf, multiplayer_data_priority::_2);
}

namespace dsx {

namespace {

static void multi_do_bounty(const multiplayer_rspan<multiplayer_command_t::MULTI_DO_BOUNTY> buf)
{
	if ( multi_i_am_master() )
		return;
	const unsigned pnum = buf[1];
	multi_new_bounty_target_with_sound(pnum, vcplayerptr(pnum)->callsign);
}

void multi_new_bounty_target_with_sound(const playernum_t pnum, const char *const callsign)
{
#if defined(DXX_BUILD_DESCENT_I)
	digi_play_sample( SOUND_CONTROL_CENTER_WARNING_SIREN, F1_0 * 3 );
#elif defined(DXX_BUILD_DESCENT_II)
	digi_play_sample( SOUND_BUDDY_MET_GOAL, F1_0 * 2 );
#endif
	multi_new_bounty_target(pnum, callsign);
}

static void multi_do_save_game(const multiplayer_rspan<multiplayer_command_t::MULTI_SAVE_GAME> buf)
{
	int count = 1;
	ubyte slot;
	d_game_unique_state::savegame_description desc;

	slot = buf[count];			count += 1;
	const auto id{GET_INTEL_INT(&buf[count])};			count += 4;
	memcpy(desc.data(), &buf[count], desc.size());
	desc.back() = 0;

	multi_save_game(static_cast<unsigned>(slot), id, desc);
}

}

}

namespace {

static void multi_do_restore_game(const multiplayer_rspan<multiplayer_command_t::MULTI_RESTORE_GAME> buf)
{
	int count = 1;
	ubyte slot;

	slot = buf[count];			count += 1;
	const unsigned id{GET_INTEL_INT(&buf[count])};			count += 4;

	multi_restore_game( slot, id );
}

}

namespace dcx {
namespace {

static void multi_send_save_game(const d_game_unique_state::save_slot slot, const unsigned id, const d_game_unique_state::savegame_description &desc)
{
	int count = 0;
	
	count += 1;
	multi_command<multiplayer_command_t::MULTI_SAVE_GAME> multibuf;
	multibuf[count] = static_cast<uint8_t>(slot);				count += 1; // Save slot=0
	PUT_INTEL_INT(&multibuf[count], id );		count += 4; // Save id
	memcpy(&multibuf[count], desc.data(), desc.size());

	multi_send_data(multibuf, multiplayer_data_priority::_2);
}

static void multi_send_restore_game(ubyte slot, uint id)
{
	int count = 0;
	
	count += 1;
	multi_command<multiplayer_command_t::MULTI_RESTORE_GAME> multibuf;
	multibuf[count] = slot;				count += 1; // Save slot=0
	PUT_INTEL_INT(&multibuf[count], id );		count += 4; // Save id

	multi_send_data(multibuf, multiplayer_data_priority::_2);
}

static void multi_do_msgsend_state(const playernum_t pnum, const multiplayer_rspan<multiplayer_command_t::MULTI_TYPING_STATE> buf)
{
	multi_sending_message[pnum] = msgsend_state{buf[2]};
}

}
}

namespace dsx {

void multi_initiate_save_game()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	auto &vcobjptr = Objects.vcptr;

	if (const auto reason = multi_i_am_master() ? multi_interactive_deny_save_game(vcobjptr, partial_range(Players, N_players), LevelUniqueControlCenterState) : "Only the host is allowed to save a game!")
	{
		HUD_init_message(HM_MULTI, "Cannot save: %s", reason);
		return;
	}

	d_game_unique_state::savegame_file_path filename{};
	d_game_unique_state::savegame_description desc{};
	const auto slot = state_get_save_file(*grd_curcanv, filename, &desc, blind_save::no);
	if (!GameUniqueState.valid_save_slot(slot))
		return;
	const auto &&player_range = partial_const_range(Players, N_players);
	// Execute "alive" and "duplicate callsign" checks again in case things changed while host decided upon the savegame.
	if (const auto reason = multi_interactive_deny_save_game(vcobjptr, player_range, LevelUniqueControlCenterState))
	{
		HUD_init_message(HM_MULTI, "Cannot save: %s", reason);
		return;
	}
	multi_execute_save_game(slot, desc, player_range);
}

void multi_execute_save_game(const d_game_unique_state::save_slot slot, const d_game_unique_state::savegame_description &desc, const std::ranges::subrange<const player *> player_range)
{
	// Make a unique game id
	fix game_id;
	game_id = static_cast<fix>(timer_query());
	game_id ^= N_players<<4;
	range_for (auto &i, player_range)
	{
		fix call2i;
		memcpy(&call2i, static_cast<const char *>(i.callsign), sizeof(fix));
		game_id ^= call2i;
	}
	if ( game_id == 0 )
		game_id = 1; // 0 is invalid

	multi_send_save_game( slot, game_id, desc );
	multi_do_frame();
	multi_save_game(static_cast<unsigned>(slot), game_id, desc);
}

void multi_initiate_restore_game()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;

	if (Network_status == network_state::endlevel || LevelUniqueControlCenterState.Control_center_destroyed)
		return;

	if (const auto reason = multi_i_am_master() ? multi_interactive_deny_save_game(vcobjptr, partial_const_range(Players, N_players), LevelUniqueControlCenterState) : "Only host is allowed to load a game!")
	{
		HUD_init_message(HM_MULTI, "Cannot load: %s", reason);
		return;
	}
	d_game_unique_state::savegame_file_path filename;
	const auto eslot = state_get_restore_file(*grd_curcanv, filename, blind_save::no);
	if (!GameUniqueState.valid_load_slot(eslot))
		return;
	/* Recheck the interactive conditions, but not the host status.  If
	 * this system was the host before, it must still be the host now.
	 */
	if (const auto reason = multi_interactive_deny_save_game(vcobjptr, partial_const_range(Players, N_players), LevelUniqueControlCenterState))
	{
		HUD_init_message(HM_MULTI, "Cannot load: %s", reason);
		return;
	}
	state_game_id = state_get_game_id(filename);
	if (!state_game_id)
		return;
	const unsigned slot = static_cast<unsigned>(eslot);
	multi_send_restore_game(slot,state_game_id);
	multi_do_frame();
	multi_restore_game(slot,state_game_id);
}

namespace {

void multi_save_game(const unsigned slot, const unsigned id, const d_game_unique_state::savegame_description &desc)
{
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	char filename[PATH_MAX];

	if (Network_status == network_state::endlevel || LevelUniqueControlCenterState.Control_center_destroyed)
		return;

	snprintf(filename, sizeof(filename), PLAYER_DIRECTORY_STRING("%s.mg%x"), static_cast<const char *>(get_local_player().callsign), slot);
	HUD_init_message(HM_MULTI, "Saving game #%d, '%s'", slot, desc.data());
	state_game_id = id;
	pause_game_world_time p;
	state_save_all_sub(filename, desc.data());
}

void multi_restore_game(const unsigned slot, const unsigned id)
{
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	d_game_unique_state::savegame_file_path filename;

	if (Network_status == network_state::endlevel || LevelUniqueControlCenterState.Control_center_destroyed)
		return;

	auto &plr = get_local_player();
	snprintf(filename.data(), filename.size(), PLAYER_DIRECTORY_STRING("%s.mg%x"), static_cast<const char *>(plr.callsign), slot);
   
	for (unsigned i = 0, n = N_players; i < n; ++i)
		multi_strip_robots(i);
	if (multi_i_am_master()) // put all players to wait-state again so we can sync up properly
		range_for (auto &i, Players)
			if (i.connected == player_connection_status::playing && &i != &plr)
				i.connected = player_connection_status::waiting;
   
	const auto thisid = state_get_game_id(filename);
	if (thisid!=id)
	{
		nm_messagebox_str(menu_title{nullptr}, nm_messagebox_tie(TXT_OK), menu_subtitle{"A multi-save game was restored\nthat you are missing or does not\nmatch that of the others.\nYou must rejoin if you wish to\ncontinue."});
		return;
	}

#if defined(DXX_BUILD_DESCENT_II)
	auto &LevelSharedDestructibleLightState = LevelSharedSegmentState.DestructibleLights;
#endif
	state_restore_all_sub(
#if defined(DXX_BUILD_DESCENT_II)
		LevelSharedDestructibleLightState, secret_restore::none,
#endif
		filename.data());
	multi_send_score(); // send my restored scores. I sent 0 when I loaded the level anyways...
}

}

}

void multi_send_msgsend_state(const msgsend_state state)
{
	multi_command<multiplayer_command_t::MULTI_TYPING_STATE> multibuf;
	/* Obsolete - reclaim player number field on next multiplayer protocol version bump */
	multibuf[1] = Player_num;
	multibuf[2] = static_cast<char>(state);
	
	multi_send_data(multibuf, multiplayer_data_priority::_2);
}

namespace {

// Specific variables related to our game mode we want the clients to know about
void multi_send_gmode_update()
{
	if (!multi_i_am_master())
		return;
	if (!(Game_mode & GM_TEAM || Game_mode & GM_BOUNTY)) // expand if necessary
		return;
	multi_command<multiplayer_command_t::MULTI_GMODE_UPDATE> multibuf;
	multibuf[1] = Netgame.team_vector;
	multibuf[2] = Bounty_target;
	
	multi_send_data(multibuf, multiplayer_data_priority::_0);
}

static void multi_do_gmode_update(const multiplayer_rspan<multiplayer_command_t::MULTI_GMODE_UPDATE> buf)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	if (multi_i_am_master())
		return;
	if (Game_mode & GM_TEAM)
	{
		if (buf[1] != Netgame.team_vector)
		{
			Netgame.team_vector = buf[1];
			range_for (auto &t, partial_const_range(Players, N_players))
				if (t.connected != player_connection_status::disconnected)
					multi_reset_object_texture (vmobjptr(t.objnum));
			reset_cockpit();
		}
	}
	if (Game_mode & GM_BOUNTY)
	{
		Bounty_target = buf[2]; // accept silently - message about change we SHOULD have gotten due to kill computation
	}
}

}

/*
 * Send player inventory to all other players. Intended to be used for the host to repopulate the level with new powerups.
 * Could also be used to let host decide which powerups a client is allowed to collect and/or drop, anti-cheat functions (needs shield/energy update then and more frequent updates/triggers).
 */
namespace dsx {
void multi_send_player_inventory(const multiplayer_data_priority priority)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	multi_command<multiplayer_command_t::MULTI_PLAYER_INV> multibuf;
	int count = 0;

	count++;
	multibuf[count++] = Player_num;

	auto &player_info = get_local_plrobj().ctype.player_info;
	PUT_WEAPON_FLAGS(multibuf, count, player_info.primary_weapon_flags);
	multibuf[count++] = static_cast<char>(player_info.laser_level);

	auto &secondary_ammo = player_info.secondary_ammo;
	multibuf[count++] = secondary_ammo[secondary_weapon_index_t::HOMING_INDEX];
	multibuf[count++] = secondary_ammo[secondary_weapon_index_t::CONCUSSION_INDEX];
	multibuf[count++] = secondary_ammo[secondary_weapon_index_t::SMART_INDEX];
	multibuf[count++] = secondary_ammo[secondary_weapon_index_t::MEGA_INDEX];
	multibuf[count++] = secondary_ammo[secondary_weapon_index_t::PROXIMITY_INDEX];

#if defined(DXX_BUILD_DESCENT_II)
	multibuf[count++] = secondary_ammo[secondary_weapon_index_t::SMISSILE1_INDEX];
	multibuf[count++] = secondary_ammo[secondary_weapon_index_t::GUIDED_INDEX];
	multibuf[count++] = secondary_ammo[secondary_weapon_index_t::SMART_MINE_INDEX];
	multibuf[count++] = secondary_ammo[secondary_weapon_index_t::SMISSILE4_INDEX];
	multibuf[count++] = secondary_ammo[secondary_weapon_index_t::SMISSILE5_INDEX];
#endif

	PUT_INTEL_SHORT(&multibuf[count], player_info.vulcan_ammo);
	count += 2;
	PUT_INTEL_INT(&multibuf[count], player_info.powerup_flags.get_player_flags());
	count += 4;

	multi_send_data(multibuf, priority);
}

namespace {

static void multi_do_player_inventory(const playernum_t pnum, const multiplayer_rspan<multiplayer_command_t::MULTI_PLAYER_INV> buf)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	int count;

#ifdef NDEBUG
	if (pnum >= N_players || pnum == Player_num)
		return;
#else
	Assert(pnum < N_players);
        Assert(pnum != Player_num);
#endif

	count = 2;
#if defined(DXX_BUILD_DESCENT_I)
#define GET_WEAPON_FLAGS(buf,count)	buf[count++]
#elif defined(DXX_BUILD_DESCENT_II)
#define GET_WEAPON_FLAGS(buf,count)	(count += sizeof(uint16_t), GET_INTEL_SHORT(&buf[(count - sizeof(uint16_t))]))
#endif
	const auto &&objp = vmobjptridx(vcplayerptr(pnum)->objnum);
	auto &player_info = objp->ctype.player_info;
	player_info.primary_weapon_flags = GET_WEAPON_FLAGS(buf, count);
	player_info.laser_level = laser_level{buf[count]};
	count++;

	auto &secondary_ammo = player_info.secondary_ammo;
	secondary_ammo[secondary_weapon_index_t::HOMING_INDEX] = buf[count];                count++;
	secondary_ammo[secondary_weapon_index_t::CONCUSSION_INDEX] = buf[count];count++;
	secondary_ammo[secondary_weapon_index_t::SMART_INDEX] = buf[count];         count++;
	secondary_ammo[secondary_weapon_index_t::MEGA_INDEX] = buf[count];          count++;
	secondary_ammo[secondary_weapon_index_t::PROXIMITY_INDEX] = buf[count]; count++;

#if defined(DXX_BUILD_DESCENT_II)
	secondary_ammo[secondary_weapon_index_t::SMISSILE1_INDEX] = buf[count]; count++;
	secondary_ammo[secondary_weapon_index_t::GUIDED_INDEX]    = buf[count]; count++;
	secondary_ammo[secondary_weapon_index_t::SMART_MINE_INDEX]= buf[count]; count++;
	secondary_ammo[secondary_weapon_index_t::SMISSILE4_INDEX] = buf[count]; count++;
	secondary_ammo[secondary_weapon_index_t::SMISSILE5_INDEX] = buf[count]; count++;
#endif

	player_info.vulcan_ammo = GET_INTEL_SHORT(&buf[count]); count += 2;
	player_info.powerup_flags = player_flags(GET_INTEL_INT(&buf[count]));    count += 4;
}

/*
 * Count the inventory of the level. Initial (start) or current (now).
 * In 'current', also consider player inventories (and the thief bot).
 * NOTE: We add actual ammo amount - we do not want to count in 'amount of powerups'. Makes it easier to keep track of overhead (proximities, vulcan ammo)
 */
static void MultiLevelInv_CountLevelPowerups()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
        if (!(Game_mode & GM_MULTI) || (Game_mode & GM_MULTI_COOP))
                return;
        MultiLevelInv.Current = {};

        range_for (const auto &&objp, vmobjptridx)
        {
                if (objp->type == OBJ_WEAPON) // keep live bombs in inventory so they will respawn after they're gone
                {
                        auto wid = get_weapon_id(objp);
                        if (wid == weapon_id_type::PROXIMITY_ID)
                                MultiLevelInv.Current[powerup_type_t::POW_PROXIMITY_WEAPON]++;
#if defined(DXX_BUILD_DESCENT_II)
                        if (wid == weapon_id_type::SUPERPROX_ID)
                                MultiLevelInv.Current[powerup_type_t::POW_SMART_MINE]++;
#endif
                }
                if (objp->type != OBJ_POWERUP)
                        continue;
                auto pid = get_powerup_id(objp);
                switch (pid)
                {
					case powerup_type_t::POW_VULCAN_WEAPON:
#if defined(DXX_BUILD_DESCENT_II)
					case powerup_type_t::POW_GAUSS_WEAPON:
#endif
						MultiLevelInv.Current[powerup_type_t::POW_VULCAN_AMMO] += objp->ctype.powerup_info.count; // add contained ammo so we do not lose this from level when used up
						/* fall through to increment Current[pid] */
						[[fallthrough]];
                        case powerup_type_t::POW_LASER:
                        case powerup_type_t::POW_QUAD_FIRE:
                        case powerup_type_t::POW_SPREADFIRE_WEAPON:
                        case powerup_type_t::POW_PLASMA_WEAPON:
                        case powerup_type_t::POW_FUSION_WEAPON:
                        case powerup_type_t::POW_MISSILE_1:
                        case powerup_type_t::POW_HOMING_AMMO_1:
                        case powerup_type_t::POW_SMARTBOMB_WEAPON:
                        case powerup_type_t::POW_MEGA_WEAPON:
                        case powerup_type_t::POW_CLOAK:
                        case powerup_type_t::POW_INVULNERABILITY:
#if defined(DXX_BUILD_DESCENT_II)
                        case powerup_type_t::POW_SUPER_LASER:
                        case powerup_type_t::POW_HELIX_WEAPON:
                        case powerup_type_t::POW_PHOENIX_WEAPON:
                        case powerup_type_t::POW_OMEGA_WEAPON:
                        case powerup_type_t::POW_SMISSILE1_1:
                        case powerup_type_t::POW_GUIDED_MISSILE_1:
                        case powerup_type_t::POW_MERCURY_MISSILE_1:
                        case powerup_type_t::POW_EARTHSHAKER_MISSILE:
                        case powerup_type_t::POW_FULL_MAP:
                        case powerup_type_t::POW_CONVERTER:
                        case powerup_type_t::POW_AMMO_RACK:
                        case powerup_type_t::POW_AFTERBURNER:
                        case powerup_type_t::POW_HEADLIGHT:
                        case powerup_type_t::POW_FLAG_BLUE:
                        case powerup_type_t::POW_FLAG_RED:
#endif
                                MultiLevelInv.Current[pid]++;
                                break;
                        case powerup_type_t::POW_MISSILE_4:
                        case powerup_type_t::POW_HOMING_AMMO_4:
#if defined(DXX_BUILD_DESCENT_II)
                        case powerup_type_t::POW_SMISSILE1_4:
                        case powerup_type_t::POW_GUIDED_MISSILE_4:
                        case powerup_type_t::POW_MERCURY_MISSILE_4:
#endif
                                MultiLevelInv.Current[static_cast<powerup_type_t>(underlying_value(pid) - 1)] += 4;
                                break;
                        case powerup_type_t::POW_PROXIMITY_WEAPON:
#if defined(DXX_BUILD_DESCENT_II)
                        case powerup_type_t::POW_SMART_MINE:
#endif
                                MultiLevelInv.Current[pid] += 4; // count the actual bombs
                                break;
                        case powerup_type_t::POW_VULCAN_AMMO:
                                MultiLevelInv.Current[pid] += VULCAN_AMMO_AMOUNT; // count the actual ammo
                                break;
                        default:
                                break; // All other items either do not exist or we NEVER want to have them respawn.
                }
        }
}

static void MultiLevelInv_CountPlayerInventory()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	auto &Current = MultiLevelInv.Current;
                for (playernum_t i = 0; i < MAX_PLAYERS; i++)
                {
					if (vcplayerptr(i)->connected != player_connection_status::playing)
                                continue;
		auto &obj = *vcobjptr(vcplayerptr(i)->objnum);
		if (obj.type == OBJ_GHOST) // Player is dead. Their items are dropped now.
                                continue;
		auto &player_info = obj.ctype.player_info;
                        // NOTE: We do not need to consider Granted Spawn Items here. These are replaced with shields and even if not, the repopulation function will take care of it.
#if defined(DXX_BUILD_DESCENT_II)
                        if (player_info.laser_level > MAX_LASER_LEVEL)
                        {
                                /*
                                 * We do not know exactly how many normal lasers the player collected before going super so assume they have all.
                                 * This loss possible is insignificant since we have super lasers and normal ones may respawn some time after this player dies.
                                 */
							Current[powerup_type_t::POW_LASER] += 4;
							Current[powerup_type_t::POW_SUPER_LASER] += static_cast<unsigned>(player_info.laser_level) - static_cast<unsigned>(MAX_LASER_LEVEL) + 1; // Laser levels start at 0!
                        }
                        else
#endif
						{
							Current[powerup_type_t::POW_LASER] += static_cast<unsigned>(player_info.laser_level) + 1; // Laser levels start at 0!
                        }
						accumulate_flags_count<player_flags, PLAYER_FLAG> powerup_flags(Current, player_info.powerup_flags);
						accumulate_flags_count<player_info::primary_weapon_flag_type, unsigned> primary_weapon_flags(Current, player_info.primary_weapon_flags);
						powerup_flags.process(PLAYER_FLAGS_QUAD_LASERS, powerup_type_t::POW_QUAD_FIRE);
						primary_weapon_flags.process(HAS_PRIMARY_FLAG(primary_weapon_index_t::VULCAN_INDEX), powerup_type_t::POW_VULCAN_WEAPON);
						primary_weapon_flags.process(HAS_PRIMARY_FLAG(primary_weapon_index_t::SPREADFIRE_INDEX), powerup_type_t::POW_SPREADFIRE_WEAPON);
						primary_weapon_flags.process(HAS_PRIMARY_FLAG(primary_weapon_index_t::PLASMA_INDEX), powerup_type_t::POW_PLASMA_WEAPON);
						primary_weapon_flags.process(HAS_PRIMARY_FLAG(primary_weapon_index_t::FUSION_INDEX), powerup_type_t::POW_FUSION_WEAPON);
						powerup_flags.process(PLAYER_FLAGS_CLOAKED, powerup_type_t::POW_CLOAK);
						powerup_flags.process(PLAYER_FLAGS_INVULNERABLE, powerup_type_t::POW_INVULNERABILITY);
                        // NOTE: The following can probably be simplified.
#if defined(DXX_BUILD_DESCENT_II)
						primary_weapon_flags.process(HAS_PRIMARY_FLAG(primary_weapon_index_t::GAUSS_INDEX), powerup_type_t::POW_GAUSS_WEAPON);
						primary_weapon_flags.process(HAS_PRIMARY_FLAG(primary_weapon_index_t::HELIX_INDEX), powerup_type_t::POW_HELIX_WEAPON);
						primary_weapon_flags.process(HAS_PRIMARY_FLAG(primary_weapon_index_t::PHOENIX_INDEX), powerup_type_t::POW_PHOENIX_WEAPON);
						primary_weapon_flags.process(HAS_PRIMARY_FLAG(primary_weapon_index_t::OMEGA_INDEX), powerup_type_t::POW_OMEGA_WEAPON);
						powerup_flags.process(PLAYER_FLAGS_MAP_ALL, powerup_type_t::POW_FULL_MAP);
						powerup_flags.process(PLAYER_FLAGS_CONVERTER, powerup_type_t::POW_CONVERTER);
						powerup_flags.process(PLAYER_FLAGS_AMMO_RACK, powerup_type_t::POW_AMMO_RACK);
						powerup_flags.process(PLAYER_FLAGS_AFTERBURNER, powerup_type_t::POW_AFTERBURNER);
						powerup_flags.process(PLAYER_FLAGS_HEADLIGHT, powerup_type_t::POW_HEADLIGHT);
                        if ((Game_mode & GM_CAPTURE) && (player_info.powerup_flags & PLAYER_FLAGS_FLAG))
                        {
							++Current[(get_team(i) == team_number::blue) ? powerup_type_t::POW_FLAG_RED : powerup_type_t::POW_FLAG_BLUE];
                        }
#endif
		Current[powerup_type_t::POW_VULCAN_AMMO] += player_info.vulcan_ammo;
		Current[powerup_type_t::POW_MISSILE_1] += player_info.secondary_ammo[secondary_weapon_index_t::CONCUSSION_INDEX];
		Current[powerup_type_t::POW_HOMING_AMMO_1] += player_info.secondary_ammo[secondary_weapon_index_t::HOMING_INDEX];
		Current[powerup_type_t::POW_PROXIMITY_WEAPON] += player_info.secondary_ammo[secondary_weapon_index_t::PROXIMITY_INDEX];
		Current[powerup_type_t::POW_SMARTBOMB_WEAPON] += player_info.secondary_ammo[secondary_weapon_index_t::SMART_INDEX];
		Current[powerup_type_t::POW_MEGA_WEAPON] += player_info.secondary_ammo[secondary_weapon_index_t::MEGA_INDEX];
#if defined(DXX_BUILD_DESCENT_II)
		Current[powerup_type_t::POW_SMISSILE1_1] += player_info.secondary_ammo[secondary_weapon_index_t::SMISSILE1_INDEX];
		Current[powerup_type_t::POW_GUIDED_MISSILE_1] += player_info.secondary_ammo[secondary_weapon_index_t::GUIDED_INDEX];
		Current[powerup_type_t::POW_SMART_MINE] += player_info.secondary_ammo[secondary_weapon_index_t::SMART_MINE_INDEX];
		Current[powerup_type_t::POW_MERCURY_MISSILE_1] += player_info.secondary_ammo[secondary_weapon_index_t::SMISSILE4_INDEX];
		Current[powerup_type_t::POW_EARTHSHAKER_MISSILE] += player_info.secondary_ammo[secondary_weapon_index_t::SMISSILE5_INDEX];
#endif
                }
#if defined(DXX_BUILD_DESCENT_II)
                if (Game_mode & GM_MULTI_ROBOTS) // Add (possible) thief inventory
                {
                        range_for (auto &i, LevelUniqueObjectState.ThiefState.Stolen_items)
                        {
							if (!Current.valid_index(i) || i == powerup_type_t::POW_ENERGY || i == powerup_type_t::POW_SHIELD_BOOST)
                                        continue;
						auto &c = Current[i];
                                // NOTE: We don't need to consider vulcan ammo or 4pack items as the thief should not steal those items.
							if (i == powerup_type_t::POW_PROXIMITY_WEAPON || i == powerup_type_t::POW_SMART_MINE)
								c += 4;
                                else
								++c;
                        }
                }
#endif
}
}
}

void MultiLevelInv_InitializeCount()
{
	MultiLevelInv_CountLevelPowerups();
	MultiLevelInv.Initial = MultiLevelInv.Current;
	MultiLevelInv.RespawnTimer = {};
}

void MultiLevelInv_Recount()
{
	MultiLevelInv_CountLevelPowerups();
	MultiLevelInv_CountPlayerInventory();
}

// Takes a powerup type and checks if we are allowed to spawn it.
namespace dsx {
bool MultiLevelInv_AllowSpawn(powerup_type_t powerup_type)
{
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	if ((Game_mode & GM_MULTI_COOP) || LevelUniqueControlCenterState.Control_center_destroyed || Network_status != network_state::playing)
                return 0;

        int req_amount = 1; // required amount of item to drop a powerup.

        if (powerup_type == powerup_type_t::POW_VULCAN_AMMO)
                req_amount = VULCAN_AMMO_AMOUNT;
        else if (powerup_type == powerup_type_t::POW_PROXIMITY_WEAPON
#if defined(DXX_BUILD_DESCENT_II)
                || powerup_type == powerup_type_t::POW_SMART_MINE
#endif
        )
                req_amount = 4;

        if (MultiLevelInv.Initial[powerup_type] == 0 || MultiLevelInv.Current[powerup_type] > MultiLevelInv.Initial[powerup_type]) // Item does not exist in level or we have too many.
                return 0;
        else if (MultiLevelInv.Initial[powerup_type] - MultiLevelInv.Current[powerup_type] >= req_amount)
                return 1;
        return 0;
}

namespace {

// Repopulate the level with missing items.
void MultiLevelInv_Repopulate(fix frequency)
{
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	if (!multi_i_am_master() || (Game_mode & GM_MULTI_COOP) || LevelUniqueControlCenterState.Control_center_destroyed)
                return;

	MultiLevelInv_Recount(); // recount current items
	for (const uint8_t i : constant_xrange<uint8_t, uint8_t{0}, uint8_t{MAX_POWERUP_TYPES}>{})
	{
		const powerup_type_t pi{i};
		auto &rt = MultiLevelInv.RespawnTimer[pi];
		if (MultiLevelInv_AllowSpawn(pi))
			rt += frequency;
		else
		{
			rt = 0;
			continue;
		}

		if (rt >= F1_0*2)
		{
			con_printf(CON_VERBOSE, "MultiLevelInv_Repopulate type: %i - Init: %i Cur: %i", i, MultiLevelInv.Initial[pi], MultiLevelInv.Current[pi]);
			rt = 0;
			maybe_drop_net_powerup(pi, 0, 1);
		}
	}
}

}

#if defined(DXX_BUILD_DESCENT_II)
namespace {

///
/// CODE TO LOAD HOARD DATA
///

class hoard_resources_type
{
	static constexpr std::integral_constant<bitmap_index, bitmap_index{UINT16_MAX}> invalid_bm_idx{};
	static constexpr auto invalid_snd_idx = std::integral_constant<unsigned, ~0u>{};
public:
	bitmap_index bm_idx = invalid_bm_idx;
	unsigned snd_idx = invalid_snd_idx;
	void reset();
	~hoard_resources_type()
	{
		reset();
	}
};

static hoard_resources_type hoard_resources;

}

int HoardEquipped()
{
	static int checked=-1;

	if (unlikely(checked == -1))
	{
		checked = PHYSFSX_exists_ignorecase(hoard_ham_basename);
	}
	return (checked);
}

std::array<grs_main_bitmap, 2> Orb_icons;

void hoard_resources_type::reset()
{
	if (bm_idx != invalid_bm_idx)
		d_free(GameBitmaps[std::exchange(bm_idx, invalid_bm_idx)].bm_mdata);
	if (snd_idx != invalid_snd_idx)
	{
		const auto idx = std::exchange(snd_idx, invalid_snd_idx);
		range_for (auto &i, partial_range(GameSounds, idx, idx + 4))
			i.data.reset();
	}
	range_for (auto &i, Orb_icons)
		i.reset();
}

void init_hoard_data(d_vclip_array &Vclip)
{
	auto &Effects = LevelUniqueEffectsClipState.Effects;
	hoard_resources.reset();
	unsigned n_orb_frames,n_goal_frames;
	int orb_w,orb_h;
	palette_array_t palette;
	uint8_t *bitmap_data1;
	int save_pos;
	uint16_t bitmap_num = Num_bitmap_files;
	hoard_resources.bm_idx = bitmap_index{bitmap_num};
	auto &TmapInfo = LevelUniqueTmapInfoState.TmapInfo;

	auto &&[ifile, physfserr] = PHYSFSX_openReadBuffered(hoard_ham_basename);
	if (!ifile)
		Error("Failed to open <hoard.ham>: %s", PHYSFS_getErrorByCode(physfserr));

	n_orb_frames = PHYSFSX_readShort(ifile);
	orb_w = PHYSFSX_readShort(ifile);
	orb_h = PHYSFSX_readShort(ifile);
	save_pos = PHYSFS_tell(ifile);
	PHYSFSX_fseek(ifile,sizeof(palette)+n_orb_frames*orb_w*orb_h,SEEK_CUR);
	n_goal_frames = PHYSFSX_readShort(ifile);
	PHYSFS_seek(ifile, save_pos);

	//Allocate memory for bitmaps
	MALLOC( bitmap_data1, ubyte, n_orb_frames*orb_w*orb_h + n_goal_frames*64*64 );

	//Create orb vclip
	const auto nvc = Vclip.valid_index(Num_vclips);
	if (!nvc)
		throw std::runtime_error("too many vclips");
	++ Num_vclips;
	const auto orb_vclip{*nvc};
	auto &vcorb = Vclip[orb_vclip];
	vcorb.play_time = F1_0/2;
	vcorb.num_frames = n_orb_frames;
	vcorb.frame_time = vcorb.play_time / vcorb.num_frames;
	vcorb.flags = 0;
	vcorb.sound_num = -1;
	vcorb.light_value = F1_0;
	for (auto &i : partial_range(vcorb.frames, n_orb_frames))
	{
		const bitmap_index bi{bitmap_num};
		i = bi;
		gr_init_bitmap(GameBitmaps[bi], bm_mode::linear, 0, 0, orb_w, orb_h, orb_w, bitmap_data1);
		gr_set_transparent(GameBitmaps[bi], 1);
		bitmap_data1 += orb_w*orb_h;
		bitmap_num++;
		Assert(bitmap_num < MAX_BITMAP_FILES);
	}

	//Create obj powerup
	Powerup_info[powerup_type_t::POW_HOARD_ORB].vclip_num = orb_vclip;
	Powerup_info[powerup_type_t::POW_HOARD_ORB].hit_sound = -1;
	Powerup_info[powerup_type_t::POW_HOARD_ORB].size = Powerup_info[powerup_type_t::POW_SHIELD_BOOST].size;
	Powerup_info[powerup_type_t::POW_HOARD_ORB].light = Powerup_info[powerup_type_t::POW_SHIELD_BOOST].light;

	//Create orb goal wall effect
	const auto goal_eclip = Num_effects++;
	assert(goal_eclip < Effects.size());
	Effects[goal_eclip] = Effects[94];        //copy from blue goal
	Effects[goal_eclip].changing_wall_texture = NumTextures;
	Effects[goal_eclip].vc.num_frames=n_goal_frames;

	TmapInfo[NumTextures] = find_required_goal_texture(LevelUniqueTmapInfoState, tmapinfo_flag::goal_blue);
	TmapInfo[NumTextures].eclip_num = goal_eclip;
	TmapInfo[NumTextures].flags = static_cast<tmapinfo_flags>(tmapinfo_flag::goal_hoard);
	NumTextures++;
	Assert(NumTextures < MAX_TEXTURES);
	range_for (auto &i, partial_range(Effects[goal_eclip].vc.frames, n_goal_frames))
	{
		const bitmap_index bi{bitmap_num};
		i = bi;
		gr_init_bitmap(GameBitmaps[bi], bm_mode::linear, 0, 0, 64, 64, 64, bitmap_data1);
		bitmap_data1 += 64*64;
		bitmap_num++;
		Assert(bitmap_num < MAX_BITMAP_FILES);
	}

	//Load and remap bitmap data for orb
	PHYSFSX_readBytes(ifile, palette, sizeof(palette[0]) * std::size(palette));
	range_for (auto &i, partial_const_range(vcorb.frames, n_orb_frames))
	{
		grs_bitmap *const bm = &GameBitmaps[i];
		PHYSFSX_readBytes(ifile, bm->get_bitmap_data(), orb_w * orb_h);
		gr_remap_bitmap_good(*bm, palette, 255, -1);
	}

	//Load and remap bitmap data for goal texture
	PHYSFSX_skipBytes<2>(ifile);		//skip frame count
	PHYSFSX_readBytes(ifile, palette, sizeof(palette[0]) * std::size(palette));
	range_for (auto &i, partial_const_range(Effects[goal_eclip].vc.frames, n_goal_frames))
	{
		grs_bitmap *const bm = &GameBitmaps[i];
		PHYSFSX_readBytes(ifile, bm->get_bitmap_data(), 64 * 64);
		gr_remap_bitmap_good(*bm, palette, 255, -1);
	}

	//Load and remap bitmap data for HUD icons
	range_for (auto &i, Orb_icons)
	{
		const unsigned icon_w = PHYSFSX_readShort(ifile);
		if (icon_w > 32)
			return;
		const unsigned icon_h = PHYSFSX_readShort(ifile);
		if (icon_h > 32)
			return;
		const unsigned extent = icon_w * icon_h;
		RAIIdmem<uint8_t[]> bitmap_data2;
		MALLOC(bitmap_data2, uint8_t[], extent);
		PHYSFSX_readBytes(ifile, palette, sizeof(palette[0]) * std::size(palette));
		PHYSFSX_readBytes(ifile, bitmap_data2.get(), extent);
		gr_init_main_bitmap(i, bm_mode::linear, 0, 0, icon_w, icon_h, icon_w, std::move(bitmap_data2));
		gr_set_transparent(i, 1);
		gr_remap_bitmap_good(i, palette, 255, -1);
	}

	//Load sounds for orb game
	hoard_resources.snd_idx = Num_sound_files;
	const auto SndDigiSampleRate = GameArg.SndDigiSampleRate;
	range_for (const unsigned i, xrange(4u))
	{
		int len;

		len = PHYSFSX_readInt(ifile);        //get 11k len

		if (SndDigiSampleRate == sound_sample_rate::_22k)
		{
			PHYSFSX_fseek(ifile,len,SEEK_CUR);     //skip over 11k sample
			len = PHYSFSX_readInt(ifile);    //get 22k len
		}

		auto &gs = GameSounds[Num_sound_files+i];
		gs.length = len;
		gs.data = digi_sound::allocated_data{std::make_unique<uint8_t[]>(len), game_sound_offset{}};
		PHYSFSX_readBytes(ifile, gs.data.get(), len);

		if (SndDigiSampleRate == sound_sample_rate::_11k)
		{
			len = PHYSFSX_readInt(ifile);    //get 22k len
			PHYSFSX_fseek(ifile,len,SEEK_CUR);     //skip over 22k sample
		}

		Sounds[SOUND_YOU_GOT_ORB+i] = Num_sound_files+i;
		AltSounds[SOUND_YOU_GOT_ORB+i] = Sounds[SOUND_YOU_GOT_ORB+i];
	}
}

#if DXX_USE_EDITOR
void save_hoard_data(void)
{
	static constexpr char sounds[][13] = {"selforb.raw","selforb.r22",          //SOUND_YOU_GOT_ORB
				"teamorb.raw","teamorb.r22",    //SOUND_FRIEND_GOT_ORB
				"enemyorb.raw","enemyorb.r22",  //SOUND_OPPONENT_GOT_ORB
				"OPSCORE1.raw","OPSCORE1.r22"}; //SOUND_OPPONENT_HAS_SCORED
	auto ofile{PHYSFSX_openWriteBuffered(hoard_ham_basename).first};
	if (!ofile)
		return;

	{
		const auto read_result = iff_read_animbrush("orb.abm");
		auto &bm = read_result.bm;
		auto &palette = read_result.palette;
		const auto nframes = read_result.n_bitmaps;
		assert(read_result.status == iff_status_code::no_error);
	PHYSFS_writeULE16(ofile, nframes);
	PHYSFS_writeULE16(ofile, bm[0]->bm_w);
	PHYSFS_writeULE16(ofile, bm[0]->bm_h);
	PHYSFSX_writeBytes(ofile, palette.data(), sizeof(palette[0]) * palette.size());
	range_for (auto &i, partial_const_range(bm, nframes))
		PHYSFSX_writeBytes(ofile, i->bm_data, i->bm_w * i->bm_h);
	}

	{
		const auto read_result = iff_read_animbrush("orbgoal.abm");
		auto &bm = read_result.bm;
		auto &palette = read_result.palette;
		const auto nframes = read_result.n_bitmaps;
		assert(read_result.status == iff_status_code::no_error);
	Assert(bm[0]->bm_w == 64 && bm[0]->bm_h == 64);
	PHYSFS_writeULE16(ofile, nframes);
	PHYSFSX_writeBytes(ofile, palette.data(), sizeof(palette[0]) * palette.size());
	range_for (auto &i, partial_const_range(bm, nframes))
		PHYSFSX_writeBytes(ofile, i->bm_data, i->bm_w * i->bm_h);
	}

	range_for (const unsigned i, xrange(2u))
	{
		palette_array_t palette;
		grs_bitmap icon;
		const auto iff_error{iff_read_bitmap(i ? "orbb.bbm" : "orb.bbm", icon, &palette)};
		(void)iff_error;
		assert(iff_error == iff_status_code::no_error);
		PHYSFS_writeULE16(ofile, icon.bm_w);
		PHYSFS_writeULE16(ofile, icon.bm_h);
		PHYSFSX_writeBytes(ofile, palette.data(), sizeof(palette[0]) * palette.size());
		PHYSFSX_writeBytes(ofile, icon.bm_data, icon.bm_w * icon.bm_h);
	}
	range_for (auto &i, sounds)
		if (RAIIPHYSFS_File ifile{PHYSFS_openRead(i)})
	{
		int size;
		size = PHYSFS_fileLength(ifile);
		const auto buf = std::make_unique<uint8_t[]>(size);
		PHYSFSX_readBytes(ifile, buf, size);
		PHYSFS_writeULE32(ofile, size);
		PHYSFSX_writeBytes(ofile, buf, size);
	}
}
#endif
#endif

namespace {

static void multi_process_data(const d_level_shared_robot_info_state &LevelSharedRobotInfoState, const playernum_t pnum, const std::span<const uint8_t> data, const multiplayer_command_t type)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &imobjptridx = Objects.imptridx;
	auto &vmobjptr = Objects.vmptr;
	auto &vmobjptridx = Objects.vmptridx;
	// Take an entire message (that has already been checked for validity,
	// if necessary) and act on it.
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vmwallptr = Walls.vmptr;
	switch (type)
	{
		case multiplayer_command_t::MULTI_POSITION:
			multi_do_position(vmobjptridx, pnum, multi_subspan_first<multiplayer_command_t::MULTI_POSITION>(data));
			break;
		case multiplayer_command_t::MULTI_REAPPEAR:
			multi_do_reappear(pnum, multi_subspan_first<multiplayer_command_t::MULTI_REAPPEAR>(data));
			break;
		case multiplayer_command_t::MULTI_FIRE:
		case multiplayer_command_t::MULTI_FIRE_TRACK:
		case multiplayer_command_t::MULTI_FIRE_BOMB:
			multi_do_fire(vmobjptridx, pnum, multi_subspan_first<multiplayer_command_t::MULTI_FIRE>(data),
							type == multiplayer_command_t::MULTI_FIRE_TRACK
							? objnum_remote_to_local(GET_INTEL_SHORT(&data[17]), data[19])
							: object_none,
							type == multiplayer_command_t::MULTI_FIRE_BOMB
							? std::optional(GET_INTEL_SHORT(&data[17]))
							: std::nullopt);
			break;
		case multiplayer_command_t::MULTI_REMOVE_OBJECT:
			multi_do_remobj(vmobjptr, multi_subspan_first<multiplayer_command_t::MULTI_REMOVE_OBJECT>(data));
			break;
		case multiplayer_command_t::MULTI_PLAYER_DERES:
			multi_do_player_deres(LevelSharedRobotInfoState.Robot_info, Objects, pnum, multi_subspan_first<multiplayer_command_t::MULTI_PLAYER_DERES>(data));
			break;
		case multiplayer_command_t::MULTI_MESSAGE:
			multi_do_message(pnum, multi_subspan_first<multiplayer_command_t::MULTI_MESSAGE>(data));
			break;
		case multiplayer_command_t::MULTI_QUIT:
			multi_do_quit(pnum);
			break;
		case multiplayer_command_t::MULTI_CONTROLCEN:
			multi_do_controlcen_destroy(LevelSharedRobotInfoState.Robot_info, imobjptridx, multi_subspan_first<multiplayer_command_t::MULTI_CONTROLCEN>(data));
			break;
		case multiplayer_command_t::MULTI_DROP_WEAPON:
			multi_do_drop_weapon(vmobjptr, pnum, multi_subspan_first<multiplayer_command_t::MULTI_DROP_WEAPON>(data));
			break;
		case multiplayer_command_t::MULTI_VULWPN_AMMO_ADJ:
			multi_do_vulcan_weapon_ammo_adjust(vmobjptr, multi_subspan_first<multiplayer_command_t::MULTI_VULWPN_AMMO_ADJ>(data));
			break;
#if defined(DXX_BUILD_DESCENT_II)
		case multiplayer_command_t::MULTI_SOUND_FUNCTION:
			multi_do_sound_function(pnum, multi_subspan_first<multiplayer_command_t::MULTI_SOUND_FUNCTION>(data));
			break;
		case multiplayer_command_t::MULTI_MARKER:
			multi_do_drop_marker(Objects, vmsegptridx, pnum, multi_subspan_first<multiplayer_command_t::MULTI_MARKER>(data));
			break;
		case multiplayer_command_t::MULTI_DROP_FLAG:
			multi_do_drop_flag(pnum, multi_subspan_first<multiplayer_command_t::MULTI_DROP_FLAG>(data));
			break;
		case multiplayer_command_t::MULTI_GUIDED:
			multi_do_guided(LevelUniqueObjectState, pnum, multi_subspan_first<multiplayer_command_t::MULTI_GUIDED>(data));
			break;
		case multiplayer_command_t::MULTI_STOLEN_ITEMS:
			multi_do_stolen_items(LevelUniqueObjectState.ThiefState, multi_subspan_first<multiplayer_command_t::MULTI_STOLEN_ITEMS>(data));
			break;
		case multiplayer_command_t::MULTI_WALL_STATUS:
			multi_do_wall_status(vmwallptr, multi_subspan_first<multiplayer_command_t::MULTI_WALL_STATUS>(data));
			break;
		case multiplayer_command_t::MULTI_SEISMIC:
			multi_do_seismic(multi_subspan_first<multiplayer_command_t::MULTI_SEISMIC>(data));
			break;
		case multiplayer_command_t::MULTI_LIGHT:
			multi_do_light(multi_subspan_first<multiplayer_command_t::MULTI_LIGHT>(data));
			break;
#endif
		case multiplayer_command_t::MULTI_ENDLEVEL_START:
			multi_do_escape(vmobjptridx, pnum, multi_subspan_first<multiplayer_command_t::MULTI_ENDLEVEL_START>(data));
			break;
		case multiplayer_command_t::MULTI_CLOAK:
			multi_do_cloak(vmobjptr, pnum);
			break;
		case multiplayer_command_t::MULTI_DECLOAK:
			multi_do_decloak(pnum); break;
		case multiplayer_command_t::MULTI_DOOR_OPEN:
			multi_do_door_open(vmwallptr, multi_subspan_first<multiplayer_command_t::MULTI_DOOR_OPEN>(data));
			break;
		case multiplayer_command_t::MULTI_CREATE_EXPLOSION:
			multi_do_create_explosion(vmobjptridx, pnum);
			break;
		case multiplayer_command_t::MULTI_CONTROLCEN_FIRE:
			multi_do_controlcen_fire(multi_subspan_first<multiplayer_command_t::MULTI_CONTROLCEN_FIRE>(data));
			break;
		case multiplayer_command_t::MULTI_CREATE_POWERUP:
			multi_do_create_powerup(vmsegptridx, pnum, multi_subspan_first<multiplayer_command_t::MULTI_CREATE_POWERUP>(data));
			break;
		case multiplayer_command_t::MULTI_PLAY_SOUND:
			multi_do_play_sound(Objects, pnum, multi_subspan_first<multiplayer_command_t::MULTI_PLAY_SOUND>(data));
			break;
#if defined(DXX_BUILD_DESCENT_II)
		case multiplayer_command_t::MULTI_CAPTURE_BONUS:
			multi_do_capture_bonus(pnum); break;
		case multiplayer_command_t::MULTI_ORB_BONUS:
			multi_do_orb_bonus(pnum, multi_subspan_first<multiplayer_command_t::MULTI_ORB_BONUS>(data));
			break;
		case multiplayer_command_t::MULTI_GOT_FLAG:
			multi_do_got_flag(pnum); break;
		case multiplayer_command_t::MULTI_GOT_ORB:
			multi_do_got_orb(pnum); break;
		case multiplayer_command_t::MULTI_FINISH_GAME:
			multi_do_finish_game();
			break;  // do this one regardless of endsequence
#endif
		case multiplayer_command_t::MULTI_RANK:
			multi_do_ranking (pnum, multi_subspan_first<multiplayer_command_t::MULTI_RANK>(data));
			break;
		case multiplayer_command_t::MULTI_ROBOT_CLAIM:
			multi_do_claim_robot(pnum, multi_subspan_first<multiplayer_command_t::MULTI_ROBOT_CLAIM>(data));
			break;
		case multiplayer_command_t::MULTI_ROBOT_POSITION:
			multi_do_robot_position(pnum, multi_subspan_first<multiplayer_command_t::MULTI_ROBOT_POSITION>(data));
			break;
		case multiplayer_command_t::MULTI_ROBOT_EXPLODE:
			multi_do_robot_explode(LevelSharedRobotInfoState.Robot_info, multi_subspan_first<multiplayer_command_t::MULTI_ROBOT_EXPLODE>(data));
			break;
		case multiplayer_command_t::MULTI_ROBOT_RELEASE:
			multi_do_release_robot(pnum, multi_subspan_first<multiplayer_command_t::MULTI_ROBOT_RELEASE>(data));
			break;
		case multiplayer_command_t::MULTI_ROBOT_FIRE:
			multi_do_robot_fire(multi_subspan_first<multiplayer_command_t::MULTI_ROBOT_FIRE>(data));
			break;
		case multiplayer_command_t::MULTI_SCORE:
			multi_do_score(vmobjptr, pnum, multi_subspan_first<multiplayer_command_t::MULTI_SCORE>(data));
			break;
		case multiplayer_command_t::MULTI_CREATE_ROBOT:
			multi_do_create_robot(LevelSharedRobotInfoState.Robot_info, Vclip, pnum, multi_subspan_first<multiplayer_command_t::MULTI_CREATE_ROBOT>(data));
			break;
		case multiplayer_command_t::MULTI_TRIGGER:
			multi_do_trigger(pnum, multi_subspan_first<multiplayer_command_t::MULTI_TRIGGER>(data));
			break;
#if defined(DXX_BUILD_DESCENT_II)
		case multiplayer_command_t::MULTI_START_TRIGGER:
			multi_do_start_trigger(multi_subspan_first<multiplayer_command_t::MULTI_START_TRIGGER>(data));
			break;
		case multiplayer_command_t::MULTI_EFFECT_BLOWUP:
			multi_do_effect_blowup(pnum, multi_subspan_first<multiplayer_command_t::MULTI_EFFECT_BLOWUP>(data));
			break;
		case multiplayer_command_t::MULTI_FLAGS:
			multi_do_flags(vmobjptr, pnum, multi_subspan_first<multiplayer_command_t::MULTI_FLAGS>(data));
			break;
		case multiplayer_command_t::MULTI_DROP_BLOB:
			multi_do_drop_blob(vmobjptr, pnum);
			break;
		case multiplayer_command_t::MULTI_UPDATE_BUDDY_STATE:
			multi_recv_escort_goal(LevelUniqueObjectState.BuddyState, multi_subspan_first<multiplayer_command_t::MULTI_UPDATE_BUDDY_STATE>(data));
			break;
#endif
		case multiplayer_command_t::MULTI_BOSS_TELEPORT:
			multi_do_boss_teleport(LevelSharedRobotInfoState.Robot_info, Vclip, pnum, multi_subspan_first<multiplayer_command_t::MULTI_BOSS_TELEPORT>(data));
			break;
		case multiplayer_command_t::MULTI_BOSS_CLOAK:
			multi_do_boss_cloak(multi_subspan_first<multiplayer_command_t::MULTI_BOSS_CLOAK>(data));
			break;
		case multiplayer_command_t::MULTI_BOSS_START_GATE:
			multi_do_boss_start_gate(multi_subspan_first<multiplayer_command_t::MULTI_BOSS_START_GATE>(data));
			break;
		case multiplayer_command_t::MULTI_BOSS_STOP_GATE:
			multi_do_boss_stop_gate(multi_subspan_first<multiplayer_command_t::MULTI_BOSS_STOP_GATE>(data));
			break;
		case multiplayer_command_t::MULTI_BOSS_CREATE_ROBOT:
			multi_do_boss_create_robot(pnum, multi_subspan_first<multiplayer_command_t::MULTI_BOSS_CREATE_ROBOT>(data));
			break;
		case multiplayer_command_t::MULTI_CREATE_ROBOT_POWERUPS:
			multi_do_create_robot_powerups(pnum, multi_subspan_first<multiplayer_command_t::MULTI_CREATE_ROBOT_POWERUPS>(data));
			break;
		case multiplayer_command_t::MULTI_HOSTAGE_DOOR:
			multi_do_hostage_door_status(vmsegptridx, Walls, multi_subspan_first<multiplayer_command_t::MULTI_HOSTAGE_DOOR>(data));
			break;
		case multiplayer_command_t::MULTI_SAVE_GAME:
			multi_do_save_game(multi_subspan_first<multiplayer_command_t::MULTI_SAVE_GAME>(data));
			break;
		case multiplayer_command_t::MULTI_RESTORE_GAME:
			multi_do_restore_game(multi_subspan_first<multiplayer_command_t::MULTI_RESTORE_GAME>(data));
			break;
		case multiplayer_command_t::MULTI_HEARTBEAT:
			multi_do_heartbeat(multi_subspan_first<multiplayer_command_t::MULTI_HEARTBEAT>(data));
			break;
		case multiplayer_command_t::MULTI_KILLGOALS:
			multi_do_kill_goal_counts(vmobjptr, multi_subspan_first<multiplayer_command_t::MULTI_KILLGOALS>(data));
			break;
		case multiplayer_command_t::MULTI_DO_BOUNTY:
			multi_do_bounty(multi_subspan_first<multiplayer_command_t::MULTI_DO_BOUNTY>(data));
			break;
		case multiplayer_command_t::MULTI_TYPING_STATE:
			multi_do_msgsend_state(pnum, multi_subspan_first<multiplayer_command_t::MULTI_TYPING_STATE>(data));
			break;
		case multiplayer_command_t::MULTI_GMODE_UPDATE:
			multi_do_gmode_update(multi_subspan_first<multiplayer_command_t::MULTI_GMODE_UPDATE>(data));
			break;
		case multiplayer_command_t::MULTI_KILL_HOST:
			multi_do_kill_host(Objects, pnum, multi_subspan_first<multiplayer_command_t::MULTI_KILL_HOST>(data));
			break;
		case multiplayer_command_t::MULTI_KILL_CLIENT:
			multi_do_kill_client(Objects, pnum, multi_subspan_first<multiplayer_command_t::MULTI_KILL_CLIENT>(data));
			break;
		case multiplayer_command_t::MULTI_PLAYER_INV:
			multi_do_player_inventory(pnum, multi_subspan_first<multiplayer_command_t::MULTI_PLAYER_INV>(data));
			break;
	}
}

}

// Following functions convert object to object_rw and back.
// turn object to object_rw for sending
void multi_object_to_object_rw(const object &obj, object_rw *obj_rw)
{
	/* Avoid leaking any uninitialized bytes onto the network.  Some of
	 * the unions are incompletely initialized for some branches.
	 *
	 * For poison enabled builds, poison any uninitialized fields.
	 * Everything that the peer should read will be initialized by the
	 * end of this function.
	 */
	*obj_rw = {};
	DXX_POISON_DEFINED_VAR(*obj_rw, 0xfd);
	obj_rw->signature     = INTEL_SHORT(static_cast<uint16_t>(obj.signature));
	obj_rw->type          = obj.type;
	obj_rw->id            = obj.id;
	/* next, prev not maintained here */
	obj_rw->control_source = underlying_value(obj.control_source);
	obj_rw->movement_source = underlying_value(obj.movement_source);
	const auto rtype = obj.render_type;
	obj_rw->render_type   = underlying_value(rtype);
	obj_rw->flags         = obj.flags;
	obj_rw->segnum        = INTEL_SHORT(obj.segnum);
	/* attached_obj not maintained here */
	{
		const auto pos = build_little_endian_vector_from_native_endian(obj.pos);
		obj_rw->pos = pos;
		obj_rw->last_pos = pos;
	}
	obj_rw->orient = build_little_endian_matrix_from_native_endian(obj.orient);
	obj_rw->size          = INTEL_INT(obj.size);
	obj_rw->shields       = INTEL_INT(obj.shields);
	obj_rw->contains_type = underlying_value(obj.contains.type);
	obj_rw->contains_id   = underlying_value(obj.contains.id.robot);
	obj_rw->contains_count= obj.contains.count;
	obj_rw->matcen_creator= obj.matcen_creator;
	obj_rw->lifeleft      = INTEL_INT(obj.lifeleft);

	switch (typename object::movement_type{obj_rw->movement_source})
	{
		case object::movement_type::None:
			obj_rw->mtype = {};
			break;
		case object::movement_type::physics:
			obj_rw->mtype.phys_info.velocity  = build_little_endian_vector_from_native_endian(obj.mtype.phys_info.velocity);
			obj_rw->mtype.phys_info.thrust    = build_little_endian_vector_from_native_endian(obj.mtype.phys_info.thrust);
			obj_rw->mtype.phys_info.mass        = INTEL_INT(obj.mtype.phys_info.mass);
			obj_rw->mtype.phys_info.drag        = INTEL_INT(obj.mtype.phys_info.drag);
			obj_rw->mtype.phys_info.rotvel    = build_little_endian_vector_from_native_endian(obj.mtype.phys_info.rotvel);
			obj_rw->mtype.phys_info.rotthrust = build_little_endian_vector_from_native_endian(obj.mtype.phys_info.rotthrust);
			obj_rw->mtype.phys_info.turnroll    = INTEL_INT(obj.mtype.phys_info.turnroll);
			obj_rw->mtype.phys_info.flags       = INTEL_SHORT(obj.mtype.phys_info.flags);
			break;
			
		case object::movement_type::spinning:
			obj_rw->mtype.spin_rate = build_little_endian_vector_from_native_endian(obj.mtype.spin_rate);
			break;
	}

	switch (typename object::control_type{obj_rw->control_source})
	{
		case object::control_type::weapon:
			obj_rw->ctype.laser_info.parent_type      = INTEL_SHORT(obj.ctype.laser_info.parent_type);
			obj_rw->ctype.laser_info.parent_num       = INTEL_SHORT(obj.ctype.laser_info.parent_num);
			obj_rw->ctype.laser_info.parent_signature = INTEL_INT(static_cast<uint16_t>(obj.ctype.laser_info.parent_signature));
			if (const auto c = obj.ctype.laser_info.creation_time - GameTime64; c < F1_0*(-18000))
				obj_rw->ctype.laser_info.creation_time = INTEL_INT(F1_0*(-18000));
			else
				obj_rw->ctype.laser_info.creation_time = INTEL_INT(static_cast<int>(c));
			obj_rw->ctype.laser_info.last_hitobj      = INTEL_SHORT(obj.ctype.laser_info.get_last_hitobj());
			obj_rw->ctype.laser_info.track_goal       = INTEL_SHORT(obj.ctype.laser_info.track_goal);
			obj_rw->ctype.laser_info.multiplier       = INTEL_INT(obj.ctype.laser_info.multiplier);
			break;
			
		case object::control_type::explosion:
			obj_rw->ctype.expl_info.spawn_time    = INTEL_INT(obj.ctype.expl_info.spawn_time);
			obj_rw->ctype.expl_info.delete_time   = INTEL_INT(obj.ctype.expl_info.delete_time);
			obj_rw->ctype.expl_info.delete_objnum = INTEL_SHORT(obj.ctype.expl_info.delete_objnum);
			obj_rw->ctype.expl_info.attach_parent = INTEL_SHORT(obj.ctype.expl_info.attach_parent);
			obj_rw->ctype.expl_info.prev_attach   = INTEL_SHORT(obj.ctype.expl_info.prev_attach);
			obj_rw->ctype.expl_info.next_attach   = INTEL_SHORT(obj.ctype.expl_info.next_attach);
			break;
			
		case object::control_type::ai:
		{
			obj_rw->ctype.ai_info.behavior               = static_cast<uint8_t>(obj.ctype.ai_info.behavior);
			obj_rw->ctype.ai_info.flags[0] = underlying_value(obj.ctype.ai_info.CURRENT_GUN);
			obj_rw->ctype.ai_info.flags[1] = obj.ctype.ai_info.CURRENT_STATE;
			obj_rw->ctype.ai_info.flags[2] = obj.ctype.ai_info.GOAL_STATE;
			obj_rw->ctype.ai_info.flags[3] = obj.ctype.ai_info.PATH_DIR;
#if defined(DXX_BUILD_DESCENT_I)
			obj_rw->ctype.ai_info.flags[4] = obj.ctype.ai_info.SUBMODE;
#elif defined(DXX_BUILD_DESCENT_II)
			obj_rw->ctype.ai_info.flags[4] = obj.ctype.ai_info.SUB_FLAGS;
#endif
			obj_rw->ctype.ai_info.flags[5] = underlying_value(obj.ctype.ai_info.GOALSIDE);
			obj_rw->ctype.ai_info.flags[6] = obj.ctype.ai_info.CLOAKED;
			obj_rw->ctype.ai_info.flags[7] = obj.ctype.ai_info.SKIP_AI_COUNT;
			obj_rw->ctype.ai_info.flags[8] = obj.ctype.ai_info.REMOTE_OWNER;
			obj_rw->ctype.ai_info.flags[9] = obj.ctype.ai_info.REMOTE_SLOT_NUM;
			obj_rw->ctype.ai_info.hide_segment           = INTEL_SHORT(obj.ctype.ai_info.hide_segment);
			obj_rw->ctype.ai_info.hide_index             = INTEL_SHORT(obj.ctype.ai_info.hide_index);
			obj_rw->ctype.ai_info.path_length            = INTEL_SHORT(obj.ctype.ai_info.path_length);
#if defined(DXX_BUILD_DESCENT_I)
			obj_rw->ctype.ai_info.cur_path_index         = INTEL_SHORT(obj.ctype.ai_info.cur_path_index);
#elif defined(DXX_BUILD_DESCENT_II)
			obj_rw->ctype.ai_info.cur_path_index         = obj.ctype.ai_info.cur_path_index;
#endif
			obj_rw->ctype.ai_info.danger_laser_num       = INTEL_SHORT(obj.ctype.ai_info.danger_laser_num);
			if (obj.ctype.ai_info.danger_laser_num != object_none)
				obj_rw->ctype.ai_info.danger_laser_signature = INTEL_INT(static_cast<uint16_t>(obj.ctype.ai_info.danger_laser_signature));
			else
				obj_rw->ctype.ai_info.danger_laser_signature = 0;
#if defined(DXX_BUILD_DESCENT_I)
			obj_rw->ctype.ai_info.follow_path_start_seg  = segment_none;
			obj_rw->ctype.ai_info.follow_path_end_seg    = segment_none;
#elif defined(DXX_BUILD_DESCENT_II)
			obj_rw->ctype.ai_info.dying_sound_playing    = obj.ctype.ai_info.dying_sound_playing;
			if (obj.ctype.ai_info.dying_start_time == 0) // if bot not dead, anything but 0 will kill it
				obj_rw->ctype.ai_info.dying_start_time = 0;
			else
				obj_rw->ctype.ai_info.dying_start_time = INTEL_INT(static_cast<int>(obj.ctype.ai_info.dying_start_time - GameTime64));
#endif
			break;
		}
			
		case object::control_type::light:
			obj_rw->ctype.light_info.intensity = INTEL_INT(obj.ctype.light_info.intensity);
			break;
			
		case object::control_type::powerup:
			obj_rw->ctype.powerup_info.count         = INTEL_INT(obj.ctype.powerup_info.count);
#if defined(DXX_BUILD_DESCENT_II)
			if (const auto c = obj.ctype.powerup_info.creation_time - GameTime64; c < F1_0*(-18000))
				obj_rw->ctype.powerup_info.creation_time = INTEL_INT(F1_0*(-18000));
			else
				obj_rw->ctype.powerup_info.creation_time = INTEL_INT(static_cast<int>(c));
			obj_rw->ctype.powerup_info.flags         = INTEL_INT(obj.ctype.powerup_info.flags);
#endif
			break;
		case object::control_type::None:
		case object::control_type::flying:
		case object::control_type::slew:
		case object::control_type::flythrough:
		case object::control_type::repaircen:
		case object::control_type::morph:
		case object::control_type::debris:
		case object::control_type::remote:
		default:
			break;
	}

	switch (rtype)
	{
		case render_type::RT_NONE:
			if (obj.type != OBJ_GHOST) // HACK: when a player is dead or not connected yet, clients still expect to get polyobj data - even if render_type == RT_NONE at this time.
				break;
			[[fallthrough]];
		case render_type::RT_MORPH:
		case render_type::RT_POLYOBJ:
		{
			int i;
			obj_rw->rtype.pobj_info.model_num = INTEL_INT(underlying_value(obj.rtype.pobj_info.model_num));
			for (i=0;i<MAX_SUBMODELS;i++)
				obj_rw->rtype.pobj_info.anim_angles[i] = build_little_endian_angvec_from_native_endian(obj.rtype.pobj_info.anim_angles[i]);
			obj_rw->rtype.pobj_info.subobj_flags             = INTEL_INT(obj.rtype.pobj_info.subobj_flags);
			obj_rw->rtype.pobj_info.tmap_override            = INTEL_INT(obj.rtype.pobj_info.tmap_override);
			obj_rw->rtype.pobj_info.alt_textures             = INTEL_INT(obj.rtype.pobj_info.alt_textures);
			break;
		}
			
		case render_type::RT_WEAPON_VCLIP:
		case render_type::RT_HOSTAGE:
		case render_type::RT_POWERUP:
		case render_type::RT_FIREBALL:
			obj_rw->rtype.vclip_info.vclip_num = INTEL_INT(underlying_value(obj.rtype.vclip_info.vclip_num));
			obj_rw->rtype.vclip_info.frametime = INTEL_INT(obj.rtype.vclip_info.frametime);
			obj_rw->rtype.vclip_info.framenum  = obj.rtype.vclip_info.framenum;
			break;
			
		case render_type::RT_LASER:
			break;
			
	}
}

// turn object_rw to object after receiving
void multi_object_rw_to_object(const object_rw *const obj_rw, object &obj)
{
	obj = {};
	DXX_POISON_VAR(obj, 0xfd);
	set_object_type(obj, obj_rw->type);
	if (obj.type == OBJ_NONE)
		return;
	obj.signature     = object_signature_t{static_cast<uint16_t>(INTEL_INT(obj_rw->signature))};
	obj.id            = obj_rw->id;
	/* obj->next,obj->prev handled by caller based on segment */
	obj.control_source  = typename object::control_type{obj_rw->control_source};
	obj.movement_source = typename object::movement_type{obj_rw->movement_source};
	if (const auto rtype = obj_rw->render_type; valid_render_type(rtype))
		obj.render_type = render_type{rtype};
	else
	{
		con_printf(CON_URGENT, "peer sent bogus render type %#x for object %p; using none instead", rtype, &obj);
		obj.render_type = render_type::RT_NONE;
	}
	obj.flags         = obj_rw->flags;
	obj.segnum = vmsegidx_t::check_nothrow_index(INTEL_SHORT(obj_rw->segnum)).value_or(segment_none);
	obj.attached_obj = object_none;
	obj.pos         = build_native_endian_vector_from_little_endian(obj_rw->pos);
	obj.orient = build_native_endian_matrix_from_little_endian(obj_rw->orient);
	obj.size          = INTEL_INT(obj_rw->size);
	obj.shields       = INTEL_INT(obj_rw->shields);
	obj.contains = build_contained_object_parameters_from_untrusted(obj_rw->contains_type, obj_rw->contains_id, obj_rw->contains_count);
	obj.matcen_creator= obj_rw->matcen_creator;
	obj.lifeleft      = INTEL_INT(obj_rw->lifeleft);
	
	switch (obj.movement_source)
	{
		case object::movement_type::None:
			break;
		case object::movement_type::physics:
			obj.mtype.phys_info.velocity  = build_native_endian_vector_from_little_endian(obj_rw->mtype.phys_info.velocity);
			obj.mtype.phys_info.thrust  = build_native_endian_vector_from_little_endian(obj_rw->mtype.phys_info.thrust);
			obj.mtype.phys_info.mass        = INTEL_INT(obj_rw->mtype.phys_info.mass);
			obj.mtype.phys_info.drag        = INTEL_INT(obj_rw->mtype.phys_info.drag);
			obj.mtype.phys_info.rotvel  = build_native_endian_vector_from_little_endian(obj_rw->mtype.phys_info.rotvel);
			obj.mtype.phys_info.rotthrust  = build_native_endian_vector_from_little_endian(obj_rw->mtype.phys_info.rotthrust);
			obj.mtype.phys_info.turnroll    = INTEL_INT(obj_rw->mtype.phys_info.turnroll);
			obj.mtype.phys_info.flags       = INTEL_INT(obj_rw->mtype.phys_info.flags);
			break;
			
		case object::movement_type::spinning:
			obj.mtype.spin_rate = build_native_endian_vector_from_little_endian(obj_rw->mtype.spin_rate);
			break;
	}
	
	switch (obj.control_source)
	{
		case object::control_type::weapon:
			obj.ctype.laser_info.parent_type      = INTEL_SHORT(obj_rw->ctype.laser_info.parent_type);
			obj.ctype.laser_info.parent_num       = INTEL_SHORT(obj_rw->ctype.laser_info.parent_num);
			obj.ctype.laser_info.parent_signature = object_signature_t{static_cast<uint16_t>(INTEL_INT(obj_rw->ctype.laser_info.parent_signature))};
			obj.ctype.laser_info.creation_time    = INTEL_INT(obj_rw->ctype.laser_info.creation_time);
			/* `last_hitobj` is untrusted network data, so it must be checked before use. */
			if (const auto last_hitobj{vcobjidx_t::check_nothrow_index(INTEL_SHORT(obj_rw->ctype.laser_info.last_hitobj))})
				obj.ctype.laser_info.reset_hitobj(*last_hitobj);
			else
				obj.ctype.laser_info.clear_hitobj();
			obj.ctype.laser_info.track_goal       = INTEL_SHORT(obj_rw->ctype.laser_info.track_goal);
			obj.ctype.laser_info.multiplier       = INTEL_INT(obj_rw->ctype.laser_info.multiplier);
#if defined(DXX_BUILD_DESCENT_II)
			obj.ctype.laser_info.last_afterburner_time = 0;
#endif
			break;
			
		case object::control_type::explosion:
			obj.ctype.expl_info.spawn_time    = INTEL_INT(obj_rw->ctype.expl_info.spawn_time);
			obj.ctype.expl_info.delete_time   = INTEL_INT(obj_rw->ctype.expl_info.delete_time);
			obj.ctype.expl_info.delete_objnum = INTEL_SHORT(obj_rw->ctype.expl_info.delete_objnum);
			obj.ctype.expl_info.attach_parent = INTEL_SHORT(obj_rw->ctype.expl_info.attach_parent);
			obj.ctype.expl_info.prev_attach   = INTEL_SHORT(obj_rw->ctype.expl_info.prev_attach);
			obj.ctype.expl_info.next_attach   = INTEL_SHORT(obj_rw->ctype.expl_info.next_attach);
			break;
			
		case object::control_type::ai:
		{
			obj.ctype.ai_info.behavior               = static_cast<ai_behavior>(obj_rw->ctype.ai_info.behavior);
			{
				const uint8_t gun_num = obj_rw->ctype.ai_info.flags[0];
				obj.ctype.ai_info.CURRENT_GUN = (gun_num < MAX_GUNS) ? robot_gun_number{gun_num} : robot_gun_number{};
			}
			obj.ctype.ai_info.CURRENT_STATE = build_ai_state_from_untrusted(obj_rw->ctype.ai_info.flags[1]).value();
			obj.ctype.ai_info.GOAL_STATE = build_ai_state_from_untrusted(obj_rw->ctype.ai_info.flags[2]).value();
			obj.ctype.ai_info.PATH_DIR = obj_rw->ctype.ai_info.flags[3];
#if defined(DXX_BUILD_DESCENT_I)
			obj.ctype.ai_info.SUBMODE = obj_rw->ctype.ai_info.flags[4];
#elif defined(DXX_BUILD_DESCENT_II)
			obj.ctype.ai_info.SUB_FLAGS = obj_rw->ctype.ai_info.flags[4];
#endif
			obj.ctype.ai_info.GOALSIDE = build_sidenum_from_untrusted(obj_rw->ctype.ai_info.flags[5]).value();
			obj.ctype.ai_info.CLOAKED = obj_rw->ctype.ai_info.flags[6];
			obj.ctype.ai_info.SKIP_AI_COUNT = obj_rw->ctype.ai_info.flags[7];
			obj.ctype.ai_info.REMOTE_OWNER = obj_rw->ctype.ai_info.flags[8];
			obj.ctype.ai_info.REMOTE_SLOT_NUM = obj_rw->ctype.ai_info.flags[9];
			obj.ctype.ai_info.hide_segment = vmsegidx_t::check_nothrow_index(INTEL_SHORT(obj_rw->ctype.ai_info.hide_segment)).value_or(segment_none);
			obj.ctype.ai_info.hide_index             = INTEL_SHORT(obj_rw->ctype.ai_info.hide_index);
			obj.ctype.ai_info.path_length            = INTEL_SHORT(obj_rw->ctype.ai_info.path_length);
			obj.ctype.ai_info.cur_path_index         =
#if defined(DXX_BUILD_DESCENT_I)
				INTEL_SHORT(obj_rw->ctype.ai_info.cur_path_index);
#elif defined(DXX_BUILD_DESCENT_II)
				obj_rw->ctype.ai_info.cur_path_index;
#endif
			obj.ctype.ai_info.danger_laser_num       = INTEL_SHORT(obj_rw->ctype.ai_info.danger_laser_num);
			if (obj.ctype.ai_info.danger_laser_num != object_none)
				obj.ctype.ai_info.danger_laser_signature = object_signature_t{static_cast<uint16_t>(INTEL_INT(obj_rw->ctype.ai_info.danger_laser_signature))};
#if defined(DXX_BUILD_DESCENT_I)
#elif defined(DXX_BUILD_DESCENT_II)
			obj.ctype.ai_info.dying_sound_playing    = obj_rw->ctype.ai_info.dying_sound_playing;
			obj.ctype.ai_info.dying_start_time       = INTEL_INT(obj_rw->ctype.ai_info.dying_start_time);
#endif
			break;
		}
			
		case object::control_type::light:
			obj.ctype.light_info.intensity = INTEL_INT(obj_rw->ctype.light_info.intensity);
			break;
			
		case object::control_type::powerup:
			obj.ctype.powerup_info.count         = INTEL_INT(obj_rw->ctype.powerup_info.count);
#if defined(DXX_BUILD_DESCENT_I)
			obj.ctype.powerup_info.creation_time = 0;
			obj.ctype.powerup_info.flags         = 0;
#elif defined(DXX_BUILD_DESCENT_II)
			obj.ctype.powerup_info.creation_time = INTEL_INT(obj_rw->ctype.powerup_info.creation_time);
			obj.ctype.powerup_info.flags         = INTEL_INT(obj_rw->ctype.powerup_info.flags);
#endif
			break;
		case object::control_type::cntrlcen:
		{
			// gun points of reactor now part of the object but of course not saved in object_rw. Let's just recompute them.
			calc_controlcen_gun_point(obj);
			break;
		}
		case object::control_type::None:
		case object::control_type::flying:
		case object::control_type::slew:
		case object::control_type::flythrough:
		case object::control_type::repaircen:
		case object::control_type::morph:
		case object::control_type::debris:
		case object::control_type::remote:
		default:
			break;
	}
	
	switch (obj.render_type)
	{
		case render_type::RT_NONE:
			if (obj.type != OBJ_GHOST) // HACK: when a player is dead or not connected yet, clients still expect to get polyobj data - even if render_type == RT_NONE at this time.
				break;
			[[fallthrough]];
		case render_type::RT_MORPH:
		case render_type::RT_POLYOBJ:
		{
			int i;
			obj.rtype.pobj_info.model_num                = build_polygon_model_index_from_untrusted(INTEL_INT(obj_rw->rtype.pobj_info.model_num));
			for (i=0;i<MAX_SUBMODELS;i++)
			{
				obj.rtype.pobj_info.anim_angles[i] = build_native_endian_angvec_from_little_endian(obj_rw->rtype.pobj_info.anim_angles[i]);
			}
			obj.rtype.pobj_info.subobj_flags             = INTEL_INT(obj_rw->rtype.pobj_info.subobj_flags);
			obj.rtype.pobj_info.tmap_override            = INTEL_INT(obj_rw->rtype.pobj_info.tmap_override);
			obj.rtype.pobj_info.alt_textures             = INTEL_INT(obj_rw->rtype.pobj_info.alt_textures);
			break;
		}
			
		case render_type::RT_WEAPON_VCLIP:
		case render_type::RT_HOSTAGE:
		case render_type::RT_POWERUP:
		case render_type::RT_FIREBALL:
			obj.rtype.vclip_info.vclip_num = build_vclip_index_from_untrusted(INTEL_INT(obj_rw->rtype.vclip_info.vclip_num));
			obj.rtype.vclip_info.frametime = INTEL_INT(obj_rw->rtype.vclip_info.frametime);
			obj.rtype.vclip_info.framenum  = obj_rw->rtype.vclip_info.framenum;
			break;
			
		case render_type::RT_LASER:
			break;
			
	}
}

void show_netgame_info(const netgame_info &netgame)
{
	struct netgame_info_menu_items
	{
		enum netgame_menu_info_index
		{
			game_name,
			mission_name,
			level_number,
			game_mode,
			player_counts,
			blank_1,
			game_options_header,
			difficulty,
			reactor_life,
			max_time,
			kill_goal,
			blank_2,
			duplicate_powerups_header,
			duplicate_primaries,
			duplicate_secondaries,
#if defined(DXX_BUILD_DESCENT_II)
			duplicate_accessories,
#endif
			blank_3,
			spawn_site_header,
			spawn_count,
			spawn_invulnerable_time,
			blank_4,
			objects_allowed_header,
			allow_laser_upgrade,
			allow_quad_laser,
			allow_vulcan_cannon,
			allow_spreadfire_cannon,
			allow_plasma_cannon,
			allow_fusion_cannon,
			/* concussion missiles are always allowed, so there is no line
			 * item for them */
			allow_homing_missiles,
			allow_proximity_bombs,
			allow_smart_missiles,
			allow_mega_missiles,
#if defined(DXX_BUILD_DESCENT_II)
			allow_super_laser_upgrade,
			allow_gauss_cannon,
			allow_helix_cannon,
			allow_phoenix_cannon,
			allow_omega_cannon,
			allow_flash_missiles,
			allow_guided_missiles,
			allow_smart_mines,
			allow_mercury_missiles,
			allow_earthshaker_missiles,
#endif
			allow_cloaking,
			allow_invulnerability,
#if defined(DXX_BUILD_DESCENT_II)
			allow_afterburner,
			allow_ammo_rack,
			allow_energy_converter,
			allow_headlight,
#endif
			blank_5,
			objects_granted_header,
			grant_laser_level,
			grant_quad_laser,
			grant_vulcan_cannon,
			grant_spreadfire_cannon,
			grant_plasma_cannon,
			grant_fusion_cannon,
#if defined(DXX_BUILD_DESCENT_II)
			grant_gauss_cannon,
			grant_helix_cannon,
			grant_phoenix_cannon,
			grant_omega_cannon,
			grant_afterburner,
			grant_ammo_rack,
			grant_energy_converter,
			grant_headlight,
#endif
			blank_6,
			misc_options_header,
			show_all_players_on_automap,
#if defined(DXX_BUILD_DESCENT_II)
			allow_marker_camera,
			indestructible_lights,
			thief_permitted,
			thief_steals_energy,
			guidebot_enabled,
#endif
			bright_player_ships,
			enemy_names_on_hud,
			friendly_fire,
			blank_7,
			network_options_header,
			packets_per_second,
		};
		enum
		{
			count_array_elements = static_cast<unsigned>(packets_per_second) + 1
		};
		enumerated_array<std::array<char, 50>, count_array_elements, netgame_menu_info_index> lines;
		enumerated_array<newmenu_item, count_array_elements, netgame_menu_info_index> menu_items;
		netgame_info_menu_items(const netgame_info &netgame)
		{
			for (auto &&[m, l] : zip(menu_items, lines))
				nm_set_item_text(m, l.data());

			menu_items[blank_1].text =
				menu_items[blank_2].text =
				menu_items[blank_3].text =
				menu_items[blank_4].text =
				menu_items[blank_5].text =
				menu_items[blank_6].text =
				menu_items[blank_7].text =
				const_cast<char *>(" ");

			menu_items[game_options_header].text = const_cast<char *>("Game Options:");
			menu_items[duplicate_powerups_header].text = const_cast<char *>("Duplicate Powerups:");
			menu_items[spawn_site_header].text = const_cast<char *>("Spawn Options:");
			menu_items[objects_allowed_header].text = const_cast<char *>("Objects Allowed:");
			menu_items[objects_granted_header].text = const_cast<char *>("Objects Granted At Spawn:");
			menu_items[misc_options_header].text = const_cast<char *>("Misc. Options:");
			menu_items[network_options_header].text = const_cast<char *>("Network Options:");

			array_snprintf(lines[game_name], "Game Name\t  %s", netgame.game_name.data());
			array_snprintf(lines[mission_name], "Mission Name\t  %s", netgame.mission_title.data());
			array_snprintf(lines[level_number], "Level\t  %s%i", (netgame.levelnum < 0) ? "S" : " ", abs(netgame.levelnum));
			const auto gamemode = underlying_value(netgame.gamemode);
			array_snprintf(lines[game_mode], "Game Mode\t  %s", gamemode < GMNames.size() ? GMNames[gamemode] : "INVALID");
			array_snprintf(lines[player_counts], "Players\t  %i/%i", netgame.numplayers, netgame.max_numplayers);
			array_snprintf(lines[difficulty], "Difficulty\t  %s", MENU_DIFFICULTY_TEXT(netgame.difficulty));
			array_snprintf(lines[reactor_life], "Reactor Life\t  %i %s", netgame.control_invul_time / F1_0 / 60, TXT_MINUTES_ABBREV);
			array_snprintf(lines[max_time], "Max Time\t  %i %s", netgame.PlayTimeAllowed.count() / (F1_0 * 60), TXT_MINUTES_ABBREV);
			array_snprintf(lines[kill_goal], "Kill Goal\t  %i", netgame.KillGoal * 5);
			array_snprintf(lines[duplicate_primaries], "Primaries\t  %i", static_cast<int>(netgame.DuplicatePowerups.get_primary_count()));
			array_snprintf(lines[duplicate_secondaries], "Secondaries\t  %i", static_cast<int>(netgame.DuplicatePowerups.get_secondary_count()));
#if defined(DXX_BUILD_DESCENT_II)
			array_snprintf(lines[duplicate_accessories], "Accessories\t  %i", static_cast<int>(netgame.DuplicatePowerups.get_accessory_count()));
#endif
			array_snprintf(lines[spawn_count], "Use * Furthest Spawn Sites\t  %i", netgame.SecludedSpawns+1);
			array_snprintf(lines[spawn_invulnerable_time], "Invulnerable Time\t  %1.1f sec", static_cast<float>(netgame.InvulAppear) / 2);
			array_snprintf(lines[allow_laser_upgrade], "Laser Upgrade\t  %s", (netgame.AllowedItems & netflag_flag::NETFLAG_DOLASER) != netflag_flag::None ? TXT_YES : TXT_NO);
			array_snprintf(lines[allow_quad_laser], "Quad Lasers\t  %s", (netgame.AllowedItems & netflag_flag::NETFLAG_DOQUAD) != netflag_flag::None ? TXT_YES : TXT_NO);
			array_snprintf(lines[allow_vulcan_cannon], "Vulcan Cannon\t  %s", (netgame.AllowedItems & netflag_flag::NETFLAG_DOVULCAN) != netflag_flag::None ? TXT_YES : TXT_NO);
			array_snprintf(lines[allow_spreadfire_cannon], "Spreadfire Cannon\t  %s", (netgame.AllowedItems & netflag_flag::NETFLAG_DOSPREAD) != netflag_flag::None ? TXT_YES : TXT_NO);
			array_snprintf(lines[allow_plasma_cannon], "Plasma Cannon\t  %s", (netgame.AllowedItems & netflag_flag::NETFLAG_DOPLASMA) != netflag_flag::None ? TXT_YES : TXT_NO);
			array_snprintf(lines[allow_fusion_cannon], "Fusion Cannon\t  %s", (netgame.AllowedItems & netflag_flag::NETFLAG_DOFUSION) != netflag_flag::None ? TXT_YES : TXT_NO);
			array_snprintf(lines[allow_homing_missiles], "Homing Missiles\t  %s", (netgame.AllowedItems & netflag_flag::NETFLAG_DOHOMING) != netflag_flag::None ? TXT_YES : TXT_NO);
			array_snprintf(lines[allow_proximity_bombs], "Proximity Bombs\t  %s", (netgame.AllowedItems & netflag_flag::NETFLAG_DOPROXIM) != netflag_flag::None ? TXT_YES : TXT_NO);
			array_snprintf(lines[allow_smart_missiles], "Smart Missiles\t  %s", (netgame.AllowedItems & netflag_flag::NETFLAG_DOSMART) != netflag_flag::None ? TXT_YES : TXT_NO);
			array_snprintf(lines[allow_mega_missiles], "Mega Missiles\t  %s", (netgame.AllowedItems & netflag_flag::NETFLAG_DOMEGA) != netflag_flag::None ? TXT_YES : TXT_NO);
#if defined(DXX_BUILD_DESCENT_II)
			array_snprintf(lines[allow_super_laser_upgrade], "Super Lasers\t  %s", (netgame.AllowedItems & netflag_flag::NETFLAG_DOSUPERLASER) != netflag_flag::None ? TXT_YES : TXT_NO);
			array_snprintf(lines[allow_gauss_cannon], "Gauss Cannon\t  %s", (netgame.AllowedItems & netflag_flag::NETFLAG_DOGAUSS) != netflag_flag::None ? TXT_YES : TXT_NO);
			array_snprintf(lines[allow_helix_cannon], "Helix Cannon\t  %s", (netgame.AllowedItems & netflag_flag::NETFLAG_DOHELIX) != netflag_flag::None ? TXT_YES : TXT_NO);
			array_snprintf(lines[allow_phoenix_cannon], "Phoenix Cannon\t  %s", (netgame.AllowedItems & netflag_flag::NETFLAG_DOPHOENIX) != netflag_flag::None ? TXT_YES : TXT_NO);
			array_snprintf(lines[allow_omega_cannon], "Omega Cannon\t  %s", (netgame.AllowedItems & netflag_flag::NETFLAG_DOOMEGA) != netflag_flag::None ? TXT_YES : TXT_NO);
			array_snprintf(lines[allow_flash_missiles], "Flash Missiles\t  %s", (netgame.AllowedItems & netflag_flag::NETFLAG_DOFLASH) != netflag_flag::None ? TXT_YES : TXT_NO);
			array_snprintf(lines[allow_guided_missiles], "Guided Missiles\t  %s", (netgame.AllowedItems & netflag_flag::NETFLAG_DOGUIDED) != netflag_flag::None ? TXT_YES : TXT_NO);
			array_snprintf(lines[allow_smart_mines], "Smart Mines\t  %s", (netgame.AllowedItems & netflag_flag::NETFLAG_DOSMARTMINE) != netflag_flag::None ? TXT_YES : TXT_NO);
			array_snprintf(lines[allow_mercury_missiles], "Mercury Missiles\t  %s", (netgame.AllowedItems & netflag_flag::NETFLAG_DOMERCURY) != netflag_flag::None ? TXT_YES : TXT_NO);
			array_snprintf(lines[allow_earthshaker_missiles], "Earthshaker Missiles\t  %s", (netgame.AllowedItems & netflag_flag::NETFLAG_DOSHAKER) != netflag_flag::None ? TXT_YES : TXT_NO);
#endif
			array_snprintf(lines[allow_cloaking], "Cloaking\t  %s", (netgame.AllowedItems & netflag_flag::NETFLAG_DOCLOAK) != netflag_flag::None ? TXT_YES : TXT_NO);
			array_snprintf(lines[allow_invulnerability], "Invulnerability\t  %s", (netgame.AllowedItems & netflag_flag::NETFLAG_DOINVUL) != netflag_flag::None ? TXT_YES : TXT_NO);
#if defined(DXX_BUILD_DESCENT_II)
			array_snprintf(lines[allow_afterburner], "Afterburners\t  %s", (netgame.AllowedItems & netflag_flag::NETFLAG_DOAFTERBURNER) != netflag_flag::None ? TXT_YES : TXT_NO);
			array_snprintf(lines[allow_ammo_rack], "Ammo Rack\t  %s", (netgame.AllowedItems & netflag_flag::NETFLAG_DOAMMORACK) != netflag_flag::None ? TXT_YES : TXT_NO);
			array_snprintf(lines[allow_energy_converter], "Energy Converter\t  %s", (netgame.AllowedItems & netflag_flag::NETFLAG_DOCONVERTER) != netflag_flag::None ? TXT_YES : TXT_NO);
			array_snprintf(lines[allow_headlight], "Headlight\t  %s", (netgame.AllowedItems & netflag_flag::NETFLAG_DOHEADLIGHT) != netflag_flag::None ? TXT_YES : TXT_NO);
#endif
			array_snprintf(lines[grant_laser_level], "Laser Level\t  %u", static_cast<unsigned>(map_granted_flags_to_laser_level(netgame.SpawnGrantedItems)) + 1);
			array_snprintf(lines[grant_quad_laser], "Quad Lasers\t  %s", menu_bit_wrapper(netgame.SpawnGrantedItems.mask, netgrant_flag::NETGRANT_QUAD) != netgrant_flag::None ? TXT_YES : TXT_NO);
			array_snprintf(lines[grant_vulcan_cannon], "Vulcan Cannon\t  %s", menu_bit_wrapper(netgame.SpawnGrantedItems.mask, netgrant_flag::NETGRANT_VULCAN) != netgrant_flag::None ? TXT_YES : TXT_NO);
			array_snprintf(lines[grant_spreadfire_cannon], "Spreadfire Cannon\t  %s", menu_bit_wrapper(netgame.SpawnGrantedItems.mask, netgrant_flag::NETGRANT_SPREAD) != netgrant_flag::None ? TXT_YES : TXT_NO);
			array_snprintf(lines[grant_plasma_cannon], "Plasma Cannon\t  %s", menu_bit_wrapper(netgame.SpawnGrantedItems.mask, netgrant_flag::NETGRANT_PLASMA) != netgrant_flag::None ? TXT_YES : TXT_NO);
			array_snprintf(lines[grant_fusion_cannon], "Fusion Cannon\t  %s", menu_bit_wrapper(netgame.SpawnGrantedItems.mask, netgrant_flag::NETGRANT_FUSION) != netgrant_flag::None ? TXT_YES : TXT_NO);
#if defined(DXX_BUILD_DESCENT_II)
			array_snprintf(lines[grant_gauss_cannon], "Gauss Cannon\t  %s", menu_bit_wrapper(netgame.SpawnGrantedItems.mask, netgrant_flag::NETGRANT_GAUSS) != netgrant_flag::None ? TXT_YES : TXT_NO);
			array_snprintf(lines[grant_helix_cannon], "Helix Cannon\t  %s", menu_bit_wrapper(netgame.SpawnGrantedItems.mask, netgrant_flag::NETGRANT_HELIX) != netgrant_flag::None ? TXT_YES : TXT_NO);
			array_snprintf(lines[grant_phoenix_cannon], "Phoenix Cannon\t  %s", menu_bit_wrapper(netgame.SpawnGrantedItems.mask, netgrant_flag::NETGRANT_PHOENIX) != netgrant_flag::None ? TXT_YES : TXT_NO);
			array_snprintf(lines[grant_omega_cannon], "Omega Cannon\t  %s", menu_bit_wrapper(netgame.SpawnGrantedItems.mask, netgrant_flag::NETGRANT_OMEGA) != netgrant_flag::None ? TXT_YES : TXT_NO);
			array_snprintf(lines[grant_afterburner], "Afterburner\t  %s", menu_bit_wrapper(netgame.SpawnGrantedItems.mask, netgrant_flag::NETGRANT_AFTERBURNER) != netgrant_flag::None ? TXT_YES : TXT_NO);
			array_snprintf(lines[grant_ammo_rack], "Ammo Rack\t  %s", menu_bit_wrapper(netgame.SpawnGrantedItems.mask, netgrant_flag::NETGRANT_AMMORACK) != netgrant_flag::None ? TXT_YES : TXT_NO);
			array_snprintf(lines[grant_energy_converter], "Energy Converter\t  %s", menu_bit_wrapper(netgame.SpawnGrantedItems.mask, netgrant_flag::NETGRANT_CONVERTER) != netgrant_flag::None ? TXT_YES : TXT_NO);
			array_snprintf(lines[grant_headlight], "Headlight\t  %s", menu_bit_wrapper(netgame.SpawnGrantedItems.mask, netgrant_flag::NETGRANT_HEADLIGHT) != netgrant_flag::None ? TXT_YES : TXT_NO);
#endif
			array_snprintf(lines[show_all_players_on_automap], "Show All Players On Automap\t  %s", ((netgame.game_flag & netgame_rule_flags::show_all_players_on_automap) != netgame_rule_flags::None) ? TXT_YES : TXT_NO);
#if defined(DXX_BUILD_DESCENT_II)
			array_snprintf(lines[allow_marker_camera], "Allow Marker Camera Views\t  %s", netgame.Allow_marker_view?TXT_YES:TXT_NO);
			array_snprintf(lines[indestructible_lights], "Indestructible Lights\t  %s", netgame.AlwaysLighting?TXT_YES:TXT_NO);
			array_snprintf(lines[thief_permitted], "Thief permitted\t  %s", (netgame.ThiefModifierFlags & ThiefModifier::Absent) ? TXT_NO : TXT_YES);
			array_snprintf(lines[thief_steals_energy], "Thief steals energy weapons\t  %s", (netgame.ThiefModifierFlags & ThiefModifier::NoEnergyWeapons) ? TXT_NO : TXT_YES);
			array_snprintf(lines[guidebot_enabled], "Guidebot enabled (experimental)\t  %s", Netgame.AllowGuidebot ? TXT_YES : TXT_NO);
#endif
			array_snprintf(lines[bright_player_ships], "Bright Player Ships\t  %s", netgame.BrightPlayers?TXT_YES:TXT_NO);
			array_snprintf(lines[enemy_names_on_hud], "Enemy Names On Hud\t  %s", netgame.ShowEnemyNames?TXT_YES:TXT_NO);
			array_snprintf(lines[friendly_fire], "Friendly Fire (Team, Coop)\t  %s", netgame.NoFriendlyFire?TXT_NO:TXT_YES);
			array_snprintf(lines[packets_per_second], "Packets Per Second\t  %i", netgame.PacketsPerSec);
		}
	};
	struct netgame_info_menu : netgame_info_menu_items, passive_newmenu
	{
		netgame_info_menu(const netgame_info &netgame, grs_canvas &src) :
			netgame_info_menu_items(netgame),
			passive_newmenu(menu_title{nullptr}, menu_subtitle{"Netgame Info & Rules"}, menu_filename{nullptr}, tiny_mode_flag::tiny, tab_processing_flag::ignore, adjusted_citem::create(menu_items, 0), src)
			{
			}
	};
	auto menu = window_create<netgame_info_menu>(netgame, grd_curscreen->sc_canvas);
	(void)menu;
}
}
