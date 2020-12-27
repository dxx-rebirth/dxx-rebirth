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
 * Defines and exported variables for multi.c
 *
 */

#pragma once

#include <type_traits>
#include "dxxsconf.h"
#include "fwd-partial_range.h"
#include "fwd-player.h"
#include "player-callsign.h"
#include "player-flags.h"
#include "weapon.h"
#include "mission.h"
#include "newmenu.h"
#include "powerup.h"
#include "fwd-object.h"
#include "fwd-wall.h"
#include "window.h"
#include "game.h"
#include "gameplayopt.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/time.h>
#endif

#include <stdexcept>
#include "digi.h"
#include "pack.h"
#include "ntstring.h"
#include "compiler-static_assert.h"
#include <array>

namespace dcx {

struct _sockaddr
{
	union {
		sockaddr sa;
		sockaddr_in sin;
#if DXX_USE_IPv6
		sockaddr_in6 sin6;
#define DXX_IPv6(v4,v6) v6
#else
#define DXX_IPv6(v4,v6) v4
#endif
	};
	enum {
		presentation_buffer_size = DXX_IPv6(INET_ADDRSTRLEN, INET6_ADDRSTRLEN),
	};
	static int address_family()
	{
		return DXX_IPv6(AF_INET, AF_INET6);
	}
	static int protocol_family()
	{
		return DXX_IPv6(PF_INET, PF_INET6);
	}
#undef DXX_IPv6
};

// PROTOCOL VARIABLES AND DEFINES
extern int multi_protocol; // set and determinate used protocol

}
#define MULTI_PROTO_UDP 1 // UDP protocol

// What version of the multiplayer protocol is this? Increment each time something drastic changes in Multiplayer without the version number changes. Reset to 0 each time the version of the game changes
#define MULTI_PROTO_VERSION	static_cast<uint16_t>(12)
// PROTOCOL VARIABLES AND DEFINES - END

// limits for Packets (i.e. positional updates) per sec
#define DEFAULT_PPS 30
#define MIN_PPS 5
#define MAX_PPS 40

#define MAX_MESSAGE_LEN 35


#if defined(DXX_BUILD_DESCENT_I)
#define MAX_NET_CREATE_OBJECTS 20
#define MAX_MULTI_MESSAGE_LEN  90 //didn't change it, just moved it up
#elif defined(DXX_BUILD_DESCENT_II)
#define MAX_NET_CREATE_OBJECTS  40
#define MAX_MULTI_MESSAGE_LEN   120

#endif

#define NETGAME_ANARCHY         0
#define NETGAME_TEAM_ANARCHY    1
#define NETGAME_ROBOT_ANARCHY   2
#define NETGAME_COOPERATIVE     3
#if defined(DXX_BUILD_DESCENT_II)
#define NETGAME_CAPTURE_FLAG    4
#define NETGAME_HOARD           5
#define NETGAME_TEAM_HOARD      6
#endif
#define NETGAME_BOUNTY		7

#define NETSTAT_MENU                0
#define NETSTAT_PLAYING             1
#define NETSTAT_BROWSING            2
#define NETSTAT_WAITING             3
#define NETSTAT_STARTING            4
#define NETSTAT_ENDLEVEL            5

#define CONNECT_DISCONNECTED        0
#define CONNECT_PLAYING             1
#define CONNECT_WAITING             2
#define CONNECT_DIED_IN_MINE        3
#define CONNECT_FOUND_SECRET        4
#define CONNECT_ESCAPE_TUNNEL       5
#define CONNECT_END_MENU            6
#if defined(DXX_BUILD_DESCENT_II)
#define CONNECT_KMATRIX_WAITING     7 // Like CONNECT_WAITING but used especially in kmatrix.c to seperate "escaped" and "waiting"
#endif

