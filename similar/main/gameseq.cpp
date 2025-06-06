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
#include <ranges>
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
#include "dxxerror.h"
#include "joy.h"
#include "timer.h"
#include "laser.h"
#include "event.h"
#include "screens.h"
#include "textures.h"
#include "gauges.h"
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
#include "newdemo.h"
#include "titles.h"
#include "weapon.h"
#include "sounds.h"
#include "args.h"
#include "gameseq.h"
#include "gamefont.h"
#include "newmenu.h"
#include "hudmsg.h"
#include "endlevel.h"
#include "kmatrix.h"
#include "net_udp.h"
#include "playsave.h"
#include "fireball.h"
#include "kconfig.h"
#include "robot.h"
#include "automap.h"
#include "cntrlcen.h"
#include "powerup.h"
#include "text.h"
#include "piggy.h"
#include "mission.h"
#include "state.h"
#include "songs.h"
#include "gamepal.h"
#include "controls.h"
#include "credits.h"
#if DXX_USE_EDITOR
#include "editor/editor.h"
#endif
#include "strutil.h"
#include "segment.h"
#include "gameseg.h"
#include "fmtcheck.h"

#include "compiler-range_for.h"
#include "d_enumerate.h"
#include "d_levelstate.h"
#include "partial_range.h"
#include "d_range.h"
#include "d_underlying_value.h"
#include "d_zip.h"

#if DXX_BUILD_DESCENT == 1
#include "custom.h"
#define GLITZ_BACKGROUND	Menu_pcx_name

namespace d1x {
namespace {

static int8_t find_next_level(const next_level_request_secret_flag secret_flag, const int current_level_num, const Mission &mission)
{
	if (secret_flag != next_level_request_secret_flag::only_normal_level)
	{			//go to secret level instead
		for (const auto &&[idx, table_entry] : enumerate(
				unchecked_partial_range(
					mission.secret_level_table.get(),
					static_cast<unsigned>(-mission.last_secret_level)
				)
		))
			if (table_entry == current_level_num)
				return -(idx + 1);
		//couldn't find which secret level
		LevelError("secret exit from level %u failed to find secret level; continuing to next normal level", current_level_num);
	}
	else if (current_level_num < 0)
	{			//on secret level, where to go?
		//shouldn't be going to secret level
		assert(current_level_num <= -1 && current_level_num >= mission.last_secret_level);
		return mission.secret_level_table[(-current_level_num) - 1] + 1;
	}
	return current_level_num + 1;		//assume go to next normal level
}

}
}
#elif DXX_BUILD_DESCENT == 2
#include "movie.h"
#define GLITZ_BACKGROUND	STARS_BACKGROUND

namespace dsx {
namespace {
static void StartNewLevelSecret(int level_num, int page_in_textures);
static void InitPlayerPosition(fvmobjptridx &vmobjptridx, fvmsegptridx &vmsegptridx, int random_flag);
static void DoEndGame();
}
PHYSFSX_gets_line_t<FILENAME_LEN> Current_level_palette;
int First_secret_visit{1};
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
	preserve_player_object_info(fvcobjptr &vcobjptr, const objnum_t &o) :
		objnum(o)
	{
		auto &plr = *vcobjptr(objnum);
		plr_shields = plr.shields;
		plr_info = plr.ctype.player_info;
	}
	void restore(fvmobjptr &vmobjptr) const
	{
		auto &plr = *vmobjptr(objnum);
		plr.shields = plr_shields;
		plr.ctype.player_info = plr_info;
	}
};

}

namespace dcx {
//Current_level_num starts at 1 for the first level
//-1,-2,-3 are secret levels
//0 used to mean not a real level loaded (i.e. editor generated level), but this hack has been removed
int Current_level_num{1};
PHYSFSX_gets_line_t<LEVEL_NAME_LEN> Current_level_name;

// Global variables describing the player
unsigned N_players{1};	// Number of players ( >1 means a net game, eh?)
playernum_t Player_num;	// The player number who is on the console.
fix StartingShields=INITIAL_SHIELDS;
per_player_array<obj_position> Player_init;

// Global variables telling what sort of game we have
unsigned NumNetPlayerPositions;
int Do_appearance_effect{0};

namespace {

template <object_type_t type>
static bool is_object_of_type(const object_base &o)
{
	return o.type == type;
}

static unsigned get_starting_concussion_missile_count()
{
	return 2 + NDL - underlying_value(GameUniqueState.Difficulty_level);
}

}

}

