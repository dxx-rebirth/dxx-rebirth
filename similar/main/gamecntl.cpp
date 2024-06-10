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
 * Game Controls Stuff
 *
 */

#include <algorithm>
#include <stdio.h>
#include <cstdlib>
#include <string.h>
#include <stdarg.h>
#include <ranges>

#include "window.h"
#include "console.h"
#include "collide.h"
#include "strutil.h"
#include "game.h"
#include "player.h"
#include "key.h"
#include "object.h"
#include "menu.h"
#include "dxxerror.h"
#include "joy.h"
#include "timer.h"
#include "render.h"
#include "laser.h"
#include "screens.h"
#include "textures.h"
#include "slew.h"
#include "gauges.h"
#include "texmap.h"
#include "3d.h"
#include "gameseg.h"
#include "wall.h"
#include "ai.h"
#include "rbaudio.h"
#include "digi.h"
#include "u_mem.h"
#include "palette.h"
#include "morph.h"
#include "robot.h"
#include "lighting.h"
#include "newdemo.h"
#include "weapon.h"
#include "sounds.h"
#include "args.h"
#include "gameseq.h"
#include "automap.h"
#include "text.h"
#include "powerup.h"
#include "songs.h"
#include "newmenu.h"
#include "gamefont.h"
#include "endlevel.h"
#include "config.h"
#include "hudmsg.h"
#include "kconfig.h"
#include "titles.h"
#include "gr.h"
#include "playsave.h"

#include "multi.h"
#include "cntrlcen.h"
#include "fuelcen.h"
#include "state.h"
#include "piggy.h"
#include "ai.h"
#include "rbaudio.h"
#include "switch.h"
#if defined(DXX_BUILD_DESCENT_II)
#include "escort.h"
#include "movie.h"
#endif
#include "window.h"

#if DXX_USE_EDITOR
#include "editor/editor.h"
#include "editor/esegment.h"
#endif

#include "compiler-range_for.h"
#include "d_levelstate.h"
#include "d_range.h"
#include "partial_range.h"
#include <array>
#include <utility>

#include <SDL.h>

#if defined(__GNUC__) && defined(WIN32)
/* Mingw64 _mingw_print_pop.h changes PRIi64 to POSIX-style.  Change it
 * back here.
 *
 * Some outdated mingw32 users are also affected.
 */
#undef PRIi64
#define PRIi64 "I64i"
#endif

using std::min;

namespace dcx {

namespace {

struct pause_window : window
{
	using window::window;
	std::array<char, 128> msg;
};

}

}

namespace dsx {

namespace {

struct pause_window : ::dcx::pause_window
{
	using ::dcx::pause_window::pause_window;
	virtual window_event_result event_handler(const d_event &) override;
};

#ifndef RELEASE
static void do_cheat_menu(object &plrobj, grs_canvas &cv_canvas);
static void play_test_sound();
#endif

int allowed_to_fire_flare(player_info &player_info)
{
	auto &Next_flare_fire_time = player_info.Next_flare_fire_time;
	if (Next_flare_fire_time > GameTime64)
		return 0;
#if defined(DXX_BUILD_DESCENT_II)
	if (player_info.energy < Weapon_info[weapon_id_type::FLARE_ID].energy_usage)
#define	FLARE_BIG_DELAY	(F1_0*2)
		Next_flare_fire_time = GameTime64 + FLARE_BIG_DELAY;
	else
#endif
		Next_flare_fire_time = GameTime64 + F1_0/4;
	return 1;
}

#define key_isfunc(k) (((k&0xff)>=KEY_F1 && (k&0xff)<=KEY_F10) || (k&0xff)==KEY_F11 || (k&0xff)==KEY_F12)

// Functions ------------------------------------------------------------------

#if defined(DXX_BUILD_DESCENT_II)
#define CONVERTER_RATE  20		//10 units per second xfer rate
#define CONVERTER_SCALE  2		//2 units energy -> 1 unit shields

#define CONVERTER_SOUND_DELAY (f1_0/2)		//play every half second

static void transfer_energy_to_shield(object &plrobj)
{
	static fix64 last_play_time=0;

	auto &player_info = plrobj.ctype.player_info;
	auto &shields = plrobj.shields;
	auto &energy = player_info.energy;
	//how much energy gets transfered
	const fix e = min(min(FrameTime*CONVERTER_RATE, energy - INITIAL_ENERGY), (MAX_SHIELDS - shields) * CONVERTER_SCALE);

	if (e <= 0) {

		if (energy <= INITIAL_ENERGY) {
			HUD_init_message(HM_DEFAULT, "Need more than %i energy to enable transfer", f2i(INITIAL_ENERGY));
		}
		else if (shields >= MAX_SHIELDS)
		{
			HUD_init_message_literal(HM_DEFAULT, "No transfer: Shields already at max");
		}
		return;
	}

	energy  -= e;
	shields += e / CONVERTER_SCALE;

	if (last_play_time > GameTime64)
		last_play_time = 0;

	if (GameTime64 > last_play_time+CONVERTER_SOUND_DELAY) {
		last_play_time = {GameTime64};
		digi_play_sample_once(SOUND_CONVERT_ENERGY, F1_0);
	}
}
#endif

}

}

namespace {

// Control Functions

static fix64 newdemo_single_frame_time;

static void update_vcr_state(void)
{
	if ((keyd_pressed[KEY_LSHIFT] || keyd_pressed[KEY_RSHIFT]) && keyd_pressed[KEY_RIGHT] && d_tick_step)
		Newdemo_vcr_state = ND_STATE_FASTFORWARD;
	else if ((keyd_pressed[KEY_LSHIFT] || keyd_pressed[KEY_RSHIFT]) && keyd_pressed[KEY_LEFT] && d_tick_step)
		Newdemo_vcr_state = ND_STATE_REWINDING;
	else if (!(keyd_pressed[KEY_LCTRL] || keyd_pressed[KEY_RCTRL]) && keyd_pressed[KEY_RIGHT] && ((GameTime64 - newdemo_single_frame_time) >= F1_0) && d_tick_step)
		Newdemo_vcr_state = ND_STATE_ONEFRAMEFORWARD;
	else if (!(keyd_pressed[KEY_LCTRL] || keyd_pressed[KEY_RCTRL]) && keyd_pressed[KEY_LEFT] && ((GameTime64 - newdemo_single_frame_time) >= F1_0) && d_tick_step)
		Newdemo_vcr_state = ND_STATE_ONEFRAMEBACKWARD;
	else if ((Newdemo_vcr_state == ND_STATE_FASTFORWARD) || (Newdemo_vcr_state == ND_STATE_REWINDING))
		Newdemo_vcr_state = ND_STATE_PLAYBACK;
}

}

namespace dsx {
#if defined(DXX_BUILD_DESCENT_II)

namespace {

//returns which bomb will be dropped next time the bomb key is pressed
template <typename T>
secondary_weapon_index_t read_update_which_proximity_mine_to_use(T &player_info)
{
	//use the last one selected, unless there aren't any, in which case use
	//the other if there are any
	auto &Secondary_last_was_super = player_info.Secondary_last_was_super;
	const auto mask = HAS_SECONDARY_FLAG(secondary_weapon_index_t::PROXIMITY_INDEX);
	const auto [bomb, alt_bomb] = (Secondary_last_was_super & mask)
		? std::pair(secondary_weapon_index_t::SMART_MINE_INDEX, secondary_weapon_index_t::PROXIMITY_INDEX)
		: std::pair(secondary_weapon_index_t::PROXIMITY_INDEX, secondary_weapon_index_t::SMART_MINE_INDEX);
	auto &secondary_ammo = player_info.secondary_ammo;
	if (secondary_ammo[bomb])
		/* Player has the requested bomb type available.  Use it. */
		return bomb;
	if (secondary_ammo[alt_bomb])
	{
		/* Player has the alternate bomb type, but not the requested
		 * bomb type.  Switch.
		 */
		if constexpr (!std::is_const<T>::value)
			Secondary_last_was_super ^= mask;
		return alt_bomb;
	}
	/* Player has no bombs of either type. */
	return bomb;
}

}

secondary_weapon_index_t which_bomb(const player_info &player_info)
{
	return read_update_which_proximity_mine_to_use(player_info);
}

secondary_weapon_index_t which_bomb(player_info &player_info)
{
	return read_update_which_proximity_mine_to_use(player_info);
}
#endif

namespace {

static void do_weapon_n_item_stuff(object_array &Objects, control_info &Controls)
{
	auto &vmobjptridx = Objects.vmptridx;
	const auto &&plrobjidx = vmobjptridx(get_local_player().objnum);
	auto &plrobj = *plrobjidx;
	auto &player_info = plrobj.ctype.player_info;
	if (Controls.state.fire_flare > 0)
	{
		Controls.state.fire_flare = 0;
		if (allowed_to_fire_flare(player_info))
			Flare_create(plrobjidx);
	}

	if (Controls.state.fire_secondary && allowed_to_fire_missile(player_info))
	{
		Global_missile_firing_count += Weapon_info[Secondary_weapon_to_weapon_info[player_info.Secondary_weapon]].fire_count;
	}

	if (Global_missile_firing_count) {
		--Global_missile_firing_count;
		do_missile_firing(player_info.Secondary_weapon, plrobjidx);
	}

	if (Controls.state.cycle_primary > 0)
	{
		for (uint_fast32_t i = std::exchange(Controls.state.cycle_primary, 0); i--;)
			CyclePrimary(player_info);
	}
	if (Controls.state.cycle_secondary > 0)
	{
		for (uint_fast32_t i = std::exchange(Controls.state.cycle_secondary, 0); i--;)
			CycleSecondary(player_info);
	}
	if (Controls.state.select_weapon > 0)
	{
		const auto select_weapon = std::exchange(Controls.state.select_weapon, 0) - 1;
		const auto weapon_num = select_weapon > 4 ? select_weapon - 5 : select_weapon;
		if (select_weapon > 4)
			do_secondary_weapon_select(player_info, static_cast<secondary_weapon_index_t>(select_weapon - 5));
		else
			do_primary_weapon_select(player_info, static_cast<primary_weapon_index_t>(weapon_num));
	}
#if defined(DXX_BUILD_DESCENT_II)
	if (auto &headlight = Controls.state.headlight)
	{
		if (std::exchange(headlight, 0) & 1)
			toggle_headlight_active(plrobj);
	}
#endif

	if (Global_missile_firing_count < 0)
		Global_missile_firing_count = 0;

	//	Drop proximity bombs.
	if (Controls.state.drop_bomb > 0)
	{
		const auto bomb = which_bomb(player_info);
		for (uint_fast32_t i = std::exchange(Controls.state.drop_bomb, 0); i--;)
			do_missile_firing(bomb, plrobjidx);
	}
#if defined(DXX_BUILD_DESCENT_II)
	if (Controls.state.toggle_bomb > 0)
	{
		auto &Secondary_last_was_super = player_info.Secondary_last_was_super;
		auto &secondary_ammo = player_info.secondary_ammo;
		int sound;
		if (!secondary_ammo[secondary_weapon_index_t::PROXIMITY_INDEX] && !secondary_ammo[secondary_weapon_index_t::SMART_MINE_INDEX])
		{
			HUD_init_message_literal(HM_DEFAULT, "No bombs available!");
			sound = SOUND_BAD_SELECTION;
		}
		else
		{	
			const auto mask = HAS_SECONDARY_FLAG(secondary_weapon_index_t::PROXIMITY_INDEX);
			const auto &&[desc, bomb] = (Secondary_last_was_super & mask) ? std::pair("Proximity bombs", secondary_weapon_index_t::PROXIMITY_INDEX) : std::pair("Smart mines", secondary_weapon_index_t::SMART_MINE_INDEX);
			if (secondary_ammo[bomb] == 0)
			{
				HUD_init_message(HM_DEFAULT, "No %s available!", desc);
				sound = SOUND_BAD_SELECTION;
			}
			else
			{
				Secondary_last_was_super ^= mask;
				sound = SOUND_GOOD_SELECTION_SECONDARY;
			}
		}
		digi_play_sample_once(sound, F1_0);
		Controls.state.toggle_bomb = 0;
	}

	if (Controls.state.energy_to_shield && (player_info.powerup_flags & PLAYER_FLAGS_CONVERTER))
		transfer_energy_to_shield(plrobj);
#endif
}

}
}

