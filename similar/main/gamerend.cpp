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
 * Stuff for rendering the HUD
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "timer.h"
#include "pstypes.h"
#include "console.h"
#include "inferno.h"
#include "dxxerror.h"
#include "gr.h"
#include "palette.h"
#include "bm.h"
#include "player.h"
#include "render.h"
#include "newmenu.h"
#include "screens.h"
#include "config.h"
#include "maths.h"
#include "robot.h"
#include "game.h"
#include "gauges.h"
#include "gamefont.h"
#include "newdemo.h"
#include "text.h"
#include "multi.h"
#include "hudmsg.h"
#include "endlevel.h"
#include "powerup.h"
#include "playsave.h"
#include "automap.h"
#include "mission.h"
#include "gameseq.h"
#include "args.h"
#include "object.h"

#include "compiler-range_for.h"
#include "d_levelstate.h"
#include "d_range.h"

#if DXX_USE_OGL
#include "ogl_init.h"
#endif

namespace dcx {
int netplayerinfo_on;
cockpit_mode_t last_drawn_cockpit = cockpit_mode_t{UINT8_MAX};
}

namespace dsx {
namespace {
#if defined(DXX_BUILD_DESCENT_I)
static inline void game_draw_marker_message(grs_canvas &)
{
}
#elif defined(DXX_BUILD_DESCENT_II)
static void game_draw_marker_message(grs_canvas &canvas)
{
	if (MarkerState.DefiningMarkerMessage())
	{
		gr_set_fontcolor(canvas, BM_XRGB(0, 63, 0),-1);
		auto &game_font = *GAME_FONT;
		gr_printf(canvas, game_font, 0x8000, (LINE_SPACING(game_font, game_font) * 5) + FSPACY(1), "Marker: %s%c", &Marker_input[0u], Marker_input[Marker_input.size() - 2] ? 0 : '_');
	}
}
#endif

static void update_cockpits(grs_canvas &);
}
}

namespace dcx {

namespace {

static void game_draw_multi_message(grs_canvas &canvas)
{
	if (!(Game_mode&GM_MULTI))
		return;
	const auto sending = multi_sending_message[Player_num];
	std::array<char, 64> buffer;
	if (sending != msgsend_state::none)
		std::snprintf(std::data(buffer), std::size(buffer), "%s: %s_", TXT_MESSAGE, Network_message.data());
	else if (const auto defining = multi_defining_message; defining != multi_macro_message_index::None)
		/* Use 1-based counting for user-visible text. */
		std::snprintf(std::data(buffer), std::size(buffer), "%s #%d: %s_", TXT_MACRO, 1 + underlying_value(defining), Network_message.data());
	else
		return;
	gr_set_fontcolor(canvas, BM_XRGB(0, 63, 0),-1);
	auto &game_font = *GAME_FONT;
	const auto &&y = (LINE_SPACING(game_font, game_font) * 5) + FSPACY(1);
	gr_string(canvas, game_font, 0x8000, y, std::data(buffer));
}

static void show_framerate(grs_canvas &canvas)
{
	static int fps_count = 0, fps_rate = 0;
	static fix64 fps_time = 0;
	fps_count++;
	const auto tq = timer_query();
	if (tq >= fps_time + F1_0)
	{
		fps_rate = fps_count;
		fps_count = 0;
		fps_time += F1_0;
	}
	const auto &&line_spacing = LINE_SPACING(*canvas.cv_font, *GAME_FONT);
	unsigned line_displacement;
	switch (PlayerCfg.CockpitMode[1])
	{
		case cockpit_mode_t::full_cockpit:
			line_displacement = line_spacing * 2;
			break;
		case cockpit_mode_t::status_bar:
			line_displacement = line_spacing;
			break;
		case cockpit_mode_t::full_screen:
			switch (PlayerCfg.HudMode)
			{
				case HudType::Standard:
					line_displacement = line_spacing * 4;
					break;
				case HudType::Alternate1:
					line_displacement = line_spacing * 6;
					if (Game_mode & GM_MULTI)
						line_displacement -= line_spacing * 4;
					break;
				case HudType::Alternate2:
					line_displacement = line_spacing;
					break;
				case HudType::Hidden:
				default:
					return;
			}
			break;
		case cockpit_mode_t::letterbox:
		case cockpit_mode_t::rear_view:
		default:
			return;
	}
	const auto &game_font = *GAME_FONT;
	gr_set_fontcolor(canvas, BM_XRGB(0, 31, 0),-1);
	char buf[16];
	if (CGameArg.DbgVerbose)
		snprintf(buf, sizeof(buf), "%iFPS (%.2fms)", fps_rate, (FrameTime * 1000.) / F1_0);
	else
		snprintf(buf, sizeof(buf), "%iFPS", fps_rate);
	const auto &&[w, h] = gr_get_string_size(game_font, buf);
	const auto bm_h = canvas.cv_bitmap.bm_h;
	gr_string(canvas, game_font, FSPACX(318) - w, bm_h - line_displacement, buf, w, h);
}

}

}

