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
#include "key.h"
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
#include "lighting.h"
#include "controls.h"
#include "kconfig.h"
#include "newdemo.h"
#include "escort.h"
#if DXX_USE_EDITOR
#include "gr.h"	//	for powerup outline drawing
#include "editor/editor.h"
#endif
#include "hudmsg.h"
#include "playsave.h"

namespace dcx {
unsigned N_powerup_types;
}
namespace dsx {
array<powerup_type_info, MAX_POWERUP_TYPES> Powerup_info;

//process this powerup for this frame
void do_powerup_frame(const vmobjptridx_t obj)
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
		object_create_explosion(vmsegptridx(obj->segnum), obj->pos, F1_0*7/2, VCLIP_POWERUP_DISAPPEARANCE);

		if ( Vclip[VCLIP_POWERUP_DISAPPEARANCE].sound_num > -1 )
			digi_link_sound_to_object(Vclip[VCLIP_POWERUP_DISAPPEARANCE].sound_num, obj, 0, F1_0, sound_stack::allow_stacking);
	}
}

}

namespace dcx {

void draw_powerup(grs_canvas &canvas, const object_base &obj)
{
	auto &vci = obj.rtype.vclip_info;
	draw_object_blob(canvas, obj, Vclip[vci.vclip_num].frames[vci.framenum]);
}

}

static void _powerup_basic_nonhud(int redadd, int greenadd, int blueadd, int score)
{
	PALETTE_FLASH_ADD(redadd,greenadd,blueadd);
	add_points_to_score(ConsoleObject->ctype.player_info, score);
}

void (powerup_basic)(int redadd, int greenadd, int blueadd, int score, const char *format, ...)
{
	va_list	args;

	va_start(args, format );
	HUD_init_message_va(HM_DEFAULT, format, args);
	va_end(args);
	_powerup_basic_nonhud(redadd, greenadd, blueadd, score);
}

void powerup_basic_str(int redadd, int greenadd, int blueadd, int score, const char *str)
{
	HUD_init_message_literal(HM_DEFAULT, str);
	_powerup_basic_nonhud(redadd, greenadd, blueadd, score);
}

//#ifndef RELEASE
//	Give the megawow powerup!
namespace dsx {
void do_megawow_powerup(int quantity)
{
	powerup_basic(30, 0, 30, 1, "MEGA-WOWIE-ZOWIE!");
	auto &player_info = get_local_plrobj().ctype.player_info;
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
	get_local_plrobj().shields = F1_0*200;
	player_info.powerup_flags |= PLAYER_FLAGS_QUAD_LASERS;
#if defined(DXX_BUILD_DESCENT_I)
	const auto laser_level = MAX_LASER_LEVEL;
#elif defined(DXX_BUILD_DESCENT_II)
	player_info.Omega_charge = MAX_OMEGA_CHARGE;
	if (game_mode_hoard())
		player_info.hoard.orbs = player_info.max_hoard_orbs;
	const auto laser_level = MAX_SUPER_LASER_LEVEL;
#endif
	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_laser_level(player_info.laser_level, laser_level);
	player_info.laser_level = laser_level;


	update_laser_weapon_info();

}
}
//#endif