namespace dcx {

namespace {

#ifndef RELEASE
#if DXX_USE_EDITOR
struct choose_curseg_menu_items
{
	std::array<char, 40> caption;
	std::array<char, sizeof("65535")> text;
	std::array<newmenu_item, 2> m;
	choose_curseg_menu_items(const d_level_shared_segment_state &LevelSharedSegmentState) :
		m{{
			newmenu_item::nm_item_text{(snprintf(caption.data(), caption.size(), "Enter target segment number (max=%u)", LevelSharedSegmentState.get_segments().get_count()), caption.data())},
			newmenu_item::nm_item_input{(text.front() = 0, text)},
		}}
	{
	}
};

struct choose_curseg_menu : choose_curseg_menu_items, passive_newmenu
{
	choose_curseg_menu(const d_level_shared_segment_state &LevelSharedSegmentState, grs_canvas &src) :
		choose_curseg_menu_items(LevelSharedSegmentState),
		passive_newmenu(menu_title{nullptr}, menu_subtitle{nullptr}, menu_filename{nullptr}, tiny_mode_flag::normal, tab_processing_flag::ignore, adjusted_citem::create(m, 0), src)
		{
		}
	virtual window_event_result event_handler(const d_event &) override;
};

window_event_result choose_curseg_menu::event_handler(const d_event &event)
{
	switch (event.type)
	{
		case event_type::window_close:
			{
				char *p;
				const auto s = strtoul(text.data(), &p, 0);
				if (*p)
					break;
				if (s >= Segments.get_count())
					break;
				Cursegp = Segments.vmptridx(static_cast<segnum_t>(s));
				break;
			}
		default:
			break;
	}
	return newmenu::event_handler(event);
}
#endif
#endif

}

}

namespace dsx {

namespace {

//Process selected keys until game unpaused
window_event_result pause_window::event_handler(const d_event &event)
{
	switch (event.type)
	{
		case event_type::window_activated:
			game_flush_inputs(Controls);
			break;

		case event_type::key_command:
			switch (event_key_get(event))
			{
				case 0:
					break;
				case KEY_ESC:
					return window_event_result::close;
				case KEY_F1:
					show_help();
					return window_event_result::handled;
				case KEY_PAUSE:
					return window_event_result::close;
				default:
					break;
			}
			break;

		case event_type::idle:
			break;

		case event_type::window_draw:
			gr_set_default_canvas();
			show_boxed_message(*grd_curcanv, msg.data());
			break;

		case event_type::window_close:
			songs_resume();
			break;

		default:
			break;
	}
	return window_event_result::ignored;
}

static void do_game_pause()
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;

	if (Game_mode & GM_MULTI)
	{
		netplayerinfo_on= !netplayerinfo_on;
		return;
	}

	auto p = window_create<pause_window>(grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT);
	songs_pause();

	auto &plr = get_local_player();
	struct hms_time
	{
		unsigned h, m, s;
		hms_time(const unsigned hours_extra, const int d1_rem, const std::div_t d2) :
			h(d2.quot + hours_extra), m(d2.rem), s(d1_rem)
		{
		}
		hms_time(const unsigned hours_extra, const std::div_t d1) :
			hms_time(hours_extra, d1.rem, std::div(d1.quot, 60))
		{
		}
		hms_time(const unsigned hours_extra, const fix seconds) :
			hms_time(hours_extra, std::div(f2i(seconds), 60))
		{
		}
	};
	auto &player_info = vcobjptr(plr.objnum)->ctype.player_info;
	if (Newdemo_state!=ND_STATE_PLAYBACK)
	{
		const hms_time human_time_total{plr.hours_total, plr.time_total}, human_time_level{plr.hours_level, plr.time_level};
		snprintf(&p->msg[0], p->msg.size(), "PAUSE\n\n"
				 "Skill level:  %s\n"
				 "Hostages on board:  %d\n"
				 "Time on level: %1u:%02u:%02u\n"
				 "Total time in game: %1u:%02u:%02u",
				 MENU_DIFFICULTY_TEXT(GameUniqueState.Difficulty_level), player_info.mission.hostages_on_board, human_time_level.h, human_time_level.m, human_time_level.s, human_time_total.h, human_time_total.m, human_time_total.s);
	}
	else
		snprintf(&p->msg[0], p->msg.size(), "PAUSE\n\n\n\n");
	set_screen_mode(SCREEN_MENU);

