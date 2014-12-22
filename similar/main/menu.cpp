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
 * Inferno main menu.
 *
 */

#include <stdio.h>
#include <string.h>

#include "menu.h"
#include "inferno.h"
#include "game.h"
#include "gr.h"
#include "key.h"
#include "mouse.h"
#include "iff.h"
#include "u_mem.h"
#include "dxxerror.h"
#include "bm.h"
#include "screens.h"
#include "joy.h"
#include "vecmat.h"
#include "effects.h"
#include "game.h"
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
#include "scores.h"
#include "playsave.h"
#include "kconfig.h"
#include "titles.h"
#include "credits.h"
#include "texmap.h"
#include "polyobj.h"
#include "state.h"
#include "mission.h"
#include "songs.h"
#ifdef USE_SDLMIXER
#include "jukebox.h" // for jukebox_exts
#endif
#include "config.h"
#if defined(DXX_BUILD_DESCENT_II)
#include "movie.h"
#endif
#include "gamepal.h"
#include "gauges.h"
#include "powerup.h"
#include "strutil.h"
#include "multi.h"
#include "vers_id.h"
#ifdef USE_UDP
#include "net_udp.h"
#endif
#ifdef EDITOR
#include "editor/editor.h"
#include "editor/kdefs.h"
#endif
#ifdef OGL
#include "ogl_init.h"
#endif

#include "dxxsconf.h"
#include "compiler-make_unique.h"

// Menu IDs...
enum MENUS
{
    MENU_NEW_GAME = 0,
    MENU_GAME,
    MENU_EDITOR,
    MENU_VIEW_SCORES,
    MENU_QUIT,
    MENU_LOAD_GAME,
    MENU_SAVE_GAME,
    MENU_DEMO_PLAY,
    MENU_CONFIG,
    MENU_REJOIN_NETGAME,
    MENU_DIFFICULTY,
    MENU_HELP,
    MENU_NEW_PLAYER,
    #if defined(USE_UDP)
        MENU_MULTIPLAYER,
    #endif

    MENU_SHOW_CREDITS,
    MENU_ORDER_INFO,

    #ifdef USE_UDP
    MENU_START_UDP_NETGAME,
    MENU_JOIN_MANUAL_UDP_NETGAME,
    MENU_JOIN_LIST_UDP_NETGAME,
    #endif
    #ifndef RELEASE
    MENU_SANDBOX
    #endif
};

//ADD_ITEM("Start netgame...", MENU_START_NETGAME, -1 );
//ADD_ITEM("Send net message...", MENU_SEND_NET_MESSAGE, -1 );

#define ADD_ITEM(t,value,key)  do { nm_set_item_menu(&m[num_options], t); menu_choice[num_options]=value;num_options++; } while (0)

static window *menus[16] = { NULL };

// Function Prototypes added after LINTING
static int do_option(int select);
static int do_new_game_menu(void);
static void do_multi_player_menu();
#ifndef RELEASE
static void do_sandbox_menu();
#endif

template <typename T>
class select_file_subfunction_t
{
public:
	typedef int (*type)(T *, const char *);
};

template <typename T>
static int select_file_recursive(const char *title, const char *orig_path, const file_extension_t *ext_list, int select_dir, typename select_file_subfunction_t<T>::type when_selected, T *userdata) __attribute_nonnull();

template <>
int select_file_recursive<void>(const char *title, const char *orig_path, const file_extension_t *ext_list, int select_dir, select_file_subfunction_t<void>::type when_selected, void *userdata) __attribute_nonnull();

template <typename T>
static int select_file_recursive(const char *title, const char *orig_path, const file_extension_t *ext_list, int select_dir, typename select_file_subfunction_t<T>::type when_selected, T *userdata)
{
	return select_file_recursive(title, orig_path, ext_list, select_dir, (select_file_subfunction_t<void>::type)when_selected, (void *)userdata);
}

// Hide all menus
int hide_menus(void)
{
	window *wind;
	int i;

	if (menus[0])
		return 0;		// there are already hidden menus

	for (i = 0; (i < 15) && (wind = window_get_front()); i++)
	{
		menus[i] = wind;
		window_set_visible(wind, 0);
	}

	Assert(window_get_front() == NULL);
	menus[i] = NULL;

	return 1;
}

// Show all menus, with the front one shown first
// This makes sure EVENT_WINDOW_ACTIVATED is only sent to that window
void show_menus(void)
{
	int i;

	for (i = 0; (i < 16) && menus[i]; i++)
		if (window_exists(menus[i]))
			window_set_visible(menus[i], 1);

	menus[0] = NULL;
}

//pairs of chars describing ranges
static const char playername_allowed_chars[] = "azAZ09__--";

static int MakeNewPlayerFile(int allow_abort)
{
	int x;
	char filename[PATH_MAX];
	newmenu_item m;
	callsign_t text = Players[Player_num].callsign;

try_again:
	nm_set_item_input(&m, CALLSIGN_LEN, text.buffer());

	Newmenu_allowed_chars = playername_allowed_chars;
	x = newmenu_do( NULL, TXT_ENTER_PILOT_NAME, 1, &m, unused_newmenu_subfunction, unused_newmenu_userdata );
	Newmenu_allowed_chars = NULL;

	if ( x < 0 ) {
		if ( allow_abort ) return 0;
		goto try_again;
	}

	if (!*static_cast<const char *>(text))	//null string
		goto try_again;

	text.lower();

	snprintf(filename, sizeof(filename), PLAYER_DIRECTORY_STRING("%s.plr"), static_cast<const char *>(text) );

	if (PHYSFSX_exists(filename,0))
	{
		nm_messagebox(NULL, 1, TXT_OK, "%s '%s' %s", TXT_PLAYER, static_cast<const char *>(text), TXT_ALREADY_EXISTS );
		goto try_again;
	}

	if ( !new_player_config() )
		goto try_again;			// They hit Esc during New player config

	Players[Player_num].callsign = text;

	write_player_file();

	return 1;
}

static void delete_player_saved_games(const char * name);

static int player_menu_keycommand( listbox *lb,const d_event &event )
{
	const char **items = listbox_get_items(lb);
	int citem = listbox_get_citem(lb);

	switch (event_key_get(event))
	{
		case KEY_CTRLED+KEY_D:
			if (citem > 0)
			{
				int x = 1;
				x = nm_messagebox( NULL, 2, TXT_YES, TXT_NO, "%s %s?", TXT_DELETE_PILOT, items[citem]+((items[citem][0]=='$')?1:0) );
				if (x==0)	{
					char plxfile[PATH_MAX], efffile[PATH_MAX], ngpfile[PATH_MAX];
					int ret;
					char name[PATH_MAX];

					snprintf(name, sizeof(name), PLAYER_DIRECTORY_STRING("%.8s.plr"), items[citem]);

					ret = !PHYSFS_delete(name);

					if (!ret)
					{
						delete_player_saved_games( items[citem] );
						// delete PLX file
						snprintf(plxfile, sizeof(plxfile), PLAYER_DIRECTORY_STRING("%.8s.plx"), items[citem]);
						if (PHYSFSX_exists(plxfile,0))
							PHYSFS_delete(plxfile);
						// delete EFF file
						snprintf(efffile, sizeof(efffile), PLAYER_DIRECTORY_STRING("%.8s.eff"), items[citem]);
						if (PHYSFSX_exists(efffile,0))
							PHYSFS_delete(efffile);
						// delete NGP file
						snprintf(ngpfile, sizeof(ngpfile), PLAYER_DIRECTORY_STRING("%.8s.ngp"), items[citem]);
						if (PHYSFSX_exists(ngpfile,0))
							PHYSFS_delete(ngpfile);
					}

					if (ret)
						nm_messagebox( NULL, 1, TXT_OK, "%s %s %s", TXT_COULDNT, TXT_DELETE_PILOT, items[citem]+((items[citem][0]=='$')?1:0) );
					else
						listbox_delete_item(lb, citem);
				}

				return 1;
			}
			break;
	}

	return 0;
}

static int player_menu_handler( listbox *lb,const d_event &event, char **list )
{
	const char **items = listbox_get_items(lb);
	int citem = listbox_get_citem(lb);

	switch (event.type)
	{
		case EVENT_KEY_COMMAND:
			return player_menu_keycommand(lb, event);
			break;

		case EVENT_NEWMENU_SELECTED:
			if (citem < 0)
				return 0;		// shouldn't happen
			else if (citem == 0)
			{
				// They selected 'create new pilot'
				return !MakeNewPlayerFile(1);
			}
			else
			{
				Players[Player_num].callsign.copy_lower(items[citem], strlen(items[citem]));
			}
			break;

		case EVENT_WINDOW_CLOSE:
			if (read_player_file() != EZERO)
				return 1;		// abort close!

			WriteConfigFile();		// Update lastplr

			PHYSFS_freeList(list);
			d_free(items);
			break;

		default:
			break;
	}

	return 0;
}

//Inputs the player's name, without putting up the background screen
int RegisterPlayer()
{
	const char **m;
	char **f;
	char **list;
	static const file_extension_t types[] = { "plr", "" };
	int i = 0, NumItems;
	int citem = 0;
	int allow_abort_flag = 1;

	if (!*static_cast<const char *>(Players[Player_num].callsign))
	{
		if (!*static_cast<const char *>(GameCfg.LastPlayer))
		{
			Players[Player_num].callsign = "player";
			allow_abort_flag = 0;
		}
		else
		{
			// Read the last player's name from config file, not lastplr.txt
			Players[Player_num].callsign = GameCfg.LastPlayer;
		}
	}

	list = PHYSFSX_findFiles(PLAYER_DIRECTORY_STRING(""), types);
	if (!list)
		return 0;	// memory error
	if (!*list)
	{
		MakeNewPlayerFile(0);	// make a new player without showing listbox
		PHYSFS_freeList(list);
		return 0;
	}


	for (NumItems = 0; list[NumItems] != NULL; NumItems++) {}
	NumItems++;		// for TXT_CREATE_NEW

	MALLOC(m, const char *, NumItems);
	if (m == NULL)
	{
		PHYSFS_freeList(list);
		return 0;
	}

	m[i++] = TXT_CREATE_NEW;

	for (f = list; *f != NULL; f++)
	{
		char *p;

		size_t lenf = strlen(*f);
		if (lenf > FILENAME_LEN-1 || lenf < 5) // sorry guys, can only have up to eight chars for the player name
		{
			NumItems--;
			continue;
		}
		m[i++] = *f;
		p = strchr(*f, '.');
		if (p)
			*p = '\0';		// chop the .plr
	}

	if (NumItems <= 1) // so it seems all plr files we found were too long. funny. let's make a real player
	{
		MakeNewPlayerFile(0);	// make a new player without showing listbox
		PHYSFS_freeList(list);
		return 0;
	}

	// Sort by name, except the <Create New Player> string
	qsort(&m[1], NumItems - 1, sizeof(char *), (int (*)( const void *, const void * ))string_array_sort_func);

	for ( i=0; i<NumItems; i++ )
		if (!d_stricmp(static_cast<const char *>(Players[Player_num].callsign), m[i]) )
			citem = i;

	newmenu_listbox1(TXT_SELECT_PILOT, NumItems, m, allow_abort_flag, citem, player_menu_handler, list);

	return 1;
}