namespace dsx {
namespace {
static void init_player_stats_ship(object &, fix GameTime64);
static window_event_result AdvanceLevel(
#if DXX_BUILD_DESCENT == 1
#undef AdvanceLevel
	next_level_request_secret_flag secret_flag
#elif DXX_BUILD_DESCENT == 2
#define AdvanceLevel(secret_flag)	((void)secret_flag,AdvanceLevel())
#endif
	);
static void StartLevel(int random_flag);
static void copy_defaults_to_robot_all(const d_robot_info_array &Robot_info);

//--------------------------------------------------------------------
static void verify_console_object()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
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

constexpr constant_xrange<sidenum_t, sidenum_t::WRIGHT, sidenum_t::WFRONT> displacement_sides{};
static_assert(static_cast<uint8_t>(sidenum_t::WBACK) + 1 == static_cast<uint8_t>(sidenum_t::WFRONT), "side ordering error");

static unsigned generate_extra_starts_by_copying(object_array &Objects, valptridx<player>::array_managed_type &Players, segment_array &Segments, const xrange<unsigned, std::integral_constant<unsigned, 0>> preplaced_start_range, const per_player_array<sidemask_t> &player_init_segment_capacity_flag, const unsigned total_required_num_starts, unsigned synthetic_player_idx)
{
	for (const auto side : displacement_sides)
	{
		for (const auto old_player_idx : preplaced_start_range)
		{
			auto &old_player_ref = *Players.vcptr(old_player_idx);
			const auto &&old_player_ptridx = Objects.vcptridx(old_player_ref.objnum);
			auto &old_player_obj = *old_player_ptridx;
			if (player_init_segment_capacity_flag[old_player_idx] & build_sidemask(side))
			{
				auto &&segp = Segments.vmptridx(old_player_obj.segnum);
				/* Copy the start exactly.  The next loop in the caller will
				 * fix the collisions caused by placing the clone on top of the
				 * original.
				 *
				 * Currently, there is no handling for the case that the
				 * level author already put two players too close together.
				 * If this is a problem, more logic can be added to suppress
				 * cloning in that case.
				 */
				const auto &&extra_player_ptridx = obj_create_copy(old_player_obj, segp);
				if (extra_player_ptridx == object_none)
				{
					con_printf(CON_URGENT, "%s:%u: warning: failed to copy start object %hu", __FILE__, __LINE__, old_player_ptridx.get_unchecked_index());
					continue;
				}
				Players.vmptr(synthetic_player_idx)->objnum = extra_player_ptridx;
				auto &extra_player_obj = *extra_player_ptridx;
				set_player_id(extra_player_obj, synthetic_player_idx);
				con_printf(CON_NORMAL, "Copied player %u (object %hu at {%i, %i, %i}) to create player %u (object %hu).", old_player_idx, old_player_ptridx.get_unchecked_index(), old_player_obj.pos.x, old_player_obj.pos.y, old_player_obj.pos.z, synthetic_player_idx, extra_player_ptridx.get_unchecked_index());
				if (++ synthetic_player_idx >= total_required_num_starts)
					return synthetic_player_idx;
			}
		}
	}
	return synthetic_player_idx;
}

static unsigned generate_extra_starts_by_displacement_within_segment(const unsigned preplaced_starts, const unsigned total_required_num_starts)
{
	const auto &&preplaced_start_range = xrange(preplaced_starts);
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vcobjptr = Objects.vcptr;
	auto &vmobjptr = Objects.vmptr;
	per_player_array<sidemask_t> player_init_segment_capacity_flag{};
	DXX_MAKE_VAR_UNDEFINED(player_init_segment_capacity_flag);
	static_assert(static_cast<uint8_t>(sidenum_t::WRIGHT) + 1 == static_cast<uint8_t>(sidenum_t::WBOTTOM), "side ordering error");
	static_assert(static_cast<uint8_t>(sidenum_t::WBOTTOM) + 1 == static_cast<uint8_t>(sidenum_t::WBACK), "side ordering error");
	constexpr auto capacity_x = build_sidemask(sidenum_t::WRIGHT);
	constexpr auto capacity_y = build_sidemask(sidenum_t::WBOTTOM);
	constexpr auto capacity_z = build_sidemask(sidenum_t::WBACK);
	/* When players are displaced, they are moved by their size
	 * multiplied by this constant.  Larger values provide more
	 * separation between the player starts, but increase the chance
	 * that the player will be too close to a wall or that the segment
	 * will be deemed too small to support displacement.
	 */
	constexpr fix size_scalar = 0x18000;	// 1.5 in fixed point
	unsigned segments_with_spare_capacity{0};
	auto &vcvertptr = Vertices.vcptr;
	for (const auto i : preplaced_start_range)
	{
		/* For each existing Player_init, compute whether the segment is
		 * large enough in each dimension to support adding more ships.
		 */
		const auto &pi = Player_init[i];
		const auto segnum{pi.segnum};
		const shared_segment &seg = *vcsegptr(segnum);
		auto &plr = *Players.vcptr(i);
		auto &old_player_obj = *vcobjptr(plr.objnum);
		const vm_distance_squared size2{fixmul64(old_player_obj.size * old_player_obj.size, size_scalar)};
		auto &v0 = *vcvertptr(seg.verts[segment_relative_vertnum::_0]);
		sidemask_t capacity_flag{};
		if (vm_vec_dist2(v0, vcvertptr(seg.verts[segment_relative_vertnum::_1])) > size2)
			capacity_flag |= capacity_x;
		if (vm_vec_dist2(v0, vcvertptr(seg.verts[segment_relative_vertnum::_3])) > size2)
			capacity_flag |= capacity_y;
		if (vm_vec_dist2(v0, vcvertptr(seg.verts[segment_relative_vertnum::_4])) > size2)
			capacity_flag |= capacity_z;
		player_init_segment_capacity_flag[i] = capacity_flag;
		con_printf(CON_NORMAL, "Original player %u has size %u, starts in segment #%hu, and has segment capacity flags %x.", i, old_player_obj.size, static_cast<segnum_t>(segnum), underlying_value(capacity_flag));
		if (capacity_flag != sidemask_t{})
			++segments_with_spare_capacity;
	}
	if (!segments_with_spare_capacity)
		/* Every segment checked was too small to add an extra start.  Give up
		 * early.
		 */
		return preplaced_starts;
	unsigned k = generate_extra_starts_by_copying(Objects, Players, Segments, preplaced_start_range, player_init_segment_capacity_flag, total_required_num_starts, preplaced_starts);
	for (const auto old_player_idx : preplaced_start_range)
	{
		auto &old_player_init = Player_init[old_player_idx];
		const auto old_player_pos = old_player_init.pos;
		auto &old_player_obj = *vmobjptr(Players.vcptr(old_player_idx)->objnum);
		std::array<vms_vector, 3> vec_displacement{};
		DXX_MAKE_VAR_UNDEFINED(vec_displacement);
		const shared_segment &seg = *vcsegptr(old_player_init.segnum);
		/* For each of [right, bottom, back], compute the vector between
		 * the center of that side and the reference player's start
		 * point.  This will be used in the next loop.
		 */
		for (auto &&[side, displacement] : zip(displacement_sides, vec_displacement))
		{
			const auto center_on_side{compute_center_point_on_side(vcvertptr, seg, side)};
			displacement = vm_vec_build_sub(center_on_side, old_player_init.pos);
		}
		const auto displace_player = [&](const unsigned plridx, object_base &plrobj, const unsigned displacement_direction) {
			vms_vector disp{};
			unsigned dimensions{0};
			for (const auto &&[i, side] : enumerate(displacement_sides))
			{
				if (!(player_init_segment_capacity_flag[old_player_idx] & build_sidemask(side)))
				{
					con_printf(CON_NORMAL, "Cannot displace player %u at {%i, %i, %i}: not enough room in dimension %u.", plridx, plrobj.pos.x, plrobj.pos.y, plrobj.pos.z, underlying_value(side));
					continue;
				}
				const auto &v = vec_displacement[i];
				const auto &va{(displacement_direction & (1 << i)) ? v : vm_vec_build_negated(v)};
				con_printf(CON_NORMAL, "Add displacement of {%i, %i, %i} for dimension %u for player %u.", va.x, va.y, va.z, underlying_value(side), plridx);
				++ dimensions;
				vm_vec_add2(disp, va);
			}
			if (!dimensions)
				return;
			vm_vec_normalize(disp);
			vm_vec_scale(disp, fixmul(old_player_obj.size, size_scalar >> 1));
			const auto target_position = vm_vec_build_add(Player_init[plridx].pos, disp);
			if (const auto sidemask = get_seg_masks(vcvertptr, target_position, vcsegptr(plrobj.segnum), 1).sidemask; sidemask != sidemask_t{})
			{
				con_printf(CON_NORMAL, "Cannot displace player %u at {%i, %i, %i} to {%i, %i, %i}: would be outside segment for sides %x.", plridx, plrobj.pos.x, plrobj.pos.y, plrobj.pos.z, target_position.x, target_position.y, target_position.z, underlying_value(sidemask));
				return;
			}
			con_printf(CON_NORMAL, "Displace player %u at {%i, %i, %i} by {%i, %i, %i} to {%i, %i, %i}.", plridx, plrobj.pos.x, plrobj.pos.y, plrobj.pos.z, disp.x, disp.y, disp.z, target_position.x, target_position.y, target_position.z);
			Player_init[plridx].pos = target_position;
			plrobj.pos = Player_init[plridx].pos;
		};
		for (unsigned extra_player_idx = preplaced_starts, displacements = 0; extra_player_idx < k; ++extra_player_idx)
		{
			auto &extra_player_obj = *vmobjptr(Players.vcptr(extra_player_idx)->objnum);
			if (old_player_pos != extra_player_obj.pos)
				/* This clone is associated with some other player.
				 * Skip it here.  It will be handled in a different pass
				 * of the loop.
				 */
				continue;
			auto &extra_player_init = Player_init[extra_player_idx];
			extra_player_init = old_player_init;
			if (!displacements++)
				displace_player(old_player_idx, old_player_obj, 0);
			displace_player(extra_player_idx, extra_player_obj, displacements);
		}
	}
	return k;
}

//added 10/12/95: delete buddy bot if coop game.  Probably doesn't really belong here. -MT
#if DXX_BUILD_DESCENT == 1
#define gameseq_init_network_players(Robot_info,Objects)	gameseq_init_network_players(Objects)
#elif DXX_BUILD_DESCENT == 2
#undef gameseq_init_network_players
#endif
static void gameseq_init_network_players(const d_robot_info_array &Robot_info, object_array &Objects)
{

	// Initialize network player start locations and object numbers

	ConsoleObject = &Objects.front();
	unsigned j{0}, k = 0;
	const auto multiplayer_coop = Game_mode & GM_MULTI_COOP;
#if DXX_BUILD_DESCENT == 2
	const auto remove_thief = Netgame.ThiefModifierFlags & ThiefModifier::Absent;
	const auto multiplayer{Game_mode & GM_MULTI};
	const auto retain_guidebot{Netgame.AllowGuidebot};
#endif
	auto &vmobjptridx = Objects.vmptridx;
	range_for (const auto &&o, vmobjptridx)
	{
		const auto type = o->type;
		if (type == OBJ_PLAYER || type == OBJ_GHOST || type == OBJ_COOP)
		{
			if (likely(k < Player_init.size()) &&
				multiplayer_coop
				? (j == 0 || type == OBJ_COOP)
				: (type == OBJ_PLAYER || type == OBJ_GHOST)
			)
			{
				o->type=OBJ_PLAYER;
				auto &pi = Player_init[k];
				pi.pos = o->pos;
				pi.orient = o->orient;
				pi.segnum = o->segnum;
				vmplayerptr(k)->objnum = o;
				set_player_id(o, k);
				k++;
			}
			else
				obj_delete(LevelUniqueObjectState, Segments, o);
			j++;
		}
#if DXX_BUILD_DESCENT == 2
		else if (type == OBJ_ROBOT && multiplayer)
		{
			auto &ri = Robot_info[get_robot_id(o)];
			if ((!retain_guidebot && robot_is_companion(ri)) ||
				(remove_thief && robot_is_thief(ri)))
			{
				object_create_robot_egg(Robot_info, o);
				obj_delete(LevelUniqueObjectState, Segments, o);		//kill the buddy in netgames
			}
		}
#endif
	}
	if (multiplayer_coop)
	{
	const unsigned total_required_num_starts = Netgame.max_numplayers;
	if (k < total_required_num_starts)
	{
		con_printf(CON_NORMAL, "Insufficient cooperative starts found in mission \"%s\" level %u (need %u, found %u).  Generating extra starts...", Current_mission->path.c_str(), Current_level_num, total_required_num_starts, k);
		/*
		 * First, try displacing the starts within the existing segment.
		 */
		const unsigned preplaced_starts = k;
		k = generate_extra_starts_by_displacement_within_segment(preplaced_starts, total_required_num_starts);
		con_printf(CON_NORMAL, "Generated %u starts by displacement within the original segment.", k - preplaced_starts);
	}
	else
		con_printf(CON_NORMAL, "Found %u cooperative starts in mission \"%s\" level %u.", k, Current_mission->path.c_str(), Current_level_num);
	}
	NumNetPlayerPositions = k;
}

}

void gameseq_remove_unused_players(const d_robot_info_array &Robot_info)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	// 'Remove' the unused players

