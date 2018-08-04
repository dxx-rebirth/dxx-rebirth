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
 * Routines for EndGame, EndLevel, etc.
 *
 */

#include "dxxsconf.h"
#include <cctype>
#include <utility>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(_MSC_VER) && !defined(macintosh)
#include <unistd.h>
#endif
#include <time.h>

#if DXX_USE_OGL
#include "ogl_init.h"
#endif

#include "inferno.h"
#include "game.h"
#include "player.h"
#include "key.h"
#include "object.h"
#include "physics.h"
#include "dxxerror.h"
#include "joy.h"
#include "iff.h"
#include "pcx.h"
#include "timer.h"
#include "render.h"
#include "laser.h"
#include "event.h"
#include "screens.h"
#include "textures.h"
#include "slew.h"
#include "gauges.h"
#include "texmap.h"
#include "3d.h"
#include "effects.h"
#include "menu.h"
#include "gameseg.h"
#include "wall.h"
#include "ai.h"
#include "fuelcen.h"
#include "switch.h"
#include "digi.h"
#include "gamesave.h"
#include "scores.h"
#include "u_mem.h"
#include "palette.h"
#include "morph.h"
#include "lighting.h"
#include "newdemo.h"
#include "titles.h"
#include "collide.h"
#include "weapon.h"
#include "sounds.h"
#include "args.h"
#include "gameseq.h"
#include "gamefont.h"
#include "newmenu.h"
#include "hudmsg.h"
#include "endlevel.h"
#include "kmatrix.h"
#  include "multi.h"
#include "playsave.h"
#include "fireball.h"
#include "kconfig.h"
#include "config.h"
#include "robot.h"
#include "automap.h"
#include "cntrlcen.h"
#include "powerup.h"
#include "text.h"
#include "piggy.h"
#include "texmerge.h"
#include "paging.h"
#include "mission.h"
#include "state.h"
#include "songs.h"
#include "gamepal.h"
#include "controls.h"
#include "credits.h"
#include "gamemine.h"
#if DXX_USE_EDITOR
#include "editor/editor.h"
#endif
#include "strutil.h"
#include "rle.h"
#include "segment.h"
#include "gameseg.h"
#include "fmtcheck.h"

#include "compiler-range_for.h"
#include "partial_range.h"

#if defined(DXX_BUILD_DESCENT_I)
#include "custom.h"
#define GLITZ_BACKGROUND	Menu_pcx_name

#elif defined(DXX_BUILD_DESCENT_II)
#include "movie.h"
#define GLITZ_BACKGROUND	STARS_BACKGROUND

namespace dsx {
static void StartNewLevelSecret(int level_num, int page_in_textures);
static void InitPlayerPosition(fvmobjptridx &vmobjptridx, fvmsegptridx &vmsegptridx, int random_flag);
static void DoEndGame();
static void filter_objects_from_level(fvmobjptr &vmobjptr);
PHYSFSX_gets_line_t<FILENAME_LEN> Current_level_palette;
int	First_secret_visit = 1;
}
#endif

namespace {

class preserve_player_object_info
{
	player_info plr_info;
	fix plr_shields;
	/* Cache the reference, not the value.  This class is designed
	 * to be alive across a call to LoadLevel, which may change the
	 * value of the object number.
	 */
	const objnum_t &objnum;
public:
	preserve_player_object_info(const objnum_t &o) :
		objnum(o)
	{
		auto &plr = *vcobjptr(objnum);
		plr_shields = plr.shields;
		plr_info = plr.ctype.player_info;
	}
	void restore() const
	{
		auto &plr = *vmobjptr(objnum);
		plr.shields = plr_shields;
		plr.ctype.player_info = plr_info;
	}
};

}

namespace dsx {
static window_event_result AdvanceLevel(int secret_flag);
static void StartLevel(int random_flag);
}
static void copy_defaults_to_robot_all(void);

namespace dcx {
//Current_level_num starts at 1 for the first level
//-1,-2,-3 are secret levels
//0 used to mean not a real level loaded (i.e. editor generated level), but this hack has been removed
int	Current_level_num=1,Next_level_num;
PHYSFSX_gets_line_t<LEVEL_NAME_LEN> Current_level_name;

// Global variables describing the player
unsigned	N_players=1;	// Number of players ( >1 means a net game, eh?)
playernum_t Player_num;	// The player number who is on the console.
}
namespace dcx {
fix StartingShields=INITIAL_SHIELDS;
array<obj_position, MAX_PLAYERS> Player_init;

// Global variables telling what sort of game we have
unsigned NumNetPlayerPositions;
int	Do_appearance_effect=0;

template <object_type_t type>
static bool is_object_of_type(const object_base &o)
{
	return o.type == type;
}

}

