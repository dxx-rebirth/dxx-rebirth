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
#include <SDL.h>

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
#include "player.h"
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
#if DXX_USE_SDLMIXER
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
#if DXX_USE_UDP
#include "net_udp.h"
#endif
#if DXX_USE_EDITOR
#include "editor/editor.h"
#include "editor/kdefs.h"
#endif
#if DXX_USE_OGL
#include "ogl_init.h"
#endif
#include "physfs_list.h"

#include "dxxsconf.h"
#include "dsx-ns.h"
#include "compiler-exchange.h"
#include "compiler-make_unique.h"
#include "compiler-range_for.h"
#include "partial_range.h"

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
#if DXX_USE_UDP
        MENU_MULTIPLAYER,
    #endif

    MENU_SHOW_CREDITS,
    MENU_ORDER_INFO,

#if DXX_USE_UDP
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

#define ADD_ITEM(t,value,key)  do { nm_set_item_menu(m[num_options], t); menu_choice[num_options]=value;num_options++; } while (0)

static array<window *, 16> menus;

// Function Prototypes added after LINTING
static int do_option(int select);
static window_event_result do_new_game_menu(void);
#if DXX_USE_UDP
static void do_multi_player_menu();
#endif
#ifndef RELEASE
static void do_sandbox_menu();
#endif

namespace {

template <typename T>
using select_file_subfunction = window_event_result (*)(T *, const char *);

}

__attribute_nonnull()
static int select_file_recursive2(const char *title, const char *orig_path, const partial_range_t<const file_extension_t *> &ext_list, int select_dir, select_file_subfunction<void> when_selected, void *userdata);

template <typename T>
__attribute_nonnull()
static int select_file_recursive(const char *title, const char *orig_path, const partial_range_t<const file_extension_t *> &ext_list, int select_dir, select_file_subfunction<T> when_selected, T *userdata)
{
	return select_file_recursive2(title, orig_path, ext_list, select_dir, reinterpret_cast<select_file_subfunction<void>>(when_selected), reinterpret_cast<void *>(userdata));
}

// Hide all menus
int hide_menus(void)
{
	window *wind;
	if (menus[0])
		return 0;		// there are already hidden menus

	wind = window_get_front();
	range_for (auto &i, menus)
	{
		i = wind;
		if (!wind)
			break;
		wind = window_set_visible(*wind, 0);
	}
	Assert(window_get_front() == NULL);
	return 1;
}

// Show all menus, with the front one shown first
// This makes sure EVENT_WINDOW_ACTIVATED is only sent to that window
void show_menus(void)
{
	range_for (auto &i, menus)
	{
		if (!i)
			break;
		
		// Hidden windows don't receive events, so the only way to close is outside its handler
		// Which there should be no cases of here
		// window_exists could return a false positive if a new window was created
		// with the same pointer value as the deleted one, so killing window_exists (call and function)
		// if (window_exists(i))
		window_set_visible(exchange(i, nullptr), 1);
	}
}

namespace dcx {

/* This is a hack to prevent writing to freed memory.  Various points in
 * the game code call `hide_menus()`, then later use `show_menus()` to
 * reverse the effect.  If the forcibly hidden window is deleted before
 * `show_menus()` is called, the attempt to show it would write to freed
 * memory.  This hook is called when a window is deleted, so that the
 * deleted window can be removed from menus[].  Removing it from menus[]
 * prevents `show_menus()` trying to make it visible later.
 *
 * It would be cleaner, but more invasive, to restructure the code so
 * that the menus[] array does not need to exist and window pointers are
 * not stored outside the control of their owner.
 */
void menu_destroy_hook(window *w)
{
	const auto &&e = menus.end();
	const auto &&i = std::find(menus.begin(), e, w);
	if (i == e)
		/* Not a hidden menu */
		return;
	/* This is not run often enough to merit a clever loop that stops
	 * when it reaches an unused element.
	 */
	std::move(std::next(i), e, i);
	menus.back() = nullptr;
}

//pairs of chars describing ranges
constexpr char playername_allowed_chars[] = "azAZ09__--";

}

static int MakeNewPlayerFile(int allow_abort)
{
	int x;
	char filename[PATH_MAX];
	callsign_t text = get_local_player().callsign;

try_again:
	{
		array<newmenu_item, 1> m{{
			nm_item_input(text.buffer()),
		}};
	Newmenu_allowed_chars = playername_allowed_chars;
		x = newmenu_do( NULL, TXT_ENTER_PILOT_NAME, m, unused_newmenu_subfunction, unused_newmenu_userdata );
	}
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

	get_local_player().callsign = text;

	write_player_file();

	return 1;
}

static void delete_player_saved_games(const char * name);

static window_event_result player_menu_keycommand( listbox *lb,const d_event &event )
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

				return window_event_result::handled;
			}
			break;
	}

	return window_event_result::ignored;
}

static window_event_result player_menu_handler( listbox *lb,const d_event &event, char **list )
{
	const char **items = listbox_get_items(lb);
	switch (event.type)
	{
		case EVENT_KEY_COMMAND:
			return player_menu_keycommand(lb, event);
		case EVENT_NEWMENU_SELECTED:
		{
			auto &citem = static_cast<const d_select_event &>(event).citem;
			if (citem < 0)
				return window_event_result::ignored;		// shouldn't happen
			else if (citem == 0)
			{
				// They selected 'create new pilot'
				return MakeNewPlayerFile(1) ? window_event_result::close : window_event_result::handled;
			}
			else
			{
				get_local_player().callsign.copy_lower(items[citem], strlen(items[citem]));
			}
			return window_event_result::close;
			break;
		}

		case EVENT_WINDOW_CLOSE:
			if (read_player_file() != EZERO)
				return window_event_result::handled;		// abort close!

			WriteConfigFile();		// Update lastplr

			PHYSFS_freeList(list);
			d_free(items);
			break;

		default:
			break;
	}

	return window_event_result::ignored;
}

//Inputs the player's name, without putting up the background screen
int RegisterPlayer()
{
	static const array<file_extension_t, 1> types{{"plr"}};
	int i = 0, NumItems;
	int citem = 0;
	int allow_abort_flag = 1;

	auto &plr = get_local_player();
	if (!*static_cast<const char *>(plr.callsign))
	{
		if (!*static_cast<const char *>(GameCfg.LastPlayer))
		{
			plr.callsign = "player";
			allow_abort_flag = 0;
		}
		else
		{
			// Read the last player's name from config file, not lastplr.txt
			plr.callsign = GameCfg.LastPlayer;
		}
	}

	auto list = PHYSFSX_findFiles(PLAYER_DIRECTORY_STRING(""), types);
	if (!list)
		return 0;	// memory error
	if (!list[0])
	{
		MakeNewPlayerFile(0);	// make a new player without showing listbox
		return 0;
	}


	for (NumItems = 0; list[NumItems] != NULL; NumItems++) {}
	NumItems++;		// for TXT_CREATE_NEW

	RAIIdmem<const char *[]> m;
	MALLOC(m, const char *[], NumItems);
	if (m == NULL)
	{
		return 0;
	}

	m[i++] = TXT_CREATE_NEW;

	range_for (const auto f, list)
	{
		char *p;

		size_t lenf = strlen(f);
		if (lenf > FILENAME_LEN-1 || lenf < 5) // sorry guys, can only have up to eight chars for the player name
		{
			NumItems--;
			continue;
		}
		m[i++] = f;
		p = strchr(f, '.');
		if (p)
			*p = '\0';		// chop the .plr
	}

	if (NumItems <= 1) // so it seems all plr files we found were too long. funny. let's make a real player
	{
		MakeNewPlayerFile(0);	// make a new player without showing listbox
		return 0;
	}

	// Sort by name, except the <Create New Player> string
	qsort(&m[1], NumItems - 1, sizeof(char *), string_array_sort_func);

	for ( i=0; i<NumItems; i++ )
		if (!d_stricmp(static_cast<const char *>(get_local_player().callsign), m[i]) )
			citem = i;

	newmenu_listbox1(TXT_SELECT_PILOT, NumItems, m.release(), allow_abort_flag, citem, player_menu_handler, list.release());
	return 1;
}

// Draw Copyright and Version strings
static void draw_copyright()
{
	gr_set_default_canvas();
	auto &canvas = *grd_curcanv;
	auto &game_font = *GAME_FONT;
	gr_set_fontcolor(canvas, BM_XRGB(6, 6, 6), -1);
	const auto &&line_spacing = LINE_SPACING(game_font, game_font);
	gr_string(canvas, game_font, 0x8000, SHEIGHT - line_spacing, TXT_COPYRIGHT);
	gr_set_fontcolor(canvas, BM_XRGB(25, 0, 0), -1);
	gr_string(canvas, game_font, 0x8000, SHEIGHT - (line_spacing * 2), DESCENT_VERSION);
}