	// Keycode returning ripped out (kreatordxx)
}

static window_event_result HandleEndlevelKey(int key)
{
	switch (key)
	{
		case KEY_COMMAND+KEY_P:
		case KEY_PAUSE:
			do_game_pause();
			return window_event_result::handled;

		case KEY_ESC:
			last_drawn_cockpit = cockpit_mode_t{UINT8_MAX};
			return stop_endlevel_sequence();
	}

	return window_event_result::ignored;
}

static int HandleDeathInput(const d_event &event, control_info &Controls)
{
	const auto input_aborts_death_sequence = [&]() {
		const auto RespawnMode = PlayerCfg.RespawnMode;
		if (event.type == event_type::key_command)
		{
			const auto key = event_key_get(event);
			if ((RespawnMode == RespawnPress::Any && !key_isfunc(key) && key != KEY_PAUSE && key) ||
				(key == KEY_ESC && ConsoleObject->flags & OF_EXPLODING))
				return 1;
		}

		if (RespawnMode == RespawnPress::Any
			? (
#if DXX_MAX_BUTTONS_PER_JOYSTICK
				event.type == event_type::joystick_button_up ||
#endif
				event.type == event_type::mouse_button_up)
			: (Controls.state.fire_primary || Controls.state.fire_secondary || Controls.state.fire_flare))
			return 1;
		return 0;
	};
	if (Player_dead_state == player_dead_state::exploded && input_aborts_death_sequence())
	{
		GameViewUniqueState.Death_sequence_aborted = 1;
	}

	if (GameViewUniqueState.Death_sequence_aborted)
	{
		game_flush_respawn_inputs(Controls);
		return 1;
	}

	return 0;
}

#if DXX_USE_SCREENSHOT
static void save_pr_screenshot()
{
	gr_set_default_canvas();
	auto &canvas = *grd_curcanv;
	{
		window_rendered_data window;
		render_frame(canvas, 0, window);
	}
	auto &medium2_font = *MEDIUM2_FONT;
	gr_string(canvas, medium2_font, SWIDTH - FSPACX(92), SHEIGHT - LINE_SPACING(medium2_font, *GAME_FONT), "DXX-Rebirth\n");
	gr_flip();
	save_screen_shot(0);
}

static inline void game_render_frame_mono(const d_robot_info_array &Robot_info, int skip_flip, const control_info &Controls)
{
	game_render_frame_mono(Robot_info, Controls);
	if (!skip_flip)
		gr_flip();
}

static void save_clean_screenshot()
{
	game_render_frame_mono(LevelSharedRobotInfoState.Robot_info, CGameArg.DbgNoDoubleBuffer, Controls);
	save_screen_shot(0);
}
#endif

static window_event_result HandleDemoKey(int key)
{
	switch (key) {
		KEY_MAC(case KEY_COMMAND+KEY_1:)
		case KEY_F1:	show_newdemo_help();	break;
		KEY_MAC(case KEY_COMMAND+KEY_2:)
		case KEY_F2:	do_options_menu();	break;
		KEY_MAC(case KEY_COMMAND+KEY_3:)
		case KEY_F3:
			 if (Viewer->type == OBJ_PLAYER)
				toggle_cockpit();
			 break;
		KEY_MAC(case KEY_COMMAND+KEY_4:)
		case KEY_F4:	Newdemo_show_percentage = !Newdemo_show_percentage; break;
		KEY_MAC(case KEY_COMMAND+KEY_7:)
		case KEY_F7:
			Show_kill_list = static_cast<show_kill_list_mode>((underlying_value(Show_kill_list) + 1) % ((Newdemo_game_mode & GM_TEAM) ? 4 : 3));
			break;
		case KEY_ESC:
			if (CGameArg.SysAutoDemo)
			{
				int choice;
				choice = nm_messagebox_str(menu_title{nullptr}, nm_messagebox_tie(TXT_YES, TXT_NO), menu_subtitle{TXT_ABORT_AUTODEMO});
				if (choice == 0)
					CGameArg.SysAutoDemo = false;
				else
					break;
			}
			newdemo_stop_playback();
			return window_event_result::close;
			break;
		case KEY_UP:
			Newdemo_vcr_state = ND_STATE_PLAYBACK;
			break;
		case KEY_DOWN:
			Newdemo_vcr_state = ND_STATE_PAUSED;
			break;
		case KEY_LEFT:
			newdemo_single_frame_time = {GameTime64};
			Newdemo_vcr_state = ND_STATE_ONEFRAMEBACKWARD;
			break;
		case KEY_RIGHT:
			newdemo_single_frame_time = {GameTime64};
			Newdemo_vcr_state = ND_STATE_ONEFRAMEFORWARD;
			break;
		case KEY_CTRLED + KEY_RIGHT:
			return newdemo_goto_end(0);
			break;
		case KEY_CTRLED + KEY_LEFT:
			return newdemo_goto_beginning();
			break;

		KEY_MAC(case KEY_COMMAND+KEY_P:)
		case KEY_PAUSE:
			do_game_pause();
			break;

#if DXX_USE_SCREENSHOT
#ifdef macintosh
		case KEY_COMMAND + KEY_SHIFTED + KEY_3:
#endif
		case KEY_PRINT_SCREEN:
		{
			if (PlayerCfg.PRShot)
			{
				save_pr_screenshot();
			}
			else
			{
				int old_state;
				old_state = Newdemo_show_percentage;
				Newdemo_show_percentage = 0;
				save_clean_screenshot();
				Newdemo_show_percentage = old_state;
			}
			break;
		}
#endif
#ifndef NDEBUG
		case KEY_DEBUGGED + KEY_I:
			Newdemo_do_interpolate = !Newdemo_do_interpolate;
			HUD_init_message(HM_DEFAULT, "Demo playback interpolation %s", Newdemo_do_interpolate?"ON":"OFF");
			break;
#endif

		default:
			return window_event_result::ignored;
	}

	return window_event_result::handled;
}

#if defined(DXX_BUILD_DESCENT_II)
//switch a cockpit window to the next function
static int select_next_window_function(const gauge_inset_window_view w)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptridx = Objects.vmptridx;

	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	switch (PlayerCfg.Cockpit3DView[w]) {
		case cockpit_3d_view::None:
			PlayerCfg.Cockpit3DView[w] = cockpit_3d_view::Rear;
			break;
		case cockpit_3d_view::Rear:
			if (find_escort(vmobjptridx, Robot_info) != object_none)
			{
				PlayerCfg.Cockpit3DView[w] = cockpit_3d_view::Escort;
				break;
			}
			//if no ecort, fall through
			[[fallthrough]];
		case cockpit_3d_view::Escort:
			Coop_view_player[w] = UINT_MAX;		//force first player
			[[fallthrough]];
		case cockpit_3d_view::Coop:
			Marker_viewer_num[w] = game_marker_index::None;
			if ((Game_mode & GM_MULTI_COOP) || (Game_mode & GM_TEAM)) {
				PlayerCfg.Cockpit3DView[w] = cockpit_3d_view::Coop;
				for (;;)
				{
					const auto cvp = ++ Coop_view_player[w];
					if (cvp == MAX_PLAYERS - 1)
					{
						PlayerCfg.Cockpit3DView[w] = cockpit_3d_view::Marker;
						goto case_marker;
					}
					if (cvp == Player_num)
						continue;
					if (vcplayerptr(cvp)->connected != player_connection_status::playing)
						continue;

					if (Game_mode & GM_MULTI_COOP)
						break;
					else if (get_team(cvp) == get_team(Player_num))
						break;
				}
				break;
			}
			//if not multi,
			[[fallthrough]];
		case cockpit_3d_view::Marker:
		case_marker:;
			if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP) && Netgame.Allow_marker_view) {	//anarchy only
				PlayerCfg.Cockpit3DView[w] = cockpit_3d_view::Marker;
				auto &mvn = Marker_viewer_num[w];
				const auto gmi0 = convert_player_marker_index_to_game_marker_index(Game_mode, Netgame.max_numplayers, Player_num, player_marker_index::_0);
				if (!MarkerState.imobjidx.valid_index(mvn))
					mvn = gmi0;
				else
				{
					++ mvn;
					if (!MarkerState.imobjidx.valid_index(mvn))
						PlayerCfg.Cockpit3DView[w] = cockpit_3d_view::None;
				}
			}
			else
				PlayerCfg.Cockpit3DView[w] = cockpit_3d_view::None;
			break;
	}
	write_player_file();

	return 1;	 //screen_changed
}
#endif

//this is for system-level keys, such as help, etc.
//returns 1 if screen changed
static window_event_result HandleSystemKey(int key)
{
	if (Player_dead_state == player_dead_state::no)
		switch (key)
		{
			case KEY_ESC:
			{
				const bool allow_saveload = !(Game_mode & GM_MULTI) || ((Game_mode & GM_MULTI_COOP) && Player_num == 0);
				const auto choice = nm_messagebox_str(menu_title{nullptr}, allow_saveload ? nm_messagebox_tie("Abort Game", TXT_OPTIONS_, "Save Game...", TXT_LOAD_GAME) : nm_messagebox_tie("Abort Game", TXT_OPTIONS_), menu_subtitle{"Game Menu"});
				switch(choice)
				{
					case 0:
						return window_event_result::close;
					case 1:
						return HandleSystemKey(KEY_F2);
					case 2:
						return HandleSystemKey(KEY_ALTED | KEY_F2);
					case 3:
						return HandleSystemKey(KEY_ALTED | KEY_F3);
				}
				return window_event_result::handled;
			}
#if defined(DXX_BUILD_DESCENT_II)
// fleshed these out because F1 and F2 aren't sequenctial keycodes on mac -- MWA

			KEY_MAC(case KEY_COMMAND+KEY_SHIFTED+KEY_1:)
			case KEY_SHIFTED+KEY_F1:
				select_next_window_function(gauge_inset_window_view::primary);
				return window_event_result::handled;
			KEY_MAC(case KEY_COMMAND+KEY_SHIFTED+KEY_2:)
			case KEY_SHIFTED+KEY_F2:
				select_next_window_function(gauge_inset_window_view::secondary);
				return window_event_result::handled;
#endif
		}

	switch (key)
	{
                KEY_MAC( case KEY_COMMAND+KEY_PAUSE+KEY_SHIFTED: )
                case KEY_PAUSE+KEY_SHIFTED:
                        if (Game_mode & GM_MULTI)
                                show_netgame_info(Netgame);
                        break;
		KEY_MAC( case KEY_COMMAND+KEY_P: )
		case KEY_PAUSE:
			do_game_pause();
			break;


#if DXX_USE_SCREENSHOT
#ifdef macintosh
		case KEY_COMMAND + KEY_SHIFTED + KEY_3:
#endif
		case KEY_PRINT_SCREEN:
		{
			if (PlayerCfg.PRShot)
			{
				save_pr_screenshot();
			}
			else
			{
				save_clean_screenshot();
			}
			break;
		}
#endif

		KEY_MAC(case KEY_COMMAND+KEY_1:)
		case KEY_F1:				if (Game_mode & GM_MULTI) show_netgame_help(); else show_help();	break;

		KEY_MAC(case KEY_COMMAND+KEY_2:)
		case KEY_F2:
			{
				do_options_menu();
				break;
			}


		KEY_MAC(case KEY_COMMAND+KEY_3:)

		case KEY_F3:
			if (Player_dead_state == player_dead_state::no && Viewer->type == OBJ_PLAYER)
			{
				toggle_cockpit();
			}
			break;

		KEY_MAC(case KEY_COMMAND+KEY_5:)
		case KEY_F5:
			if ( Newdemo_state == ND_STATE_RECORDING )
				newdemo_stop_recording();
			else if ( Newdemo_state == ND_STATE_NORMAL )
				newdemo_start_recording();
			break;
		KEY_MAC(case KEY_COMMAND+KEY_ALTED+KEY_4:)
		case KEY_ALTED + KEY_F4:
			Show_reticle_name = !Show_reticle_name;
			break;

		KEY_MAC(case KEY_COMMAND+KEY_7:)
		case KEY_F7:
			Show_kill_list = static_cast<show_kill_list_mode>((underlying_value(Show_kill_list) + 1) % ((Game_mode & GM_TEAM) ? 4 : 3));
			if (Game_mode & GM_MULTI)
				multi_sort_kill_list();
			break;

		KEY_MAC(case KEY_COMMAND+KEY_8:)
		case KEY_F8:
			multi_send_message_start();
			break;

		case KEY_F9:
		case KEY_F10:
		case KEY_F11:
		case KEY_F12:
			multi_send_macro(key);
			break;		// send taunt macros

#if defined(__APPLE__) || defined(macintosh)
		case KEY_9 + KEY_COMMAND:
			multi_send_macro(KEY_F9);
			break;
		case KEY_0 + KEY_COMMAND:
			multi_send_macro(KEY_F10);
			break;
		case KEY_1 + KEY_COMMAND + KEY_CTRLED:
			multi_send_macro(KEY_F11);
			break;
		case KEY_2 + KEY_COMMAND + KEY_CTRLED:
			multi_send_macro(KEY_F12);
			break;
#endif

		case KEY_SHIFTED + KEY_F9:
		case KEY_SHIFTED + KEY_F10:
		case KEY_SHIFTED + KEY_F11:
		case KEY_SHIFTED + KEY_F12:
			multi_define_macro(key);
			break;		// redefine taunt macros

#if defined(__APPLE__) || defined(macintosh)
		case KEY_9 + KEY_SHIFTED + KEY_COMMAND:
			multi_define_macro(KEY_F9);
			break;
		case KEY_0 + KEY_SHIFTED + KEY_COMMAND:
			multi_define_macro(KEY_F10);
			break;
		case KEY_1 + KEY_SHIFTED + KEY_COMMAND + KEY_CTRLED:
			multi_define_macro(KEY_F11);
			break;
		case KEY_2 + KEY_SHIFTED + KEY_COMMAND + KEY_CTRLED:
			multi_define_macro(KEY_F12);
			break;
#endif


		KEY_MAC(case KEY_COMMAND+KEY_SHIFTED+KEY_S:)
		KEY_MAC(case KEY_COMMAND+KEY_ALTED+KEY_2:)
		case KEY_ALTED+KEY_F2:
			if (Player_dead_state == player_dead_state::no)
				state_save_all(secret_save::none, blind_save::no); // 0 means not between levels.
			break;

		KEY_MAC(case KEY_COMMAND+KEY_S:)
		case KEY_ALTED+KEY_SHIFTED+KEY_F2:
			if (Player_dead_state == player_dead_state::no)
				state_save_all(secret_save::none, blind_save::yes);
			break;
		KEY_MAC(case KEY_COMMAND+KEY_SHIFTED+KEY_O:)
		KEY_MAC(case KEY_COMMAND+KEY_ALTED+KEY_3:)
		case KEY_ALTED+KEY_F3:
			if (!((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP)))
				state_restore_all(1, secret_restore::none, nullptr, blind_save::no);
			break;
		KEY_MAC(case KEY_COMMAND+KEY_O:)
		case KEY_ALTED+KEY_SHIFTED+KEY_F3:
			if (!((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP)))
				state_restore_all(1, secret_restore::none, nullptr, blind_save::yes);
			break;

#if defined(DXX_BUILD_DESCENT_II)
		KEY_MAC(case KEY_COMMAND+KEY_SHIFTED+KEY_4:)
		case KEY_F4 + KEY_SHIFTED:
			do_escort_menu();
			break;


		KEY_MAC(case KEY_COMMAND+KEY_SHIFTED+KEY_ALTED+KEY_4:)
		case KEY_F4 + KEY_SHIFTED + KEY_ALTED:
			change_guidebot_name();
			break;
#endif

#if DXX_USE_STEREOSCOPIC_RENDER
#if 0
			/* These conflict with the drop-primary and drop-secondary
			 * keybindings.  Dropping items is more common than using VR, so
			 * disable these.
			 */
		case KEY_SHIFTED + KEY_F5:
			VR_eye_offset -= 1;
			reset_cockpit();
			break;
		case KEY_SHIFTED + KEY_F6:
			VR_eye_offset += 1;
			reset_cockpit();
			break;
#endif
		case KEY_SHIFTED + KEY_ALTED + KEY_F5:
			VR_eye_width -= (F1_0/10); //*= 10/11;
			reset_cockpit();
			break;
		case KEY_SHIFTED + KEY_ALTED + KEY_F6:
			VR_eye_width += (F1_0/10); //*= 11/10;
			reset_cockpit();
			break;
		case KEY_SHIFTED + KEY_F7:
		case KEY_SHIFTED + KEY_ALTED + KEY_F7:
			VR_eye_width = F1_0;
			VR_eye_offset = 0;
			reset_cockpit();
			break;
		case KEY_SHIFTED + KEY_F8:
		case KEY_SHIFTED + KEY_ALTED + KEY_F8:
			VR_stereo = static_cast<StereoFormat>((static_cast<unsigned>(VR_stereo) + 1) % (static_cast<unsigned>(StereoFormat::HighestFormat) + 1));
			init_stereo();
			reset_cockpit();
			break;
#endif

			/*
			 * Jukebox hotkeys -- MD2211, 2007
			 * Now for all music
			 * ==============================================
			 */
		case KEY_ALTED + KEY_SHIFTED + KEY_F9:
		KEY_MAC(case KEY_COMMAND+KEY_E:)
#if DXX_USE_SDL_REDBOOK_AUDIO
			if (CGameCfg.MusicType == music_type::Redbook)
			{
				songs_stop_all();
				RBAEjectDisk();
			}
#endif
			break;

		case KEY_ALTED + KEY_SHIFTED + KEY_F10:
		KEY_MAC(case KEY_COMMAND+KEY_UP:)
		KEY_MAC(case KEY_COMMAND+KEY_DOWN:)
			songs_pause_resume();
			break;

		case KEY_MINUS + KEY_ALTED:
		case KEY_ALTED + KEY_SHIFTED + KEY_F11:
		KEY_MAC(case KEY_COMMAND+KEY_LEFT:)
			songs_play_level_song( Current_level_num, -1 );
			break;
		case KEY_EQUAL + KEY_ALTED:
		case KEY_ALTED + KEY_SHIFTED + KEY_F12:
		KEY_MAC(case KEY_COMMAND+KEY_RIGHT:)
			songs_play_level_song( Current_level_num, 1 );
			break;

		default:
			return window_event_result::ignored;
	}
	return window_event_result::handled;
}