namespace dsx {

//--------------------------------------------------------------------
static void verify_console_object()
{
	Assert(Player_num < Players.size());
	const auto &&console = vmobjptr(get_local_player().objnum);
	ConsoleObject = console;
	Assert(console->type == OBJ_PLAYER);
	Assert(get_player_id(console) == Player_num);
}

template <object_type_t type>
static unsigned count_number_of_objects_of_type(fvcobjptr &vcobjptr)
{
	return std::count_if(vcobjptr.begin(), vcobjptr.end(), is_object_of_type<type>);
}

#define count_number_of_robots	count_number_of_objects_of_type<OBJ_ROBOT>
#define count_number_of_hostages	count_number_of_objects_of_type<OBJ_HOSTAGE>

//added 10/12/95: delete buddy bot if coop game.  Probably doesn't really belong here. -MT
static void gameseq_init_network_players(object_array &objects)
{

	// Initialize network player start locations and object numbers

	ConsoleObject = &objects.front();
	unsigned j = 0, k = 0;
#if defined(DXX_BUILD_DESCENT_II)
	const auto remove_thief = Netgame.ThiefModifierFlags & ThiefModifier::Absent;
	const auto multiplayer = Game_mode & GM_MULTI;
#endif
	const auto multiplayer_coop = Game_mode & GM_MULTI_COOP;
	auto &vmobjptridx = objects.vmptridx;
	range_for (const auto &&o, vmobjptridx)
	{
		const auto type = o->type;
		if (type == OBJ_PLAYER || type == OBJ_GHOST || type == OBJ_COOP)
		{
			if (multiplayer_coop
				? (j == 0 || type == OBJ_COOP)
				: (type == OBJ_PLAYER || type == OBJ_GHOST)
			)
			{
				o->type=OBJ_PLAYER;
				Player_init[k].pos = o->pos;
				Player_init[k].orient = o->orient;
				Player_init[k].segnum = o->segnum;
				vmplayerptr(k)->objnum = o;
				set_player_id(o, k);
				k++;
			}
			else
				obj_delete(o);
			j++;
		}
#if defined(DXX_BUILD_DESCENT_II)
		else if (type == OBJ_ROBOT && multiplayer)
		{
			auto &ri = Robot_info[get_robot_id(o)];
			if (robot_is_companion(ri) || (robot_is_thief(ri) && remove_thief))
				obj_delete(o);		//kill the buddy in netgames
		}
#endif
	}
	NumNetPlayerPositions = k;
}

void gameseq_remove_unused_players()
{
	// 'Remove' the unused players

	if (Game_mode & GM_MULTI)
	{
		for (unsigned i = 0; i < NumNetPlayerPositions; ++i)
		{
			if ((!vcplayerptr(i)->connected) || (i >= N_players))
			{
				multi_make_player_ghost(i);
			}
		}
	}
	else
	{		// Note link to above if!!!
		range_for (auto &i, partial_const_range(Players, 1u, NumNetPlayerPositions))
		{
			obj_delete(vmobjptridx(i.objnum));
		}
#if defined(DXX_BUILD_DESCENT_II)
		if (PlayerCfg.ThiefModifierFlags & ThiefModifier::Absent)
		{
			range_for (const auto &&o, vmobjptridx)
			{
				const auto type = o->type;
				if (type == OBJ_ROBOT)
				{
					auto &ri = Robot_info[get_robot_id(o)];
					if (robot_is_thief(ri))
						obj_delete(o);
				}
			}
		}
#endif
	}
}

// Setup player for new game
void init_player_stats_game(const playernum_t pnum)
{
	auto &plr = *vmplayerptr(pnum);
	plr.lives = INITIAL_LIVES;
	plr.level = 1;
	plr.time_level = 0;
	plr.time_total = 0;
	plr.hours_level = 0;
	plr.hours_total = 0;
	plr.num_kills_level = 0;
	plr.num_kills_total = 0;
	plr.num_robots_level = 0;
	plr.num_robots_total = 0;
	plr.hostages_level = 0;
	plr.hostages_total = 0;
	const auto &&plobj = vmobjptr(plr.objnum);
	auto &player_info = plobj->ctype.player_info;
	player_info.powerup_flags = {};
	player_info.net_killed_total = 0;
	player_info.net_kills_total = 0;
	player_info.KillGoalCount = 0;
	player_info.mission.score = 0;
	player_info.mission.last_score = 0;
	player_info.mission.hostages_rescued_total = 0;

	init_player_stats_new_ship(pnum);
#if defined(DXX_BUILD_DESCENT_II)
	if (pnum == Player_num)
		First_secret_visit = 1;
#endif
}

static void init_ammo_and_energy(object &plrobj)
{
	auto &player_info = plrobj.ctype.player_info;
	{
		auto &energy = player_info.energy;
#if defined(DXX_BUILD_DESCENT_II)
		if (player_info.primary_weapon_flags & HAS_PRIMARY_FLAG(OMEGA_INDEX))
		{
			const auto old_omega_charge = player_info.Omega_charge;
			if (old_omega_charge < MAX_OMEGA_CHARGE)
			{
				const auto energy_used = get_omega_energy_consumption((player_info.Omega_charge = MAX_OMEGA_CHARGE) - old_omega_charge);
				energy -= energy_used;
			}
		}
#endif
		if (energy < INITIAL_ENERGY)
			energy = INITIAL_ENERGY;
	}
	{
		auto &shields = plrobj.shields;
		if (shields < StartingShields)
			shields = StartingShields;
	}
	auto &concussion = player_info.secondary_ammo[CONCUSSION_INDEX];
	if (concussion < 2 + NDL - Difficulty_level)
		concussion = 2 + NDL - Difficulty_level;
}

#if defined(DXX_BUILD_DESCENT_II)
extern	ubyte	Last_afterburner_state;
#endif

// Setup player for new level (After completion of previous level)
#if defined(DXX_BUILD_DESCENT_I)
void init_player_stats_level(player &plr, object &plrobj)
#elif defined(DXX_BUILD_DESCENT_II)
void init_player_stats_level(player &plr, object &plrobj, const secret_restore secret_flag)
#endif
{
#if defined(DXX_BUILD_DESCENT_I)
	static constexpr std::integral_constant<secret_restore, secret_restore::none> secret_flag{};
#endif

	plr.level = Current_level_num;

	if (!Network_rejoined) {
		plr.time_level = 0;
		plr.hours_level = 0;
	}

	auto &player_info = plrobj.ctype.player_info;
	player_info.mission.last_score = player_info.mission.score;

	plr.num_kills_level = 0;
	plr.num_robots_level = count_number_of_robots(vcobjptr);
	plr.num_robots_total += plr.num_robots_level;

	plr.hostages_level = count_number_of_hostages(vcobjptr);
	plr.hostages_total += plr.hostages_level;

	if (secret_flag == secret_restore::none) {
		init_ammo_and_energy(plrobj);

		auto &powerup_flags = player_info.powerup_flags;
		powerup_flags &= ~(PLAYER_FLAGS_INVULNERABLE | PLAYER_FLAGS_CLOAKED);
#if defined(DXX_BUILD_DESCENT_II)
		powerup_flags &= ~(PLAYER_FLAGS_MAP_ALL);
#endif

		DXX_MAKE_VAR_UNDEFINED(player_info.cloak_time);
		DXX_MAKE_VAR_UNDEFINED(player_info.invulnerable_time);

		const auto all_keys = PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_GOLD_KEY | PLAYER_FLAGS_RED_KEY;
		if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
			powerup_flags |= all_keys;
		else
			powerup_flags &= ~all_keys;
	}

	Player_dead_state = player_dead_state::no; // Added by RH
	Dead_player_camera = NULL;

#if defined(DXX_BUILD_DESCENT_II)
	Controls.state.afterburner = 0;
	Last_afterburner_state = 0;

	digi_kill_sound_linked_to_object(vcobjptridx(plr.objnum));
#endif
	init_gauges();
#if defined(DXX_BUILD_DESCENT_II)
	Missile_viewer = NULL;
#endif
	init_player_stats_ship(plrobj);
}

// Setup player for a brand-new ship
void init_player_stats_new_ship(const playernum_t pnum)
{
	auto &plr = *vcplayerptr(pnum);
	const auto &&plrobj = vmobjptridx(plr.objnum);
	plrobj->shields = StartingShields;
	auto &player_info = plrobj->ctype.player_info;
	player_info.energy = INITIAL_ENERGY;
	player_info.secondary_ammo = {{
		static_cast<uint8_t>(2 + NDL - Difficulty_level)
	}};
	const auto GrantedItems = (Game_mode & GM_MULTI) ? Netgame.SpawnGrantedItems : 0;
	player_info.vulcan_ammo = map_granted_flags_to_vulcan_ammo(GrantedItems);
	const auto granted_laser_level = map_granted_flags_to_laser_level(GrantedItems);
	player_info.laser_level = granted_laser_level;
	const auto granted_primary_weapon_flags = HAS_LASER_FLAG | map_granted_flags_to_primary_weapon_flags(GrantedItems);
	player_info.primary_weapon_flags = granted_primary_weapon_flags;
	player_info.powerup_flags &= ~(PLAYER_FLAGS_QUAD_LASERS | PLAYER_FLAGS_CLOAKED | PLAYER_FLAGS_INVULNERABLE);
#if defined(DXX_BUILD_DESCENT_II)
	player_info.powerup_flags &= ~(PLAYER_FLAGS_AFTERBURNER | PLAYER_FLAGS_MAP_ALL | PLAYER_FLAGS_CONVERTER | PLAYER_FLAGS_AMMO_RACK | PLAYER_FLAGS_HEADLIGHT | PLAYER_FLAGS_HEADLIGHT_ON | PLAYER_FLAGS_FLAG);
	player_info.Omega_charge = (granted_primary_weapon_flags & HAS_OMEGA_FLAG)
		? MAX_OMEGA_CHARGE
		: 0;
	player_info.Omega_recharge_delay = 0;
#endif
	player_info.powerup_flags |= map_granted_flags_to_player_flags(GrantedItems);
	DXX_MAKE_VAR_UNDEFINED(player_info.cloak_time);
	DXX_MAKE_VAR_UNDEFINED(player_info.invulnerable_time);
	if (pnum == Player_num)
	{
		if (Game_mode & GM_MULTI && Netgame.InvulAppear)
		{
			player_info.powerup_flags |= PLAYER_FLAGS_INVULNERABLE;
			player_info.invulnerable_time = GameTime64 - (i2f(58 - Netgame.InvulAppear) >> 1);
			player_info.FakingInvul = 1;
		}
		set_primary_weapon(player_info, [=]{
			range_for (auto i, PlayerCfg.PrimaryOrder)
			{
				if (i >= MAX_PRIMARY_WEAPONS)
					break;
				if (i == primary_weapon_index_t::LASER_INDEX)
					break;
#if defined(DXX_BUILD_DESCENT_II)
				if (i == primary_weapon_index_t::SUPER_LASER_INDEX)
				{
					if (granted_laser_level <= LASER_LEVEL_4)
						/* Granted lasers are not super lasers */
						continue;
					/* Super lasers still set LASER_INDEX, not
					 * SUPER_LASER_INDEX
					 */
					break;
				}
#endif
				if (HAS_PRIMARY_FLAG(i) & static_cast<unsigned>(granted_primary_weapon_flags))
					return static_cast<primary_weapon_index_t>(i);
			}
			return primary_weapon_index_t::LASER_INDEX;
		}());
#if defined(DXX_BUILD_DESCENT_II)
		auto primary_last_was_super = player_info.Primary_last_was_super;
		for (uint_fast32_t i = primary_weapon_index_t::VULCAN_INDEX, mask = 1 << i; i != primary_weapon_index_t::SUPER_LASER_INDEX; ++i, mask <<= 1)
		{
			/* If no super granted, force to non-super. */
			if (!(HAS_PRIMARY_FLAG(i + 5) & granted_primary_weapon_flags))
				primary_last_was_super &= ~mask;
			/* If only super granted, force to super. */
			else if (!(HAS_PRIMARY_FLAG(i) & granted_primary_weapon_flags))
				primary_last_was_super |= mask;
			/* else both granted, so leave as-is. */
			else
				continue;
		}
		player_info.Primary_last_was_super = primary_last_was_super;
#endif
		if (Newdemo_state == ND_STATE_RECORDING)
		{
			newdemo_record_laser_level(player_info.laser_level, 0);
		}
		set_secondary_weapon_to_concussion(player_info);
		dead_player_end(); //player no longer dead
		Player_dead_state = player_dead_state::no;
		player_info.Player_eggs_dropped = false;
		Dead_player_camera = 0;
#if defined(DXX_BUILD_DESCENT_II)
		auto &Secondary_last_was_super = player_info.Secondary_last_was_super;
		Secondary_last_was_super = {};
		Afterburner_charge = GrantedItems.has_afterburner() ? F1_0 : 0;
		Controls.state.afterburner = 0;
		Last_afterburner_state = 0;
		Missile_viewer = nullptr; //reset missile camera if out there
#endif
		init_ai_for_ship();
	}
	digi_kill_sound_linked_to_object(plrobj);
	init_player_stats_ship(plrobj);
}

void init_player_stats_ship(object &plrobj)
{
	auto &player_info = plrobj.ctype.player_info;
	player_info.lavafall_hiss_playing = false;
	player_info.missile_gun = 0;
	player_info.killer_objnum = object_none;
#if defined(DXX_BUILD_DESCENT_II)
	player_info.Omega_recharge_delay = 0;
#endif
	player_info.mission.hostages_on_board = 0;
	player_info.homing_object_dist = -F1_0; // Added by RH
	player_info.Next_flare_fire_time = player_info.Next_laser_fire_time = player_info.Next_missile_fire_time = GameTime64;
}

}