// Draw Copyright and Version strings
static void draw_copyright()
{
	gr_set_current_canvas(NULL);
	gr_set_curfont(GAME_FONT);
	gr_set_fontcolor(BM_XRGB(6,6,6),-1);
	gr_string(0x8000,SHEIGHT-LINE_SPACING,TXT_COPYRIGHT);
	gr_set_fontcolor( BM_XRGB(25,0,0), -1);
	gr_string(0x8000,SHEIGHT-(LINE_SPACING*2),DESCENT_VERSION);
}

// ------------------------------------------------------------------------
static int main_menu_handler(newmenu *menu,const d_event &event, int *menu_choice )
{
	newmenu_item *items = newmenu_get_items(menu);

	switch (event.type)
	{
		case EVENT_WINDOW_ACTIVATED:
			load_palette(MENU_PALETTE,0,1);		//get correct palette

			if (!*static_cast<const char *>(Players[Player_num].callsign))
				RegisterPlayer();
			else
				keyd_time_when_last_pressed = timer_query();		// .. 20 seconds from now!
			break;

		case EVENT_KEY_COMMAND:
			// Don't allow them to hit ESC in the main menu.
			if (event_key_get(event)==KEY_ESC)
				return 1;
			break;

		case EVENT_MOUSE_BUTTON_DOWN:
		case EVENT_MOUSE_BUTTON_UP:
			// Don't allow mousebutton-closing in main menu.
			if (event_mouse_get_button(event) == MBTN_RIGHT)
				return 1;
			break;

		case EVENT_IDLE:
#if defined(DXX_BUILD_DESCENT_I)
#define DXX_DEMO_KEY_DELAY	45
#elif defined(DXX_BUILD_DESCENT_II)
#define DXX_DEMO_KEY_DELAY	25
#endif
			if ( keyd_time_when_last_pressed+i2f(DXX_DEMO_KEY_DELAY) < timer_query() || GameArg.SysAutoDemo  )
			{
				keyd_time_when_last_pressed = timer_query();			// Reset timer so that disk won't thrash if no demos.

#if defined(DXX_BUILD_DESCENT_II)
				int n_demos = newdemo_count_demos();
				if (((d_rand() % (n_demos+1)) == 0) && !GameArg.SysAutoDemo)
				{
#ifdef OGL
					Screen_mode = -1;
#endif
					PlayMovie("intro.tex", "intro.mve",0);
					songs_play_song(SONG_TITLE,1);
					set_screen_mode(SCREEN_MENU);
				}
				else
#endif
				{
					newdemo_start_playback(NULL);		// Randomly pick a file, assume native endian (crashes if not)
#if defined(DXX_BUILD_DESCENT_II)
					if (Newdemo_state == ND_STATE_PLAYBACK)
						return 0;
#endif
				}
			}
			break;

		case EVENT_NEWMENU_DRAW:
			draw_copyright();
			break;

		case EVENT_NEWMENU_SELECTED:
			return do_option(menu_choice[newmenu_get_citem(menu)]);
			break;

		case EVENT_WINDOW_CLOSE:
			d_free(menu_choice);
			d_free(items);
			break;

		default:
			break;
	}

	return 0;
}

//	-----------------------------------------------------------------------------
//	Create the main menu.
static void create_main_menu(newmenu_item *m, int *menu_choice, int *callers_num_options)
{
	int num_options = 0;

	#ifndef DEMO_ONLY
	ADD_ITEM(TXT_NEW_GAME,MENU_NEW_GAME,KEY_N);

	ADD_ITEM(TXT_LOAD_GAME,MENU_LOAD_GAME,KEY_L);
#if defined(USE_UDP)
	ADD_ITEM(TXT_MULTIPLAYER_,MENU_MULTIPLAYER,-1);
#endif

	ADD_ITEM(TXT_OPTIONS_, MENU_CONFIG, -1 );
	ADD_ITEM(TXT_CHANGE_PILOTS,MENU_NEW_PLAYER,unused);
	ADD_ITEM(TXT_VIEW_DEMO,MENU_DEMO_PLAY,0);
	ADD_ITEM(TXT_VIEW_SCORES,MENU_VIEW_SCORES,KEY_V);
#if defined(DXX_BUILD_DESCENT_I)
	if (!PHYSFSX_exists("warning.pcx",1)) /* SHAREWARE */
#elif defined(DXX_BUILD_DESCENT_II)
	if (PHYSFSX_exists("orderd2.pcx",1)) /* SHAREWARE */
#endif
		ADD_ITEM(TXT_ORDERING_INFO,MENU_ORDER_INFO,-1);
	ADD_ITEM(TXT_CREDITS,MENU_SHOW_CREDITS,-1);
	#endif
	ADD_ITEM(TXT_QUIT,MENU_QUIT,KEY_Q);

	#ifndef RELEASE
	if (!(Game_mode & GM_MULTI ))	{
		#ifdef EDITOR
		ADD_ITEM("  Editor", MENU_EDITOR, KEY_E);
		#endif
	}
	ADD_ITEM("  SANDBOX", MENU_SANDBOX, -1);
	#endif

	*callers_num_options = num_options;
}

//returns number of item chosen
int DoMenu()
{
	int *menu_choice;
	newmenu_item *m;
	int num_options = 0;

	CALLOC(menu_choice, int, 25);
	if (!menu_choice)
		return -1;
	CALLOC(m, newmenu_item, 25);
	if (!m)
	{
		d_free(menu_choice);
		return -1;
	}

	create_main_menu(m, menu_choice, &num_options); // may have to change, eg, maybe selected pilot and no save games.

	newmenu_do3( "", NULL, num_options, m, main_menu_handler, menu_choice, 0, Menu_pcx_name);

	return 0;
}

//returns flag, true means quit menu
int do_option ( int select)
{
	switch (select) {
		case MENU_NEW_GAME:
			select_mission(0, "New Game\n\nSelect mission", do_new_game_menu);
			break;
		case MENU_GAME:
			break;
		case MENU_DEMO_PLAY:
			select_demo();
			break;
		case MENU_LOAD_GAME:
			state_restore_all(0, 0, NULL, 0);
			break;
		#ifdef EDITOR
		case MENU_EDITOR:
			if (!Current_mission)
			{
				create_new_mine();
				SetPlayerFromCurseg();
			}

			hide_menus();
			init_editor();
			break;
		#endif
		case MENU_VIEW_SCORES:
			scores_view(NULL, -1);
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
			return 0;

		case MENU_NEW_PLAYER:
			RegisterPlayer();
			break;

#ifdef USE_UDP
		case MENU_START_UDP_NETGAME:
			multi_protocol = MULTI_PROTO_UDP;
			select_mission(1, TXT_MULTI_MISSION, net_udp_setup_game);
			break;
		case MENU_JOIN_MANUAL_UDP_NETGAME:
			multi_protocol = MULTI_PROTO_UDP;
			net_udp_manual_join_game();
			break;
		case MENU_JOIN_LIST_UDP_NETGAME:
			multi_protocol = MULTI_PROTO_UDP;
			net_udp_list_join_game();
			break;
#endif
#if defined(USE_UDP)
		case MENU_MULTIPLAYER:
			do_multi_player_menu();
			break;
#endif
		case MENU_CONFIG:
			do_options_menu();
			break;
		case MENU_SHOW_CREDITS:
			credits_show(NULL);
			break;
#ifndef RELEASE
		case MENU_SANDBOX:
			do_sandbox_menu();
			break;
#endif
		default:
			Error("Unknown option %d in do_option",select);
			break;
	}

	return 1;		// stay in main menu unless quitting
}

static void delete_player_saved_games(const char * name)
{
	int i;
	char filename[PATH_MAX];

	for (i=0;i<10; i++)
	{
		snprintf( filename, sizeof(filename), PLAYER_DIRECTORY_STRING("%s.sg%x"), name, i );
		PHYSFS_delete(filename);
		snprintf( filename, sizeof(filename), PLAYER_DIRECTORY_STRING("%s.mg%x"), name, i );
		PHYSFS_delete(filename);
	}
}

static int demo_menu_keycommand( listbox *lb,const d_event &event )
{
	const char **items = listbox_get_items(lb);
	int citem = listbox_get_citem(lb);

	switch (event_key_get(event))
	{
		case KEY_CTRLED+KEY_D:
			if (citem >= 0)
			{
				int x = 1;
				x = nm_messagebox( NULL, 2, TXT_YES, TXT_NO, "%s %s?", TXT_DELETE_DEMO, items[citem]+((items[citem][0]=='$')?1:0) );
				if (x==0)
				{
					int ret;
					char name[PATH_MAX];

					strcpy(name, DEMO_DIR);
					strcat(name,items[citem]);

					ret = !PHYSFS_delete(name);

					if (ret)
						nm_messagebox( NULL, 1, TXT_OK, "%s %s %s", TXT_COULDNT, TXT_DELETE_DEMO, items[citem]+((items[citem][0]=='$')?1:0) );
					else
						listbox_delete_item(lb, citem);
				}

				return 1;
			}
			break;

		case KEY_CTRLED+KEY_C:
			{
				int x = 1;
				char bakname[PATH_MAX];

				// Get backup name
				change_filename_extension(bakname, items[citem]+((items[citem][0]=='$')?1:0), DEMO_BACKUP_EXT);
				x = nm_messagebox( NULL, 2, TXT_YES, TXT_NO,	"Are you sure you want to\n"
								  "swap the endianness of\n"
								  "%s? If the file is\n"
								  "already endian native, D1X\n"
								  "will likely crash. A backup\n"
								  "%s will be created", items[citem]+((items[citem][0]=='$')?1:0), bakname );
				if (!x)
					newdemo_swap_endian(items[citem]);

				return 1;
			}
			break;
	}

	return 0;
}