namespace dsx {

namespace {

static void show_netplayerinfo(grs_canvas &canvas)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	int x=0, y=0;
	static const char *const eff_strings[]={"trashing","really hurting","seriously affecting","hurting","affecting","tarnishing"};

	gr_set_fontcolor(canvas, 255, -1);

	const auto &&fspacx = FSPACX();
	const auto &&fspacx120 = fspacx(120);
	const auto &&fspacy84 = FSPACY(84);
	x = (SWIDTH / 2) - fspacx120;
	y = (SHEIGHT / 2) - fspacy84;

	gr_settransblend(canvas, gr_fade_level{14}, gr_blend::normal);
	const uint8_t color000 = BM_XRGB(0, 0, 0);
	gr_rect(canvas, (SWIDTH / 2) - fspacx120, (SHEIGHT / 2) - fspacy84, (SWIDTH / 2) + fspacx120, (SHEIGHT / 2) + fspacy84, color000);
	gr_settransblend(canvas, GR_FADE_OFF, gr_blend::normal);

	// general game information
	const auto &&line_spacing = LINE_SPACING(*canvas.cv_font, *GAME_FONT);
	y += line_spacing;
	auto &game_font = *GAME_FONT;
	gr_string(canvas, game_font, 0x8000, y, Netgame.game_name.data());
	y += line_spacing;
	gr_printf(canvas, game_font, 0x8000, y, "%s - lvl: %i", Netgame.mission_title.data(), Netgame.levelnum);

	const auto &&fspacx8 = fspacx(8);
	x += fspacx8;
	y += line_spacing * 2;
	const auto gamemode = underlying_value(Netgame.gamemode);
	gr_printf(canvas, game_font, x, y, "game mode: %s", gamemode < GMNames.size() ? GMNames[gamemode] : "INVALID");
	y += line_spacing;
	gr_printf(canvas, game_font, x, y,"difficulty: %s", MENU_DIFFICULTY_TEXT(Netgame.difficulty));
	y += line_spacing;
	{
		auto &plr = get_local_player();
		gr_printf(canvas, game_font, x, y,"level time: %i:%02i:%02i", plr.hours_level, f2i(plr.time_level) / 60 % 60, f2i(plr.time_level) % 60);
	y += line_spacing;
		gr_printf(canvas, game_font, x, y,"total time: %i:%02i:%02i", plr.hours_total, f2i(plr.time_total) / 60 % 60, f2i(plr.time_total) % 60);
	}
	y += line_spacing;
	if (Netgame.KillGoal)
		gr_printf(canvas, game_font, x, y,"Kill goal: %d",Netgame.KillGoal*5);

	// player information (name, kills, ping, game efficiency)
	y += line_spacing * 2;
	gr_string(canvas, game_font, x, y, "player");
	gr_string(canvas, game_font, x + fspacx8 * 7, y, ((Game_mode & GM_MULTI_COOP)
		? "score"
		: (gr_string(canvas, game_font, x + fspacx8 * 12, y, "deaths"), "kills")
	));
	gr_string(canvas, game_font, x + fspacx8 * 18, y, "ping");
	gr_string(canvas, game_font, x + fspacx8 * 23, y, "efficiency");

	// process players table
	for (unsigned i = 0; i < MAX_PLAYERS; ++i)
	{
		auto &plr = *vcplayerptr(i);
		if (plr.connected == player_connection_status::disconnected)
			continue;

		y += line_spacing;

		const auto color = get_player_or_team_color(i);
		auto &prgb = player_rgb[color];
		gr_set_fontcolor(canvas, BM_XRGB(prgb.r, prgb.g, prgb.b), -1);
		gr_string(canvas, game_font, x, y, plr.callsign);
		{
			auto &plrobj = *vcobjptr(plr.objnum);
			auto &player_info = plrobj.ctype.player_info;
			auto v = ((Game_mode & GM_MULTI_COOP)
				? player_info.mission.score
				: (gr_printf(canvas, game_font, x + fspacx8 * 12, y,"%-6d", player_info.net_killed_total), player_info.net_kills_total)
			);
			gr_printf(canvas, game_font, x + fspacx8 * 7, y, "%-6d", v);
		}

		gr_printf(canvas, game_font, x + fspacx8 * 18, y,"%-6d", Netgame.players[i].ping);
		if (i != Player_num)
			gr_printf(canvas, game_font, x + fspacx8 * 23, y, "%hu/%hu", kill_matrix[Player_num][i], kill_matrix[i][Player_num]);
	}

	y += (line_spacing * 2) + (line_spacing * (MAX_PLAYERS - N_players));