//do whatever needs to be done when a player dies in multiplayer

static void DoGameOver()
{
	if (PLAYING_BUILTIN_MISSION)
		scores_maybe_add_player();
}

//update various information about the player
void update_player_stats()
{
	auto &plr = get_local_player();
	plr.time_level += FrameTime;	//the never-ending march of time...
	if (plr.time_level > i2f(3600))
	{
		plr.time_level -= i2f(3600);
		++ plr.hours_level;
	}

	plr.time_total += FrameTime;	//the never-ending march of time...
	if (plr.time_total > i2f(3600))
	{
		plr.time_total -= i2f(3600);
		++ plr.hours_total;
	}
}

//go through this level and start any eclip sounds
namespace dsx {

static void set_sound_sources(fvcsegptridx &vcsegptridx)
{
	int sidenum;

	digi_init_sounds();		//clear old sounds
#if defined(DXX_BUILD_DESCENT_II)
	Dont_start_sound_objects = 1;
#endif

	range_for (const auto &&seg, vcsegptridx)
	{
		for (sidenum=0;sidenum<MAX_SIDES_PER_SEGMENT;sidenum++) {
			int tm,ec,sn;

#if defined(DXX_BUILD_DESCENT_I)
			if ((tm=seg->sides[sidenum].tmap_num2) != 0)
				if ((ec = TmapInfo[tm & 0x3fff].eclip_num) != eclip_none)
#elif defined(DXX_BUILD_DESCENT_II)
			const auto wid = WALL_IS_DOORWAY(GameBitmaps, Textures, vcwallptr, seg, seg, sidenum);
			if (wid & WID_RENDER_FLAG)
				if ((((tm = seg->sides[sidenum].tmap_num2) != 0) &&
					 ((ec = TmapInfo[tm & 0x3fff].eclip_num) != eclip_none)) ||
					(ec = TmapInfo[seg->sides[sidenum].tmap_num].eclip_num) != eclip_none)
#endif
					if ((sn=Effects[ec].sound_num)!=-1) {
#if defined(DXX_BUILD_DESCENT_II)
						auto csegnum = seg->children[sidenum];

						//check for sound on other side of wall.  Don't add on
						//both walls if sound travels through wall.  If sound
						//does travel through wall, add sound for lower-numbered
						//segment.

						if (IS_CHILD(csegnum) && csegnum < seg) {
							if (wid & (WID_FLY_FLAG|WID_RENDPAST_FLAG)) {
								const auto &&csegp = vcsegptr(seg->children[sidenum]);
								auto csidenum = find_connect_side(seg, csegp);

								if (csegp->sides[csidenum].tmap_num2 == seg->sides[sidenum].tmap_num2)
									continue;		//skip this one
							}
						}
#endif

						const auto &&pnt = compute_center_point_on_side(vcvertptr, seg, sidenum);
						digi_link_sound_to_pos(sn, seg, sidenum, pnt, 1, F1_0/2);
					}
		}
	}

#if defined(DXX_BUILD_DESCENT_II)
	Dont_start_sound_objects = 0;
#endif
}

constexpr fix flash_dist=fl2f(.9);

//create flash for player appearance
void create_player_appearance_effect(const object_base &player_obj)
{
	const auto pos = (&player_obj == Viewer)
		? vm_vec_scale_add(player_obj.pos, player_obj.orient.fvec, fixmul(player_obj.size, flash_dist))
		: player_obj.pos;

	const auto &&seg = vmsegptridx(player_obj.segnum);
	const auto &&effect_obj = object_create_explosion(seg, pos, player_obj.size, VCLIP_PLAYER_APPEARANCE);

	if (effect_obj) {
		effect_obj->orient = player_obj.orient;

		const auto sound_num = Vclip[VCLIP_PLAYER_APPEARANCE].sound_num;
		if (sound_num > -1)
			digi_link_sound_to_pos(sound_num, seg, 0, effect_obj->pos, 0, F1_0);
	}
}
}

//
// New Game sequencing functions
//

//get level filename. level numbers start at 1.  Secret levels are -1,-2,-3
static const d_fname &get_level_file(int level_num)
{
	if (level_num<0)                //secret level
		return Secret_level_names[-level_num-1];
	else                                    //normal level
		return Level_names[level_num-1];
}

// routine to calculate the checksum of the segments.
static void do_checksum_calc(const uint8_t *b, int len, unsigned int *s1, unsigned int *s2)
{

	while(len--) {
		*s1 += *b++;
		if (*s1 >= 255) *s1 -= 255;
		*s2 += *s1;
	}
}

namespace dsx {
static ushort netmisc_calc_checksum()
{
	unsigned int sum1,sum2;
	short s;
	int t;

	sum1 = sum2 = 0;
	range_for (auto &&segp, vcsegptr)
	{
		auto &i = *segp;
		range_for (auto &j, i.sides)
		{
			do_checksum_calc(reinterpret_cast<const uint8_t *>(&(j.get_type())), 1, &sum1, &sum2);
			s = INTEL_SHORT(j.wall_num);
			do_checksum_calc(reinterpret_cast<uint8_t *>(&s), 2, &sum1, &sum2);
			s = INTEL_SHORT(j.tmap_num);
			do_checksum_calc(reinterpret_cast<uint8_t *>(&s), 2, &sum1, &sum2);
			s = INTEL_SHORT(j.tmap_num2);
			do_checksum_calc(reinterpret_cast<uint8_t *>(&s), 2, &sum1, &sum2);
			range_for (auto &k, j.uvls)
			{
				t = INTEL_INT(k.u);
				do_checksum_calc(reinterpret_cast<uint8_t *>(&t), 4, &sum1, &sum2);
				t = INTEL_INT(k.v);
				do_checksum_calc(reinterpret_cast<uint8_t *>(&t), 4, &sum1, &sum2);
				t = INTEL_INT(k.l);
				do_checksum_calc(reinterpret_cast<uint8_t *>(&t), 4, &sum1, &sum2);
			}
			range_for (auto &k, j.normals)
			{
				t = INTEL_INT(k.x);
				do_checksum_calc(reinterpret_cast<uint8_t *>(&t), 4, &sum1, &sum2);
				t = INTEL_INT(k.y);
				do_checksum_calc(reinterpret_cast<uint8_t *>(&t), 4, &sum1, &sum2);
				t = INTEL_INT(k.z);
				do_checksum_calc(reinterpret_cast<uint8_t *>(&t), 4, &sum1, &sum2);
			}
		}
		range_for (auto &j, i.children)
		{
			s = INTEL_SHORT(j);
			do_checksum_calc(reinterpret_cast<uint8_t *>(&s), 2, &sum1, &sum2);
		}
		range_for (const uint16_t j, i.verts)
		{
			s = INTEL_SHORT(j);
			do_checksum_calc(reinterpret_cast<uint8_t *>(&s), 2, &sum1, &sum2);
		}
		s = INTEL_SHORT(i.objects);
		do_checksum_calc(reinterpret_cast<uint8_t *>(&s), 2, &sum1, &sum2);
#if defined(DXX_BUILD_DESCENT_I)
		do_checksum_calc(&i.special, 1, &sum1, &sum2);
		do_checksum_calc(reinterpret_cast<const uint8_t *>(&i.matcen_num), 1, &sum1, &sum2);
		t = INTEL_INT(i.static_light);
		do_checksum_calc(reinterpret_cast<uint8_t *>(&t), 4, &sum1, &sum2);
#endif
		do_checksum_calc(&i.station_idx, 1, &sum1, &sum2);
	}
	sum2 %= 255;
	return ((sum1<<8)+ sum2);
}
}

