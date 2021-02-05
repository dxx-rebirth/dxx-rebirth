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
 * Kill matrix displayed at end of level.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "dxxerror.h"
#include "pstypes.h"
#include "gr.h"
#include "window.h"
#include "key.h"
#include "palette.h"
#include "game.h"
#include "gameseq.h"
#include "window.h"
#include "physfsx.h"
#include "gamefont.h"
#include "u_mem.h"
#include "newmenu.h"
#include "menu.h"
#include "player.h"
#include "screens.h"
#include "joy.h"
#include "timer.h"
#include "text.h"
#include "net_udp.h"
#include "kmatrix.h"
#include "gauges.h"
#include "pcx.h"
#include "object.h"
#include "args.h"

#include "compiler-range_for.h"
#include "d_levelstate.h"

#define CENTERING_OFFSET(x) ((300 - (70 + (x)*25 ))/2)
#define CENTERSCREEN (SWIDTH/2)
#define KMATRIX_VIEW_SEC 7 // Time after reactor explosion until new level - in seconds

namespace {

enum class kmatrix_status_mode
{
	reactor_countdown_running,
	level_finished,
	mission_finished,
};

static void kmatrix_redraw_coop(fvcobjptr &vcobjptr, font_y_scale_float);

static void kmatrix_draw_item(fvcobjptr &vcobjptr, grs_canvas &canvas, const grs_font &cv_font, const int i, const playernum_array_t &sorted, const font_y_scale_float fspacy)
{
	const auto y = fspacy(80 + i * 9);
	const auto &&fspacx = FSPACX();
	auto &p = *vcplayerptr(sorted[i]);
	gr_string(canvas, cv_font, fspacx(CENTERING_OFFSET(N_players)), y, static_cast<const char *>(p.callsign));

	const auto &&rgb10 = BM_XRGB(10, 10, 10);
	const auto &&rgb25 = BM_XRGB(25, 25, 25);
	for (int j=0; j<N_players; j++)
	{
		const auto x = fspacx(70 + CENTERING_OFFSET(N_players) + j * 25);

		const auto kmij = kill_matrix[sorted[i]][sorted[j]];
		if (sorted[i]==sorted[j])
		{
			if (kmij == 0)
			{
				gr_set_fontcolor(canvas, rgb10, -1);
				gr_string(canvas, cv_font, x, y, "0");
			}
			else
			{
				gr_set_fontcolor(canvas, rgb25, -1);
				gr_printf(canvas, cv_font, x, y, "-%hu", kmij);
			}
		}
		else
		{
			gr_set_fontcolor(canvas, kmij <= 0 ? rgb10 : rgb25, -1);
			gr_printf(canvas, cv_font, x, y, "%hu", kmij);
		}
	}

		auto &player_info = vcobjptr(p.objnum)->ctype.player_info;
		const int eff = (player_info.net_killed_total + player_info.net_kills_total <= 0)
			? 0
			: static_cast<int>(
				static_cast<float>(player_info.net_kills_total) / (
					static_cast<float>(player_info.net_killed_total) + static_cast<float>(player_info.net_kills_total)
				) * 100.0
			);

	const auto x = fspacx(60 + CENTERING_OFFSET(N_players) + N_players * 25);
	gr_set_fontcolor(canvas, rgb25, -1);
	gr_printf(canvas, cv_font, x, y, "%4d/%i%%", player_info.net_kills_total, eff <= 0 ? 0 : eff);
}

static void kmatrix_draw_names(grs_canvas &canvas, const grs_font &cv_font, const playernum_array_t &sorted, const font_y_scale_float fspacy)
{
	const auto &&fspacx = FSPACX();
	const auto &&fspacy_header = fspacy(65);
	const auto &&rgb31 = BM_XRGB(31, 31, 31);
	for (int j=0; j<N_players; j++)
	{
		const auto x = fspacx(70 + CENTERING_OFFSET(N_players) + j * 25);

		color_t c;
		auto &p = *vcplayerptr(sorted[j]);
		if (p.connected==CONNECT_DISCONNECTED)
			c = rgb31;
		else
		{
			const auto color = get_player_or_team_color(sorted[j]);
			const auto &rgb = player_rgb[color];
			c = BM_XRGB(rgb.r, rgb.g, rgb.b);
		}
		gr_set_fontcolor(canvas, c, -1);
		gr_printf(canvas, cv_font, x, fspacy_header, "%c", p.callsign[0u]);
	}
	const auto x = fspacx(72 + CENTERING_OFFSET(N_players) + N_players * 25);
	gr_set_fontcolor(canvas, rgb31, -1);
	gr_string(canvas, cv_font, x, fspacy_header, "K/E");
	if (const auto m = Current_mission.get())
	{
		gr_string(canvas, cv_font, 0x8000, fspacy(30), m->mission_name.data());
		if (const auto level_num = Current_level_num; level_num > 0 && level_num <= m->last_level)
			gr_printf(canvas, cv_font, 0x8000, fspacy(42), "%s [%u/%u]", Current_level_name.line().data(), level_num, m->last_level);
	}
}

static void kmatrix_draw_coop_names(grs_canvas &canvas, const grs_font &cv_font, const font_y_scale_float fspacy)
{
	gr_set_fontcolor(canvas, BM_XRGB(63, 31, 31),-1);
	const auto centerscreen = CENTERSCREEN;
	if (const auto m = Current_mission.get())
	{
		gr_string(canvas, cv_font, 0x8000, fspacy(30), m->mission_name.data());
		if (const auto level_num = Current_level_num; level_num > 0 && level_num <= m->last_level)
			gr_printf(canvas, cv_font, 0x8000, fspacy(42), "%s [%u/%u]", Current_level_name.line().data(), level_num, m->last_level);
	}
	const auto &&fspacy_header = fspacy(58);
	gr_string(canvas, cv_font, centerscreen, fspacy_header, "SCORE");
	gr_string(canvas, cv_font, centerscreen + FSPACX(50), fspacy_header, "DEATHS");
}

static void kmatrix_status_msg(grs_canvas &canvas, const fix time, const kmatrix_status_mode message_mode)
{
	gr_set_fontcolor(canvas, gr_find_closest_color(255, 255, 255),-1);
	auto &game_font = *GAME_FONT;
	gr_printf(canvas, game_font, 0x8000, SHEIGHT - LINE_SPACING(game_font, game_font),
		message_mode == kmatrix_status_mode::reactor_countdown_running
		? "Waiting for players to finish level. Reactor time: T-%d"
		: (
			message_mode == kmatrix_status_mode::level_finished
			? (
				time > 0
				? "Level finished.  Wait %d seconds to proceed or press ESC to Quit."
				: "Level finished.  Focus score screen to proceed."
			)
			: "Mission finished.  Press ESC to Quit."
		)
	, time);
}

}