	// printf team scores
	if (Game_mode & GM_TEAM)
	{
		gr_set_fontcolor(canvas, 255, -1);
		gr_string(canvas, game_font, x, y, "team");
		gr_string(canvas, game_font, x + fspacx8 * 8, y, "score");
		y += line_spacing;
		gr_set_fontcolor(canvas, BM_XRGB(player_rgb[0].r, player_rgb[0].g, player_rgb[0].b),-1);
		gr_printf(canvas, game_font, x, y, "%s:", Netgame.team_name[team_number::blue].operator const char *());
		gr_printf(canvas, game_font, x + fspacx8 * 8, y, "%i", team_kills[team_number::blue]);
		y += line_spacing;
		gr_set_fontcolor(canvas, BM_XRGB(player_rgb[1].r, player_rgb[1].g, player_rgb[1].b),-1);
		gr_printf(canvas, game_font, x, y, "%s:", Netgame.team_name[team_number::red].operator const char *());
		gr_printf(canvas, game_font, x + fspacx8 * 8, y, "%i", team_kills[team_number::red]);
		y += line_spacing * 2;
	}
	else
		y += line_spacing * 4;

	gr_set_fontcolor(canvas, 255, -1);

	// additional information about game - hoard, ranking

#if defined(DXX_BUILD_DESCENT_II)
	if (game_mode_hoard())
	{
		if (hoard_highest_record_stats.player >= Players.size())
			gr_string(canvas, game_font, 0x8000, y, "There is no record yet for this level.");
		else
			gr_printf(canvas, game_font, 0x8000, y, "%s has the record at %d points.", static_cast<const char *>(vcplayerptr(hoard_highest_record_stats.player)->callsign), hoard_highest_record_stats.points);
	}
	else
#endif
	if (!PlayerCfg.NoRankings)
	{
		const int ieff = (PlayerCfg.NetlifeKills + PlayerCfg.NetlifeKilled <= 0)
			? 0
			: static_cast<int>(
				static_cast<float>(PlayerCfg.NetlifeKills) / (
					static_cast<float>(PlayerCfg.NetlifeKilled) + static_cast<float>(PlayerCfg.NetlifeKills)
				) * 100.0
			);
		const unsigned eff = ieff < 0 ? 0 : static_cast<unsigned>(ieff);
		gr_printf(canvas, game_font, 0x8000, y, "Your lifetime efficiency of %d%% (%d/%d)", eff, PlayerCfg.NetlifeKills, PlayerCfg.NetlifeKilled);
		y += line_spacing;
		if (eff<60)
			gr_printf(canvas, game_font, 0x8000, y, "is %s your ranking.", eff_strings[eff / 10]);
		else
			gr_string(canvas, game_font, 0x8000, y, "is serving you well.");
		y += line_spacing;
		gr_printf(canvas, game_font, 0x8000, y, "your rank is: %s", RankStrings[GetMyNetRanking()]);
	}
}

}

}

#ifndef NDEBUG

fix Show_view_text_timer = -1;

namespace {

static void draw_window_label(object_array &Objects, grs_canvas &canvas)
{
	auto &vcobjptridx = Objects.vcptridx;
	if ( Show_view_text_timer > 0 )
	{
		const char	*viewer_name,*control_name,*viewer_id;
		Show_view_text_timer -= FrameTime;

		viewer_id = "";
		switch( Viewer->type )
		{
			case OBJ_FIREBALL:	viewer_name = "Fireball"; break;
			case OBJ_ROBOT:		viewer_name = "Robot";
#if DXX_USE_EDITOR
				viewer_id = Robot_names[get_robot_id(*Viewer)].data();
#endif
				break;
			case OBJ_HOSTAGE:		viewer_name = "Hostage"; break;
			case OBJ_PLAYER:		viewer_name = "Player"; break;
			case OBJ_WEAPON:		viewer_name = "Weapon"; break;
			case OBJ_CAMERA:		viewer_name = "Camera"; break;
			case OBJ_POWERUP:		viewer_name = "Powerup";
#if DXX_USE_EDITOR
				viewer_id = Powerup_names[get_powerup_id(*Viewer)].data();
#endif
				break;
			case OBJ_DEBRIS:		viewer_name = "Debris"; break;
			case OBJ_CNTRLCEN:	viewer_name = "Reactor"; break;
			default:					viewer_name = "Unknown"; break;
		}

		switch ( Viewer->control_source) {
			case object::control_type::None:			control_name = "Stopped"; break;
			case object::control_type::ai:				control_name = "AI"; break;
			case object::control_type::flying:		control_name = "Flying"; break;
			case object::control_type::slew:			control_name = "Slew"; break;
			case object::control_type::flythrough:	control_name = "Flythrough"; break;
			case object::control_type::morph:			control_name = "Morphing"; break;
			default:					control_name = "Unknown"; break;
		}

		gr_set_fontcolor(canvas, BM_XRGB(31, 0, 0),-1);
		auto &game_font = *GAME_FONT;
		gr_printf(canvas, game_font, 0x8000, (SHEIGHT / 10), "%hu: %s [%s] View - %s", vcobjptridx(Viewer).get_unchecked_index(), viewer_name, viewer_id, control_name);

	}
}

}
#endif

