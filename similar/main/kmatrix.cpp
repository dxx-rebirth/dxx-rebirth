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
#include "cntrlcen.h"
#include "mouse.h"
#include "joy.h"
#include "timer.h"
#include "text.h"
#include "rbaudio.h"
#include "multi.h"
#include "kmatrix.h"
#include "gauges.h"
#include "pcx.h"
#include "object.h"
#include "args.h"

#include "compiler-range_for.h"

#if DXX_USE_OGL
#include "ogl_init.h"
#endif

#define CENTERING_OFFSET(x) ((300 - (70 + (x)*25 ))/2)
#define CENTERSCREEN (SWIDTH/2)
#define KMATRIX_VIEW_SEC 7 // Time after reactor explosion until new level - in seconds
static void kmatrix_redraw_coop();

static void kmatrix_draw_item(int  i, playernum_array_t &sorted)
{
	int x, y;
	char temp[10];

	y = FSPACY(50+i*9);
	const auto &&fspacx = FSPACX();
	gr_string(fspacx(CENTERING_OFFSET(N_players)), y, static_cast<const char *>(Players[sorted[i]].callsign));

	const auto &&rgb10 = BM_XRGB(10, 10, 10);
	const auto &&rgb25 = BM_XRGB(25, 25, 25);
	for (int j=0; j<N_players; j++)
	{
		x = fspacx(70 + CENTERING_OFFSET(N_players) + j * 25);

		const auto kmij = kill_matrix[sorted[i]][sorted[j]];
		if (sorted[i]==sorted[j])
		{
			if (kmij == 0)
			{
				gr_set_fontcolor(rgb10, -1);
				gr_string(x, y, "0");
			}
			else
			{
				gr_set_fontcolor(rgb25, -1);
				gr_printf(x, y, "-%hu", kmij);
			}
		}
		else
		{
			gr_set_fontcolor(kmij <= 0 ? rgb10 : rgb25, -1);
			gr_printf(x, y, "%hu", kmij);
		}
	}

	{
		auto &p = Players[sorted[i]];
		const int eff = (p.net_killed_total + p.net_kills_total <= 0)
			? 0
			: static_cast<int>(
				static_cast<float>(p.net_kills_total) / (
					static_cast<float>(p.net_killed_total) + static_cast<float>(p.net_kills_total)
				) * 100.0
			);
		snprintf(temp, sizeof(temp), "%i%%", eff <= 0 ? 0 : eff);
	}

	x = fspacx(60 + CENTERING_OFFSET(N_players) + N_players * 25);
	gr_set_fontcolor(rgb25, -1);
	gr_printf( x ,y,"%4d/%s",Players[sorted[i]].net_kills_total,temp);
}

static void kmatrix_draw_names(playernum_array_t &sorted)
{
	int x;

	const auto &&fspacx = FSPACX();
	const auto &&fspacy = FSPACY();
	const auto &&rgb31 = BM_XRGB(31, 31, 31);
	for (int j=0; j<N_players; j++)
	{
		x = fspacx(70 + CENTERING_OFFSET(N_players) + j * 25);

		color_t c;
		if (Players[sorted[j]].connected==CONNECT_DISCONNECTED)
			c = rgb31;
		else
		{
			const auto color = get_player_or_team_color(sorted[j]);
			const auto &rgb = player_rgb[color];
			c = BM_XRGB(rgb.r, rgb.g, rgb.b);
		}
		gr_set_fontcolor(c, -1);
		gr_printf(x, fspacy(40), "%c", Players[sorted[j]].callsign[0u]);
	}

	x = fspacx(72 + CENTERING_OFFSET(N_players) + N_players * 25);
	gr_set_fontcolor(rgb31, -1);
	gr_string(x, fspacy(40), "K/E");
}

static void kmatrix_draw_coop_names(playernum_array_t &)
{
	gr_set_fontcolor( BM_XRGB(63,31,31),-1 );
	const auto &&fspacy40 = FSPACY(40);
	const auto centerscreen = CENTERSCREEN;
	gr_string(centerscreen, fspacy40, "SCORE");
	gr_string(centerscreen + FSPACX(50), fspacy40, "DEATHS");
}

static void kmatrix_status_msg (fix time, int reactor)
{
	gr_set_curfont(GAME_FONT);
	gr_set_fontcolor(gr_find_closest_color(255,255,255),-1);

	if (reactor)
		gr_printf(0x8000, SHEIGHT-LINE_SPACING, "Waiting for players to finish level. Reactor time: T-%d", time);
	else
		gr_printf(0x8000, SHEIGHT-LINE_SPACING, "Level finished. Wait (%d) to proceed or ESC to Quit.", time);
}

namespace {

struct kmatrix_screen : ignore_window_pointer_t
{
	grs_main_bitmap background;
	int network;
	fix64 end_time;
	int playing;
        int aborted;
};

}

namespace dsx {
static void kmatrix_redraw(kmatrix_screen *km)
{
	playernum_array_t sorted;

	gr_set_current_canvas(NULL);
	show_fullscr(km->background);
	
	if (Game_mode & GM_MULTI_COOP)
	{
		kmatrix_redraw_coop();
	}
	else
	{
		multi_sort_kill_list();
		gr_set_curfont(MEDIUM3_FONT);

		const auto title =
#if defined(DXX_BUILD_DESCENT_II)
			game_mode_capture_flag()
			? "CAPTURE THE FLAG SUMMARY"
			: game_mode_hoard()
				? "HOARD SUMMARY"
				:
#endif
				TXT_KILL_MATRIX_TITLE;
		gr_string(0x8000, FSPACY(10), title);

		gr_set_curfont(GAME_FONT);
		multi_get_kill_list(sorted);
		kmatrix_draw_names(sorted);

		for (int i=0; i<N_players; i++ )
		{
			if (Players[sorted[i]].connected==CONNECT_DISCONNECTED)
				gr_set_fontcolor(gr_find_closest_color(31,31,31),-1);
			else
			{
				const auto color = get_player_or_team_color(sorted[i]);
				gr_set_fontcolor(BM_XRGB(player_rgb[color].r,player_rgb[color].g,player_rgb[color].b),-1 );
			}

			kmatrix_draw_item( i, sorted );
		}
	}

	gr_palette_load(gr_palette);
}
}