	if (Game_mode & GM_MULTI)
	{
		for (unsigned i = 0; i < NumNetPlayerPositions; ++i)
		{
			if (vcplayerptr(i)->connected == player_connection_status::disconnected || i >= N_players)
			{
				multi_make_player_ghost(i);
			}
		}
	}
	else
	{		// Note link to above if!!!
		range_for (auto &i, partial_const_range(Players, 1u, NumNetPlayerPositions))
		{
			obj_delete(LevelUniqueObjectState, Segments, vmobjptridx(i.objnum));
		}
#if DXX_BUILD_DESCENT == 2
		if (PlayerCfg.ThiefModifierFlags & ThiefModifier::Absent)
		{
			range_for (const auto &&o, vmobjptridx)
			{
				const auto type = o->type;
				if (type == OBJ_ROBOT)
				{
					auto &ri = Robot_info[get_robot_id(o)];
					if (robot_is_thief(ri))
					{
						object_create_robot_egg(Robot_info, o);
						obj_delete(LevelUniqueObjectState, Segments, o);
					}
				}
			}
		}
#endif
	}
}

// Setup player for new game
void init_player_stats_game(const playernum_t pnum)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	auto &plr = *vmplayerptr(pnum);
	plr.lives = INITIAL_LIVES;
	plr.level = 1;
	plr.time_level = 0;
	plr.time_total = 0;
	plr.hours_level = 0;
	plr.hours_total = 0;
	plr.num_kills_level = 0;
	plr.num_kills_total = 0;
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
#if DXX_BUILD_DESCENT == 2
	if (pnum == Player_num)
		First_secret_visit = 1;
#endif
}

namespace {

static void init_ammo_and_energy(object &plrobj)
{
	auto &player_info = plrobj.ctype.player_info;
	{
		auto &energy = player_info.energy;
#if DXX_BUILD_DESCENT == 2
		if (player_info.primary_weapon_flags & HAS_OMEGA_FLAG)
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
	const unsigned minimum_missiles = get_starting_concussion_missile_count();
	auto &concussion = player_info.secondary_ammo[secondary_weapon_index_t::CONCUSSION_INDEX];
	if (concussion < minimum_missiles)
		concussion = minimum_missiles;
}

}

#if DXX_BUILD_DESCENT == 2
extern	ubyte	Last_afterburner_state;
#endif

namespace {

// Setup player for new level (After completion of previous level)
static void init_player_stats_level(player &plr, object &plrobj, const secret_restore secret_flag)
{
#if DXX_BUILD_DESCENT == 2
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptridx = Objects.vcptridx;
#endif

	plr.level = Current_level_num;

	if (!Network_rejoined) {
		plr.time_level = 0;
		plr.hours_level = 0;
	}

	auto &player_info = plrobj.ctype.player_info;
	player_info.mission.last_score = player_info.mission.score;

	plr.num_kills_level = 0;

	if (secret_flag == secret_restore::none) {
		init_ammo_and_energy(plrobj);

		auto &powerup_flags = player_info.powerup_flags;
		powerup_flags &= ~(PLAYER_FLAGS_INVULNERABLE | PLAYER_FLAGS_CLOAKED);
#if DXX_BUILD_DESCENT == 2
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

#if DXX_BUILD_DESCENT == 2
	Controls.state.afterburner = 0;
	Last_afterburner_state = 0;

	digi_kill_sound_linked_to_object(vcobjptridx(plr.objnum));
#endif
	init_gauges();
#if DXX_BUILD_DESCENT == 2
	Missile_viewer = NULL;
#endif
	init_player_stats_ship(plrobj, GameTime64);
}

}

// Setup player for a brand-new ship
void init_player_stats_new_ship(const playernum_t pnum)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	auto &plr = *vcplayerptr(pnum);
	const auto &&plrobj = vmobjptridx(plr.objnum);
	plrobj->shields = StartingShields;
	auto &player_info = plrobj->ctype.player_info;
	player_info.energy = INITIAL_ENERGY;
	player_info.secondary_ammo = {{{
		static_cast<uint8_t>(get_starting_concussion_missile_count())
	}}};
	const auto GrantedItems = (Game_mode & GM_MULTI) ? Netgame.SpawnGrantedItems : packed_spawn_granted_items{};
	player_info.vulcan_ammo = map_granted_flags_to_vulcan_ammo(GrantedItems);
	const auto granted_laser_level = map_granted_flags_to_laser_level(GrantedItems);
	player_info.laser_level = granted_laser_level;
	const auto granted_primary_weapon_flags = HAS_LASER_FLAG | map_granted_flags_to_primary_weapon_flags(GrantedItems);
	player_info.primary_weapon_flags = granted_primary_weapon_flags;
	player_info.powerup_flags &= ~(PLAYER_FLAGS_QUAD_LASERS | PLAYER_FLAGS_CLOAKED | PLAYER_FLAGS_INVULNERABLE);
#if DXX_BUILD_DESCENT == 2
	player_info.powerup_flags &= ~(PLAYER_FLAGS_AFTERBURNER | PLAYER_FLAGS_MAP_ALL | PLAYER_FLAGS_CONVERTER | PLAYER_FLAGS_AMMO_RACK | PLAYER_FLAGS_HEADLIGHT | PLAYER_FLAGS_HEADLIGHT_ON | PLAYER_FLAGS_FLAG);
	player_info.Omega_charge = (granted_primary_weapon_flags & HAS_OMEGA_FLAG)
		? MAX_OMEGA_CHARGE
		: 0;
	player_info.Omega_recharge_delay = 0;
	if (game_mode_hoard(Game_mode))
		player_info.hoard.orbs = 0;
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
				if (underlying_value(i) >= MAX_PRIMARY_WEAPONS)
					break;
				if (i == primary_weapon_index_t::LASER_INDEX)
					break;
#if DXX_BUILD_DESCENT == 2
				if (i == primary_weapon_index_t::SUPER_LASER_INDEX)
				{
					if (granted_laser_level <= laser_level::_4)
						/* Granted lasers are not super lasers */
						continue;
					/* Super lasers still set LASER_INDEX, not
					 * SUPER_LASER_INDEX
					 */
					break;
				}
#endif
				if (HAS_PRIMARY_FLAG(i) & static_cast<unsigned>(granted_primary_weapon_flags))
					return i;
			}
			return primary_weapon_index_t::LASER_INDEX;
		}());
#if DXX_BUILD_DESCENT == 2
		auto primary_last_was_super = player_info.Primary_last_was_super;
		for (uint8_t i = static_cast<uint8_t>(primary_weapon_index_t::VULCAN_INDEX), mask = 1 << i; i != static_cast<uint8_t>(primary_weapon_index_t::SUPER_LASER_INDEX); ++i, mask <<= 1)
		{
			/* If no super granted, force to non-super. */
			if (!(HAS_PRIMARY_FLAG(primary_weapon_index_t{static_cast<uint8_t>(i + 5)}) & granted_primary_weapon_flags))
				primary_last_was_super &= ~mask;
			/* If only super granted, force to super. */
			else if (!(HAS_PRIMARY_FLAG(primary_weapon_index_t{i}) & granted_primary_weapon_flags))
				primary_last_was_super |= mask;
			/* else both granted, so leave as-is. */
			else
				continue;
		}
		player_info.Primary_last_was_super = primary_last_was_super;
#endif
		if (Newdemo_state == ND_STATE_RECORDING)
		{
			newdemo_record_laser_level(player_info.laser_level, laser_level::_1);
		}
		set_secondary_weapon_to_concussion(player_info);
		dead_player_end(); //player no longer dead
		Player_dead_state = player_dead_state::no;
		player_info.Player_eggs_dropped = false;
		Dead_player_camera = 0;
#if DXX_BUILD_DESCENT == 2
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
	init_player_stats_ship(plrobj, GameTime64);
}