namespace dsx {

namespace {

static void render_countdown_gauge(grs_canvas &canvas)
{
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	int Countdown_seconds_left;
	if (!Endlevel_sequence && LevelUniqueControlCenterState.Control_center_destroyed && (Countdown_seconds_left = LevelUniqueControlCenterState.Countdown_seconds_left) > -1)
	{ // && (Countdown_seconds_left<127))
#if defined(DXX_BUILD_DESCENT_II)
		if (!is_D2_OEM && !is_MAC_SHARE && !is_SHAREWARE)    // no countdown on registered only
		{
			//	On last level, we don't want a countdown.
			if (PLAYING_BUILTIN_MISSION && Current_level_num == Current_mission->last_level)
			{
				if (!(Game_mode & GM_MULTI))
					return;
				if (Game_mode & GM_MULTI_ROBOTS)
					return;
			}
		}
#endif
		gr_set_fontcolor(canvas, BM_XRGB(0, 63, 0),-1);
		auto &game_font = *GAME_FONT;
		gr_printf(canvas, game_font, 0x8000, (LINE_SPACING(game_font, game_font) * 6) + FSPACY(1), "T-%d s", Countdown_seconds_left);
	}
}

static void game_draw_hud_stuff(const d_robot_info_array &Robot_info, grs_canvas &canvas, const control_info &Controls)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
#ifndef NDEBUG
	draw_window_label(Objects, canvas);
#endif

	game_draw_multi_message(canvas);

	game_draw_marker_message(canvas);

	if ((Newdemo_state == ND_STATE_PLAYBACK || Newdemo_state == ND_STATE_RECORDING) && PlayerCfg.CockpitMode[1] != cockpit_mode_t::rear_view)
	{
		int y;

		auto &game_font = *GAME_FONT;
		gr_set_curfont(canvas, game_font);
		gr_set_fontcolor(canvas, BM_XRGB(27, 0, 0), -1);

		y = canvas.cv_bitmap.bm_h - (LINE_SPACING(*canvas.cv_font, *GAME_FONT) * 2);

		if (PlayerCfg.CockpitMode[1] == cockpit_mode_t::full_cockpit)
			y = canvas.cv_bitmap.bm_h / 1.2 ;
		if (Newdemo_state == ND_STATE_PLAYBACK) {
			if (Newdemo_show_percentage) {
				gr_printf(canvas, game_font, 0x8000, y, "%s (%d%% %s)", TXT_DEMO_PLAYBACK, newdemo_get_percent_done(), TXT_DONE);
			}
		} else {
			gr_printf(canvas, game_font, 0x8000, y, "%s (%dK)", TXT_DEMO_RECORDING, (Newdemo_num_written / 1024));
		}
	}

	render_countdown_gauge(canvas);

	if (CGameCfg.FPSIndicator && PlayerCfg.CockpitMode[1] != cockpit_mode_t::rear_view)
		show_framerate(canvas);

	auto previous_game_mode = Game_mode;
	if (Newdemo_state == ND_STATE_PLAYBACK)
		Game_mode = Newdemo_game_mode;

	auto &plrobj = get_local_plrobj();
	draw_hud(Robot_info, canvas, plrobj, Controls, Game_mode);

	if (Newdemo_state == ND_STATE_PLAYBACK)
		Game_mode = previous_game_mode;

	if (Player_dead_state != player_dead_state::no)
		player_dead_message(canvas);
}

}

#if defined(DXX_BUILD_DESCENT_II)

ubyte RenderingType=0;
ubyte DemoDoingRight=0,DemoDoingLeft=0;