// reasons for a packet with type PID_DUMP
#define DUMP_CLOSED     0 // no new players allowed after game started
#define DUMP_FULL       1 // player cound maxed out
#define DUMP_ENDLEVEL   2
#define DUMP_DORK       3
#define DUMP_ABORTED    4
#define DUMP_CONNECTED  5 // never used
#define DUMP_LEVEL      6
#define DUMP_KICKED     7
#define DUMP_PKTTIMEOUT 8

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
#define NETFLAG_LABEL_QUAD	 "Quad Lasers"
#define NETFLAG_LABEL_VULCAN	 "Vulcan cannon"
#define NETFLAG_LABEL_SPREAD	 "Spreadfire cannon"
#define NETFLAG_LABEL_PLASMA	 "Plasma cannon"
#define NETFLAG_LABEL_FUSION	 "Fusion cannon"
#define for_each_netflag_value(VALUE)	\
	VALUE(NETFLAG_DOLASER, "Laser upgrade")	\
	VALUE(NETFLAG_DOQUAD, NETFLAG_LABEL_QUAD)	\
	VALUE(NETFLAG_DOVULCAN, NETFLAG_LABEL_VULCAN)	\
	VALUE(NETFLAG_DOSPREAD, NETFLAG_LABEL_SPREAD)	\
	VALUE(NETFLAG_DOPLASMA, NETFLAG_LABEL_PLASMA)	\
	VALUE(NETFLAG_DOFUSION, NETFLAG_LABEL_FUSION)	\
	VALUE(NETFLAG_DOHOMING, "Homing Missiles")	\
	VALUE(NETFLAG_DOPROXIM, "Proximity Bombs")	\
	VALUE(NETFLAG_DOSMART, "Smart Missiles")	\
	VALUE(NETFLAG_DOMEGA, "Mega Missiles")	\
	VALUE(NETFLAG_DOCLOAK, "Cloaking")	\
	VALUE(NETFLAG_DOINVUL, "Invulnerability")	\
	D2X_MP_NETFLAGS(VALUE)	\

#define for_each_netgrant_value(VALUE)	\
	VALUE(NETGRANT_QUAD, NETFLAG_LABEL_QUAD)	\
	VALUE(NETGRANT_VULCAN, NETFLAG_LABEL_VULCAN)	\
	VALUE(NETGRANT_SPREAD, NETFLAG_LABEL_SPREAD)	\
	VALUE(NETGRANT_PLASMA, NETFLAG_LABEL_PLASMA)	\
	VALUE(NETGRANT_FUSION, NETFLAG_LABEL_FUSION)	\
	D2X_MP_NETGRANT(VALUE)

#define MULTI_GAME_TYPE_COUNT	8
namespace dsx {
#if defined(DXX_BUILD_DESCENT_I)
constexpr std::integral_constant<unsigned, 13> MULTI_GAME_NAME_LENGTH{};
constexpr std::integral_constant<unsigned, 18> MULTI_ALLOW_POWERUP_TEXT_LENGTH{};
#define MULTI_ALLOW_POWERUP_MAX 12
#define D2X_MP_NETFLAGS(VALUE)
#define DXX_GRANT_LASER_LEVEL_BITS	2
#define D2X_MP_NETGRANT(VALUE)
#elif defined(DXX_BUILD_DESCENT_II)
constexpr std::integral_constant<unsigned, 17> MULTI_GAME_NAME_LENGTH{};
constexpr std::integral_constant<unsigned, 21> MULTI_ALLOW_POWERUP_TEXT_LENGTH{};
#define MULTI_ALLOW_POWERUP_MAX 26
#define NETFLAG_LABEL_GAUSS	"Gauss cannon"
#define NETFLAG_LABEL_HELIX	"Helix cannon"
#define NETFLAG_LABEL_PHOENIX	"Phoenix cannon"
#define NETFLAG_LABEL_OMEGA	"Omega cannon"
#define NETFLAG_LABEL_AFTERBURNER	"Afterburners"
#define NETFLAG_LABEL_AMMORACK	"Ammo rack"
#define NETFLAG_LABEL_CONVERTER	"Energy Converter"
#define NETFLAG_LABEL_HEADLIGHT	"Headlight"
#define D2X_MP_NETFLAGS(VALUE)	\
	VALUE(NETFLAG_DOSUPERLASER, "Super lasers")	\
	VALUE(NETFLAG_DOGAUSS, NETFLAG_LABEL_GAUSS)	\
	VALUE(NETFLAG_DOHELIX, NETFLAG_LABEL_HELIX)	\
	VALUE(NETFLAG_DOPHOENIX, NETFLAG_LABEL_PHOENIX)	\
	VALUE(NETFLAG_DOOMEGA, NETFLAG_LABEL_OMEGA)	\
	VALUE(NETFLAG_DOFLASH, "Flash Missiles")	\
	VALUE(NETFLAG_DOGUIDED, "Guided Missiles")	\
	VALUE(NETFLAG_DOSMARTMINE, "Smart Mines")	\
	VALUE(NETFLAG_DOMERCURY, "Mercury Missiles")	\
	VALUE(NETFLAG_DOSHAKER, "Earthshaker Missiles")	\
	VALUE(NETFLAG_DOAFTERBURNER, NETFLAG_LABEL_AFTERBURNER)	\
	VALUE(NETFLAG_DOAMMORACK, NETFLAG_LABEL_AMMORACK)	\
	VALUE(NETFLAG_DOCONVERTER, NETFLAG_LABEL_CONVERTER)	\
	VALUE(NETFLAG_DOHEADLIGHT, NETFLAG_LABEL_HEADLIGHT)

#define DXX_GRANT_LASER_LEVEL_BITS	3
#define D2X_MP_NETGRANT(VALUE)	\
	VALUE(NETGRANT_GAUSS, NETFLAG_LABEL_GAUSS)	\
	VALUE(NETGRANT_HELIX, NETFLAG_LABEL_HELIX)	\
	VALUE(NETGRANT_PHOENIX, NETFLAG_LABEL_PHOENIX)	\
	VALUE(NETGRANT_OMEGA, NETFLAG_LABEL_OMEGA)	\
	VALUE(NETGRANT_AFTERBURNER, NETFLAG_LABEL_AFTERBURNER)	\
	VALUE(NETGRANT_AMMORACK, NETFLAG_LABEL_AMMORACK)	\
	VALUE(NETGRANT_CONVERTER, NETFLAG_LABEL_CONVERTER)	\
	VALUE(NETGRANT_HEADLIGHT, NETFLAG_LABEL_HEADLIGHT)

#endif

namespace multi {
using endlevel_poll_function_type = int(newmenu *menu,const d_event &event, const unused_newmenu_userdata_t *);
struct dispatch_table
{
	constexpr const dispatch_table *operator->() const
	{
		return this;
	}
	virtual int objnum_is_past(objnum_t objnum) const = 0;
	virtual void do_protocol_frame(int force, int listen) const = 0;
	virtual window_event_result level_sync() const = 0;
	virtual void send_endlevel_packet() const = 0;
	virtual void kick_player(const _sockaddr &dump_addr, int why) const = 0;
	virtual void disconnect_player(int playernum) const = 0;
	virtual int end_current_level(int *secret) const = 0;
	virtual void leave_game() const = 0;
};
}
}

