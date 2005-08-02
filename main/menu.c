/* $Id: menu.c,v 1.44 2005-08-02 06:13:56 chris Exp $ */
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
 * Inferno main menu.
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdio.h>
#include <string.h>

#include "menu.h"
#include "inferno.h"
#include "game.h"
#include "gr.h"
#include "key.h"
#include "iff.h"
#include "u_mem.h"
#include "error.h"
#include "bm.h"
#include "screens.h"
#include "mono.h"
#include "joy.h"
#include "vecmat.h"
#include "effects.h"
#include "slew.h"
#include "gamemine.h"
#include "gamesave.h"
#include "palette.h"
#include "args.h"
#include "newdemo.h"
#include "timer.h"
#include "sounds.h"
#include "gameseq.h"
#include "text.h"
#include "gamefont.h"
#include "newmenu.h"
#ifdef NETWORK
#  include "network.h"
#  include "ipx.h"
#  include "multi.h"
#endif
#include "scores.h"
#include "joydefs.h"
#ifdef NETWORK
#include "modem.h"
#endif
#include "playsave.h"
#include "kconfig.h"
#include "titles.h"
#include "credits.h"
#include "texmap.h"
#include "polyobj.h"
#include "state.h"
#include "mission.h"
#include "songs.h"
#include "config.h"
#include "movie.h"
#include "gamepal.h"
#include "gauges.h"
#include "powerup.h"
#include "strutil.h"
#include "reorder.h"

#ifdef MACINTOSH
	#include "resource.h"
	#include "isp.h"
	#include <Dialogs.h>
#endif

#ifdef EDITOR
#include "editor/editor.h"
#endif

//char *menu_difficulty_text[] = { "Trainee", "Rookie", "Fighter", "Hotshot", "Insane" };
//char *menu_detail_text[] = { "Lowest", "Low", "Medium", "High", "Highest", "", "Custom..." };

#define MENU_NEW_GAME                   0
#define MENU_GAME                       1 
#define MENU_EDITOR                     2
#define MENU_VIEW_SCORES                3
#define MENU_QUIT                       4
#define MENU_LOAD_GAME                  5
#define MENU_SAVE_GAME                  6
#define MENU_DEMO_PLAY                  8
#define MENU_LOAD_LEVEL                 9
#define MENU_START_IPX_NETGAME          10
#define MENU_JOIN_IPX_NETGAME           11
#define MENU_CONFIG                     13
#define MENU_REJOIN_NETGAME             14
#define MENU_DIFFICULTY                 15
#define MENU_START_SERIAL               18
#define MENU_HELP                       19
#define MENU_NEW_PLAYER                 20
#define MENU_MULTIPLAYER                21
#define MENU_STOP_MODEM                 22
#define MENU_SHOW_CREDITS               23
#define MENU_ORDER_INFO                 24
#define MENU_PLAY_SONG                  25
//#define MENU_START_TCP_NETGAME          26 // TCP/IP support was planned in Descent II,
//#define MENU_JOIN_TCP_NETGAME           27 // but never realized.
#define MENU_START_APPLETALK_NETGAME    28
#define MENU_JOIN_APPLETALK_NETGAME     29
#define MENU_START_UDP_NETGAME          30 // UDP/IP support copied from d1x
#define MENU_JOIN_UDP_NETGAME           31
#define MENU_START_KALI_NETGAME         32 // Kali support copied from d1x
#define MENU_JOIN_KALI_NETGAME          33
#define MENU_START_MCAST4_NETGAME       34 // UDP/IP over multicast networks
#define MENU_JOIN_MCAST4_NETGAME        35

//ADD_ITEM("Start netgame...", MENU_START_NETGAME, -1 );
//ADD_ITEM("Send net message...", MENU_SEND_NET_MESSAGE, -1 );

#define ADD_ITEM(t,value,key)  do { m[num_options].type=NM_TYPE_MENU; m[num_options].text=t; menu_choice[num_options]=value;num_options++; } while (0)

//unused - extern int last_joy_time;               //last time the joystick was used
#ifndef NDEBUG
extern int Speedtest_on;
#else
#define Speedtest_on 0
#endif

void do_sound_menu();
void do_toggles_menu();

ubyte do_auto_demo = 1;                 // Flag used to enable auto demo starting in main menu.
int Player_default_difficulty; // Last difficulty level chosen by the player
int Auto_leveling_on = 1;
int Guided_in_big_window = 0;
int Menu_draw_copyright = 0;
int EscortHotKeys=1;

// Function Prototypes added after LINTING
void do_option(int select);
void do_detail_level_menu_custom(void);
void do_new_game_menu(void);
#ifdef NETWORK
void do_multi_player_menu(void);
void ipx_set_driver(int ipx_driver);
#endif //NETWORK

//returns the number of demo files on the disk
int newdemo_count_demos();
extern ubyte Version_major,Version_minor;

