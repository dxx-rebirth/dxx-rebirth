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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * Kill matrix displayed at end of level.
 * This source file contains code for both newer networking protocols and IPX. Pretty much redundant stuff but lets keep a clean cut until IPX dies completly.
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
#include "gamefont.h"
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

/* IPX CODE - START */

#define MAX_VIEW_TIME       F1_0*15
#define ENDLEVEL_IDLE_TIME  F1_0*10
fix StartAbortMenuTime;
char ConditionLetters[]={' ','P','E','D','E','E','V','W'};
extern void newmenu_free_background();

void kmatrix_ipx_reactor (char *message);
void kmatrix_ipx_phallic ();
void kmatrix_ipx_redraw_coop();

void kmatrix_ipx_draw_item( int  i, int *sorted )
{
	int j, x, y;
	char temp[10];

	y = FSPACY(50+i*9);

	// Print player name.

	gr_printf( FSPACX(CENTERING_OFFSET(N_players)), y, "%s", Players[sorted[i]].callsign );

	gr_printf (FSPACX(CENTERING_OFFSET(N_players)-15),y,"%c",ConditionLetters[Players[sorted[i]].connected]);

	for (j=0; j<N_players; j++) {

		x = FSPACX(70 + CENTERING_OFFSET(N_players) + j*25);

		if (sorted[i]==sorted[j]) {
			if (kill_matrix[sorted[i]][sorted[j]] == 0) {
				gr_set_fontcolor( BM_XRGB(10,10,10),-1 );
				gr_printf( x, y, "%d", kill_matrix[sorted[i]][sorted[j]] );
			} else {
				gr_set_fontcolor( BM_XRGB(25,25,25),-1 );
				gr_printf( x, y, "-%d", kill_matrix[sorted[i]][sorted[j]] );
			}
		} else {
			if (kill_matrix[sorted[i]][sorted[j]] <= 0) {
				gr_set_fontcolor( BM_XRGB(10,10,10),-1 );
				gr_printf( x, y, "%d", kill_matrix[sorted[i]][sorted[j]] );
			} else {
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

void kmatrix_ipx_draw_coop_item( int  i, int *sorted )
{
	int  x, y;
	
	y = FSPACY(50+i*9);
	
	// Print player name.
	
	gr_printf( FSPACX(CENTERING_OFFSET(N_players)), y, "%s", Players[sorted[i]].callsign );
	gr_printf (FSPACX(CENTERING_OFFSET(N_players)-15),y,"%c",ConditionLetters[Players[sorted[i]].connected]);
	
	x = CENTERSCREEN;
	
	gr_set_fontcolor( BM_XRGB(60,40,10),-1 );
	gr_printf( x, y, "%d", Players[sorted[i]].score );
	
	x = CENTERSCREEN+FSPACX(50);
	
	gr_set_fontcolor( BM_XRGB(60,40,10),-1 );
	gr_printf( x, y, "%d", Players[sorted[i]].net_killed_total);
}

void kmatrix_ipx_draw_names(int *sorted)
{
	int j, x, color;

	for (j=0; j<N_players; j++) {
		if (Game_mode & GM_TEAM)
			color = get_team(sorted[j]);
		else
			color = sorted[j];

		x = FSPACX(70 + CENTERING_OFFSET(N_players) + j*25);

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

void kmatrix_ipx_draw_coop_names(int *sorted)
{
	sorted=sorted;
	
	gr_set_fontcolor( BM_XRGB(63,31,31),-1 );
	gr_printf( CENTERSCREEN, FSPACY(40), "SCORE");
	gr_set_fontcolor( BM_XRGB(63,31,31),-1 );
	gr_printf( CENTERSCREEN+FSPACX(50), FSPACY(40), "DEATHS");
}

void kmatrix_ipx_draw_deaths(int *sorted)
{
	int y,x;
	char reactor_message[50];
	
	sorted=sorted;

	y = FSPACY(55 + 72 + 35);
	x = FSPACX(35);

	{
		int sw, sh, aw;

		gr_set_fontcolor(gr_find_closest_color(63,20,0),-1);
		gr_get_string_size("P-Playing E-Escaped D-Died", &sw, &sh, &aw);

		gr_printf( CENTERSCREEN-(sw/2), y,"P-Playing E-Escaped D-Died");

		y+=(sh+5);
		gr_get_string_size("V-Viewing scores W-Waiting", &sw, &sh, &aw);
		
		gr_printf( CENTERSCREEN-(sw/2), y,"V-Viewing scores W-Waiting");
	}

	y+=FSPACY(20);

	{
		int sw, sh, aw;
		gr_set_fontcolor(gr_find_closest_color(63,63,63),-1);

		if (Players[Player_num].connected==CONNECT_KMATRIX_WAITING)
		{
			gr_get_string_size("Waiting for other players...",&sw, &sh, &aw);
			gr_printf( CENTERSCREEN-(sw/2), y,"Waiting for other players...");
		}
		else
		{
			gr_get_string_size(TXT_PRESS_ANY_KEY2, &sw, &sh, &aw);
			gr_printf( CENTERSCREEN-(sw/2), y, TXT_PRESS_ANY_KEY2);
		}
	}

	if (Countdown_seconds_left <=0)
		kmatrix_ipx_reactor(TXT_REACTOR_EXPLODED);
	else
	{
		sprintf((char *)&reactor_message, "%s: %d %s  ", TXT_TIME_REMAINING, Countdown_seconds_left, TXT_SECONDS);
		kmatrix_ipx_reactor ((char *)&reactor_message);
	}

	if (Game_mode & GM_HOARD) 
		kmatrix_ipx_phallic();
}

void kmatrix_ipx_reactor (char *message)
{
	static char oldmessage[50]={0};
	int sw, sh, aw;
	
	grd_curcanv->cv_font = GAME_FONT;
	
	if (oldmessage[0]!=0)
	{
		gr_set_fontcolor(gr_find_closest_color(0,0,0),-1);
		gr_get_string_size(oldmessage, &sw, &sh, &aw);
	}
	
	gr_set_fontcolor(gr_find_closest_color(0,32,63),-1);
	gr_get_string_size(message, &sw, &sh, &aw);
	gr_printf( CENTERSCREEN-(sw/2), FSPACY(55+72+12), message);
	strcpy ((char *)&oldmessage,message);
}

extern int PhallicLimit,PhallicMan;

void kmatrix_ipx_phallic ()
{
	int sw, sh, aw;
	char message[80];
	
	if (!(Game_mode & GM_HOARD))
		return;
	
	if (PhallicMan==-1)
		strcpy (message,"There was no record set for this level.");
	else
		sprintf (message,"%s had the best record at %d points.",Players[PhallicMan].callsign,PhallicLimit);
	
	grd_curcanv->cv_font = GAME_FONT;
	gr_set_fontcolor(gr_find_closest_color(63,63,63),-1);
	gr_get_string_size(message, &sw, &sh, &aw);
	gr_printf( CENTERSCREEN-(sw/2), FSPACY(55+72+3), message);
}

typedef struct kmatrix_ipx_screen
{
	grs_bitmap background;
	fix entry_time;
	int oldstates[MAX_PLAYERS];
	int network;
} kmatrix_ipx_screen;

void kmatrix_ipx_redraw(kmatrix_ipx_screen *km)
{
	int i, color;
	int sorted[MAX_NUM_NET_PLAYERS];

	if (Game_mode & GM_MULTI_COOP)
	{
		kmatrix_ipx_redraw_coop();
		return;
	}
	
	multi_sort_kill_list();

	gr_set_current_canvas(NULL);
	
	show_fullscr(&km->background);

	grd_curcanv->cv_font = MEDIUM3_FONT;

	if (Game_mode & GM_CAPTURE)
		gr_string( 0x8000, FSPACY(10), "CAPTURE THE FLAG SUMMARY");
	else if (Game_mode & GM_HOARD)
		gr_string( 0x8000, FSPACY(10), "HOARD SUMMARY");
	else
		gr_string( 0x8000, FSPACY(10), TXT_KILL_MATRIX_TITLE);

	grd_curcanv->cv_font = GAME_FONT;

	multi_get_kill_list(sorted);

	kmatrix_ipx_draw_names(sorted);

	for (i=0; i<N_players; i++ )  {
		if (Game_mode & GM_TEAM)
			color = get_team(sorted[i]);
		else
			color = sorted[i];

		if (Players[sorted[i]].connected==CONNECT_DISCONNECTED)
			gr_set_fontcolor(gr_find_closest_color(31,31,31),-1);
		else
			gr_set_fontcolor(BM_XRGB(player_rgb[color].r,player_rgb[color].g,player_rgb[color].b),-1 );

		kmatrix_ipx_draw_item( i, sorted );
	}

	kmatrix_ipx_draw_deaths(sorted);
}

void kmatrix_ipx_redraw_coop()
{
	int i, color;
	int sorted[MAX_NUM_NET_PLAYERS];
	
	multi_sort_kill_list();
	
	grd_curcanv->cv_font = MEDIUM3_FONT;
	gr_string( 0x8000, FSPACY(10), "COOPERATIVE SUMMARY");
	
	grd_curcanv->cv_font = GAME_FONT;
	
	multi_get_kill_list(sorted);
	
	kmatrix_ipx_draw_coop_names(sorted);
	
	for (i=0; i<N_players; i++ )  {
		
		color = sorted[i];
		
		if (Players[sorted[i]].connected==CONNECT_DIED_IN_MINE)
			gr_set_fontcolor(gr_find_closest_color(31,31,31),-1);
		else
			gr_set_fontcolor(BM_XRGB(player_rgb[color].r,player_rgb[color].g,player_rgb[color].b),-1 );
		
		kmatrix_ipx_draw_coop_item( i, sorted );
	}
	
	kmatrix_ipx_draw_deaths(sorted);
	
	gr_palette_load(gr_palette);
}

int kmatrix_ipx_handler(window *wind, d_event *event, kmatrix_ipx_screen *km)
{
	int i, k, choice;
	int num_ready,num_escaped;
	
	switch (event->type)
	{
		case EVENT_KEY_COMMAND:
			k = ((d_event_keycommand *)event)->keycode;
			switch( k )
			{
				case KEY_ENTER:
				case KEY_SPACEBAR:
					if (is_D2_OEM)
					{
						if (Current_level_num==8)
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
					}
					
					Players[Player_num].connected=CONNECT_KMATRIX_WAITING;
					if (km->network)	
						multi_send_endlevel_packet();
					return 1;
					
				case KEY_ESC:
					if (Game_mode & GM_NETWORK)
					{
						StartAbortMenuTime=timer_get_fixed_seconds();
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
			
		case EVENT_IDLE:
			timer_delay2(50);

			if (timer_get_fixed_seconds() >= (km->entry_time+MAX_VIEW_TIME) && Players[Player_num].connected!=CONNECT_KMATRIX_WAITING)
			{
				if (is_D2_OEM)
				{
					if (Current_level_num==8)
					{
						Players[Player_num].connected=CONNECT_DISCONNECTED;
						if (km->network)
							multi_send_endlevel_packet();
						multi_leave_game();
						window_close(wind);
						if (Game_wind)
							window_close(Game_wind);
						return 0;
					}
				}
				
				Players[Player_num].connected=CONNECT_KMATRIX_WAITING;
				if (km->network)
					multi_send_endlevel_packet();
			}
			
			if (km->network && (Game_mode & GM_NETWORK))
			{
				multi_endlevel_poll1(NULL, event, NULL);
				
				for (num_escaped=0,num_ready=0,i=0;i<N_players;i++)
				{
					if (Players[i].connected && i!=Player_num)
					{
						// Check timeout for idle players
						if (timer_get_fixed_seconds() > Netgame.players[i].LastPacketTime+ENDLEVEL_IDLE_TIME)
						{
							Players[i].connected = CONNECT_DISCONNECTED;
						}
					}
					
					// HACK: If a player legally exits kmatrix loop he will send the previous connect byte which can invalidate us from setting him to ready and let us stuck here forever... it's stupid to solve it this way, but for now I have no better idea.
					if ((km->oldstates[i]==CONNECT_END_MENU || km->oldstates[i]==CONNECT_KMATRIX_WAITING) && // player was viewing scores before...
						(Players[i].connected!=CONNECT_DISCONNECTED || Players[i].connected!=CONNECT_END_MENU || Players[i].connected!=CONNECT_KMATRIX_WAITING)) // ... but now he sends neither disconnect or further waiting signal - so he MUST be out of kmatrix already
						Players[i].connected=CONNECT_KMATRIX_WAITING;
					
					if (Players[i].connected!=km->oldstates[i])
					{
						if (ConditionLetters[Players[i].connected]!=ConditionLetters[km->oldstates[i]])
							km->oldstates[i]=Players[i].connected;
						multi_send_endlevel_packet();
					}
					
					if (Players[i].connected==CONNECT_DISCONNECTED || Players[i].connected==CONNECT_KMATRIX_WAITING)
						num_ready++;
					
					if (Players[i].connected!=CONNECT_PLAYING)
						num_escaped++;
				}
				
				if (num_ready>=N_players)
				{
					window_close(wind);
					
					Players[Player_num].connected=CONNECT_KMATRIX_WAITING;
					
					if (km->network)
						multi_send_endlevel_packet();  // make sure
				}
				if (num_escaped>=N_players)
					Countdown_seconds_left=-1;
			}
			break;

		case EVENT_WINDOW_DRAW:
			kmatrix_ipx_redraw(km);
			break;
			
		case EVENT_WINDOW_CLOSE:
			game_flush_inputs();
			gr_free_bitmap_data(&km->background);
			d_free(km);
			break;
			
		default:
			break;
	}
	
	return 0;
}

void kmatrix_ipx_view(int network)
{
	window *wind;
	kmatrix_ipx_screen *km;
	int i;
	int key;
	
	MALLOC(km, kmatrix_ipx_screen, 1);
	if (!km)
		return;

	gr_init_bitmap_data(&km->background);
	if (pcx_read_bitmap(STARS_BACKGROUND, &km->background, BM_LINEAR, gr_palette) != PCX_ERROR_NONE)
	{
		d_free(km);
		return;
	}
	gr_palette_load(gr_palette);
	
	km->entry_time = timer_get_fixed_seconds();
	km->network = network;

	for (i=0;i<MAX_NUM_NET_PLAYERS;i++)
		digi_kill_sound_linked_to_object (Players[i].objnum);

	set_screen_mode( SCREEN_MENU );

	game_flush_inputs();

	for (i=0;i<N_players;i++)
		km->oldstates[i]=Players[i].connected;

	if (km->network)
		multi_endlevel(&key);
	
	wind = window_create(&grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT, (int (*)(window *, d_event *, void *))kmatrix_ipx_handler, km);
	if (!wind)
	{
		d_free(km);
		return;
	}

	while (window_exists(wind))
		event_process();
}

/* IPX CODE - END */

/* NEW CODE - START */

#define KMATRIX_VIEW_SEC 7 // Time after reactor explosion until new level - in seconds
void kmatrix_phallic ();
void kmatrix_redraw_coop();

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

extern int PhallicLimit,PhallicMan;

void kmatrix_phallic ()
{
	int sw, sh, aw;
	char message[80];

	if (PhallicMan==-1)
		strcpy (message,"There was no record set for this level.");
	else
		sprintf (message,"%s had the best record at %d points.",Players[PhallicMan].callsign,PhallicLimit);

	grd_curcanv->cv_font = GAME_FONT;
	gr_set_fontcolor(gr_find_closest_color(63,63,63),-1);
	gr_get_string_size(message, &sw, &sh, &aw);
	gr_printf( CENTERSCREEN-(sw/2), FSPACY(55+72+3), message);
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
	fix end_time;
	int playing;
} kmatrix_screen;

void kmatrix_redraw(kmatrix_screen *km)
{
	int i, color;
	int sorted[MAX_NUM_NET_PLAYERS];

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

		if (Game_mode & GM_CAPTURE)
			gr_string( 0x8000, FSPACY(10), "CAPTURE THE FLAG SUMMARY");
		else if (Game_mode & GM_HOARD)
			gr_string( 0x8000, FSPACY(10), "HOARD SUMMARY");
		else
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
	int sorted[MAX_NUM_NET_PLAYERS];

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
	fix time = timer_get_fixed_seconds();
	
	switch (event->type)
	{
		case EVENT_KEY_COMMAND:
			k = ((d_event_keycommand *)event)->keycode;
			switch( k )
			{
				case KEY_ESC:
					if (km->network)
					{
						StartAbortMenuTime=timer_get_fixed_seconds();
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
			
		case EVENT_IDLE:
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
				km->end_time = time + (KMATRIX_VIEW_SEC * F1_0);
			
			// Check if end_time has been reached and exit loop
			if (time >= km->end_time && km->end_time != -1)
			{
				if (km->network)
					multi_send_endlevel_packet();  // make sure
				
				if (is_D2_OEM)
				{
					if (Current_level_num==8)
					{
						Players[Player_num].connected=CONNECT_DISCONNECTED;
						
						if (km->network)
							multi_send_endlevel_packet();
						
						multi_leave_game();
						window_close(wind);
						if (Game_wind)
							window_close(Game_wind);
						return 0;
					}
				}
				
				window_close(wind);
			}
			break;

		case EVENT_WINDOW_DRAW:
			kmatrix_redraw(km);
			
			if (km->playing)
				kmatrix_status_msg(Countdown_seconds_left, 1);
			else
				kmatrix_status_msg(f2i(time-km->end_time), 0);
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

	for (i=0;i<MAX_NUM_NET_PLAYERS;i++)
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

/* NEW CODE - END */