namespace dcx {

namespace {

struct kmatrix_window : window
{
	kmatrix_window(grs_canvas &src, int x, int y, int w, int h, kmatrix_result &result) :
		window(src, x, y, w, h), result(result)
	{
	}
	grs_main_bitmap background;
	fix64 end_time = -1;
	kmatrix_network network;
	kmatrix_result &result;
};

}

}

namespace dsx {

namespace {

struct kmatrix_window : ::dcx::kmatrix_window
{
	using ::dcx::kmatrix_window::kmatrix_window;
	virtual window_event_result event_handler(const d_event &) override;
};

static void kmatrix_redraw(kmatrix_window *const km)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptr = Objects.vcptr;
	playernum_array_t sorted;

	gr_set_default_canvas();
	auto &canvas = *grd_curcanv;
	show_fullscr(canvas, km->background);
	const auto &&fspacy = FSPACY();
	
	if (Game_mode & GM_MULTI_COOP)
	{
		kmatrix_redraw_coop(vcobjptr, fspacy);
	}
	else
	{
		multi_sort_kill_list();
		const auto title =
#if defined(DXX_BUILD_DESCENT_II)
			game_mode_capture_flag()
			? "CAPTURE THE FLAG SUMMARY"
			: game_mode_hoard()
				? "HOARD SUMMARY"
				:
#endif
				TXT_KILL_MATRIX_TITLE;
		auto &medium3_font = *MEDIUM3_FONT;
		gr_string(canvas, medium3_font, 0x8000, FSPACY(10), title);

		auto &game_font = *GAME_FONT;
		multi_get_kill_list(sorted);
		kmatrix_draw_names(canvas, game_font, sorted, fspacy);

		for (int i=0; i<N_players; i++ )
		{
			if (vcplayerptr(sorted[i])->connected == CONNECT_DISCONNECTED)
				gr_set_fontcolor(canvas, gr_find_closest_color(31, 31, 31),-1);
			else
			{
				const auto color = get_player_or_team_color(sorted[i]);
				gr_set_fontcolor(canvas, BM_XRGB(player_rgb[color].r, player_rgb[color].g, player_rgb[color].b),-1);
			}
			kmatrix_draw_item(vcobjptr, canvas, game_font, i, sorted, fspacy);
		}
	}