namespace {

constexpr weapon_box_user DemoWBUType[]={weapon_box_user::weapon, weapon_box_user::guided, weapon_box_user::missile, weapon_box_user::rear, weapon_box_user::escort, weapon_box_user::marker, weapon_box_user::weapon};
constexpr char DemoRearCheck[]={0,0,0,1,0,0,0};
constexpr char DemoExtraMessage[][10] = {
	"PLAYER",
	"GUIDED",
	"MISSILE",
	"REAR",
	"GUIDE-BOT",
	"MARKER",
	"SHIP"
};

static const char *get_missile_name(const unsigned laser_type)
{
	switch(laser_type)
	{
		case weapon_id_type::CONCUSSION_ID:
			return "CONCUSSION";
		case weapon_id_type::HOMING_ID:
			return "HOMING";
		case weapon_id_type::SMART_ID:
			return "SMART";
		case weapon_id_type::MEGA_ID:
			return "MEGA";
		case weapon_id_type::FLASH_ID:
			return "FLASH";
		case weapon_id_type::MERCURY_ID:
			return "MERCURY";
		case weapon_id_type::EARTHSHAKER_ID:
			return "SHAKER";
		default:
			return "MISSILE";	// Bad!
	}
}

static void set_missile_viewer(object &o)
{
	Missile_viewer = &o;
	Missile_viewer_sig = o.signature;
}

static int clear_missile_viewer()
{
        if (!Missile_viewer)
                return 0;
        Missile_viewer = nullptr;
        return 1;
}

[[nodiscard]]
static bool is_viewable_missile(weapon_id_type laser_type)
{
	return laser_type == weapon_id_type::CONCUSSION_ID ||
		laser_type == weapon_id_type::HOMING_ID ||
		laser_type == weapon_id_type::SMART_ID ||
		laser_type == weapon_id_type::MEGA_ID ||
		laser_type == weapon_id_type::FLASH_ID ||
		laser_type == weapon_id_type::GUIDEDMISS_ID ||
		laser_type == weapon_id_type::MERCURY_ID ||
		laser_type == weapon_id_type::EARTHSHAKER_ID;
}

static bool choose_missile_viewer()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	auto &vmobjptr = Objects.vmptr;
	const auto MissileViewEnabled = PlayerCfg.MissileViewEnabled;
	if (unlikely(MissileViewEnabled == MissileViewMode::None))
		return false;
	const auto need_new_missile_viewer = []{
		if (!Missile_viewer)
			return true;
		if (Missile_viewer->type != OBJ_WEAPON)
			return true;
		if (Missile_viewer->signature != Missile_viewer_sig)
			return true;
		/* No check on parent here.  Missile_viewer is cleared if needed
		 * when a missile is fired.
		 */
		return false;
	};
	if (likely(!need_new_missile_viewer()))
		/* Valid viewer already set */
		return true;
	const auto better_match = [](object *const a, object &b) {
		if (!a)
			return true;
		return a->lifeleft < b.lifeleft;
	};
	/* Find new missile */
	object *local_player_missile = nullptr, *other_player_missile = nullptr;
	const auto game_mode = Game_mode;
	const auto local_player_objnum = get_local_player().objnum;
	range_for (object &o, vmobjptr)
	{
		if (o.type != OBJ_WEAPON)
			continue;
		if (o.ctype.laser_info.parent_type != OBJ_PLAYER)
			continue;
		const auto laser_type = get_weapon_id(o);
		if (!is_viewable_missile(laser_type))
			continue;
		if (o.ctype.laser_info.parent_num == local_player_objnum)
		{
			if (!better_match(local_player_missile, o))
				continue;
			local_player_missile = &o;
		}
		else
		{
			if (MissileViewEnabled != MissileViewMode::EnabledSelfAndAllies)
				continue;
			if (!better_match(other_player_missile, o))
				continue;
			else if (game_mode & GM_MULTI_COOP)
			{
				/* Always allow missiles of other players */
			}
			else if (game_mode & GM_TEAM)
			{
				/* Allow missiles from same team */
				if (get_team(Player_num) != get_team(get_player_id(vcobjptr(o.ctype.laser_info.parent_num))))
					continue;
			}
			else
				continue;
			other_player_missile = &o;
		}
	}
	object *o;
	if ((o = local_player_missile) != nullptr ||
		(o = other_player_missile) != nullptr)
		set_missile_viewer(*o);
	else
		return false;
	return true;
}