#define define_netflag_bit_enum(NAME,STR)	BIT_##NAME,
#define define_netflag_bit_mask(NAME,STR)	static constexpr auto NAME = std::integral_constant<unsigned, (1 << BIT_##NAME)>{};
#define define_netflag_powerup_mask(NAME,STR)	| (NAME)
enum { for_each_netflag_value(define_netflag_bit_enum) };
// Bitmask for netgame_info->AllowedItems to set allowed items in Netgame
for_each_netflag_value(define_netflag_bit_mask);
enum { NETFLAG_DOPOWERUP = 0 for_each_netflag_value(define_netflag_powerup_mask) };
enum {
	BIT_NETGRANT_LASER = DXX_GRANT_LASER_LEVEL_BITS - 1,
	for_each_netgrant_value(define_netflag_bit_enum)
	BIT_NETGRANT_MAXIMUM
};
for_each_netgrant_value(define_netflag_bit_mask);
#undef define_netflag_bit_enum
#undef define_netflag_bit_mask
#undef define_netflag_powerup_mask

namespace dsx {

struct packed_spawn_granted_items
{
#if defined(DXX_BUILD_DESCENT_I)
	typedef uint8_t mask_type;
#elif defined(DXX_BUILD_DESCENT_II)
	typedef uint16_t mask_type;
#endif
	mask_type mask;
	static_assert(BIT_NETGRANT_MAXIMUM <= sizeof(mask) << 3, "mask too small");
	packed_spawn_granted_items() = default;
	constexpr packed_spawn_granted_items(mask_type m) :
		mask(m)
	{
	}
	template <unsigned U>
		constexpr packed_spawn_granted_items(std::integral_constant<unsigned, U>) :
			mask(U)
	{
		assert_equal(U, static_cast<mask_type>(U), "truncation error");
	}
	explicit operator bool() const { return mask; }
	bool has_quad_laser() const { return mask & NETGRANT_QUAD; }
#if defined(DXX_BUILD_DESCENT_II)
	bool has_afterburner() const { return mask & NETGRANT_AFTERBURNER; }
#endif
};

class packed_netduplicate_items
{
public:
	enum
	{
		primary_shift = 0,
		primary_width = 3,
		secondary_shift = primary_shift + primary_width,
		secondary_width = 3,
#if defined(DXX_BUILD_DESCENT_II)
		accessory_shift = secondary_shift + secondary_width,
		accessory_width = 3,
#endif
	};
private:
#if defined(DXX_BUILD_DESCENT_I)
	typedef uint8_t count_type;
#elif defined(DXX_BUILD_DESCENT_II)
	typedef uint16_t count_type;
#endif
	count_type count;
	template <uint_fast32_t shift, uint_fast32_t width>
	uint_fast32_t get_sub_field() const
	{
		static_assert(shift + width <= sizeof(count) << 3, "shift+width too big");
		constexpr auto low_mask = (1 << width) - 1;
		return (count >> shift) & low_mask;
	}
public:
	template <uint_fast32_t shift, uint_fast32_t width>
	void set_sub_field(uint_fast32_t value)
	{
		constexpr auto low_mask = (1 << width) - 1;
		constexpr auto shifted_mask = low_mask << shift;
		count = (count & ~shifted_mask) | (value << shift);
	}
#define DEFINE_ACCESSOR(N)	\
	uint_fast32_t get_##N##_count() const	\
	{	\
		return get_sub_field<N##_shift, N##_width>();	\
	}	\
	void set_##N##_count(uint_fast32_t value)	\
	{	\
		set_sub_field<N##_shift, N##_width>(value);	\
	}
	DEFINE_ACCESSOR(primary);
	DEFINE_ACCESSOR(secondary);
#if defined(DXX_BUILD_DESCENT_II)
	DEFINE_ACCESSOR(accessory);
#endif
	count_type get_packed_field() const
	{
		return count;
	}
	void set_packed_field(count_type c)
	{
		count = c;
	}
};

static inline laser_level map_granted_flags_to_laser_level(const packed_spawn_granted_items &grant)
{
	/* Laser level in lowest bits */
	return laser_level{static_cast<uint8_t>(grant.mask & ((1 << DXX_GRANT_LASER_LEVEL_BITS) - 1))};
}
player_flags map_granted_flags_to_player_flags(packed_spawn_granted_items grant);
uint_fast32_t map_granted_flags_to_primary_weapon_flags(packed_spawn_granted_items grant);
uint16_t map_granted_flags_to_vulcan_ammo(packed_spawn_granted_items grant);
void multi_digi_link_sound_to_pos(int soundnum, vcsegptridx_t segnum, unsigned sidenum, const vms_vector &pos, int forever, fix max_volume);
void multi_object_to_object_rw(object &obj, object_rw *obj_rw);
void multi_object_rw_to_object(object_rw *obj_rw, object &obj);

using GMNames_array = std::array<char[MULTI_GAME_NAME_LENGTH], MULTI_GAME_TYPE_COUNT>;
extern const GMNames_array GMNames;
using multi_allow_powerup_text_array = std::array<char[MULTI_ALLOW_POWERUP_TEXT_LENGTH], MULTI_ALLOW_POWERUP_MAX>;
extern const multi_allow_powerup_text_array multi_allow_powerup_text;
extern const std::array<char[8], MULTI_GAME_TYPE_COUNT> GMNamesShrt;
}

