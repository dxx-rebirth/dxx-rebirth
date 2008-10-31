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


#ifdef NETWORK

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>

#include "error.h"
#include "pstypes.h"
#include "gr.h"
#include "key.h"
#include "palette.h"
#include "game.h"
#include "gamefont.h"
#include "u_mem.h"
#include "newmenu.h"
#include "menu.h"
#include "player.h"
#include "screens.h"
#include "mouse.h"
#include "joy.h"
#include "timer.h"
#include "text.h"
#include "songs.h"
#include "multi.h"
#include "kmatrix.h"
#include "gauges.h"
#include "pcx.h"
#include "network.h"

#ifdef OGL
#include "ogl_init.h"
#endif

static int rescale_x(int x)
{
	return x * GWIDTH / 320;
}

static int rescale_y(int y)
{
	return y * GHEIGHT / 200;
}

#define CENTERING_OFFSET(x) ((300 - (70 + (x)*25 ))/2)

int kmatrix_kills_changed = 0;

void kmatrix_draw_item( int  i, int *sorted )
{
	int j, x, y;

	y = rescale_y(50+i*9);

	// Print player name.

	gr_printf( rescale_x(CENTERING_OFFSET(N_players)), y, "%s", Players[sorted[i]].callsign );

	for (j=0; j<N_players; j++) {

		x = rescale_x(70 + CENTERING_OFFSET(N_players) + j*25);

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
	
	x = rescale_x(70 + CENTERING_OFFSET(N_players) + N_players*25);
	gr_set_fontcolor( BM_XRGB(25,25,25),-1 );
	gr_printf( x ,y,"%4d",Players[sorted[i]].net_kills_total);
}

void kmatrix_draw_names(int *sorted)
{
	int j, x;
	
	int color;

	for (j=0; j<N_players; j++) {
		
		if (Game_mode & GM_TEAM)
			color = get_team(sorted[j]);
		else
			color = sorted[j];

		x = rescale_x(70 + CENTERING_OFFSET(N_players) + j*25);
		gr_set_fontcolor(BM_XRGB(player_rgb[color].r,player_rgb[color].g,player_rgb[color].b),-1 );
		gr_printf( x, rescale_y(40), "%c", Players[sorted[j]].callsign[0] );
	}

	x = rescale_x(70 + CENTERING_OFFSET(N_players) + N_players*25);
	gr_set_fontcolor( BM_XRGB(31,31,31),-1 );
	gr_printf( x, rescale_y(40), TXT_KILLS);

		
}


void kmatrix_draw_deaths(int *sorted)
{
	int j, x, y;
	
	y = rescale_y(55 + N_players * 9);

//	gr_set_fontcolor(BM_XRGB(player_rgb[j].r,player_rgb[j].g,player_rgb[j].b),-1 );
	gr_set_fontcolor( BM_XRGB(31,31,31),-1 );

	x = rescale_x(CENTERING_OFFSET(N_players));
	gr_printf( x, y, TXT_DEATHS );

	for (j=0; j<N_players; j++) {
		x = rescale_x(70 + CENTERING_OFFSET(N_players) + j*25);
		gr_printf( x, y, "%d", Players[sorted[j]].net_killed_total );
	}

	y = rescale_y(55 + 72 + 12);
	x = rescale_x(35);

	{
		int sw, sh, aw;
		gr_get_string_size(TXT_PRESS_ANY_KEY2, &sw, &sh, &aw);	
		gr_printf( rescale_x(160)-(sw/2), y, TXT_PRESS_ANY_KEY2);
	}
}


void kmatrix_redraw()
{
	int i, pcx_error, color;
		
	int sorted[MAX_NUM_NET_PLAYERS];

	multi_sort_kill_list();

	gr_set_current_canvas(NULL);
	
	pcx_error = pcx_read_fullscr(STARS_BACKGROUND, gr_palette);
	Assert(pcx_error == PCX_ERROR_NONE);

	grd_curcanv->cv_font = MEDIUM3_FONT;

	gr_string( 0x8000, rescale_y(15), TXT_KILL_MATRIX_TITLE	);

	grd_curcanv->cv_font = GAME_FONT;

	multi_get_kill_list(sorted);

	kmatrix_draw_names(sorted);

	for (i=0; i<N_players; i++ )		{
		if (Game_mode & GM_TEAM)
			color = get_team(sorted[i]);
		else
			color = sorted[i];

		gr_set_fontcolor(BM_XRGB(player_rgb[color].r,player_rgb[color].g,player_rgb[color].b),-1 );
		kmatrix_draw_item( i, sorted );
	}

	kmatrix_draw_deaths(sorted);
}

#define MAX_VIEW_TIME	F1_0*60


void kmatrix_view(int network)
{
	int k, done;
	fix entry_time = timer_get_approx_seconds();
	//edit 05/18/99 Matt Mueller - should be initialized.
	int key=0;
	//end edit -MM

	set_screen_mode( SCREEN_MENU );

	game_flush_inputs();

	done = 0;

	while(!done)	{
		timer_delay2(50);
		kmatrix_redraw();

		//see if redbook song needs to be restarted
		songs_check_redbook_repeat();
		
		k = key_inkey();
		switch( k )	{
			case KEY_ENTER:
			case KEY_SPACEBAR:
			case KEY_ESC:
				done=1;
				break;
			case KEY_PRINT_SCREEN:
				save_screen_shot(0);
				break;
			case KEY_BACKSP:
				Int3();
				break;
			default:
				break;
		}
		if (timer_get_approx_seconds() > entry_time+MAX_VIEW_TIME)
			done=1;

		if (network && (Game_mode & GM_NETWORK))
		{
			kmatrix_kills_changed = 0;
			network_endlevel_poll2(0, NULL, &key, 0);
			if ( kmatrix_kills_changed )	{
				kmatrix_redraw();
			}
			if (key < -1)
				done = 1;
		}
		gr_flip();
	}

	game_flush_inputs();
}
#endif