namespace dsx {

static int pick_up_energy(player_info &player_info)
{
	int	used=0;

	auto &energy = player_info.energy;
	if (energy < MAX_ENERGY) {
		fix boost;
		boost = 3*F1_0 + 3*F1_0*(NDL - Difficulty_level);
#if defined(DXX_BUILD_DESCENT_II)
		if (Difficulty_level == 0)
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

static int pick_up_primary_or_energy(player_info &player_info, int weapon_index)
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

}

static int pick_up_key(const int r, const int g, const int b, player_flags &player_flags, const PLAYER_FLAG key_flag, const char *const key_name, const powerup_type_t id)
{
	if (player_flags & key_flag)
		return 0;
	player_flags |= key_flag;
	powerup_basic(r, g, b, KEY_SCORE, "%s %s", key_name, TXT_ACCESS_GRANTED);
	multi_digi_play_sample(Powerup_info[id].hit_sound, F1_0);
	invalidate_escort_goal();
	return (Game_mode & GM_MULTI) ? 0 : 1;
}

//	returns true if powerup consumed
namespace dsx {

namespace {

#if defined(DXX_BUILD_DESCENT_II)
template <int r, int g, int b>
struct player_hit_basic_silent_powerup
{
	const char *const desc_pickup;
	player_hit_basic_silent_powerup(const char *const p) :
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
	DXX_INHERIT_CONSTRUCTORS(player_hit_basic_sound_powerup, base_type);
	template <PLAYER_FLAG player_flag>
		void pickup(player_flags &powerup_flags) const
		{
			multi_digi_play_sample(Powerup_info[id].hit_sound, F1_0);
			base_type::template pickup<player_flag>(powerup_flags);
		}
};

using player_hit_silent_rb_powerup = player_hit_basic_silent_powerup<15, 0, 15>;

struct player_hit_afterburner_powerup : player_hit_basic_sound_powerup<15, 15, 15, POW_AFTERBURNER>
{
	using base_type = player_hit_basic_sound_powerup<15, 15, 15, POW_AFTERBURNER>;
	DXX_INHERIT_CONSTRUCTORS(player_hit_afterburner_powerup, base_type);
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
		const auto active = PlayerCfg.HeadlightActiveDefault;
		powerup_flags |= active
			? PLAYER_FLAG::HEADLIGHT_PRESENT_AND_ON
			: PLAYER_FLAG::HEADLIGHT;
		powerup_basic(15, 0, 15, 0, "HEADLIGHT BOOST! (Headlight is O%s)", active ? "N" : "FF");
		multi_digi_play_sample(Powerup_info[POW_HEADLIGHT].hit_sound, F1_0);
		if (active && (Game_mode & GM_MULTI))
			multi_send_flags (Player_num);
	}
};

template <unsigned TEAM>
static int player_hit_flag_powerup(player_info &player_info, const char *const desc)
{
	if (!game_mode_capture_flag())
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

}

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

int do_powerup(const vmobjptridx_t obj)
{
	int used=0;
#if defined(DXX_BUILD_DESCENT_I)
	int vulcan_ammo_to_add_with_cannon;
#endif
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
		fix mydist = vm_vec_normalized_dir(tvec, obj->pos, ConsoleObject->pos);

		for (unsigned i = 0; i < MAX_PLAYERS; ++i)
		{
			if (i == Player_num)
				continue;
			auto &plr = *vcplayerptr(i);
			if (plr.connected != CONNECT_PLAYING)
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
		case POW_EXTRA_LIFE:
			get_local_player().lives++;
			powerup_basic_str(15, 15, 15, 0, TXT_EXTRA_LIFE);
			used=1;
			break;
		case POW_ENERGY:
			used = pick_up_energy(player_info);
			break;
		case POW_SHIELD_BOOST:
			{
				auto &shields = plrobj.shields;
			if (shields < MAX_SHIELDS) {
				fix boost = 3*F1_0 + 3*F1_0*(NDL - Difficulty_level);
#if defined(DXX_BUILD_DESCENT_II)
				if (Difficulty_level == 0)
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
		case POW_LASER:
			if (player_info.laser_level >= MAX_LASER_LEVEL) {
#if defined(DXX_BUILD_DESCENT_I)
				player_info.laser_level = MAX_LASER_LEVEL;
#endif
				HUD_init_message(HM_DEFAULT|HM_REDUNDANT|HM_MAYDUPL, TXT_MAXED_OUT,TXT_LASER);
			} else {
				if (Newdemo_state == ND_STATE_RECORDING)
					newdemo_record_laser_level(player_info.laser_level, player_info.laser_level + 1);
				++ player_info.laser_level;
				powerup_basic(10, 0, 10, LASER_SCORE, "%s %s %d",TXT_LASER,TXT_BOOSTED_TO, player_info.laser_level+1);
				update_laser_weapon_info();
				pick_up_primary(player_info, primary_weapon_index_t::LASER_INDEX);
				used=1;
			}
			if (!used && !(Game_mode & GM_MULTI) )
				used = pick_up_energy(player_info);
			break;
		case POW_MISSILE_1:
			used=pick_up_secondary(player_info, CONCUSSION_INDEX, 1);
			break;
		case POW_MISSILE_4:
			used=pick_up_secondary(player_info, CONCUSSION_INDEX, 4);
			break;

		case POW_KEY_BLUE:
			used = pick_up_key(0, 0, 15, player_info.powerup_flags, PLAYER_FLAGS_BLUE_KEY, TXT_BLUE, id);
			break;
		case POW_KEY_RED:
			used = pick_up_key(15, 0, 0, player_info.powerup_flags, PLAYER_FLAGS_RED_KEY, TXT_RED, id);
			break;
		case POW_KEY_GOLD:
			used = pick_up_key(15, 15, 7, player_info.powerup_flags, PLAYER_FLAGS_GOLD_KEY, TXT_YELLOW, id);
			break;
		case POW_QUAD_FIRE:
			used = player_hit_powerup<PLAYER_FLAGS_QUAD_LASERS>(player_info, TXT_QUAD_LASERS, player_hit_quadlaser_powerup());
			break;

		case	POW_VULCAN_WEAPON:
#if defined(DXX_BUILD_DESCENT_I)
			if ((used = pick_up_primary(player_info, primary_weapon_index_t::VULCAN_INDEX)) != 0) {
				vulcan_ammo_to_add_with_cannon = obj->ctype.powerup_info.count;
				if (vulcan_ammo_to_add_with_cannon < VULCAN_WEAPON_AMMO_AMOUNT) vulcan_ammo_to_add_with_cannon = VULCAN_WEAPON_AMMO_AMOUNT;
				pick_up_vulcan_ammo(player_info, vulcan_ammo_to_add_with_cannon);
			}

//added/edited 8/3/98 by Victor Rachels to fix vulcan multi bug
//check if multi, if so, pick up ammo w/o using, set ammo left. else, normal

//killed 8/27/98 by Victor Rachels to fix vulcan ammo multiplying.  new way
// is by spewing the current held ammo when dead.
//-killed                        if (!used && (Game_mode & GM_MULTI))
//-killed                        {
//-killed                         int tempcount;                          
//-killed                           tempcount=Players[Player_num].primary_ammo[VULCAN_INDEX];
//-killed                            if (pick_up_ammo(CLASS_PRIMARY, VULCAN_INDEX, obj->ctype.powerup_info.count))
//-killed                             obj->ctype.powerup_info.count -= Players[Player_num].primary_ammo[VULCAN_INDEX]-tempcount;
//-killed                        }
//end kill - Victor Rachels

			if (!used && !(Game_mode & GM_MULTI) )
//end addition/edit - Victor Rachels
				used = pick_up_vulcan_ammo(player_info);
			break;
#elif defined(DXX_BUILD_DESCENT_II)
		case	POW_GAUSS_WEAPON: {
			int ammo = obj->ctype.powerup_info.count;

			used = pick_up_primary(player_info, (get_powerup_id(obj) == POW_VULCAN_WEAPON)
				? primary_weapon_index_t::VULCAN_INDEX
				: primary_weapon_index_t::GAUSS_INDEX
			);

			//didn't get the weapon (because we already have it), but
			//maybe snag some of the ammo.  if single-player, grab all the ammo
			//and remove the powerup.  If multi-player take ammo in excess of
			//the amount in a powerup, and leave the rest.
			if (! used)
				if ((Game_mode & GM_MULTI) )
					ammo -= VULCAN_AMMO_AMOUNT;	//don't let take all ammo

			if (ammo > 0) {
				int ammo_used;
				ammo_used = pick_up_vulcan_ammo(player_info, ammo);
				obj->ctype.powerup_info.count -= ammo_used;
				if (!used && ammo_used) {
					powerup_basic(7, 14, 21, VULCAN_AMMO_SCORE, "%s!", TXT_VULCAN_AMMO);
					special_used = 1;
					id = POW_VULCAN_AMMO;		//set new id for making sound at end of this function
					if (obj->ctype.powerup_info.count == 0)
						used = 1;		//say used if all ammo taken
                                        if (Game_mode & GM_MULTI)
                                                multi_send_vulcan_weapon_ammo_adjust(obj); // let other players know how much ammo we took.
				}
			}

			break;
		}
#endif

		case	POW_SPREADFIRE_WEAPON:
			used = pick_up_primary_or_energy(player_info, primary_weapon_index_t::SPREADFIRE_INDEX);
			break;
		case	POW_PLASMA_WEAPON:
			used = pick_up_primary_or_energy(player_info, primary_weapon_index_t::PLASMA_INDEX);
			break;
		case	POW_FUSION_WEAPON:
			used = pick_up_primary_or_energy(player_info, primary_weapon_index_t::FUSION_INDEX);
			break;

#if defined(DXX_BUILD_DESCENT_II)
		case	POW_HELIX_WEAPON:
			used = pick_up_primary_or_energy(player_info, primary_weapon_index_t::HELIX_INDEX);
			break;

		case	POW_PHOENIX_WEAPON:
			used = pick_up_primary_or_energy(player_info, primary_weapon_index_t::PHOENIX_INDEX);
			break;

		case	POW_OMEGA_WEAPON:
			used = pick_up_primary(player_info, primary_weapon_index_t::OMEGA_INDEX);
			if (used)
				player_info.Omega_charge = obj->ctype.powerup_info.count;
			if (!used && !(Game_mode & GM_MULTI) )
				used = pick_up_energy(player_info);
			break;
#endif

		case	POW_PROXIMITY_WEAPON:
			used=pick_up_secondary(player_info, PROXIMITY_INDEX, 4);
			break;
		case	POW_SMARTBOMB_WEAPON:
			used=pick_up_secondary(player_info, SMART_INDEX, 1);
			break;
		case	POW_MEGA_WEAPON:
			used=pick_up_secondary(player_info, MEGA_INDEX, 1);
			break;
#if defined(DXX_BUILD_DESCENT_II)
		case	POW_SMISSILE1_1:
			used=pick_up_secondary(player_info, SMISSILE1_INDEX, 1);
			break;
		case	POW_SMISSILE1_4:
			used=pick_up_secondary(player_info, SMISSILE1_INDEX, 4);
			break;
		case	POW_GUIDED_MISSILE_1:
			used=pick_up_secondary(player_info, GUIDED_INDEX, 1);
			break;
		case	POW_GUIDED_MISSILE_4:
			used=pick_up_secondary(player_info, GUIDED_INDEX, 4);
			break;
		case	POW_SMART_MINE:
			used=pick_up_secondary(player_info, SMART_MINE_INDEX, 4);
			break;
		case	POW_MERCURY_MISSILE_1:
			used=pick_up_secondary(player_info, SMISSILE4_INDEX, 1);
			break;
		case	POW_MERCURY_MISSILE_4:
			used=pick_up_secondary(player_info, SMISSILE4_INDEX, 4);
			break;
		case	POW_EARTHSHAKER_MISSILE:
			used=pick_up_secondary(player_info, SMISSILE5_INDEX, 1);
			break;
#endif
		case	POW_VULCAN_AMMO:
			used = pick_up_vulcan_ammo(player_info);
#if defined(DXX_BUILD_DESCENT_I)
			if (!used && !(Game_mode & GM_MULTI) )
				used = pick_up_vulcan_ammo(player_info);
#endif
			break;
		case	POW_HOMING_AMMO_1:
			used=pick_up_secondary(player_info, HOMING_INDEX, 1);
			break;
		case	POW_HOMING_AMMO_4:
			used=pick_up_secondary(player_info, HOMING_INDEX, 4);
			break;
		case	POW_CLOAK:
			if (player_info.powerup_flags & PLAYER_FLAGS_CLOAKED) {
				HUD_init_message(HM_DEFAULT|HM_REDUNDANT|HM_MAYDUPL, "%s %s!",TXT_ALREADY_ARE,TXT_CLOAKED);
				break;
			} else {
				player_info.cloak_time = GameTime64;	//	Not! changed by awareness events (like player fires laser).
				player_info.powerup_flags |= PLAYER_FLAGS_CLOAKED;
				ai_do_cloak_stuff();
				if (Game_mode & GM_MULTI)
					multi_send_cloak();
				powerup_basic(-10,-10,-10, CLOAK_SCORE, "%s!",TXT_CLOAKING_DEVICE);
				used = 1;
				break;
			}
		case	POW_INVULNERABILITY:
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
				player_info.invulnerable_time = GameTime64;
				powerup_basic(7, 14, 21, INVULNERABILITY_SCORE, "%s!",TXT_INVULNERABILITY);
				used = 1;
				break;
			}
	#ifndef RELEASE
		case	POW_MEGAWOW:
			do_megawow_powerup(50);
			used = 1;
			break;
	#endif

#if defined(DXX_BUILD_DESCENT_II)
		case POW_FULL_MAP:
			used = player_hit_powerup<PLAYER_FLAGS_MAP_ALL>(player_info, "the FULL MAP", player_hit_silent_rb_powerup("FULL MAP!"));
			break;

		case POW_CONVERTER:
			used = player_hit_powerup<PLAYER_FLAGS_CONVERTER>(player_info, "the Converter", player_hit_silent_rb_powerup("Energy -> shield converter!"));
			break;

		case POW_SUPER_LASER:
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
				powerup_basic(10, 0, 10, LASER_SCORE, "Super Boost to Laser level %d", player_info.laser_level + 1);
				update_laser_weapon_info();
				if (player_info.Primary_weapon != primary_weapon_index_t::LASER_INDEX)
					check_to_use_primary_super_laser(player_info);
				used=1;
			}
			if (!used && !(Game_mode & GM_MULTI) )
				used = pick_up_energy(player_info);
			break;

		case POW_AMMO_RACK:
			used = player_hit_powerup<PLAYER_FLAGS_AMMO_RACK>(player_info, "the Ammo rack", player_hit_basic_sound_powerup<15, 0, 15, POW_AMMO_RACK>("AMMO RACK!"));
			break;

		case POW_AFTERBURNER:
			used = player_hit_powerup<PLAYER_FLAGS_AFTERBURNER>(player_info, "the Afterburner", player_hit_afterburner_powerup("AFTERBURNER!"));
			break;

		case POW_HEADLIGHT:
			used = player_hit_powerup<PLAYER_FLAGS_HEADLIGHT>(player_info, "the Headlight boost", player_hit_headlight_powerup());
			break;

		case POW_FLAG_BLUE:
			used = player_hit_flag_powerup<TEAM_RED>(player_info, "BLUE FLAG!");
		   break;

		case POW_HOARD_ORB:
			if (game_mode_hoard())			
			{
				auto &proximity = player_info.hoard.orbs;
				if (proximity < player_info.max_hoard_orbs)
				{
					++ proximity;
					powerup_basic(15, 0, 15, 0, "Orb!!!");
					player_info.powerup_flags |= PLAYER_FLAGS_FLAG;
					used=1;
					multi_send_got_orb (Player_num);
				}
			}
		  break;	

		case POW_FLAG_RED:
			used = player_hit_flag_powerup<TEAM_BLUE>(player_info, "RED FLAG!");
		   break;

//		case POW_HOARD_ORB:

#endif
		default:
			break;
		}

//always say used, until physics problem (getting stuck on unused powerup)
//is solved.  Note also the break statements above that are commented out
//!!	used=1;

	if ((used || special_used) && Powerup_info[id].hit_sound  > -1 ) {
		multi_digi_play_sample(Powerup_info[id].hit_sound, F1_0);
		detect_escort_goal_accomplished(obj);
	}

	return used;

}
}

DEFINE_SERIAL_UDT_TO_MESSAGE(powerup_type_info, pti, (pti.vclip_num, pti.hit_sound, pti.size, pti.light));
ASSERT_SERIAL_UDT_MESSAGE_SIZE(powerup_type_info, 16);

void powerup_type_info_read(PHYSFS_File *fp, powerup_type_info &pti)
{
	PHYSFSX_serialize_read(fp, pti);
}

void powerup_type_info_write(PHYSFS_File *fp, const powerup_type_info &pti)
{
	PHYSFSX_serialize_write(fp, pti);
}