#if defined(DXX_BUILD_DESCENT_II)
namespace dsx {
// load just the hxm file
void load_level_robots(int level_num)
{
	Assert(level_num <= Last_level  && level_num >= Last_secret_level  && level_num != 0);
	const d_fname &level_name = get_level_file(level_num);
	if (Robot_replacements_loaded) {
		free_polygon_models();
		load_mission_ham();
		Robot_replacements_loaded = 0;
	}
	load_robot_replacements(level_name);
}
}
#endif

//load a level off disk. level numbers start at 1.  Secret levels are -1,-2,-3
namespace dsx {
void LoadLevel(int level_num,int page_in_textures)
{
	preserve_player_object_info p(vcplayerptr(Player_num)->objnum);

	auto &plr = get_local_player();
	auto save_player = plr;

	Assert(level_num <= Last_level  && level_num >= Last_secret_level  && level_num != 0);
	const d_fname &level_name = get_level_file(level_num);
#if defined(DXX_BUILD_DESCENT_I)
	if (!load_level(level_name))
		Current_level_num=level_num;

	gr_use_palette_table( "palette.256" );
#elif defined(DXX_BUILD_DESCENT_II)
	gr_set_default_canvas();
	gr_clear_canvas(*grd_curcanv, BM_XRGB(0, 0, 0));		//so palette switching is less obvious

	load_level_robots(level_num);

	int load_ret = load_level(level_name);		//actually load the data from disk!

	if (load_ret)
		Error("Could not load level file <%s>, error = %d",static_cast<const char *>(level_name),load_ret);

	Current_level_num=level_num;

	load_palette(Current_level_palette,1,1);		//don't change screen
#endif

#if DXX_USE_EDITOR
	if (!EditorWindow)
#endif
		show_boxed_message(TXT_LOADING, 0);
#ifdef RELEASE
	timer_delay(F1_0);
#endif

	load_endlevel_data(level_num);
#if defined(DXX_BUILD_DESCENT_I)
	load_custom_data(level_name);
#elif defined(DXX_BUILD_DESCENT_II)
	if (EMULATING_D1)
		load_d1_bitmap_replacements();
	else
		load_bitmap_replacements(level_name);

	if ( page_in_textures )
		piggy_load_level_data();
#endif

	my_segments_checksum = netmisc_calc_checksum();

	reset_network_objects();

	plr = save_player;

	set_sound_sources(vcsegptridx);

#if DXX_USE_EDITOR
	if (!EditorWindow)
#endif
		songs_play_level_song( Current_level_num, 0 );

	gr_palette_load(gr_palette);		//actually load the palette
#if defined(DXX_BUILD_DESCENT_I)
	if ( page_in_textures )
		piggy_load_level_data();
#endif

	gameseq_init_network_players(Objects);
	p.restore();
}
}

//sets up Player_num & ConsoleObject
void InitPlayerObject()
{
	Assert(Player_num<MAX_PLAYERS);

	if (Player_num != 0 )	{
		Players[0u] = get_local_player();
		Player_num = 0;
	}

	auto &plr = get_local_player();
	plr.objnum = object_first;
	const auto &&console = vmobjptr(plr.objnum);
	ConsoleObject = console;
	console->type				= OBJ_PLAYER;
	set_player_id(console, Player_num);
	console->control_type	= CT_FLYING;
	console->movement_type	= MT_PHYSICS;
}

//starts a new game on the given level
namespace dsx {
void StartNewGame(int start_level)
{
	state_quick_item = -1;	// for first blind save, pick slot to save in

	Game_mode = GM_NORMAL;

	Next_level_num = 0;

	InitPlayerObject();				//make sure player's object set up

	init_player_stats_game(Player_num);		//clear all stats

	N_players = 1;

#if defined(DXX_BUILD_DESCENT_II)
	if (start_level < 0)
		StartNewLevelSecret(start_level, 0);
	else
#endif
		StartNewLevel(start_level);

	get_local_player().starting_level = start_level;		// Mark where they started

	game_disable_cheats();
#if defined(DXX_BUILD_DESCENT_II)
	init_seismic_disturbances();
#endif
}

//	-----------------------------------------------------------------------------
//	Does the bonus scoring.
//	Call with dead_flag = 1 if player died, but deserves some portion of bonus (only skill points), anyway.
static void DoEndLevelScoreGlitz()
{
	int level_points, skill_points, energy_points, shield_points, hostage_points;
	int	all_hostage_points;
	int	endgame_points;
	char	all_hostage_text[64];
	char	endgame_text[64];
	#define N_GLITZITEMS 9
	char				m_str[N_GLITZITEMS][30];
	newmenu_item	m[N_GLITZITEMS];
	int				i,c;
	char				title[128];
	int				is_last_level;
#if defined(DXX_BUILD_DESCENT_I)
	gr_palette_load( gr_palette );
#elif defined(DXX_BUILD_DESCENT_II)
	int				mine_level;

	//	Compute level player is on, deal with secret levels (negative numbers)
	mine_level = get_local_player().level;
	if (mine_level < 0)
		mine_level *= -(Last_level/N_secret_levels);
#endif

	auto &plrobj = get_local_plrobj();
	auto &player_info = plrobj.ctype.player_info;
	level_points = player_info.mission.score - player_info.mission.last_score;

	if (!cheats.enabled) {
		if (Difficulty_level > 1) {
#if defined(DXX_BUILD_DESCENT_I)
			skill_points = level_points*(Difficulty_level-1)/2;
#elif defined(DXX_BUILD_DESCENT_II)
			skill_points = level_points*(Difficulty_level)/4;
#endif
			skill_points -= skill_points % 100;
		} else
			skill_points = 0;

		hostage_points = player_info.mission.hostages_on_board * 500 * (Difficulty_level+1);
#if defined(DXX_BUILD_DESCENT_I)
		shield_points = f2i(plrobj.shields) * 10 * (Difficulty_level+1);
		energy_points = f2i(player_info.energy) * 5 * (Difficulty_level+1);
#elif defined(DXX_BUILD_DESCENT_II)
		shield_points = f2i(plrobj.shields) * 5 * mine_level;
		energy_points = f2i(player_info.energy) * 2 * mine_level;

		shield_points -= shield_points % 50;
		energy_points -= energy_points % 50;
#endif
	} else {
		skill_points = 0;
		shield_points = 0;
		energy_points = 0;
		hostage_points = 0;
	}

	all_hostage_text[0] = 0;
	endgame_text[0] = 0;

	auto &plr = get_local_player();
	if (!cheats.enabled && player_info.mission.hostages_on_board == plr.hostages_level)
	{
		all_hostage_points = player_info.mission.hostages_on_board * 1000 * (Difficulty_level+1);
		snprintf(all_hostage_text, sizeof(all_hostage_text), "%s%i\n", TXT_FULL_RESCUE_BONUS, all_hostage_points);
	} else
		all_hostage_points = 0;

	if (!cheats.enabled && !(Game_mode & GM_MULTI) && plr.lives && Current_level_num == Last_level)
	{		//player has finished the game!
		endgame_points = plr.lives * 10000;
		snprintf(endgame_text, sizeof(endgame_text), "%s%i\n", TXT_SHIP_BONUS, endgame_points);
		is_last_level=1;
	} else
		endgame_points = is_last_level = 0;

	add_bonus_points_to_score(player_info, skill_points + energy_points + shield_points + hostage_points + all_hostage_points + endgame_points);

	c = 0;
	snprintf(m_str[c++], sizeof(m_str[0]), "%s%i", TXT_SHIELD_BONUS, shield_points);		// Return at start to lower menu...
	snprintf(m_str[c++], sizeof(m_str[0]), "%s%i", TXT_ENERGY_BONUS, energy_points);
	snprintf(m_str[c++], sizeof(m_str[0]), "%s%i", TXT_HOSTAGE_BONUS, hostage_points);
	snprintf(m_str[c++], sizeof(m_str[0]), "%s%i", TXT_SKILL_BONUS, skill_points);

	snprintf(m_str[c++], sizeof(m_str[0]), "%s", all_hostage_text);
	if (!(Game_mode & GM_MULTI) && plr.lives && Current_level_num == Last_level)
		snprintf(m_str[c++], sizeof(m_str[0]), "%s", endgame_text);

	snprintf(m_str[c++], sizeof(m_str[0]), "%s%i\n", TXT_TOTAL_BONUS, shield_points + energy_points + hostage_points + skill_points + all_hostage_points + endgame_points);
	snprintf(m_str[c++], sizeof(m_str[0]), "%s%i", TXT_TOTAL_SCORE, player_info.mission.score);

	for (i=0; i<c; i++) {
		nm_set_item_text(m[i], m_str[i]);
	}

	auto current_level_num = Current_level_num;
	const auto txt_level = (current_level_num < 0) ? (current_level_num = -current_level_num, TXT_SECRET_LEVEL) : TXT_LEVEL;
	snprintf(title, sizeof(title), "%s%s %d %s\n%s %s", is_last_level?"\n\n\n":"\n", txt_level, current_level_num, TXT_COMPLETE, static_cast<const char *>(Current_level_name), TXT_DESTROYED);

	Assert(c <= N_GLITZITEMS);

	newmenu_do2(nullptr, title, c, m, unused_newmenu_subfunction, unused_newmenu_userdata, 0, GLITZ_BACKGROUND);
}
}