namespace dsx {

// ------------------------------------------------------------------------
static int main_menu_handler(newmenu *menu,const d_event &event, int *menu_choice )
{
	newmenu_item *items = newmenu_get_items(menu);

	switch (event.type)
	{
		case EVENT_WINDOW_ACTIVATED:
			load_palette(MENU_PALETTE,0,1);		//get correct palette

			if (!*static_cast<const char *>(get_local_player().callsign))
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
			if (keyd_time_when_last_pressed + i2f(DXX_DEMO_KEY_DELAY) < timer_query() || CGameArg.SysAutoDemo)
			{
				keyd_time_when_last_pressed = timer_query();			// Reset timer so that disk won't thrash if no demos.

#if defined(DXX_BUILD_DESCENT_II)
				int n_demos = newdemo_count_demos();
				if ((d_rand() % (n_demos+1)) == 0 && !CGameArg.SysAutoDemo)
				{
#if DXX_USE_OGL
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
		{
			auto &citem = static_cast<const d_select_event &>(event).citem;
			return do_option(menu_choice[citem]);
		}

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
#if DXX_USE_UDP
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
#if DXX_USE_EDITOR
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
			state_restore_all(0, secret_restore::none, nullptr, blind_save::no);
			break;
#if DXX_USE_EDITOR
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
#if DXX_USE_EDITOR
			if (! SafetyCheck()) break;
			#endif
			return 0;

		case MENU_NEW_PLAYER:
			RegisterPlayer();
			break;

#if DXX_USE_UDP
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
#if DXX_USE_UDP
		case MENU_MULTIPLAYER:
			do_multi_player_menu();
			break;
#endif
		case MENU_CONFIG:
			do_options_menu();
			break;
		case MENU_SHOW_CREDITS:
			credits_show();
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

static window_event_result demo_menu_keycommand( listbox *lb,const d_event &event )
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

				return window_event_result::handled;
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

				return window_event_result::handled;
			}
			break;
	}

	return window_event_result::ignored;
}

static window_event_result demo_menu_handler(listbox *lb, const d_event &event, char **items)
{
	switch (event.type)
	{
		case EVENT_KEY_COMMAND:
			return demo_menu_keycommand(lb, event);
		case EVENT_NEWMENU_SELECTED:
		{
			auto &citem = static_cast<const d_select_event &>(event).citem;
			if (citem < 0)
				return window_event_result::ignored;		// shouldn't happen
			newdemo_start_playback(items[citem]);
			return window_event_result::handled;		// stay in demo selector
		}
		case EVENT_WINDOW_CLOSE:
			PHYSFS_freeList(items);
			break;
		default:
			break;
	}
	return window_event_result::ignored;
}

int select_demo(void)
{
	int NumItems;

	auto list = PHYSFSX_findFiles(DEMO_DIR, demo_file_extensions);
	if (!list)
		return 0;	// memory error
	if (!list[0])
	{
		nm_messagebox( NULL, 1, TXT_OK, "%s %s\n%s", TXT_NO_DEMO_FILES, TXT_USE_F5, TXT_TO_CREATE_ONE);
		return 0;
	}

	for (NumItems = 0; list[NumItems] != NULL; NumItems++) {}

	// Sort by name
	qsort(list.get(), NumItems, sizeof(char *), string_array_sort_func);

	auto clist = const_cast<const char **>(list.get());
	newmenu_listbox1(TXT_SELECT_DEMO, NumItems, clist, 1, 0, demo_menu_handler, list.release());

	return 1;
}

static int do_difficulty_menu()
{
	array<newmenu_item, NDL> m{{
		nm_item_menu(MENU_DIFFICULTY_TEXT(0)),
		nm_item_menu(MENU_DIFFICULTY_TEXT(1)),
		nm_item_menu(MENU_DIFFICULTY_TEXT(2)),
		nm_item_menu(MENU_DIFFICULTY_TEXT(3)),
		nm_item_menu(MENU_DIFFICULTY_TEXT(4)),
	}};

	const unsigned s = newmenu_do1(nullptr, TXT_DIFFICULTY_LEVEL, m.size(), &m.front(), unused_newmenu_subfunction, unused_newmenu_userdata, Difficulty_level);

	if (s <= Difficulty_4)
	{
		const auto d = static_cast<Difficulty_level_type>(s);
		if (d != Difficulty_level)
		{
			PlayerCfg.DefaultDifficulty = d;
			write_player_file();
		}
		Difficulty_level = d;
		return 1;
	}
	return 0;
}

window_event_result do_new_game_menu()
{
	int new_level_num,player_highest_level;

	new_level_num = 1;
#ifdef NDEBUG
	player_highest_level = get_highest_level();

	if (player_highest_level > Last_level)
#endif
		player_highest_level = Last_level;
	if (player_highest_level > 1) {
		char info_text[80];
		int choice;
		int valid = 0;

		snprintf(info_text,sizeof(info_text),"%s %d",TXT_START_ANY_LEVEL, player_highest_level);
		while (!valid)
		{
			array<char, 10> num_text{"1"};
			array<newmenu_item, 2> m{{
				nm_item_text(info_text),
				nm_item_input(num_text),
			}};
			choice = newmenu_do( NULL, TXT_SELECT_START_LEV, m, unused_newmenu_subfunction, unused_newmenu_userdata );

			if (choice==-1 || m[1].text[0]==0)
				return window_event_result::handled;

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
		return window_event_result::handled;

	StartNewGame(new_level_num);

	return window_event_result::close;	// exit mission listbox
}

static void do_sound_menu();
static void input_config();
static void change_res();
namespace dsx {
static void hud_config();
static void graphics_config();
}
static void gameplay_config();

#define DXX_OPTIONS_MENU(VERB)	\
	DXX_MENUITEM(VERB, MENU, "Sound & music...", sfx)	\
	DXX_MENUITEM(VERB, MENU, TXT_CONTROLS_, controls)	\
	DXX_MENUITEM(VERB, MENU, "Graphics...", graphics)	\
	DXX_MENUITEM(VERB, MENU, "Gameplay...", misc)	\

namespace {

class options_menu_items
{
public:
	enum
	{
		DXX_OPTIONS_MENU(ENUM)
	};
	array<newmenu_item, DXX_OPTIONS_MENU(COUNT)> m;
	options_menu_items()
	{
		DXX_OPTIONS_MENU(ADD);
	}
};

}

static int options_menuset(newmenu *, const d_event &event, options_menu_items *items)
{
	switch (event.type)
	{
		case EVENT_NEWMENU_CHANGED:
			break;

		case EVENT_NEWMENU_SELECTED:
		{
			auto &citem = static_cast<const d_select_event &>(event).citem;
			switch (citem)
			{
				case options_menu_items::sfx:
					do_sound_menu();
					break;
				case options_menu_items::controls:
					input_config();
					break;
				case options_menu_items::graphics:
					graphics_config();
					break;
				case options_menu_items::misc:
					gameplay_config();
					break;
			}
			return 1;	// stay in menu until escape
		}

		case EVENT_WINDOW_CLOSE:
		{
			std::default_delete<options_menu_items>()(items);
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
	newmenu_item m[50+8];
	array<char, 12> crestext, casptext;

	int mc = 0, citem = -1;

#if SDL_MAJOR_VERSION == 1
	array<screen_mode, 50> modes;
	const auto num_presets = gr_list_modes(modes);
	array<array<char, 12>, 50> restext;

	range_for (auto &i, partial_const_range(modes, num_presets))
	{
		const auto &&sm_w = SM_W(i);
		const auto &&sm_h = SM_H(i);
		snprintf(restext[mc].data(), restext[mc].size(), "%ix%i", sm_w, sm_h);
		const auto checked = (citem == -1 && Game_screen_mode == i && GameCfg.AspectY == sm_w / gcd(sm_w, sm_h) && GameCfg.AspectX == sm_h / gcd(sm_w, sm_h));
		if (checked)
			citem = mc;
		nm_set_item_radio(m[mc], restext[mc].data(), checked, 0);
		mc++;
	}

	nm_set_item_text(m[mc], ""); mc++; // little space for overview
	// the fields for custom resolution and aspect
	const auto opt_cval = mc;
#endif
	nm_set_item_radio(m[mc], "use custom values", (citem == -1), 0); mc++;
	nm_set_item_text(m[mc], "resolution:"); mc++;
	snprintf(crestext.data(), crestext.size(), "%ix%i", SM_W(Game_screen_mode), SM_H(Game_screen_mode));
	nm_set_item_input(m[mc], crestext);
	mc++;
	nm_set_item_text(m[mc], "aspect:"); mc++;
	snprintf(casptext.data(), casptext.size(), "%ix%i", GameCfg.AspectY, GameCfg.AspectX);
	nm_set_item_input(m[mc], casptext);
	mc++;
	nm_set_item_text(m[mc], ""); mc++; // little space for overview
	// fullscreen
#if SDL_MAJOR_VERSION == 1
	const auto opt_fullscr = mc;
	nm_set_item_checkbox(m[mc], "Fullscreen", gr_check_fullscreen());
	mc++;
#endif

	// create the menu
	newmenu_do1(NULL, "Screen Resolution", mc, m, unused_newmenu_subfunction, unused_newmenu_userdata, 0);

	// menu is done, now do what we need to do

	// check which resolution field was selected
#if SDL_MAJOR_VERSION == 1
	unsigned i;
	for (i = 0; i <= mc; i++)
		if (m[i].type == NM_TYPE_RADIO && m[i].radio().group == 0 && m[i].value == 1)
			break;

	// now check for fullscreen toggle and apply if necessary
	if (m[opt_fullscr].value != gr_check_fullscreen())
		gr_toggle_fullscreen();
#endif

	screen_mode new_mode;
#if SDL_MAJOR_VERSION == 1
	if (i == opt_cval) // set custom resolution and aspect
#endif
	{
		char revert[32];
		char *x;
		const char *errstr;
		unsigned long w = strtoul(crestext.data(), &x, 10), h;
		screen_mode cmode;
		if (
			((x == crestext.data() || *x != 'x' || !x[1] || ((h = strtoul(x + 1, &x, 10)), *x)) && (errstr = "Entered resolution must\nbe formatted as\n<number>x<number>", true)) ||
			((w < 320 || h < 200) && (errstr = "Entered resolution must\nbe at least 320x200", true))
			)
		{
			cmode = Game_screen_mode;
			w = SM_W(cmode);
			h = SM_H(cmode);
			snprintf(revert, sizeof(revert), "Revert to %lux%lu", w, h);
			nm_messagebox_str(TXT_WARNING, revert, errstr);
		}
		else
		{
			cmode.width = w;
			cmode.height = h;
		}
		auto casp = cmode;
		w = strtoul(casptext.data(), &x, 10);
		if (
			((x == casptext.data() || *x != 'x' || !x[1] || ((h = strtoul(x + 1, &x, 10)), *x)) && (errstr = "Entered aspect ratio must\nbe formatted as\n<number>x<number>", true)) ||
			((!w || !h) && (errstr = "Entered aspect ratio must\nnot use 0 term", true))
			)
		{
			nm_messagebox_str(TXT_WARNING, "IGNORE ASPECT RATIO", errstr);
		}
		else
		{
			// we even have a custom aspect set up
			casp.width = w;
			casp.height = h;
		}
		const auto g = gcd(SM_W(casp), SM_H(casp));
		GameCfg.AspectY = SM_W(casp) / g;
		GameCfg.AspectX = SM_H(casp) / g;
		new_mode = cmode;
	}
#if SDL_MAJOR_VERSION == 1
	else if (i < num_presets) // set preset resolution
	{
		new_mode = modes[i];
		const auto g = gcd(SM_W(new_mode), SM_H(new_mode));
		GameCfg.AspectY = SM_W(new_mode) / g;
		GameCfg.AspectX = SM_H(new_mode) / g;
	}
#endif

	// clean up and apply everything
	newmenu_free_background();
	set_screen_mode(SCREEN_MENU);
	if (new_mode != Game_screen_mode)
	{
		gr_set_mode(new_mode);
		Game_screen_mode = new_mode;
		if (Game_wind) // shortly activate Game_wind so it's canvas will align to new resolution. really minor glitch but whatever
		{
			{
				const d_event event{EVENT_WINDOW_ACTIVATED};
				WINDOW_SEND_EVENT(Game_wind);
			}
			{
				const d_event event{EVENT_WINDOW_DEACTIVATED};
				WINDOW_SEND_EVENT(Game_wind);
			}
		}
	}
	game_init_render_buffers(SM_W(Game_screen_mode), SM_H(Game_screen_mode));
}

static void input_config_keyboard()
{
#define DXX_INPUT_SENSITIVITY(VERB,OPT,VAL)	\
	DXX_MENUITEM(VERB, SLIDER, TXT_TURN_LR, opt_##OPT##_turn_lr, VAL[0], 0, 16)	\
	DXX_MENUITEM(VERB, SLIDER, TXT_PITCH_UD, opt_##OPT##_pitch_ud, VAL[1], 0, 16)	\
	DXX_MENUITEM(VERB, SLIDER, TXT_SLIDE_LR, opt_##OPT##_slide_lr, VAL[2], 0, 16)	\
	DXX_MENUITEM(VERB, SLIDER, TXT_SLIDE_UD, opt_##OPT##_slide_ud, VAL[3], 0, 16)	\
	DXX_MENUITEM(VERB, SLIDER, TXT_BANK_LR, opt_##OPT##_bank_lr, VAL[4], 0, 16)	\

#define DXX_INPUT_CONFIG_MENU(VERB)	\
	DXX_MENUITEM(VERB, TEXT, "Keyboard Sensitivity:", opt_label_kb)	\
	DXX_INPUT_SENSITIVITY(VERB,kb,PlayerCfg.KeyboardSens)	             \


	class menu_items
	{
	public:
		enum
		{
			DXX_INPUT_CONFIG_MENU(ENUM)
		};
		array<newmenu_item, DXX_INPUT_CONFIG_MENU(COUNT)> m;
		menu_items()
		{
			DXX_INPUT_CONFIG_MENU(ADD);
		}
	};
#undef DXX_INPUT_CONFIG_MENU
#undef DXX_INPUT_SENSITIVITY
	menu_items items;
	newmenu_do1(nullptr, "Keyboard Calibration", items.m.size(), items.m.data(), unused_newmenu_subfunction, unused_newmenu_userdata, 1);

	constexpr uint_fast32_t keysens = items.opt_label_kb + 1;
	const auto &m = items.m;

	for (unsigned i = 0; i < 5; i++)
	{
		PlayerCfg.KeyboardSens[i] = m[keysens+i].value;
	}
}

static void input_config_mouse()
{
#define DXX_INPUT_SENSITIVITY(VERB,OPT,VAL)	                           \
	DXX_MENUITEM(VERB, SLIDER, TXT_TURN_LR, opt_##OPT##_turn_lr, VAL[0], 0, 16)	\
	DXX_MENUITEM(VERB, SLIDER, TXT_PITCH_UD, opt_##OPT##_pitch_ud, VAL[1], 0, 16)	\
	DXX_MENUITEM(VERB, SLIDER, TXT_SLIDE_LR, opt_##OPT##_slide_lr, VAL[2], 0, 16)	\
	DXX_MENUITEM(VERB, SLIDER, TXT_SLIDE_UD, opt_##OPT##_slide_ud, VAL[3], 0, 16)	\
	DXX_MENUITEM(VERB, SLIDER, TXT_BANK_LR, opt_##OPT##_bank_lr, VAL[4], 0, 16)	\

#define DXX_INPUT_THROTTLE_SENSITIVITY(VERB,OPT,VAL)	\
	DXX_INPUT_SENSITIVITY(VERB,OPT,VAL)	\
	DXX_MENUITEM(VERB, SLIDER, TXT_THROTTLE, opt_##OPT##_throttle, VAL[5], 0, 16)	\

#define DXX_INPUT_CONFIG_MENU(VERB)	                                   \
	DXX_MENUITEM(VERB, TEXT, "Mouse Sensitivity:", opt_label_ms)	             \
	DXX_INPUT_THROTTLE_SENSITIVITY(VERB,ms,PlayerCfg.MouseSens)	\
	DXX_MENUITEM(VERB, TEXT, "", opt_label_blank_ms)	\
	DXX_MENUITEM(VERB, TEXT, "Mouse Overrun Buffer:", opt_label_mo)	\
	DXX_INPUT_THROTTLE_SENSITIVITY(VERB,mo,PlayerCfg.MouseOverrun)	\
	DXX_MENUITEM(VERB, TEXT, "", opt_label_blank_mo)	\
	DXX_MENUITEM(VERB, TEXT, "Mouse FlightSim Deadzone:", opt_label_mfsd)	\
	DXX_MENUITEM(VERB, SLIDER, "X/Y", opt_mfsd_deadzone, PlayerCfg.MouseFSDead, 0, 16)	\

	class menu_items
	{
	public:
		enum
		{
			DXX_INPUT_CONFIG_MENU(ENUM)
		};
		array<newmenu_item, DXX_INPUT_CONFIG_MENU(COUNT)> m;
		menu_items()
		{
			DXX_INPUT_CONFIG_MENU(ADD);
		}
	};
#undef DXX_INPUT_CONFIG_MENU
	menu_items items;
	newmenu_do1(nullptr, "Mouse Calibration", items.m.size(), items.m.data(), unused_newmenu_subfunction, unused_newmenu_userdata, 1);

	constexpr uint_fast32_t mousesens = items.opt_label_ms + 1;
    constexpr uint_fast32_t mouseoverrun = items.opt_label_mo + 1;
	const auto &m = items.m;

	for (unsigned i = 0; i <= 5; i++)
	{
		
		PlayerCfg.MouseSens[i] = m[mousesens+i].value;
        PlayerCfg.MouseOverrun[i] = m[mouseoverrun+i].value;
	}
	constexpr uint_fast32_t mousefsdead = items.opt_mfsd_deadzone;
	PlayerCfg.MouseFSDead = m[mousefsdead].value;
}

#if DXX_MAX_AXES_PER_JOYSTICK
static void input_config_joystick()
{
#define DXX_INPUT_CONFIG_MENU(VERB)	                                   \
	DXX_MENUITEM(VERB, TEXT, "Joystick Sensitivity:", opt_label_js)	          \
	DXX_INPUT_THROTTLE_SENSITIVITY(VERB,js,PlayerCfg.JoystickSens)	\
	DXX_MENUITEM(VERB, TEXT, "", opt_label_blank_js)	\
	DXX_MENUITEM(VERB, TEXT, "Joystick Linearity:", opt_label_jl)	\
	DXX_INPUT_THROTTLE_SENSITIVITY(VERB,jl,PlayerCfg.JoystickLinear)	  \
	DXX_MENUITEM(VERB, TEXT, "", opt_label_blank_jl)	\
	DXX_MENUITEM(VERB, TEXT, "Joystick Linear Speed:", opt_label_jp)	\
	DXX_INPUT_THROTTLE_SENSITIVITY(VERB,jp,PlayerCfg.JoystickSpeed)	   \
	DXX_MENUITEM(VERB, TEXT, "", opt_label_blank_jp)	\
	DXX_MENUITEM(VERB, TEXT, "Joystick Deadzone:", opt_label_jd)	\
	DXX_INPUT_THROTTLE_SENSITIVITY(VERB,jd,PlayerCfg.JoystickDead)	    \

	class menu_items
	{
	public:
		enum
		{
			DXX_INPUT_CONFIG_MENU(ENUM)
		};
		array<newmenu_item, DXX_INPUT_CONFIG_MENU(COUNT)> m;
		menu_items()
		{
			DXX_INPUT_CONFIG_MENU(ADD);
		}
	};
#undef DXX_INPUT_CONFIG_MENU
	menu_items items;
	newmenu_do1(nullptr, "Joystick Calibration", items.m.size(), items.m.data(), unused_newmenu_subfunction, unused_newmenu_userdata, 1);

	constexpr uint_fast32_t joysens = items.opt_label_js + 1;
	constexpr uint_fast32_t joylin = items.opt_label_jl + 1;
	constexpr uint_fast32_t joyspd = items.opt_label_jp + 1;
	constexpr uint_fast32_t joydead = items.opt_label_jd + 1;
	const auto &m = items.m;

	for (unsigned i = 0; i <= 5; i++)
	{
		PlayerCfg.JoystickLinear[i] = m[joylin+i].value;
		PlayerCfg.JoystickSpeed[i] = m[joyspd+i].value;
		PlayerCfg.JoystickSens[i] = m[joysens+i].value;
		PlayerCfg.JoystickDead[i] = m[joydead+i].value;
	}
}
#endif

#undef DXX_INPUT_THROTTLE_SENSITIVITY
#undef DXX_INPUT_SENSITIVITY

namespace {

class input_config_menu_items
{
#if DXX_MAX_JOYSTICKS
#define DXX_INPUT_CONFIG_JOYSTICK_ITEM(I)	I
#else
#define DXX_INPUT_CONFIG_JOYSTICK_ITEM(I)
#endif

#if DXX_MAX_AXES_PER_JOYSTICK
#define DXX_INPUT_CONFIG_JOYSTICK_AXIS_ITEM(I)	I
#else
#define DXX_INPUT_CONFIG_JOYSTICK_AXIS_ITEM(I)
#endif

#define DXX_INPUT_CONFIG_MENU(VERB)	\
	DXX_INPUT_CONFIG_JOYSTICK_ITEM(DXX_MENUITEM(VERB, CHECK, "Use joystick", opt_ic_usejoy, PlayerCfg.ControlType & CONTROL_USING_JOYSTICK))	\
	DXX_MENUITEM(VERB, CHECK, "Use mouse", opt_ic_usemouse, PlayerCfg.ControlType & CONTROL_USING_MOUSE)	\
	DXX_MENUITEM(VERB, TEXT, "", opt_label_blank_use)	\
	DXX_MENUITEM(VERB, MENU, TXT_CUST_KEYBOARD, opt_ic_confkey)	\
	DXX_INPUT_CONFIG_JOYSTICK_ITEM(DXX_MENUITEM(VERB, MENU, "Customize Joystick", opt_ic_confjoy))	\
	DXX_MENUITEM(VERB, MENU, "Customize Mouse", opt_ic_confmouse)	\
	DXX_MENUITEM(VERB, MENU, "Customize Weapon Keys", opt_ic_confweap)	\
	DXX_MENUITEM(VERB, TEXT, "", opt_label_blank_customize)	\
	DXX_MENUITEM(VERB, TEXT, "Mouse Control Type:", opt_label_mouse_control_type)	\
	DXX_MENUITEM(VERB, RADIO, "Normal", opt_mouse_control_normal, PlayerCfg.MouseFlightSim == 0, optgrp_mouse_control_type)	\
	DXX_MENUITEM(VERB, RADIO, "FlightSim", opt_mouse_control_flightsim, PlayerCfg.MouseFlightSim == 1, optgrp_mouse_control_type)	\
	DXX_MENUITEM(VERB, TEXT, "", opt_label_blank_mouse_control_type)	\
	DXX_MENUITEM(VERB, MENU, "Keyboard Calibration", opt_ic_keyboard)	        \
	DXX_MENUITEM(VERB, MENU, "Mouse Calibration", opt_ic_mouse)	              \
	DXX_INPUT_CONFIG_JOYSTICK_AXIS_ITEM(DXX_MENUITEM(VERB, MENU, "Joystick Calibration", opt_ic_joystick))	         \
	DXX_MENUITEM(VERB, TEXT, "", opt_label_blank_sensitivity_deadzone)	\
	DXX_MENUITEM(VERB, CHECK, "Keep Keyboard/Mouse focus", opt_ic_grabinput, CGameCfg.Grabinput)	\
	DXX_MENUITEM(VERB, CHECK, "Mouse FlightSim Indicator", opt_ic_mousefsgauge, PlayerCfg.MouseFSIndicator)	\
	DXX_MENUITEM(VERB, TEXT, "", opt_label_blank_focus)	\
	DXX_MENUITEM(VERB, TEXT, "When dead, respawn by pressing:", opt_label_respawn_mode)	\
	DXX_MENUITEM(VERB, RADIO, "Any key", opt_respawn_any_key, PlayerCfg.RespawnMode == RespawnPress::Any, optgrp_respawn_mode)	\
	DXX_MENUITEM(VERB, RADIO, "Fire keys (pri., sec., flare)", opt_respawn_fire_key, PlayerCfg.RespawnMode == RespawnPress::Fire, optgrp_respawn_mode)	\
	DXX_MENUITEM(VERB, TEXT, "", opt_label_blank_respawn)	\
	DXX_MENUITEM(VERB, TEXT, "Uncapped turning in:", opt_label_mouselook_mode)	\
	DXX_MENUITEM(VERB, CHECK, "Single player", opt_ic_mouselook_sp, PlayerCfg.MouselookFlags & MouselookMode::Singleplayer)	\
	DXX_MENUITEM(VERB, CHECK, "Multi Coop (if host allows)", opt_ic_mouselook_mp_cooperative, PlayerCfg.MouselookFlags & MouselookMode::MPCoop)	\
	DXX_MENUITEM(VERB, CHECK, "Multi Anarchy (if host allows)", opt_ic_mouselook_mp_anarchy, PlayerCfg.MouselookFlags & MouselookMode::MPAnarchy)	\
	DXX_MENUITEM(VERB, TEXT, "", opt_label_blank_mouselook)	\
	DXX_MENUITEM(VERB, MENU, "GAME SYSTEM KEYS", opt_ic_help0)	\
	DXX_MENUITEM(VERB, MENU, "NETGAME SYSTEM KEYS", opt_ic_help1)	\
	DXX_MENUITEM(VERB, MENU, "DEMO SYSTEM KEYS", opt_ic_help2)	\

public:
	enum
	{
		optgrp_mouse_control_type,
		optgrp_respawn_mode,
	};
	enum
	{
		DXX_INPUT_CONFIG_MENU(ENUM)
	};
	array<newmenu_item, DXX_INPUT_CONFIG_MENU(COUNT)> m;
	input_config_menu_items()
	{
		DXX_INPUT_CONFIG_MENU(ADD);
	}
	static int menuset(newmenu *, const d_event &event, input_config_menu_items *pitems);
#undef DXX_INPUT_CONFIG_MENU
#undef DXX_INPUT_CONFIG_JOYSTICK_AXIS_ITEM
#undef DXX_INPUT_CONFIG_JOYSTICK_ITEM
};

}

int input_config_menu_items::menuset(newmenu *, const d_event &event, input_config_menu_items *pitems)
{
	const auto &items = pitems->m;
	switch (event.type)
	{
		case EVENT_NEWMENU_CHANGED:
		{
			const auto citem = static_cast<const d_change_event &>(event).citem;
			MouselookMode mousemode;
#if DXX_MAX_JOYSTICKS
			if (citem == opt_ic_usejoy)
			{
				constexpr auto flag = CONTROL_USING_JOYSTICK;
				if (items[citem].value)
					PlayerCfg.ControlType |= flag;
				else
					PlayerCfg.ControlType &= ~flag;
			}
#endif
			if (citem == opt_ic_usemouse)
			{
				constexpr auto flag = CONTROL_USING_MOUSE;
				if (items[citem].value)
					PlayerCfg.ControlType |= flag;
				else
					PlayerCfg.ControlType &= ~flag;
			}
			if (citem == opt_mouse_control_normal)
				PlayerCfg.MouseFlightSim = 0;
			if (citem == opt_mouse_control_flightsim)
				PlayerCfg.MouseFlightSim = 1;
			if (citem == opt_ic_grabinput)
				CGameCfg.Grabinput = items[citem].value;
			if (citem == opt_ic_mousefsgauge)
				PlayerCfg.MouseFSIndicator = items[citem].value;
			else if (citem == opt_respawn_any_key)
				PlayerCfg.RespawnMode = RespawnPress::Any;
			else if (citem == opt_respawn_fire_key)
				PlayerCfg.RespawnMode = RespawnPress::Fire;
			else if ((citem == opt_ic_mouselook_sp && (mousemode = MouselookMode::Singleplayer, true)) ||
				(citem == opt_ic_mouselook_mp_cooperative && (mousemode = MouselookMode::MPCoop, true)) ||
				(citem == opt_ic_mouselook_mp_anarchy && (mousemode = MouselookMode::MPAnarchy, true)))
			{
				if (items[citem].value)
					PlayerCfg.MouselookFlags |= mousemode;
				else
					PlayerCfg.MouselookFlags &= ~mousemode;
			}
			break;
		}
		case EVENT_NEWMENU_SELECTED:
		{
			const auto citem = static_cast<const d_select_event &>(event).citem;
			if (citem == opt_ic_confkey)
				kconfig(kconfig_type::keyboard);
#if DXX_MAX_JOYSTICKS
			if (citem == opt_ic_confjoy)
				kconfig(kconfig_type::joystick);
#endif
			if (citem == opt_ic_confmouse)
				kconfig(kconfig_type::mouse);
			if (citem == opt_ic_confweap)
				kconfig(kconfig_type::rebirth);
			if (citem == opt_ic_keyboard)
				input_config_keyboard();
			if (citem == opt_ic_mouse)
				input_config_mouse();
#if DXX_MAX_AXES_PER_JOYSTICK
			if (citem == opt_ic_joystick)
				input_config_joystick();
#endif
			if (citem == opt_ic_help0)
				show_help();
			if (citem == opt_ic_help1)
				show_netgame_help();
			if (citem == opt_ic_help2)
				show_newdemo_help();
			return 1;		// stay in menu
		}

		default:
			break;
	}

	return 0;
}

void input_config()
{
	input_config_menu_items menu_items;
	newmenu_do1(nullptr, TXT_CONTROLS, menu_items.m.size(), menu_items.m.data(), &input_config_menu_items::menuset, &menu_items, menu_items.opt_ic_confkey);
}

static void reticle_config()
{
#if DXX_USE_OGL
#define DXX_RETICLE_TYPE_OGL(VERB)	\
	DXX_MENUITEM(VERB, RADIO, "Classic Reboot", opt_reticle_classic_reboot, 0, optgrp_reticle)
#else
#define DXX_RETICLE_TYPE_OGL(VERB)
#endif
#define DXX_RETICLE_CONFIG_MENU(VERB)	\
	DXX_MENUITEM(VERB, TEXT, "Reticle Type:", opt_label_reticle_type)	\
	DXX_MENUITEM(VERB, RADIO, "Classic", opt_reticle_classic, 0, optgrp_reticle)	\
	DXX_RETICLE_TYPE_OGL(VERB)	\
	DXX_MENUITEM(VERB, RADIO, "None", opt_reticle_none, 0, optgrp_reticle)	\
	DXX_MENUITEM(VERB, RADIO, "X", opt_reticle_x, 0, optgrp_reticle)	\
	DXX_MENUITEM(VERB, RADIO, "Dot", opt_reticle_dot, 0, optgrp_reticle)	\
	DXX_MENUITEM(VERB, RADIO, "Circle", opt_reticle_circle, 0, optgrp_reticle)	\
	DXX_MENUITEM(VERB, RADIO, "Cross V1", opt_reticle_cross1, 0, optgrp_reticle)	\
	DXX_MENUITEM(VERB, RADIO, "Cross V2", opt_reticle_cross2, 0, optgrp_reticle)	\
	DXX_MENUITEM(VERB, RADIO, "Angle", opt_reticle_angle, 0, optgrp_reticle)	\
	DXX_MENUITEM(VERB, TEXT, "", opt_label_blank_reticle_type)	\
	DXX_MENUITEM(VERB, TEXT, "Reticle Color:", opt_label_reticle_color)	\
	DXX_MENUITEM(VERB, SCALE_SLIDER, "Red", opt_reticle_color_red, PlayerCfg.ReticleRGBA[0], 0, 16, 2)	\
	DXX_MENUITEM(VERB, SCALE_SLIDER, "Green", opt_reticle_color_green, PlayerCfg.ReticleRGBA[1], 0, 16, 2)	\
	DXX_MENUITEM(VERB, SCALE_SLIDER, "Blue", opt_reticle_color_blue, PlayerCfg.ReticleRGBA[2], 0, 16, 2)	\
	DXX_MENUITEM(VERB, SCALE_SLIDER, "Alpha", opt_reticle_color_alpha, PlayerCfg.ReticleRGBA[3], 0, 16, 2)	\
	DXX_MENUITEM(VERB, TEXT, "", opt_label_blank_reticle_color)	\
	DXX_MENUITEM(VERB, SLIDER, "Reticle Size:", opt_label_reticle_size, PlayerCfg.ReticleSize, 0, 4)	\

	class menu_items
	{
	public:
		enum
		{
			optgrp_reticle,
		};
		enum
		{
			DXX_RETICLE_CONFIG_MENU(ENUM)
		};
		array<newmenu_item, DXX_RETICLE_CONFIG_MENU(COUNT)> m;
		menu_items()
		{
			DXX_RETICLE_CONFIG_MENU(ADD);
		}
		void read()
		{
			DXX_RETICLE_CONFIG_MENU(READ);
		}
	};
#undef DXX_RETICLE_CONFIG_MENU
#undef DXX_RETICLE_TYPE_OGL
	menu_items items;
	{
	auto i = PlayerCfg.ReticleType;
#if !DXX_USE_OGL
	if (i > 1) i--;
#endif
	items.m[items.opt_reticle_classic + i].value = 1;
	}

	newmenu_do1(nullptr, "Reticle Customization", items.m.size(), items.m.data(), unused_newmenu_subfunction, unused_newmenu_userdata, 1);

	for (uint_fast32_t i = items.opt_reticle_classic; i != items.opt_label_blank_reticle_type; ++i)
		if (items.m[i].value)
		{
#if !DXX_USE_OGL
			if (i != items.opt_reticle_classic)
				++i;
#endif
			PlayerCfg.ReticleType = i - items.opt_reticle_classic;
			break;
		}
	items.read();
}

#if defined(DXX_BUILD_DESCENT_I)
#define DXX_GAME_SPECIFIC_HUDOPTIONS(VERB)	\
	DXX_MENUITEM(VERB, CHECK, "Always-on Bomb Counter",opt_d2bomb,PlayerCfg.BombGauge)	\

#elif defined(DXX_BUILD_DESCENT_II)
enum {
	optgrp_missileview,
};
#define DXX_GAME_SPECIFIC_HUDOPTIONS(VERB)	\
	DXX_MENUITEM(VERB, TEXT, "Missile view:", opt_missileview_label)	\
	DXX_MENUITEM(VERB, RADIO, "Disabled", opt_missileview_none, PlayerCfg.MissileViewEnabled == MissileViewMode::None, optgrp_missileview)	\
	DXX_MENUITEM(VERB, RADIO, "Only own missiles", opt_missileview_selfonly, PlayerCfg.MissileViewEnabled == MissileViewMode::EnabledSelfOnly, optgrp_missileview)	\
	DXX_MENUITEM(VERB, RADIO, "Friendly missiles, preferring self", opt_missileview_selfandallies, PlayerCfg.MissileViewEnabled == MissileViewMode::EnabledSelfAndAllies, optgrp_missileview)	\
	DXX_MENUITEM(VERB, CHECK, "Show guided missile in main display", opt_guidedbigview,PlayerCfg.GuidedInBigWindow )	\

#endif
#define DXX_HUD_MENU_OPTIONS(VERB)	\
        DXX_MENUITEM(VERB, MENU, "Reticle Customization...", opt_hud_reticlemenu)	\
	DXX_MENUITEM(VERB, CHECK, "Screenshots without HUD",opt_screenshot,PlayerCfg.PRShot)	\
	DXX_MENUITEM(VERB, CHECK, "No redundant pickup messages",opt_redundant,PlayerCfg.NoRedundancy)	\
	DXX_MENUITEM(VERB, CHECK, "Show Player chat only (Multi)",opt_playerchat,PlayerCfg.MultiMessages)	\
	DXX_MENUITEM(VERB, CHECK, "Show Player ping (Multi)",opt_playerping,PlayerCfg.MultiPingHud)	\
	DXX_MENUITEM(VERB, CHECK, "Cloak/Invulnerability Timers",opt_cloakinvultimer,PlayerCfg.CloakInvulTimer)	\
	DXX_GAME_SPECIFIC_HUDOPTIONS(VERB)	\

enum {
	DXX_HUD_MENU_OPTIONS(ENUM)
};

static int hud_config_menuset(newmenu *, const d_event &event, const unused_newmenu_userdata_t *)
{
	switch (event.type)
	{
		case EVENT_NEWMENU_SELECTED:
		{
			auto &citem = static_cast<const d_select_event &>(event).citem;
                        if (citem == opt_hud_reticlemenu)
                                reticle_config();
			return 1;		// stay in menu
		}

		default:
			break;
	}

	return 0;
}

namespace dsx {
void hud_config()
{
	int i = 0;

	do {
		newmenu_item m[DXX_HUD_MENU_OPTIONS(COUNT)];
		DXX_HUD_MENU_OPTIONS(ADD);
		i = newmenu_do1( NULL, "Hud Options", sizeof(m)/sizeof(*m), m, hud_config_menuset, unused_newmenu_userdata, 0 );
		DXX_HUD_MENU_OPTIONS(READ);
#if defined(DXX_BUILD_DESCENT_II)
		PlayerCfg.MissileViewEnabled = m[opt_missileview_selfandallies].value
			? MissileViewMode::EnabledSelfAndAllies
			: (m[opt_missileview_selfonly].value
				? MissileViewMode::EnabledSelfOnly
				: MissileViewMode::None);
#endif
	} while( i>-1 );

}
}

#define DXX_GRAPHICS_MENU(VERB)	\
	DXX_MENUITEM(VERB, MENU, "Screen resolution...", opt_gr_screenres)	\
	DXX_MENUITEM(VERB, MENU, "HUD Options...", opt_gr_hudmenu)	\
	DXX_MENUITEM(VERB, SLIDER, TXT_BRIGHTNESS, opt_gr_brightness, gr_palette_get_gamma(), 0, 16)	\
	DXX_MENUITEM(VERB, TEXT, "", blank1)	\
	DXX_OGL0_GRAPHICS_MENU(VERB)	\
	DXX_OGL1_GRAPHICS_MENU(VERB)	\
	DXX_MENUITEM(VERB, CHECK, "FPS Counter", opt_gr_fpsindi, CGameCfg.FPSIndicator)	\

#if DXX_USE_OGL
enum {
	optgrp_texfilt,
};
#define DXX_OGL0_GRAPHICS_MENU(VERB)	\
	DXX_MENUITEM(VERB, TEXT, "Texture Filtering:", opt_gr_texfilt)	\
	DXX_MENUITEM(VERB, RADIO, "Classic", opt_filter_none, 0, optgrp_texfilt)	\
	DXX_MENUITEM(VERB, RADIO, "Blocky Filtered", opt_filter_upscale, 0, optgrp_texfilt)	\
	DXX_MENUITEM(VERB, RADIO, "Smooth", opt_filter_trilinear, 0, optgrp_texfilt)	\
	DXX_MENUITEM(VERB, CHECK, "Anisotropic Filtering", opt_filter_anisotropy, CGameCfg.TexAnisotropy)	\
	D2X_OGL_GRAPHICS_MENU(VERB)	\
	DXX_MENUITEM(VERB, TEXT, "", blank2)	\

#define DXX_OGL1_GRAPHICS_MENU(VERB)	\
	DXX_MENUITEM(VERB, CHECK, "Transparency Effects", opt_gr_alphafx, PlayerCfg.AlphaEffects)	\
	DXX_MENUITEM(VERB, CHECK, "Colored Dynamic Light", opt_gr_dynlightcolor, PlayerCfg.DynLightColor)	\
	DXX_MENUITEM(VERB, CHECK, "VSync", opt_gr_vsync, CGameCfg.VSync)	\
	DXX_MENUITEM(VERB, CHECK, "4x multisampling", opt_gr_multisample, CGameCfg.Multisample)	\

#if defined(DXX_BUILD_DESCENT_I)
#define D2X_OGL_GRAPHICS_MENU(VERB)
#elif defined(DXX_BUILD_DESCENT_II)
#define D2X_OGL_GRAPHICS_MENU(VERB)	\
	DXX_MENUITEM(VERB, CHECK, "Cutscene Smoothing", opt_gr_movietexfilt, GameCfg.MovieTexFilt)
#endif

#else
#define DXX_OGL0_GRAPHICS_MENU(VERB)
#define DXX_OGL1_GRAPHICS_MENU(VERB)
#endif

enum {
	DXX_GRAPHICS_MENU(ENUM)
};

static int graphics_config_menuset(newmenu *, const d_event &event, newmenu_item *const items)
{
	switch (event.type)
	{
		case EVENT_NEWMENU_CHANGED:
		{
			auto &citem = static_cast<const d_change_event &>(event).citem;
			if (citem == opt_gr_brightness)
				gr_palette_set_gamma(items[citem].value);
#if DXX_USE_OGL
			else
			if (citem == opt_filter_anisotropy && ogl_maxanisotropy <= 1.0)
			{
				nm_messagebox( TXT_ERROR, 1, TXT_OK, "Anisotropic Filtering not\nsupported by your hardware/driver.");
				items[opt_filter_anisotropy].value = 0;
			}
#endif
			break;
		}
		case EVENT_NEWMENU_SELECTED:
		{
			auto &citem = static_cast<const d_select_event &>(event).citem;
                        if (citem == opt_gr_screenres)
                                change_res();
			if (citem == opt_gr_hudmenu)
				hud_config();
			return 1;		// stay in menu
		}

		default:
			break;
	}

	return 0;
}

namespace dsx {
void graphics_config()
{
	array<newmenu_item, DXX_GRAPHICS_MENU(COUNT)> m;
	DXX_GRAPHICS_MENU(ADD);

#if DXX_USE_OGL
	m[opt_filter_none+CGameCfg.TexFilt].value=1;
#endif

	newmenu_do1(nullptr, "Graphics Options", m.size(), m.data(), graphics_config_menuset, m.data(), 0);

#if DXX_USE_OGL
	if (CGameCfg.VSync != m[opt_gr_vsync].value || CGameCfg.Multisample != m[opt_gr_multisample].value)
		nm_messagebox( NULL, 1, TXT_OK, "Setting VSync or 4x Multisample\nrequires restart on some systems.");

	for (uint_fast32_t i = 0; i != 3; ++i)
		if (m[i+opt_filter_none].value)
		{
			CGameCfg.TexFilt = i;
			break;
		}
	CGameCfg.TexAnisotropy = m[opt_filter_anisotropy].value;
#if defined(DXX_BUILD_DESCENT_II)
	GameCfg.MovieTexFilt = m[opt_gr_movietexfilt].value;
#endif
	PlayerCfg.AlphaEffects = m[opt_gr_alphafx].value;
	PlayerCfg.DynLightColor = m[opt_gr_dynlightcolor].value;
	CGameCfg.VSync = m[opt_gr_vsync].value;
	CGameCfg.Multisample = m[opt_gr_multisample].value;
#endif
	GameCfg.GammaLevel = m[opt_gr_brightness].value;
	CGameCfg.FPSIndicator = m[opt_gr_fpsindi].value;
#if DXX_USE_OGL
	gr_set_attributes();
	gr_set_mode(Game_screen_mode);
#endif
}
}

#if PHYSFS_VER_MAJOR >= 2
namespace {

struct browser
{
	browser(const partial_range_t<const file_extension_t *> &r) :
		ext_range(r)
	{
	}
	const char	*title;			// The title - needed for making another listbox when changing directory
	window_event_result (*when_selected)(void *userdata, const char *filename);	// What to do when something chosen
	void	*userdata;		// Whatever you want passed to when_selected
	string_array_t list;
	// List of file extensions we're looking for (if looking for a music file many types are possible)
	const partial_range_t<const file_extension_t *> ext_range;
	int		select_dir;		// Allow selecting the current directory (e.g. for Jukebox level song directory)
	int		new_path;		// Whether the view_path is a new searchpath, if so, remove it when finished
	char	view_path[PATH_MAX];	// The absolute path we're currently looking at
};

}

static void list_dir_el(void *vb, const char *, const char *fname)
{
	browser *b = reinterpret_cast<browser *>(vb);
	const char *r = PHYSFS_getRealDir(fname);
	if (!r)
		r = "";
	if (!strcmp(r, b->view_path) && (PHYSFS_isDirectory(fname) || PHYSFSX_checkMatchingExtension(fname, b->ext_range))
#if defined(__APPLE__) && defined(__MACH__)
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

static window_event_result select_file_handler(listbox *menu,const d_event &event, browser *b)
{
	char newpath[PATH_MAX];
	const char **list = listbox_get_items(menu);
	const char *sep = PHYSFS_getDirSeparator();
	memset(newpath, 0, sizeof(char)*PATH_MAX);
	switch (event.type)
	{
#ifdef _WIN32
		case EVENT_KEY_COMMAND:
		{
			if (event_key_get(event) == KEY_CTRLED + KEY_D)
			{
				char text[4] = "c";
				int rval = 0;

				array<newmenu_item, 1> m{{
					nm_item_input(text),
				}};
				rval = newmenu_do( NULL, "Enter drive letter", m, unused_newmenu_subfunction, unused_newmenu_userdata );
				text[1] = '\0'; 
				snprintf(newpath, sizeof(char)*PATH_MAX, "%s:%s", text, sep);
				if (!rval && text[0])
				{
					select_file_recursive(b->title, newpath, b->ext_range, b->select_dir, b->when_selected, b->userdata);
					// close old box.
					return window_event_result::close;
				}
				return window_event_result::handled;
			}
			break;
		}
#endif
		case EVENT_NEWMENU_SELECTED:
		{
			auto &citem = static_cast<const d_select_event &>(event).citem;
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
#if defined(__APPLE__) && defined(__MACH__)
					if (!d_stricmp(p, "/Volumes"))
						return window_event_result::handled;
#endif
					if (p[len_sep] != '\0')
						p[len_sep] = '\0';
					else
					{
#if defined(__APPLE__) && defined(__MACH__)
						// For Mac OS X, list all active volumes if we leave the root
						strcpy(newpath, "/Volumes");
#else
						return window_event_result::handled;
#endif
					}
				}
				else
					*p = '\0';
			}
			else if (citem == 1 && b->select_dir)
				return (*b->when_selected)(b->userdata, "");
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
				return select_file_recursive(b->title, newpath, b->ext_range, b->select_dir, b->when_selected, b->userdata) ? window_event_result::close : window_event_result::handled;
			}
			return (*b->when_selected)(b->userdata, list[citem]);
		}
		case EVENT_WINDOW_CLOSE:
			if (b->new_path)
				PHYSFS_removeFromSearchPath(b->view_path);

			std::default_delete<browser>()(b);
			break;
			
		default:
			break;
	}
	
	return window_event_result::ignored;
}

static int select_file_recursive2(const char *title, const char *orig_path, const partial_range_t<const file_extension_t *> &ext_range, int select_dir, select_file_subfunction<void> when_selected, void *userdata)
{
	const char *sep = PHYSFS_getDirSeparator();
	char *p;
	array<char, PATH_MAX> new_path;
	
	auto b = make_unique<browser>(ext_range);
	b->title = title;
	b->when_selected = when_selected;
	b->userdata = userdata;
	b->select_dir = select_dir;
	b->view_path[0] = '\0';
	b->new_path = 1;
	
	// Check for a PhysicsFS path first, saves complication!
	if (strncmp(orig_path, sep, strlen(sep)) && PHYSFSX_exists(orig_path,0))
	{
		PHYSFSX_getRealPath(orig_path, new_path);
		orig_path = new_path.data();
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
		const size_t len_sep = strlen(sep);
		while (b->new_path = PHYSFSX_isNewPath(b->view_path), !PHYSFS_addToSearchPath(b->view_path, 0))
		{
			while ((p > b->view_path) && strncmp(p, sep, len_sep))
				p--;
			*p = '\0';
			
			if (p == b->view_path)
				break;
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
	return newmenu_listbox1(title, pb->list.pointer().size(), &pb->list.pointer().front(), 1, 0, select_file_handler, std::move(b)) != NULL;
}

#define DXX_MENU_ITEM_BROWSE(VERB, TXT, OPT)	\
	DXX_MENUITEM(VERB, MENU, TXT " (browse...)", OPT)
#else

int select_file_recursive2(const char *title, const char *orig_path, const partial_range_t<const file_extension_t *> &ext_range, int select_dir, int (*when_selected)(void *userdata, const char *filename), void *userdata)
{
	return 0;
}

	/* Include blank string to force a compile error if TXT cannot be
	 * string-pasted
	 */
#define DXX_MENU_ITEM_BROWSE(VERB, TXT, OPT)	\
	DXX_MENUITEM(VERB, TEXT, TXT "", OPT)
#endif

#if DXX_USE_SDLMIXER
static window_event_result get_absolute_path(char *full_path, const char *rel_path)
{
	PHYSFSX_getRealPath(rel_path, full_path, PATH_MAX);
	return window_event_result::close;
}

#define SELECT_SONG(t, s)	select_file_recursive(t, CGameCfg.CMMiscMusic[s].data(), jukebox_exts, 0, get_absolute_path, CGameCfg.CMMiscMusic[s].data())
#endif

namespace {

#if defined(DXX_BUILD_DESCENT_I)
#define REDBOOK_PLAYORDER_TEXT	"force mac cd track order"
#elif defined(DXX_BUILD_DESCENT_II)
#define REDBOOK_PLAYORDER_TEXT	"force descent ][ cd track order"
#endif

#if DXX_USE_SDLMIXER || defined(_WIN32)
#define DXX_SOUND_ADDON_MUSIC_MENU_ITEM(VERB)	\
	DXX_MENUITEM(VERB, RADIO, "Built-in/Addon music", opt_sm_mtype1, GameCfg.MusicType == MUSIC_TYPE_BUILTIN, optgrp_music_type)	\

#else
#define DXX_SOUND_ADDON_MUSIC_MENU_ITEM(VERB)
#endif

#if SDL_MAJOR_VERSION == 1
#define DXX_SOUND_CD_MUSIC_MENU_ITEM(VERB)	\
	DXX_MENUITEM(VERB, RADIO, "CD music", opt_sm_mtype2, GameCfg.MusicType == MUSIC_TYPE_REDBOOK, optgrp_music_type)	\

#define DXX_MUSIC_OPTIONS_CD_LABEL "CD music"
#else
#define DXX_SOUND_CD_MUSIC_MENU_ITEM(VERB)
#define DXX_MUSIC_OPTIONS_CD_LABEL ""
#endif

#if DXX_USE_SDLMIXER
#define DXX_SOUND_JUKEBOX_MENU_ITEM(VERB)	\
	DXX_MENUITEM(VERB, RADIO, "Jukebox", opt_sm_mtype3, GameCfg.MusicType == MUSIC_TYPE_CUSTOM, optgrp_music_type)	\

#define DXX_MUSIC_OPTIONS_JUKEBOX_LABEL "Jukebox"
#define DXX_SOUND_SDLMIXER_MENU_ITEMS(VERB)	\
	DXX_MENUITEM(VERB, TEXT, "", opt_label_blank2)	\
	DXX_MENUITEM(VERB, TEXT, "Jukebox options:", opt_label_jukebox_options)	\
	DXX_MENU_ITEM_BROWSE(VERB, "Path for level music", opt_sm_mtype3_lmpath)	\
	DXX_MENUITEM(VERB, INPUT, CGameCfg.CMLevelMusicPath, opt_sm_mtype3_lmpath_input)	\
	DXX_MENUITEM(VERB, TEXT, "", opt_label_blank3)	\
	DXX_MENUITEM(VERB, TEXT, "Level music play order:", opt_label_lm_order)	\
	DXX_MENUITEM(VERB, RADIO, "continuous", opt_sm_mtype3_lmplayorder1, GameCfg.CMLevelMusicPlayOrder == MUSIC_CM_PLAYORDER_CONT, optgrp_music_order)	\
	DXX_MENUITEM(VERB, RADIO, "one track per level", opt_sm_mtype3_lmplayorder2, GameCfg.CMLevelMusicPlayOrder == MUSIC_CM_PLAYORDER_LEVEL, optgrp_music_order)	\
	DXX_MENUITEM(VERB, RADIO, "random", opt_sm_mtype3_lmplayorder3, GameCfg.CMLevelMusicPlayOrder == MUSIC_CM_PLAYORDER_RAND, optgrp_music_order)	\
	DXX_MENUITEM(VERB, TEXT, "", opt_label_blank4)	\
	DXX_MENUITEM(VERB, TEXT, "Non-level music:", opt_label_nonlevel_music)	\
	DXX_MENU_ITEM_BROWSE(VERB, "Main menu", opt_sm_cm_mtype3_file1_b)	\
	DXX_MENUITEM(VERB, INPUT, CGameCfg.CMMiscMusic[SONG_TITLE], opt_sm_cm_mtype3_file1)	\
	DXX_MENU_ITEM_BROWSE(VERB, "Briefing", opt_sm_cm_mtype3_file2_b)	\
	DXX_MENUITEM(VERB, INPUT, CGameCfg.CMMiscMusic[SONG_BRIEFING], opt_sm_cm_mtype3_file2)	\
	DXX_MENU_ITEM_BROWSE(VERB, "Credits", opt_sm_cm_mtype3_file3_b)	\
	DXX_MENUITEM(VERB, INPUT, CGameCfg.CMMiscMusic[SONG_CREDITS], opt_sm_cm_mtype3_file3)	\
	DXX_MENU_ITEM_BROWSE(VERB, "Escape sequence", opt_sm_cm_mtype3_file4_b)	\
	DXX_MENUITEM(VERB, INPUT, CGameCfg.CMMiscMusic[SONG_ENDLEVEL], opt_sm_cm_mtype3_file4)	\
	DXX_MENU_ITEM_BROWSE(VERB, "Game ending", opt_sm_cm_mtype3_file5_b)	\
	DXX_MENUITEM(VERB, INPUT, CGameCfg.CMMiscMusic[SONG_ENDGAME], opt_sm_cm_mtype3_file5)	\

#else
#define DXX_SOUND_JUKEBOX_MENU_ITEM(VERB)
#define DXX_MUSIC_OPTIONS_JUKEBOX_LABEL ""
#define DXX_SOUND_SDLMIXER_MENU_ITEMS(VERB)
#endif

#if SDL_MAJOR_VERSION == 1 && DXX_USE_SDLMIXER
#define DXX_MUSIC_OPTIONS_SEPARATOR_TEXT " / "
#else
#define DXX_MUSIC_OPTIONS_SEPARATOR_TEXT ""
#endif

#define DXX_SOUND_MENU(VERB)	\
	DXX_MENUITEM(VERB, SLIDER, TXT_FX_VOLUME, opt_sm_digivol, GameCfg.DigiVolume, 0, 8)	\
	DXX_MENUITEM(VERB, SLIDER, "Music volume", opt_sm_musicvol, GameCfg.MusicVolume, 0, 8)	\
	DXX_MENUITEM(VERB, CHECK, TXT_REVERSE_STEREO, opt_sm_revstereo, GameCfg.ReverseStereo)	\
	DXX_MENUITEM(VERB, TEXT, "", opt_label_blank0)	\
	DXX_MENUITEM(VERB, TEXT, "Music type:", opt_label_music_type)	\
	DXX_MENUITEM(VERB, RADIO, "No music", opt_sm_mtype0, GameCfg.MusicType == MUSIC_TYPE_NONE, optgrp_music_type)	\
	DXX_SOUND_ADDON_MUSIC_MENU_ITEM(VERB)	\
	DXX_SOUND_CD_MUSIC_MENU_ITEM(VERB)	\
	DXX_SOUND_JUKEBOX_MENU_ITEM(VERB)	\
	DXX_MENUITEM(VERB, TEXT, "", opt_label_blank1)	\
	DXX_MENUITEM(VERB, TEXT, DXX_MUSIC_OPTIONS_CD_LABEL DXX_MUSIC_OPTIONS_SEPARATOR_TEXT DXX_MUSIC_OPTIONS_JUKEBOX_LABEL " options:", opt_label_music_options)	\
	DXX_MENUITEM(VERB, CHECK, REDBOOK_PLAYORDER_TEXT, opt_sm_redbook_playorder, GameCfg.OrigTrackOrder)	\
	DXX_SOUND_SDLMIXER_MENU_ITEMS(VERB)	\

class sound_menu_items
{
public:
	enum
	{
		optgrp_music_type,
#if DXX_USE_SDLMIXER
		optgrp_music_order,
#endif
	};
	enum
	{
		DXX_SOUND_MENU(ENUM)
	};
	array<newmenu_item, DXX_SOUND_MENU(COUNT)> m;
	sound_menu_items()
	{
		DXX_SOUND_MENU(ADD);
	}
	void read()
	{
		DXX_SOUND_MENU(READ);
	}
	static int menuset(newmenu *, const d_event &event, sound_menu_items *pitems);
};

#undef DXX_SOUND_MENU

}

int sound_menu_items::menuset(newmenu *, const d_event &event, sound_menu_items *pitems)
{
	const auto &items = pitems->m;
	int replay = 0;
	int rval = 0;
	switch (event.type)
	{
		case EVENT_NEWMENU_CHANGED:
		{
			auto &citem = static_cast<const d_change_event &>(event).citem;
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
			/*
			 * When builtin music is enabled, the next line expands to
			 * `#if +1 + 0`; when it is disabled, the line expands to
			 * `#if + 0`.
			 */
#if DXX_SOUND_ADDON_MUSIC_MENU_ITEM(COUNT) + 0
			else if (citem == opt_sm_mtype1)
			{
				GameCfg.MusicType = MUSIC_TYPE_BUILTIN;
				replay = 1;
			}
#endif
#if SDL_MAJOR_VERSION == 1
			else if (citem == opt_sm_mtype2)
			{
				GameCfg.MusicType = MUSIC_TYPE_REDBOOK;
				replay = 1;
			}
#endif
#if DXX_USE_SDLMIXER
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
#if DXX_USE_SDLMIXER
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
		}
		case EVENT_NEWMENU_SELECTED:
		{
#if DXX_USE_SDLMIXER
			auto &citem = static_cast<const d_select_event &>(event).citem;
#ifdef _WIN32
#define WINDOWS_DRIVE_CHANGE_TEXT	".\nCTRL-D to change drive"
#else
#define WINDOWS_DRIVE_CHANGE_TEXT
#endif
			if (citem == opt_sm_mtype3_lmpath)
			{
				static const array<file_extension_t, 1> ext_list{{"m3u"}};		// select a directory or M3U playlist
				const auto cfgpath = CGameCfg.CMLevelMusicPath.data();
				select_file_recursive(
					"Select directory or\nM3U playlist to\n play level music from" WINDOWS_DRIVE_CHANGE_TEXT,
									  cfgpath, ext_list, 1,	// look in current music path for ext_list files and allow directory selection
									  get_absolute_path, cfgpath);	// just copy the absolute path
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
		}
		case EVENT_WINDOW_CLOSE:
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

void do_sound_menu()
{

#if DXX_USE_SDLMIXER
	const auto old_CMLevelMusicPath = CGameCfg.CMLevelMusicPath;
	const auto old_CMMiscMusic0 = CGameCfg.CMMiscMusic[SONG_TITLE];
#endif

	sound_menu_items items;
	newmenu_do1(nullptr, "Sound Effects & Music", items.m.size(), items.m.data(), &sound_menu_items::menuset, &items, 0);

#if DXX_USE_SDLMIXER
	if ((Game_wind != NULL && strcmp(old_CMLevelMusicPath.data(), CGameCfg.CMLevelMusicPath.data())) || (Game_wind == NULL && strcmp(old_CMMiscMusic0.data(), CGameCfg.CMMiscMusic[SONG_TITLE].data())))
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

#elif defined(DXX_BUILD_DESCENT_II)
#define DXX_GAME_SPECIFIC_OPTIONS(VERB)	\
	DXX_MENUITEM(VERB, CHECK, "Headlight on when picked up", opt_headlighton,PlayerCfg.HeadlightActiveDefault )	\
	DXX_MENUITEM(VERB, CHECK, "Escort robot hot keys",opt_escorthotkey,PlayerCfg.EscortHotKeys)	\
	DXX_MENUITEM(VERB, CHECK, "Movie Subtitles",opt_moviesubtitle,GameCfg.MovieSubtitles)	\
	DXX_MENUITEM(VERB, CHECK, "Remove Thief at level start", opt_thief_presence, thief_absent)	\
	DXX_MENUITEM(VERB, CHECK, "Prevent Thief Stealing Energy Weapons", opt_thief_steal_energy, thief_cannot_steal_energy_weapons)	\

#endif

enum {
	optgrp_autoselect_firing,
};

#define DXX_GAMEPLAY_MENU_OPTIONS(VERB)	\
	DXX_MENUITEM(VERB, CHECK, "Ship auto-leveling",opt_autolevel, PlayerCfg.AutoLeveling)	\
	DXX_MENUITEM(VERB, CHECK, "Persistent Debris",opt_persist_debris,PlayerCfg.PersistentDebris)	\
	DXX_MENUITEM(VERB, CHECK, "No Rankings (Multi)",opt_noranking,PlayerCfg.NoRankings)	\
	DXX_MENUITEM(VERB, CHECK, "Free Flight in Automap",opt_freeflight, PlayerCfg.AutomapFreeFlight)	\
	DXX_GAME_SPECIFIC_OPTIONS(VERB)	\
	DXX_MENUITEM(VERB, TEXT, "", opt_label_blank)	\
        DXX_MENUITEM(VERB, TEXT, "Weapon Autoselect options:", opt_label_autoselect)	\
	DXX_MENUITEM(VERB, MENU, "Primary ordering...", opt_gameplay_reorderprimary_menu)	\
	DXX_MENUITEM(VERB, MENU, "Secondary ordering...", opt_gameplay_reordersecondary_menu)	\
	DXX_MENUITEM(VERB, TEXT, "Autoselect while firing:", opt_autoselect_firing_label)	\
	DXX_MENUITEM(VERB, RADIO, "Immediately", opt_autoselect_firing_immediate, PlayerCfg.NoFireAutoselect == FiringAutoselectMode::Immediate, optgrp_autoselect_firing)	\
	DXX_MENUITEM(VERB, RADIO, "Never", opt_autoselect_firing_never, PlayerCfg.NoFireAutoselect == FiringAutoselectMode::Never, optgrp_autoselect_firing)	\
	DXX_MENUITEM(VERB, RADIO, "When firing stops", opt_autoselect_firing_delayed, PlayerCfg.NoFireAutoselect == FiringAutoselectMode::Delayed, optgrp_autoselect_firing)	\
	DXX_MENUITEM(VERB, CHECK, "Only Cycle Autoselect Weapons",opt_only_autoselect,PlayerCfg.CycleAutoselectOnly)	\

enum {
        DXX_GAMEPLAY_MENU_OPTIONS(ENUM)
};

static int gameplay_config_menuset(newmenu *, const d_event &event, const unused_newmenu_userdata_t *)
{
	switch (event.type)
	{
		case EVENT_NEWMENU_SELECTED:
		{
			auto &citem = static_cast<const d_select_event &>(event).citem;
                        if (citem == opt_gameplay_reorderprimary_menu)
                                ReorderPrimary();
                        if (citem == opt_gameplay_reordersecondary_menu)
                                ReorderSecondary();
			return 1;		// stay in menu
		}

		default:
			break;
	}

	return 0;
}

void gameplay_config()
{
	int i = 0;

	do {
		newmenu_item m[DXX_GAMEPLAY_MENU_OPTIONS(COUNT)];
#if defined(DXX_BUILD_DESCENT_II)
		auto thief_absent = PlayerCfg.ThiefModifierFlags & ThiefModifier::Absent;
		auto thief_cannot_steal_energy_weapons = PlayerCfg.ThiefModifierFlags & ThiefModifier::NoEnergyWeapons;
#endif
		DXX_GAMEPLAY_MENU_OPTIONS(ADD);
                i = newmenu_do1( NULL, "Gameplay Options", sizeof(m)/sizeof(*m), m, gameplay_config_menuset, unused_newmenu_userdata, 0 );
		DXX_GAMEPLAY_MENU_OPTIONS(READ);
		PlayerCfg.NoFireAutoselect = m[opt_autoselect_firing_delayed].value
			? FiringAutoselectMode::Delayed
			: (m[opt_autoselect_firing_immediate].value
				? FiringAutoselectMode::Immediate
				: FiringAutoselectMode::Never);
#if defined(DXX_BUILD_DESCENT_II)
		PlayerCfg.ThiefModifierFlags =
			(thief_absent ? ThiefModifier::Absent : 0) |
			(thief_cannot_steal_energy_weapons ? ThiefModifier::NoEnergyWeapons : 0);
#endif
	} while( i>-1 );

}

#if DXX_USE_UDP
static int multi_player_menu_handler(newmenu *menu,const d_event &event, int *menu_choice)
{
	newmenu_item *items = newmenu_get_items(menu);

	switch (event.type)
	{
		case EVENT_NEWMENU_SELECTED:
		{
			auto &citem = static_cast<const d_select_event &>(event).citem;
			// stay in multiplayer menu, even after having played a game
			return do_option(menu_choice[citem]);
		}

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

#if DXX_USE_UDP
	ADD_ITEM("HOST GAME", MENU_START_UDP_NETGAME, -1);
#if DXX_USE_TRACKER
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
	auto items = new options_menu_items;
	// Fall back to main event loop
	// Allows clean closing and re-opening when resolution changes
	newmenu_do3(nullptr, TXT_OPTIONS, items->m.size(), items->m.data(), options_menuset, items, 0, nullptr);
}

#ifndef RELEASE
namespace dsx {
static window_event_result polygon_models_viewer_handler(window *, const d_event &event, const unused_window_userdata_t *)
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
			{
				auto &canvas = *grd_curcanv;
				draw_model_picture(canvas, view_idx, ang);
				gr_set_fontcolor(canvas, BM_XRGB(255, 255, 255), -1);
				auto &game_font = *GAME_FONT;
				gr_printf(canvas, game_font, FSPACX(1), FSPACY(1), "ESC: leave\nSPACE/BACKSP: next/prev model (%i/%i)\nA/D: rotate y\nW/S: rotate x\nQ/E: rotate z\nR: reset orientation", view_idx, N_polygon_models - 1);
			}
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
}

static void polygon_models_viewer()
{
	const auto wind = window_create(grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT, polygon_models_viewer_handler, unused_window_userdata);
	if (!wind)
	{
		d_event event = { EVENT_WINDOW_CLOSE };
		polygon_models_viewer_handler(NULL, event, NULL);
		return;
	}

	event_process_all();
}

namespace dsx {
static window_event_result gamebitmaps_viewer_handler(window *, const d_event &event, const unused_window_userdata_t *)
{
	static int view_idx = 0;
	int key = 0;
#if DXX_USE_OGL
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
			{
				auto &canvas = *grd_curcanv;
				gr_clear_canvas(canvas, BM_XRGB(0,0,0));
#if DXX_USE_OGL
				scale = (bm->bm_w > bm->bm_h)?(SHEIGHT/bm->bm_w)*0.8:(SHEIGHT/bm->bm_h)*0.8;
				ogl_ubitmapm_cs(canvas, (SWIDTH / 2) - (bm->bm_w * scale / 2), (SHEIGHT / 2) - (bm->bm_h * scale / 2), bm->bm_w * scale, bm->bm_h * scale, *bm, ogl_colors::white, F1_0);
#else
				gr_bitmap(canvas, (SWIDTH / 2) - (bm->bm_w / 2), (SHEIGHT / 2) - (bm->bm_h / 2), *bm);
#endif
				gr_set_fontcolor(canvas, BM_XRGB(255, 255, 255), -1);
				auto &game_font = *GAME_FONT;
				gr_printf(canvas, game_font, FSPACX(1), FSPACY(1), "ESC: leave\nSPACE/BACKSP: next/prev bitmap (%i/%i)", view_idx, Num_bitmap_files-1);
			}
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
}

static void gamebitmaps_viewer()
{
	const auto wind = window_create(grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT, gamebitmaps_viewer_handler, unused_window_userdata);
	if (!wind)
	{
		d_event event = { EVENT_WINDOW_CLOSE };
		gamebitmaps_viewer_handler(NULL, event, NULL);
		return;
	}

	event_process_all();
}

#define DXX_SANDBOX_MENU(VERB)	\
	DXX_MENUITEM(VERB, MENU, "Polygon_models viewer", polygon_models)	\
	DXX_MENUITEM(VERB, MENU, "GameBitmaps viewer", bitmaps)	\

namespace {

class sandbox_menu_items
{
public:
	enum
	{
		DXX_SANDBOX_MENU(ENUM)
	};
	array<newmenu_item, DXX_SANDBOX_MENU(COUNT)> m;
	sandbox_menu_items()
	{
		DXX_SANDBOX_MENU(ADD);
	}
};

}

static int sandbox_menuset(newmenu *, const d_event &event, sandbox_menu_items *items)
{
	switch (event.type)
	{
		case EVENT_NEWMENU_CHANGED:
			break;

		case EVENT_NEWMENU_SELECTED:
		{
			auto &citem = static_cast<const d_select_event &>(event).citem;
			switch (citem)
			{
				case sandbox_menu_items::polygon_models:
					polygon_models_viewer();
					break;
				case sandbox_menu_items::bitmaps:
					gamebitmaps_viewer();
					break;
			}
			return 1; // stay in menu until escape
		}

		case EVENT_WINDOW_CLOSE:
		{
			std::default_delete<sandbox_menu_items>()(items);
			break;
		}

		default:
			break;
	}
	return 0;
}

void do_sandbox_menu()
{
	auto items = new sandbox_menu_items;
	newmenu_do3(nullptr, "Coder's sandbox", items->m.size(), items->m.data(), sandbox_menuset, items, 0, nullptr);
}
#endif