static int demo_menu_handler( listbox *lb,const d_event &event, unused_listbox_userdata_t *)
{
	const char **items = listbox_get_items(lb);
	int citem = listbox_get_citem(lb);
	switch (event.type)
	{
		case EVENT_KEY_COMMAND:
			return demo_menu_keycommand(lb, event);
			break;

		case EVENT_NEWMENU_SELECTED:
			if (citem < 0)
				return 0;		// shouldn't happen

			newdemo_start_playback(items[citem]);
			return 1;		// stay in demo selector

		case EVENT_WINDOW_CLOSE:
			PHYSFS_freeList(items);
			break;

		default:
			break;
	}

	return 0;
}

int select_demo(void)
{
	char **list;
	int NumItems;

	list = PHYSFSX_findFiles(DEMO_DIR, demo_file_extensions);
	if (!list)
		return 0;	// memory error
	if ( !*list )
	{
		nm_messagebox( NULL, 1, TXT_OK, "%s %s\n%s", TXT_NO_DEMO_FILES, TXT_USE_F5, TXT_TO_CREATE_ONE);
		PHYSFS_freeList(list);
		return 0;
	}

	for (NumItems = 0; list[NumItems] != NULL; NumItems++) {}

	// Sort by name
	qsort(list, NumItems, sizeof(char *), (int (*)( const void *, const void * ))string_array_sort_func);

	newmenu_listbox1(TXT_SELECT_DEMO, NumItems, (const char **) list, 1, 0, demo_menu_handler, unused_listbox_userdata);

	return 1;
}

static int do_difficulty_menu()
{
	int s;
	newmenu_item m[5];

	nm_set_item_menu(&m[0], MENU_DIFFICULTY_TEXT(0));
	nm_set_item_menu(&m[1], MENU_DIFFICULTY_TEXT(1));
	nm_set_item_menu(&m[2], MENU_DIFFICULTY_TEXT(2));
	nm_set_item_menu(&m[3], MENU_DIFFICULTY_TEXT(3));
	nm_set_item_menu(&m[4], MENU_DIFFICULTY_TEXT(4));

	s = newmenu_do1( NULL, TXT_DIFFICULTY_LEVEL, NDL, m, unused_newmenu_subfunction, unused_newmenu_userdata, Difficulty_level);

	if (s > -1 )	{
		if (s != Difficulty_level)
		{
			PlayerCfg.DefaultDifficulty = s;
			write_player_file();
		}
		Difficulty_level = s;
		return 1;
	}
	return 0;
}

int do_new_game_menu()
{
	int new_level_num,player_highest_level;

	new_level_num = 1;
#ifdef NDEBUG
	player_highest_level = get_highest_level();

	if (player_highest_level > Last_level)
#endif
		player_highest_level = Last_level;
	if (player_highest_level > 1) {
		newmenu_item m[4];
		char info_text[80];
		char num_text[10];
		int choice;
		int n_items;
		int valid = 0;

		snprintf(info_text,sizeof(info_text),"%s %d",TXT_START_ANY_LEVEL, player_highest_level);
		while (!valid)
		{

			nm_set_item_text(& m[0], info_text);
			nm_set_item_input(&m[1], 10, num_text);
			n_items = 2;

			strcpy(num_text,"1");

			choice = newmenu_do( NULL, TXT_SELECT_START_LEV, n_items, m, unused_newmenu_subfunction, unused_newmenu_userdata );

			if (choice==-1 || m[1].text[0]==0)
				return 0;

			new_level_num = atoi(m[1].text);

			if (!(new_level_num>0 && new_level_num<=player_highest_level)) {
				nm_messagebox( NULL, 1, TXT_OK, TXT_INVALID_LEVEL);
				valid = 0;
			}
			else
				valid = 1;
		}
	}

	Difficulty_level = PlayerCfg.DefaultDifficulty;

	if (!do_difficulty_menu())
		return 0;

	StartNewGame(new_level_num);

	return 1;	// exit mission listbox
}

static void do_sound_menu();
static void input_config();
static void change_res();
static void graphics_config();
static void do_misc_menu();

static int options_menuset(newmenu *menu,const d_event &event, const unused_newmenu_userdata_t *)
{
	switch (event.type)
	{
		case EVENT_NEWMENU_CHANGED:
			break;

		case EVENT_NEWMENU_SELECTED:
			switch(newmenu_get_citem(menu))
			{
				case  0: do_sound_menu();		break;
				case  2: input_config();		break;
				case  4: change_res();			break;
				case  5: graphics_config();		break;
				case  7: ReorderPrimary();		break;
				case  8: ReorderSecondary();		break;
				case  9: do_misc_menu();		break;
			}
			return 1;	// stay in menu until escape
			break;

		case EVENT_WINDOW_CLOSE:
		{
			newmenu_item *items = newmenu_get_items(menu);
			d_free(items);
			write_player_file();
			break;
		}

		default:
			break;
	}
	return 0;
}

static int gcd(int a, int b)
{
	if (!b)
		return a;

	return gcd(b, a%b);
}

void change_res()
{
	array<uint32_t, 50> modes;
	u_int32_t new_mode = 0;
	int i = 0, mc = 0, num_presets = 0, citem = -1, opt_cval = -1, opt_fullscr = -1;

	num_presets = gr_list_modes( modes );

	{
	newmenu_item m[50+8];
	char restext[50][12], crestext[12], casptext[12];

	for (i = 0; i <= num_presets-1; i++)
	{
		snprintf(restext[mc], sizeof(restext[mc]), "%ix%i", SM_W(modes[i]), SM_H(modes[i]));

		nm_set_item_radio(&m[mc], restext[mc], ((citem == -1) && (Game_screen_mode == modes[i]) && GameCfg.AspectY == SM_W(modes[i])/gcd(SM_W(modes[i]),SM_H(modes[i])) && GameCfg.AspectX == SM_H(modes[i])/gcd(SM_W(modes[i]),SM_H(modes[i]))), 0);
		if (m[mc].value)
			citem = mc;
		mc++;
	}

	nm_set_item_text(& m[mc], ""); mc++; // little space for overview
	// the fields for custom resolution and aspect
	opt_cval = mc;
	nm_set_item_radio(&m[mc], "use custom values", (citem == -1), 0); mc++;
	nm_set_item_text(& m[mc], "resolution:"); mc++;
	snprintf(crestext, sizeof(crestext), "%ix%i", SM_W(Game_screen_mode), SM_H(Game_screen_mode));
	nm_set_item_input(&m[mc], 11, crestext); modes[mc] = 0; mc++;
	nm_set_item_text(& m[mc], "aspect:"); mc++;
	snprintf(casptext, sizeof(casptext), "%ix%i", GameCfg.AspectY, GameCfg.AspectX);
	nm_set_item_input(&m[mc], 11, casptext); modes[mc] = 0; mc++;
	nm_set_item_text(& m[mc], ""); mc++; // little space for overview
	// fullscreen
	opt_fullscr = mc;
	nm_set_item_checkbox(&m[mc], "Fullscreen", gr_check_fullscreen()); mc++;

	// create the menu
	newmenu_do1(NULL, "Screen Resolution", mc, m, unused_newmenu_subfunction, unused_newmenu_userdata, 0);

	// menu is done, now do what we need to do

	// check which resolution field was selected
	for (i = 0; i <= mc; i++)
		if ((m[i].type == NM_TYPE_RADIO) && (m[i].group==0) && (m[i].value == 1))
			break;

	// now check for fullscreen toggle and apply if necessary
	if (m[opt_fullscr].value != gr_check_fullscreen())
		gr_toggle_fullscreen();

	if (i == opt_cval) // set custom resolution and aspect
	{
		u_int32_t cmode = Game_screen_mode, casp = Game_screen_mode;

		if (!strchr(crestext, 'x'))
			return;

		cmode = SM(atoi(crestext), atoi(strchr(crestext, 'x')+1));
		if (SM_W(cmode) < 320 || SM_H(cmode) < 200) // oh oh - the resolution is too small. Revert!
		{
			nm_messagebox( TXT_WARNING, 1, "OK", "Entered resolution is too small.\nReverting ..." );
			cmode = new_mode;
		}

		casp = cmode;
		if (strchr(casptext, 'x')) // we even have a custom aspect set up
		{
			casp = SM(atoi(casptext), atoi(strchr(casptext, 'x')+1));
		}
		GameCfg.AspectY = SM_W(casp)/gcd(SM_W(casp),SM_H(casp));
		GameCfg.AspectX = SM_H(casp)/gcd(SM_W(casp),SM_H(casp));
		new_mode = cmode;
	}
	else if (i >= 0 && i < num_presets) // set preset resolution
	{
		new_mode = modes[i];
		GameCfg.AspectY = SM_W(new_mode)/gcd(SM_W(new_mode),SM_H(new_mode));
		GameCfg.AspectX = SM_H(new_mode)/gcd(SM_W(new_mode),SM_H(new_mode));
	}

	// clean up and apply everything
	newmenu_free_background();
	set_screen_mode(SCREEN_MENU);
	if (new_mode != Game_screen_mode)
	{
		gr_set_mode(new_mode);
		Game_screen_mode = new_mode;
		if (Game_wind) // shortly activate Game_wind so it's canvas will align to new resolution. really minor glitch but whatever
		{
			d_event event;
			WINDOW_SEND_EVENT(Game_wind, EVENT_WINDOW_ACTIVATED);
			WINDOW_SEND_EVENT(Game_wind, EVENT_WINDOW_DEACTIVATED);
		}
	}
	game_init_render_buffers(SM_W(Game_screen_mode), SM_H(Game_screen_mode));
	}
}

