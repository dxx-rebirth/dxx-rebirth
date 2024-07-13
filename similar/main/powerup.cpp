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
 * Code for powerup objects.
 *
 */


#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "maths.h"
#include "vecmat.h"
#include "gr.h"
#include "3d.h"
#include "dxxerror.h"
#include "inferno.h"
#include "object.h"
#include "game.h"
#include "fireball.h"
#include "powerup.h"
#include "gauges.h"
#include "sounds.h"
#include "player.h"
#include "physfs-serial.h"
#include "text.h"
#include "weapon.h"
#include "laser.h"
#include "scores.h"
#include "multi.h"
#include "segment.h"
#include "controls.h"
#include "kconfig.h"
#include "newdemo.h"
#include "escort.h"
#if DXX_USE_EDITOR
#include "gr.h"	//	for powerup outline drawing
#endif
#include "hudmsg.h"
#include "playsave.h"
#include "d_enumerate.h"
#include "d_levelstate.h"
#include "partial_range.h"
#include "vclip.h"

namespace dcx {
uint8_t N_powerup_types;
}
namespace dsx {
d_powerup_info_array Powerup_info;

//process this powerup for this frame
void do_powerup_frame(const d_vclip_array &Vclip, const vmobjptridx_t obj)
{
	vclip_info *vci = &obj->rtype.vclip_info;

#if defined(DXX_BUILD_DESCENT_I)
	const fix fudge = 0;
#elif defined(DXX_BUILD_DESCENT_II)
	long objnum = obj;
	const fix fudge = (FrameTime * (objnum&3)) >> 4;
#endif
	
	auto &vc = Vclip[vci->vclip_num];
	const auto vc_frame_time = vc.frame_time;
	if (vc_frame_time > 0)
	{
	const auto vc_num_frames1 = vc.num_frames - 1;
	vci->frametime -= FrameTime+fudge;
	
	while (vci->frametime < 0 ) {

		vci->frametime += vc_frame_time;
		if (vci->framenum > vc_num_frames1)
			vci->framenum=0;

#if defined(DXX_BUILD_DESCENT_II)
		if (objnum&1)
		{
			if (-- vci->framenum > vc_num_frames1)
				vci->framenum = vc_num_frames1;
		}
		else
#endif
		{
			if (vci->framenum >= vc_num_frames1)
				vci->framenum=0;
			else
				vci->framenum++;
		}
	}
	}

	if (obj->lifeleft <= 0) {
		object_create_explosion_without_damage(Vclip, vmsegptridx(obj->segnum), obj->pos, F1_0 * 7 / 2, vclip_index::powerup_disappearance);

		if (const auto sound_num = Vclip[vclip_index::powerup_disappearance].sound_num; sound_num > -1)
			digi_link_sound_to_object(sound_num, obj, 0, F1_0, sound_stack::allow_stacking);
	}
}

void draw_powerup(const d_vclip_array &Vclip, grs_canvas &canvas, const object_base &obj)
{
	auto &vci = obj.rtype.vclip_info;
	draw_object_blob(GameBitmaps, *Viewer, canvas, obj, Vclip[vci.vclip_num].frames[vci.framenum]);
}

namespace {

static void _powerup_basic_nonhud(int redadd, int greenadd, int blueadd, int score)
{
	PALETTE_FLASH_ADD(redadd,greenadd,blueadd);
	add_points_to_score(ConsoleObject->ctype.player_info, score, Game_mode);
}

__attribute_format_printf(5, 6)
void (powerup_basic)(int redadd, int greenadd, int blueadd, int score, const char *format, ...)
{
	va_list	args;

	va_start(args, format );
	HUD_init_message_va(HM_DEFAULT, format, args);
	va_end(args);
	_powerup_basic_nonhud(redadd, greenadd, blueadd, score);
}

}

void powerup_basic_str(int redadd, int greenadd, int blueadd, int score, const std::span<const char> str)
{
	HUD_init_message_literal(HM_DEFAULT, str);
	_powerup_basic_nonhud(redadd, greenadd, blueadd, score);
}

//#ifndef RELEASE
//	Give the megawow powerup!
void do_megawow_powerup(object &plrobj, const int quantity)
{
	powerup_basic_str(30, 0, 30, 1, "MEGA-WOWIE-ZOWIE!");
	auto &player_info = plrobj.ctype.player_info;
#if defined(DXX_BUILD_DESCENT_I)
	player_info.primary_weapon_flags = (HAS_LASER_FLAG | HAS_VULCAN_FLAG | HAS_SPREADFIRE_FLAG | HAS_PLASMA_FLAG | HAS_FUSION_FLAG);
#elif defined(DXX_BUILD_DESCENT_II)
	player_info.primary_weapon_flags = (HAS_LASER_FLAG | HAS_VULCAN_FLAG | HAS_SPREADFIRE_FLAG | HAS_PLASMA_FLAG | HAS_FUSION_FLAG) | (HAS_GAUSS_FLAG | HAS_HELIX_FLAG | HAS_PHOENIX_FLAG | HAS_OMEGA_FLAG);
#endif
	player_info.vulcan_ammo = VULCAN_AMMO_MAX;

	auto &secondary_ammo = player_info.secondary_ammo;
	range_for (auto &i, partial_range(secondary_ammo, 3u))
		i = quantity;

	range_for (auto &i, partial_range(secondary_ammo, 3u, secondary_ammo.size()))
		i = quantity/5;

	player_info.energy = F1_0*200;
	plrobj.shields = F1_0*200;
	player_info.powerup_flags |= PLAYER_FLAGS_QUAD_LASERS;
#if defined(DXX_BUILD_DESCENT_I)
	const auto laser_level = MAX_LASER_LEVEL;
#elif defined(DXX_BUILD_DESCENT_II)
	player_info.Omega_charge = MAX_OMEGA_CHARGE;
	if (game_mode_hoard(Game_mode))
		player_info.hoard.orbs = player_info.max_hoard_orbs;
	const auto laser_level = MAX_SUPER_LASER_LEVEL;
#endif
	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_laser_level(player_info.laser_level, laser_level);
	player_info.laser_level = laser_level;
}
//#endif

namespace {

static int pick_up_energy(player_info &player_info)
{
	int	used=0;

	auto &energy = player_info.energy;
	if (energy < MAX_ENERGY) {
		fix boost;
		const auto Difficulty_level = GameUniqueState.Difficulty_level;
		boost = 3 * F1_0 + 3 * F1_0 * (NDL - underlying_value(Difficulty_level));
#if defined(DXX_BUILD_DESCENT_II)
		if (Difficulty_level == Difficulty_level_type::_0)
			boost += boost/2;
#endif
		energy += boost;
		if (energy > MAX_ENERGY)
			energy = MAX_ENERGY;
		powerup_basic(15, 15, 7, ENERGY_SCORE, "%s %s %d", TXT_ENERGY, TXT_BOOSTED_TO, f2ir(energy));
		used=1;
	} else
		HUD_init_message(HM_DEFAULT|HM_REDUNDANT|HM_MAYDUPL, TXT_MAXED_OUT,TXT_ENERGY);

	return used;
}

static int pick_up_primary_or_energy(player_info &player_info, const primary_weapon_index_t weapon_index)
{
	const auto used = pick_up_primary(player_info, weapon_index);
	if (used || (Game_mode & GM_MULTI))
		return used;
	return pick_up_energy(player_info);
}

static int pick_up_vulcan_ammo(player_info &player_info)
{
	int	used=0;
	if (pick_up_vulcan_ammo(player_info, VULCAN_AMMO_AMOUNT, false)) {
		powerup_basic(7, 14, 21, VULCAN_AMMO_SCORE, "%s!", TXT_VULCAN_AMMO);
		used = 1;
	} else {
		const auto max = PLAYER_MAX_AMMO(player_info.powerup_flags, VULCAN_AMMO_MAX);
		HUD_init_message(HM_DEFAULT | HM_REDUNDANT | HM_MAYDUPL, "%s %d %s!", TXT_ALREADY_HAVE, vulcan_ammo_scale(max), TXT_VULCAN_ROUNDS);
		used = 0;
	}
	return used;
}

static int pick_up_key(const int r, const int g, const int b, player_flags &player_flags, const PLAYER_FLAG key_flag, const char *const key_name, const powerup_type_t id)
{
	if (player_flags & key_flag)
		return 0;
	player_flags |= key_flag;
	powerup_basic(r, g, b, KEY_SCORE, "%s %s", key_name, TXT_ACCESS_GRANTED);
	multi_digi_play_sample(Powerup_info[id].hit_sound, F1_0);
#if defined(DXX_BUILD_DESCENT_II)
	auto &BuddyState = LevelUniqueObjectState.BuddyState;
	invalidate_escort_goal(BuddyState);
#endif
	return (Game_mode & GM_MULTI) ? 0 : 1;
}

//	returns true if powerup consumed
#if defined(DXX_BUILD_DESCENT_II)
template <int r, int g, int b>
struct player_hit_basic_silent_powerup
{
	const std::span<const char> desc_pickup;
	player_hit_basic_silent_powerup(const std::span<const char> p) :
		desc_pickup(p)
	{
	}
	void report() const
	{
		powerup_basic_str(r, g, b, 0, desc_pickup);
	}
	template <PLAYER_FLAG player_flag>
		void pickup(player_flags &powerup_flags) const
		{
			powerup_flags |= player_flag;
			report();
		}
};

template <int r, int g, int b, powerup_type_t id>
struct player_hit_basic_sound_powerup : player_hit_basic_silent_powerup<r, g, b>
{
	using base_type = player_hit_basic_silent_powerup<r, g, b>;
	using base_type::base_type;
	template <PLAYER_FLAG player_flag>
		void pickup(player_flags &powerup_flags) const
		{
			multi_digi_play_sample(Powerup_info[id].hit_sound, F1_0);
			base_type::template pickup<player_flag>(powerup_flags);
		}
};

using player_hit_silent_rb_powerup = player_hit_basic_silent_powerup<15, 0, 15>;

struct player_hit_afterburner_powerup : player_hit_basic_sound_powerup<15, 15, 15, powerup_type_t::POW_AFTERBURNER>
{
	using base_type = player_hit_basic_sound_powerup<15, 15, 15, powerup_type_t::POW_AFTERBURNER>;
	using base_type::base_type;
	template <PLAYER_FLAG player_flag>
		void pickup(player_flags &powerup_flags) const
		{
			Afterburner_charge = f1_0;
			base_type::template pickup<player_flag>(powerup_flags);
		}
};

struct player_hit_headlight_powerup
{
	/* Template parameter unused, but required for signature
	 * compatibility with the other player_hit_* structures.
	 */
	template <PLAYER_FLAG>
		void pickup(player_flags &powerup_flags) const
		{
			process(powerup_flags);
		}
	void process(player_flags &powerup_flags) const
	{
		const auto active{PlayerCfg.HeadlightActiveDefault};
		powerup_flags |= active
			? PLAYER_FLAG::HEADLIGHT_PRESENT_AND_ON
			: PLAYER_FLAG::HEADLIGHT;
		powerup_basic(15, 0, 15, 0, "HEADLIGHT BOOST! (Headlight is O%s)", active ? "N" : "FF");
		multi_digi_play_sample(Powerup_info[powerup_type_t::POW_HEADLIGHT].hit_sound, F1_0);
		if (active && (Game_mode & GM_MULTI))
			multi_send_flags (Player_num);
	}
};

template <team_number TEAM>
static int player_hit_flag_powerup(player_info &player_info, const std::span<const char> desc)
{
	if (!game_mode_capture_flag(Game_mode))
		return 0;
	const auto pnum = Player_num;
	if (get_team(pnum) == TEAM)
	{
		player_info.powerup_flags |= PLAYER_FLAGS_FLAG;
		powerup_basic_str(15, 0, 15, 0, desc);
		multi_send_got_flag(pnum);
		return 1;
	}
	return 0;
}
#endif

struct player_hit_quadlaser_powerup
{
	/* Template parameter unused, but required for signature
	 * compatibility with the other player_hit_* structures.
	 */
	template <PLAYER_FLAG>
		void pickup(player_flags &powerup_flags) const
		{
			process(powerup_flags);
		}
	void process(player_flags &powerup_flags) const
	{
		powerup_flags |= PLAYER_FLAGS_QUAD_LASERS;
		powerup_basic(15, 15, 7, QUAD_FIRE_SCORE, "%s!", TXT_QUAD_LASERS);
		update_laser_weapon_info();
	}
};

static int player_has_powerup(player_info &player_info, const char *const desc_have)
{
	HUD_init_message(HM_DEFAULT | HM_REDUNDANT | HM_MAYDUPL, "%s %s!", TXT_ALREADY_HAVE, desc_have);
	return (Game_mode & GM_MULTI) ? 0 : pick_up_energy(player_info);
}

template <PLAYER_FLAG player_flag, typename F>
static int player_hit_powerup(player_info &player_info, const char *const desc_have, const F &&pickup)
{
	auto &powerup_flags = player_info.powerup_flags;
	return (powerup_flags & player_flag)
		? player_has_powerup(player_info, desc_have)
		: (pickup.template pickup<player_flag>(powerup_flags), 1);
}

}

int do_powerup(const vmobjptridx_t obj)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	auto &vmobjptr = Objects.vmptr;
	int used=0;
	int special_used=0;		//for when hitting vulcan cannon gets vulcan ammo