	gr_palette_load(gr_palette);
}

}

}

namespace {

static void kmatrix_redraw_coop(fvcobjptr &vcobjptr, const font_y_scale_float fspacy)
{
	playernum_array_t sorted;

	multi_sort_kill_list();
	auto &canvas = *grd_curcanv;
	auto &medium3_font = *MEDIUM3_FONT;
	gr_string(canvas, medium3_font,  0x8000, FSPACY(10), "COOPERATIVE SUMMARY");
	multi_get_kill_list(sorted);
	auto &game_font = *GAME_FONT;
	const auto &&fspacx = FSPACX();
	kmatrix_draw_coop_names(canvas, game_font, fspacy);
	const auto x_callsign = fspacx(CENTERING_OFFSET(N_players));
	const auto x_centerscreen = CENTERSCREEN;
	const auto &&fspacx50 = fspacx(50);
	const auto rgb60_40_10 = BM_XRGB(60, 40, 10);

	for (playernum_t i = 0; i < N_players; ++i)
	{
		auto &plr = *vcplayerptr(sorted[i]);
		int r, g, b;
		if (plr.connected == CONNECT_DISCONNECTED)
			r = g = b = 31;
		else
		{
			auto &color = player_rgb_normal[get_player_color(sorted[i])];
			r = color.r * 2;
			g = color.g * 2;
			b = color.b * 2;
		}
		gr_set_fontcolor(canvas, gr_find_closest_color(r, g, b), -1);

		const auto &&y = fspacy(80 + i * 9);
		gr_string(canvas, game_font, x_callsign, y, static_cast<const char *>(plr.callsign));
		gr_set_fontcolor(canvas, rgb60_40_10, -1);
		auto &player_info = vcobjptr(plr.objnum)->ctype.player_info;
		gr_printf(canvas, game_font, x_centerscreen, y, "%d", player_info.mission.score);
		gr_printf(canvas, game_font, x_centerscreen + fspacx50, y, "%d", player_info.net_killed_total);
	}

	gr_palette_load(gr_palette);
}

}