static void input_config_sensitivity()
{
	newmenu_item m[33];
	int i = 0, nitems = 0, keysens = 0, joysens = 0, joydead = 0, mousesens = 0, mousefsdead;

	nm_set_item_text(& m[nitems++], "Keyboard Sensitivity:");
	keysens = nitems;
	nm_set_item_slider(&m[nitems], TXT_TURN_LR, PlayerCfg.KeyboardSens[0], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_PITCH_UD, PlayerCfg.KeyboardSens[1], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_SLIDE_LR, PlayerCfg.KeyboardSens[2], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_SLIDE_UD, PlayerCfg.KeyboardSens[3], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_BANK_LR, PlayerCfg.KeyboardSens[4], 0, 16); nitems++;
	nm_set_item_text(& m[nitems++], "");
	nm_set_item_text(& m[nitems++], "Joystick Sensitivity:");
	joysens = nitems;
	nm_set_item_slider(&m[nitems], TXT_TURN_LR, PlayerCfg.JoystickSens[0], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_PITCH_UD, PlayerCfg.JoystickSens[1], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_SLIDE_LR, PlayerCfg.JoystickSens[2], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_SLIDE_UD, PlayerCfg.JoystickSens[3], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_BANK_LR, PlayerCfg.JoystickSens[4], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_THROTTLE, PlayerCfg.JoystickSens[5], 0, 16); nitems++;
	nm_set_item_text(& m[nitems], ""); nitems++;
	nm_set_item_text(& m[nitems], "Joystick Deadzone:"); nitems++;
	joydead = nitems;
	nm_set_item_slider(&m[nitems], TXT_TURN_LR, PlayerCfg.JoystickDead[0], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_PITCH_UD, PlayerCfg.JoystickDead[1], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_SLIDE_LR, PlayerCfg.JoystickDead[2], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_SLIDE_UD, PlayerCfg.JoystickDead[3], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_BANK_LR, PlayerCfg.JoystickDead[4], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_THROTTLE, PlayerCfg.JoystickDead[5], 0, 16); nitems++;
	nm_set_item_text(& m[nitems], ""); nitems++;
	nm_set_item_text(& m[nitems], "Mouse Sensitivity:"); nitems++;
	mousesens = nitems;
	nm_set_item_slider(&m[nitems], TXT_TURN_LR, PlayerCfg.MouseSens[0], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_PITCH_UD, PlayerCfg.MouseSens[1], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_SLIDE_LR, PlayerCfg.MouseSens[2], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_SLIDE_UD, PlayerCfg.MouseSens[3], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_BANK_LR, PlayerCfg.MouseSens[4], 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], TXT_THROTTLE, PlayerCfg.MouseSens[5], 0, 16); nitems++;
	nm_set_item_text(& m[nitems], ""); nitems++;
	nm_set_item_text(& m[nitems], "Mouse FlightSim Deadzone:"); nitems++;
	mousefsdead = nitems;
	nm_set_item_slider(&m[nitems], "X/Y", PlayerCfg.MouseFSDead, 0, 16); nitems++;

	newmenu_do1(NULL, "SENSITIVITY & DEADZONE", nitems, m, unused_newmenu_subfunction, unused_newmenu_userdata, 1);

	for (i = 0; i <= 5; i++)
	{
		if (i < 5)
			PlayerCfg.KeyboardSens[i] = m[keysens+i].value;
		PlayerCfg.JoystickSens[i] = m[joysens+i].value;
		PlayerCfg.JoystickDead[i] = m[joydead+i].value;
		PlayerCfg.MouseSens[i] = m[mousesens+i].value;
	}
	PlayerCfg.MouseFSDead = m[mousefsdead].value;
}

static int opt_ic_usejoy = 0, opt_ic_usemouse = 0, opt_ic_confkey = 0, opt_ic_confjoy = 0, opt_ic_confmouse = 0, opt_ic_confweap = 0, opt_ic_mouseflightsim = 0, opt_ic_joymousesens = 0, opt_ic_grabinput = 0, opt_ic_mousefsgauge = 0, opt_ic_help0 = 0, opt_ic_help1 = 0, opt_ic_help2 = 0;
static int input_config_menuset(newmenu *menu,const d_event &event, const unused_newmenu_userdata_t *)
{
	newmenu_item *items = newmenu_get_items(menu);
	int citem = newmenu_get_citem(menu);

	switch (event.type)
	{
		case EVENT_NEWMENU_CHANGED:
			if (citem == opt_ic_usejoy)
				(items[citem].value)?(PlayerCfg.ControlType|=CONTROL_USING_JOYSTICK):(PlayerCfg.ControlType&=~CONTROL_USING_JOYSTICK);
			if (citem == opt_ic_usemouse)
				(items[citem].value)?(PlayerCfg.ControlType|=CONTROL_USING_MOUSE):(PlayerCfg.ControlType&=~CONTROL_USING_MOUSE);
			if (citem == opt_ic_mouseflightsim)
				PlayerCfg.MouseFlightSim = 0;
			if (citem == opt_ic_mouseflightsim+1)
				PlayerCfg.MouseFlightSim = 1;
			if (citem == opt_ic_grabinput)
				GameCfg.Grabinput = items[citem].value;
			if (citem == opt_ic_mousefsgauge)
				PlayerCfg.MouseFSIndicator = items[citem].value;
			break;

		case EVENT_NEWMENU_SELECTED:
			if (citem == opt_ic_confkey)
				kconfig(0, "KEYBOARD");
			if (citem == opt_ic_confjoy)
				kconfig(1, "JOYSTICK");
			if (citem == opt_ic_confmouse)
				kconfig(2, "MOUSE");
			if (citem == opt_ic_confweap)
				kconfig(3, "WEAPON KEYS");
			if (citem == opt_ic_joymousesens)
				input_config_sensitivity();
			if (citem == opt_ic_help0)
				show_help();
			if (citem == opt_ic_help1)
				show_netgame_help();
			if (citem == opt_ic_help2)
				show_newdemo_help();
			return 1;		// stay in menu
			break;

		default:
			break;
	}

	return 0;
}

void input_config()
{
	newmenu_item m[20];
	int nitems = 0;

	opt_ic_usejoy = nitems;
	nm_set_item_checkbox(&m[nitems], "USE JOYSTICK", (PlayerCfg.ControlType&CONTROL_USING_JOYSTICK)); nitems++;
	opt_ic_usemouse = nitems;
	nm_set_item_checkbox(&m[nitems], "USE MOUSE", (PlayerCfg.ControlType&CONTROL_USING_MOUSE)); nitems++;
	nm_set_item_text(& m[nitems], ""); nitems++;
	opt_ic_confkey = nitems;
	nm_set_item_menu(&m[nitems], "CUSTOMIZE KEYBOARD"); nitems++;
	opt_ic_confjoy = nitems;
	nm_set_item_menu(&m[nitems], "CUSTOMIZE JOYSTICK"); nitems++;
	opt_ic_confmouse = nitems;
	nm_set_item_menu(&m[nitems], "CUSTOMIZE MOUSE"); nitems++;
	opt_ic_confweap = nitems;
	nm_set_item_menu(&m[nitems], "CUSTOMIZE WEAPON KEYS"); nitems++;
	nm_set_item_text(& m[nitems], ""); nitems++;
	nm_set_item_text(& m[nitems], "MOUSE CONTROL TYPE:"); nitems++;
	opt_ic_mouseflightsim = nitems;
	nm_set_item_radio(&m[nitems], "normal", !PlayerCfg.MouseFlightSim, 0); nitems++;
	nm_set_item_radio(&m[nitems], "FlightSim", PlayerCfg.MouseFlightSim, 0); nitems++;
	nm_set_item_text(& m[nitems], ""); nitems++;
	opt_ic_joymousesens = nitems;
	nm_set_item_menu(&m[nitems], "SENSITIVITY & DEADZONE"); nitems++;
	nm_set_item_text(& m[nitems], ""); nitems++;
	opt_ic_grabinput = nitems;
	nm_set_item_checkbox(&m[nitems], "Keep Keyboard/Mouse focus", GameCfg.Grabinput); nitems++;
	opt_ic_mousefsgauge = nitems;
	nm_set_item_checkbox(&m[nitems], "Mouse FlightSim Indicator", PlayerCfg.MouseFSIndicator); nitems++;
	nm_set_item_text(& m[nitems], ""); nitems++;
	opt_ic_help0 = nitems;
	nm_set_item_menu(&m[nitems], "GAME SYSTEM KEYS"); nitems++;
	opt_ic_help1 = nitems;
	nm_set_item_menu(&m[nitems], "NETGAME SYSTEM KEYS"); nitems++;
	opt_ic_help2 = nitems;
	nm_set_item_menu(&m[nitems], "DEMO SYSTEM KEYS"); nitems++;

	newmenu_do1(NULL, TXT_CONTROLS, nitems, m, input_config_menuset, unused_newmenu_userdata, 3);
}

static void reticle_config()
{
#ifdef OGL
	newmenu_item m[18];
#else
	newmenu_item m[17];
#endif
	int nitems = 0, i, opt_ret_type, opt_ret_rgba, opt_ret_size;
	nm_set_item_text(& m[nitems], "Reticle Type:"); nitems++;
	opt_ret_type = nitems;
	nm_set_item_radio(&m[nitems], "Classic", 0, 0); nitems++;
#ifdef OGL
	nm_set_item_radio(&m[nitems], "Classic Reboot", 0, 0); nitems++;
#endif
	nm_set_item_radio(&m[nitems], "None", 0, 0); nitems++;
	nm_set_item_radio(&m[nitems], "X", 0, 0); nitems++;
	nm_set_item_radio(&m[nitems], "Dot", 0, 0); nitems++;
	nm_set_item_radio(&m[nitems], "Circle", 0, 0); nitems++;
	nm_set_item_radio(&m[nitems], "Cross V1", 0, 0); nitems++;
	nm_set_item_radio(&m[nitems], "Cross V2", 0, 0); nitems++;
	nm_set_item_radio(&m[nitems], "Angle", 0, 0); nitems++;
	nm_set_item_text(& m[nitems], ""); nitems++;
	nm_set_item_text(& m[nitems], "Reticle Color:"); nitems++;
	opt_ret_rgba = nitems;
	nm_set_item_slider(&m[nitems], "Red", (PlayerCfg.ReticleRGBA[0]/2), 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], "Green", (PlayerCfg.ReticleRGBA[1]/2), 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], "Blue", (PlayerCfg.ReticleRGBA[2]/2), 0, 16); nitems++;
	nm_set_item_slider(&m[nitems], "Alpha", (PlayerCfg.ReticleRGBA[3]/2), 0, 16); nitems++;
	nm_set_item_text(& m[nitems], ""); nitems++;
	opt_ret_size = nitems;
	nm_set_item_slider(&m[nitems], "Reticle Size:", PlayerCfg.ReticleSize, 0, 4); nitems++;

	i = PlayerCfg.ReticleType;