#if defined(DXX_BUILD_DESCENT_II)
//	-----------------------------------------------------------------------------------------------------
//called when the player is starting a level (new game or new ship)
static void StartSecretLevel()
{
	Assert(Player_dead_state == player_dead_state::no);

	InitPlayerPosition(vmobjptridx, vmsegptridx, 0);

	verify_console_object();

	ConsoleObject->control_type	= CT_FLYING;
	ConsoleObject->movement_type	= MT_PHYSICS;

	// -- WHY? -- disable_matcens();
	clear_transient_objects(0);		//0 means leave proximity bombs

	// create_player_appearance_effect(ConsoleObject);
	Do_appearance_effect = 1;

	ai_reset_all_paths();
	// -- NO? -- reset_time();

	reset_rear_view();
	auto &player_info = ConsoleObject->ctype.player_info;
	player_info.Auto_fire_fusion_cannon_time = 0;
	player_info.Fusion_charge = 0;
}

//	Returns true if secret level has been destroyed.
int p_secret_level_destroyed(void)
{
	if (First_secret_visit) {
		return 0;		//	Never been there, can't have been destroyed.
	} else {
		if (PHYSFSX_exists(SECRETC_FILENAME,0))
		{
			return 0;
		} else {
			return 1;
		}
	}
}

//	-----------------------------------------------------------------------------------------------------
#define TXT_SECRET_RETURN  "Returning to level %i", Entered_from_level
#define TXT_SECRET_ADVANCE "Base level destroyed.\nAdvancing to level %i", Entered_from_level+1
#endif

static int draw_endlevel_background(newmenu *,const d_event &event, grs_bitmap *background)
{
	switch (event.type)
	{
		case EVENT_WINDOW_DRAW:
			gr_set_default_canvas();
			show_fullscr(*grd_curcanv, *background);
			break;
			
		default:
			break;
	}
	
	return 0;
}

static void do_screen_message(const char *msg) __attribute_nonnull();
static void do_screen_message(const char *msg)
{
	
	if (Game_mode & GM_MULTI)
		return;
	grs_main_bitmap background;
	if (pcx_read_bitmap(GLITZ_BACKGROUND, background, gr_palette) != PCX_ERROR_NONE)
		return;

	gr_palette_load(gr_palette);
	array<newmenu_item, 1> nm_message_items{{
		nm_item_menu(TXT_OK),
	}};
	newmenu_do( NULL, msg, nm_message_items, draw_endlevel_background, static_cast<grs_bitmap *>(&background));
}

#if defined(DXX_BUILD_DESCENT_II)
namespace dsx {
static void do_screen_message_fmt(const char *fmt, ...) __attribute_format_printf(1, 2);
static void do_screen_message_fmt(const char *fmt, ...)
{
	va_list arglist;
	char msg[1024];
	va_start(arglist, fmt);
	vsnprintf(msg, sizeof(msg), fmt, arglist);
	va_end(arglist);
	do_screen_message(msg);
}
#define do_screen_message(F,...)	dxx_call_printf_checked(do_screen_message_fmt,do_screen_message,(),F,##__VA_ARGS__)

//	-----------------------------------------------------------------------------------------------------
// called when the player is starting a new level for normal game mode and restore state
//	Need to deal with whether this is the first time coming to this level or not.  If not the
//	first time, instead of initializing various things, need to do a game restore for all the
//	robots, powerups, walls, doors, etc.
static void StartNewLevelSecret(int level_num, int page_in_textures)
{
        ThisLevelTime=0;

	last_drawn_cockpit = -1;

	if (Newdemo_state == ND_STATE_PAUSED)
		Newdemo_state = ND_STATE_RECORDING;

	if (Newdemo_state == ND_STATE_RECORDING) {
		newdemo_set_new_level(level_num);
		newdemo_record_start_frame(FrameTime );
	} else if (Newdemo_state != ND_STATE_PLAYBACK) {

		set_screen_mode(SCREEN_MENU);

		if (First_secret_visit) {
			do_screen_message(TXT_SECRET_EXIT);
		} else {
			if (PHYSFSX_exists(SECRETC_FILENAME,0))
			{
				do_screen_message(TXT_SECRET_EXIT);
			} else {
				do_screen_message("Secret level already destroyed.\nAdvancing to level %i.", Current_level_num+1);
			}
		}
	}

	LoadLevel(level_num,page_in_textures);

	Assert(Current_level_num == level_num); // make sure level set right

	HUD_clear_messages();

	automap_clear_visited();

	Viewer = &get_local_plrobj();

	gameseq_remove_unused_players();

	Game_suspended = 0;

	Control_center_destroyed = 0;

	init_cockpit();
	reset_palette_add();

	if (First_secret_visit || (Newdemo_state == ND_STATE_PLAYBACK)) {
		init_robots_for_level();
		init_ai_objects();
		init_smega_detonates();
		init_morphs();
		init_all_matcens();
		reset_special_effects();
		StartSecretLevel();
	} else {
		if (PHYSFSX_exists(SECRETC_FILENAME,0))
		{
			auto &player_info = get_local_plrobj().ctype.player_info;
			const auto pw_save = player_info.Primary_weapon;
			const auto sw_save = player_info.Secondary_weapon;
			state_restore_all(1, secret_restore::survived, SECRETC_FILENAME, blind_save::no);
			player_info.Primary_weapon = pw_save;
			player_info.Secondary_weapon = sw_save;
			reset_special_effects();
			StartSecretLevel();
			// -- No: This is only for returning to base level: set_pos_from_return_segment();
		} else {
			do_screen_message("Secret level already destroyed.\nAdvancing to level %i.", Current_level_num+1);
			return;
		}
	}

	if (First_secret_visit) {
		copy_defaults_to_robot_all();
	}

	init_controlcen_for_level();

	// Say player can use FLASH cheat to mark path to exit.
	Last_level_path_created = -1;

	First_secret_visit = 0;
}

static int Entered_from_level;

// ---------------------------------------------------------------------------------------------------------------
//	Called from switch.c when player is on a secret level and hits exit to return to base level.
window_event_result ExitSecretLevel()
{
	auto result = window_event_result::handled;

	if (Newdemo_state == ND_STATE_PLAYBACK)
		return window_event_result::ignored;

	if (Game_wind)
		window_set_visible(Game_wind, 0);

	if (!Control_center_destroyed) {
		state_save_all(secret_save::c, blind_save::no);
	}

	if (PHYSFSX_exists(SECRETB_FILENAME,0))
	{
		do_screen_message(TXT_SECRET_RETURN);
		auto &player_info = get_local_plrobj().ctype.player_info;
		const auto pw_save = player_info.Primary_weapon;
		const auto sw_save = player_info.Secondary_weapon;
		state_restore_all(1, secret_restore::survived, SECRETB_FILENAME, blind_save::no);
		player_info.Primary_weapon = pw_save;
		player_info.Secondary_weapon = sw_save;
	} else {
		// File doesn't exist, so can't return to base level.  Advance to next one.
		if (Entered_from_level == Last_level)
		{
			DoEndGame();
			result = window_event_result::close;
		}
		else {
			do_screen_message(TXT_SECRET_ADVANCE);
			StartNewLevel(Entered_from_level+1);
		}
	}

	if (Game_wind)
		window_set_visible(Game_wind, 1);
	reset_time();

	return result;
}

// ---------------------------------------------------------------------------------------------------------------
//	Set invulnerable_time and cloak_time in player struct to preserve amount of time left to
//	be invulnerable or cloaked.
void do_cloak_invul_secret_stuff(fix64 old_gametime, player_info &player_info)
{
	auto &pl_flags = player_info.powerup_flags;
	if (pl_flags & PLAYER_FLAGS_INVULNERABLE)
	{
		fix64	time_used;

		auto &t = player_info.invulnerable_time;
		time_used = old_gametime - t;
		t = GameTime64 - time_used;
	}

	if (pl_flags & PLAYER_FLAGS_CLOAKED)
	{
		fix	time_used;

		auto &t = player_info.cloak_time;
		time_used = old_gametime - t;
		t = GameTime64 - time_used;
	}
}

// ---------------------------------------------------------------------------------------------------------------
//	Called from switch.c when player passes through secret exit.  That means he was on a non-secret level and he
//	is passing to the secret level.
//	Do a savegame.
void EnterSecretLevel(void)
{
	int i;

	Assert(! (Game_mode & GM_MULTI) );

	if (Game_wind)
		window_set_visible(Game_wind, 0);

	digi_play_sample( SOUND_SECRET_EXIT, F1_0 );	// after above call which stops all sounds
	
	Entered_from_level = Current_level_num;

	if (Control_center_destroyed)
		DoEndLevelScoreGlitz();

	if (Newdemo_state != ND_STATE_PLAYBACK)
		state_save_all(secret_save::b, blind_save::no);	//	Not between levels (ie, save all), IS a secret level, NO filename override

	//	Find secret level number to go to, stuff in Next_level_num.
	for (i=0; i<-Last_secret_level; i++)
		if (Secret_level_table[i]==Current_level_num) {
			Next_level_num = -(i+1);
			break;
		} else if (Secret_level_table[i] > Current_level_num) {	//	Allows multiple exits in same group.
			Next_level_num = -i;
			break;
		}

	if (! (i<-Last_secret_level))		//didn't find level, so must be last
		Next_level_num = Last_secret_level;

	// NMN 04/09/07  Do a REAL start level routine if we are playing a D1 level so we have
	//               briefings
	if (EMULATING_D1)
	{
		set_screen_mode(SCREEN_MENU);
		do_screen_message("Alternate Exit Found!\n\nProceeding to Secret Level!");
		StartNewLevel(Next_level_num);
	} else {
 	   	StartNewLevelSecret(Next_level_num, 1);
	}
	// END NMN

	// do_cloak_invul_stuff();
	if (Game_wind)
		window_set_visible(Game_wind, 1);
	reset_time();
}
}
#endif