static window_event_result HandleGameKey(int key, control_info &Controls)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	switch (key) {
#if defined(DXX_BUILD_DESCENT_II)
		case KEY_1 + KEY_SHIFTED:
		case KEY_2 + KEY_SHIFTED:
		case KEY_3 + KEY_SHIFTED:
		case KEY_4 + KEY_SHIFTED:
		case KEY_5 + KEY_SHIFTED:
		case KEY_6 + KEY_SHIFTED:
		case KEY_7 + KEY_SHIFTED:
		case KEY_8 + KEY_SHIFTED:
		case KEY_9 + KEY_SHIFTED:
		case KEY_0 + KEY_SHIFTED:
			if (PlayerCfg.EscortHotKeys)
			{
				auto &BuddyState = LevelUniqueObjectState.BuddyState;
				if (Game_mode & GM_MULTI)
				{
					if (!check_warn_local_player_can_control_guidebot(Objects.vcptr, BuddyState, Netgame))
						return window_event_result::handled;
				}
				set_escort_special_goal(BuddyState, key);
				game_flush_inputs(Controls);
				return window_event_result::handled;
			}
			else
				return window_event_result::ignored;
#endif

		case KEY_ALTED+KEY_F7:
		KEY_MAC(case KEY_COMMAND+KEY_ALTED+KEY_7:)
			PlayerCfg.HudMode = static_cast<HudType>((static_cast<unsigned>(PlayerCfg.HudMode) + 1) % GAUGE_HUD_NUMMODES);
			write_player_file();
			switch (PlayerCfg.HudMode)
			{
				case HudType::Standard:
					HUD_init_message_literal(HM_DEFAULT, "Standard HUD");
					break;
				case HudType::Alternate1:
					HUD_init_message_literal(HM_DEFAULT, "Alternative HUD #1");
					break;
				case HudType::Alternate2:
					HUD_init_message_literal(HM_DEFAULT, "Alternative HUD #2");
					break;
				case HudType::Hidden:
					HUD_init_message_literal(HM_DEFAULT, "No HUD");
					break;
			}
			return window_event_result::handled;

		KEY_MAC(case KEY_COMMAND+KEY_6:)
		case KEY_F6:
			if (Netgame.RefusePlayers && WaitForRefuseAnswer && !(Game_mode & GM_TEAM))
			{
				RefuseThisPlayer=1;
				HUD_init_message_literal(HM_MULTI, "Player accepted!");
			}
			return window_event_result::handled;
		case KEY_ALTED + KEY_1:
			if (Netgame.RefusePlayers && WaitForRefuseAnswer && (Game_mode & GM_TEAM))
				{
					RefuseThisPlayer=1;
					HUD_init_message_literal(HM_MULTI, "Player accepted!");
					RefuseTeam=1;
					game_flush_inputs(Controls);
				}
			return window_event_result::handled;
		case KEY_ALTED + KEY_2:
			if (Netgame.RefusePlayers && WaitForRefuseAnswer && (Game_mode & GM_TEAM))
				{
					RefuseThisPlayer=1;
					HUD_init_message_literal(HM_MULTI, "Player accepted!");
					RefuseTeam=2;
					game_flush_inputs(Controls);
				}
			return window_event_result::handled;

		default:
			break;

	}	 //switch (key)

	if (Player_dead_state == player_dead_state::no)
		switch (key)
		{
				KEY_MAC(case KEY_COMMAND+KEY_SHIFTED+KEY_5:)
			case KEY_F5 + KEY_SHIFTED:
				DropCurrentWeapon(get_local_plrobj().ctype.player_info);
				break;

			KEY_MAC(case KEY_COMMAND+KEY_SHIFTED+KEY_6:)
			case KEY_F6 + KEY_SHIFTED:
				DropSecondaryWeapon(get_local_plrobj().ctype.player_info);
				break;

#if defined(DXX_BUILD_DESCENT_II)
			case KEY_0 + KEY_ALTED:
				DropFlag ();
				game_flush_inputs(Controls);
				break;

			KEY_MAC(case KEY_COMMAND+KEY_4:)
			case KEY_F4:
				if (!MarkerState.DefiningMarkerMessage())
					InitMarkerInput();
				break;
#endif

			default:
				return window_event_result::ignored;
		}
	else
		return window_event_result::ignored;

	return window_event_result::handled;
}

#if defined(DXX_BUILD_DESCENT_II)
static void kill_all_robots(void)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	int	dead_count=0;
	//int	boss_index = -1;
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;

	// Kill all bots except for Buddy bot and boss.  However, if only boss and buddy left, kill buddy.
	for (auto &obj : vmobjptr)
	{
		if (obj.type == OBJ_ROBOT)
		{
			auto &ri = Robot_info[get_robot_id(obj)];
			if (!ri.companion && ri.boss_flag == boss_robot_id::None)
			{
				dead_count++;
				obj.flags |= OF_EXPLODING | OF_SHOULD_BE_DEAD;
			}
		}
	}