namespace dcx {
extern std::array<objnum_t, MAX_NET_CREATE_OBJECTS> Net_create_objnums;
extern unsigned Net_create_loc;
int multi_maybe_disable_friendly_fire(const object_base *attacker);
}

namespace dsx {

void multi_send_fire(int laser_gun, laser_level, int laser_flags, int laser_fired, objnum_t laser_track, imobjptridx_t is_bomb_objnum);
void multi_send_destroy_controlcen(objnum_t objnum, playernum_t player);
void multi_send_position(object &objnum);
void multi_send_kill(vmobjptridx_t objnum);
void multi_send_remobj(vmobjidx_t objnum);
void multi_send_door_open(vcsegidx_t segnum, unsigned side, uint8_t flag);
void multi_send_drop_weapon(vmobjptridx_t objnum,int seed);
void multi_reset_player_object(object &objp);
}
#endif

enum msgsend_state_t {
	msgsend_none,
	msgsend_typing,
	msgsend_automap,
};

enum deres_type_t {
	deres_explode,
	deres_drop,
};

// Exported functions

struct owned_remote_objnum
{
	int8_t owner;
	uint16_t objnum;
};

extern int GetMyNetRanking();
extern void ClipRank (ubyte *rank);
objnum_t objnum_remote_to_local(uint16_t remote_obj, int8_t owner);
uint16_t objnum_local_to_remote(objnum_t local_obj, int8_t *owner);
owned_remote_objnum objnum_local_to_remote(objnum_t local);
void map_objnum_local_to_remote(int local, int remote, int owner);
void map_objnum_local_to_local(objnum_t objnum);
void reset_network_objects();
void multi_do_ping_frame();

