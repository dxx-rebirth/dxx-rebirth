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
 * Inferno High Scores and Statistics System
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "scores.h"
#include "error.h"
#include "pstypes.h"
#include "window.h"
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
#include "strutil.h"
#include "rbaudio.h"

#ifdef OGL
#include "ogl_init.h"
#endif

#define VERSION_NUMBER 		1
#define SCORES_FILENAME 	"descent.hi"
#define COOL_MESSAGE_LEN 	50
#define MAX_HIGH_SCORES 	10

typedef struct stats_info {
  	char	name[CALLSIGN_LEN+1];
	int		score;
	sbyte   starting_level;
	sbyte   ending_level;
	sbyte   diff_level;
	short 	kill_ratio;		// 0-100
	short	hostage_ratio;  // 
	int		seconds;		// How long it took in seconds...
} stats_info;

typedef struct all_scores {
	char			signature[3];			// DHS
	sbyte           version;				// version
	char			cool_saying[COOL_MESSAGE_LEN];
	stats_info	stats[MAX_HIGH_SCORES];
} all_scores;

extern void nm_draw_background1(char * filename);

static all_scores Scores;
stats_info Last_game;
char scores_filename[128];

char * get_scores_filename()
{
#ifndef RELEASE
	// Only use the MINER variable for internal developement
	char *p;
	p=getenv( "MINER" );
	if (p)	{
		sprintf( scores_filename, "%s\\game\\%s", p, SCORES_FILENAME );
		Assert(strlen(scores_filename) < 128);
		return scores_filename;
	}
#endif
	sprintf( scores_filename, "%s", SCORES_FILENAME );
	return scores_filename;
}

#ifndef D2_OEM
#define COOL_SAYING TXT_REGISTER_DESCENT
#else
#define COOL_SAYING "Get all 30 levels of D2 from 1-800-INTERPLAY"
#endif

void scores_read()
{
	PHYSFS_file *fp;
	int fsize;

	// clear score array...
	memset( &Scores, 0, sizeof(all_scores) );

	fp = PHYSFS_openRead(get_scores_filename());
	if (fp==NULL) {
		int i;

	 	// No error message needed, code will work without a scores file
		sprintf( Scores.cool_saying, COOL_SAYING );
		sprintf( Scores.stats[0].name, "Parallax" );
		sprintf( Scores.stats[1].name, "Matt" );
		sprintf( Scores.stats[2].name, "Mike" );
		sprintf( Scores.stats[3].name, "Adam" );
		sprintf( Scores.stats[4].name, "Mark" );
		sprintf( Scores.stats[5].name, "Jasen" );
		sprintf( Scores.stats[6].name, "Samir" );
		sprintf( Scores.stats[7].name, "Doug" );
		sprintf( Scores.stats[8].name, "Dan" );
		sprintf( Scores.stats[9].name, "Jason" );

		for (i=0; i<10; i++)
			Scores.stats[i].score = (10-i)*1000;
		return;
	}

	fsize = PHYSFS_fileLength(fp);

	if ( fsize != sizeof(all_scores) )	{
		PHYSFS_close(fp);
		return;
	}
	// Read 'em in...
	PHYSFS_read(fp, &Scores, sizeof(all_scores), 1);
	PHYSFS_close(fp);

	if ( (Scores.version!=VERSION_NUMBER)||(Scores.signature[0]!='D')||(Scores.signature[1]!='H')||(Scores.signature[2]!='S') )	{
		memset( &Scores, 0, sizeof(all_scores) );
		return;
	}
}

void scores_write()
{
	PHYSFS_file *fp;

	fp = PHYSFS_openWrite(get_scores_filename());
	if (fp==NULL) {
		nm_messagebox( TXT_WARNING, 1, TXT_OK, "%s\n'%s'", TXT_UNABLE_TO_OPEN, get_scores_filename()  );
		return;
	}

	Scores.signature[0]='D';
	Scores.signature[1]='H';
	Scores.signature[2]='S';
	Scores.version = VERSION_NUMBER;
	PHYSFS_write(fp, &Scores,sizeof(all_scores), 1);
	PHYSFS_close(fp);
}