namespace {
void init_player_stats_ship(object &plrobj, const fix GameTime64)
{
	auto &player_info = plrobj.ctype.player_info;
	player_info.lavafall_hiss_playing = false;
	player_info.missile_gun = 0;
	player_info.Spreadfire_toggle = 0;
	player_info.killer_objnum = object_none;
#if DXX_BUILD_DESCENT == 2
	player_info.Omega_recharge_delay = 0;
	player_info.Helix_orientation = 0;
#endif
	player_info.mission.hostages_on_board = 0;
	player_info.homing_object_dist = -F1_0; // Added by RH
	player_info.Next_flare_fire_time = player_info.Next_laser_fire_time = player_info.Next_missile_fire_time = {GameTime64};
}
}

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
namespace {

static void set_sound_sources(fvcsegptridx &vcsegptridx, fvcvertptr &vcvertptr)
{
	auto &Effects = LevelUniqueEffectsClipState.Effects;
	auto &TmapInfo = LevelUniqueTmapInfoState.TmapInfo;
	digi_init_sounds();		//clear old sounds
#if DXX_BUILD_DESCENT == 2
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptr = Walls.vcptr;
#endif
	Dont_start_sound_objects = 1;

	const auto get_eclip_for_tmap = [](const d_level_unique_tmap_info_state::TmapInfo_array &TmapInfo, const unique_side &side) {
		if (const auto tm2 = side.tmap_num2; tm2 != texture2_value::None)
		{
			const auto ec = TmapInfo[get_texture_index(tm2)].eclip_num;
#if DXX_BUILD_DESCENT == 2
			if (ec != eclip_none)
#endif
				return ec;
		}
#if DXX_BUILD_DESCENT == 1
		return eclip_none.value;
#elif DXX_BUILD_DESCENT == 2
		return TmapInfo[get_texture_index(side.tmap_num)].eclip_num;
#endif
	};

	range_for (const auto &&seg, vcsegptridx)
	{
		for (const auto sidenum : MAX_SIDES_PER_SEGMENT)
		{
#if DXX_BUILD_DESCENT == 2
			const auto wid = WALL_IS_DOORWAY(GameBitmaps, Textures, vcwallptr, seg, sidenum);
			if (!(wid & WALL_IS_DOORWAY_FLAG::render))
				continue;
#endif
			const auto ec = get_eclip_for_tmap(TmapInfo, seg->unique_segment::sides[sidenum]);
			if (ec != eclip_none)
			{
				if (const auto sn{Effects[ec].sound_num}; sn != sound_effect::None)
				{
#if DXX_BUILD_DESCENT == 2
						auto csegnum = seg->children[sidenum];

						//check for sound on other side of wall.  Don't add on
						//both walls if sound travels through wall.  If sound
						//does travel through wall, add sound for lower-numbered
						//segment.

						if (IS_CHILD(csegnum) && csegnum < seg) {
							if (wid & (WALL_IS_DOORWAY_FLAG::fly | WALL_IS_DOORWAY_FLAG::rendpast)) {
								const auto &&csegp = vcsegptr(seg->children[sidenum]);
								auto csidenum = find_connect_side(seg, csegp);

								if (csegp->unique_segment::sides[csidenum].tmap_num2 == seg->unique_segment::sides[sidenum].tmap_num2)
									continue;		//skip this one
							}
						}
#endif

						const auto pnt{compute_center_point_on_side(vcvertptr, seg, sidenum)};
						digi_link_sound_to_pos(sn, seg, sidenum, pnt, 1, F1_0/2);
					}
			}
		}
	}
	Dont_start_sound_objects = 0;
}

constexpr fix flash_dist=fl2f(.9);
}

//create flash for player appearance
void create_player_appearance_effect(const d_vclip_array &Vclip, const object_base &player_obj)
{
	const auto pos = (&player_obj == Viewer)
		? vm_vec_scale_add(player_obj.pos, player_obj.orient.fvec, fixmul(player_obj.size, flash_dist))
		: player_obj.pos;

	const auto &&seg = vmsegptridx(player_obj.segnum);
	const auto &&effect_obj = object_create_explosion_without_damage(Vclip, seg, pos, player_obj.size, vclip_index::player_appearance);

	if (effect_obj) {
		effect_obj->orient = player_obj.orient;

		const auto sound_num = Vclip[vclip_index::player_appearance].sound_num;
		if (sound_num != sound_effect::None)
			digi_link_sound_to_pos(sound_num, seg, sidenum_t::WLEFT, effect_obj->pos, 0, F1_0);
	}
}
}

//
// New Game sequencing functions
//

namespace {

//get level filename. level numbers start at 1.  Secret levels are -1,-2,-3
static const d_fname &get_level_file(int level_num)
{
	if (level_num<0)                //secret level
		return Current_mission->secret_level_names[-level_num - 1];
	else                                    //normal level
		return Current_mission->level_names[level_num - 1];
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

}

namespace dsx {
namespace {
static ushort netmisc_calc_checksum()
{
	unsigned int sum1,sum2;
	short s;
	int t;

	sum1 = sum2 = 0;
	for (const cscusegment i : vcsegptr)
	{
		for (auto &&[sside, uside] : zip(i.s.sides, i.u.sides))
		{
			do_checksum_calc(reinterpret_cast<const uint8_t *>(&sside.type), 1, &sum1, &sum2);
			s = INTEL_SHORT(underlying_value(sside.wall_num));
			do_checksum_calc(reinterpret_cast<uint8_t *>(&s), 2, &sum1, &sum2);
			s = underlying_value(uside.tmap_num);
			s = INTEL_SHORT(s);
			do_checksum_calc(reinterpret_cast<uint8_t *>(&s), 2, &sum1, &sum2);
			s = underlying_value(uside.tmap_num2);
			s = INTEL_SHORT(s);
			do_checksum_calc(reinterpret_cast<uint8_t *>(&s), 2, &sum1, &sum2);
			range_for (auto &k, uside.uvls)
			{
				t = INTEL_INT(k.u);
				do_checksum_calc(reinterpret_cast<uint8_t *>(&t), 4, &sum1, &sum2);
				t = INTEL_INT(k.v);
				do_checksum_calc(reinterpret_cast<uint8_t *>(&t), 4, &sum1, &sum2);
				t = INTEL_INT(k.l);
				do_checksum_calc(reinterpret_cast<uint8_t *>(&t), 4, &sum1, &sum2);
			}
			range_for (auto &k, sside.normals)
			{
				t = INTEL_INT(k.x);
				do_checksum_calc(reinterpret_cast<uint8_t *>(&t), 4, &sum1, &sum2);
				t = INTEL_INT(k.y);
				do_checksum_calc(reinterpret_cast<uint8_t *>(&t), 4, &sum1, &sum2);
				t = INTEL_INT(k.z);
				do_checksum_calc(reinterpret_cast<uint8_t *>(&t), 4, &sum1, &sum2);
			}
		}
		for (const typename std::underlying_type<segnum_t>::type j : i.s.children)
		{
			const auto s = INTEL_SHORT(j);
			do_checksum_calc(reinterpret_cast<const uint8_t *>(&s), 2, &sum1, &sum2);
		}
		range_for (const auto vn, i.s.verts)
		{
			const auto j{underlying_value(vn)};
			static_assert(MAX_VERTICES <= UINT16_MAX);
			s = INTEL_SHORT(static_cast<uint16_t>(j));
			do_checksum_calc(reinterpret_cast<uint8_t *>(&s), 2, &sum1, &sum2);
		}
		s = INTEL_SHORT(i.u.objects);
		do_checksum_calc(reinterpret_cast<uint8_t *>(&s), 2, &sum1, &sum2);
#if DXX_BUILD_DESCENT == 1
		const auto special = underlying_value(i.s.special);
		do_checksum_calc(&special, 1, &sum1, &sum2);
		do_checksum_calc(reinterpret_cast<const uint8_t *>(&i.s.matcen_num), 1, &sum1, &sum2);
		t = INTEL_INT(i.u.static_light);
		do_checksum_calc(reinterpret_cast<uint8_t *>(&t), 4, &sum1, &sum2);
#endif
		{
			const auto station_idx = underlying_value(i.s.station_idx);
			do_checksum_calc(&station_idx, 1, &sum1, &sum2);
		}
	}
	sum2 %= 255;
	return ((sum1<<8)+ sum2);
}
}
}

#if DXX_BUILD_DESCENT == 2
namespace dsx {

void load_level_robots(int level_num)
{
	const d_fname &level_name = get_level_file(level_num);
	load_level_robots(level_name);
}

// load just the hxm file
void load_level_robots(const d_fname &level_name)
{
	if (Robot_replacements_loaded) {
		free_polygon_models(LevelSharedPolygonModelState);
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
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vcobjptr = Objects.vcptr;
	auto &vmobjptr = Objects.vmptr;
	preserve_player_object_info p(vcobjptr, vcplayerptr(Player_num)->objnum);

	auto &plr = get_local_player();
	auto save_player = plr;

	assert(level_num <= Current_mission->last_level && level_num >= Current_mission->last_secret_level && level_num != 0);
	const d_fname &level_name = get_level_file(level_num);
#if DXX_BUILD_DESCENT == 1
	if (!load_level(level_name))
		Current_level_num=level_num;

	gr_use_palette_table( "palette.256" );
#elif DXX_BUILD_DESCENT == 2
	gr_set_default_canvas();
	gr_clear_canvas(*grd_curcanv, BM_XRGB(0, 0, 0));		//so palette switching is less obvious

	load_level_robots(level_name);

	auto &LevelSharedDestructibleLightState = LevelSharedSegmentState.DestructibleLights;
	int load_ret = load_level(LevelSharedDestructibleLightState, level_name);		//actually load the data from disk!

	if (load_ret)
		Error("Could not load level file <%s>, error = %d",static_cast<const char *>(level_name),load_ret);

	Current_level_num=level_num;

	load_palette(Current_level_palette.line(), load_palette_use::level, load_palette_change_screen::delayed);		//don't change screen
#endif

#if DXX_USE_EDITOR
	if (!EditorWindow)
#endif
	{
		gr_set_default_canvas();
		show_boxed_message(*grd_curcanv, TXT_LOADING);
		gr_flip();
	}
#ifdef RELEASE
	timer_delay(F1_0);
#endif

	load_endlevel_data(level_num);
#if DXX_BUILD_DESCENT == 1
	load_custom_data(level_name);
#elif DXX_BUILD_DESCENT == 2
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

	auto &vcvertptr = Vertices.vcptr;
	set_sound_sources(vcsegptridx, vcvertptr);

#if DXX_USE_EDITOR
	if (!EditorWindow)
#endif
		songs_play_level_song( Current_level_num, 0 );

	gr_palette_load(gr_palette);		//actually load the palette
#if DXX_BUILD_DESCENT == 1
	if ( page_in_textures )
		piggy_load_level_data();
#endif

	gameseq_init_network_players(LevelSharedRobotInfoState.Robot_info, Objects);
	p.restore(vmobjptr);
}
}

//sets up Player_num & ConsoleObject
void InitPlayerObject()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
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
	console->control_source	= object::control_type::flying;
	console->movement_source	= object::movement_type::physics;
}

//starts a new game on the given level
namespace dsx {

void StartNewGame(const int start_level)
{
	GameUniqueState.quicksave_selection = d_game_unique_state::save_slot::None;	// for first blind save, pick slot to save in
	reset_globals_for_new_game();

	Game_mode = GM_NORMAL;

	InitPlayerObject();				//make sure player's object set up

	LevelUniqueObjectState.accumulated_robots = 0;
	LevelUniqueObjectState.total_hostages = 0;
	GameUniqueState.accumulated_robots = 0;
	GameUniqueState.total_hostages = 0;
	init_player_stats_game(Player_num);		//clear all stats

	N_players = 1;

#if DXX_BUILD_DESCENT == 2
	if (start_level < 0)
	{
		/* Allow an autosave as soon as the user exits the secret level.
		 */
		state_set_immediate_autosave(GameUniqueState);
		StartNewLevelSecret(start_level, 0);
	}
	else
#endif
	{
		StartNewLevel(start_level);
		/* Override Next_autosave to avoid creating an autosave
		 * immediately after starting a new game.  No state can be lost
		 * at that point, so there is no reason to save.
		 */
		state_set_next_autosave(GameUniqueState, PlayerCfg.SPGameplayOptions.AutosaveInterval);
	}

	auto &plr = get_local_player();
	plr.starting_level = start_level;		// Mark where they started
	plr.callsign = InterfaceUniqueState.PilotName;

	game_disable_cheats();
#if DXX_BUILD_DESCENT == 2
	init_seismic_disturbances();
#endif
}

namespace {

//	-----------------------------------------------------------------------------
//	Does the bonus scoring.
//	Call with dead_flag = 1 if player died, but deserves some portion of bonus (only skill points), anyway.
static void DoEndLevelScoreGlitz()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	int level_points, skill_points, energy_points, shield_points, hostage_points;
	#define N_GLITZITEMS 9
	char				m_str[N_GLITZITEMS][32];
	newmenu_item	m[N_GLITZITEMS];
	int				i;
#if DXX_BUILD_DESCENT == 1
	gr_palette_load( gr_palette );
#elif DXX_BUILD_DESCENT == 2
	int				mine_level;