#ifndef OGL
	if (i > 1) i--;
#endif
	m[opt_ret_type+i].value=1;

	newmenu_do1( NULL, "Reticle Options", nitems, m, unused_newmenu_subfunction, unused_newmenu_userdata, 1 );

#ifdef OGL
	for (i = 0; i < 9; i++)
		if (m[i+opt_ret_type].value)
			PlayerCfg.ReticleType = i;
#else
	for (i = 0; i < 8; i++)
		if (m[i+opt_ret_type].value)
			PlayerCfg.ReticleType = i;
	if (PlayerCfg.ReticleType > 1) PlayerCfg.ReticleType++;
#endif
	for (i = 0; i < 4; i++)
		PlayerCfg.ReticleRGBA[i] = (m[i+opt_ret_rgba].value*2);
	PlayerCfg.ReticleSize = m[opt_ret_size].value;
}

int opt_gr_texfilt, opt_gr_brightness, opt_gr_reticlemenu, opt_gr_alphafx, opt_gr_dynlightcolor, opt_gr_vsync, opt_gr_multisample, opt_gr_fpsindi;
#if defined(DXX_BUILD_DESCENT_II)
int opt_gr_movietexfilt;
#endif
static int graphics_config_menuset(newmenu *menu,const d_event &event, const unused_newmenu_userdata_t *)
{
	newmenu_item *items = newmenu_get_items(menu);
	int citem = newmenu_get_citem(menu);

	switch (event.type)
	{
		case EVENT_NEWMENU_CHANGED:
			if ( citem == opt_gr_texfilt + 3
#ifdef OGL
				&& ogl_maxanisotropy <= 1.0
#endif
				)
			{
				nm_messagebox( TXT_ERROR, 1, TXT_OK, "Anisotropic Filtering not\nsupported by your hardware/driver.");
				items[opt_gr_texfilt + 3].value = 0;
				items[opt_gr_texfilt + 2].value = 1;
			}
			if ( citem == opt_gr_brightness)
				gr_palette_set_gamma(items[citem].value);
			break;

		case EVENT_NEWMENU_SELECTED:
			if (citem == opt_gr_reticlemenu)
				reticle_config();
			return 1;		// stay in menu
			break;

		default:
			break;
	}

	return 0;
}

void graphics_config()
{
#ifdef OGL
#if defined(DXX_BUILD_DESCENT_I)
	newmenu_item m[13];
#elif defined(DXX_BUILD_DESCENT_II)
	newmenu_item m[14];
#endif
	int i = 0;
#else
	newmenu_item m[3];
#endif
	int nitems = 0;

#ifdef OGL
	nm_set_item_text(& m[nitems], "Texture Filtering:"); nitems++;
	opt_gr_texfilt = nitems;
	nm_set_item_radio(&m[nitems++], "None (Classical)", 0, 0);
	nm_set_item_radio(&m[nitems++], "Bilinear", 0, 0);
	nm_set_item_radio(&m[nitems++], "Trilinear", 0, 0);
	nm_set_item_radio(&m[nitems++], "Anisotropic", 0, 0);
#if defined(DXX_BUILD_DESCENT_II)
	opt_gr_movietexfilt = nitems;
	nm_set_item_checkbox(&m[nitems++], "Movie Filter", GameCfg.MovieTexFilt);
#endif
	nm_set_item_text(& m[nitems], ""); nitems++;
#endif
	opt_gr_brightness = nitems;
	nm_set_item_slider(&m[nitems], TXT_BRIGHTNESS, gr_palette_get_gamma(), 0, 16); nitems++;
	opt_gr_reticlemenu = nitems;
	nm_set_item_menu(&m[nitems], "Reticle Options"); nitems++;
#ifdef OGL
	opt_gr_alphafx = nitems;
	nm_set_item_checkbox(&m[nitems], "Transparency Effects", PlayerCfg.AlphaEffects); nitems++;
	opt_gr_dynlightcolor = nitems;
	nm_set_item_checkbox(&m[nitems], "Colored Dynamic Light", PlayerCfg.DynLightColor); nitems++;
	opt_gr_vsync = nitems;
	nm_set_item_checkbox(&m[nitems],"VSync", GameCfg.VSync); nitems++;
	opt_gr_multisample = nitems;
	nm_set_item_checkbox(&m[nitems],"4x multisampling", GameCfg.Multisample); nitems++;
#endif
	opt_gr_fpsindi = nitems;
	nm_set_item_checkbox(&m[nitems],"FPS Counter", GameCfg.FPSIndicator); nitems++;
#ifdef OGL
	m[opt_gr_texfilt+GameCfg.TexFilt].value=1;
#endif

	newmenu_do1( NULL, "Graphics Options", nitems, m, graphics_config_menuset, unused_newmenu_userdata, 1 );

#ifdef OGL
	if (GameCfg.VSync != m[opt_gr_vsync].value || GameCfg.Multisample != m[opt_gr_multisample].value)
		nm_messagebox( NULL, 1, TXT_OK, "Setting VSync or 4x Multisample\nrequires restart on some systems.");

	for (i = 0; i <= 3; i++)
		if (m[i+opt_gr_texfilt].value)
			GameCfg.TexFilt = i;
#if defined(DXX_BUILD_DESCENT_II)
	GameCfg.MovieTexFilt = m[opt_gr_movietexfilt].value;
#endif
	PlayerCfg.AlphaEffects = m[opt_gr_alphafx].value;
	PlayerCfg.DynLightColor = m[opt_gr_dynlightcolor].value;
	GameCfg.VSync = m[opt_gr_vsync].value;
	GameCfg.Multisample = m[opt_gr_multisample].value;
#endif
	GameCfg.GammaLevel = m[opt_gr_brightness].value;
	GameCfg.FPSIndicator = m[opt_gr_fpsindi].value;
#ifdef OGL
	gr_set_attributes();
	gr_set_mode(Game_screen_mode);
#endif
}

#if PHYSFS_VER_MAJOR >= 2
struct browser
{
	const char	*title;			// The title - needed for making another listbox when changing directory
	int		(*when_selected)(void *userdata, const char *filename);	// What to do when something chosen
	void	*userdata;		// Whatever you want passed to when_selected
	string_array_t list;
	const file_extension_t *ext_list;		// List of file extensions we're looking for (if looking for a music file many types are possible)
	int		select_dir;		// Allow selecting the current directory (e.g. for Jukebox level song directory)
	char	view_path[PATH_MAX];	// The absolute path we're currently looking at
	int		new_path;		// Whether the view_path is a new searchpath, if so, remove it when finished
};

static void list_dir_el(void *vb, const char *, const char *fname)
{
	browser *b = reinterpret_cast<browser *>(vb);
	const char *r = PHYSFS_getRealDir(fname);
	if (!r)
		r = "";
	if (!strcmp(r, b->view_path) && (PHYSFS_isDirectory(fname) || PHYSFSX_checkMatchingExtension(b->ext_list, fname))
#if defined(__MACH__) && defined(__APPLE__)
		&& d_stricmp(fname, "Volumes")	// this messes things up, use '..' instead
#endif
		)
		b->list.add(fname);
}

static int list_directory(browser *b)
{
	b->list.clear();
	b->list.add("..");		// go to parent directory
	if (b->select_dir)
	{
		b->list.add("<this directory>");	// choose the directory being viewed
	}
	
	PHYSFS_enumerateFilesCallback("", list_dir_el, b);
	b->list.tidy(1 + (b->select_dir ? 1 : 0),
#ifdef __linux__
					  strcmp
#else
					  d_stricmp
#endif
					  );
					  
	return 1;
}

static int select_file_handler(listbox *menu,const d_event &event, browser *b)
{
	char newpath[PATH_MAX];
	const char **list = listbox_get_items(menu);
	int citem = listbox_get_citem(menu);
	const char *sep = PHYSFS_getDirSeparator();

	memset(newpath, 0, sizeof(char)*PATH_MAX);
	switch (event.type)
	{
#ifdef _WIN32
		case EVENT_KEY_COMMAND:
		{
			if (event_key_get(event) == KEY_CTRLED + KEY_D)
			{
				newmenu_item m;
				char text[4] = "c";
				int rval = 0;

				nm_set_item_input(&m, sizeof(text) - 1, text);
				rval = newmenu_do( NULL, "Enter drive letter", 1, &m, unused_newmenu_subfunction, unused_newmenu_userdata );
				text[1] = '\0'; 
				snprintf(newpath, sizeof(char)*PATH_MAX, "%s:%s", text, sep);
				if (!rval && text[0])
				{
					select_file_recursive(b->title, newpath, b->ext_list, b->select_dir, b->when_selected, b->userdata);
					// close old box.
					window_close(listbox_get_window(menu));
				}
				return 0;
			}
			break;
		}
#endif
		case EVENT_NEWMENU_SELECTED:
			strcpy(newpath, b->view_path);

			if (citem == 0)		// go to parent dir
			{
				char *p;
				
				size_t len_newpath = strlen(newpath);
				size_t len_sep = strlen(sep);
				if ((p = strstr(&newpath[len_newpath - len_sep], sep)))
					if (p != strstr(newpath, sep))	// if this isn't the only separator (i.e. it's not about to look at the root)
						*p = 0;
				
				p = newpath + len_newpath - 1;
				while ((p > newpath) && strncmp(p, sep, len_sep))	// make sure full separator string is matched (typically is)
					p--;
				
				if (p == strstr(newpath, sep))	// Look at root directory next, if not already
				{
#if defined(__MACH__) && defined(__APPLE__)
					if (!d_stricmp(p, "/Volumes"))
						return 1;
#endif
					if (p[len_sep] != '\0')
						p[len_sep] = '\0';
					else
					{
#if defined(__MACH__) && defined(__APPLE__)
						// For Mac OS X, list all active volumes if we leave the root
						strcpy(newpath, "/Volumes");
#else
						return 1;
#endif
					}
				}
				else
					*p = '\0';
			}
			else if (citem == 1 && b->select_dir)
				return !(*b->when_selected)(b->userdata, "");
			else
			{
				size_t len_newpath = strlen(newpath);
				size_t len_sep = strlen(sep);
				if (strncmp(&newpath[len_newpath - len_sep], sep, len_sep))
				{
					strncat(newpath, sep, PATH_MAX - 1 - len_newpath);
					newpath[PATH_MAX - 1] = '\0';
				}
				strncat(newpath, list[citem], PATH_MAX - 1 - len_newpath);
				newpath[PATH_MAX - 1] = '\0';
			}
			
			if ((citem == 0) || PHYSFS_isDirectory(list[citem]))
			{
				// If it fails, stay in this one
				return !select_file_recursive(b->title, newpath, b->ext_list, b->select_dir, b->when_selected, b->userdata);
			}
			
			return !(*b->when_selected)(b->userdata, list[citem]);
			break;
			
		case EVENT_WINDOW_CLOSE:
			if (b->new_path)
				PHYSFS_removeFromSearchPath(b->view_path);

			std::default_delete<browser>()(b);
			break;
			
		default:
			break;
	}
	
	return 0;
}

