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
 * Inferno High Scores and Statistics System
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "scores.h"
#include "dxxerror.h"
#include "pstypes.h"
#include "window.h"
#include "gr.h"
#include "key.h"
#include "mouse.h"
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
#include "strutil.h"
#include "rbaudio.h"
#include "physfsx.h"

#ifdef OGL
#include "ogl_init.h"
#endif

#define VERSION_NUMBER 		1
#define SCORES_FILENAME 	"descent.hi"
#define COOL_MESSAGE_LEN 	50
#define MAX_HIGH_SCORES 	10

#if defined(DXX_BUILD_DESCENT_I)
#define DXX_SCORE_STRUCT_PACK	__pack__
#elif defined(DXX_BUILD_DESCENT_II)
#define DXX_SCORE_STRUCT_PACK
#endif

struct stats_info
{
	callsign_t name;
	int		score;
	sbyte   starting_level;
	sbyte   ending_level;
	sbyte   diff_level;
	short 	kill_ratio;		// 0-100
	short	hostage_ratio;  // 
	int		seconds;		// How long it took in seconds...
} DXX_SCORE_STRUCT_PACK;

struct all_scores
{
	char			signature[3];			// DHS
	sbyte           version;				// version
	char			cool_saying[COOL_MESSAGE_LEN];
	stats_info	stats[MAX_HIGH_SCORES];
} DXX_SCORE_STRUCT_PACK;


static void scores_read(all_scores *scores)
{
	int fsize;

	// clear score array...
	*scores = {};

	RAIIPHYSFS_File fp{PHYSFS_openRead(SCORES_FILENAME)};
	if (!fp)
	{
	 	// No error message needed, code will work without a scores file
		sprintf( scores->cool_saying, "%s", TXT_REGISTER_DESCENT );
		scores->stats[0].name = "Parallax";
		scores->stats[1].name = "Matt";
		scores->stats[2].name = "Mike";
		scores->stats[3].name = "Adam";
		scores->stats[4].name = "Mark";
		scores->stats[5].name = "Jasen";
		scores->stats[6].name = "Samir";
		scores->stats[7].name = "Doug";
		scores->stats[8].name = "Dan";
		scores->stats[9].name = "Jason";

		for (int i=0; i<10; i++)
			scores->stats[i].score = (10-i)*1000;
		return;
	}

	fsize = PHYSFS_fileLength(fp);

	if ( fsize != sizeof(all_scores) )	{
		return;
	}
	// Read 'em in...
	PHYSFS_read(fp, scores, sizeof(all_scores), 1);
	if ( (scores->version!=VERSION_NUMBER)||(scores->signature[0]!='D')||(scores->signature[1]!='H')||(scores->signature[2]!='S') )	{
		*scores = {};
		return;
	}
}

static void scores_write(all_scores *scores)
{
	RAIIPHYSFS_File fp{PHYSFS_openWrite(SCORES_FILENAME)};
	if (!fp)
	{
		nm_messagebox( TXT_WARNING, 1, TXT_OK, "%s\n'%s'", TXT_UNABLE_TO_OPEN, SCORES_FILENAME  );
		return;
	}

	scores->signature[0]='D';
	scores->signature[1]='H';
	scores->signature[2]='S';
	scores->version = VERSION_NUMBER;
	PHYSFS_write(fp, scores,sizeof(all_scores), 1);
}

static void int_to_string( int number, char *dest )
{
	int l,c;
	char buffer[20],*p;

	sprintf( buffer, "%d", number );

	l = strlen(buffer);
	if (l<=3) {
		// Don't bother with less than 3 digits
		sprintf( dest, "%d", number );
		return;
	}

	c = 0;
	p=dest;
	for (int i=l-1; i>=0; i-- ) {
		if (c==3) {
			*p++=',';
			c = 0;
		}
		c++;
		*p++ = buffer[i];
	}
	*p++ = '\0';
	d_strrev(dest);
}

static void scores_fill_struct(stats_info * stats)
{
	stats->name = Players[Player_num].callsign;
	stats->score = Players[Player_num].score;
	stats->ending_level = Players[Player_num].level;
	if (Players[Player_num].num_robots_total > 0 )	
		stats->kill_ratio = (Players[Player_num].num_kills_total*100)/Players[Player_num].num_robots_total;
	else
		stats->kill_ratio = 0;

	if (Players[Player_num].hostages_total > 0 )	
		stats->hostage_ratio = (Players[Player_num].hostages_rescued_total*100)/Players[Player_num].hostages_total;
	else
		stats->hostage_ratio = 0;

	stats->seconds = f2i(Players[Player_num].time_total)+(Players[Player_num].hours_total*3600);

	stats->diff_level = Difficulty_level;
	stats->starting_level = Players[Player_num].starting_level;
}

static inline const char *get_placement_slot_string(const unsigned position)
{
	switch(position)
	{
		default:
			Int3();
		case 0: return TXT_1ST;
		case 1: return TXT_2ND;
		case 2: return TXT_3RD;
		case 3: return TXT_4TH;
		case 4: return TXT_5TH;
		case 5: return TXT_6TH;
		case 6: return TXT_7TH;
		case 7: return TXT_8TH;
		case 8: return TXT_9TH;
		case 9: return TXT_10TH;
	}
}