static void show_one_extra_view(grs_canvas &canvas, const gauge_inset_window_view w);
static void show_extra_views(grs_canvas &canvas)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	int did_missile_view=0;
	int save_newdemo_state = Newdemo_state;
	if (Newdemo_state==ND_STATE_PLAYBACK)
	{
		if (DemoDoLeft)
		{
			DemoDoingLeft=DemoDoLeft;

			if (DemoDoLeft==3)
				do_cockpit_window_view(canvas, gauge_inset_window_view::primary, *ConsoleObject, 1, weapon_box_user::rear, "REAR");
			else
				do_cockpit_window_view(canvas, gauge_inset_window_view::primary, DemoLeftExtra, DemoRearCheck[DemoDoLeft], DemoWBUType[DemoDoLeft], DemoExtraMessage[DemoDoLeft], DemoDoLeft == 1 ? &get_local_plrobj().ctype.player_info : nullptr);
		}
		else
			do_cockpit_window_view(gauge_inset_window_view::primary, weapon_box_user::weapon);
	
		if (DemoDoRight)
		{
			DemoDoingRight=DemoDoRight;
			
			if (DemoDoRight==3)
				do_cockpit_window_view(canvas, gauge_inset_window_view::secondary, *ConsoleObject, 1, weapon_box_user::rear, "REAR");
			else
				do_cockpit_window_view(canvas, gauge_inset_window_view::secondary, DemoRightExtra, DemoRearCheck[DemoDoRight], DemoWBUType[DemoDoRight], DemoExtraMessage[DemoDoRight], DemoDoLeft == 1 ? &get_local_plrobj().ctype.player_info : nullptr);
		}
		else
			do_cockpit_window_view(gauge_inset_window_view::secondary, weapon_box_user::weapon);
		
		DemoDoLeft=DemoDoRight=0;
		DemoDoingLeft=DemoDoingRight=0;
		return;
	}

	const auto &&gimobj = LevelUniqueObjectState.Guided_missile.get_player_active_guided_missile(vmobjptr, Player_num);
	if (gimobj != nullptr)
	{
		if (PlayerCfg.GuidedInBigWindow)
		{
			RenderingType=6+(1<<4);
			do_cockpit_window_view(canvas, gauge_inset_window_view::secondary, *Viewer, 0, weapon_box_user::missile, "SHIP");
		}
		else
		{
			RenderingType=1+(1<<4);
			auto &player_info = get_local_plrobj().ctype.player_info;
			do_cockpit_window_view(canvas, gauge_inset_window_view::secondary, *gimobj, 0, weapon_box_user::guided, "GUIDED", &player_info);
		}
			
		did_missile_view=1;
	}
	else {
		if (choose_missile_viewer())
		//do missile view
			{
  				RenderingType=2+(1<<4);
				do_cockpit_window_view(canvas, gauge_inset_window_view::secondary, *Missile_viewer, 0, weapon_box_user::missile, get_missile_name(get_weapon_id(*Missile_viewer)));
				did_missile_view=1;
			}
			else {
				if (clear_missile_viewer())
					do_cockpit_window_view(gauge_inset_window_view::secondary, weapon_box_user::post_missile_static);
                                RenderingType=255;
			}
	}
	show_one_extra_view(canvas, gauge_inset_window_view::primary);
	if (!did_missile_view)		//if showing missile view in right window, can't show anything else
		show_one_extra_view(canvas, gauge_inset_window_view::secondary);
	RenderingType=0;
	Newdemo_state = save_newdemo_state;
}

static void show_one_extra_view(grs_canvas &canvas, const gauge_inset_window_view w)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	auto &vmobjptridx = Objects.vmptridx;
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
		//show special views if selected
		switch (PlayerCfg.Cockpit3DView[w]) {
			case cockpit_3d_view::None:
				RenderingType=255;
				do_cockpit_window_view(w, weapon_box_user::weapon);
				break;
			case cockpit_3d_view::Rear:
				RenderingType = 3 + (static_cast<unsigned>(w) << 4);
				{
					int rear_view_flag;
					const char *label;
					if (Rear_view)	//if big window is rear view, show front here
					{
						rear_view_flag = 0;
						label = "FRONT";
					}
					else	//show normal rear view
					{
						rear_view_flag = 1;
						label = "REAR";
					}
					do_cockpit_window_view(canvas, w, *ConsoleObject, rear_view_flag, weapon_box_user::rear, label);
				}
			 	break;
			case cockpit_3d_view::Escort: {
				const auto &&buddy = find_escort(vmobjptridx, Robot_info);
				if (buddy == object_none) {
					do_cockpit_window_view(w, weapon_box_user::weapon);
					PlayerCfg.Cockpit3DView[w] = cockpit_3d_view::None;
				}
				else {
					RenderingType = 4 + (static_cast<unsigned>(w) << 4);
					do_cockpit_window_view(canvas, w, *buddy, 0, weapon_box_user::escort, PlayerCfg.GuidebotName);
				}
				break;
			}
			case cockpit_3d_view::Coop: {
				const auto player = Coop_view_player[w];

	         RenderingType=255; // don't handle coop stuff			
				
				if (player < Players.size() && vcplayerptr(player)->connected != player_connection_status::disconnected && ((Game_mode & GM_MULTI_COOP) || ((Game_mode & GM_TEAM) && (get_team(player) == get_team(Player_num)))))
				{
					auto &p = *vcplayerptr(player);
					do_cockpit_window_view(canvas, w, *vcobjptr(p.objnum), 0, weapon_box_user::coop, p.callsign);
				}
				else {
					do_cockpit_window_view(w, weapon_box_user::weapon);
					PlayerCfg.Cockpit3DView[w] = cockpit_3d_view::None;
				}
				break;
			}
			case cockpit_3d_view::Marker: {
				char label[10];
				RenderingType = 5 + (static_cast<unsigned>(w) << 4);
				const auto mvn = Marker_viewer_num[w];
				if (!MarkerState.imobjidx.valid_index(mvn))
				{
					PlayerCfg.Cockpit3DView[w] = cockpit_3d_view::None;
					break;
				}
				const auto mo = MarkerState.imobjidx[mvn];
				if (mo == object_none)
				{
					PlayerCfg.Cockpit3DView[w] = cockpit_3d_view::None;
					break;
				}
				const auto displayed_marker_id = static_cast<unsigned>(mvn) + 1;
				snprintf(label, sizeof(label), "Marker %u", displayed_marker_id);
				do_cockpit_window_view(canvas, w, *vcobjptr(mo), 0, weapon_box_user::marker, label);
				break;
			}
			default:
				Int3();		//invalid window type
		}
}

}