// --		// Now, if more than boss and buddy left, un-kill boss.
// --		if ((dead_count > 2) && (boss_index != -1)) {
// --			Objects[boss_index].flags &= ~(OF_EXPLODING|OF_SHOULD_BE_DEAD);
// --			dead_count--;
// --		} else if (boss_index != -1)
// --			HUD_init_message(HM_DEFAULT, "Toasted the BOSS!");

	// Toast the buddy if nothing else toasted!
	if (dead_count == 0)
		for (auto &obj : vmobjptr)
		{
			if (obj.type == OBJ_ROBOT)
				if (Robot_info[get_robot_id(obj)].companion) {
					obj.flags |= OF_EXPLODING | OF_SHOULD_BE_DEAD;
					HUD_init_message_literal(HM_DEFAULT, "Toasted the Buddy! *sniff*");
					dead_count++;
				}
		}

	HUD_init_message(HM_DEFAULT, "%i robots toasted!", dead_count);
}
#endif

//	--------------------------------------------------------------------------
//	Detonate reactor.
//	Award player all powerups in mine.
//	Place player just outside exit.
//	Kill all bots in mine.
//	Yippee!!
static void kill_and_so_forth(const d_robot_info_array &Robot_info, fvmobjptridx &vmobjptridx, fvmsegptridx &vmsegptridx)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vmobjptr = Objects.vmptr;
	HUD_init_message_literal(HM_DEFAULT, "Killing, awarding, etc.!");

	range_for (const auto &&o, vmobjptridx)
	{
		switch (o->type) {
			case OBJ_ROBOT:
				apply_damage_to_robot(Robot_info, o, o->shields + 1, get_local_player().objnum);
				break;
			case OBJ_POWERUP:
				do_powerup(o);
				break;
			default:
				break;
		}
	}

	do_controlcen_destroyed_stuff(object_none);

	auto &Triggers = LevelUniqueWallSubsystemState.Triggers;
	auto &Walls = LevelUniqueWallSubsystemState.Walls;
	auto &vcwallptr = Walls.vcptr;
	for (const auto &&t : Triggers.vcptridx)
	{
		if (trigger_is_exit(t))
		{
			for (auto &w : vcwallptr)
			{
				if (w.trigger == t)
				{
					const auto &&segp = vmsegptridx(w.segnum);
					auto &vcvertptr = Vertices.vcptr;
					ConsoleObject->pos = compute_segment_center(vcvertptr, segp);
					obj_relink(vmobjptr, vmsegptr, vmobjptridx(ConsoleObject), segp);
					return;
				}
			}
		}
	}
}

#ifndef RELEASE
#if defined(DXX_BUILD_DESCENT_II)
static void kill_all_snipers(void) __attribute_used;
static void kill_all_snipers(void)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	int     dead_count=0;

	//	Kill all snipers.
	for (auto &obj : vmobjptr)
	{
		if (obj.type == OBJ_ROBOT)
			if (obj.ctype.ai_info.behavior == ai_behavior::AIB_SNIPE)
			{
				dead_count++;
				obj.flags |= OF_EXPLODING | OF_SHOULD_BE_DEAD;
			}
	}

	HUD_init_message(HM_DEFAULT, "%i sniper robots toasted!", dead_count);
}

static void kill_thief(void) __attribute_used;
static void kill_thief(void)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	//	Kill thief.
	for (auto &obj : vmobjptr)
	{
		if (obj.type == OBJ_ROBOT)
			if (Robot_info[get_robot_id(obj)].thief)
			{
				obj.flags |= OF_EXPLODING | OF_SHOULD_BE_DEAD;
				HUD_init_message_literal(HM_DEFAULT, "Thief toasted!");
				break;
			}
	}
}

static void kill_buddy(void) __attribute_used;
static void kill_buddy(void)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	auto &Robot_info = LevelSharedRobotInfoState.Robot_info;
	//	Kill buddy.
	for (auto &obj : vmobjptr)
	{
		if (obj.type == OBJ_ROBOT)
			if (Robot_info[get_robot_id(obj)].companion)
			{
				obj.flags |= OF_EXPLODING | OF_SHOULD_BE_DEAD;
				HUD_init_message_literal(HM_DEFAULT, "Buddy toasted!");
				break;
			}
	}
}
#endif

static window_event_result HandleTestKey(const d_level_shared_robot_info_state &LevelSharedRobotInfoState, fvmsegptridx &vmsegptridx, int key, control_info &Controls)
{
	auto &LevelSharedVertexState = LevelSharedSegmentState.get_vertex_state();
	auto &LevelUniqueMorphObjectState = LevelUniqueObjectState.MorphObjectState;
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &Vertices = LevelSharedVertexState.get_vertices();
	auto &vmobjptr = Objects.vmptr;
	auto &vmobjptridx = Objects.vmptridx;
	switch (key)
	{

#ifdef SHOW_EXIT_PATH
		case KEY_DEBUGGED+KEY_1:	create_special_path();	break;
#endif

		case KEY_DEBUGGED+KEY_Y:
			do_controlcen_destroyed_stuff(object_none);
			break;

#if defined(DXX_BUILD_DESCENT_II)
	case KEY_DEBUGGED+KEY_ALTED+KEY_D:
			PlayerCfg.NetlifeKills=4000; PlayerCfg.NetlifeKilled=5;
			multi_add_lifetime_kills(1);
			break;

		case KEY_DEBUGGED+KEY_R+KEY_SHIFTED:
			kill_all_robots();
			break;
#endif

		case KEY_BACKSP:
		case KEY_CTRLED+KEY_BACKSP:
		case KEY_ALTED+KEY_BACKSP:
		case KEY_SHIFTED+KEY_BACKSP:
		case KEY_SHIFTED+KEY_ALTED+KEY_BACKSP:
		case KEY_CTRLED+KEY_ALTED+KEY_BACKSP:
		case KEY_SHIFTED+KEY_CTRLED+KEY_BACKSP:
		case KEY_SHIFTED+KEY_CTRLED+KEY_ALTED+KEY_BACKSP:

			Int3(); break;

		case KEY_DEBUGGED+KEY_P:
			if (Game_suspended & SUSP_ROBOTS)
				Game_suspended &= ~SUSP_ROBOTS;		//robots move
			else
				Game_suspended |= SUSP_ROBOTS;		//robots don't move
			break;
		case KEY_DEBUGGED+KEY_M:
		{
			static uint8_t i = 0;
			const auto &&segp = vmsegptridx(ConsoleObject->segnum);
			auto &vcvertptr = Vertices.vcptr;
			const auto &&new_obj = create_morph_robot(LevelSharedRobotInfoState.Robot_info, segp, compute_segment_center(vcvertptr, segp), robot_id{i});
			if (new_obj != object_none)
				morph_start(LevelUniqueMorphObjectState, LevelSharedPolygonModelState, new_obj);
			i++;
			if (i >= LevelSharedRobotInfoState.N_robot_types)
				i = 0;
			break;
		}
		case KEY_DEBUGGED+KEY_K:
			get_local_plrobj().shields = 1;
			break;							//	a virtual kill
		case KEY_DEBUGGED+KEY_SHIFTED + KEY_K:
			get_local_plrobj().shields = -1;
			break;  //	an actual kill
		case KEY_DEBUGGED+KEY_X: get_local_player().lives++; break; // Extra life cheat key.
		case KEY_DEBUGGED+KEY_H:
		{
			if (Player_dead_state != player_dead_state::no)
				return window_event_result::ignored;

			auto &player_info = get_local_plrobj().ctype.player_info;
			auto &pl_flags = player_info.powerup_flags;
			pl_flags ^= PLAYER_FLAGS_CLOAKED;
			if (pl_flags & PLAYER_FLAGS_CLOAKED) {
				if (Game_mode & GM_MULTI)
					multi_send_cloak();
				ai_do_cloak_stuff();
				player_info.cloak_time = {GameTime64};
			}
			break;
		}

		case KEY_DEBUGGED+KEY_R:
			cheats.robotfiringsuspended = !cheats.robotfiringsuspended;
			break;

#if DXX_USE_EDITOR		//editor-specific functions

		case KEY_E + KEY_DEBUGGED:
		{
			const auto g = Game_wind;
			g->set_visible(0);	// don't let the game do anything while we set the editor up
			auto old_gamestate = gamestate;
			gamestate = editor_gamestate::unsaved;	// saved game editing mode

			init_editor();
			// If editor failed to load, carry on playing
			if (!EditorWindow)
			{
				g->set_visible(1);
				gamestate = old_gamestate;
				return window_event_result::handled;
			}
			return window_event_result::close;
		}

#if defined(DXX_BUILD_DESCENT_II)
	case KEY_Q + KEY_SHIFTED + KEY_DEBUGGED:
		{
			palette_array_t save_pal;
			save_pal = gr_palette;
			PlayMovie ("end.tex", "end.mve", play_movie_warn_missing::urgent);
			Screen_mode = -1;
			set_screen_mode(SCREEN_GAME);
			reset_cockpit();
			gr_palette = save_pal;
			gr_palette_load(gr_palette);
			break;
		}
#endif
		case KEY_C + KEY_CTRLED + KEY_DEBUGGED:
			window_create<choose_curseg_menu>(LevelSharedSegmentState, *grd_curcanv);
			break;
		case KEY_C + KEY_SHIFTED + KEY_DEBUGGED:
			if (Player_dead_state == player_dead_state::no &&
				!(Game_mode & GM_MULTI))
				move_player_2_segment(Cursegp,Curside);
			break;   //move eye to curseg


		case KEY_DEBUGGED+KEY_W:	draw_world_from_game(); break;

		#endif  //#ifdef EDITOR

		case KEY_DEBUGGED + KEY_LAPOSTRO:
			Show_view_text_timer = 0x30000;
			object_goto_next_viewer(LevelUniqueObjectState.Objects, Viewer);
			break;
		case KEY_DEBUGGED+KEY_SHIFTED+KEY_LAPOSTRO: Viewer=ConsoleObject; break;
		case KEY_DEBUGGED+KEY_O: toggle_outline_mode(); break;
		case KEY_DEBUGGED+KEY_T:
#if defined(DXX_BUILD_DESCENT_II)
		{
			static int Toggle_var;
			Toggle_var = !Toggle_var;
			if (Toggle_var)
				CGameArg.SysMaxFPS = 300;
			else
				CGameArg.SysMaxFPS = 30;
		}
#endif
			break;
		case KEY_DEBUGGED + KEY_L:
#if !DXX_USE_OGL
			if (++Lighting_on >= 2)
                                Lighting_on = 0;
#endif
                        break;
		case KEY_PAD5: slew_stop(); break;

#ifndef NDEBUG
		case KEY_DEBUGGED + KEY_F11: play_test_sound(); break;
#endif

		case KEY_DEBUGGED + KEY_C:
		{
			auto &plrobj = get_local_plrobj();
			do_cheat_menu(plrobj, grd_curscreen->sc_canvas);
			break;
		}
		case KEY_DEBUGGED + KEY_SHIFTED + KEY_A:
		{
			auto &plrobj = get_local_plrobj();
			do_megawow_powerup(plrobj, 10);
			break;
		}
		case KEY_DEBUGGED + KEY_A:	{
			auto &plrobj = get_local_plrobj();
			do_megawow_powerup(plrobj, 200);
				break;
		}

		case KEY_DEBUGGED+KEY_SPACEBAR:		//KEY_F7:				// Toggle physics flying
			slew_stop();
			game_flush_inputs(Controls);
			if (ConsoleObject->control_source != object::control_type::flying)
			{
				fly_init(*ConsoleObject);
				Game_suspended &= ~SUSP_ROBOTS;	//robots move
			} else {
				slew_init(vmobjptr(ConsoleObject));			//start player slewing
				Game_suspended |= SUSP_ROBOTS;	//robots don't move
			}
			break;

		case KEY_DEBUGGED+KEY_COMMA: Render_zoom = fixmul(Render_zoom,62259); break;
		case KEY_DEBUGGED+KEY_PERIOD: Render_zoom = fixmul(Render_zoom,68985); break;

		#ifndef NDEBUG
		case KEY_DEBUGGED+KEY_D:
			if ((CGameArg.DbgNoDoubleBuffer = !CGameArg.DbgNoDoubleBuffer)!=0)
				init_cockpit();
			break;
		#endif

#if DXX_USE_EDITOR
		case KEY_DEBUGGED+KEY_Q:
			{
				pause_game_world_time p;
			dump_used_textures_all();
			}
			break;
#endif

		case KEY_DEBUGGED+KEY_B: {
			d_fname text{};
			int item;
			std::array<newmenu_item, 1> m{{
				newmenu_item::nm_item_input(text),
			}};
			struct briefing_menu : passive_newmenu
			{
				briefing_menu(grs_canvas &canvas, const std::ranges::subrange<newmenu_item *> items) :
					passive_newmenu(menu_title{nullptr}, menu_subtitle{"Briefing to play?"}, menu_filename{nullptr}, tiny_mode_flag::normal, tab_processing_flag::ignore, adjusted_citem::create(items, 0), canvas)
				{
				}
			};
			item = run_blocking_newmenu<briefing_menu>(*grd_curcanv, m);
			if (item != -1) {
				do_briefing_screens(text,1);
			}
			break;
		}

		case KEY_DEBUGGED+KEY_SHIFTED+KEY_B:
			if (Player_dead_state != player_dead_state::no)
				return window_event_result::ignored;

			kill_and_so_forth(LevelSharedRobotInfoState.Robot_info, vmobjptridx, vmsegptridx);
			break;
		case KEY_DEBUGGED+KEY_G:
			GameTime64 = (INT64_MAX) - (F1_0*10);
			HUD_init_message(HM_DEFAULT, "GameTime %" PRIi64 " - Reset in 10 seconds!", GameTime64);
			break;
		default:
			return window_event_result::ignored;
	}
	return window_event_result::handled;
}
#endif		//#ifndef RELEASE

