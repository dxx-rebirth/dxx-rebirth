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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/kmatrix.c,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:44:38 $
 * 
 * Kill matrix displayed at end of level.
 * 
 * $Log: kmatrix.c,v $
 * Revision 1.1.1.1  2006/03/17 19:44:38  zicodxx
 * initial import
 *
 * Revision 1.2  2002/03/26 22:10:09  donut
 * fix multiplayer endlevel score display in high res
 *
 * Revision 1.1.1.1  1999/06/14 22:08:16  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.3  1995/05/02  17:01:22  john
 * Fixed bug with kill list not showing up in VFX mode.
 * 
 * Revision 2.2  1995/03/21  14:38:20  john
 * Ifdef'd out the NETWORK code.
 * 
 * Revision 2.1  1995/03/06  15:22:54  john
 * New screen techniques.
 * 
 * Revision 2.0  1995/02/27  11:25:56  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.19  1995/02/15  14:47:23  john
 * Added code to keep track of kills during endlevel.
 * 
 * Revision 1.18  1995/02/08  11:00:06  rob
 * Moved string to localized file
 * 
 * Revision 1.17  1995/02/01  23:45:55  rob
 * Fixed string.
 * 
 * Revision 1.16  1995/01/30  21:47:11  rob
 * Added a line of instructions.
 * 
 * Revision 1.15  1995/01/20  16:58:43  rob
 * careless careless careless... 
 * 
 * 
 * Revision 1.14  1995/01/20  13:43:48  rob
 * Longer time to view.
 * 
 * Revision 1.13  1995/01/20  13:42:34  rob
 * Fixed sorting bug.
 * 
 * Revision 1.12  1995/01/19  17:35:21  rob
 * Fixed coloration of player names in team mode.
 * 
 * Revision 1.11  1995/01/16  21:26:15  rob
 * Fixed it!!
 * 
 * Revision 1.10  1995/01/16  18:55:41  rob
 * Added include of network.h
 * 
 * Revision 1.9  1995/01/16  18:22:35  rob
 * Fixed problem with signs.
 * 
 * Revision 1.8  1995/01/12  16:07:51  rob
 * ADded sorting before display.
 * 
 * Revision 1.7  1995/01/04  08:46:53  rob
 * JOHN CHECKED IN FOR ROB !!!
 * 
 * Revision 1.6  1994/12/09  20:17:20  yuan
 * Touched up
 * 
 * Revision 1.5  1994/12/09  19:46:35  yuan
 * Localized the sucker.
 * 
 * Revision 1.4  1994/12/09  19:24:58  rob
 * Yuan's fix to the centering.
 * 
 * Revision 1.3  1994/12/09  19:02:37  yuan
 * Cleaned up a bit.
 * 
 * Revision 1.2  1994/12/09  16:19:46  yuan
 * kill matrix stuff.
 * 
 * Revision 1.1  1994/12/09  15:08:58  yuan
 * Initial revision
 * 
 * 
 */


#ifdef RCS
#pragma off (unreferenced)
static char rcsid[] = "$Id: kmatrix.c,v 1.1.1.1 2006/03/17 19:44:38 zicodxx Exp $";
#pragma on (unreferenced)
#endif

#ifdef NETWORK

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>

#include "error.h"
#include "types.h"
#include "gr.h"
#include "mono.h"
#include "key.h"
#include "palette.h"
#include "game.h"
#include "gamefont.h"
#include "u_mem.h"
#include "newmenu.h"
#include "menu.h"
#include "player.h"
#include "screens.h"
#include "gamefont.h"
#include "mouse.h"
#include "joy.h"
#include "timer.h"
#include "text.h"
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
		gr_set_fontcolor(gr_getcolor(player_rgb[color].r,player_rgb[color].g,player_rgb[color].b),-1 );
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

//	gr_set_fontcolor(gr_getcolor(player_rgb[j].r,player_rgb[j].g,player_rgb[j].b),-1 );
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
	
	pcx_error = pcx_read_fullscr("STARS.PCX", NULL);
	Assert(pcx_error == PCX_ERROR_NONE);

	grd_curcanv->cv_font = Gamefonts[GFONT_MEDIUM_3];

	gr_string( 0x8000, rescale_y(15), TXT_KILL_MATRIX_TITLE	);

	grd_curcanv->cv_font = Gamefonts[GFONT_SMALL];

	multi_get_kill_list(sorted);

	kmatrix_draw_names(sorted);

	for (i=0; i<N_players; i++ )		{
//		mprintf((0, "Sorted kill list pos %d = %d.\n", i+1, sorted[i]));

		if (Game_mode & GM_TEAM)
			color = get_team(sorted[i]);
		else
			color = sorted[i];

		gr_set_fontcolor(gr_getcolor(player_rgb[color].r,player_rgb[color].g,player_rgb[color].b),-1 );
		kmatrix_draw_item( i, sorted );
	}

	kmatrix_draw_deaths(sorted);

        gr_update();
}

#define MAX_VIEW_TIME	F1_0*60


void kmatrix_view(int network)
{
	int i, k, done;
	fix entry_time = timer_get_approx_seconds();
	//edit 05/18/99 Matt Mueller - should be initialized.
	int key=0;
	//end edit -MM

	set_screen_mode( SCREEN_MENU );

	gr_palette_fade_in( gr_palette,32, 0);
	game_flush_inputs();

	done = 0;

	while(!done)	{
		timer_delay2(20);
		kmatrix_redraw();

		for (i=0; i<4; i++ )	
			if (joy_get_button_down_cnt(i)>0) done=1;
		for (i=0; i<3; i++ )	
			if (mouse_button_down_count(i)>0) done=1;

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
#ifdef OGL
		gr_flip();
#endif
	}

// Restore background and exit
	gr_palette_fade_out( gr_palette, 32, 0 );

	game_flush_inputs();
}
#endif