int BigWindowSwitch=0;
#endif

//render a frame for the game
void game_render_frame_mono(const d_robot_info_array &Robot_info, const control_info &Controls)
{
	int no_draw_hud = 0;

	gr_set_current_canvas(Screen_3d_window);
	{
	auto &canvas = *grd_curcanv;
#if defined(DXX_BUILD_DESCENT_II)
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	if (const auto &&gimobj = (
			PlayerCfg.GuidedInBigWindow
			? LevelUniqueObjectState.Guided_missile.get_player_active_guided_missile(LevelUniqueObjectState.get_objects().vmptr, Player_num)
			: nullptr))
	{
		const auto viewer_save = Viewer;

		if (PlayerCfg.CockpitMode[1] == cockpit_mode_t::full_cockpit || PlayerCfg.CockpitMode[1] == cockpit_mode_t::rear_view)
		{
			 BigWindowSwitch=1;
			 force_cockpit_redraw=1;
			 PlayerCfg.CockpitMode[1] = cockpit_mode_t::status_bar;
			 return;
		}

		Viewer = gimobj;

		window_rendered_data window;
#if DXX_USE_STEREOSCOPIC_RENDER
		if (VR_stereo != StereoFormat::None)
		{
			render_frame(canvas, -VR_eye_width, window);
			render_frame(canvas, +VR_eye_width, window);
		}
		else
#endif
		{
			render_frame(canvas, 0, window);
		}

		wake_up_rendered_objects(*Viewer, window);
		show_HUD_names(Robot_info, canvas, Game_mode);

		Viewer = viewer_save;

		auto &game_font = *GAME_FONT;
		gr_set_fontcolor(canvas, BM_XRGB(27, 0, 0), -1);

		gr_string(canvas, game_font, 0x8000, FSPACY(1), "Guided Missile View");

		auto &player_info = get_local_plrobj().ctype.player_info;
		show_reticle(canvas, player_info, RET_TYPE_CROSS_V1, 0);

		HUD_render_message_frame(canvas);

		no_draw_hud=1;
	}
	else
#endif
	{
#if defined(DXX_BUILD_DESCENT_II)
		if (BigWindowSwitch)
		{
			force_cockpit_redraw=1;
			PlayerCfg.CockpitMode[1] = (Rear_view ? cockpit_mode_t::rear_view : cockpit_mode_t::full_cockpit);
			BigWindowSwitch=0;
			return;
		}
#endif
		window_rendered_data window;
#if DXX_USE_STEREOSCOPIC_RENDER
		if (VR_stereo != StereoFormat::None)
		{
			render_frame(canvas, -VR_eye_width, window);
			render_frame(canvas, +VR_eye_width, window);
		}
		else
#endif
		{
			render_frame(canvas, 0, window);
		}
	}
	}
	gr_set_default_canvas();
	{
	auto &canvas = *grd_curcanv;
	update_cockpits(canvas);

	if (PlayerCfg.CockpitMode[1] == cockpit_mode_t::full_cockpit || PlayerCfg.CockpitMode[1] == cockpit_mode_t::status_bar)
		render_gauges(canvas, Newdemo_state == ND_STATE_PLAYBACK ? Newdemo_game_mode : Game_mode);
	}

	if (!no_draw_hud) {
#if DXX_USE_STEREOSCOPIC_RENDER
		if (VR_stereo != StereoFormat::None)
		{
			game_draw_hud_stuff(Robot_info, VR_hud_left, Controls);
			game_draw_hud_stuff(Robot_info, VR_hud_right, Controls);
		}
		else
#endif
		{
			game_draw_hud_stuff(Robot_info, Screen_3d_window, Controls);
		}
	}

#if defined(DXX_BUILD_DESCENT_II)
	gr_set_default_canvas();
	show_extra_views(*grd_curcanv);		//missile view, buddy bot, etc.
#endif

	if (netplayerinfo_on && Game_mode & GM_MULTI)
	{
		gr_set_default_canvas();
		show_netplayerinfo(*grd_curcanv);
	}
}

}