void multi_init_objects(void);
window_event_result multi_do_frame();

#ifdef dsx
namespace dsx {

enum class multi_endlevel_type : bool
{
	normal,
#if defined(DXX_BUILD_DESCENT_I)
	secret,
#endif
};
#if defined(DXX_BUILD_DESCENT_I)
void multi_send_endlevel_start(multi_endlevel_type);
#elif defined(DXX_BUILD_DESCENT_II)
void multi_send_endlevel_start();
static inline void multi_send_endlevel_start(multi_endlevel_type)
{
	multi_send_endlevel_start();
}
#endif
void multi_send_player_deres(deres_type_t type);
void multi_send_create_powerup(powerup_type_t powerup_type, vcsegidx_t segnum, vcobjidx_t objnum, const vms_vector &pos);
}
void multi_send_play_sound(int sound_num, fix volume, sound_stack once);
#endif
void multi_send_reappear();
void multi_send_create_explosion(playernum_t);
void multi_send_controlcen_fire(const vms_vector &to_target, int gun_num, objnum_t objnum);
namespace dcx {
void multi_send_cloak(void);
void multi_send_decloak(void);
}
void multi_digi_play_sample(int sndnum, fix max_volume);
void multi_digi_play_sample_once(int soundnum, fix max_volume);
void multi_send_score(void);
void multi_send_trigger(int trigger);
#if defined(DXX_BUILD_DESCENT_II)
namespace dsx {
extern char Multi_is_guided;
void multi_send_flags(playernum_t);
struct marker_message_text_t;
void multi_send_drop_marker(unsigned player, const vms_vector &position, player_marker_index messagenum, const marker_message_text_t &text);
void multi_send_markers();
void multi_send_guided_info (const object_base &miss, char);
void multi_send_orb_bonus(playernum_t pnum, uint8_t);
void multi_send_got_orb(playernum_t pnum);
void multi_send_effect_blowup(vcsegidx_t segnum, unsigned side, const vms_vector &pnt);
void multi_send_vulcan_weapon_ammo_adjust(const vmobjptridx_t objnum);
#ifndef RELEASE
void multi_add_lifetime_kills(int count);
#endif
}
#endif
void multi_send_bounty( void );

void multi_consistency_error(int reset);
window_event_result multi_level_sync();
#ifdef dsx
namespace dsx {
void multi_send_hostage_door_status(vcwallptridx_t wallnum);
void multi_prep_level_objects(const d_vclip_array &Vclip);
void multi_prep_level_player();
void multi_leave_game(void);
void multi_process_bigdata(playernum_t pnum, const uint8_t *buf, uint_fast32_t len);
void multi_make_ghost_player(playernum_t);
void multi_make_player_ghost(playernum_t);
}
#endif
void multi_define_macro(int key);
void multi_send_macro(int key);
int multi_get_kill_list(playernum_array_t &sorted_kills);
void multi_new_game(void);
#ifdef dsx
namespace dsx {
void multi_sort_kill_list(void);
}
#endif
void multi_reset_stuff(void);
int get_team(playernum_t pnum);
void multi_disconnect_player(playernum_t);

#ifdef dsx
namespace dsx {
void multi_initiate_save_game();
void multi_initiate_restore_game();
void multi_execute_save_game(d_game_unique_state::save_slot slot, const d_game_unique_state::savegame_description &desc, partial_range_t<const player *> player_range);
#if defined(DXX_BUILD_DESCENT_I)
static inline void multi_send_got_flag (playernum_t) {}
#elif defined(DXX_BUILD_DESCENT_II)
void multi_send_got_flag (playernum_t);
#endif
}
#endif

// Exported variables