template <>
int select_file_recursive(const char *title, const char *orig_path, const file_extension_t *ext_list, int select_dir, select_file_subfunction_t<void>::type when_selected, void *userdata)
{
	const char *sep = PHYSFS_getDirSeparator();
	char *p;
	char new_path[PATH_MAX];
	
	auto b = make_unique<browser>();
	b->title = title;
	b->when_selected = when_selected;
	b->userdata = userdata;
	b->ext_list = ext_list;
	b->select_dir = select_dir;
	b->view_path[0] = '\0';
	b->new_path = 1;
	
	// Check for a PhysicsFS path first, saves complication!
	if (orig_path && strncmp(orig_path, sep, strlen(sep)) && PHYSFSX_exists(orig_path,0))
	{
		PHYSFSX_getRealPath(orig_path, new_path);
		orig_path = new_path;
	}

	// Set the viewing directory to orig_path, or some parent of it
	if (orig_path)
	{
		// Must make this an absolute path for directory browsing to work properly
#ifdef _WIN32
		if (!(isalpha(orig_path[0]) && (orig_path[1] == ':')))	// drive letter prompt (e.g. "C:"
#elif defined(macintosh)
		if (orig_path[0] == ':')
#else
		if (orig_path[0] != '/')
#endif
		{
			strncpy(b->view_path, PHYSFS_getBaseDir(), PATH_MAX - 1);		// current write directory must be set to base directory
			b->view_path[PATH_MAX - 1] = '\0';
#ifdef macintosh
			orig_path++;	// go past ':'
#endif
			strncat(b->view_path, orig_path, PATH_MAX - 1 - strlen(b->view_path));
			b->view_path[PATH_MAX - 1] = '\0';
		}
		else
		{
			strncpy(b->view_path, orig_path, PATH_MAX - 1);
			b->view_path[PATH_MAX - 1] = '\0';
		}

		p = b->view_path + strlen(b->view_path) - 1;
		b->new_path = PHYSFSX_isNewPath(b->view_path);
		
		while (!PHYSFS_addToSearchPath(b->view_path, 0))
		{
			size_t len_sep = strlen(sep);
			while ((p > b->view_path) && strncmp(p, sep, len_sep))
				p--;
			*p = '\0';
			
			if (p == b->view_path)
				break;
			
			b->new_path = PHYSFSX_isNewPath(b->view_path);
		}
	}
	
	// Set to user directory if we couldn't find a searchpath
	if (!b->view_path[0])
	{
		strncpy(b->view_path, PHYSFS_getUserDir(), PATH_MAX - 1);
		b->view_path[PATH_MAX - 1] = '\0';
		b->new_path = PHYSFSX_isNewPath(b->view_path);
		if (!PHYSFS_addToSearchPath(b->view_path, 0))
		{
			return 0;
		}
	}
	
	if (!list_directory(b.get()))
	{
		return 0;
	}
	
	auto pb = b.get();
	return newmenu_listbox1(title, pb->list.ptr.size(), &pb->list.ptr[0], 1, 0, select_file_handler, std::move(b)) != NULL;
}

#define BROWSE_TXT " (browse...)"
static inline void nm_set_item_browse(newmenu_item *ni, const char *text)
{
	nm_set_item_menu(ni, text);
}

#else

template <>
int select_file_recursive(const char *title, const char *orig_path, const file_extension_t *ext_list, int select_dir, int (*when_selected)(void *userdata, const char *filename), void *userdata)
{
	return 0;
}

#define BROWSE_TXT
static inline void nm_set_item_browse(newmenu_item *ni, const char *text)
{
	nm_set_item_text(ni, text);
}

#endif

int opt_sm_digivol = -1, opt_sm_musicvol = -1, opt_sm_revstereo = -1, opt_sm_mtype0 = -1, opt_sm_mtype1 = -1, opt_sm_mtype2 = -1, opt_sm_mtype3 = -1, opt_sm_redbook_playorder = -1, opt_sm_mtype3_lmpath = -1, opt_sm_mtype3_lmplayorder1 = -1, opt_sm_mtype3_lmplayorder2 = -1, opt_sm_mtype3_lmplayorder3 = -1, opt_sm_cm_mtype3_file1_b = -1, opt_sm_cm_mtype3_file1 = -1, opt_sm_cm_mtype3_file2_b = -1, opt_sm_cm_mtype3_file2 = -1, opt_sm_cm_mtype3_file3_b = -1, opt_sm_cm_mtype3_file3 = -1, opt_sm_cm_mtype3_file4_b = -1, opt_sm_cm_mtype3_file4 = -1, opt_sm_cm_mtype3_file5_b = -1, opt_sm_cm_mtype3_file5 = -1;

#ifdef USE_SDLMIXER
static int get_absolute_path(char *full_path, const char *rel_path)
{
	PHYSFSX_getRealPath(rel_path, full_path);
	return 1;
}

#define SELECT_SONG(t, s)	select_file_recursive(t, GameCfg.CMMiscMusic[s], jukebox_exts, 0, get_absolute_path, GameCfg.CMMiscMusic[s])
#endif

static int sound_menuset(newmenu *menu,const d_event &event, const unused_newmenu_userdata_t *)
{
	newmenu_item *items = newmenu_get_items(menu);
	int citem = newmenu_get_citem(menu);
	//int nitems = newmenu_get_nitems(menu);
	int replay = 0;
	int rval = 0;

	switch (event.type)
	{
		case EVENT_NEWMENU_CHANGED:
			if (citem == opt_sm_digivol)
			{
				GameCfg.DigiVolume = items[citem].value;
				digi_set_digi_volume( (GameCfg.DigiVolume*32768)/8 );
				digi_play_sample_once( SOUND_DROP_BOMB, F1_0 );
			}
			else if (citem == opt_sm_musicvol)
			{
				GameCfg.MusicVolume = items[citem].value;
				songs_set_volume(GameCfg.MusicVolume);
			}
			else if (citem == opt_sm_revstereo)
			{
				GameCfg.ReverseStereo = items[citem].value;
			}
			else if (citem == opt_sm_mtype0)
			{
				GameCfg.MusicType = MUSIC_TYPE_NONE;
				replay = 1;
			}
			else if (citem == opt_sm_mtype1)
			{
				GameCfg.MusicType = MUSIC_TYPE_BUILTIN;
				replay = 1;
			}
			else if (citem == opt_sm_mtype2)
			{
				GameCfg.MusicType = MUSIC_TYPE_REDBOOK;
				replay = 1;
			}
#ifdef USE_SDLMIXER
			else if (citem == opt_sm_mtype3)
			{
				GameCfg.MusicType = MUSIC_TYPE_CUSTOM;
				replay = 1;
			}
#endif
			else if (citem == opt_sm_redbook_playorder)
			{
				GameCfg.OrigTrackOrder = items[citem].value;
				replay = (Game_wind != NULL);
			}
#ifdef USE_SDLMIXER
			else if (citem == opt_sm_mtype3_lmplayorder1)
			{
				GameCfg.CMLevelMusicPlayOrder = MUSIC_CM_PLAYORDER_CONT;
				replay = (Game_wind != NULL);
			}
			else if (citem == opt_sm_mtype3_lmplayorder2)
			{
				GameCfg.CMLevelMusicPlayOrder = MUSIC_CM_PLAYORDER_LEVEL;
				replay = (Game_wind != NULL);
			}
			else if (citem == opt_sm_mtype3_lmplayorder3)
			{
				GameCfg.CMLevelMusicPlayOrder = MUSIC_CM_PLAYORDER_RAND;
				replay = (Game_wind != NULL);
			}
#endif
			break;

		case EVENT_NEWMENU_SELECTED:
#ifdef USE_SDLMIXER
#ifdef _WIN32
#define WINDOWS_DRIVE_CHANGE_TEXT	".\nCTRL-D to change drive"
#else
#define WINDOWS_DRIVE_CHANGE_TEXT
#endif
			if (citem == opt_sm_mtype3_lmpath)
			{
				static const file_extension_t ext_list[] = { "m3u", "" };		// select a directory or M3U playlist
				select_file_recursive(
					"Select directory or\nM3U playlist to\n play level music from" WINDOWS_DRIVE_CHANGE_TEXT,
									  GameCfg.CMLevelMusicPath.data(), ext_list, 1,	// look in current music path for ext_list files and allow directory selection
									  get_absolute_path, GameCfg.CMLevelMusicPath.data());	// just copy the absolute path
			}
			else if (citem == opt_sm_cm_mtype3_file1_b)
				SELECT_SONG("Select main menu music" WINDOWS_DRIVE_CHANGE_TEXT, SONG_TITLE);
			else if (citem == opt_sm_cm_mtype3_file2_b)
				SELECT_SONG("Select briefing music" WINDOWS_DRIVE_CHANGE_TEXT, SONG_BRIEFING);
			else if (citem == opt_sm_cm_mtype3_file3_b)
				SELECT_SONG("Select credits music" WINDOWS_DRIVE_CHANGE_TEXT, SONG_CREDITS);
			else if (citem == opt_sm_cm_mtype3_file4_b)
				SELECT_SONG("Select escape sequence music" WINDOWS_DRIVE_CHANGE_TEXT, SONG_ENDLEVEL);
			else if (citem == opt_sm_cm_mtype3_file5_b)
				SELECT_SONG("Select game ending music" WINDOWS_DRIVE_CHANGE_TEXT, SONG_ENDGAME);
#endif
			rval = 1;	// stay in menu
			break;

		case EVENT_WINDOW_CLOSE:
			d_free(items);
			break;

		default:
			break;
	}

	if (replay)
	{
		songs_uninit();

		if (Game_wind)
			songs_play_level_song( Current_level_num, 0 );
		else
			songs_play_song(SONG_TITLE, 1);
	}

	return rval;
}