#define CHEAT_MAX_LEN 15
struct cheat_code
{
	const char string[CHEAT_MAX_LEN];
	int8_t game_cheats::*stateptr;
};

constexpr cheat_code cheat_codes[] = {
#if defined(DXX_BUILD_DESCENT_I)
	{ "gabbagabbahey", &game_cheats::enabled },
	{ "scourge", &game_cheats::wowie },
	{ "bigred", &game_cheats::wowie2 },
	{ "mitzi", &game_cheats::allkeys },
	{ "racerx", &game_cheats::invul },
	{ "guile", &game_cheats::cloak },
	{ "twilight", &game_cheats::shields },
	{ "poboys", &game_cheats::killreactor },
	{ "farmerjoe", &game_cheats::levelwarp },
	{ "bruin", &game_cheats::extralife },
	{ "porgys", &game_cheats::rapidfire },
	{ "ahimsa", &game_cheats::robotfiringsuspended },
	{ "baldguy", &game_cheats::baldguy },
#elif defined(DXX_BUILD_DESCENT_II)
	{ "gabbagabbahey", &game_cheats::lamer },
	{ "motherlode", &game_cheats::lamer },
	{ "currygoat", &game_cheats::lamer },
	{ "zingermans", &game_cheats::lamer },
	{ "eatangelos", &game_cheats::lamer },
	{ "ericaanne", &game_cheats::lamer },
	{ "joshuaakira", &game_cheats::lamer },
	{ "whammazoom", &game_cheats::lamer },
	{ "honestbob", &game_cheats::wowie },
	{ "oralgroove", &game_cheats::allkeys },
	{ "alifalafel", &game_cheats::accessory },
	{ "almighty", &game_cheats::invul },
	{ "blueorb", &game_cheats::shields },
	{ "delshiftb", &game_cheats::killreactor },
	{ "freespace", &game_cheats::levelwarp },
	{ "rockrgrl", &game_cheats::fullautomap },
	{ "wildfire", &game_cheats::rapidfire },
	{ "duddaboo", &game_cheats::bouncyfire },
	{ "lpnlizard", &game_cheats::homingfire },
	{ "imagespace", &game_cheats::robotfiringsuspended },
	{ "spaniard", &game_cheats::killallrobots },
	{ "silkwing", &game_cheats::robotskillrobots },
	{ "godzilla", &game_cheats::monsterdamage },
	{ "helpvishnu", &game_cheats::buddyclone },
	{ "gowingnut", &game_cheats::buddyangry },
#endif
	{ "flash", &game_cheats::exitpath },
	{ "astral", &game_cheats::ghostphysics },
	{ "buggin", &game_cheats::turbo },
	{ "bittersweet", &game_cheats::acid },
};

struct levelwarp_menu_items
{
	std::array<char, 8> text{};
	std::array<newmenu_item, 1> menu_items{{
		newmenu_item::nm_item_input(text),
	}};
};

struct levelwarp_menu : levelwarp_menu_items, passive_newmenu
{
	levelwarp_menu(grs_canvas &src) :
		passive_newmenu(menu_title{nullptr}, menu_subtitle{TXT_WARP_TO_LEVEL}, menu_filename{nullptr}, tiny_mode_flag::normal, tab_processing_flag::ignore, adjusted_citem::create(menu_items, 0), src)
		{
		}
	virtual window_event_result event_handler(const d_event &event) override;
	void handle_close_event();
};

void levelwarp_menu::handle_close_event()
{
	if (!text[0])
		return;
	char *p;
	const auto l = strtoul(text.data(), &p, 0);
	if (*p)
		return;
	/* No handling for secret levels.  Warping to secret
	 * levels is not supported.
	 */
	if (l > Current_mission->last_level)
		return;
	const auto g = Game_wind;
	g->set_visible(0);
	StartNewLevel(l);
	g->set_visible(1);
}

window_event_result levelwarp_menu::event_handler(const d_event &event)
{
	switch (event.type)
	{
		case event_type::window_close:
			handle_close_event();
			return window_event_result::ignored;
		default:
			return newmenu::event_handler(event);
	}
}

static window_event_result FinalCheats(const d_level_shared_robot_info_state &LevelSharedRobotInfoState)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vmobjptr = Objects.vmptr;
	auto &vmobjptridx = Objects.vmptridx;

	if (Game_mode & GM_MULTI)
		return window_event_result::ignored;

	static std::array<char, CHEAT_MAX_LEN> cheat_buffer;
	std::move(std::next(cheat_buffer.begin()), cheat_buffer.end(), cheat_buffer.begin());
	cheat_buffer.back() = key_ascii();
	int8_t game_cheats::*gotcha;
	for (unsigned i = 0;; i++)
	{
		if (i >= std::size(cheat_codes))
			return window_event_result::ignored;
		int cheatlen = strlen(cheat_codes[i].string);
		Assert(cheatlen <= CHEAT_MAX_LEN);
		if (d_strnicmp(cheat_codes[i].string, &cheat_buffer[CHEAT_MAX_LEN-cheatlen], cheatlen)==0)
		{
			gotcha = cheat_codes[i].stateptr;
#if defined(DXX_BUILD_DESCENT_I)
			if (!cheats.enabled && gotcha != &game_cheats::enabled)
				return window_event_result::ignored;
			if (!cheats.enabled)
			{
				const auto &&m = TXT_CHEATS_ENABLED;
				HUD_init_message_literal(HM_DEFAULT, {m, strlen(m)});
			}
#endif
			cheats.*gotcha = !(cheats.*gotcha);
			cheats.enabled = 1;
			digi_play_sample( SOUND_CHEATER, F1_0);
			break;
		}
	}
	auto &plrobj = get_local_plrobj();
	auto &player_info = plrobj.ctype.player_info;
	player_info.mission.score = 0;

#if defined(DXX_BUILD_DESCENT_I)
	if (gotcha == &game_cheats::wowie)
	{
		{
			const auto &&m = TXT_WOWIE_ZOWIE;
			HUD_init_message_literal(HM_DEFAULT, {m, strlen(m)});
		}

		player_info.primary_weapon_flags |= (HAS_LASER_FLAG | HAS_VULCAN_FLAG | HAS_SPREADFIRE_FLAG);

		player_info.vulcan_ammo = VULCAN_AMMO_MAX;
		auto &secondary_ammo = player_info.secondary_ammo;
		for (const auto i : {secondary_weapon_index_t::CONCUSSION_INDEX, secondary_weapon_index_t::HOMING_INDEX, secondary_weapon_index_t::PROXIMITY_INDEX})
			secondary_ammo[i] = Secondary_ammo_max[i];

		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_laser_level(player_info.laser_level, MAX_LASER_LEVEL);

		player_info.energy = MAX_ENERGY;
		player_info.laser_level = MAX_LASER_LEVEL;
		player_info.powerup_flags |= PLAYER_FLAGS_QUAD_LASERS;
		update_laser_weapon_info();
	}

	if (gotcha == &game_cheats::wowie2)
	{
		HUD_init_message(HM_DEFAULT, "SUPER %s",TXT_WOWIE_ZOWIE);

		player_info.primary_weapon_flags = (HAS_LASER_FLAG | HAS_VULCAN_FLAG | HAS_SPREADFIRE_FLAG | HAS_PLASMA_FLAG | HAS_FUSION_FLAG);

		player_info.vulcan_ammo = VULCAN_AMMO_MAX;
		auto &secondary_ammo = player_info.secondary_ammo;
		secondary_ammo = Secondary_ammo_max;

		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_laser_level(player_info.laser_level, MAX_LASER_LEVEL);

		player_info.energy = MAX_ENERGY;
		player_info.laser_level = MAX_LASER_LEVEL;
		player_info.powerup_flags |= PLAYER_FLAGS_QUAD_LASERS;
		update_laser_weapon_info();
	}