namespace dcx {
extern int Network_status;

// IMPORTANT: These variables needed for player rejoining done by protocol-specific code
extern int Network_send_objects;
extern int Network_send_object_mode;
extern int Network_send_objnum;
extern int Network_rejoined;
extern int Network_sending_extras;
extern int VerifyPlayerJoined;
extern int Player_joining_extras;
extern int Network_player_added;

extern std::array<std::array<uint16_t, MAX_PLAYERS>, MAX_PLAYERS> kill_matrix;
extern std::array<int16_t, 2> team_kills;

extern ushort my_segments_checksum;

//do we draw the kill list on the HUD?
extern int Show_kill_list;
extern int Show_reticle_name;
extern fix Show_kill_list_timer;

// Used to send network messages

extern ntstring<MAX_MESSAGE_LEN - 1> Network_message;
extern int Network_message_reciever;

// Which player 'owns' each local object for network purposes
extern std::array<sbyte, MAX_OBJECTS> object_owner;

extern int multi_quit_game;

extern std::array<msgsend_state_t, MAX_PLAYERS> multi_sending_message;
extern int multi_defining_message;
}
extern void multi_send_message_start();
void multi_send_msgsend_state(msgsend_state_t state);

#if defined(DXX_BUILD_DESCENT_II)
namespace dsx {
extern std::array<grs_main_bitmap, 2> Orb_icons;
struct hoard_highest_record
{
	unsigned points;
	unsigned player = UINT_MAX;
};

extern hoard_highest_record hoard_highest_record_stats;
}
#endif
namespace dcx {
extern playernum_t Bounty_target;

extern std::array<std::array<bitmap_index, N_PLAYER_SHIP_TEXTURES>, MAX_PLAYERS> multi_player_textures;

extern const std::array<char[16], 10> RankStrings;
#define GetRankStringWithSpace(I)	(PlayerCfg.NoRankings ? std::pair<const char *, const char *>("", "") : std::pair<const char *, const char *>(RankStrings[I], " "))

// Globals for protocol-bound Refuse-functions
extern char RefuseThisPlayer,WaitForRefuseAnswer,RefuseTeam,RefusePlayerName[12];
extern fix64 RefuseTimeLimit;
#define REFUSE_INTERVAL (F1_0*8)
}

#ifdef dsx
namespace dsx {
window_event_result multi_message_input_sub(int key, control_info &Controls);
struct bit_game_flags {
	unsigned closed : 1;
	unsigned : 1;
	unsigned show_on_map : 1;
	/*
	 * These #define are written to .NGP files and to the network.
	 * Changing them breaks ABI compatibility.
	 * The bit flags need not match in value, and are converted below in
	 * pack_game_flags / unpack_game_flags.
	 */
#define NETGAME_FLAG_CLOSED             1
#define NETGAME_FLAG_SHOW_MAP           4
#if defined(DXX_BUILD_DESCENT_II)
	unsigned hoard : 1;
	unsigned team_hoard : 1;
	unsigned endlevel : 1;
	unsigned forming : 1;
#define NETGAME_FLAG_HOARD              8
#define NETGAME_FLAG_TEAM_HOARD         16
#define NETGAME_FLAG_REALLY_ENDLEVEL    32
#define NETGAME_FLAG_REALLY_FORMING     64
#endif
} __pack__;
}

namespace dcx {
struct packed_game_flags
{
	unsigned char value;
};

#if DXX_USE_TRACKER
enum TrackerNATHolePunchWarn : uint8_t
{
	Unset,
	UserEnabledHP,
	UserRejectedHP,
};
#endif
}

namespace dsx {

static inline bit_game_flags unpack_game_flags(const packed_game_flags *p)
{
	bit_game_flags flags;
	flags.closed = !!(p->value & NETGAME_FLAG_CLOSED);
	flags.show_on_map = !!(p->value & NETGAME_FLAG_SHOW_MAP);
#if defined(DXX_BUILD_DESCENT_II)
	flags.hoard = !!(p->value & NETGAME_FLAG_HOARD);
	flags.team_hoard = !!(p->value & NETGAME_FLAG_TEAM_HOARD);
	flags.endlevel = !!(p->value & NETGAME_FLAG_REALLY_ENDLEVEL);
	flags.forming = !!(p->value & NETGAME_FLAG_REALLY_FORMING);
#endif
	return flags;
}

static inline packed_game_flags pack_game_flags(const bit_game_flags *flags)
{
	packed_game_flags p;
	p.value =
		(flags->closed ? NETGAME_FLAG_CLOSED : 0) |
		(flags->show_on_map ? NETGAME_FLAG_SHOW_MAP : 0) |
#if defined(DXX_BUILD_DESCENT_II)
		(flags->hoard ? NETGAME_FLAG_HOARD : 0) |
		(flags->team_hoard ? NETGAME_FLAG_TEAM_HOARD : 0) |
		(flags->endlevel ? NETGAME_FLAG_REALLY_ENDLEVEL : 0) |
		(flags->forming ? NETGAME_FLAG_REALLY_FORMING : 0) |
#endif
		0;
	return p;
}

#define NETGAME_NAME_LEN	25

extern struct netgame_info Netgame;
}
#endif