// ------------------------------------------------------------------------
void autodemo_menu_check(int nitems, newmenu_item * items, int *last_key, int citem )
{
	int curtime;

	nitems = nitems;
	items=items;
	citem = citem;

	//draw copyright message
	if ( Menu_draw_copyright )              {
		int w,h,aw;

		Menu_draw_copyright = 0;
		gr_set_current_canvas(NULL);
		gr_set_curfont(GAME_FONT);
		gr_set_fontcolor(BM_XRGB(6,6,6),-1);

		gr_get_string_size("V2.2", &w, &h, &aw );
	
			gr_printf(0x8000,grd_curcanv->cv_bitmap.bm_h-GAME_FONT->ft_h-2,TXT_COPYRIGHT);
			#ifdef MACINTOSH	// print out fix level as well if it exists
				if (Version_fix != 0)
				{
					gr_get_string_size("V2.2.2", &w, &h, &aw );
					gr_printf(grd_curcanv->cv_bitmap.bm_w-w-2,
							  grd_curcanv->cv_bitmap.bm_h-GAME_FONT->ft_h-2,
							  "V%d.%d.%d",
							  Version_major,Version_minor,Version_fix);
				}
				else
				{
					gr_printf(grd_curcanv->cv_bitmap.bm_w-w-2,
							  grd_curcanv->cv_bitmap.bm_h-GAME_FONT->ft_h-2,
							  "V%d.%d",
							  Version_major,Version_minor);
				}
			#else
				gr_printf(grd_curcanv->cv_bitmap.bm_w-w-2,grd_curcanv->cv_bitmap.bm_h-GAME_FONT->ft_h-2,"V%d.%d",Version_major,Version_minor);
			#endif

		//say this is vertigo version
		if (cfexist(MISSION_DIR "d2x.hog")) {
			gr_set_curfont(MEDIUM2_FONT);
			gr_printf(MenuHires?495:248, MenuHires?88:37, "Vertigo");
		}
	}
	
	// Don't allow them to hit ESC in the main menu.
	if (*last_key==KEY_ESC) *last_key = 0;

	if ( do_auto_demo )     {
		curtime = timer_get_approx_seconds();
		//if ( ((keyd_time_when_last_pressed+i2f(20)) < curtime) && ((last_joy_time+i2f(20)) < curtime) && (!Speedtest_on)  ) {
		#ifndef MACINTOSH		// for now only!!!!
		if ( ((keyd_time_when_last_pressed+i2f(25)) < curtime) && (!Speedtest_on)  ) {
		#else
		if ( (keyd_time_when_last_pressed+i2f(40)) < curtime ) {
		#endif
			int n_demos;

			n_demos = newdemo_count_demos();

try_again:;

			if ((d_rand() % (n_demos+1)) == 0)
			{
				#ifndef SHAREWARE
#ifdef OGL
					Screen_mode = -1;
#endif
					PlayMovie("intro.mve",0);
					songs_play_song(SONG_TITLE,1);
					*last_key = -3; //exit menu to force redraw even if not going to game mode. -3 tells menu system not to restore
					set_screen_mode(SCREEN_MENU);
				#endif // end of ifndef shareware
			}
			else {
				WIN(HideCursorW());
				keyd_time_when_last_pressed = curtime;                  // Reset timer so that disk won't thrash if no demos.
				newdemo_start_playback(NULL);           // Randomly pick a file
				if (Newdemo_state == ND_STATE_PLAYBACK) {
					Function_mode = FMODE_GAME;
					*last_key = -3; //exit menu to get into game mode. -3 tells menu system not to restore
				}
				else
					goto try_again;	//keep trying until we get a demo that works
			}
		}
	}
}

//static int First_time = 1;
static int main_menu_choice = 0;

//      -----------------------------------------------------------------------------
//      Create the main menu.
void create_main_menu(newmenu_item *m, int *menu_choice, int *callers_num_options)
{
	int     num_options;

	#ifndef DEMO_ONLY
	num_options = 0;

	set_screen_mode (SCREEN_MENU);

	ADD_ITEM(TXT_NEW_GAME,MENU_NEW_GAME,KEY_N);

	ADD_ITEM(TXT_LOAD_GAME,MENU_LOAD_GAME,KEY_L);

#ifdef NETWORK
	ADD_ITEM(TXT_MULTIPLAYER_,MENU_MULTIPLAYER,-1);
#endif

	ADD_ITEM(TXT_OPTIONS_, MENU_CONFIG, -1 );
	ADD_ITEM(TXT_CHANGE_PILOTS,MENU_NEW_PLAYER,unused);
	ADD_ITEM(TXT_VIEW_DEMO,MENU_DEMO_PLAY,0);
	ADD_ITEM(TXT_VIEW_SCORES,MENU_VIEW_SCORES,KEY_V);
	if (cfexist("orderd2.pcx")) /* SHAREWARE */
		ADD_ITEM(TXT_ORDERING_INFO,MENU_ORDER_INFO,-1);
	ADD_ITEM(TXT_CREDITS,MENU_SHOW_CREDITS,-1);
	#endif
	ADD_ITEM(TXT_QUIT,MENU_QUIT,KEY_Q);

	#ifndef RELEASE
	if (!(Game_mode & GM_MULTI ))   {
		//m[num_options].type=NM_TYPE_TEXT;
		//m[num_options++].text=" Debug options:";

		ADD_ITEM("  Load level...",MENU_LOAD_LEVEL ,KEY_N);
		#ifdef EDITOR
		ADD_ITEM("  Editor", MENU_EDITOR, KEY_E);
		#endif
	}

	//ADD_ITEM( "  Play song", MENU_PLAY_SONG, -1 );
	#endif

	*callers_num_options = num_options;
}

//returns number of item chosen
int DoMenu() 
{
	int menu_choice[25];
	newmenu_item m[25];
	int num_options = 0;

	load_palette(MENU_PALETTE,0,1);		//get correct palette

	if ( Players[Player_num].callsign[0]==0 )       {
		RegisterPlayer();
		return 0;
	}
	
	if ((Game_mode & GM_SERIAL) || (Game_mode & GM_MODEM)) {
		do_option(MENU_START_SERIAL);
		return 0;
	}

	do {
		create_main_menu(m, menu_choice, &num_options); // may have to change, eg, maybe selected pilot and no save games.

		keyd_time_when_last_pressed = timer_get_fixed_seconds();                // .. 20 seconds from now!
		if (main_menu_choice < 0 )
			main_menu_choice = 0;
		Menu_draw_copyright = 1;
		main_menu_choice = newmenu_do2( "", NULL, num_options, m, autodemo_menu_check, main_menu_choice, Menu_pcx_name);
		if ( main_menu_choice > -1 ) do_option(menu_choice[main_menu_choice]);
	} while( Function_mode==FMODE_MENU );

//      if (main_menu_choice != -2)
//              do_auto_demo = 0;               // No more auto demos
	if ( Function_mode==FMODE_GAME )
		gr_palette_fade_out( gr_palette, 32, 0 );

	return main_menu_choice;
}

extern void show_order_form(void);      // John didn't want this in inferno.h so I just externed it.

//returns flag, true means quit menu
void do_option ( int select) 
{
	switch (select) {
		case MENU_NEW_GAME:
			do_new_game_menu();
			break;
		case MENU_GAME:
			break;
		case MENU_DEMO_PLAY:
		{
			char demo_file[16];
			if (newmenu_get_filename(TXT_SELECT_DEMO, "dem", demo_file, 1))
				newdemo_start_playback(demo_file);
			break;
		}
		case MENU_LOAD_GAME:
			state_restore_all(0, 0, NULL);
			break;
		#ifdef EDITOR
		case MENU_EDITOR:
			Function_mode = FMODE_EDITOR;
			init_cockpit();
			break;
		#endif
		case MENU_VIEW_SCORES:
			gr_palette_fade_out( gr_palette,32,0 );
			scores_view(-1);
			break;
#if 1 //def SHAREWARE
		case MENU_ORDER_INFO:
			show_order_form();
			break;
#endif
		case MENU_QUIT:
			#ifdef EDITOR
			if (! SafetyCheck()) break;
			#endif
			gr_palette_fade_out( gr_palette,32,0);
			Function_mode = FMODE_EXIT;
			break;
		case MENU_NEW_PLAYER:
			RegisterPlayer();               //1 == allow escape out of menu
			break;

		case MENU_HELP:
			do_show_help();
			break;

#ifndef RELEASE

		case MENU_PLAY_SONG:    {
				int i;
				char * m[MAX_NUM_SONGS];

				for (i=0;i<Num_songs;i++) {
					m[i] = Songs[i].filename;
				}
				i = newmenu_listbox( "Select Song", Num_songs, m, 1, NULL );

				if ( i > -1 )   {
					songs_play_song( i, 0 );
				}
			}
			break;
	case MENU_LOAD_LEVEL:
		if (Current_mission || select_mission(0, "Load Level\n\nSelect mission"))
		{
			newmenu_item m;
			char text[10]="";
			int new_level_num;

			m.type=NM_TYPE_INPUT; m.text_len = 10; m.text = text;

			newmenu_do( NULL, "Enter level to load", 1, &m, NULL );

			new_level_num = atoi(m.text);

			if (new_level_num!=0 && new_level_num>=Last_secret_level && new_level_num<=Last_level)  {
				gr_palette_fade_out( gr_palette, 32, 0 );
				StartNewGame(new_level_num);
			}
		}
		break;

#endif //ifndef RELEASE


#ifdef NETWORK
		//case MENU_START_TCP_NETGAME:
		//case MENU_JOIN_TCP_NETGAME:
		case MENU_START_IPX_NETGAME:
		case MENU_JOIN_IPX_NETGAME:
		case MENU_START_UDP_NETGAME:
		case MENU_JOIN_UDP_NETGAME:
		case MENU_START_KALI_NETGAME:
		case MENU_JOIN_KALI_NETGAME:
		case MENU_START_MCAST4_NETGAME:
		case MENU_JOIN_MCAST4_NETGAME:
//			load_mission(Builtin_mission_num);
#ifdef MACINTOSH
			Network_game_type = IPX_GAME;
#endif
//			WIN(ipx_create_read_thread());
			switch (select & ~0x1) {
			case MENU_START_IPX_NETGAME: ipx_set_driver(IPX_DRIVER_IPX); break;
			case MENU_START_UDP_NETGAME: ipx_set_driver(IPX_DRIVER_UDP); break;
			case MENU_START_KALI_NETGAME: ipx_set_driver(IPX_DRIVER_KALI); break;
			case MENU_START_MCAST4_NETGAME: ipx_set_driver(IPX_DRIVER_MCAST4); break;
			default: Int3();
			}

			if ((select & 0x1) == 0) // MENU_START_*_NETGAME
				network_start_game();
			else // MENU_JOIN_*_NETGAME
				network_join_game();
			break;

#ifdef MACINTOSH
		case MENU_START_APPLETALK_NETGAME:
//			load_mission(Builtin_mission_num);
			#ifdef MACINTOSH
			Network_game_type = APPLETALK_GAME;
			#endif
			network_start_game();
			break;

		case MENU_JOIN_APPLETALK_NETGAME:
//			load_mission(Builtin_mission_num);
			#ifdef MACINTOSH
			Network_game_type = APPLETALK_GAME;
			#endif
			network_join_game();
			break;
#endif
#if 0
		case MENU_START_TCP_NETGAME:
		case MENU_JOIN_TCP_NETGAME:
			nm_messagebox (TXT_SORRY,1,TXT_OK,"Not available in shareware version!");
			// DoNewIPAddress();
			break;
#endif
		case MENU_START_SERIAL:
			com_main_menu();
			break;
		case MENU_MULTIPLAYER:
			do_multi_player_menu();
			break;
#endif //NETWORK
		case MENU_CONFIG:
			do_options_menu();
			break;
		case MENU_SHOW_CREDITS:
			gr_palette_fade_out( gr_palette,32,0);
			songs_stop_all();
			credits_show(NULL); 
			break;
		default:
			Error("Unknown option %d in do_option",select);
			break;
	}

}

int do_difficulty_menu()
{
	int s;
	newmenu_item m[5];

	m[0].type=NM_TYPE_MENU; m[0].text=MENU_DIFFICULTY_TEXT(0);
	m[1].type=NM_TYPE_MENU; m[1].text=MENU_DIFFICULTY_TEXT(1);
	m[2].type=NM_TYPE_MENU; m[2].text=MENU_DIFFICULTY_TEXT(2);
	m[3].type=NM_TYPE_MENU; m[3].text=MENU_DIFFICULTY_TEXT(3);
	m[4].type=NM_TYPE_MENU; m[4].text=MENU_DIFFICULTY_TEXT(4);

	s = newmenu_do1( NULL, TXT_DIFFICULTY_LEVEL, NDL, m, NULL, Difficulty_level);

	if (s > -1 )    {
		if (s != Difficulty_level)
		{       
			Player_default_difficulty = s;
			write_player_file();
		}
		Difficulty_level = s;
		mprintf((0, "%s %s %i\n", TXT_DIFFICULTY_LEVEL, TXT_SET_TO, Difficulty_level));
		return 1;
	}
	return 0;
}

int     Max_debris_objects, Max_objects_onscreen_detailed;
int     Max_linear_depth_objects;

sbyte   Object_complexity=2, Object_detail=2;
sbyte   Wall_detail=2, Wall_render_depth=2, Debris_amount=2, SoundChannels = 2;

sbyte   Render_depths[NUM_DETAIL_LEVELS-1] =                        { 6,  9, 12, 15, 50};
sbyte   Max_perspective_depths[NUM_DETAIL_LEVELS-1] =               { 1,  2,  3,  5,  8};
sbyte   Max_linear_depths[NUM_DETAIL_LEVELS-1] =                    { 3,  5,  7, 10, 50};
sbyte   Max_linear_depths_objects[NUM_DETAIL_LEVELS-1] =            { 1,  2,  3,  7, 20};
sbyte   Max_debris_objects_list[NUM_DETAIL_LEVELS-1] =              { 2,  4,  7, 10, 15};
sbyte   Max_objects_onscreen_detailed_list[NUM_DETAIL_LEVELS-1] =   { 2,  4,  7, 10, 15};
sbyte   Smts_list[NUM_DETAIL_LEVELS-1] =                            { 2,  4,  8, 16, 50};   //      threshold for models to go to lower detail model, gets multiplied by obj->size
sbyte   Max_sound_channels[NUM_DETAIL_LEVELS-1] =                   { 2,  4,  8, 12, 16};

//      -----------------------------------------------------------------------------
//      Set detail level based stuff.
//      Note: Highest detail level (detail_level == NUM_DETAIL_LEVELS-1) is custom detail level.
void set_detail_level_parameters(int detail_level)
{
	Assert((detail_level >= 0) && (detail_level < NUM_DETAIL_LEVELS));

	if (detail_level < NUM_DETAIL_LEVELS-1) {
		Render_depth = Render_depths[detail_level];
		Max_perspective_depth = Max_perspective_depths[detail_level];
		Max_linear_depth = Max_linear_depths[detail_level];
		Max_linear_depth_objects = Max_linear_depths_objects[detail_level];

		Max_debris_objects = Max_debris_objects_list[detail_level];
		Max_objects_onscreen_detailed = Max_objects_onscreen_detailed_list[detail_level];

		Simple_model_threshhold_scale = Smts_list[detail_level];

		digi_set_max_channels( Max_sound_channels[ detail_level ] );

		//      Set custom menu defaults.
		Object_complexity = detail_level;
		Wall_render_depth = detail_level;
		Object_detail = detail_level;
		Wall_detail = detail_level;
		Debris_amount = detail_level;
		SoundChannels = detail_level;
	}
}

//      -----------------------------------------------------------------------------
void do_detail_level_menu(void)
{
	int s;
	newmenu_item m[8];

	m[0].type=NM_TYPE_MENU; m[0].text=MENU_DETAIL_TEXT(0);
	m[1].type=NM_TYPE_MENU; m[1].text=MENU_DETAIL_TEXT(1);
	m[2].type=NM_TYPE_MENU; m[2].text=MENU_DETAIL_TEXT(2);
	m[3].type=NM_TYPE_MENU; m[3].text=MENU_DETAIL_TEXT(3);
	m[4].type=NM_TYPE_MENU; m[4].text=MENU_DETAIL_TEXT(4);
	m[5].type=NM_TYPE_TEXT; m[5].text="";
	m[6].type=NM_TYPE_MENU; m[6].text=MENU_DETAIL_TEXT(5);
	m[7].type=NM_TYPE_CHECK; m[7].text="Show High Res movies"; m[7].value=MovieHires;

	s = newmenu_do1( NULL, TXT_DETAIL_LEVEL , NDL+3, m, NULL, Detail_level);

	if (s > -1 )    {
		switch (s)      {
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
				Detail_level = s;
				mprintf((0, "Detail level set to %i\n", Detail_level));
				set_detail_level_parameters(Detail_level);
				break;
			case 6:
				Detail_level = 5;
				do_detail_level_menu_custom();
				break;
		}
	}
	MovieHires = m[7].value;
}

//      -----------------------------------------------------------------------------
void do_detail_level_menu_custom_menuset(int nitems, newmenu_item * items, int *last_key, int citem )
{
	nitems = nitems;
	*last_key = *last_key;
	citem = citem;

	Object_complexity = items[0].value;
	Object_detail = items[1].value;
	Wall_detail = items[2].value;
	Wall_render_depth = items[3].value;
	Debris_amount = items[4].value;
	SoundChannels = items[5].value;
}

void set_custom_detail_vars(void)
{
	Render_depth = Render_depths[Wall_render_depth];

	Max_perspective_depth = Max_perspective_depths[Wall_detail];
	Max_linear_depth = Max_linear_depths[Wall_detail];

	Max_debris_objects = Max_debris_objects_list[Debris_amount];

	Max_objects_onscreen_detailed = Max_objects_onscreen_detailed_list[Object_complexity];
	Simple_model_threshhold_scale = Smts_list[Object_complexity];
	Max_linear_depth_objects = Max_linear_depths_objects[Object_detail];

	digi_set_max_channels( Max_sound_channels[ SoundChannels ] );
	
}

#define	DL_MAX	10

//      -----------------------------------------------------------------------------

void do_detail_level_menu_custom(void)
{
	int	count;
	int	s=0;
	newmenu_item m[DL_MAX];

	do {
		count = 0;
		m[count].type = NM_TYPE_SLIDER;
		m[count].text = TXT_OBJ_COMPLEXITY;
		m[count].value = Object_complexity;
		m[count].min_value = 0;
		m[count++].max_value = NDL-1;

		m[count].type = NM_TYPE_SLIDER;
		m[count].text = TXT_OBJ_DETAIL;
		m[count].value = Object_detail;
		m[count].min_value = 0;
		m[count++].max_value = NDL-1;

		m[count].type = NM_TYPE_SLIDER;
		m[count].text = TXT_WALL_DETAIL;
		m[count].value = Wall_detail;
		m[count].min_value = 0;
		m[count++].max_value = NDL-1;

		m[count].type = NM_TYPE_SLIDER;
		m[count].text = TXT_WALL_RENDER_DEPTH;
		m[count].value = Wall_render_depth;
		m[count].min_value = 0;
		m[count++].max_value = NDL-1;

		m[count].type = NM_TYPE_SLIDER;
		m[count].text= TXT_DEBRIS_AMOUNT;
		m[count].value = Debris_amount;
		m[count].min_value = 0;
		m[count++].max_value = NDL-1;

		m[count].type = NM_TYPE_SLIDER;
		m[count].text= TXT_SOUND_CHANNELS;
		m[count].value = SoundChannels;
		m[count].min_value = 0;
		m[count++].max_value = NDL-1;

		m[count].type = NM_TYPE_TEXT;
		m[count++].text= TXT_LO_HI;

		Assert(count < DL_MAX);

		s = newmenu_do1( NULL, TXT_DETAIL_CUSTOM, count, m, do_detail_level_menu_custom_menuset, s);
	} while (s > -1);

	set_custom_detail_vars();
}

#ifndef MACINTOSH
int Default_display_mode=0;
int Current_display_mode=0;
#else
int Default_display_mode=1;
int Current_display_mode=1;
#endif

extern int MenuHiresAvailable;

typedef struct {
	int	VGA_mode;
	short	w,h;
	short	render_method;
	short	flags;
} dmi;

dmi display_mode_info[7] = {
			{SM(320,200),	 320,	200, VR_NONE, VRF_ALLOW_COCKPIT+VRF_COMPATIBLE_MENUS}, 
			{SM(640,480),	 640, 480, VR_NONE, VRF_COMPATIBLE_MENUS+VRF_ALLOW_COCKPIT},
			{SM(320,400),	 320, 400, VR_NONE, VRF_USE_PAGING},
			{SM(640,400),	 640, 400, VR_NONE, VRF_COMPATIBLE_MENUS}, 
			{SM(800,600),	 800, 600, VR_NONE, VRF_COMPATIBLE_MENUS}, 
			{SM(1024,768),	1024,	768, VR_NONE, VRF_COMPATIBLE_MENUS}, 	
			{SM(1280,1024),1280,1024, VR_NONE, VRF_COMPATIBLE_MENUS}, 
};


void set_display_mode(int mode)
{
	dmi *dmi;

	if ((Current_display_mode == -1)||(VR_render_mode != VR_NONE))	//special VR mode
		return;								//...don't change

	if (0) // (mode >= 5 && !FindArg("-superhires"))
		mode = 4;

	if (!MenuHiresAvailable && (mode != 2))
		mode = 0;

	if (gr_check_mode(display_mode_info[mode].VGA_mode) != 0)		//can't do mode
		#ifndef MACINTOSH
		mode = 0;
		#else
		mode = 1;
		#endif

	Current_display_mode = mode;

	dmi = &display_mode_info[mode];

	if (Current_display_mode != -1) {

		game_init_render_buffers(dmi->VGA_mode,dmi->w,dmi->h,dmi->render_method,dmi->flags);
		Default_display_mode = Current_display_mode;
	}

	Screen_mode = -1;		//force screen reset
}

#ifdef MACINTOSH	// use Mac version of do_screen_res_menu

void do_screen_res_menu()
{
	#define N_SCREENRES_ITEMS 6
	
	newmenu_item m[N_SCREENRES_ITEMS];
	int citem, i, n_items, odisplay_mode, result;

	if ((Current_display_mode == -1)||(VR_render_mode != VR_NONE))		//special VR mode
	{				
		nm_messagebox(TXT_SORRY, 1, TXT_OK, 
				"You may not change screen\n"
				"resolution when VR modes enabled.");
		return;
	}

	m[0].type=NM_TYPE_TEXT;	 m[0].value=0; m[0].text="Modes w/ Cockpit:";
	m[1].type=NM_TYPE_RADIO; m[1].value=0; m[1].group=0; m[1].text=" 640x480";
	m[2].type=NM_TYPE_TEXT;	 m[2].value=0; m[2].text="Modes w/o Cockpit:";
	m[3].type=NM_TYPE_RADIO; m[3].value=0; m[3].group=0; m[3].text=" 800x600";
//	m[4].type=NM_TYPE_RADIO; m[4].value=0; m[4].group=0; m[4].text=" 1024x768";
//	m[5].type=NM_TYPE_RADIO; m[5].value=0; m[5].group=0; m[5].text=" 1280x1024";
	n_items = 4;

	odisplay_mode = VGA_current_mode;
	citem = Current_display_mode;
	if (Current_display_mode >= 2)
		citem--;

	if (citem >= n_items)
		citem = n_items-1;

	m[citem].value = 1;

	newmenu_do1( NULL, "Select screen mode", n_items, m, NULL, citem);

	for (i=0;i<n_items;i++)
		if (m[i].value)
			break;
	if (i >= 3)
		i++;

#if 0 //def SHAREWARE
	if (i > 1)
		nm_messagebox(TXT_SORRY, 1, TXT_OK, 
			"High resolution modes are\n"
			"only available in the\n"
			"Commercial version of Descent 2.");
	return;
#else
	result = vga_check_mode(display_mode_info[i].VGA_mode);
	
	if (result) {
		nm_messagebox(TXT_SORRY, 1, TXT_OK, 
				"Cannot set requested\n"
				"mode on this video card.");
		return;
	}
	
	set_display_mode(i);
	reset_cockpit();
#endif

}

#else	// PC version of do_screen_res_menu is below

void do_screen_res_menu()
{
#ifdef GR_SUPPORTS_FULLSCREEN_TOGGLE
#define N_SCREENRES_ITEMS 10
	int fullscreenc;
#else
        #define N_SCREENRES_ITEMS 9
#endif
	newmenu_item m[N_SCREENRES_ITEMS];
	int citem;
	int i;
	int n_items;

	if ((Current_display_mode == -1)||(VR_render_mode != VR_NONE)) {				//special VR mode
		nm_messagebox(TXT_SORRY, 1, TXT_OK, 
				"You may not change screen\n"
				"resolution when VR modes enabled.");
		return;
	}

	m[0].type=NM_TYPE_TEXT;	 m[0].value=0;    			  m[0].text="Modes w/ Cockpit:";
	
	m[1].type=NM_TYPE_RADIO; m[1].value=0; m[1].group=0; m[1].text=" 320x200";
	m[2].type=NM_TYPE_RADIO; m[2].value=0; m[2].group=0; m[2].text=" 640x480";
	m[3].type=NM_TYPE_TEXT;	 m[3].value=0;   				  m[3].text="Modes w/o Cockpit:";
	m[4].type=NM_TYPE_RADIO; m[4].value=0; m[4].group=0; m[4].text=" 320x400";
	m[5].type=NM_TYPE_RADIO; m[5].value=0; m[5].group=0; m[5].text=" 640x400";
	m[6].type=NM_TYPE_RADIO; m[6].value=0; m[6].group=0; m[6].text=" 800x600";
	n_items = 7;
	if (1) { //(FindArg("-superhires")) {
		m[7].type=NM_TYPE_RADIO; m[7].value=0; m[7].group=0; m[7].text=" 1024x768";
		m[8].type=NM_TYPE_RADIO; m[8].value=0; m[8].group=0; m[8].text=" 1280x1024";
		n_items += 2;
	}

#ifdef GR_SUPPORTS_FULLSCREEN_TOGGLE
	m[n_items].type = NM_TYPE_CHECK; m[n_items].text = "Fullscreen";
	m[n_items].value = gr_check_fullscreen();
	fullscreenc=n_items++;
#endif

	citem = Current_display_mode+1;
	
	if (Current_display_mode >= 2)
		citem++;

	if (citem >= n_items)
		citem = n_items-1;

	m[citem].value = 1;

	newmenu_do1( NULL, "Select screen mode", n_items, m, NULL, citem);

#ifdef GR_SUPPORTS_FULLSCREEN_TOGGLE
	if (m[fullscreenc].value != gr_check_fullscreen()){
		gr_toggle_fullscreen();
		Game_screen_mode = -1;
	}
#endif

	for (i=0;i<n_items;i++)
		if (m[i].value)
			break;

	if (i >= 4)
		i--;

	i--;

	if (((i != 0) && (i != 2) && !MenuHiresAvailable) || gr_check_mode(display_mode_info[i].VGA_mode)) {
		nm_messagebox(TXT_SORRY, 1, TXT_OK, 
				"Cannot set requested\n"
				"mode on this video card.");
		return;
	}
#ifdef SHAREWARE
		if (i != 0)
			nm_messagebox(TXT_SORRY, 1, TXT_OK, 
				"High resolution modes are\n"
				"only available in the\n"
				"Commercial version of Descent 2.");
		return;
#else
		if (i != Current_display_mode)
			set_display_mode(i);
#endif

}
#endif	// end of PC version of do_screen_res_menu()



void do_new_game_menu()
{
	int new_level_num,player_highest_level;

    if (!select_mission(0, "New Game\n\nSelect mission"))
        return;
    
	new_level_num = 1;

	player_highest_level = get_highest_level();

	if (player_highest_level > Last_level)
		player_highest_level = Last_level;

	if (player_highest_level > 1) {
		newmenu_item m[4];
		char info_text[80];
		char num_text[10];
		int choice;
		int n_items;

try_again:
		sprintf(info_text,"%s %d",TXT_START_ANY_LEVEL, player_highest_level);

		m[0].type=NM_TYPE_TEXT; m[0].text = info_text;
		m[1].type=NM_TYPE_INPUT; m[1].text_len = 10; m[1].text = num_text;
		n_items = 2;

		#ifdef WINDOWS
		m[2].type = NM_TYPE_TEXT; m[2].text = "";
		m[3].type = NM_TYPE_MENU; m[3].text = "          Ok";
		n_items = 4;
		#endif

		strcpy(num_text,"1");

		choice = newmenu_do( NULL, TXT_SELECT_START_LEV, n_items, m, NULL );

		if (choice==-1 || m[1].text[0]==0)
			return;

		new_level_num = atoi(m[1].text);

		if (!(new_level_num>0 && new_level_num<=player_highest_level)) {
			m[0].text = TXT_ENTER_TO_CONT;
			nm_messagebox( NULL, 1, TXT_OK, TXT_INVALID_LEVEL); 
			goto try_again;
		}
	}

	Difficulty_level = Player_default_difficulty;

	if (!do_difficulty_menu())
		return;

	gr_palette_fade_out( gr_palette, 32, 0 );
	StartNewGame(new_level_num);

}

extern void GameLoop(int, int );

extern int Redbook_enabled;

void options_menuset(int nitems, newmenu_item * items, int *last_key, int citem )
{
	if ( citem==5)
	{
		gr_palette_set_gamma(items[5].value);
	}

	nitems++;		//kill warning
	last_key++;		//kill warning
}


// added on 9/20/98 by Victor Rachels to attempt to add screen res change ingame
// Changed on 3/24/99 by Owen Evans to make it work  =)
void change_res_poll()
{
}

void change_res()
{
	// edited 05/27/99 Matt Mueller - ingame fullscreen changing
	newmenu_item m[11];
	u_int32_t modes[11];
	int i = 0, mc = 0, num_presets = 0;
	char customres[16];
#ifdef GR_SUPPORTS_FULLSCREEN_TOGGLE
	int fullscreenc;
#endif
	//end edit -MM
	u_int32_t screen_mode = 0;
	int screen_width = 0;
	int screen_height = 0;
	int vr_mode = VR_NONE;
	int screen_flags = 0;

	m[mc].type = NM_TYPE_RADIO; m[mc].text = "320x200"; m[mc].value = (Game_screen_mode == SM(320, 200)); m[mc].group = 0; modes[mc] = SM(320, 200); mc++;
	m[mc].type = NM_TYPE_RADIO; m[mc].text = "640x480"; m[mc].value = (Game_screen_mode == SM(640, 480)); m[mc].group = 0; modes[mc] = SM(640, 480); mc++;
	m[mc].type = NM_TYPE_RADIO; m[mc].text = "320x400"; m[mc].value = (Game_screen_mode == SM(320, 400)); m[mc].group = 0; modes[mc] = SM(320, 400); mc++;
	m[mc].type = NM_TYPE_RADIO; m[mc].text = "640x400"; m[mc].value = (Game_screen_mode == SM(640, 400)); m[mc].group = 0; modes[mc] = SM(640, 400); mc++;
	m[mc].type = NM_TYPE_RADIO; m[mc].text = "800x600"; m[mc].value = (Game_screen_mode == SM(800, 600)); m[mc].group = 0; modes[mc] = SM(800, 600); mc++;
	m[mc].type = NM_TYPE_RADIO; m[mc].text = "1024x768"; m[mc].value = (Game_screen_mode == SM(1024, 768)); m[mc].group = 0; modes[mc] = SM(1024, 768); mc++;
	m[mc].type = NM_TYPE_RADIO; m[mc].text = "1280x1024"; m[mc].value = (Game_screen_mode == SM(1280, 1024)); m[mc].group = 0; modes[mc] = SM(1280, 1024); mc++;

	num_presets = mc;
	for (i = 0; i < mc; i++)
		if (m[mc].value)
			break;

	m[mc].type = NM_TYPE_RADIO; m[mc].text = "custom:"; m[mc].value = (i == mc); m[mc].group = 0; modes[mc] = 0; mc++;
	sprintf(customres, "%ix%i", SM_W(Game_screen_mode), SM_H(Game_screen_mode));
	m[mc].type = NM_TYPE_INPUT; m[mc].text = customres; m[mc].text_len = 11; modes[mc] = 0; mc++;

	//m[mc].type = NM_TYPE_CHECK; m[mc].text = "No Doublebuffer"; m[mc].value = use_double_buffer;

	// added 05/27/99 Matt Mueller - ingame fullscreen changing
#ifdef GR_SUPPORTS_FULLSCREEN_TOGGLE
	fullscreenc = mc; m[mc].type = NM_TYPE_CHECK; m[mc].text = "Fullscreen"; m[mc].value = gr_check_fullscreen(); mc++;
#endif
	// end addition -MM


	i = newmenu_do1(NULL, "Screen Resolution", mc, m, &change_res_poll, 0);

	// added 05/27/99 Matt Mueller - ingame fullscreen changing
#ifdef GR_SUPPORTS_FULLSCREEN_TOGGLE
	if (m[fullscreenc].value != gr_check_fullscreen())
	{
		gr_toggle_fullscreen();
        Game_screen_mode = -1;
	}
#endif
	// end addition -MM

	for (i = 0; (m[i].value == 0) && (i < num_presets); i++);

	for(i = 0; (m[i].value == 0) && (i < num_presets); i++);

	if (modes[i]==0)
	{
		char *h = strchr(customres, 'x');
		if (!h)
			return;
		screen_mode = SM(atoi(customres), atoi(h+1));
	}
	else
	{
		screen_mode = modes[i];
	}

	screen_width = SM_W(screen_mode);
	screen_height = SM_H(screen_mode);
	if (screen_height <= 0 || screen_width <= 0)
		return;

	switch (screen_mode)
	{
	case SM(320, 200):
	case SM(640, 480):
		screen_flags = VRF_ALLOW_COCKPIT + VRF_COMPATIBLE_MENUS;
		break;
	default:
		screen_flags = VRF_COMPATIBLE_MENUS;
		break;
	}

#ifdef __MSDOS__
	if (FindArg("-nodoublebuffer"))
#endif
	{
		screen_flags &= ~VRF_USE_PAGING;
	}

	// added 6/15/1999 by Owen Evans to eliminate unneccesary mode modification
	if (Game_screen_mode == screen_mode)
		return;
	//gr_set_mode(Game_screen_mode);
	// end section - OE

	//VR_offscreen_buffer = 0; // Disable VR (so that VR_Screen_mode doesnt mess us up
	Game_screen_mode = screen_mode;
	Game_window_w = screen_width;
	Game_window_h = screen_height;
	game_init_render_buffers(screen_mode, screen_width, screen_height, vr_mode, screen_flags);
}
//End changed section (OE)


//added on 8/18/98 by Victor Rachels to add d1x options menu, maxfps setting
//added/edited on 8/18/98 by Victor Rachels to set maxfps always on, max=80
//added/edited on 9/7/98 by Victor Rachels to attempt dir browsing.  failed.

void d2x_options_menu_poll(int nitems, newmenu_item * menus, int * key, int citem)
{
}


void d2x_options_menu()
{
	newmenu_item m[14];
	int i=0;
	int opt = 0;
	int inputs, commands;
#if 0
	int checks;
#endif

	char smaxfps[4];
#if 0
	char shudmaxnumdisp[4];
	char thogdir[64];
	extern int gr_message_color_level;

	sprintf(thogdir,AltHogDir);
#endif
	sprintf(smaxfps,"%d",maxfps);
#if 0
	sprintf(shudmaxnumdisp,"%d",HUD_max_num_disp);

	m[opt].type = NM_TYPE_MENU;  m[opt].text = "Primary autoselect ordering...";   opt++;
	m[opt].type = NM_TYPE_MENU;  m[opt].text = "Secondary autoselect ordering..."; opt++;
#endif

#ifdef D2X_KEYS
	//added on 2/4/99 by Victor Rachels for new key menu
	m[opt].type = NM_TYPE_MENU;  m[opt].text = "D2X Keys"; opt++;
	//end this section addition - VR
#endif

	// enabled 3/24/99 - Owen Evans
	m[opt].type = NM_TYPE_MENU; m[opt].text = "Change Screen Resolution"; opt++;
	// end enabled stuff - OE

	commands=opt;
#if 0
	//added on 2/2/99 by Victor Rachels for bans
#ifdef NETWORK
	m[opt].type = NM_TYPE_MENU; m[opt].text = "Save bans now"; opt++;
#endif
	//end this section addition - VR
#endif // 0

	m[opt].type = NM_TYPE_TEXT;  m[opt].text = "Maximum Framerate (1-80):";       opt++;


	inputs=opt;
	m[opt].type = NM_TYPE_INPUT; m[opt].text = smaxfps; m[opt].text_len=3;         opt++;
#if 0
	m[opt].type = NM_TYPE_TEXT;  m[opt].text = "Mission Directory";                opt++;
	m[opt].type = NM_TYPE_INPUT; m[opt].text = thogdir; m[opt].text_len=64;        opt++;
	m[opt].type = NM_TYPE_TEXT;  m[opt].text = "Hud Messages lines (1-80):";       opt++;
	m[opt].type = NM_TYPE_INPUT; m[opt].text = shudmaxnumdisp; m[opt].text_len=3;  opt++;
	m[opt].type = NM_TYPE_SLIDER; m[opt].text = "Message colorization level"; m[opt].value=gr_message_color_level;m[opt].min_value=0;m[opt].max_value=3;  opt++;
	checks=opt;
#ifdef __MSDOS__
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Joy is sidewinder"; m[opt].value=Joy_is_Sidewinder;  opt++;
#endif
#ifdef SUPPORTS_NICEFPS
	m[opt].type = NM_TYPE_CHECK; m[opt].text = "Nice FPS (free cpu cycles)"; m[opt].value = use_nice_fps; opt++;
#endif
#endif // 0

	for(;;)
	{
		i=newmenu_do1( NULL, "D2X options", opt, m, &d2x_options_menu_poll, i);

		if(i>-1)
		{
            if(i<commands)
			{
				switch(i)
				{
#if 0
				case 0: reorder_primary(); break;
				case 1: reorder_secondary(); break;
#endif
#ifdef D2X_KEYS
					//added on 2/4/99 by Victor Rachels for new key menu
				case 0: kconfig(4, "D2X Keys"); break;
					//end this section addition - VR
#endif
					// enabled 3/24/99 - Owen Evans
				case 0: change_res(); break;
					// end enabled stuff - OE
				}
			}

#if 0
            //added on 2/4/99 by Victor Rachels for bans
#ifdef NETWORK
            if(i==commands+0)
			{              

				nm_messagebox(NULL,1,TXT_OK, "%i Bans saved",writebans());

			}
#endif
            //end this section addition - VR
#endif // 0

            if(i == inputs+0)
			{
				maxfps = atoi(smaxfps);
				if(maxfps < 1 || maxfps > MAX_FPS)
				{
					nm_messagebox(TXT_ERROR, 1, TXT_OK, "Invalid value for maximum framerate");
					maxfps = MAX_FPS;
					i = (inputs+0);
				}
			}
#if 0
            else if(i==inputs+2)
				cfile_use_alternate_hogdir(thogdir);
			else if(i==inputs+4)
			{
				HUD_max_num_disp = atoi(shudmaxnumdisp);
                if(HUD_max_num_disp < 1||HUD_max_num_disp>HUD_MAX_NUM)
				{
					nm_messagebox(TXT_ERROR, 1, TXT_OK, "Invalid value for hud lines");
					HUD_max_num_disp=4;
					//                   i=(inputs+4);//???
				}
			}
			gr_message_color_level=m[inputs+5].value;

			sprintf(shudmaxnumdisp,"%d",HUD_max_num_disp);
#endif // 0
			sprintf(smaxfps,"%d",maxfps);
			//           m[inputs+0].text=smaxfps;//redundant.. its not going anywhere
#if 0
			sprintf(thogdir,AltHogDir);
			//           m[inputs+2].text=thogdir;//redundant
#endif
		}
		else
			break;
	}

#if 0
	write_player_file();

#ifdef __MSDOS__
	Joy_is_Sidewinder=m[(checks+0)].value;
#endif
#ifdef __linux__
	Joy_is_Sidewinder=0;
#endif
#ifdef SUPPORTS_NICEFPS
	use_nice_fps=m[(checks+0)].value;
#else
	use_nice_fps=0;
#endif
#endif // 0
}

//end edit - Victor Rachels
//end addition - Victor Rachels


void do_options_menu()
{
	newmenu_item m[13];
	int i = 0;

	do {
		m[ 0].type = NM_TYPE_MENU;   m[ 0].text="Sound effects & music...";
		m[ 1].type = NM_TYPE_TEXT;   m[ 1].text="";
		#if defined(MACINTOSH) && defined(APPLE_DEMO)
		m [2].type = NM_TYPE_TEXT;   m[ 2].text="";
		#else
		m[ 2].type = NM_TYPE_MENU;   m[ 2].text=TXT_CONTROLS_;
		#endif
	#ifdef WINDOWS
		m[ 3].type = NM_TYPE_MENU;   m[ 3].text="INVOKE JOYSTICK CONTROL PANEL";
	#else
		m[ 3].type = NM_TYPE_MENU;   m[ 3].text=TXT_CAL_JOYSTICK;
	#endif
		m[ 4].type = NM_TYPE_TEXT;   m[ 4].text="";

		m[5].type = NM_TYPE_SLIDER;
		m[5].text = TXT_BRIGHTNESS;
		m[5].value = gr_palette_get_gamma();
		m[5].min_value = 0;
		m[5].max_value = 16; // CCA too dim, was 8;


		m[ 6].type = NM_TYPE_MENU;   m[ 6].text=TXT_DETAIL_LEVELS;
		m[ 7].type = NM_TYPE_MENU;   m[ 7].text="Screen resolution...";

		m[ 8].type = NM_TYPE_TEXT;   m[ 8].text="";
		m[ 9].type = NM_TYPE_MENU;   m[ 9].text="Primary autoselect ordering...";
		m[10].type = NM_TYPE_MENU;   m[10].text="Secondary autoselect ordering...";
		m[11].type = NM_TYPE_MENU;   m[11].text="Toggles...";

		m[12].type = NM_TYPE_MENU;   m[12].text="D2X options...";

		i = newmenu_do1( NULL, TXT_OPTIONS, sizeof(m)/sizeof(*m), m, options_menuset, i );
			
		switch(i)       {
			case  0: do_sound_menu();			break;
			case  2: joydefs_config();			break;
			case  3: joydefs_calibrate();		break;
			case  6: do_detail_level_menu(); 	break;
			case  7: do_screen_res_menu();		break;
			case  9: ReorderPrimary();			break;
			case 10: ReorderSecondary();		break;
			case 11: do_toggles_menu();			break;
			case 12: d2x_options_menu();        break;
		}

	} while( i>-1 );

	write_player_file();
}

extern int Redbook_playing;
void set_redbook_volume(int volume);

WIN(extern int RBCDROM_State);
WIN(static BOOL windigi_driver_off=FALSE);

void sound_menuset(int nitems, newmenu_item * items, int *last_key, int citem )
{
	nitems=nitems;          
	*last_key = *last_key;

	if ( Config_digi_volume != items[0].value )     {
		Config_digi_volume = items[0].value;

		#ifdef WINDOWS
			if (windigi_driver_off) {
				digi_midi_wait();
				digi_init_digi();
				Sleep(500);
				windigi_driver_off = FALSE;
			}
		#endif			
		
		#ifndef MACINTOSH
			digi_set_digi_volume( (Config_digi_volume*32768)/8 );
		#else
			digi_set_digi_volume( (Config_digi_volume*256)/8 );
		#endif
		digi_play_sample_once( SOUND_DROP_BOMB, F1_0 );
	}

#ifdef WINDOWS
	if (!wmidi_support_volchange()) {
		if (!items[1].value && Config_midi_volume) {
			Config_midi_volume = 0;
			digi_set_midi_volume(0);
			digi_play_midi_song( NULL, NULL, NULL, 0 );
		}
		else if (Config_midi_volume == 0 && items[1].value) {
			digi_set_midi_volume(64);
			Config_midi_volume = 4;
		}
	}
	else 	 // LINK TO BELOW IF
#endif
	if (Config_midi_volume != items[1].value )   {
 		Config_midi_volume = items[1].value;
		#ifdef WINDOWS
			if (!windigi_driver_off) {
				Sleep(200);
				digi_close_digi();
				Sleep(100);
				windigi_driver_off = TRUE;
			}
		#endif
		#ifndef MACINTOSH
			digi_set_midi_volume( (Config_midi_volume*128)/8 );
		#else
			digi_set_midi_volume( (Config_midi_volume*256)/8 );
		#endif
	}
#ifdef MACINTOSH
	if (Config_master_volume != items[3].value ) {
		Config_master_volume = items[3].value;
		digi_set_master_volume( Config_master_volume );
		digi_play_sample_once( SOUND_DROP_BOMB, F1_0 );
	}
#endif

	// don't enable redbook for a non-apple demo version of the shareware demo
	#if !defined(SHAREWARE) || ( defined(SHAREWARE) && defined(APPLE_DEMO) )

	if (Config_redbook_volume != items[2].value )   {
		Config_redbook_volume = items[2].value;
		set_redbook_volume(Config_redbook_volume);
	}

	if (items[4].value != (Redbook_playing!=0)) {

		if (items[4].value && FindArg("-noredbook")) {
			nm_messagebox (TXT_SORRY,1,TXT_OK,"Redbook audio has been disabled\non the command line");
			items[4].value = 0;
			items[4].redraw = 1;
		}
		else {
			Redbook_enabled = items[4].value;

			mprintf((1, "Redbook_enabled = %d\n", Redbook_enabled));

			if (Function_mode == FMODE_MENU)
				songs_play_song(SONG_TITLE,1);
			else if (Function_mode == FMODE_GAME)
				songs_play_level_song( Current_level_num );
			else
				Int3();

			if (items[4].value && !Redbook_playing) {
			#ifdef WINDOWS
				if (RBCDROM_State == -1) 
					nm_messagebox (TXT_SORRY,1,TXT_OK,"Cannot start CD Music.\nAnother application is\nusing the CD player.\n");
				else // link to next code line!
			#endif
				nm_messagebox (TXT_SORRY,1,TXT_OK,"Cannot start CD Music.  Insert\nyour Descent II CD and try again");
				items[4].value = 0;
				items[4].redraw = 1;
			}

			items[1].type = (Redbook_playing?NM_TYPE_TEXT:NM_TYPE_SLIDER);
			items[1].redraw = 1;
			items[2].type = (Redbook_playing?NM_TYPE_SLIDER:NM_TYPE_TEXT);
			items[2].redraw = 1;

		}
	}

	#endif

	citem++;		//kill warning
}

void do_sound_menu()
{
   newmenu_item m[6];
	int i = 0;

 #ifdef WINDOWS
 	extern BOOL DIGIDriverInit;
 	if (!DIGIDriverInit) windigi_driver_off = TRUE;
 	else windigi_driver_off = FALSE;
 #endif

	do {
		m[ 0].type = NM_TYPE_SLIDER; m[ 0].text=TXT_FX_VOLUME; m[0].value=Config_digi_volume;m[0].min_value=0; m[0].max_value=8; 
		m[ 1].type = (Redbook_playing?NM_TYPE_TEXT:NM_TYPE_SLIDER); m[ 1].text="MIDI music volume"; m[1].value=Config_midi_volume;m[1].min_value=0; m[1].max_value=8;

	#ifdef WINDOWS
		if (!wmidi_support_volchange() && !Redbook_playing) {
			m[1].type = NM_TYPE_CHECK;
			m[1].text = "MIDI MUSIC";
			if (Config_midi_volume) m[1].value = 1;
		}
	#endif

		#ifdef SHAREWARE
			m[ 2].type = NM_TYPE_TEXT; m[ 2].text="";
			m[ 3].type = NM_TYPE_TEXT; m[ 3].text="";
			m[ 4].type = NM_TYPE_TEXT; m[ 4].text="";
			#ifdef MACINTOSH
				m[ 3].type = NM_TYPE_SLIDER; m[ 3].text="Sound Manager Volume"; m[3].value=Config_master_volume;m[3].min_value=0; m[3].max_value=8;

				#ifdef APPLE_DEMO
					m[ 2].type = (Redbook_playing?NM_TYPE_SLIDER:NM_TYPE_TEXT); m[ 2].text="CD music volume"; m[2].value=Config_redbook_volume;m[2].min_value=0; m[2].max_value=8;
					m[ 4].type = NM_TYPE_CHECK;  m[ 4].text="CD Music (Redbook) enabled"; m[4].value=(Redbook_playing!=0);
				#endif

			#endif

		#else		// ifdef SHAREWARE
			m[ 2].type = (Redbook_playing?NM_TYPE_SLIDER:NM_TYPE_TEXT); m[ 2].text="CD music volume"; m[2].value=Config_redbook_volume;m[2].min_value=0; m[2].max_value=8;

			#ifndef MACINTOSH
				m[ 3].type = NM_TYPE_TEXT; m[ 3].text="";
			#else
				m[ 3].type = NM_TYPE_SLIDER; m[ 3].text="Sound Manager Volume"; m[3].value=Config_master_volume;m[3].min_value=0; m[3].max_value=8; 
			#endif
		
			m[ 4].type = NM_TYPE_CHECK;  m[ 4].text="CD Music (Redbook) enabled"; m[4].value=(Redbook_playing!=0);
		#endif
	
		m[ 5].type = NM_TYPE_CHECK;  m[ 5].text=TXT_REVERSE_STEREO; m[5].value=Config_channels_reversed; 
				
		i = newmenu_do1( NULL, "Sound Effects & Music", sizeof(m)/sizeof(*m), m, sound_menuset, i );

		Redbook_enabled = m[4].value;
		Config_channels_reversed = m[5].value;

	} while( i>-1 );

#ifdef WINDOWS
	if (windigi_driver_off) {
		digi_midi_wait();
		Sleep(500);
		digi_init_digi();
		windigi_driver_off=FALSE;
	}
#endif


	if ( Config_midi_volume < 1 )   {
		#ifndef MACINTOSH
			digi_play_midi_song( NULL, NULL, NULL, 0 );
		#else
			digi_play_midi_song(-1, 0);
		#endif
	}

}


extern int Automap_always_hires;

#define ADD_CHECK(n,txt,v)  do { m[n].type=NM_TYPE_CHECK; m[n].text=txt; m[n].value=v;} while (0)

void do_toggles_menu()
{
	newmenu_item m[7];
	int i = 0;

	do {
		#if defined(MACINTOSH) && defined(USE_ISP)
			if (ISpEnabled())
			{
				m[0].type = NM_TYPE_TEXT; m[0].text = "";
			}
			else
			{
				ADD_CHECK(0, "Ship auto-leveling", Auto_leveling_on);
			}
		#else 
			ADD_CHECK(0, "Ship auto-leveling", Auto_leveling_on);
		#endif
		ADD_CHECK(1, "Show reticle", Reticle_on);
		ADD_CHECK(2, "Missile view", Missile_view_enabled);
		ADD_CHECK(3, "Headlight on when picked up", Headlight_active_default );
		ADD_CHECK(4, "Show guided missile in main display", Guided_in_big_window );
		ADD_CHECK(5, "Escort robot hot keys",EscortHotKeys);
		//when adding more options, change N_TOGGLE_ITEMS above

		i = newmenu_do1( NULL, "Toggles", 7, m, NULL, i );
			
		Auto_leveling_on			= m[0].value;
		Reticle_on					= m[1].value;
		Missile_view_enabled    	= m[2].value;
		Headlight_active_default	= m[3].value;
		Guided_in_big_window		= m[4].value;
		EscortHotKeys				= m[5].value;


		if (MenuHiresAvailable)
			Automap_always_hires = m[6].value;
		else if (m[6].value)
			nm_messagebox(TXT_SORRY,1,"OK","High Resolution modes are\nnot available on this video card");
	} while( i>-1 );

}

#ifdef NETWORK
void do_multi_player_menu()
{
	int menu_choice[9];
	newmenu_item m[9];
	int choice = 0, num_options = 0;
	int old_game_mode;

	do {
//		WIN(ipx_destroy_read_thread());

		old_game_mode = Game_mode;
		num_options = 0;

#ifdef NATIVE_IPX
		ADD_ITEM(TXT_START_IPX_NET_GAME, MENU_START_IPX_NETGAME, -1);
		ADD_ITEM(TXT_JOIN_IPX_NET_GAME, MENU_JOIN_IPX_NETGAME, -1);
#endif //NATIVE_IPX
		//ADD_ITEM(TXT_START_TCP_NET_GAME, MENU_START_TCP_NETGAME, -1);
		//ADD_ITEM(TXT_JOIN_TCP_NET_GAME, MENU_JOIN_TCP_NETGAME, -1);
		ADD_ITEM("Start UDP/IP Netgame", MENU_START_UDP_NETGAME, -1);
		ADD_ITEM("Join UDP/IP Netgame\n", MENU_JOIN_UDP_NETGAME, -1);
		ADD_ITEM("Start Multicast UDP/IP Netgame", MENU_START_MCAST4_NETGAME, -1);
		ADD_ITEM("Join Multicast UDP/IP Netgame\n", MENU_JOIN_MCAST4_NETGAME, -1);
#ifdef KALINIX
		ADD_ITEM("Start Kali Netgame", MENU_START_KALI_NETGAME, -1);
		ADD_ITEM("Join Kali Netgame\n", MENU_JOIN_KALI_NETGAME, -1);
#endif // KALINIX

#ifdef MACINTOSH
		ADD_ITEM("Start Appletalk Netgame", MENU_START_APPLETALK_NETGAME, -1 );
		ADD_ITEM("Join Appletalk Netgame\n", MENU_JOIN_APPLETALK_NETGAME, -1 );
#endif

		ADD_ITEM(TXT_MODEM_GAME, MENU_START_SERIAL, -1);

		choice = newmenu_do1( NULL, TXT_MULTIPLAYER, num_options, m, NULL, choice );
		
		if ( choice > -1 )      
			do_option(menu_choice[choice]);
	
		if (old_game_mode != Game_mode)
			break;          // leave menu

	} while( choice > -1 );

}

/*
 * ipx_set_driver was called do_network_init and located in main/inferno
 * before the change which allows the user to choose the network driver
 * from the game menu instead of having to supply command line args.
 */
void ipx_set_driver(int ipx_driver)
{
	ipx_close();

	if (!FindArg("-nonetwork")) {
		int ipx_error;
		int socket = 0, t;

		con_printf(CON_VERBOSE, "\n%s ", TXT_INITIALIZING_NETWORK);

		if ((t = FindArg("-socket")))
			socket = atoi(Args[t + 1]);

		arch_ipx_set_driver(ipx_driver);

		if ((ipx_error = ipx_init(IPX_DEFAULT_SOCKET + socket)) == IPX_INIT_OK) {
			con_printf(CON_VERBOSE, "%s %d.\n", TXT_IPX_CHANNEL, socket );
			Network_active = 1;
		} else {
			switch(ipx_error) {
			case IPX_NOT_INSTALLED: con_printf(CON_VERBOSE, "%s\n", TXT_NO_NETWORK); break;
			case IPX_SOCKET_TABLE_FULL: con_printf(CON_VERBOSE, "%s 0x%x.\n", TXT_SOCKET_ERROR, IPX_DEFAULT_SOCKET+socket); break;
			case IPX_NO_LOW_DOS_MEM: con_printf(CON_VERBOSE, "%s\n", TXT_MEMORY_IPX ); break;
			default: con_printf(CON_VERBOSE, "%s %d", TXT_ERROR_IPX, ipx_error );
			}
			con_printf(CON_VERBOSE, "%s\n",TXT_NETWORK_DISABLED);
			Network_active = 0;		// Assume no network
		}
		ipx_read_user_file("descent.usr");
		ipx_read_network_file("descent.net");
		//@@if (FindArg("-dynamicsockets"))
		//@@	Network_allow_socket_changes = 1;
		//@@else
		//@@	Network_allow_socket_changes = 0;
	} else {
		con_printf(CON_VERBOSE, "%s\n", TXT_NETWORK_DISABLED);
		Network_active = 0;		// Assume no network
	}
}

void DoNewIPAddress ()
 {
  newmenu_item m[4];
  char IPText[30];
  int choice;

  m[0].type=NM_TYPE_TEXT; m[0].text = "Enter an address or hostname:";
  m[1].type=NM_TYPE_INPUT; m[1].text_len = 50; m[1].text = IPText;
  IPText[0]=0;

  choice = newmenu_do( NULL, "Join a TCPIP game", 2, m, NULL );

  if (choice==-1 || m[1].text[0]==0)
   return;

  nm_messagebox (TXT_SORRY,1,TXT_OK,"That address is not valid!");
 }

#endif // NETWORK