namespace dsx {

window_event_result kmatrix_window::event_handler(const d_event &event)
{
	auto &LevelUniqueControlCenterState = LevelUniqueObjectState.ControlCenterState;
	int k = 0;
	
	switch (event.type)
	{
		case EVENT_KEY_COMMAND:
			k = event_key_get(event);
			switch( k )
			{
				case KEY_ESC:
					{
						using items_type = std::array<newmenu_item, 2>;
						struct abort_game_menu : items_type, passive_newmenu
						{
							abort_game_menu() :
								items_type{{
									nm_item_menu(TXT_YES),
									nm_item_menu(TXT_NO),
								}},
								passive_newmenu(menu_title{nullptr}, menu_subtitle{TXT_ABORT_GAME}, menu_filename{nullptr}, tiny_mode_flag::normal, tab_processing_flag::ignore, adjusted_citem::create(*static_cast<items_type *>(this), 0), *grd_curcanv)
							{
							}
						};
						if (run_blocking_newmenu<abort_game_menu>() != 0)
							return window_event_result::handled;
					}
					{
						get_local_player().connected=CONNECT_DISCONNECTED;
						
						if (network != kmatrix_network::offline)
							multi::dispatch->send_endlevel_packet();
						
						multi_leave_game();
						this->result = kmatrix_result::abort;

						return window_event_result::close;
					}
				default:
					break;
			}
			break;
			
		case EVENT_WINDOW_DRAW:
			{
			timer_delay2(50);
			kmatrix_redraw(this);

			if (network != kmatrix_network::offline)
				multi::dispatch->do_protocol_frame(0, 1);
			kmatrix_status_mode playing = (Current_level_num == Current_mission->last_level)
				? kmatrix_status_mode::mission_finished
				: kmatrix_status_mode::level_finished;

			// Check if all connected players are also looking at this screen ...
			range_for (auto &i, Players)
				if (i.connected)
					if (i.connected != CONNECT_END_MENU && i.connected != CONNECT_DIED_IN_MINE)
					{
						playing = kmatrix_status_mode::reactor_countdown_running;
						break;
					}
			
			// ... and let the reactor blow sky high!
			if (playing != kmatrix_status_mode::reactor_countdown_running)
			{
				LevelUniqueControlCenterState.Countdown_seconds_left = -1;
			
			// If Reactor is finished and end_time not inited, set the time when we will exit this loop
				if (end_time == -1)
					end_time = timer_query() + (KMATRIX_VIEW_SEC * F1_0);
			}

			// Check if end_time has been reached and exit loop
			if (end_time != -1 && timer_query() >= end_time && this == window_get_front())
			{
				if (network != kmatrix_network::offline)
					multi::dispatch->send_endlevel_packet();  // make sure

#if defined(DXX_BUILD_DESCENT_II)
				if (is_D2_OEM)
				{
					if (Current_level_num==8)
					{
						get_local_player().connected=CONNECT_DISCONNECTED;
						multi_leave_game();
						this->result = kmatrix_result::abort;
					}
				}
#endif
				if (playing != kmatrix_status_mode::mission_finished)
					return window_event_result::close;
			}

			kmatrix_status_msg(*grd_curcanv, playing == kmatrix_status_mode::reactor_countdown_running ? LevelUniqueControlCenterState.Countdown_seconds_left : f2i(end_time - timer_query()), playing);
			break;
			}
			
		case EVENT_WINDOW_CLOSE:
			game_flush_inputs(Controls);
			newmenu_free_background();
			break;
			
		default:
			break;
	}
	return window_event_result::ignored;
}

kmatrix_result kmatrix_view(const kmatrix_network network, control_info &Controls)
{
	auto &Objects = LevelUniqueObjectState.Objects;
	auto &vcobjptridx = Objects.vcptridx;
	kmatrix_result result = kmatrix_result::proceed;
	{
	const auto pkm = window_create<kmatrix_window>(grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT, result);
	auto &km = *pkm;
	pcx_read_bitmap_or_default(STARS_BACKGROUND, km.background, gr_palette);
	gr_palette_load(gr_palette);

	km.network = network;
	
	set_screen_mode( SCREEN_MENU );
	game_flush_inputs(Controls);

	range_for (auto &i, Players)
		if (i.objnum != object_none)
			digi_kill_sound_linked_to_object(vcobjptridx(i.objnum));

	event_process_all();
	}
	/* After event_process_all returns, kmatrix_window has been freed
	 * and cannot be accessed.  The result is therefore stored in a
	 * stack local, which will persist.
	 */
	return result;
}

}