//called when the player has finished a level
namespace dsx {
window_event_result PlayerFinishedLevel(int secret_flag)
{
	if (Game_wind)
		window_set_visible(Game_wind, 0);

	//credit the player for hostages
	auto &player_info = get_local_plrobj().ctype.player_info;
	player_info.mission.hostages_rescued_total += player_info.mission.hostages_on_board;
#if defined(DXX_BUILD_DESCENT_I)
	if (!(Game_mode & GM_MULTI) && (secret_flag)) {
		array<newmenu_item, 1> m{{
			nm_item_text(" "),			//TXT_SECRET_EXIT;
		}};
		newmenu_do2(NULL, TXT_SECRET_EXIT, m.size(), m.data(), unused_newmenu_subfunction, unused_newmenu_userdata, 0, Menu_pcx_name);
	}
#elif defined(DXX_BUILD_DESCENT_II)
	Assert(!secret_flag);
#endif
	if (Game_mode & GM_NETWORK)
		get_local_player().connected = CONNECT_WAITING; // Finished but did not die

	last_drawn_cockpit = -1;

	auto result = AdvanceLevel(secret_flag);				//now go on to the next one (if one)

	if (Game_wind)
		window_set_visible(Game_wind, 1);
	reset_time();

	return result;
}
}

#if defined(DXX_BUILD_DESCENT_II)
#define MOVIE_REQUIRED 1
#define ENDMOVIE "end"
#endif