#elif defined(DXX_BUILD_DESCENT_II)
	if (gotcha == &game_cheats::lamer)
	{
		plrobj.shields = player_info.energy = i2f(1);
		HUD_init_message_literal(HM_DEFAULT, "Take that...cheater!");
	}

	if (gotcha == &game_cheats::wowie)
	{
		{
			const auto &&m = TXT_WOWIE_ZOWIE;
			HUD_init_message_literal(HM_DEFAULT, {m, strlen(m)});
		}

		if (Piggy_hamfile_version < pig_hamfile_version::_3) // SHAREWARE
		{
			player_info.primary_weapon_flags |= (HAS_LASER_FLAG | HAS_VULCAN_FLAG | HAS_SPREADFIRE_FLAG | HAS_PLASMA_FLAG) | (HAS_GAUSS_FLAG | HAS_HELIX_FLAG);
		}
		else
		{
			player_info.primary_weapon_flags = (HAS_LASER_FLAG | HAS_VULCAN_FLAG | HAS_SPREADFIRE_FLAG | HAS_PLASMA_FLAG | HAS_FUSION_FLAG) | (HAS_GAUSS_FLAG | HAS_HELIX_FLAG | HAS_PHOENIX_FLAG | HAS_OMEGA_FLAG);
		}

		player_info.vulcan_ammo = VULCAN_AMMO_MAX;
		auto &secondary_ammo = player_info.secondary_ammo;
		secondary_ammo = Secondary_ammo_max;

		if (Piggy_hamfile_version < pig_hamfile_version::_3) // SHAREWARE
		{
			secondary_ammo[secondary_weapon_index_t::SMISSILE4_INDEX] = 0;
			secondary_ammo[secondary_weapon_index_t::SMISSILE5_INDEX] = 0;
			secondary_ammo[secondary_weapon_index_t::MEGA_INDEX] = 0;
		}

		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_laser_level(player_info.laser_level, MAX_SUPER_LASER_LEVEL);

		player_info.energy = MAX_ENERGY;
		player_info.laser_level = MAX_SUPER_LASER_LEVEL;
		player_info.powerup_flags |= PLAYER_FLAGS_QUAD_LASERS;
		update_laser_weapon_info();
	}

	if (gotcha == &game_cheats::accessory)
	{
		player_info.powerup_flags |= PLAYER_FLAGS_HEADLIGHT | PLAYER_FLAGS_AFTERBURNER | PLAYER_FLAGS_AMMO_RACK | PLAYER_FLAGS_CONVERTER;
		HUD_init_message_literal(HM_DEFAULT, "Accessories!!");
	}
#endif

	if (gotcha == &game_cheats::allkeys)
	{
		{
			const auto &&m = TXT_ALL_KEYS;
			HUD_init_message_literal(HM_DEFAULT, {m, strlen(m)});
		}
		player_info.powerup_flags |= PLAYER_FLAGS_BLUE_KEY | PLAYER_FLAGS_RED_KEY | PLAYER_FLAGS_GOLD_KEY;
	}

	if (gotcha == &game_cheats::invul)
	{
		player_info.invulnerable_time = GameTime64+i2f(1000);
		auto &pl_flags = player_info.powerup_flags;
		pl_flags ^= PLAYER_FLAGS_INVULNERABLE;
		HUD_init_message(HM_DEFAULT, "%s %s!", TXT_INVULNERABILITY, (pl_flags & PLAYER_FLAGS_INVULNERABLE) ? (player_info.FakingInvul = 0, TXT_ON) : TXT_OFF);
	}

	if (gotcha == &game_cheats::shields)
	{
		{
			const auto &&m = TXT_FULL_SHIELDS;
			HUD_init_message_literal(HM_DEFAULT, {m, strlen(m)});
		}
		plrobj.shields = MAX_SHIELDS;
	}

#if defined(DXX_BUILD_DESCENT_I)
	if (gotcha == &game_cheats::cloak)
	{
		auto &pl_flags = player_info.powerup_flags;
		pl_flags ^= PLAYER_FLAGS_CLOAKED;
		const auto have_cloaked = pl_flags & PLAYER_FLAGS_CLOAKED;
		HUD_init_message(HM_DEFAULT, "%s %s!", TXT_CLOAK, have_cloaked ? TXT_ON : TXT_OFF);
		if (have_cloaked)
		{
			ai_do_cloak_stuff();
			player_info.cloak_time = {GameTime64};
		}
	}

	if (gotcha == &game_cheats::extralife)
	{
		auto &plr = get_local_player();
		if (plr.lives < 50)
		{
			plr.lives++;
			HUD_init_message_literal(HM_DEFAULT, "Extra life!");
		}
	}
#endif

	if (gotcha == &game_cheats::killreactor)
		kill_and_so_forth(LevelSharedRobotInfoState.Robot_info, vmobjptridx, vmsegptridx);

	if (gotcha == &game_cheats::exitpath)
	{
		if (create_special_path())
			HUD_init_message_literal(HM_DEFAULT, "Exit path illuminated!");
	}

	if (gotcha == &game_cheats::levelwarp)
	{
		auto menu = window_create<levelwarp_menu>(grd_curscreen->sc_canvas);
		(void)menu;
	}

#if defined(DXX_BUILD_DESCENT_II)
	if (gotcha == &game_cheats::fullautomap)
		HUD_init_message_literal(HM_DEFAULT, cheats.fullautomap ? std::span<const char>("FULL MAP!") : std::span<const char>("REGULAR MAP"));
#endif

	if (gotcha == &game_cheats::ghostphysics)
	{
		HUD_init_message(HM_DEFAULT, "%s %s!", "Ghosty mode", cheats.ghostphysics?TXT_ON:TXT_OFF);
	}

	if (gotcha == &game_cheats::rapidfire)
	{
		HUD_init_message(HM_DEFAULT, "Rapid fire %s!", cheats.rapidfire?TXT_ON:TXT_OFF);
#if defined(DXX_BUILD_DESCENT_I)
                if (cheats.rapidfire)
					do_megawow_powerup(plrobj, 200);
#endif
	}

#if defined(DXX_BUILD_DESCENT_II)
	if (gotcha == &game_cheats::bouncyfire)
	{
		
		HUD_init_message(HM_DEFAULT, "Bouncing weapons %s!", cheats.bouncyfire?TXT_ON:TXT_OFF);
	}

	if (gotcha == &game_cheats::homingfire)
	{
		HUD_init_message(HM_DEFAULT, "Homing weapons %s!", cheats.homingfire ? (weapons_homing_all(), TXT_ON) : (weapons_homing_all_reset(), TXT_OFF));
	}
#endif

	if (gotcha == &game_cheats::turbo)
	{
		HUD_init_message(HM_DEFAULT, "%s %s!", "Turbo mode", cheats.turbo?TXT_ON:TXT_OFF);
	}

	if (gotcha == &game_cheats::robotfiringsuspended)
	{
		HUD_init_message(HM_DEFAULT, "Robot firing %s!", cheats.robotfiringsuspended?TXT_OFF:TXT_ON);
	}

#if defined(DXX_BUILD_DESCENT_II)
	if (gotcha == &game_cheats::killallrobots)
	{
		kill_all_robots();
	}

	if (gotcha == &game_cheats::robotskillrobots)
	{
		HUD_init_message_literal(HM_DEFAULT, cheats.robotskillrobots ? std::span<const char>("Rabid robots!") : std::span<const char>("Kill the player!"));
	}

	if (gotcha == &game_cheats::monsterdamage)
	{
		HUD_init_message_literal(HM_DEFAULT, cheats.monsterdamage ? std::span<const char>("Oh no, there goes Tokyo!") : std::span<const char>("What have you done, I'm shrinking!!"));
	}

	if (gotcha == &game_cheats::buddyclone)
	{
		HUD_init_message_literal(HM_DEFAULT, "What's this? Another buddy bot!");
		create_buddy_bot(LevelSharedRobotInfoState);
	}

	if (gotcha == &game_cheats::buddyangry)
	{
		
		if (cheats.buddyangry)
		{
			HUD_init_message(HM_DEFAULT, "%s gets angry!", static_cast<const char *>(PlayerCfg.GuidebotName));
			PlayerCfg.GuidebotName = "Wingnut";
		}
		else
		{
			PlayerCfg.GuidebotName = PlayerCfg.GuidebotNameReal;
			HUD_init_message(HM_DEFAULT, "%s calms down", static_cast<const char *>(PlayerCfg.GuidebotName));
		}
	}
#endif

	if (gotcha == &game_cheats::acid)
	{
		HUD_init_message_literal(HM_DEFAULT, cheats.acid ? std::span<const char>("Going up!") : std::span<const char>("Coming down!"));
	}

	return window_event_result::handled;
}

// Internal Cheat Menu
#ifndef RELEASE

class menu_fix_wrapper
{
	fix &m_value;
public:
	constexpr menu_fix_wrapper(fix &t) :
		m_value(t)
	{
	}
	operator int() const
	{
		return f2i(m_value);
	}
	menu_fix_wrapper &operator=(int n)
	{
		m_value = i2f(n);
		return *this;
	}
};