#define multi_i_am_master()	(Player_num == 0)
void change_playernum_to(playernum_t new_pnum);

// Multiplayer powerup capping
void MultiLevelInv_InitializeCount();
void MultiLevelInv_Recount();
#ifdef dsx
namespace dsx {
extern bool MultiLevelInv_AllowSpawn(powerup_type_t powerup_type);
}
#endif
extern void MultiLevelInv_Repopulate(fix frequency);
#ifdef dsx
namespace dsx {
uint_fast32_t multi_powerup_is_allowed(const unsigned id, const unsigned AllowedItems);
uint_fast32_t multi_powerup_is_allowed(const unsigned id, const unsigned AllowedItems, const unsigned SpawnGrantedItems);
void show_netgame_info(const netgame_info &netgame);
extern void multi_send_player_inventory(int priority);
const char *multi_common_deny_save_game(const fvcobjptr &vcobjptr, partial_range_t<const player *> player_range);
const char *multi_interactive_deny_save_game(const fvcobjptr &vcobjptr, partial_range_t<const player *> player_range, const d_level_unique_control_center_state &);
}
#endif
extern void multi_send_kill_goal_counts();
void multi_check_for_killgoal_winner();
#if defined(DXX_BUILD_DESCENT_II)
namespace dsx {
extern void multi_send_stolen_items();
void multi_send_trigger_specific(playernum_t pnum, uint8_t trig);
void multi_send_door_open_specific(playernum_t pnum, vcsegidx_t segnum, unsigned side, uint8_t flag);
void multi_send_wall_status_specific(playernum_t pnum, wallnum_t wallnum, uint8_t type, uint8_t flags, uint8_t state);
void multi_send_light_specific (playernum_t pnum, vcsegptridx_t segnum, uint8_t val);
void multi_send_capture_bonus (playernum_t pnum);
int multi_all_players_alive(const fvcobjptr &, partial_range_t<const player *>);
void multi_send_seismic(fix);
void multi_send_drop_blobs(playernum_t);
void multi_send_sound_function (char,char);
void DropFlag();
void multi_send_finish_game ();
void init_hoard_data(d_vclip_array &Vclip);
void multi_apply_goal_textures();
void multi_send_escort_goal(const d_unique_buddy_state &);
void multi_recv_escort_goal(d_unique_buddy_state &, const uint8_t *);

int HoardEquipped();
#if DXX_USE_EDITOR
void save_hoard_data(void);
#endif
}
#endif

//how to encode missiles & flares in weapon packets
#define MISSILE_ADJUST  100
#define FLARE_ADJUST    127

/*
 * The Network Players structure
 * Contains protocol-specific data with designated prefixes and general player-related data.
 * Note that not all of these infos will be sent to other users - some are used and/or set locally, only.
 */
struct netplayer_info : prohibit_void_ptr<netplayer_info>
{
#if DXX_USE_UDP
	union
	{
#if DXX_USE_UDP
		struct
		{
			struct _sockaddr	addr; // IP address of this peer
		} udp;
#endif
	} protocol;	
#endif
	callsign_t					callsign;
	sbyte						connected;
	ubyte						rank;
	fix							ping;
	fix64							LastPacketTime;
};

#ifdef dsx
#if defined(DXX_BUILD_DESCENT_II)
struct ThiefModifier
{
	enum Flags : uint8_t {
		Absent = 1,
		NoEnergyWeapons,
	};
};
#endif