namespace dsx {

//called when the player has finished the last level
static void DoEndGame()
{
	if ((Newdemo_state == ND_STATE_RECORDING) || (Newdemo_state == ND_STATE_PAUSED))
		newdemo_stop_recording();

	set_screen_mode( SCREEN_MENU );

	gr_set_default_canvas();

	key_flush();

	if (PLAYING_BUILTIN_MISSION && !(Game_mode & GM_MULTI))
	{ //only built-in mission, & not multi
#if defined(DXX_BUILD_DESCENT_II)
		int played=MOVIE_NOT_PLAYED;	//default is not played

		played = PlayMovie(ENDMOVIE ".tex", ENDMOVIE ".mve",MOVIE_REQUIRED);
		if (!played)
#endif
		{
			do_end_briefing_screens(Ending_text_filename);
		}
        }
        else if (!(Game_mode & GM_MULTI))    //not multi
        {
		char tname[FILENAME_LEN];

		do_end_briefing_screens (Ending_text_filename);

		//try doing special credits
		snprintf(tname, sizeof(tname), "%s.ctb", Current_mission_filename);
		credits_show(tname);
	}

	key_flush();

	if (Game_mode & GM_MULTI)
		multi_endlevel_score();
	else
		// NOTE LINK TO ABOVE
		DoEndLevelScoreGlitz();

	if (PLAYING_BUILTIN_MISSION && !((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))) {
		gr_set_default_canvas();
		gr_clear_canvas(*grd_curcanv, BM_XRGB(0,0,0));
#if defined(DXX_BUILD_DESCENT_II)
		load_palette(D2_DEFAULT_PALETTE,0,1);
#endif
		scores_maybe_add_player();
	}
}

//called to go to the next level (if there is one)
//if secret_flag is true, advance to secret level, else next normal one
//	Return true if game over.
static window_event_result AdvanceLevel(int secret_flag)
{
	auto rval = window_event_result::handled;

#if defined(DXX_BUILD_DESCENT_II)
	Assert(!secret_flag);

	// Loading a level can write over homing_flag.
	// So for simplicity, reset the homing weapon cheat here.
	if (cheats.homingfire)
	{
		cheats.homingfire = 0;
		weapons_homing_all_reset();
	}
#endif
	if (Current_level_num != Last_level)
	{
		if (Game_mode & GM_MULTI)
		{
			const auto result = multi_endlevel_score();
			if (result == kmatrix_result::abort)
				return window_event_result::close;	// Exit out of game loop
		}
		else
			// NOTE LINK TO ABOVE!!!
			DoEndLevelScoreGlitz();		//give bonuses
	}

	Control_center_destroyed = 0;

	if (Game_mode & GM_MULTI)
	{
		int result;
		result = multi_endlevel(&secret_flag); // Wait for other players to reach this point
		if (result) // failed to sync
		{
			// check if player has finished the game
			return Current_level_num == Last_level ? window_event_result::close : rval;
		}
	}

	if (Current_level_num == Last_level) {		//player has finished the game!

		DoEndGame();
		rval = window_event_result::close;

	} else {
#if defined(DXX_BUILD_DESCENT_I)
		Next_level_num = Current_level_num+1;		//assume go to next normal level

		if (secret_flag) {			//go to secret level instead
			int i;

			for (i=0;i<-Last_secret_level;i++)
				if (Secret_level_table[i]==Current_level_num) {
					Next_level_num = -(i+1);
					break;
				}
			Assert(i<-Last_secret_level);		//couldn't find which secret level
		}

		if (Current_level_num < 0)	{			//on secret level, where to go?

			Assert(!secret_flag);				//shouldn't be going to secret level
			Assert(Current_level_num<=-1 && Current_level_num>=Last_secret_level);

			Next_level_num = Secret_level_table[(-Current_level_num)-1]+1;
		}
#elif defined(DXX_BUILD_DESCENT_II)

		//NMN 04/08/07 If we are in a secret level and playing a D1
		// 	       level, then use Entered_from_level # instead
		if (Current_level_num < 0 && EMULATING_D1)
		{
		  Next_level_num = Entered_from_level+1;		//assume go to next normal level
		} else {
		  Next_level_num = Current_level_num+1;		//assume go to next normal level
                }
		// END NMN
#endif
		rval = std::max(StartNewLevel(Next_level_num), rval);
	}

	return rval;
}

window_event_result DoPlayerDead()
{
	const bool pause = !(((Game_mode & GM_MULTI) && (Newdemo_state != ND_STATE_PLAYBACK)) && (!Endlevel_sequence));
	auto result = window_event_result::handled;

	if (pause)
		stop_time();

	reset_palette_add();

	gr_palette_load (gr_palette);

	dead_player_end();		//terminate death sequence (if playing)

	auto &plr = get_local_player();
	if ( Game_mode&GM_MULTI )
	{
		multi_do_death(plr.objnum);
	}
	else
	{				//Note link to above else!
		-- plr.lives;
		if (plr.lives == 0)
		{
			DoGameOver();
			if (pause)
				start_time();
			return window_event_result::close;
		}
	}

	if ( Control_center_destroyed ) {

		//clear out stuff so no bonus
		auto &plrobj = get_local_plrobj();
		auto &player_info = plrobj.ctype.player_info;
		player_info.mission.hostages_on_board = 0;
		player_info.energy = 0;
		plrobj.shields = 0;
		plr.connected = CONNECT_DIED_IN_MINE;

		do_screen_message(TXT_DIED_IN_MINE); // Give them some indication of what happened
#if defined(DXX_BUILD_DESCENT_II)
		if (Current_level_num < 0) {
			if (PHYSFSX_exists(SECRETB_FILENAME,0))
			{
				do_screen_message(TXT_SECRET_RETURN);
				state_restore_all(1, secret_restore::died, SECRETB_FILENAME, blind_save::no);			//	2 means you died
				set_pos_from_return_segment();
				plr.lives--;						//	re-lose the life, get_local_player().lives got written over in restore.
			} else {
				if (Entered_from_level == Last_level)
				{
					DoEndGame();
					result = window_event_result::close;
				}
				else {
					do_screen_message(TXT_SECRET_ADVANCE);
					StartNewLevel(Entered_from_level+1);
					init_player_stats_new_ship(Player_num);	//	New, MK, 05/29/96!, fix bug with dying in secret level, advance to next level, keep powerups!
				}
			}
		} else 
#endif
                {

			result = AdvanceLevel(0);			//if finished, go on to next level

			init_player_stats_new_ship(Player_num);
			last_drawn_cockpit = -1;
		}
#if defined(DXX_BUILD_DESCENT_II)
	} else if (Current_level_num < 0) {
		if (PHYSFSX_exists(SECRETB_FILENAME,0))
		{
			do_screen_message(TXT_SECRET_RETURN);
			if (!Control_center_destroyed)
				state_save_all(secret_save::c, blind_save::no);
			state_restore_all(1, secret_restore::died, SECRETB_FILENAME, blind_save::no);
			set_pos_from_return_segment();
			plr.lives--;						//	re-lose the life, get_local_player().lives got written over in restore.
		} else {
			do_screen_message(TXT_DIED_IN_MINE); // Give them some indication of what happened
			if (Entered_from_level == Last_level)
			{
				DoEndGame();
				result = window_event_result::close;
			}
			else {
				do_screen_message(TXT_SECRET_ADVANCE);
				StartNewLevel(Entered_from_level+1);
				init_player_stats_new_ship(Player_num);	//	New, MK, 05/29/96!, fix bug with dying in secret level, advance to next level, keep powerups!
			}
		}
#endif
	} else {
		init_player_stats_new_ship(Player_num);
		StartLevel(1);
	}

	digi_sync_sounds();

	if (pause)
		start_time();
	reset_time();

	return result;
}

//called when the player is starting a new level for normal game mode and restore state
//	secret_flag set if came from a secret level
#if defined(DXX_BUILD_DESCENT_I)
window_event_result StartNewLevelSub(const int level_num, const int page_in_textures)
#elif defined(DXX_BUILD_DESCENT_II)
window_event_result StartNewLevelSub(const int level_num, const int page_in_textures, const secret_restore secret_flag)
#endif
{
	if (!(Game_mode & GM_MULTI)) {
		last_drawn_cockpit = -1;
	}
#if defined(DXX_BUILD_DESCENT_I)
	static constexpr std::integral_constant<secret_restore, secret_restore::none> secret_flag{};
#elif defined(DXX_BUILD_DESCENT_II)
        BigWindowSwitch=0;
#endif


	if (Newdemo_state == ND_STATE_PAUSED)
		Newdemo_state = ND_STATE_RECORDING;

	if (Newdemo_state == ND_STATE_RECORDING) {
		newdemo_set_new_level(level_num);
		newdemo_record_start_frame(FrameTime );
	}

	LoadLevel(level_num,page_in_textures);

	Assert(Current_level_num == level_num);	//make sure level set right

	Viewer = &get_local_plrobj();

	Assert(N_players <= NumNetPlayerPositions);
		//If this assert fails, there's not enough start positions

	if (Game_mode & GM_NETWORK)
	{
		multi_prep_level_objects();
		if (multi_level_sync() == window_event_result::close) // After calling this, Player_num is set
		{
			songs_play_song( SONG_TITLE, 1 ); // level song already plays but we fail to start level...
			return window_event_result::close;
		}
	}

	HUD_clear_messages();

	automap_clear_visited();

	init_player_stats_level(get_local_player(), get_local_plrobj(), secret_flag);

#if defined(DXX_BUILD_DESCENT_I)
	gr_use_palette_table( "palette.256" );
#elif defined(DXX_BUILD_DESCENT_II)
	load_palette(Current_level_palette,0,1);
#endif
	gr_palette_load(gr_palette);

	if ((Game_mode & GM_MULTI_COOP) && Network_rejoined)
	{
		for (playernum_t i = 0; i < N_players; ++i)
		{
			const auto &&plobj = vmobjptr(vcplayerptr(i)->objnum);
			plobj->ctype.player_info.powerup_flags |= Netgame.net_player_flags[i];
		}
	}

	if (Game_mode & GM_MULTI)
	{
		multi_prep_level_player();
	}

	gameseq_remove_unused_players();

	Game_suspended = 0;

	Control_center_destroyed = 0;

#if defined(DXX_BUILD_DESCENT_II)
	set_screen_mode(SCREEN_GAME);
#endif

	init_cockpit();
	init_robots_for_level();
	init_ai_objects();
	init_morphs();
	init_all_matcens();
	reset_palette_add();
	LevelUniqueStuckObjectState.init_stuck_objects();
#if defined(DXX_BUILD_DESCENT_II)
	init_smega_detonates();
	init_thief_for_level();
	if (!(Game_mode & GM_MULTI))
		filter_objects_from_level(vmobjptr);
#endif

	if (!(Game_mode & GM_MULTI) && !cheats.enabled)
		set_highest_level(Current_level_num);
	else
		read_player_file();		//get window sizes

	reset_special_effects();

#if DXX_USE_OGL
	ogl_cache_level_textures();
#endif


	if (Network_rejoined == 1)
	{
		Network_rejoined = 0;
		StartLevel(1);
	}
	else
		StartLevel(0);		// Note link to above if!

	copy_defaults_to_robot_all();
	init_controlcen_for_level();

	//	Say player can use FLASH cheat to mark path to exit.
	Last_level_path_created = -1;

	// Initialise for palette_restore()
	// Also takes care of nm_draw_background() possibly being called
	if (!((Game_mode & GM_MULTI) && (Newdemo_state != ND_STATE_PLAYBACK)))
		full_palette_save();

	if (!Game_wind)
		game();

	return window_event_result::handled;
}

}

void (bash_to_shield)(const vmobjptr_t i)
{
	set_powerup_id(i, POW_SHIELD_BOOST);
}


#if defined(DXX_BUILD_DESCENT_II)
namespace dsx {

static void filter_objects_from_level(fvmobjptr &vmobjptr)
 {
	range_for (const auto &&objp, vmobjptr)
	{
		if (objp->type==OBJ_POWERUP)
		{
			const auto powerup_id = get_powerup_id(objp);
			if (powerup_id == POW_FLAG_RED || powerup_id == POW_FLAG_BLUE)
				bash_to_shield(objp);
		}
   }

 }

struct intro_movie_t {
	int	level_num;
	char	movie_name[4];
};

const array<intro_movie_t, 7> intro_movie{{
	{ 1, "PLA"},
	{ 5, "PLB"},
	{ 9, "PLC"},
	{13, "PLD"},
	{17, "PLE"},
	{21, "PLF"},
	{24, "PLG"}
}};

static void ShowLevelIntro(int level_num)
{
	//if shareware, show a briefing?

	if (!(Game_mode & GM_MULTI)) {
		palette_array_t save_pal;

		save_pal = gr_palette;

		if (PLAYING_BUILTIN_MISSION) {

			if (is_SHAREWARE || is_MAC_SHARE)
			{
				if (level_num==1)
					do_briefing_screens (Briefing_text_filename, 1);
			}
			else if (is_D2_OEM)
			{
				if (level_num == 1 && !intro_played)
					do_briefing_screens(Briefing_text_filename, 1);
			}
			else // full version
			{
				range_for (auto &i, intro_movie)
				{
					if (i.level_num == level_num)
					{
						Screen_mode = -1;
						PlayMovie(NULL, i.movie_name, MOVIE_REQUIRED);
						break;
					}
				}

				do_briefing_screens (Briefing_text_filename,level_num);
			}
		}
		else	//not the built-in mission (maybe d1, too).  check for add-on briefing
		{
			do_briefing_screens(Briefing_text_filename, level_num);
		}

		gr_palette = save_pal;
	}
}

//	---------------------------------------------------------------------------
//	If starting a level which appears in the Secret_level_table, then set First_secret_visit.
//	Reason: On this level, if player goes to a secret level, he will be going to a different
//	secret level than he's ever been to before.
//	Sets the global First_secret_visit if necessary.  Otherwise leaves it unchanged.
static void maybe_set_first_secret_visit(int level_num)
{
	range_for (auto &i, unchecked_partial_range(Secret_level_table.get(), N_secret_levels))
	{
		if (i == level_num)
		{
			First_secret_visit = 1;
			break;
		}
	}
}
}
#endif