void toggle_cockpit()
{
	if (Rear_view || Player_dead_state != player_dead_state::no)
		return;
#if DXX_USE_STEREOSCOPIC_RENDER
	if (VR_stereo != StereoFormat::None)
		return;
#endif

	auto new_mode = cockpit_mode_t::full_screen;

	switch (PlayerCfg.CockpitMode[1])
	{
		case cockpit_mode_t::full_cockpit:
			new_mode = cockpit_mode_t::status_bar;
			break;
		case cockpit_mode_t::status_bar:
			new_mode = cockpit_mode_t::full_screen;
			break;
		case cockpit_mode_t::full_screen:
			new_mode = cockpit_mode_t::full_cockpit;
			break;
		case cockpit_mode_t::rear_view:
		case cockpit_mode_t::letterbox:
			break;
	}

	select_cockpit(new_mode);
	HUD_clear_messages();
	PlayerCfg.CockpitMode[0] = new_mode;
	write_player_file();
}

namespace dsx {
namespace {

// This actually renders the new cockpit onto the screen.
static void update_cockpits(grs_canvas &canvas)
{
	const auto raw_cockpit_mode = PlayerCfg.CockpitMode[1];
	auto mode =
#if defined(DXX_BUILD_DESCENT_II)
		HIRESMODE
			? static_cast<cockpit_mode_t>(underlying_value(raw_cockpit_mode) + (Num_cockpits / 2))
			:
#endif
		raw_cockpit_mode;

	switch( PlayerCfg.CockpitMode[1] )	{
		case cockpit_mode_t::full_cockpit:
		case cockpit_mode_t::rear_view:
			{
				const auto bi = cockpit_bitmap[mode];
				PIGGY_PAGE_IN(bi);
				grs_bitmap *const bm = &GameBitmaps[bi];
#if DXX_USE_OGL
			ogl_ubitmapm_cs(canvas, 0, 0, opengl_bitmap_use_dst_canvas, opengl_bitmap_use_dst_canvas, *bm, 255);
#else
			gr_ubitmapm(canvas, 0, 0, *bm);
#endif
			}
			break;
	
		case cockpit_mode_t::full_screen:
			break;
	
		case cockpit_mode_t::status_bar:
			{
				const auto bi = cockpit_bitmap[mode];
				PIGGY_PAGE_IN(bi);
				grs_bitmap *const bm = &GameBitmaps[bi];
#if DXX_USE_OGL
			ogl_ubitmapm_cs(canvas, 0, (HIRESMODE ? (SHEIGHT * 2) / 2.6 : (SHEIGHT * 2) / 2.72), opengl_bitmap_use_dst_canvas, (static_cast<int>(static_cast<double>(bm->bm_h) * (HIRESMODE ? static_cast<double>(SHEIGHT) / 480 : static_cast<double>(SHEIGHT) / 200) + 0.5)), *bm, 255);
#else
			gr_ubitmapm(canvas, 0, SHEIGHT - bm->bm_h, *bm);
#endif
			}
			break;
	
		case cockpit_mode_t::letterbox:
			break;
	}
	if (PlayerCfg.CockpitMode[1] != last_drawn_cockpit)
		last_drawn_cockpit = PlayerCfg.CockpitMode[1];
	else
		return;

	if (PlayerCfg.CockpitMode[1] == cockpit_mode_t::full_cockpit || PlayerCfg.CockpitMode[1] == cockpit_mode_t::status_bar)
		init_gauges();

}
}

void game_render_frame(const d_robot_info_array &Robot_info, const control_info &Controls)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	set_screen_mode( SCREEN_GAME );
	auto &player_info = get_local_plrobj().ctype.player_info;
	play_homing_warning(player_info);
	gr_set_default_canvas();
	game_render_frame_mono(Robot_info, Controls);
}

}

//show a message in a nice little box
void show_boxed_message(grs_canvas &canvas, const char *msg)
{
	int x,y;
	int center = 0x8000;
	
	gr_set_fontcolor(canvas, BM_XRGB(31, 31, 31), -1);
	auto &medium1_font = *MEDIUM1_FONT;
	auto &&[w, h] = gr_get_string_size(medium1_font, msg);
	
	x = (SWIDTH-w)/2;
	y = (SHEIGHT-h)/2;
	
#if DXX_USE_STEREOSCOPIC_RENDER
	if (VR_stereo != StereoFormat::None) {
		int dw = w, dh = h;
		gr_stereo_viewport_window(VR_stereo, x, y, dw, dh);
		gr_stereo_viewport_offset(VR_stereo, x, y);
		x -= (w - dw) / 2;
		center = x;
	}
#endif

	nm_draw_background(canvas, x - BORDERX, y - BORDERY, x + w + BORDERX, y + h + BORDERY);
	
	gr_string(canvas, medium1_font, center, y, msg, w, h);
}