	//	Compute level player is on, deal with secret levels (negative numbers)
	mine_level = get_local_player().level;
	if (mine_level < 0)
		mine_level *= -(Current_mission->last_level / Current_mission->n_secret_levels);
#endif

	auto &plrobj = get_local_plrobj();
	auto &player_info = plrobj.ctype.player_info;
	level_points = player_info.mission.score - player_info.mission.last_score;

	const auto Difficulty_level = GameUniqueState.Difficulty_level;
	if (!cheats.enabled) {
		const auto d = underlying_value(Difficulty_level);
		switch (Difficulty_level)
		{
			default:
			case Difficulty_level_type::_0:
			case Difficulty_level_type::_1:
				skill_points = 0;
				break;
			case Difficulty_level_type::_2:
			case Difficulty_level_type::_3:
			case Difficulty_level_type::_4:
#if DXX_BUILD_DESCENT == 1
				skill_points = level_points * (d - 1) / 2;
#elif DXX_BUILD_DESCENT == 2
				skill_points = level_points * d / 4;
#endif
				skill_points -= skill_points % 100;
				break;
		}

		hostage_points = player_info.mission.hostages_on_board * 500 * (d + 1);
#if DXX_BUILD_DESCENT == 1
		shield_points = f2i(plrobj.shields) * 10 * (d + 1);
		energy_points = f2i(player_info.energy) * 5 * (d + 1);
#elif DXX_BUILD_DESCENT == 2
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

	auto &plr = get_local_player();

	unsigned c{0};
	snprintf(m_str[c++], sizeof(m_str[0]), "%s%i", TXT_SHIELD_BONUS, shield_points);		// Return at start to lower menu...
	snprintf(m_str[c++], sizeof(m_str[0]), "%s%i", TXT_ENERGY_BONUS, energy_points);
	snprintf(m_str[c++], sizeof(m_str[0]), "%s%i", TXT_HOSTAGE_BONUS, hostage_points);
	snprintf(m_str[c++], sizeof(m_str[0]), "%s%i", TXT_SKILL_BONUS, skill_points);

	const unsigned hostages_on_board = player_info.mission.hostages_on_board;
	unsigned all_hostage_points{0};
	unsigned endgame_points{0};
	uint8_t is_last_level{0};
	auto &hostage_text = m_str[c++];
	if (cheats.enabled)
		snprintf(hostage_text, sizeof(hostage_text), "Hostages saved:   \t%u", hostages_on_board);
	else if (LevelUniqueObjectState.total_hostages > hostages_on_board)
	{
		const auto hostages_lost = LevelUniqueObjectState.total_hostages - hostages_on_board;
		snprintf(hostage_text, sizeof(hostage_text), "Hostages lost:    \t%u", hostages_lost);
	}
	else
	{
		all_hostage_points = hostages_on_board * 1000 * (underlying_value(Difficulty_level) + 1);
		snprintf(hostage_text, sizeof(hostage_text), "%s%i\n", TXT_FULL_RESCUE_BONUS, all_hostage_points);
	}

	auto &endgame_text = m_str[c++];
	endgame_text[0] = 0;
	if (cheats.enabled)
	{
		/* Nothing */
	}
	else if (!(Game_mode & GM_MULTI) && plr.lives && Current_level_num == Current_mission->last_level)
	{		//player has finished the game!
		endgame_points = plr.lives * 10000;
		snprintf(endgame_text, sizeof(endgame_text), "%s%i\n", TXT_SHIP_BONUS, endgame_points);
		is_last_level=1;
	}

	add_bonus_points_to_score(player_info, skill_points + energy_points + shield_points + hostage_points + all_hostage_points + endgame_points, Game_mode);

	snprintf(m_str[c++], sizeof(m_str[0]), "%s%i\n", TXT_TOTAL_BONUS, shield_points + energy_points + hostage_points + skill_points + all_hostage_points + endgame_points);
	snprintf(m_str[c++], sizeof(m_str[0]), "%s%i", TXT_TOTAL_SCORE, player_info.mission.score);

	for (i=0; i<c; i++) {
		nm_set_item_text(m[i], m_str[i]);
	}

	auto current_level_num = Current_level_num;
	const auto txt_level = (current_level_num < 0) ? (current_level_num = -current_level_num, TXT_SECRET_LEVEL) : TXT_LEVEL;
	char subtitle[128];
	snprintf(subtitle, sizeof(subtitle), "%s%s %d %s\n%s %s", is_last_level?"\n\n\n":"\n", txt_level, current_level_num, TXT_COMPLETE, static_cast<const char *>(Current_level_name), TXT_DESTROYED);

	Assert(c <= N_GLITZITEMS);

	struct glitz_menu : passive_newmenu
	{
		glitz_menu(grs_canvas &canvas, menu_subtitle subtitle, const std::ranges::subrange<newmenu_item *> items) :
			passive_newmenu(menu_title{nullptr}, subtitle, menu_filename{GLITZ_BACKGROUND}, tiny_mode_flag::normal, tab_processing_flag::ignore, adjusted_citem::create(items, 0), canvas, draw_box_flag::none)
		{
		}
	};
	run_blocking_newmenu<glitz_menu>(*grd_curcanv, menu_subtitle{subtitle}, partial_range(m, c));
}

}

#if DXX_BUILD_DESCENT == 2
namespace {
//	-----------------------------------------------------------------------------------------------------
//called when the player is starting a level (new game or new ship)
static void StartSecretLevel()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	Assert(Player_dead_state == player_dead_state::no);