void scores_maybe_add_player(int abort_flag)
{
	int position;
	all_scores scores;
	stats_info last_game;

	if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
		return;
  
	scores_read(&scores);
	
	position = MAX_HIGH_SCORES;
	for (int i=0; i<MAX_HIGH_SCORES; i++ ) {
		if ( Players[Player_num].score > scores.stats[i].score )	{
			position = i;
			break;
		}
	}
	
	if ( position == MAX_HIGH_SCORES ) {
		if (abort_flag)
			return;
		scores_fill_struct( &last_game );
	} else {
		if ( position==0 )	{
			array<char, COOL_MESSAGE_LEN+10> text1{};
			array<newmenu_item, 2> m{{
				nm_item_text(TXT_COOL_SAYING),
				nm_item_input(text1),
			}};
			newmenu_do( TXT_HIGH_SCORE, TXT_YOU_PLACED_1ST, m, unused_newmenu_subfunction, unused_newmenu_userdata );
			strncpy( scores.cool_saying, text1.data(), COOL_MESSAGE_LEN );
			if (strlen(scores.cool_saying)<1)
				sprintf( scores.cool_saying, "No Comment" );
		} else {
			nm_messagebox( TXT_HIGH_SCORE, 1, TXT_OK, "%s %s!", TXT_YOU_PLACED, get_placement_slot_string(position));
		}
	
		// move everyone down...
		for ( int i=MAX_HIGH_SCORES-1; i>position; i-- ) {
			scores.stats[i] = scores.stats[i-1];
		}

		scores_fill_struct( &scores.stats[position] );
	
		scores_write(&scores);

	}
	scores_view(&last_game, position);

	if (Game_wind)
		window_close(Game_wind);	// prevent the next game from doing funny things
}

static void scores_rputs(int x, int y, char *buffer) __attribute_nonnull();
static void scores_rputs(int x, int y, char *buffer)
{
	int w, h, aw;
	char *p;

	//replace the digit '1' with special wider 1
	for (p=buffer;*p;p++)
		if (*p=='1') *p=132;

	gr_get_string_size(buffer, &w, &h, &aw );

	gr_string( FSPACX(x)-w, FSPACY(y), buffer );
}

/* Unimplemented overload to avoid spurious warnings from the
 * macro-based dispatcher.
 */
void scores_rputs(int x, int y, const char *buffer);
static void scores_rprintf(int x, int y, const char * format, ... ) __attribute_format_printf(3, 4);
static void scores_rprintf(int x, int y, const char * format, ... )
#define scores_rprintf(A1,A2,F,...)	dxx_call_printf_checked(scores_rprintf,scores_rputs,(A1,A2),(F),##__VA_ARGS__)
{
	va_list args;
	char buffer[128];

	va_start(args, format );
	vsnprintf(buffer,sizeof(buffer),format,args);
	va_end(args);
	scores_rputs(x, y, buffer);
}

static void scores_draw_item( int i, stats_info * stats )
{
	char buffer[20];

	int y;

	y = 77+i*9;

	if (i==0)
		y -= 8;

	if ( i==MAX_HIGH_SCORES )
		y += 8;
	else
		scores_rprintf( 57, y-3, "%d.", i+1 );

	y -= 3;

	const auto &&fspacx = FSPACX();
	const auto &&fspacy = FSPACY();
	if (strlen(stats->name)==0) {
		gr_string(fspacx(66), fspacy(y), TXT_EMPTY);
		return;
	}
	gr_string(fspacx(66), fspacy(y), stats->name);
	int_to_string(stats->score, buffer);
	scores_rputs( 149, y, buffer );

	gr_string(fspacx(166), fspacy(y), MENU_DIFFICULTY_TEXT(stats->diff_level));

	if ( (stats->starting_level > 0 ) && (stats->ending_level > 0 ))
		scores_rprintf( 232, y, "%d-%d", stats->starting_level, stats->ending_level );
	else if ( (stats->starting_level < 0 ) && (stats->ending_level > 0 ))
		scores_rprintf( 232, y, "S%d-%d", -stats->starting_level, stats->ending_level );
	else if ( (stats->starting_level < 0 ) && (stats->ending_level < 0 ))
		scores_rprintf( 232, y, "S%d-S%d", -stats->starting_level, -stats->ending_level );
	else if ( (stats->starting_level > 0 ) && (stats->ending_level < 0 ))
		scores_rprintf( 232, y, "%d-S%d", stats->starting_level, -stats->ending_level );

	{
		int h, m, s;
		h = stats->seconds/3600;
		s = stats->seconds%3600;
		m = s / 60;
		s = s % 60;
		scores_rprintf( 276, y, "%d:%02d:%02d", h, m, s );
	}
}

struct scores_menu : ignore_window_pointer_t
{
	int			citem;
	fix64			t1;
	int			looper;
	all_scores	scores;
	stats_info	last_game;
};

