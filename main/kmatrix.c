/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
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

#include "error.h"
#include "pstypes.h"
#include "gr.h"
#include "window.h"
#include "key.h"
#include "palette.h"
#include "game.h"
#include "window.h"
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

#ifdef OGL
#include "ogl_init.h"
#endif

#define CENTERING_OFFSET(x) ((300 - (70 + (x)*25 ))/2)
#define CENTERSCREEN (SWIDTH/2)
#define KMATRIX_VIEW_SEC 7 // Time after reactor explosion until new level - in seconds
void kmatrix_redraw_coop();
fix64 StartAbortMenuTime;

void kmatrix_draw_item( int  i, int *sorted )
{
	int j, x, y;
	char temp[10];

	y = FSPACY(50+i*9);
	gr_printf( FSPACX(CENTERING_OFFSET(N_players)), y, "%s", Players[sorted[i]].callsign );

	for (j=0; j<N_players; j++)
	{
		x = FSPACX(70 + CENTERING_OFFSET(N_players) + j*25);

		if (sorted[i]==sorted[j])
		{
			if (kill_matrix[sorted[i]][sorted[j]] == 0)
			{
				gr_set_fontcolor( BM_XRGB(10,10,10),-1 );
				gr_printf( x, y, "%d", kill_matrix[sorted[i]][sorted[j]] );
			}
			else
			{
				gr_set_fontcolor( BM_XRGB(25,25,25),-1 );
				gr_printf( x, y, "-%d", kill_matrix[sorted[i]][sorted[j]] );
			}
		}
		else
		{
			if (kill_matrix[sorted[i]][sorted[j]] <= 0)
			{
				gr_set_fontcolor( BM_XRGB(10,10,10),-1 );
				gr_printf( x, y, "%d", kill_matrix[sorted[i]][sorted[j]] );
			}
			else
			{
				gr_set_fontcolor( BM_XRGB(25,25,25),-1 );
				gr_printf( x, y, "%d", kill_matrix[sorted[i]][sorted[j]] );
			}
		}
	}

	if (Players[sorted[i]].net_killed_total+Players[sorted[i]].net_kills_total==0)
		sprintf (temp,"NA");
	else
		sprintf (temp,"%d%%",(int)((float)((float)Players[sorted[i]].net_kills_total/((float)Players[sorted[i]].net_killed_total+(float)Players[sorted[i]].net_kills_total))*100.0));

	x = FSPACX(60 + CENTERING_OFFSET(N_players) + N_players*25);
	gr_set_fontcolor( BM_XRGB(25,25,25),-1 );
	gr_printf( x ,y,"%4d/%s",Players[sorted[i]].net_kills_total,temp);
}

void kmatrix_draw_coop_item( int  i, int *sorted )
{
	int  x, y;

	y = FSPACY(50+i*9);
	gr_printf( FSPACX(CENTERING_OFFSET(N_players)), y, "%s", Players[sorted[i]].callsign );
	x = CENTERSCREEN;
	gr_set_fontcolor( BM_XRGB(60,40,10),-1 );
	gr_printf( x, y, "%d", Players[sorted[i]].score );
	x = CENTERSCREEN+FSPACX(50);
	gr_set_fontcolor( BM_XRGB(60,40,10),-1 );
	gr_printf( x, y, "%d", Players[sorted[i]].net_killed_total);
}

void kmatrix_draw_names(int *sorted)
{
	int j, x, color;

	for (j=0; j<N_players; j++)
	{
		if (Game_mode & GM_TEAM)
			color = get_team(sorted[j]);
		else
			color = sorted[j];

		x = FSPACX (70 + CENTERING_OFFSET(N_players) + j*25);

		if (Players[sorted[j]].connected==CONNECT_DISCONNECTED)
			gr_set_fontcolor(gr_find_closest_color(31,31,31),-1);
		else
			gr_set_fontcolor(BM_XRGB(player_rgb[color].r,player_rgb[color].g,player_rgb[color].b),-1 );

		gr_printf( x, FSPACY(40), "%c", Players[sorted[j]].callsign[0] );
	}

	x = FSPACX(72 + CENTERING_OFFSET(N_players) + N_players*25);
	gr_set_fontcolor( BM_XRGB(31,31,31),-1 );
	gr_printf( x, FSPACY(40), "K/E");
}

void kmatrix_draw_coop_names(int *sorted)
{
	sorted=sorted;

	gr_set_fontcolor( BM_XRGB(63,31,31),-1 );
	gr_printf( CENTERSCREEN, FSPACY(40), "SCORE");
	gr_set_fontcolor( BM_XRGB(63,31,31),-1 );
	gr_printf( CENTERSCREEN+FSPACX(50), FSPACY(40), "DEATHS");
}

void kmatrix_status_msg (fix time, int reactor)
{
	grd_curcanv->cv_font = GAME_FONT;
	gr_set_fontcolor(gr_find_closest_color(255,255,255),-1);

	if (reactor)
		gr_printf(0x8000, SHEIGHT-LINE_SPACING, "Waiting for players to finish level. Reactor time: T-%d", time);
	else
		gr_printf(0x8000, SHEIGHT-LINE_SPACING, "Level finished. Wait (%d) to proceed or ESC to Quit.", time);
}