	InitPlayerPosition(vmobjptridx, vmsegptridx, 0);

	verify_console_object();

	ConsoleObject->control_source	= object::control_type::flying;
	ConsoleObject->movement_source	= object::movement_type::physics;

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
}

//	Returns true if secret level has been destroyed.
int p_secret_level_destroyed(void)
{
	if (First_secret_visit) {
		return 0;		//	Never been there, can't have been destroyed.
	} else {
		if (PHYSFS_exists(SECRETC_FILENAME))
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

}

namespace {

static void do_screen_message(const char *msg) dxx_compiler_attribute_nonnull();
static void do_screen_message(const char *msg)
{
	
	if (Game_mode & GM_MULTI)
		return;
	using items_type = std::array<newmenu_item, 1>;
	struct glitz_menu : items_type, passive_newmenu
	{
		glitz_menu(grs_canvas &canvas, menu_subtitle subtitle) :
			items_type{{
				newmenu_item::nm_item_menu{TXT_OK},
			}},
			passive_newmenu(menu_title{nullptr}, subtitle, menu_filename{GLITZ_BACKGROUND}, tiny_mode_flag::normal, tab_processing_flag::ignore, adjusted_citem::create(*static_cast<items_type *>(this), 0), canvas, draw_box_flag::menu_background)
		{
		}
	};
	run_blocking_newmenu<glitz_menu>(*grd_curcanv, menu_subtitle{msg});
}

}

namespace dsx {
#if DXX_BUILD_DESCENT == 2
namespace {
void do_screen_message_fmt(const char *) = delete;

dxx_compiler_attribute_format_printf(1, 2)
static void do_screen_message_fmt(const char *fmt, ...)
{
	va_list arglist;
	char msg[1024];
	va_start(arglist, fmt);
	vsnprintf(msg, sizeof(msg), fmt, arglist);
	va_end(arglist);
	do_screen_message(msg);
}

//	-----------------------------------------------------------------------------------------------------
// called when the player is starting a new level for normal game mode and restore state
//	Need to deal with whether this is the first time coming to this level or not.  If not the
//	first time, instead of initializing various things, need to do a game restore for all the
//	robots, powerups, walls, doors, etc.
static void StartNewLevelSecret(int level_num, int page_in_textures)
{
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;

	last_drawn_cockpit = cockpit_mode_t{UINT8_MAX};

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
			if (PHYSFS_exists(SECRETC_FILENAME))
			{
				do_screen_message(TXT_SECRET_EXIT);
			} else {
				do_screen_message_fmt("Secret level already destroyed.\nAdvancing to level %i.", Current_level_num+1);
			}
		}
	}

	LoadLevel(level_num,page_in_textures);

	Assert(Current_level_num == level_num); // make sure level set right

	HUD_clear_messages();

	automap_clear_visited(LevelUniqueAutomapState);

	Viewer = &get_local_plrobj();

	gameseq_remove_unused_players(LevelSharedRobotInfoState.Robot_info);

	Game_suspended = 0;

	LevelUniqueControlCenterState.Control_center_destroyed = 0;

	init_cockpit();
	reset_palette_add();

	if (First_secret_visit || (Newdemo_state == ND_STATE_PLAYBACK)) {
		init_robots_for_level();
		init_ai_objects(LevelSharedRobotInfoState.Robot_info);
		init_smega_detonates();
		init_morphs(LevelUniqueObjectState.MorphObjectState);
		init_all_matcens();
		reset_special_effects();
		StartSecretLevel();
	} else {
		if (PHYSFS_exists(SECRETC_FILENAME))
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
			do_screen_message_fmt("Secret level already destroyed.\nAdvancing to level %i.", Current_level_num+1);
			return;
		}
	}

	if (First_secret_visit) {
		copy_defaults_to_robot_all(LevelSharedRobotInfoState.Robot_info);
	}

	init_controlcen_for_level(LevelSharedRobotInfoState.Robot_info);

	// Say player can use FLASH cheat to mark path to exit.
	LevelUniqueObjectState.Level_path_created = 0;

	First_secret_visit = 0;
}

static int Entered_from_level;

static void filter_objects_from_level(const d_powerup_info_array &Powerup_info, const d_vclip_array &Vclip, fvmobjptr &vmobjptr)
{
	for (auto &obj : vmobjptr)
	{
		if (obj.type == OBJ_POWERUP)
		{
			const auto powerup_id = get_powerup_id(obj);
			if (powerup_id == powerup_type_t::POW_FLAG_RED || powerup_id == powerup_type_t::POW_FLAG_BLUE)
				bash_to_shield(Powerup_info, Vclip, obj);
		}
	}
}

}

// ---------------------------------------------------------------------------------------------------------------
//	Called from switch.c when player is on a secret level and hits exit to return to base level.
window_event_result ExitSecretLevel()
{
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	auto result = window_event_result::handled;

	if (Newdemo_state == ND_STATE_PLAYBACK)
		return window_event_result::ignored;

	const auto g{Game_wind};
	if (g)
		g->set_visible(0);

	if (!LevelUniqueControlCenterState.Control_center_destroyed)
	{
		state_save_all(secret_save::c, blind_save::no);
	}

	if (PHYSFS_exists(SECRETB_FILENAME))
	{
		do_screen_message_fmt(TXT_SECRET_RETURN);
		auto &player_info = get_local_plrobj().ctype.player_info;
		const auto pw_save = player_info.Primary_weapon;
		const auto sw_save = player_info.Secondary_weapon;
		state_restore_all(1, secret_restore::survived, SECRETB_FILENAME, blind_save::no);
		player_info.Primary_weapon = pw_save;
		player_info.Secondary_weapon = sw_save;
	} else {
		// File doesn't exist, so can't return to base level.  Advance to next one.
		if (Entered_from_level == Current_mission->last_level)
		{
			DoEndGame();
			result = window_event_result::close;
		}
		else {
			do_screen_message_fmt(TXT_SECRET_ADVANCE);
			StartNewLevel(Entered_from_level+1);
		}
	}

	if (g)
		g->set_visible(1);
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
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	int i;

	Assert(! (Game_mode & GM_MULTI) );

	const auto g{Game_wind};
	if (g)
		g->set_visible(0);

	digi_play_sample( sound_effect::SOUND_SECRET_EXIT, F1_0 );	// after above call which stops all sounds
	
	Entered_from_level = Current_level_num;

	if (LevelUniqueControlCenterState.Control_center_destroyed)
		DoEndLevelScoreGlitz();

	if (Newdemo_state != ND_STATE_PLAYBACK)
		state_save_all(secret_save::b, blind_save::no);	//	Not between levels (ie, save all), IS a secret level, NO filename override

	//	Find secret level number to go to, stuff in Next_level_num.
	int8_t Next_level_num{};
	for (i = 0; i < -Current_mission->last_secret_level; ++i)
		if (Current_mission->secret_level_table[i] == Current_level_num)
		{
			Next_level_num = -(i+1);
			break;
		}
		else if (Current_mission->secret_level_table[i] > Current_level_num)
		{	//	Allows multiple exits in same group.
			Next_level_num = -i;
			break;
		}

	if (! (i < -Current_mission->last_secret_level))		//didn't find level, so must be last
		Next_level_num = Current_mission->last_secret_level;

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
	if (g)
		g->set_visible(1);
	reset_time();
}
#endif

//called when the player has finished a level
window_event_result (PlayerFinishedLevel)(
#if DXX_BUILD_DESCENT == 1
	const next_level_request_secret_flag secret_flag
#endif
	)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	const auto g{Game_wind};
	if (g)
		g->set_visible(0);

	//credit the player for hostages
	auto &player_info = get_local_plrobj().ctype.player_info;
	player_info.mission.hostages_rescued_total += player_info.mission.hostages_on_board;
#if DXX_BUILD_DESCENT == 1
	if (!(Game_mode & GM_MULTI) && secret_flag != next_level_request_secret_flag::only_normal_level) {
		using items_type = std::array<newmenu_item, 1>;
		struct message_menu : items_type, passive_newmenu
		{
			message_menu(grs_canvas &canvas) :
				items_type{{
					newmenu_item::nm_item_text{" "},			//TXT_SECRET_EXIT;
				}},
				passive_newmenu(menu_title{nullptr}, menu_subtitle{TXT_SECRET_EXIT}, menu_filename{Menu_pcx_name}, tiny_mode_flag::normal, tab_processing_flag::ignore, adjusted_citem::create(*static_cast<items_type *>(this), 0), canvas, draw_box_flag::none)
			{
			}
		};
		run_blocking_newmenu<message_menu>(*grd_curcanv);
	}
#elif DXX_BUILD_DESCENT == 2
	constexpr auto secret_flag = next_level_request_secret_flag::only_normal_level;
#endif
	if (Game_mode & GM_NETWORK)
		get_local_player().connected = player_connection_status::waiting; // Finished but did not die