static window_event_result scores_handler(window *wind,const d_event &event, scores_menu *menu)
{
	int k;
	static const array<int8_t, 64> fades{{
		1,1,1,2,2,3,4,4,5,6,8,9,10,12,13,15,16,17,19,20,22,23,24,26,27,28,28,29,30,30,31,31,31,31,31,30,30,29,28,28,27,26,24,23,22,20,19,17,16,15,13,12,10,9,8,6,5,4,4,3,2,2,1,1
	}};
	const auto &&fspacx = FSPACX();
	const auto &&fspacy = FSPACY();
	int w = fspacx(290), h = fspacy(170);

	switch (event.type)
	{
		case EVENT_WINDOW_ACTIVATED:
			game_flush_inputs();
			break;
			
		case EVENT_KEY_COMMAND:
			k = event_key_get(event);
			switch( k )	{
				case KEY_CTRLED+KEY_R:		
					if ( menu->citem < 0 )		{
						// Reset scores...
						if ( nm_messagebox( NULL, 2,  TXT_NO, TXT_YES, TXT_RESET_HIGH_SCORES )==1 )	{
							PHYSFS_delete(SCORES_FILENAME);
							scores_view(&menu->last_game, menu->citem);	// create new scores window
							return window_event_result::close;
						}
					}
					return window_event_result::handled;
				case KEY_ENTER:
				case KEY_SPACEBAR:
				case KEY_ESC:
					return window_event_result::close;
			}
			break;

		case EVENT_MOUSE_BUTTON_DOWN:
		case EVENT_MOUSE_BUTTON_UP:
			if (event_mouse_get_button(event) == MBTN_LEFT || event_mouse_get_button(event) == MBTN_RIGHT)
			{
				return window_event_result::close;
			}
			break;

		case EVENT_IDLE:
			timer_delay2(50);
			break;

		case EVENT_WINDOW_DRAW:
			gr_set_current_canvas(NULL);
			
			nm_draw_background(((SWIDTH-w)/2)-BORDERX,((SHEIGHT-h)/2)-BORDERY,((SWIDTH-w)/2)+w+BORDERX,((SHEIGHT-h)/2)+h+BORDERY);
			
			gr_set_current_canvas(window_get_canvas(*wind));
			gr_set_curfont(MEDIUM3_FONT);
			gr_string(0x8000, fspacy(15), TXT_HIGH_SCORES);
			gr_set_curfont(GAME_FONT);
			gr_set_fontcolor( BM_XRGB(31,26,5), -1 );
			gr_string(fspacx( 71), fspacy(50), TXT_NAME);
			gr_string(fspacx(122), fspacy(50), TXT_SCORE);
			gr_string(fspacx(167), fspacy(50), TXT_SKILL);
			gr_string(fspacx(210), fspacy(50), TXT_LEVELS);
			gr_string(fspacx(253), fspacy(50), TXT_TIME);
			
			if ( menu->citem < 0 )	
				gr_string(0x8000, fspacy(175), TXT_PRESS_CTRL_R);
			
			gr_set_fontcolor( BM_XRGB(28,28,28), -1 );
			
			gr_printf(0x8000, fspacy(31), "%c%s%c  - %s", 34, menu->scores.cool_saying, 34, static_cast<const char *>(menu->scores.stats[0].name));
			
			for (int i=0; i<MAX_HIGH_SCORES; i++ ) {
				gr_set_fontcolor( BM_XRGB(28-i*2,28-i*2,28-i*2), -1 );
				scores_draw_item( i, &menu->scores.stats[i] );
			}
			
			if ( menu->citem > -1 )	{
				
				gr_set_fontcolor( BM_XRGB(7+fades[menu->looper],7+fades[menu->looper],7+fades[menu->looper]), -1 );
				if (timer_query() >= menu->t1+F1_0/128)
				{
					menu->t1 = timer_query();
					menu->looper++;
					if (menu->looper>63) menu->looper=0;
				}

				if ( menu->citem ==  MAX_HIGH_SCORES )
					scores_draw_item( MAX_HIGH_SCORES, &menu->last_game );
				else
					scores_draw_item( menu->citem, &menu->scores.stats[menu->citem] );
			}
			gr_set_current_canvas(NULL);
			break;
			
		case EVENT_WINDOW_CLOSE:
			d_free(menu);
			break;

		default:
			break;
	}
	return window_event_result::ignored;
}

void scores_view(stats_info *last_game, int citem)
{
	scores_menu *menu;

	MALLOC(menu, scores_menu, 1);
	if (!menu)
		return;

	menu->citem = citem;
	menu->t1 = timer_query();
	menu->looper = 0;
	if (last_game)
		menu->last_game = *last_game;

	newmenu_free_background();

	scores_read(&menu->scores);

	set_screen_mode(SCREEN_MENU);
	show_menus();

	const auto &&fspacx = FSPACX();
	const auto &&fspacy = FSPACY();
	window_create(&grd_curscreen->sc_canvas, (SWIDTH - fspacx(320)) / 2, (SHEIGHT - fspacy(200)) / 2, fspacx(320), fspacy(200),
				  scores_handler, menu);
}