class cheat_menu_bit_invulnerability :
	std::reference_wrapper<player_info>,
	public menu_bit_wrapper_t<player_flags, std::integral_constant<PLAYER_FLAG, PLAYER_FLAGS_INVULNERABLE>>
{
public:
	cheat_menu_bit_invulnerability(player_info &pl_info) :
		reference_wrapper(pl_info),
		menu_bit_wrapper_t(pl_info.powerup_flags, {})
	{
	}
	cheat_menu_bit_invulnerability &operator=(const uint32_t n)
	{
		this->menu_bit_wrapper_t::operator=(n);
		if (n)
		{
			auto &player_info = get();
			player_info.FakingInvul = 0;
			player_info.invulnerable_time = GameTime64+i2f(1000);
		}
		return *this;
	}
};

class cheat_menu_bit_cloak :
	std::reference_wrapper<player_info>,
	public menu_bit_wrapper_t<player_flags, std::integral_constant<PLAYER_FLAG, PLAYER_FLAGS_CLOAKED>>
{
public:
	cheat_menu_bit_cloak(player_info &pl_info) :
		reference_wrapper(pl_info),
		menu_bit_wrapper_t(pl_info.powerup_flags, {})
	{
	}
	cheat_menu_bit_cloak &operator=(const uint32_t n)
	{
		this->menu_bit_wrapper_t::operator=(n);
		if (n)
		{
			if (Game_mode & GM_MULTI)
				multi_send_cloak();
			ai_do_cloak_stuff();
			get().cloak_time = {GameTime64};
		}
		return *this;
	}
};

#if defined(DXX_BUILD_DESCENT_I)
#define WIMP_MENU_DXX(VERB)
#elif defined(DXX_BUILD_DESCENT_II)
/* Adding an afterburner like this adds it at 0% charge.  This is OK for
 * a cheat.  The player can change his energy up if he needs more.
 */
#define WIMP_MENU_DXX(VERB)	\
	DXX_MENUITEM(VERB, CHECK, TXT_AFTERBURNER, opt_afterburner, menu_bit_wrapper(pl_info.powerup_flags, PLAYER_FLAGS_AFTERBURNER))	\

#endif

#define DXX_WIMP_MENU(VERB)	\
	DXX_MENUITEM(VERB, CHECK, TXT_INVULNERABILITY, opt_invul, cheat_menu_bit_invulnerability(pl_info))	\
	DXX_MENUITEM(VERB, CHECK, TXT_CLOAKED, opt_cloak, cheat_menu_bit_cloak(pl_info))	\
	DXX_MENUITEM(VERB, CHECK, "BLUE KEY", opt_key_blue, menu_bit_wrapper(pl_info.powerup_flags, PLAYER_FLAGS_BLUE_KEY))	\
	DXX_MENUITEM(VERB, CHECK, "GOLD KEY", opt_key_gold, menu_bit_wrapper(pl_info.powerup_flags, PLAYER_FLAGS_GOLD_KEY))	\
	DXX_MENUITEM(VERB, CHECK, "RED KEY", opt_key_red, menu_bit_wrapper(pl_info.powerup_flags, PLAYER_FLAGS_RED_KEY))	\
	WIMP_MENU_DXX(VERB)	\
	DXX_MENUITEM(VERB, NUMBER, TXT_ENERGY, opt_energy, menu_fix_wrapper(pl_info.energy), 0, 200)	\
	DXX_MENUITEM(VERB, NUMBER, "Shields", opt_shields, menu_fix_wrapper(plrobj.shields), 0, 200)	\
	DXX_MENUITEM(VERB, TEXT, TXT_SCORE, opt_txt_score)	\
	DXX_MENUITEM(VERB, INPUT, score_text, opt_score)	\
	DXX_MENUITEM(VERB, NUMBER, "Laser Level", opt_laser_level, menu_number_bias_wrapper<1>(plr_laser_level), static_cast<uint8_t>(laser_level::_1) + 1, static_cast<uint8_t>(DXX_MAXIMUM_LASER_LEVEL) + 1)	\
	DXX_MENUITEM(VERB, NUMBER, "Concussion", opt_concussion, pl_info.secondary_ammo[secondary_weapon_index_t::CONCUSSION_INDEX], 0, 200)	\

struct wimp_menu_items
{
	object &plrobj;
	std::array<char, sizeof("2147483647")> score_text;
	enum {
		DXX_WIMP_MENU(ENUM)
	};
	std::array<newmenu_item, DXX_WIMP_MENU(COUNT)> m;
	wimp_menu_items(object &plr) :
		plrobj(plr)
	{
		auto &pl_info = plrobj.ctype.player_info;
		snprintf(score_text.data(), score_text.size(), "%d", pl_info.mission.score);
		const uint8_t plr_laser_level = static_cast<uint8_t>(pl_info.laser_level);
		DXX_WIMP_MENU(ADD);
	}
};

struct wimp_menu : wimp_menu_items, newmenu
{
	wimp_menu(object &plr, grs_canvas &src) :
		wimp_menu_items(plr),
		newmenu(menu_title{"Wimp Menu"}, menu_subtitle{nullptr}, menu_filename{nullptr}, tiny_mode_flag::normal, tab_processing_flag::ignore, adjusted_citem::create(m, 0), src)
		{
		}
	virtual window_event_result event_handler(const d_event &event) override;
};

window_event_result wimp_menu::event_handler(const d_event &event)
{
	switch (event.type)
	{
		case event_type::window_close:
			{
				auto &pl_info = plrobj.ctype.player_info;
				uint8_t plr_laser_level;
				DXX_WIMP_MENU(READ);
				pl_info.laser_level = laser_level{plr_laser_level};
				char *p;
				auto ul = strtoul(score_text.data(), &p, 10);
				if (!*p)
					pl_info.mission.score = static_cast<int>(ul);
				init_gauges();
				break;
			}
		default:
			break;
	}
	return newmenu::event_handler(event);
}

static void do_cheat_menu(object &plrobj, grs_canvas &cv_canvas)
{
	auto menu = window_create<wimp_menu>(plrobj, cv_canvas);
	(void)menu;
}
#endif



//	Testing functions ----------------------------------------------------------

#ifndef NDEBUG
//	Sounds for testing
__attribute_used
static int Test_sound;

static void play_test_sound()
{
	digi_play_sample(Test_sound, F1_0);
}
#endif  //ifndef NDEBUG

}

window_event_result ReadControls(const d_level_shared_robot_info_state &LevelSharedRobotInfoState, const d_event &event, control_info &Controls)
{
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	auto &Objects = LevelUniqueObjectState.Objects;
	int key;
	static ubyte exploding_flag=0;

	Player_fired_laser_this_frame=object_none;

	if (Player_dead_state == player_dead_state::exploded)
	{
		if (exploding_flag==0)  {
			exploding_flag = 1;			// When player starts exploding, clear all input devices...
			game_flush_inputs(Controls);
		}
	} else {
		exploding_flag=0;
	}
	if (Player_dead_state != player_dead_state::no &&
		!((Game_mode & GM_MULTI) &&
			(multi_sending_message[Player_num] != msgsend_state::none || multi_defining_message != multi_macro_message_index::None)
		)
	)
	HandleDeathInput(event, Controls);

	if (Newdemo_state == ND_STATE_PLAYBACK)
		update_vcr_state();

	if (event.type == event_type::key_command)
	{
		key = event_key_get(event);
#if defined(DXX_BUILD_DESCENT_II)
		if (MarkerState.DefiningMarkerMessage())
		{
			return MarkerInputMessage(key, Controls);
		}
#endif
		if ( (Game_mode & GM_MULTI) && (multi_sending_message[Player_num] != msgsend_state::none || multi_defining_message != multi_macro_message_index::None) )
		{
			return multi_message_input_sub(LevelSharedRobotInfoState.Robot_info, key, Controls);
		}

#ifndef RELEASE
		if ((key&KEY_DEBUGGED)&&(Game_mode&GM_MULTI))   {
			Network_message_reciever = 100;		// Send to everyone...
			snprintf(Network_message.data(), Network_message.size(), "%s %s", TXT_I_AM_A, TXT_CHEATER);
		}
#endif

		if (Endlevel_sequence)
		{
			auto result = HandleEndlevelKey(key);
			if (result != window_event_result::ignored)
				return result;
		}
		else if (Newdemo_state == ND_STATE_PLAYBACK )
		{
			auto r = HandleDemoKey(key);
			if (r != window_event_result::ignored)
				return r;
		}
		else
		{
			window_event_result r = FinalCheats(LevelSharedRobotInfoState);
			if (r == window_event_result::ignored)
				r = HandleSystemKey(key);
			if (r == window_event_result::ignored)
				r = HandleGameKey(key, Controls);
			if (r != window_event_result::ignored)
				return r;
		}

#ifndef RELEASE
		{
			window_event_result r = HandleTestKey(LevelSharedRobotInfoState, vmsegptridx, key, Controls);
			if (r != window_event_result::ignored)
				return r;
		}
#endif

		auto result = call_default_handler(event);
		if (result != window_event_result::ignored)
			return result;
	}

	if (!Endlevel_sequence && Newdemo_state != ND_STATE_PLAYBACK)
	{
		kconfig_read_controls(Controls, event, 0);
		const auto Player_is_dead = Player_dead_state;
		if (Player_is_dead != player_dead_state::no && HandleDeathInput(event, Controls))
			return window_event_result::handled;

		check_rear_view(Controls);

		// If automap key pressed, enable automap unless you are in network mode, control center destroyed and < 10 seconds left
		if ( Controls.state.automap )
		{
			Controls.state.automap = 0;
			if (Player_is_dead != player_dead_state::no || !((Game_mode & GM_MULTI) && LevelUniqueControlCenterState.Control_center_destroyed && LevelUniqueControlCenterState.Countdown_seconds_left < 10))
			{
				do_automap();
				return window_event_result::handled;
			}
		}
		if (Player_is_dead != player_dead_state::no)
			return window_event_result::ignored;
		do_weapon_n_item_stuff(Objects, Controls);
	}

	if (Controls.state.show_menu)
	{
		Controls.state.show_menu = 0;
		return HandleSystemKey(KEY_ESC);
	}

	return window_event_result::ignored;
}

}