	last_drawn_cockpit = cockpit_mode_t{UINT8_MAX};

	auto result = AdvanceLevel(secret_flag);				//now go on to the next one (if one)

	if (g)
		g->set_visible(1);
	reset_time();

	return result;
}

#if DXX_BUILD_DESCENT == 2
#define MOVIE_REQUIRED 1
#define ENDMOVIE "end"
#endif

namespace {

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
#if DXX_BUILD_DESCENT == 2
		auto played = PlayMovie(ENDMOVIE ".tex", ENDMOVIE ".mve", play_movie_warn_missing::urgent);
		if (played == movie_play_status::skipped)
#endif
		{
			do_end_briefing_screens(Current_mission->ending_text_filename);
		}
        }
        else if (!(Game_mode & GM_MULTI))    //not multi
        {
		char tname[FILENAME_LEN];

		do_end_briefing_screens (Current_mission->ending_text_filename);

		//try doing special credits
		snprintf(tname, sizeof(tname), "%s.ctb", &*Current_mission->filename);
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
#if DXX_BUILD_DESCENT == 2
		load_palette(D2_DEFAULT_PALETTE, load_palette_use::background, load_palette_change_screen::delayed);
#endif
		scores_maybe_add_player();
	}
}

//called to go to the next level (if there is one)
//if secret_flag is true, advance to secret level, else next normal one
//	Return true if game over.
static window_event_result (AdvanceLevel)(
#if DXX_BUILD_DESCENT == 1
	next_level_request_secret_flag secret_flag
#endif
	)
{
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	auto rval = window_event_result::handled;

#if DXX_BUILD_DESCENT == 2
	// Loading a level can write over homing_flag.
	// So for simplicity, reset the homing weapon cheat here.
	if (cheats.homingfire)
	{
		cheats.homingfire = 0;
		weapons_homing_all_reset();
	}
#endif
	if (Current_level_num != Current_mission->last_level)
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

	LevelUniqueControlCenterState.Control_center_destroyed = 0;

	if (Game_mode & GM_MULTI)
	{
		int result;
		result = multi::dispatch->end_current_level(
#if DXX_BUILD_DESCENT == 1
			&secret_flag
#endif
			); // Wait for other players to reach this point
		if (result) // failed to sync
		{
			// check if player has finished the game
			return Current_level_num == Current_mission->last_level ? window_event_result::close : rval;
		}
	}

	if (Current_level_num == Current_mission->last_level)
	{		//player has finished the game!

		DoEndGame();
		rval = window_event_result::close;

	} else {
#if DXX_BUILD_DESCENT == 1
		const auto Next_level_num = find_next_level(secret_flag, Current_level_num, *Current_mission.get());
#elif DXX_BUILD_DESCENT == 2
		int8_t Next_level_num;

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

}

window_event_result DoPlayerDead()
{
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
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
	}
	else
	{				//Note link to above else!
		-- plr.lives;
		if (plr.lives == 0)
		{
			if (PLAYING_BUILTIN_MISSION)
				scores_maybe_add_player();
			if (pause)
				start_time();
			return window_event_result::close;
		}
	}

	if (LevelUniqueControlCenterState.Control_center_destroyed)
	{
		//clear out stuff so no bonus
		auto &plrobj = get_local_plrobj();
		auto &player_info = plrobj.ctype.player_info;
		player_info.mission.hostages_on_board = 0;
		player_info.energy = 0;
		plrobj.shields = 0;
		plr.connected = player_connection_status::died_in_mine;

		do_screen_message(TXT_DIED_IN_MINE); // Give them some indication of what happened
#if DXX_BUILD_DESCENT == 2
		if (Current_level_num < 0) {
			if (PHYSFS_exists(SECRETB_FILENAME))
			{
				do_screen_message_fmt(TXT_SECRET_RETURN);
				state_restore_all(1, secret_restore::died, SECRETB_FILENAME, blind_save::no);			//	2 means you died
				set_pos_from_return_segment();
				plr.lives--;						//	re-lose the life, get_local_player().lives got written over in restore.
			} else {
				if (Entered_from_level == Current_mission->last_level)
				{
					DoEndGame();
					result = window_event_result::close;
				}
				else {
					do_screen_message_fmt(TXT_SECRET_ADVANCE);
					StartNewLevel(Entered_from_level+1);
					init_player_stats_new_ship(Player_num);	//	New, MK, 05/29/96!, fix bug with dying in secret level, advance to next level, keep powerups!
				}
			}
		} else 
#endif
                {

					const auto g{Game_wind};
					if (g)
						g->set_visible(0);
			result = AdvanceLevel(next_level_request_secret_flag::only_normal_level);			//if finished, go on to next level

			init_player_stats_new_ship(Player_num);
			last_drawn_cockpit = cockpit_mode_t{UINT8_MAX};
			if (g)
				g->set_visible(1);
		}
#if DXX_BUILD_DESCENT == 2
	} else if (Current_level_num < 0) {
		if (PHYSFS_exists(SECRETB_FILENAME))
		{
			do_screen_message_fmt(TXT_SECRET_RETURN);
			if (!LevelUniqueControlCenterState.Control_center_destroyed)
				state_save_all(secret_save::c, blind_save::no);
			state_restore_all(1, secret_restore::died, SECRETB_FILENAME, blind_save::no);
			set_pos_from_return_segment();
			plr.lives--;						//	re-lose the life, get_local_player().lives got written over in restore.
		} else {
			do_screen_message(TXT_DIED_IN_MINE); // Give them some indication of what happened
			if (Entered_from_level == Current_mission->last_level)
			{
				DoEndGame();
				result = window_event_result::close;
			}
			else {
				do_screen_message_fmt(TXT_SECRET_ADVANCE);
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
#if DXX_BUILD_DESCENT == 1
window_event_result StartNewLevelSub(const d_robot_info_array &Robot_info, const int level_num, const int page_in_textures)
#elif DXX_BUILD_DESCENT == 2
window_event_result StartNewLevelSub(const d_robot_info_array &Robot_info, const int level_num, const int page_in_textures, const secret_restore secret_flag)
#endif
{
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	if (!(Game_mode & GM_MULTI)) {
		last_drawn_cockpit = cockpit_mode_t{UINT8_MAX};
	}
#if DXX_BUILD_DESCENT == 1
	static constexpr std::integral_constant<secret_restore, secret_restore::none> secret_flag{};
#elif DXX_BUILD_DESCENT == 2
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
		multi_prep_level_objects(Powerup_info, Vclip);
		if (multi::dispatch->level_sync() == window_event_result::close) // After calling this, Player_num is set
		{
			songs_play_song(song_number::title, 1); // level song already plays but we fail to start level...
			return window_event_result::close;
		}
	}

	HUD_clear_messages();

	automap_clear_visited(LevelUniqueAutomapState);

	LevelUniqueObjectState.accumulated_robots = count_number_of_robots(Objects.vcptr);
	GameUniqueState.accumulated_robots += LevelUniqueObjectState.accumulated_robots;
	LevelUniqueObjectState.total_hostages = count_number_of_hostages(Objects.vcptr);
	GameUniqueState.total_hostages += LevelUniqueObjectState.total_hostages;
	init_player_stats_level(get_local_player(), get_local_plrobj(), secret_flag);

#if DXX_BUILD_DESCENT == 1
	gr_use_palette_table( "palette.256" );
#elif DXX_BUILD_DESCENT == 2
	load_palette(Current_level_palette.line(), load_palette_use::background, load_palette_change_screen::delayed);
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

	gameseq_remove_unused_players(Robot_info);

	Game_suspended = 0;

	LevelUniqueControlCenterState.Control_center_destroyed = 0;

#if DXX_BUILD_DESCENT == 2
	set_screen_mode(SCREEN_GAME);
#endif

	init_cockpit();
	init_robots_for_level();
	init_ai_objects(Robot_info);
	init_morphs(LevelUniqueObjectState.MorphObjectState);
	init_all_matcens();
	reset_palette_add();
	LevelUniqueStuckObjectState.init_stuck_objects();
#if DXX_BUILD_DESCENT == 2
	init_smega_detonates();
	init_thief_for_level();
	if (!(Game_mode & GM_MULTI))
		filter_objects_from_level(Powerup_info, Vclip, vmobjptr);
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

	copy_defaults_to_robot_all(Robot_info);
	init_controlcen_for_level(Robot_info);

	//	Say player can use FLASH cheat to mark path to exit.
	LevelUniqueObjectState.Level_path_created = 0;

	// Initialise for palette_restore()
	// Also takes care of nm_draw_background() possibly being called
	if (!((Game_mode & GM_MULTI) && (Newdemo_state != ND_STATE_PLAYBACK)))
		full_palette_save();

	if (!Game_wind)
		game();

	return window_event_result::handled;
}

void bash_to_shield(const d_powerup_info_array &Powerup_info, const d_vclip_array &Vclip, object_base &i)
{
	set_powerup_id(Powerup_info, Vclip, i, powerup_type_t::POW_SHIELD_BOOST);
}

#if DXX_BUILD_DESCENT == 2
namespace {

struct intro_movie_t {
	int	level_num;
	char movie_name[8];
};

constexpr std::array<intro_movie_t, 7> intro_movie{{
	{ 1, "PLA.MVE"},
	{ 5, "PLB.MVE"},
	{ 9, "PLC.MVE"},
	{13, "PLD.MVE"},
	{17, "PLE.MVE"},
	{21, "PLF.MVE"},
	{24, "PLG.MVE"}
}};

static void ShowLevelIntro(int level_num)
{
	//if shareware, show a briefing?

	if (!(Game_mode & GM_MULTI)) {
		palette_array_t save_pal = gr_palette;

		if (PLAYING_BUILTIN_MISSION) {

			if (is_SHAREWARE || is_MAC_SHARE)
			{
				if (level_num != 1)
					return;
			}
			else if (is_D2_OEM)
			{
				if (level_num != 1)
					return;
				if (intro_played)
					return;
			}
			else // full version
			{
				range_for (auto &i, intro_movie)
				{
					if (i.level_num == level_num)
					{
						Screen_mode = -1;
						PlayMovie({}, i.movie_name, play_movie_warn_missing::urgent);
						break;
					}
				}
			}
		}
		//else not the built-in mission (maybe d1, too).
		/* Play a level-appropriate briefing, whether built-in, add-on,
		 * or Descent 1.
		 */
		do_briefing_screens(Current_mission->briefing_text_filename, level_num);

		gr_palette = save_pal;
	}
}

//	---------------------------------------------------------------------------
//	If starting a level which appears in the Current_mission->secret_level_table, then set First_secret_visit.
//	Reason: On this level, if player goes to a secret level, he will be going to a different
//	secret level than he's ever been to before.
//	Sets the global First_secret_visit if necessary.  Otherwise leaves it unchanged.
static void maybe_set_first_secret_visit(int level_num)
{
	range_for (auto &i, unchecked_partial_range(Current_mission->secret_level_table.get(), Current_mission->n_secret_levels))
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
window_event_result StartNewLevel(int level_num)
{
	hide_menus();

	GameTime64 = 0;
	/* Autosave is permitted immediately on entering a new level */
	state_set_immediate_autosave(GameUniqueState);
	ThisLevelTime = {};

#if DXX_BUILD_DESCENT == 1
	if (!(Game_mode & GM_MULTI)) {
		do_briefing_screens(Current_mission->briefing_text_filename, level_num);
	}
#elif DXX_BUILD_DESCENT == 2
	if (level_num > 0) {
		maybe_set_first_secret_visit(level_num);
	}

	ShowLevelIntro(level_num);
#endif

	return StartNewLevelSub(LevelSharedRobotInfoState.Robot_info, level_num, 1, secret_restore::none);

}

namespace {

class respawn_locations
{
	typedef std::pair<int, fix> site;
	unsigned max_usable_spawn_sites;
	per_player_array<site> sites;
public:
	respawn_locations(fvmobjptr &vmobjptr, fvcsegptridx &vcsegptridx)
	{
		const auto player_num{Player_num};
		const auto find_closest_player = [player_num, &vmobjptr, &vcsegptridx](const obj_position &candidate) {
			fix closest_dist = INT32_MAX;
			const auto &&candidate_segp = vcsegptridx(candidate.segnum);
			for (playernum_t i = N_players; i--;)
			{
				if (i == player_num)
					continue;
				const auto &&objp = vmobjptr(vcplayerptr(i)->objnum);
				if (objp->type != OBJ_PLAYER)
					continue;
				const auto dist = find_connected_distance(objp->pos, candidate_segp.absolute_sibling(objp->segnum), candidate.pos, candidate_segp, -1, wall_is_doorway_mask::None);
				if (dist >= 0 && closest_dist > dist)
					closest_dist = dist;
			}
			return closest_dist;
		};
		const auto max_spawn_sites = std::min<unsigned>(NumNetPlayerPositions, sites.size());
		for (playernum_t i = max_spawn_sites; i--;)
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
	const site &operator[](const playernum_t i) const
	{
		return sites[i];
	}
};

//initialize the player object position & orientation (at start of game, or new ship)
static void InitPlayerPosition(fvmobjptridx &vmobjptridx, fvmsegptridx &vmsegptridx, int random_flag)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	reset_cruise();
	int NewPlayer{0};

	if (! ((Game_mode & GM_MULTI) && !(Game_mode&GM_MULTI_COOP)) ) // If not deathmatch
		NewPlayer = Player_num;
	else if (random_flag == 1)
	{
		const respawn_locations locations(vmobjptr, vcsegptridx);
		if (!locations.get_usable_sites())
			return;
		uint_fast32_t trys{0};
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
		reset_player_object(*ConsoleObject);
		return;
	}
	Assert(NewPlayer >= 0);
	Assert(NewPlayer < NumNetPlayerPositions);
	ConsoleObject->pos = Player_init[NewPlayer].pos;
	ConsoleObject->orient = Player_init[NewPlayer].orient;
	obj_relink(vmobjptr, vmsegptr, vmobjptridx(ConsoleObject), vmsegptridx(Player_init[NewPlayer].segnum));
	reset_player_object(*ConsoleObject);
}

}

//	-----------------------------------------------------------------------------------------------------
//	Initialize default parameters for one robot, copying from Robot_info to *objp.
//	What about setting size!?  Where does that come from?
void copy_defaults_to_robot(const d_robot_info_array &Robot_info, object_base &objp)
{
	assert(objp.type == OBJ_ROBOT);
	const auto objid = get_robot_id(objp);

	auto &robptr = Robot_info[objid];

	//	Boost shield for Thief and Buddy based on level.
	fix shields = robptr.strength;

#if DXX_BUILD_DESCENT == 2
	const auto &Difficulty_level = GameUniqueState.Difficulty_level;
	if ((robot_is_thief(robptr)) || (robot_is_companion(robptr))) {
		shields = (shields * (abs(Current_level_num)+7))/8;

		if (robot_is_companion(robptr)) {
			//	Now, scale guide-bot hits by skill level
			switch (Difficulty_level) {
				case Difficulty_level_type::_0:
					shields = i2f(20000);
					break;		//	Trainee, basically unkillable
				case Difficulty_level_type::_1:
					shields *= 3;
					break;		//	Rookie, pretty dang hard
				case Difficulty_level_type::_2:
					shields *= 2;
					break;		//	Hotshot, a bit tough
				default:	break;
			}
		}
	} else if (robptr.boss_flag != boss_robot_id::None)	//	MK, 01/16/95, make boss shields lower on lower diff levels.
	{
	//	Additional wimpification of bosses at Trainee
		shields = shields / (NDL + 3) * (Difficulty_level != Difficulty_level_type::_0 ? underlying_value(Difficulty_level) + 4 : 2);
	}
#endif
	objp.shields = shields;
}

namespace {

//	-----------------------------------------------------------------------------------------------------
//	Copy all values from the robot info structure to all instances of robots.
//	This allows us to change bitmaps.tbl and have these changes manifested in existing robots.
//	This function should be called at level load time.
static void copy_defaults_to_robot_all(const d_robot_info_array &Robot_info)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	range_for (object_base &objp, vmobjptr)
	{
		if (objp.type == OBJ_ROBOT)
			copy_defaults_to_robot(Robot_info, objp);
	}
}

//	-----------------------------------------------------------------------------------------------------
//called when the player is starting a level (new game or new ship)
static void StartLevel(int random_flag)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;
	assert(Player_dead_state == player_dead_state::no);

	InitPlayerPosition(vmobjptridx, vmsegptridx, random_flag);

	verify_console_object();

	ConsoleObject->control_source	= object::control_type::flying;
	ConsoleObject->movement_source	= object::movement_type::physics;

	// create_player_appearance_effect(ConsoleObject);
	Do_appearance_effect = 1;

	if (Game_mode & GM_MULTI)
	{
		if (Game_mode & GM_MULTI_COOP)
			multi_send_score();
	 	multi_send_reappear();
		multi::dispatch->do_protocol_frame(1, 1);
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
}