typedef struct kmatrix_screen
{
	grs_bitmap background;
	int network;
	fix64 end_time;
	int playing;
} kmatrix_screen;

void kmatrix_redraw(kmatrix_screen *km)
{
	int i, color;
	int sorted[MAX_PLAYERS];

	gr_set_current_canvas(NULL);
	show_fullscr(&km->background);
	
	if (Game_mode & GM_MULTI_COOP)
	{
		kmatrix_redraw_coop();
	}
	else
	{
		multi_sort_kill_list();
		grd_curcanv->cv_font = MEDIUM3_FONT;

		gr_string( 0x8000, FSPACY(10), TXT_KILL_MATRIX_TITLE);

		grd_curcanv->cv_font = GAME_FONT;
		multi_get_kill_list(sorted);
		kmatrix_draw_names(sorted);

		for (i=0; i<N_players; i++ )
		{
			if (Game_mode & GM_TEAM)
				color = get_team(sorted[i]);
			else
				color = sorted[i];

			if (Players[sorted[i]].connected==CONNECT_DISCONNECTED)
				gr_set_fontcolor(gr_find_closest_color(31,31,31),-1);
			else
				gr_set_fontcolor(BM_XRGB(player_rgb[color].r,player_rgb[color].g,player_rgb[color].b),-1 );

			kmatrix_draw_item( i, sorted );
		}
	}

	gr_palette_load(gr_palette);
}

void kmatrix_redraw_coop()
{
	int i, color;
	int sorted[MAX_PLAYERS];

	multi_sort_kill_list();
	grd_curcanv->cv_font = MEDIUM3_FONT;
	gr_string( 0x8000, FSPACY(10), "COOPERATIVE SUMMARY");
	grd_curcanv->cv_font = GAME_FONT;
	multi_get_kill_list(sorted);
	kmatrix_draw_coop_names(sorted);

	for (i=0; i<N_players; i++ )
	{
		color = sorted[i];

		if (Players[sorted[i]].connected==CONNECT_DISCONNECTED)
			gr_set_fontcolor(gr_find_closest_color(31,31,31),-1);
		else
			gr_set_fontcolor(BM_XRGB(player_rgb[color].r,player_rgb[color].g,player_rgb[color].b),-1 );

		kmatrix_draw_coop_item( i, sorted );
	}

	gr_palette_load(gr_palette);
}

int kmatrix_handler(window *wind, d_event *event, kmatrix_screen *km)
{
	int i = 0, k = 0, choice = 0;
	
	switch (event->type)
	{
		case EVENT_KEY_COMMAND:
			k = event_key_get(event);
			switch( k )
			{
				case KEY_ESC:
					if (km->network)
					{
						StartAbortMenuTime=timer_query();
						choice=nm_messagebox1( NULL,multi_endlevel_poll2, NULL, 2, TXT_YES, TXT_NO, TXT_ABORT_GAME );
					}
					else
						choice=nm_messagebox( NULL, 2, TXT_YES, TXT_NO, TXT_ABORT_GAME );
					
					if (choice==0)
					{
						Players[Player_num].connected=CONNECT_DISCONNECTED;
						
						if (km->network)
							multi_send_endlevel_packet();
						
						multi_leave_game();
						window_close(wind);
						if (Game_wind)
							window_close(Game_wind);
						return 1;
					}
					return 1;
					
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
			for (i = 0; i < MAX_PLAYERS; i++)
				if (Players[i].connected)
					if (Players[i].connected != CONNECT_END_MENU && Players[i].connected != CONNECT_DIED_IN_MINE)
						km->playing = 1;
			
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
				
				window_close(wind);
				break;
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
			
			gr_free_bitmap_data(&km->background);
			d_free(km);
			break;
			
		default:
			break;
	}
	
	return 0;
}

void kmatrix_view(int network)
{
	kmatrix_screen *km;
	window *wind;
	int i = 0;

	MALLOC(km, kmatrix_screen, 1);
	if (!km)
		return;

	gr_init_bitmap_data(&km->background);
	if (pcx_read_bitmap(STARS_BACKGROUND, &km->background, BM_LINEAR, gr_palette) != PCX_ERROR_NONE)
	{
		d_free(km);
		return;
	}
	gr_palette_load(gr_palette);
	
	km->network = network;
	km->end_time = -1;
	km->playing = 0;
	
	set_screen_mode( SCREEN_MENU );
	game_flush_inputs();

	for (i=0;i<MAX_PLAYERS;i++)
		digi_kill_sound_linked_to_object (Players[i].objnum);

	wind = window_create(&grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT, (int (*)(window *, d_event *, void *))kmatrix_handler, km);
	if (!wind)
	{
		d_free(km);
		return;
	}
	
	while (window_exists(wind))
		event_process();
}