static void kmatrix_redraw_coop()
{
	playernum_array_t sorted;

	multi_sort_kill_list();
	gr_set_curfont(MEDIUM3_FONT);
	gr_string( 0x8000, FSPACY(10), "COOPERATIVE SUMMARY");
	gr_set_curfont(GAME_FONT);
	multi_get_kill_list(sorted);
	kmatrix_draw_coop_names(sorted);
	const auto &&fspacx = FSPACX();
	const auto &&fspacy = FSPACY();
	const auto x_callsign = fspacx(CENTERING_OFFSET(N_players));
	const auto x_centerscreen = CENTERSCREEN;
	const auto &&fspacx50 = fspacx(50);
	const auto rgb60_40_10 = BM_XRGB(60, 40, 10);

	for (playernum_t i = 0; i < N_players; ++i)
	{
		auto &plr = Players[sorted[i]];
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
		gr_set_fontcolor(gr_find_closest_color(r, g, b), -1);

		const auto &&y = fspacy(50 + i * 9);
		gr_string(x_callsign, y, static_cast<const char *>(plr.callsign));
		gr_set_fontcolor(rgb60_40_10, -1);
		auto &player_info = vcobjptr(plr.objnum)->ctype.player_info;
		gr_printf(x_centerscreen, y, "%d", player_info.mission.score);
		gr_printf(x_centerscreen + fspacx50, y, "%d", plr.net_killed_total);
	}

	gr_palette_load(gr_palette);
}

namespace dsx {
static window_event_result kmatrix_handler(window *, const d_event &event, kmatrix_screen *km)
{
	int k = 0, choice = 0;
	
	switch (event.type)
	{
		case EVENT_KEY_COMMAND:
			k = event_key_get(event);
			switch( k )
			{
				case KEY_ESC:
					{
						array<newmenu_item, 2> nm_message_items{{
							nm_item_menu(TXT_YES),
							nm_item_menu(TXT_NO),
						}};
						choice = newmenu_do(nullptr, TXT_ABORT_GAME, nm_message_items, km->network ? get_multi_endlevel_poll2() : unused_newmenu_subfunction, unused_newmenu_userdata);
					}
					
					if (choice==0)
					{
						get_local_player().connected=CONNECT_DISCONNECTED;
						
						if (km->network)
							multi_send_endlevel_packet();
						
						multi_leave_game();
                                                km->aborted = 1;

						return window_event_result::close;
					}
					return window_event_result::handled;
					
				default:
					break;
			}
			break;
			
		case EVENT_WINDOW_DRAW:
			timer_delay2(50);

			if (km->network)
				multi_do_protocol_frame(0, 1);
			
			km->playing = 0;

			// Check if all connected players are also looking at this screen ...
			range_for (auto &i, Players)
				if (i.connected)
					if (i.connected != CONNECT_END_MENU && i.connected != CONNECT_DIED_IN_MINE)
					{
						km->playing = 1;
						break;
					}
			
			// ... and let the reactor blow sky high!
			if (!km->playing)
				Countdown_seconds_left = -1;
			
			// If Reactor is finished and end_time not inited, set the time when we will exit this loop
			if (km->end_time == -1 && Countdown_seconds_left < 0 && !km->playing)
				km->end_time = timer_query() + (KMATRIX_VIEW_SEC * F1_0);
			
			// Check if end_time has been reached and exit loop
			if (timer_query() >= km->end_time && km->end_time != -1)
			{
				if (km->network)
					multi_send_endlevel_packet();  // make sure
				
#if defined(DXX_BUILD_DESCENT_II)
				if (is_D2_OEM)
				{
					if (Current_level_num==8)
					{
						get_local_player().connected=CONNECT_DISCONNECTED;
						
						if (km->network)
							multi_send_endlevel_packet();
						
						multi_leave_game();
                                                km->aborted = 1;
					}
				}
#endif
				return window_event_result::close;
			}

			kmatrix_redraw(km);
			
			if (km->playing)
				kmatrix_status_msg(Countdown_seconds_left, 1);
			else
				kmatrix_status_msg(f2i(timer_query()-km->end_time), 0);
			break;
			
		case EVENT_WINDOW_CLOSE:
			game_flush_inputs();
			newmenu_free_background();
			break;
			
		default:
			break;
	}
	return window_event_result::ignored;
}
}

kmatrix_result kmatrix_view(int network)
{
	kmatrix_screen km;
	gr_init_bitmap_data(km.background);
	if (pcx_read_bitmap(STARS_BACKGROUND, km.background, gr_palette) != PCX_ERROR_NONE)
	{
		return kmatrix_result::abort;
	}
	gr_palette_load(gr_palette);
	
	km.network = network;
	km.end_time = -1;
	km.playing = 0;
        km.aborted = 0;
	
	set_screen_mode( SCREEN_MENU );
	game_flush_inputs();

	range_for (auto &i, Players)
		if (i.objnum != object_none)
			digi_kill_sound_linked_to_object(vcobjptridx(i.objnum));

	const auto wind = window_create(grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT, kmatrix_handler, &km);
	if (!wind)
	{
		return kmatrix_result::abort;
	}
	
	event_process_all();

	return (km.aborted ? kmatrix_result::abort : kmatrix_result::proceed);
}