	if (Player_dead_state != player_dead_state::no ||
		ConsoleObject->type == OBJ_GHOST ||
		get_local_plrobj().shields < 0)
		return 0;

	if ((obj->ctype.powerup_info.flags & PF_SPAT_BY_PLAYER) && obj->ctype.powerup_info.creation_time>0 && GameTime64<obj->ctype.powerup_info.creation_time+i2f(2))
		return 0;		//not enough time elapsed

	if (Game_mode & GM_MULTI)
	{
		/*
		 * The fact: Collecting a powerup is decided Client-side and due to PING it takes time for other players to know if one collected a powerup actually. This may lead to the case two players collect the same powerup!
		 * The solution: Let us check if someone else is closer to a powerup and if so, do not collect it.
		 * NOTE: Player positions computed by 'shortpos' and PING can still cause a small margin of error.
		 */
		vms_vector tvec;
		const fix mydist = vm_vec_normalized_dir(tvec, obj->pos, ConsoleObject->pos);

		for (auto &&[i, plr] : enumerate(Players))
		{
			if (i == Player_num)
				continue;
			if (plr.connected != player_connection_status::playing)
				continue;
			auto &o = *vcobjptr(plr.objnum);
			if (o.type == OBJ_GHOST)
				continue;
			if (mydist > vm_vec_normalized_dir(tvec, obj->pos, o.pos))
				return 0;
		}
	}