namespace dsx {
/*
 * The Network Game structure
 * Contains protocol-specific data with designated prefixes and general game-related data.
 * Note that not all of these infos will be sent to clients - some are used and/or set locally, only.
 */
struct netgame_info : prohibit_void_ptr<netgame_info>
{
	using play_time_allowed_abi_ratio = std::ratio<5 * 60>;
#if DXX_USE_UDP
	union
	{
#if DXX_USE_UDP
		struct
		{
			struct _sockaddr		addr; // IP address of this netgame's host
			std::array<short, 4>			program_iver; // IVER of program for version checking
			sbyte				valid; // Status of Netgame info: -1 = Failed, Wrong version; 0 = No info, yet; 1 = Success
			uint8_t				your_index; // Tell player his designated (re)join position in players[]
			fix				GameID;
		} udp;
#endif
	} protocol;	
#endif
	ntstring<NETGAME_NAME_LEN> game_name;
	ntstring<MISSION_NAME_LEN> mission_title;
	ntstring<8> mission_name;
	int     					levelnum;
	Difficulty_level_type difficulty;
	ubyte   					gamemode;
	ubyte   					RefusePlayers;
	ubyte   					game_status;
	ubyte   					numplayers;
	ubyte   					max_numplayers;
	ubyte   					numconnected;
	bit_game_flags game_flag;
	ubyte   					team_vector;
	uint8_t						SecludedSpawns;
	uint8_t MouselookFlags;
	uint32_t					AllowedItems;
	packed_spawn_granted_items SpawnGrantedItems;
	packed_netduplicate_items DuplicatePowerups;
	unsigned ShufflePowerupSeed;
	d_mp_gameplay_options MPGameplayOptions;
#if defined(DXX_BUILD_DESCENT_II)
	uint8_t Allow_marker_view;
	uint8_t AlwaysLighting;
	uint8_t ThiefModifierFlags;
	uint8_t AllowGuidebot;
#endif
	uint8_t ShowEnemyNames;
	uint8_t BrightPlayers;
	uint8_t InvulAppear;
	ushort						segments_checksum;
	int						KillGoal;
	/* The UI enforces that this steps in units of 5 minutes, but for
	 * efficiency, it is stored as ticks (1 second = F1_0).  The UI
	 * imposes a maximum value that is small enough that overflow is
	 * impossible.
	 */
	d_time_fix PlayTimeAllowed;
	fix						level_time;
	int						control_invul_time;
	int						monitor_vector;
	short						PacketsPerSec;
	ubyte						PacketLossPrevention;
	ubyte						NoFriendlyFire;
	std::array<callsign_t, 2>					team_name;
	std::array<uint32_t, MAX_PLAYERS>						locations;
	std::array<std::array<uint16_t, MAX_PLAYERS>, MAX_PLAYERS>						kills;
	std::array<int16_t, 2>						team_kills;
	std::array<uint16_t, MAX_PLAYERS>						killed;
	std::array<uint16_t, MAX_PLAYERS>						player_kills;
	std::array<uint32_t, MAX_PLAYERS>						player_score;
	std::array<player_flags, MAX_PLAYERS>					net_player_flags;
	std::array<netplayer_info, MAX_PLAYERS> 				players;
#if DXX_USE_TRACKER
	ubyte						Tracker;
	TrackerNATHolePunchWarn TrackerNATWarned;
#endif
};


/*
 * Structure holding all powerup types we want to keep track of. Used to cap or respawn powerups and keep the level inventory steady.
 * Using uint32_t because we don't count powerup units but what the powerup contains (1 or 4 missiles, vulcam amount, etc) so we can keep track of overhead.
 * I'm sorry if this isn't very optimized but I found this easier to handle than a single variable per powerup.
 */
struct multi_level_inv
{
        std::array<uint32_t, MAX_POWERUP_TYPES> Initial; // initial (level start) count of this powerup type
        std::array<uint32_t, MAX_POWERUP_TYPES> Current; // current count of this powerup type
        std::array<fix, MAX_POWERUP_TYPES> RespawnTimer; // incremented by FrameTime if initial-current > 0 and triggers respawn after 2 seconds. Since we deal with a certain delay from clients, their inventory updates may happen a while after they remove the powerup object and we do not want to respawn it on accident during that time window!
};

namespace multi
{
	struct level_checksum_mismatch : std::runtime_error
	{
		level_checksum_mismatch() :
			runtime_error("level checksum mismatch")
		{
		}
	};
	struct local_player_not_playing : std::runtime_error
	{
		local_player_not_playing() :
			runtime_error("local player not playing")
		{
		}
	};
}
}
#endif

/* Stub for mods that remap player colors */
static inline unsigned get_player_color(unsigned pnum)
{
	return pnum;
}

static inline unsigned get_team_color(unsigned tnum)
{
	return tnum;
}

static inline unsigned get_player_or_team_color(unsigned pnum)
{
	return Game_mode & GM_TEAM
		? get_team_color(get_team(pnum))
		: get_player_color(pnum);
}