#ifdef USE_SDLMIXER
#define SOUND_MENU_NITEMS 33
#else
#ifdef _WIN32
#define SOUND_MENU_NITEMS 11
#else
#define SOUND_MENU_NITEMS 10
#endif
#endif

void do_sound_menu()
{
	newmenu_item *m;
	int nitems = 0;
	char old_CMMiscMusic0[PATH_MAX+1];
	const auto old_CMLevelMusicPath = GameCfg.CMLevelMusicPath;
	memset(old_CMMiscMusic0, 0, sizeof(char)*(PATH_MAX+1));
	snprintf(old_CMMiscMusic0, sizeof(old_CMMiscMusic0), "%s", GameCfg.CMMiscMusic[SONG_TITLE]);

	MALLOC(m, newmenu_item, SOUND_MENU_NITEMS);
	if (!m)
		return;

	opt_sm_digivol = nitems;
	nm_set_item_slider(&m[nitems++], TXT_FX_VOLUME, GameCfg.DigiVolume, 0, 8);

	opt_sm_musicvol = nitems;
	nm_set_item_slider(&m[nitems++], "music volume", GameCfg.MusicVolume, 0, 8);

	opt_sm_revstereo = nitems;
	nm_set_item_checkbox(&m[nitems++], TXT_REVERSE_STEREO, GameCfg.ReverseStereo);

	nm_set_item_text(& m[nitems++], "");

	nm_set_item_text(& m[nitems++], "music type:");

	opt_sm_mtype0 = nitems;
	nm_set_item_radio(&m[nitems], "no music", (GameCfg.MusicType == MUSIC_TYPE_NONE), 0); nitems++;

#if defined(USE_SDLMIXER) || defined(_WIN32)
	opt_sm_mtype1 = nitems;
	nm_set_item_radio(&m[nitems], "built-in/addon music", (GameCfg.MusicType == MUSIC_TYPE_BUILTIN), 0); nitems++;
#endif

	opt_sm_mtype2 = nitems;
	nm_set_item_radio(&m[nitems], "cd music", (GameCfg.MusicType == MUSIC_TYPE_REDBOOK), 0); nitems++;

#ifdef USE_SDLMIXER
	opt_sm_mtype3 = nitems;
	nm_set_item_radio(&m[nitems], "jukebox", (GameCfg.MusicType == MUSIC_TYPE_CUSTOM), 0); nitems++;

#endif

	nm_set_item_text(& m[nitems++], "");
#ifdef USE_SDLMIXER
	nm_set_item_text(& m[nitems++], "cd music / jukebox options:");
#else
	nm_set_item_text(& m[nitems++], "cd music options:");
#endif

	opt_sm_redbook_playorder = nitems;
#if defined(DXX_BUILD_DESCENT_I)
#define REDBOOK_PLAYORDER_TEXT	"force mac cd track order"
#elif defined(DXX_BUILD_DESCENT_II)
#define REDBOOK_PLAYORDER_TEXT	"force descent ][ cd track order"
#endif
	nm_set_item_checkbox(&m[nitems++], REDBOOK_PLAYORDER_TEXT, GameCfg.OrigTrackOrder);

#ifdef USE_SDLMIXER
	nm_set_item_text(& m[nitems++], "");

	nm_set_item_text(& m[nitems++], "jukebox options:");

	opt_sm_mtype3_lmpath = nitems;
	nm_set_item_browse(&m[nitems++], "path for level music" BROWSE_TXT);

	nm_set_item_input(m[nitems++], GameCfg.CMLevelMusicPath);

	nm_set_item_text(& m[nitems++], "");

	nm_set_item_text(& m[nitems++], "level music play order:");

	opt_sm_mtype3_lmplayorder1 = nitems;
	nm_set_item_radio(&m[nitems], "continuously", (GameCfg.CMLevelMusicPlayOrder == MUSIC_CM_PLAYORDER_CONT), 1); nitems++;

	opt_sm_mtype3_lmplayorder2 = nitems;
	nm_set_item_radio(&m[nitems], "one track per level", (GameCfg.CMLevelMusicPlayOrder == MUSIC_CM_PLAYORDER_LEVEL), 1); nitems++;

	opt_sm_mtype3_lmplayorder3 = nitems;
	nm_set_item_radio(&m[nitems], "random", (GameCfg.CMLevelMusicPlayOrder == MUSIC_CM_PLAYORDER_RAND), 1); nitems++;

	nm_set_item_text(& m[nitems++], "");

	nm_set_item_text(& m[nitems++], "non-level music:");

	opt_sm_cm_mtype3_file1_b = nitems;
	nm_set_item_browse(&m[nitems++], "main menu" BROWSE_TXT);

	opt_sm_cm_mtype3_file1 = nitems;
	nm_set_item_input(&m[nitems++], NM_MAX_TEXT_LEN-1, GameCfg.CMMiscMusic[SONG_TITLE]);

	opt_sm_cm_mtype3_file2_b = nitems;
	nm_set_item_browse(&m[nitems++], "briefing" BROWSE_TXT);

	opt_sm_cm_mtype3_file2 = nitems;
	nm_set_item_input(&m[nitems++], NM_MAX_TEXT_LEN-1, GameCfg.CMMiscMusic[SONG_BRIEFING]);

	opt_sm_cm_mtype3_file3_b = nitems;
	nm_set_item_browse(&m[nitems++], "credits" BROWSE_TXT);

	opt_sm_cm_mtype3_file3 = nitems;
	nm_set_item_input(&m[nitems++], NM_MAX_TEXT_LEN-1, GameCfg.CMMiscMusic[SONG_CREDITS]);

	opt_sm_cm_mtype3_file4_b = nitems;
	nm_set_item_browse(&m[nitems++], "escape sequence" BROWSE_TXT);

	opt_sm_cm_mtype3_file4 = nitems;
	nm_set_item_input(&m[nitems++], NM_MAX_TEXT_LEN-1, GameCfg.CMMiscMusic[SONG_ENDLEVEL]);

	opt_sm_cm_mtype3_file5_b = nitems;
	nm_set_item_browse(&m[nitems++], "game ending" BROWSE_TXT);

	opt_sm_cm_mtype3_file5 = nitems;
	nm_set_item_input(&m[nitems++], NM_MAX_TEXT_LEN-1, GameCfg.CMMiscMusic[SONG_ENDGAME]);
#endif

	Assert(nitems == SOUND_MENU_NITEMS);

	newmenu_do1( NULL, "Sound Effects & Music", nitems, m, sound_menuset, unused_newmenu_userdata, 0 );

#ifdef USE_SDLMIXER
	if ( ((Game_wind != NULL) && strcmp(old_CMLevelMusicPath.data(), GameCfg.CMLevelMusicPath.data())) || ((Game_wind == NULL) && strcmp(old_CMMiscMusic0, GameCfg.CMMiscMusic[SONG_TITLE])) )
	{
		songs_uninit();

		if (Game_wind)
			songs_play_level_song( Current_level_num, 0 );
		else
			songs_play_song(SONG_TITLE, 1);
	}
#endif
}

#if defined(DXX_BUILD_DESCENT_I)
#define DXX_GAME_SPECIFIC_OPTIONS(VERB)	\
	DXX_##VERB##_CHECK("Show D2-style Prox. Bomb Gauge",opt_d2bomb,PlayerCfg.BombGauge)	\

#elif defined(DXX_BUILD_DESCENT_II)
#define DXX_GAME_SPECIFIC_OPTIONS(VERB)	\
	DXX_##VERB##_CHECK("Missile view",opt_missileview, PlayerCfg.MissileViewEnabled)	\
	DXX_##VERB##_CHECK("Headlight on when picked up", opt_headlighton,PlayerCfg.HeadlightActiveDefault )	\
	DXX_##VERB##_CHECK("Show guided missile in main display", opt_guidedbigview,PlayerCfg.GuidedInBigWindow )	\
	DXX_##VERB##_CHECK("Escort robot hot keys",opt_escorthotkey,PlayerCfg.EscortHotKeys)	\
	DXX_##VERB##_CHECK("Movie Subtitles",opt_moviesubtitle,GameCfg.MovieSubtitles)	\

#endif
#define DXX_MISC_MENU_OPTIONS(VERB)	\
	DXX_##VERB##_CHECK("Ship auto-leveling",opt_autolevel, PlayerCfg.AutoLeveling)	\
	DXX_##VERB##_CHECK("Persistent Debris",opt_persist_debris,PlayerCfg.PersistentDebris)	\
	DXX_##VERB##_CHECK("Screenshots w/o HUD",opt_screenshot,PlayerCfg.PRShot)	\
	DXX_##VERB##_CHECK("No redundant pickup messages",opt_redundant,PlayerCfg.NoRedundancy)	\
	DXX_##VERB##_CHECK("Show Player chat only (Multi)",opt_playerchat,PlayerCfg.MultiMessages)	\
	DXX_##VERB##_CHECK("No Rankings (Multi)",opt_noranking,PlayerCfg.NoRankings)	\
	DXX_##VERB##_CHECK("Free Flight controls in Automap",opt_freeflight, PlayerCfg.AutomapFreeFlight)	\
	DXX_##VERB##_CHECK("No Weapon Autoselect when firing",opt_noautoselect,PlayerCfg.NoFireAutoselect)	\
	DXX_##VERB##_CHECK("Only Cycle Autoselect Weapons",opt_only_autoselect,PlayerCfg.CycleAutoselectOnly)	\
	DXX_GAME_SPECIFIC_OPTIONS(VERB)	\