	auto &plrobj = get_local_plrobj();
	auto &player_info = plrobj.ctype.player_info;
	auto id = get_powerup_id(obj);
	switch (id)
	{
		case powerup_type_t::POW_EXTRA_LIFE:
			get_local_player().lives++;
			{
				const auto &&m = TXT_EXTRA_LIFE;
				powerup_basic_str(15, 15, 15, 0, {m, strlen(m)});
			}
			used=1;
			break;
		case powerup_type_t::POW_ENERGY:
			used = pick_up_energy(player_info);
			break;
		case powerup_type_t::POW_SHIELD_BOOST:
			{
				auto &shields = plrobj.shields;
			if (shields < MAX_SHIELDS) {
				const auto Difficulty_level = GameUniqueState.Difficulty_level;
				fix boost = 3 * F1_0 + 3 * F1_0 * (NDL - underlying_value(Difficulty_level));
#if defined(DXX_BUILD_DESCENT_II)
				if (Difficulty_level == Difficulty_level_type::_0)
					boost += boost/2;
#endif
				shields += boost;
				if (shields > MAX_SHIELDS)
					shields = MAX_SHIELDS;
				powerup_basic(0, 0, 15, SHIELD_SCORE, "%s %s %d", TXT_SHIELD, TXT_BOOSTED_TO, f2ir(shields));
				used=1;
			} else
				HUD_init_message(HM_DEFAULT|HM_REDUNDANT|HM_MAYDUPL, TXT_MAXED_OUT,TXT_SHIELD);
			break;
			}
		case powerup_type_t::POW_LASER:
			if (player_info.laser_level >= MAX_LASER_LEVEL) {
#if defined(DXX_BUILD_DESCENT_I)
				player_info.laser_level = MAX_LASER_LEVEL;
#endif
				HUD_init_message(HM_DEFAULT|HM_REDUNDANT|HM_MAYDUPL, TXT_MAXED_OUT,TXT_LASER);
			} else {
				const auto level_before_powerup = player_info.laser_level;
				++ player_info.laser_level;
				const auto level_after_powerup = player_info.laser_level;
				if (Newdemo_state == ND_STATE_RECORDING)
					newdemo_record_laser_level(level_before_powerup, level_after_powerup);
				powerup_basic(10, 0, 10, LASER_SCORE, "%s %s %u", TXT_LASER, TXT_BOOSTED_TO, static_cast<unsigned>(level_after_powerup) + 1);
				pick_up_primary(player_info, primary_weapon_index_t::LASER_INDEX);
				used=1;
			}
			if (!used && !(Game_mode & GM_MULTI) )
				used = pick_up_energy(player_info);
			break;
		case powerup_type_t::POW_MISSILE_1:
			used = pick_up_secondary(player_info, secondary_weapon_index_t::CONCUSSION_INDEX, 1, Controls);
			break;
		case powerup_type_t::POW_MISSILE_4:
			used = pick_up_secondary(player_info, secondary_weapon_index_t::CONCUSSION_INDEX, 4, Controls);
			break;

		case powerup_type_t::POW_KEY_BLUE:
			used = pick_up_key(0, 0, 15, player_info.powerup_flags, PLAYER_FLAGS_BLUE_KEY, TXT_BLUE, id);
			break;
		case powerup_type_t::POW_KEY_RED:
			used = pick_up_key(15, 0, 0, player_info.powerup_flags, PLAYER_FLAGS_RED_KEY, TXT_RED, id);
			break;
		case powerup_type_t::POW_KEY_GOLD:
			used = pick_up_key(15, 15, 7, player_info.powerup_flags, PLAYER_FLAGS_GOLD_KEY, TXT_YELLOW, id);
			break;
		case powerup_type_t::POW_QUAD_FIRE:
			used = player_hit_powerup<PLAYER_FLAGS_QUAD_LASERS>(player_info, TXT_QUAD_LASERS, player_hit_quadlaser_powerup());
			break;

		case	powerup_type_t::POW_VULCAN_WEAPON:
#if defined(DXX_BUILD_DESCENT_II)
		case	powerup_type_t::POW_GAUSS_WEAPON:
#endif
			{
			used = pick_up_primary(player_info,
#if defined(DXX_BUILD_DESCENT_II)
									(id == powerup_type_t::POW_GAUSS_WEAPON)
				? primary_weapon_index_t::GAUSS_INDEX
				:
#endif
				primary_weapon_index_t::VULCAN_INDEX
			);
			if (const auto ammo_used = pick_up_vulcan_ammo(player_info, obj->ctype.powerup_info.count))
			{
				/* Even if the cannon is made empty, leave `used` set to
				 * false, so that the cannon can remain in the mine.
				 */
				obj->ctype.powerup_info.count -= ammo_used;
				if (!used)
				{
					powerup_basic(7, 14, 21, VULCAN_AMMO_SCORE, "%s!", TXT_VULCAN_AMMO);
					special_used = 1;
					id = powerup_type_t::POW_VULCAN_AMMO;		//set new id for making sound at end of this function
                                        if (Game_mode & GM_MULTI)
                                                multi_send_vulcan_weapon_ammo_adjust(obj); // let other players know how much ammo we took.
				}
			}
			break;
		}

		case	powerup_type_t::POW_SPREADFIRE_WEAPON:
			used = pick_up_primary_or_energy(player_info, primary_weapon_index_t::SPREADFIRE_INDEX);
			break;
		case	powerup_type_t::POW_PLASMA_WEAPON:
			used = pick_up_primary_or_energy(player_info, primary_weapon_index_t::PLASMA_INDEX);
			break;
		case	powerup_type_t::POW_FUSION_WEAPON:
			used = pick_up_primary_or_energy(player_info, primary_weapon_index_t::FUSION_INDEX);
			break;

#if defined(DXX_BUILD_DESCENT_II)
		case	powerup_type_t::POW_HELIX_WEAPON:
			used = pick_up_primary_or_energy(player_info, primary_weapon_index_t::HELIX_INDEX);
			break;

		case	powerup_type_t::POW_PHOENIX_WEAPON:
			used = pick_up_primary_or_energy(player_info, primary_weapon_index_t::PHOENIX_INDEX);
			break;

		case	powerup_type_t::POW_OMEGA_WEAPON:
			used = pick_up_primary(player_info, primary_weapon_index_t::OMEGA_INDEX);
			if (used)
				player_info.Omega_charge = obj->ctype.powerup_info.count;
			if (!used && !(Game_mode & GM_MULTI) )
				used = pick_up_energy(player_info);
			break;
#endif

		case	powerup_type_t::POW_PROXIMITY_WEAPON:
			used = pick_up_secondary(player_info, secondary_weapon_index_t::PROXIMITY_INDEX, 4, Controls);
			break;
		case	powerup_type_t::POW_SMARTBOMB_WEAPON:
			used = pick_up_secondary(player_info, secondary_weapon_index_t::SMART_INDEX, 1, Controls);
			break;
		case	powerup_type_t::POW_MEGA_WEAPON:
			used = pick_up_secondary(player_info, secondary_weapon_index_t::MEGA_INDEX, 1, Controls);
			break;
#if defined(DXX_BUILD_DESCENT_II)
		case	powerup_type_t::POW_SMISSILE1_1:
			used = pick_up_secondary(player_info, secondary_weapon_index_t::SMISSILE1_INDEX, 1, Controls);
			break;
		case	powerup_type_t::POW_SMISSILE1_4:
			used = pick_up_secondary(player_info, secondary_weapon_index_t::SMISSILE1_INDEX, 4, Controls);
			break;
		case	powerup_type_t::POW_GUIDED_MISSILE_1:
			used = pick_up_secondary(player_info, secondary_weapon_index_t::GUIDED_INDEX, 1, Controls);
			break;
		case	powerup_type_t::POW_GUIDED_MISSILE_4:
			used = pick_up_secondary(player_info, secondary_weapon_index_t::GUIDED_INDEX, 4, Controls);
			break;
		case	powerup_type_t::POW_SMART_MINE:
			used = pick_up_secondary(player_info, secondary_weapon_index_t::SMART_MINE_INDEX, 4, Controls);
			break;
		case	powerup_type_t::POW_MERCURY_MISSILE_1:
			used = pick_up_secondary(player_info, secondary_weapon_index_t::SMISSILE4_INDEX, 1, Controls);
			break;
		case	powerup_type_t::POW_MERCURY_MISSILE_4:
			used = pick_up_secondary(player_info, secondary_weapon_index_t::SMISSILE4_INDEX, 4, Controls);
			break;
		case	powerup_type_t::POW_EARTHSHAKER_MISSILE:
			used = pick_up_secondary(player_info, secondary_weapon_index_t::SMISSILE5_INDEX, 1, Controls);
			break;
#endif
		case	powerup_type_t::POW_VULCAN_AMMO:
			used = pick_up_vulcan_ammo(player_info);
			break;
		case	powerup_type_t::POW_HOMING_AMMO_1:
			used = pick_up_secondary(player_info, secondary_weapon_index_t::HOMING_INDEX, 1, Controls);
			break;
		case	powerup_type_t::POW_HOMING_AMMO_4:
			used = pick_up_secondary(player_info, secondary_weapon_index_t::HOMING_INDEX, 4, Controls);
			break;
		case	powerup_type_t::POW_CLOAK:
			if (player_info.powerup_flags & PLAYER_FLAGS_CLOAKED) {
				HUD_init_message(HM_DEFAULT|HM_REDUNDANT|HM_MAYDUPL, "%s %s!",TXT_ALREADY_ARE,TXT_CLOAKED);
				break;
			} else {
				player_info.cloak_time = {GameTime64};	//	Not! changed by awareness events (like player fires laser).
				player_info.powerup_flags |= PLAYER_FLAGS_CLOAKED;
				ai_do_cloak_stuff();
				if (Game_mode & GM_MULTI)
					multi_send_cloak();
				powerup_basic(-10,-10,-10, CLOAK_SCORE, "%s!",TXT_CLOAKING_DEVICE);
				used = 1;
				break;
			}
		case	powerup_type_t::POW_INVULNERABILITY:
			{
				auto &pl_flags = player_info.powerup_flags;
				if (pl_flags & PLAYER_FLAGS_INVULNERABLE) {
					if (!player_info.FakingInvul)
					{
				HUD_init_message(HM_DEFAULT|HM_REDUNDANT|HM_MAYDUPL, "%s %s!",TXT_ALREADY_ARE,TXT_INVULNERABLE);
				break;
					}
			}
				player_info.FakingInvul = 0;
				pl_flags |= PLAYER_FLAGS_INVULNERABLE;
				player_info.invulnerable_time = {GameTime64};
				powerup_basic(7, 14, 21, INVULNERABILITY_SCORE, "%s!",TXT_INVULNERABILITY);
				used = 1;
				break;
			}
	#ifndef RELEASE
		case	powerup_type_t::POW_MEGAWOW:
			do_megawow_powerup(plrobj, 50);
			used = 1;
			break;
	#endif

#if defined(DXX_BUILD_DESCENT_II)
		case powerup_type_t::POW_FULL_MAP:
			used = player_hit_powerup<PLAYER_FLAGS_MAP_ALL>(player_info, "the FULL MAP", player_hit_silent_rb_powerup("FULL MAP!"));
			break;

		case powerup_type_t::POW_CONVERTER:
			used = player_hit_powerup<PLAYER_FLAGS_CONVERTER>(player_info, "the Converter", player_hit_silent_rb_powerup("Energy -> shield converter!"));
			break;

		case powerup_type_t::POW_SUPER_LASER:
			if (player_info.laser_level >= MAX_SUPER_LASER_LEVEL)
			{
				player_info.laser_level = MAX_SUPER_LASER_LEVEL;
				HUD_init_message_literal(HM_DEFAULT|HM_REDUNDANT|HM_MAYDUPL, "SUPER LASER MAXED OUT!");
			} else {
				const auto old_level = player_info.laser_level;

				if (player_info.laser_level <= MAX_LASER_LEVEL)
					player_info.laser_level = MAX_LASER_LEVEL;
				++ player_info.laser_level;
				if (Newdemo_state == ND_STATE_RECORDING)
					newdemo_record_laser_level(old_level, player_info.laser_level);
				powerup_basic(10, 0, 10, LASER_SCORE, "Super Boost to Laser level %u", static_cast<unsigned>(player_info.laser_level) + 1);
				if (player_info.Primary_weapon != primary_weapon_index_t::LASER_INDEX)
					check_to_use_primary_super_laser(player_info);
				used=1;
			}
			if (!used && !(Game_mode & GM_MULTI) )
				used = pick_up_energy(player_info);
			break;

		case powerup_type_t::POW_AMMO_RACK:
			used = player_hit_powerup<PLAYER_FLAGS_AMMO_RACK>(player_info, "the Ammo rack", player_hit_basic_sound_powerup<15, 0, 15, powerup_type_t::POW_AMMO_RACK>("AMMO RACK!"));
			break;

		case powerup_type_t::POW_AFTERBURNER:
			used = player_hit_powerup<PLAYER_FLAGS_AFTERBURNER>(player_info, "the Afterburner", player_hit_afterburner_powerup("AFTERBURNER!"));
			break;

		case powerup_type_t::POW_HEADLIGHT:
			used = player_hit_powerup<PLAYER_FLAGS_HEADLIGHT>(player_info, "the Headlight boost", player_hit_headlight_powerup());
			break;

		case powerup_type_t::POW_FLAG_BLUE:
			used = player_hit_flag_powerup<team_number::red>(player_info, "BLUE FLAG!");
		   break;

		case powerup_type_t::POW_HOARD_ORB:
			if (game_mode_hoard(Game_mode))
			{
				auto &proximity = player_info.hoard.orbs;
				if (proximity < player_info.max_hoard_orbs)
				{
					++ proximity;
					powerup_basic_str(15, 0, 15, 0, "Orb!!!");
					player_info.powerup_flags |= PLAYER_FLAGS_FLAG;
					used=1;
					multi_send_got_orb (Player_num);
				}
			}
		  break;	

		case powerup_type_t::POW_FLAG_RED:
			used = player_hit_flag_powerup<team_number::blue>(player_info, "RED FLAG!");
		   break;
#endif
		default:
			break;
		}

//always say used, until physics problem (getting stuck on unused powerup)
//is solved.  Note also the break statements above that are commented out
//!!	used=1;

	if (used || special_used)
	{
		if (const auto hit_sound = Powerup_info[id].hit_sound; hit_sound > -1)
			multi_digi_play_sample(hit_sound, F1_0);
		detect_escort_goal_accomplished(obj);
	}

	return used;

}
}

DEFINE_SERIAL_UDT_TO_MESSAGE(powerup_type_info, pti, (pti.vclip_num, serial::pad<3>(), pti.hit_sound, pti.size, pti.light));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(powerup_type_info, 16);

namespace dcx {

void powerup_type_info_read(const NamedPHYSFS_File fp, powerup_type_info &pti)
{
	PHYSFSX_serialize_read(fp, pti);
}

void powerup_type_info_write(PHYSFS_File *fp, const powerup_type_info &pti)
{
	PHYSFSX_serialize_write(fp, pti);
}

}