//called when the player is starting a new level for normal game model
//	secret_flag if came from a secret level
namespace dsx {
window_event_result StartNewLevel(int level_num)
{
	hide_menus();

	GameTime64 = 0;
	ThisLevelTime=0;

#if defined(DXX_BUILD_DESCENT_I)
	if (!(Game_mode & GM_MULTI)) {
		do_briefing_screens(Briefing_text_filename, level_num);
	}
#elif defined(DXX_BUILD_DESCENT_II)
	if (level_num > 0) {
		maybe_set_first_secret_visit(level_num);
	}

	ShowLevelIntro(level_num);
#endif

	return StartNewLevelSub(level_num, 1, secret_restore::none);

}
}

namespace
{

class respawn_locations
{
	typedef std::pair<int, fix> site;
	unsigned max_usable_spawn_sites;
	array<site, MAX_PLAYERS> sites;
public:
	respawn_locations(fvcsegptridx &vcsegptridx)
	{
		const auto player_num = Player_num;
		const auto find_closest_player = [player_num, &vcsegptridx](const obj_position &candidate) {
			fix closest_dist = INT32_MAX;
			const auto &&candidate_segp = vcsegptridx(candidate.segnum);
			for (playernum_t i = N_players; i--;)
			{
				if (i == player_num)
					continue;
				const auto &&objp = vmobjptr(vcplayerptr(i)->objnum);
				if (objp->type != OBJ_PLAYER)
					continue;
				const auto dist = find_connected_distance(objp->pos, candidate_segp.absolute_sibling(objp->segnum), candidate.pos, candidate_segp, -1, WALL_IS_DOORWAY_FLAG<0>());
				if (dist >= 0 && closest_dist > dist)
					closest_dist = dist;
			}
			return closest_dist;
		};
		const auto max_spawn_sites = std::min<unsigned>(NumNetPlayerPositions, sites.size());
		for (uint_fast32_t i = max_spawn_sites; i--;)
		{
			auto &s = sites[i];
			s.first = i;
			s.second = find_closest_player(Player_init[i]);
		}
		const unsigned SecludedSpawns = Netgame.SecludedSpawns + 1;
		if (max_spawn_sites > SecludedSpawns)
		{
			max_usable_spawn_sites = SecludedSpawns;
			const auto &&predicate = [](const site &a, const site &b) {
				return a.second > b.second;
			};
			const auto b = sites.begin();
			const auto m = std::next(b, SecludedSpawns);
			const auto e = std::next(b, max_spawn_sites);
			std::partial_sort(b, m, e, predicate);
		}
		else
			max_usable_spawn_sites = max_spawn_sites;
	}
	unsigned get_usable_sites() const
	{
		return max_usable_spawn_sites;
	}
	const site &operator[](std::size_t i) const
	{
		return sites[i];
	}
};

}

namespace dsx {

//initialize the player object position & orientation (at start of game, or new ship)
static void InitPlayerPosition(fvmobjptridx &vmobjptridx, fvmsegptridx &vmsegptridx, int random_flag)
{
	reset_cruise();
	int NewPlayer=0;

	if (! ((Game_mode & GM_MULTI) && !(Game_mode&GM_MULTI_COOP)) ) // If not deathmatch
		NewPlayer = Player_num;
	else if (random_flag == 1)
	{
		const respawn_locations locations(vcsegptridx);
		if (!locations.get_usable_sites())
			return;
		uint_fast32_t trys=0;
		d_srand(static_cast<fix>(timer_update()));
		do {
			trys++;
			NewPlayer = d_rand() % locations.get_usable_sites();
			const auto closest_dist = locations[NewPlayer].second;
			if (closest_dist >= i2f(15*20))
				break;
		} while (trys < MAX_PLAYERS * 2);
		NewPlayer = locations[NewPlayer].first;
	}
	else {
		// If deathmatch and not random, positions were already determined by sync packet
		reset_player_object();
		return;
	}
	Assert(NewPlayer >= 0);
	Assert(NewPlayer < NumNetPlayerPositions);
	ConsoleObject->pos = Player_init[NewPlayer].pos;
	ConsoleObject->orient = Player_init[NewPlayer].orient;
	obj_relink(vmobjptr, vmsegptr, vmobjptridx(ConsoleObject), vmsegptridx(Player_init[NewPlayer].segnum));
	reset_player_object();
}

}

//	-----------------------------------------------------------------------------------------------------
//	Initialize default parameters for one robot, copying from Robot_info to *objp.
//	What about setting size!?  Where does that come from?
namespace dsx {
void copy_defaults_to_robot(const vmobjptr_t objp)
{
	int			objid;

	Assert(objp->type == OBJ_ROBOT);
	objid = get_robot_id(objp);
	Assert(objid < N_robot_types);

	auto &robptr = Robot_info[objid];

	//	Boost shield for Thief and Buddy based on level.
	objp->shields = robptr.strength;

#if defined(DXX_BUILD_DESCENT_II)
	if ((robot_is_thief(robptr)) || (robot_is_companion(robptr))) {
		objp->shields = (objp->shields * (abs(Current_level_num)+7))/8;

		if (robot_is_companion(robptr)) {
			//	Now, scale guide-bot hits by skill level
			switch (Difficulty_level) {
				case 0:	objp->shields = i2f(20000);	break;		//	Trainee, basically unkillable
				case 1:	objp->shields *= 3;				break;		//	Rookie, pretty dang hard
				case 2:	objp->shields *= 2;				break;		//	Hotshot, a bit tough
				default:	break;
			}
		}
	} else if (robptr.boss_flag)	//	MK, 01/16/95, make boss shields lower on lower diff levels.
		objp->shields = objp->shields/(NDL+3) * (Difficulty_level+4);

	//	Additional wimpification of bosses at Trainee
	if (robptr.boss_flag && Difficulty_level == 0)
		objp->shields /= 2;
#endif
}
}

//	-----------------------------------------------------------------------------------------------------
//	Copy all values from the robot info structure to all instances of robots.
//	This allows us to change bitmaps.tbl and have these changes manifested in existing robots.
//	This function should be called at level load time.
static void copy_defaults_to_robot_all(void)
{
	range_for (const auto &&objp, vmobjptr)
	{
		if (objp->type == OBJ_ROBOT)
			copy_defaults_to_robot(objp);
	}
}

//	-----------------------------------------------------------------------------------------------------
//called when the player is starting a level (new game or new ship)
namespace dsx {
static void StartLevel(int random_flag)
{
	assert(Player_dead_state == player_dead_state::no);

	InitPlayerPosition(vmobjptridx, vmsegptridx, random_flag);

	verify_console_object();

	ConsoleObject->control_type	= CT_FLYING;
	ConsoleObject->movement_type	= MT_PHYSICS;

	// create_player_appearance_effect(ConsoleObject);
	Do_appearance_effect = 1;

	if (Game_mode & GM_MULTI)
	{
		if (Game_mode & GM_MULTI_COOP)
			multi_send_score();
	 	multi_send_reappear();
		multi_do_protocol_frame(1, 1);
	}
	else // in Singleplayer, after we died ...
	{
		disable_matcens(); // ... disable matcens and ...
		clear_transient_objects(0); // ... clear all transient objects.
	}

	ai_reset_all_paths();
	ai_init_boss_for_ship();

	reset_rear_view();
	auto &player_info = ConsoleObject->ctype.player_info;
	player_info.Auto_fire_fusion_cannon_time = 0;
	player_info.Fusion_charge = 0;
}
}