void do_misc_menu()
{
	enum {
		DXX_MISC_MENU_OPTIONS(ENUM)
	};
	int i = 0;

	do {
		newmenu_item m[DXX_MISC_MENU_OPTIONS(COUNT)];
		DXX_MISC_MENU_OPTIONS(ADD);
		i = newmenu_do1( NULL, "Misc Options", sizeof(m)/sizeof(*m), m, unused_newmenu_subfunction, unused_newmenu_userdata, i );
		DXX_MISC_MENU_OPTIONS(READ);
	} while( i>-1 );

}

#if defined(USE_UDP)
static int multi_player_menu_handler(newmenu *menu,const d_event &event, int *menu_choice)
{
	newmenu_item *items = newmenu_get_items(menu);

	switch (event.type)
	{
		case EVENT_NEWMENU_SELECTED:
			// stay in multiplayer menu, even after having played a game
			return do_option(menu_choice[newmenu_get_citem(menu)]);

		case EVENT_WINDOW_CLOSE:
			d_free(menu_choice);
			d_free(items);
			break;

		default:
			break;
	}

	return 0;
}

void do_multi_player_menu()
{
	int *menu_choice;
	newmenu_item *m;
	int num_options = 0;

	MALLOC(menu_choice, int, 3);
	if (!menu_choice)
		return;

	MALLOC(m, newmenu_item, 3);
	if (!m)
	{
		d_free(menu_choice);
		return;
	}

#ifdef USE_UDP
	ADD_ITEM("HOST GAME", MENU_START_UDP_NETGAME, -1);
#ifdef USE_TRACKER
	ADD_ITEM("FIND LAN/ONLINE GAMES", MENU_JOIN_LIST_UDP_NETGAME, -1);
#else
	ADD_ITEM("FIND LAN GAMES", MENU_JOIN_LIST_UDP_NETGAME, -1);
#endif
	ADD_ITEM("JOIN GAME MANUALLY", MENU_JOIN_MANUAL_UDP_NETGAME, -1);
#endif

	newmenu_do3( NULL, TXT_MULTIPLAYER, num_options, m, multi_player_menu_handler, menu_choice, 0, NULL );
}
#endif

void do_options_menu()
{
	newmenu_item *m;

	MALLOC(m, newmenu_item, 10);
	if (!m)
		return;

	nm_set_item_menu(&m[ 0],"Sound effects & music...");
	nm_set_item_text(&m[ 1],"");
	nm_set_item_menu(&m[ 2],TXT_CONTROLS_);
	nm_set_item_text(&m[ 3],"");
	nm_set_item_menu(&m[ 4],"Screen resolution...");
	nm_set_item_menu(&m[ 5],"Graphics Options...");
	nm_set_item_text(&m[ 6],"");
	nm_set_item_menu(&m[ 7],"Primary autoselect ordering...");
	nm_set_item_menu(&m[ 8],"Secondary autoselect ordering...");
	nm_set_item_menu(&m[ 9],"Misc Options...");

	// Fall back to main event loop
	// Allows clean closing and re-opening when resolution changes
	newmenu_do3( NULL, TXT_OPTIONS, 10, m, options_menuset, unused_newmenu_userdata, 0, NULL );
}

#ifndef RELEASE
static window_event_result polygon_models_viewer_handler(window *wind,const d_event &event, const unused_window_userdata_t *)
{
	static unsigned view_idx;
	int key = 0;
	static vms_angvec ang;

	switch (event.type)
	{
		case EVENT_WINDOW_ACTIVATED:
#if defined(DXX_BUILD_DESCENT_II)
			gr_use_palette_table("groupa.256");
#endif
			key_toggle_repeat(1);
			view_idx = 0;
			ang.p = ang.b = 0;
			ang.h = F0_5-1;
			break;
		case EVENT_KEY_COMMAND:
			key = event_key_get(event);
			switch (key)
			{
				case KEY_ESC:
					window_close(wind);
					return window_event_result::close;
				case KEY_SPACEBAR:
					view_idx ++;
					if (view_idx >= N_polygon_models) view_idx = 0;
					break;
				case KEY_BACKSP:
					if (!view_idx)
						view_idx = N_polygon_models - 1;
					else
						view_idx --;
					break;
				case KEY_A:
					ang.h -= 100;
					break;
				case KEY_D:
					ang.h += 100;
					break;
				case KEY_W:
					ang.p -= 100;
					break;
				case KEY_S:
					ang.p += 100;
					break;
				case KEY_Q:
					ang.b -= 100;
					break;
				case KEY_E:
					ang.b += 100;
					break;
				case KEY_R:
					ang.p = ang.b = 0;
					ang.h = F0_5-1;
					break;
				default:
					break;
			}
			return window_event_result::handled;
		case EVENT_WINDOW_DRAW:
			timer_delay(F1_0/60);
			draw_model_picture(view_idx, &ang);
			gr_set_curfont(GAME_FONT);
			gr_set_fontcolor(BM_XRGB(255,255,255), -1);
			gr_printf(FSPACX(1), FSPACY(1), "ESC: leave\nSPACE/BACKSP: next/prev model (%i/%i)\nA/D: rotate y\nW/S: rotate x\nQ/E: rotate z\nR: reset orientation",view_idx,N_polygon_models-1);
			break;
		case EVENT_WINDOW_CLOSE:
			load_palette(MENU_PALETTE,0,1);
			key_toggle_repeat(0);
			break;
		default:
			break;
	}
	return window_event_result::ignored;
}

static void polygon_models_viewer()
{
	window *wind = window_create(&grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT, polygon_models_viewer_handler, unused_window_userdata);
	if (!wind)
	{
		d_event event = { EVENT_WINDOW_CLOSE };
		polygon_models_viewer_handler(NULL, event, NULL);
		return;
	}

	while (window_exists(wind))
		event_process();
}

static window_event_result gamebitmaps_viewer_handler(window *wind,const d_event &event, const unused_window_userdata_t *)
{
	static int view_idx = 0;
	int key = 0;
#ifdef OGL
	float scale = 1.0;
#endif
	bitmap_index bi;
	grs_bitmap *bm;

	switch (event.type)
	{
		case EVENT_WINDOW_ACTIVATED:
#if defined(DXX_BUILD_DESCENT_II)
			gr_use_palette_table("groupa.256");
#endif
			key_toggle_repeat(1);
			view_idx = 0;
			break;
		case EVENT_KEY_COMMAND:
			key = event_key_get(event);
			switch (key)
			{
				case KEY_ESC:
					window_close(wind);
					return window_event_result::close;
				case KEY_SPACEBAR:
					view_idx ++;
					if (view_idx >= Num_bitmap_files) view_idx = 0;
					break;
				case KEY_BACKSP:
					view_idx --;
					if (view_idx < 0 ) view_idx = Num_bitmap_files - 1;
					break;
				default:
					break;
			}
			return window_event_result::handled;
		case EVENT_WINDOW_DRAW:
			bi.index = view_idx;
			bm = &GameBitmaps[view_idx];
			timer_delay(F1_0/60);
			PIGGY_PAGE_IN(bi);
			gr_clear_canvas( BM_XRGB(0,0,0) );
#ifdef OGL
			scale = (bm->bm_w > bm->bm_h)?(SHEIGHT/bm->bm_w)*0.8:(SHEIGHT/bm->bm_h)*0.8;
			ogl_ubitmapm_cs((SWIDTH/2)-(bm->bm_w*scale/2),(SHEIGHT/2)-(bm->bm_h*scale/2),bm->bm_w*scale,bm->bm_h*scale,*bm,-1,F1_0);
#else
			gr_bitmap((SWIDTH/2)-(bm->bm_w/2), (SHEIGHT/2)-(bm->bm_h/2), *bm);
#endif
			gr_set_curfont(GAME_FONT);
			gr_set_fontcolor(BM_XRGB(255,255,255), -1);
			gr_printf(FSPACX(1), FSPACY(1), "ESC: leave\nSPACE/BACKSP: next/prev bitmap (%i/%i)",view_idx,Num_bitmap_files-1);
			break;
		case EVENT_WINDOW_CLOSE:
			load_palette(MENU_PALETTE,0,1);
			key_toggle_repeat(0);
			break;
		default:
			break;
	}
	return window_event_result::ignored;
}

static void gamebitmaps_viewer()
{
	window *wind = window_create(&grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT, gamebitmaps_viewer_handler, unused_window_userdata);
	if (!wind)
	{
		d_event event = { EVENT_WINDOW_CLOSE };
		gamebitmaps_viewer_handler(NULL, event, NULL);
		return;
	}

	while (window_exists(wind))
		event_process();
}

static int sandbox_menuset(newmenu *menu,const d_event &event, const unused_newmenu_userdata_t *)
{
	switch (event.type)
	{
		case EVENT_NEWMENU_CHANGED:
			break;

		case EVENT_NEWMENU_SELECTED:
			switch(newmenu_get_citem(menu))
			{
				case  0: polygon_models_viewer(); break;
				case  1: gamebitmaps_viewer(); break;
			}
			return 1; // stay in menu until escape
			break;

		case EVENT_WINDOW_CLOSE:
		{
			newmenu_item *items = newmenu_get_items(menu);
			d_free(items);
			break;
		}

		default:
			break;
	}
	return 0;
}

void do_sandbox_menu()
{
	newmenu_item *m;

	MALLOC(m, newmenu_item, 2);
	if (!m)
		return;

	nm_set_item_menu(&m[ 0],"Polygon_models viewer");
	nm_set_item_menu(&m[ 1],"GameBitmaps viewer");

	newmenu_do3( NULL, "Coder's sandbox", 2, m, sandbox_menuset, unused_newmenu_userdata, 0, NULL );
}
#endif