void int_to_string( int number, char *dest )
{
	int i,l,c;
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
	for (i=l-1; i>=0; i-- )	{
		if (c==3) {
			*p++=',';
			c = 0;
		}
		c++;
		*p++ = buffer[i];
	}
	*p++ = '\0';
	strrev(dest);
}

void scores_fill_struct(stats_info * stats)
{
	strcpy( stats->name, Players[Player_num].callsign );
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

void scores_maybe_add_player(int abort_flag)
{
	char text1[COOL_MESSAGE_LEN+10];
	newmenu_item m[10];
	int i,position;

	if ((Game_mode & GM_MULTI) && !(Game_mode & GM_MULTI_COOP))
		return;
  
	scores_read();
	
	position = MAX_HIGH_SCORES;
	for (i=0; i<MAX_HIGH_SCORES; i++ )	{
		if ( Players[Player_num].score > Scores.stats[i].score )	{
			position = i;
			break;
		}
	}
	
	if ( position == MAX_HIGH_SCORES ) {
		if (abort_flag)
			return;
		scores_fill_struct( &Last_game );
	} else {
		if ( position==0 )	{
			strcpy( text1,  "" );
			m[0].type = NM_TYPE_TEXT; m[0].text = TXT_COOL_SAYING;
			m[1].type = NM_TYPE_INPUT; m[1].text = text1; m[1].text_len = COOL_MESSAGE_LEN-5;
			newmenu_do( TXT_HIGH_SCORE, TXT_YOU_PLACED_1ST, 2, m, NULL, NULL );
			strncpy( Scores.cool_saying, text1, COOL_MESSAGE_LEN );
			if (strlen(Scores.cool_saying)<1)
				sprintf( Scores.cool_saying, "No Comment" );
		} else {
			nm_messagebox( TXT_HIGH_SCORE, 1, TXT_OK, "%s %s!", TXT_YOU_PLACED, *(&TXT_1ST + position) );
		}
	
		// move everyone down...
		for ( i=MAX_HIGH_SCORES-1; i>position; i-- )	{
			Scores.stats[i] = Scores.stats[i-1];
		}

		scores_fill_struct( &Scores.stats[position] );
	
		scores_write();

	}
	scores_view(position);
}

void scores_rprintf(int x, int y, char * format, ... )
{
	va_list args;
	char buffer[128];
	int w, h, aw;
	char *p;

	va_start(args, format );
	vsprintf(buffer,format,args);
	va_end(args);

	//replace the digit '1' with special wider 1
	for (p=buffer;*p;p++)
		if (*p=='1') *p=132;

	gr_get_string_size(buffer, &w, &h, &aw );

	gr_string( FSPACX(x)-w, FSPACY(y), buffer );
}


void scores_draw_item( int i, stats_info * stats )
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

	if (strlen(stats->name)==0) {
		gr_printf( FSPACX(66), FSPACY(y), TXT_EMPTY );
		return;
	}
	gr_printf( FSPACX(66), FSPACY(y), "%s", stats->name );
	int_to_string(stats->score, buffer);
	scores_rprintf( 149, y, "%s", buffer );

	gr_printf( FSPACX(166), FSPACY(y), "%s", MENU_DIFFICULTY_TEXT(stats->diff_level) );

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

typedef struct scores_menu
{
	int citem;
	fix t1;
	int looper;
} scores_menu;

int scores_handler(window *wind, d_event *event, scores_menu *menu)
{
	int i;
	int k;
	sbyte fades[64] = { 1,1,1,2,2,3,4,4,5,6,8,9,10,12,13,15,16,17,19,20,22,23,24,26,27,28,28,29,30,30,31,31,31,31,31,30,30,29,28,28,27,26,24,23,22,20,19,17,16,15,13,12,10,9,8,6,5,4,4,3,2,2,1,1 };
	int w = FSPACX(290), h = FSPACY(170);

	switch (event->type)
	{
		case EVENT_KEY_COMMAND:
			k = ((d_event_keycommand *)event)->keycode;
			switch( k )	{
				case KEY_CTRLED+KEY_R:		
					if ( menu->citem < 0 )		{
						// Reset scores...
						if ( nm_messagebox( NULL, 2,  TXT_NO, TXT_YES, TXT_RESET_HIGH_SCORES )==1 )	{
							PHYSFS_delete(get_scores_filename());
							scores_view(menu->citem);	// create new scores window
							window_close(wind);			// then remove the old one
						}
					}
					break;
					case KEY_BACKSP:				Int3(); k = 0; break;
					case KEY_PRINT_SCREEN:		save_screen_shot(0); k = 0; break;
					
					case KEY_ENTER:
					case KEY_SPACEBAR:
					case KEY_ESC:
					window_close(wind);
					break;
			}
			break;
			
		case EVENT_IDLE:
			timer_delay2(50);

			//see if redbook song needs to be restarted
			RBACheckFinishedHook();
			break;

		case EVENT_WINDOW_DRAW:
			gr_set_current_canvas(NULL);
#ifdef OGL
			nm_draw_background1(Menu_pcx_name);
#endif
			nm_draw_background(((SWIDTH-w)/2)-BORDERX,((SHEIGHT-h)/2)-BORDERY,((SWIDTH-w)/2)+w+BORDERX,((SHEIGHT-h)/2)+h+BORDERY);
			
			gr_set_current_canvas(window_get_canvas(wind));
			
			grd_curcanv->cv_font = MEDIUM3_FONT;
			
			gr_string( 0x8000, FSPACY(15), TXT_HIGH_SCORES );
			
			grd_curcanv->cv_font = GAME_FONT;
			
			gr_set_fontcolor( BM_XRGB(31,26,5), -1 );
			gr_string( FSPACX( 71), FSPACY(50), TXT_NAME );
			gr_string( FSPACX(122), FSPACY(50), TXT_SCORE );
			gr_string( FSPACX(167), FSPACY(50), TXT_SKILL );
			gr_string( FSPACX(210), FSPACY(50), TXT_LEVELS );
			gr_string( FSPACX(253), FSPACY(50), TXT_TIME );
			
			if ( menu->citem < 0 )	
				gr_string( 0x8000, FSPACY(175), TXT_PRESS_CTRL_R );
			
			gr_set_fontcolor( BM_XRGB(28,28,28), -1 );
			
			gr_printf( 0x8000, FSPACY(31), "%c%s%c  - %s", 34, Scores.cool_saying, 34, Scores.stats[0].name );
			
			for (i=0; i<MAX_HIGH_SCORES; i++ )		{
				gr_set_fontcolor( BM_XRGB(28-i*2,28-i*2,28-i*2), -1 );
				scores_draw_item( i, &Scores.stats[i] );
			}
			
			if ( menu->citem > -1 )	{
				
				gr_set_fontcolor( BM_XRGB(7+fades[menu->looper],7+fades[menu->looper],7+fades[menu->looper]), -1 );
				if (timer_get_fixed_seconds() >= menu->t1+F1_0/128)
				{
					menu->t1 = timer_get_fixed_seconds();
					menu->looper++;
					if (menu->looper>63) menu->looper=0;
				}

				if ( menu->citem ==  MAX_HIGH_SCORES )
					scores_draw_item( MAX_HIGH_SCORES, &Last_game );
				else
					scores_draw_item( menu->citem, &Scores.stats[menu->citem] );
			}
			gr_set_current_canvas(NULL);
			break;
			
		case EVENT_WINDOW_CLOSE:
			d_free(menu);
			break;

		default:
			break;
	}
	
	return 0;
}

void scores_view(int citem)
{
	scores_menu *menu;

	MALLOC(menu, scores_menu, 1);
	if (!menu)
		return;

	menu->citem = citem;
	menu->t1 = timer_get_fixed_seconds();
	menu->looper = 0;

	newmenu_close();

	scores_read();

	set_screen_mode(SCREEN_MENU);

	window_create(&grd_curscreen->sc_canvas, (SWIDTH - FSPACX(320))/2, (SHEIGHT - FSPACY(200))/2, FSPACX(320), FSPACY(200),
				  (int (*)(window *, d_event *, void *))scores_handler, menu);
}
