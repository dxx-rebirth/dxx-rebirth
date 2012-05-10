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
#include "error.h"
#include "bm.h"
#include "screens.h"
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
#include "gauges.h"
#include "hudmsg.h" //for HUD_max_num_disp
#include "strutil.h"
#include "multi.h"
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

#define ADD_ITEM(t,value,key)  do { m[num_options].type=NM_TYPE_MENU; m[num_options].text=t; menu_choice[num_options]=value;num_options++; } while (0)

static window *menus[16] = { NULL };

// Function Prototypes added after LINTING
int do_option(int select);
int do_new_game_menu(void);
void do_multi_player_menu();
#ifndef RELEASE
void do_sandbox_menu();
#endif
extern void newmenu_free_background();
extern void ReorderPrimary();
extern void ReorderSecondary();

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
char playername_allowed_chars[] = "azAZ09__--";

int MakeNewPlayerFile(int allow_abort)
{
	int x;
	char filename[PATH_MAX];
	newmenu_item m;
	char text[CALLSIGN_LEN+9]="";

	strncpy(text, Players[Player_num].callsign,CALLSIGN_LEN);

try_again:
	m.type=NM_TYPE_INPUT; m.text_len = CALLSIGN_LEN; m.text = text;

	Newmenu_allowed_chars = playername_allowed_chars;
	x = newmenu_do( NULL, TXT_ENTER_PILOT_NAME, 1, &m, NULL, NULL );
	Newmenu_allowed_chars = NULL;

	if ( x < 0 ) {
		if ( allow_abort ) return 0;
		goto try_again;
	}

	if (text[0]==0)	//null string
		goto try_again;

	strlwr(text);

	memset(filename, '\0', PATH_MAX);
	snprintf( filename, PATH_MAX, GameArg.SysUsePlayersDir? "Players/%s.plr" : "%s.plr", text );

	if (PHYSFSX_exists(filename,0))
	{
		nm_messagebox(NULL, 1, TXT_OK, "%s '%s' %s", TXT_PLAYER, text, TXT_ALREADY_EXISTS );
		goto try_again;
	}

	if ( !new_player_config() )
		goto try_again;			// They hit Esc during New player config

	strncpy(Players[Player_num].callsign, text, CALLSIGN_LEN);
	strlwr(Players[Player_num].callsign);

	write_player_file();

	return 1;
}

void delete_player_saved_games(char * name);

int player_menu_keycommand( listbox *lb, d_event *event )
{
	char **items = listbox_get_items(lb);
	int citem = listbox_get_citem(lb);

	switch (event_key_get(event))
	{
		case KEY_CTRLED+KEY_D:
			if (citem > 0)
			{
				int x = 1;
				x = nm_messagebox( NULL, 2, TXT_YES, TXT_NO, "%s %s?", TXT_DELETE_PILOT, items[citem]+((items[citem][0]=='$')?1:0) );
				if (x==0)	{
					char * p;
					char plxfile[PATH_MAX], efffile[PATH_MAX], ngpfile[PATH_MAX];
					int ret;
					char name[PATH_MAX];

					p = items[citem] + strlen(items[citem]);
					*p = '.';

					strcpy(name, GameArg.SysUsePlayersDir ? "Players/" : "");
					strcat(name, items[citem]);

					ret = !PHYSFS_delete(name);
					*p = 0;

					if (!ret)
					{
						delete_player_saved_games( items[citem] );
						// delete PLX file
						sprintf(plxfile, GameArg.SysUsePlayersDir? "Players/%.8s.plx" : "%.8s.plx", items[citem]);
						if (PHYSFSX_exists(plxfile,0))
							PHYSFS_delete(plxfile);
						// delete EFF file
						sprintf(efffile, GameArg.SysUsePlayersDir? "Players/%.8s.eff" : "%.8s.eff", items[citem]);
						if (PHYSFSX_exists(efffile,0))
							PHYSFS_delete(efffile);
						// delete NGP file
						sprintf(ngpfile, GameArg.SysUsePlayersDir? "Players/%.8s.ngp" : "%.8s.ngp", items[citem]);
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

int player_menu_handler( listbox *lb, d_event *event, char **list )
{
	char **items = listbox_get_items(lb);
	int citem = listbox_get_citem(lb);

	switch (event->type)
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
				strncpy(Players[Player_num].callsign,items[citem] + ((items[citem][0]=='$')?1:0), CALLSIGN_LEN);
				strlwr(Players[Player_num].callsign);
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
	char **m;
	char **f;
	char **list;
	char *types[] = { ".plr", NULL };
	int i = 0, NumItems;
	int citem = 0;
	int allow_abort_flag = 1;

	if ( Players[Player_num].callsign[0] == 0 )
	{
		if (GameCfg.LastPlayer[0]==0)
		{
			strncpy( Players[Player_num].callsign, "player", CALLSIGN_LEN );
			allow_abort_flag = 0;
		}
		else
		{
			// Read the last player's name from config file, not lastplr.txt
			strncpy( Players[Player_num].callsign, GameCfg.LastPlayer, CALLSIGN_LEN );
		}
	}

	list = PHYSFSX_findFiles(GameArg.SysUsePlayersDir ? "Players/" : "", types);
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

	MALLOC(m, char *, NumItems);
	if (m == NULL)
	{
		PHYSFS_freeList(list);
		return 0;
	}

	m[i++] = TXT_CREATE_NEW;

	for (f = list; *f != NULL; f++)
	{
		char *p;

		if (strlen(*f) > FILENAME_LEN-1 || strlen(*f) < 5) // sorry guys, can only have up to eight chars for the player name
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
		if (!stricmp(Players[Player_num].callsign, m[i]) )
			citem = i;

	newmenu_listbox1(TXT_SELECT_PILOT, NumItems, m, allow_abort_flag, citem, (int (*)(listbox *, d_event *, void *))player_menu_handler, list);

	return 1;
}

// Draw Copyright and Version strings
void draw_copyright()
{
	gr_set_current_canvas(NULL);
	gr_set_curfont(GAME_FONT);
	gr_set_fontcolor(BM_XRGB(6,6,6),-1);
	gr_printf(0x8000,SHEIGHT-LINE_SPACING,TXT_COPYRIGHT);
	gr_set_fontcolor( BM_XRGB(25,0,0), -1);
	gr_printf(0x8000,SHEIGHT-(LINE_SPACING*2),DESCENT_VERSION);
}

int main_menu_handler(newmenu *menu, d_event *event, int *menu_choice )
{
	newmenu_item *items = newmenu_get_items(menu);

	switch (event->type)
	{
		case EVENT_WINDOW_ACTIVATED:
			if ( Players[Player_num].callsign[0]==0 )
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
			if ( keyd_time_when_last_pressed+i2f(45) < timer_query() || GameArg.SysAutoDemo  )
			{
				keyd_time_when_last_pressed = timer_query();			// Reset timer so that disk won't thrash if no demos.
				newdemo_start_playback(NULL);		// Randomly pick a file
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
void create_main_menu(newmenu_item *m, int *menu_choice, int *callers_num_options)
{
	int	num_options;

	#ifndef DEMO_ONLY
	num_options = 0;

	ADD_ITEM(TXT_NEW_GAME,MENU_NEW_GAME,KEY_N);

	ADD_ITEM(TXT_LOAD_GAME,MENU_LOAD_GAME,KEY_L);
#if defined(USE_UDP)
	ADD_ITEM(TXT_MULTIPLAYER_,MENU_MULTIPLAYER,-1);
#endif

	ADD_ITEM(TXT_OPTIONS_, MENU_CONFIG, -1 );
	ADD_ITEM(TXT_CHANGE_PILOTS,MENU_NEW_PLAYER,unused);
	ADD_ITEM(TXT_VIEW_DEMO,MENU_DEMO_PLAY,0);
	ADD_ITEM(TXT_VIEW_SCORES,MENU_VIEW_SCORES,KEY_V);
	if (!PHYSFSX_exists("warning.pcx",1)) /* SHAREWARE */
		ADD_ITEM(TXT_ORDERING_INFO,MENU_ORDER_INFO,-1);
	ADD_ITEM(TXT_CREDITS,MENU_SHOW_CREDITS,-1);
	#endif
	ADD_ITEM(TXT_QUIT,MENU_QUIT,KEY_Q);

	#ifndef RELEASE
	if (!(Game_mode & GM_MULTI ))	{
		//m[num_options].type=NM_TYPE_TEXT;
		//m[num_options++].text=" Debug options:";

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

	MALLOC(menu_choice, int, 25);
	if (!menu_choice)
		return -1;
	MALLOC(m, newmenu_item, 25);
	if (!m)
	{
		d_free(menu_choice);
		return -1;
	}

	memset(menu_choice, 0, sizeof(int)*25);
	memset(m, 0, sizeof(newmenu_item)*25);

	create_main_menu(m, menu_choice, &num_options); // may have to change, eg, maybe selected pilot and no save games.

	newmenu_do3( "", NULL, num_options, m, (int (*)(newmenu *, d_event *, void *))main_menu_handler, menu_choice, 0, Menu_pcx_name);

	return 0;
}

extern void show_order_form(void);	// John didn't want this in inferno.h so I just externed it.

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
			state_restore_all(0);
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

void delete_player_saved_games(char * name)
{
	int i;
	char filename[PATH_MAX];

	for (i=0;i<10; i++)
	{
		snprintf( filename, PATH_MAX, GameArg.SysUsePlayersDir? "Players/%s.sg%x" : "%s.sg%x", name, i );
		PHYSFS_delete(filename);
		snprintf( filename, PATH_MAX, GameArg.SysUsePlayersDir? "Players/%s.mg%x" : "%s.mg%x", name, i );
		PHYSFS_delete(filename);
	}
}

int demo_menu_keycommand( listbox *lb, d_event *event )
{
	char **items = listbox_get_items(lb);
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

int demo_menu_handler( listbox *lb, d_event *event, void *userdata )
{
	char **items = listbox_get_items(lb);
	int citem = listbox_get_citem(lb);

	userdata = userdata;

	switch (event->type)
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
	char *types[] = { DEMO_EXT, NULL };
	int NumItems;

	list = PHYSFSX_findFiles(DEMO_DIR, types);
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

	newmenu_listbox1(TXT_SELECT_DEMO, NumItems, list, 1, 0, demo_menu_handler, NULL);

	return 1;
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

	s = newmenu_do1( NULL, TXT_DIFFICULTY_LEVEL, NDL, m, NULL, NULL, Difficulty_level);

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

		while (!valid)
		{
			sprintf(info_text,"%s %d",TXT_START_ANY_LEVEL, player_highest_level);

			m[0].type=NM_TYPE_TEXT; m[0].text = info_text;
			m[1].type=NM_TYPE_INPUT; m[1].text_len = 10; m[1].text = num_text;
			n_items = 2;

			strcpy(num_text,"1");

			choice = newmenu_do( NULL, TXT_SELECT_START_LEV, n_items, m, NULL, NULL );

			if (choice==-1 || m[1].text[0]==0)
				return 0;

			new_level_num = atoi(m[1].text);

			if (!(new_level_num>0 && new_level_num<=player_highest_level)) {
				m[0].text = TXT_ENTER_TO_CONT;
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

void do_sound_menu();
void input_config();
void change_res();
void graphics_config();
void do_misc_menu();

int options_menuset(newmenu *menu, d_event *event, void *userdata)
{
	switch (event->type)
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

	userdata = userdata;		//kill warning

	return 0;
}

int gcd(int a, int b)
{
	if (!b)
		return a;

	return gcd(b, a%b);
}

void change_res()
{
	u_int32_t modes[50], new_mode = 0;
	int i = 0, mc = 0, num_presets = 0, citem = -1, opt_cval = -1, opt_fullscr = -1;

	num_presets = gr_list_modes( modes );

	{
	newmenu_item m[50+8];
	char restext[50][12], crestext[12], casptext[12];

	for (i = 0; i <= num_presets-1; i++)
	{
		snprintf(restext[mc], sizeof(restext[mc]), "%ix%i", SM_W(modes[i]), SM_H(modes[i]));
		m[mc].type = NM_TYPE_RADIO;
		m[mc].text = restext[mc];
		m[mc].value = ((citem == -1) && (Game_screen_mode == modes[i]) && GameCfg.AspectY == SM_W(modes[i])/gcd(SM_W(modes[i]),SM_H(modes[i])) && GameCfg.AspectX == SM_H(modes[i])/gcd(SM_W(modes[i]),SM_H(modes[i])));
		m[mc].group = 0;
		if (m[mc].value)
			citem = mc;
		mc++;
	}

	m[mc].type = NM_TYPE_TEXT; m[mc].text = ""; mc++; // little space for overview
	// the fields for custom resolution and aspect
	opt_cval = mc;
	m[mc].type = NM_TYPE_RADIO; m[mc].text = "use custom values"; m[mc].value = (citem == -1); m[mc].group = 0; mc++;
	m[mc].type = NM_TYPE_TEXT; m[mc].text = "resolution:"; mc++;
	snprintf(crestext, sizeof(crestext), "%ix%i", SM_W(Game_screen_mode), SM_H(Game_screen_mode));
	m[mc].type = NM_TYPE_INPUT; m[mc].text = crestext; m[mc].text_len = 11; modes[mc] = 0; mc++;
	m[mc].type = NM_TYPE_TEXT; m[mc].text = "aspect:"; mc++;
	snprintf(casptext, sizeof(casptext), "%ix%i", GameCfg.AspectY, GameCfg.AspectX);
	m[mc].type = NM_TYPE_INPUT; m[mc].text = casptext; m[mc].text_len = 11; modes[mc] = 0; mc++;
	m[mc].type = NM_TYPE_TEXT; m[mc].text = ""; mc++; // little space for overview
	// fullscreen
	opt_fullscr = mc;
	m[mc].type = NM_TYPE_CHECK; m[mc].text = "Fullscreen"; m[mc].value = gr_check_fullscreen(); mc++;

	// create the menu
	newmenu_do1(NULL, "Screen Resolution", mc, m, NULL, NULL, 0);

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
	game_init_render_buffers(SM_W(Game_screen_mode), SM_H(Game_screen_mode), VR_NONE);
	}
}

void input_config_sensitivity()
{
	newmenu_item m[26];
	int i = 0, nitems = 0, joysens = 0, joydead = 0, mousesens = 0, mousefsdead;

	m[nitems].type = NM_TYPE_TEXT; m[nitems].text = "Joystick Sensitivity:"; nitems++;
	joysens = nitems;
	m[nitems].type = NM_TYPE_SLIDER; m[nitems].text = TXT_TURN_LR; m[nitems].value = PlayerCfg.JoystickSens[0]; m[nitems].min_value = 0; m[nitems].max_value = 16; nitems++;
	m[nitems].type = NM_TYPE_SLIDER; m[nitems].text = TXT_PITCH_UD; m[nitems].value = PlayerCfg.JoystickSens[1]; m[nitems].min_value = 0; m[nitems].max_value = 16; nitems++;
	m[nitems].type = NM_TYPE_SLIDER; m[nitems].text = TXT_SLIDE_LR; m[nitems].value = PlayerCfg.JoystickSens[2]; m[nitems].min_value = 0; m[nitems].max_value = 16; nitems++;
	m[nitems].type = NM_TYPE_SLIDER; m[nitems].text = TXT_SLIDE_UD; m[nitems].value = PlayerCfg.JoystickSens[3]; m[nitems].min_value = 0; m[nitems].max_value = 16; nitems++;
	m[nitems].type = NM_TYPE_SLIDER; m[nitems].text = TXT_BANK_LR; m[nitems].value = PlayerCfg.JoystickSens[4]; m[nitems].min_value = 0; m[nitems].max_value = 16; nitems++;
	m[nitems].type = NM_TYPE_SLIDER; m[nitems].text = TXT_THROTTLE; m[nitems].value = PlayerCfg.JoystickSens[5]; m[nitems].min_value = 0; m[nitems].max_value = 16; nitems++;
	m[nitems].type = NM_TYPE_TEXT; m[nitems].text = ""; nitems++;
	m[nitems].type = NM_TYPE_TEXT; m[nitems].text = "Joystick Deadzone:"; nitems++;
	joydead = nitems;
	m[nitems].type = NM_TYPE_SLIDER; m[nitems].text = TXT_TURN_LR; m[nitems].value = PlayerCfg.JoystickDead[0]; m[nitems].min_value = 0; m[nitems].max_value = 16; nitems++;
	m[nitems].type = NM_TYPE_SLIDER; m[nitems].text = TXT_PITCH_UD; m[nitems].value = PlayerCfg.JoystickDead[1]; m[nitems].min_value = 0; m[nitems].max_value = 16; nitems++;
	m[nitems].type = NM_TYPE_SLIDER; m[nitems].text = TXT_SLIDE_LR; m[nitems].value = PlayerCfg.JoystickDead[2]; m[nitems].min_value = 0; m[nitems].max_value = 16; nitems++;
	m[nitems].type = NM_TYPE_SLIDER; m[nitems].text = TXT_SLIDE_UD; m[nitems].value = PlayerCfg.JoystickDead[3]; m[nitems].min_value = 0; m[nitems].max_value = 16; nitems++;
	m[nitems].type = NM_TYPE_SLIDER; m[nitems].text = TXT_BANK_LR; m[nitems].value = PlayerCfg.JoystickDead[4]; m[nitems].min_value = 0; m[nitems].max_value = 16; nitems++;
	m[nitems].type = NM_TYPE_SLIDER; m[nitems].text = TXT_THROTTLE; m[nitems].value = PlayerCfg.JoystickDead[5]; m[nitems].min_value = 0; m[nitems].max_value = 16; nitems++;
	m[nitems].type = NM_TYPE_TEXT; m[nitems].text = ""; nitems++;
	m[nitems].type = NM_TYPE_TEXT; m[nitems].text = "Mouse Sensitivity:"; nitems++;
	mousesens = nitems;
	m[nitems].type = NM_TYPE_SLIDER; m[nitems].text = TXT_TURN_LR; m[nitems].value = PlayerCfg.MouseSens[0]; m[nitems].min_value = 0; m[nitems].max_value = 16; nitems++;
	m[nitems].type = NM_TYPE_SLIDER; m[nitems].text = TXT_PITCH_UD; m[nitems].value = PlayerCfg.MouseSens[1]; m[nitems].min_value = 0; m[nitems].max_value = 16; nitems++;
	m[nitems].type = NM_TYPE_SLIDER; m[nitems].text = TXT_SLIDE_LR; m[nitems].value = PlayerCfg.MouseSens[2]; m[nitems].min_value = 0; m[nitems].max_value = 16; nitems++;
	m[nitems].type = NM_TYPE_SLIDER; m[nitems].text = TXT_SLIDE_UD; m[nitems].value = PlayerCfg.MouseSens[3]; m[nitems].min_value = 0; m[nitems].max_value = 16; nitems++;
	m[nitems].type = NM_TYPE_SLIDER; m[nitems].text = TXT_BANK_LR; m[nitems].value = PlayerCfg.MouseSens[4]; m[nitems].min_value = 0; m[nitems].max_value = 16; nitems++;
	m[nitems].type = NM_TYPE_SLIDER; m[nitems].text = TXT_THROTTLE; m[nitems].value = PlayerCfg.MouseSens[5]; m[nitems].min_value = 0; m[nitems].max_value = 16; nitems++;
	m[nitems].type = NM_TYPE_TEXT; m[nitems].text = ""; nitems++;
	m[nitems].type = NM_TYPE_TEXT; m[nitems].text = "Mouse FlightSim Deadzone:"; nitems++;
	mousefsdead = nitems;
	m[nitems].type = NM_TYPE_SLIDER; m[nitems].text = "X/Y"; m[nitems].value = PlayerCfg.MouseFSDead; m[nitems].min_value = 0; m[nitems].max_value = 16; nitems++;

	newmenu_do1(NULL, "SENSITIVITY & DEADZONE", nitems, m, NULL, NULL, 1);

	for (i = 0; i <= 5; i++)
	{
		PlayerCfg.JoystickSens[i] = m[joysens+i].value;
		PlayerCfg.JoystickDead[i] = m[joydead+i].value;
		PlayerCfg.MouseSens[i] = m[mousesens+i].value;
	}
	PlayerCfg.MouseFSDead = m[mousefsdead].value;
}

static int opt_ic_usejoy = 0, opt_ic_usemouse = 0, opt_ic_confkey = 0, opt_ic_confjoy = 0, opt_ic_confmouse = 0, opt_ic_confweap = 0, opt_ic_mouseflightsim = 0, opt_ic_joymousesens = 0, opt_ic_grabinput = 0, opt_ic_mousefsgauge = 0, opt_ic_help0 = 0, opt_ic_help1 = 0, opt_ic_help2 = 0;
int input_config_menuset(newmenu *menu, d_event *event, void *userdata)
{
	newmenu_item *items = newmenu_get_items(menu);
	int citem = newmenu_get_citem(menu);

	userdata = userdata;

	switch (event->type)
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
	m[nitems].type = NM_TYPE_CHECK; m[nitems].text = "USE JOYSTICK"; m[nitems].value = (PlayerCfg.ControlType&CONTROL_USING_JOYSTICK); nitems++;
	opt_ic_usemouse = nitems;
	m[nitems].type = NM_TYPE_CHECK; m[nitems].text = "USE MOUSE"; m[nitems].value = (PlayerCfg.ControlType&CONTROL_USING_MOUSE); nitems++;
	m[nitems].type = NM_TYPE_TEXT; m[nitems].text = ""; nitems++;
	opt_ic_confkey = nitems;
	m[nitems].type = NM_TYPE_MENU; m[nitems].text = "CUSTOMIZE KEYBOARD"; nitems++;
	opt_ic_confjoy = nitems;
	m[nitems].type = NM_TYPE_MENU; m[nitems].text = "CUSTOMIZE JOYSTICK"; nitems++;
	opt_ic_confmouse = nitems;
	m[nitems].type = NM_TYPE_MENU; m[nitems].text = "CUSTOMIZE MOUSE"; nitems++;
	opt_ic_confweap = nitems;
	m[nitems].type = NM_TYPE_MENU; m[nitems].text = "CUSTOMIZE WEAPON KEYS"; nitems++;
	m[nitems].type = NM_TYPE_TEXT; m[nitems].text = ""; nitems++;
	m[nitems].type = NM_TYPE_TEXT; m[nitems].text = "MOUSE CONTROL TYPE:"; nitems++;
	opt_ic_mouseflightsim = nitems;
	m[nitems].type = NM_TYPE_RADIO; m[nitems].text = "normal"; m[nitems].value = !PlayerCfg.MouseFlightSim; m[nitems].group = 0; nitems++;
	m[nitems].type = NM_TYPE_RADIO; m[nitems].text = "FlightSim"; m[nitems].value = PlayerCfg.MouseFlightSim; m[nitems].group = 0; nitems++;
	m[nitems].type = NM_TYPE_TEXT; m[nitems].text = ""; nitems++;
	opt_ic_joymousesens = nitems;
	m[nitems].type = NM_TYPE_MENU; m[nitems].text = "SENSITIVITY & DEADZONE"; nitems++;
	m[nitems].type = NM_TYPE_TEXT; m[nitems].text = ""; nitems++;
	opt_ic_grabinput = nitems;
	m[nitems].type = NM_TYPE_CHECK; m[nitems].text= "Keep Keyboard/Mouse focus"; m[nitems].value = GameCfg.Grabinput; nitems++;
	opt_ic_mousefsgauge = nitems;
	m[nitems].type = NM_TYPE_CHECK; m[nitems].text= "Mouse FlightSim Indicator"; m[nitems].value = PlayerCfg.MouseFSIndicator; nitems++;
	m[nitems].type = NM_TYPE_TEXT; m[nitems].text = ""; nitems++;
	opt_ic_help0 = nitems;
	m[nitems].type = NM_TYPE_MENU; m[nitems].text = "GAME SYSTEM KEYS"; nitems++;
	opt_ic_help1 = nitems;
	m[nitems].type = NM_TYPE_MENU; m[nitems].text = "NETGAME SYSTEM KEYS"; nitems++;
	opt_ic_help2 = nitems;
	m[nitems].type = NM_TYPE_MENU; m[nitems].text = "DEMO SYSTEM KEYS"; nitems++;

	newmenu_do1(NULL, TXT_CONTROLS, nitems, m, input_config_menuset, NULL, 3);
}

void reticle_config()
{
#ifdef OGL
	newmenu_item m[18];
#else
	newmenu_item m[17];
#endif
	int nitems = 0, i, opt_ret_type, opt_ret_rgba, opt_ret_size;
	
	m[nitems].type = NM_TYPE_TEXT; m[nitems].text = "Reticle Type:"; nitems++;
	opt_ret_type = nitems;
	m[nitems].type = NM_TYPE_RADIO; m[nitems].text = "Classic"; m[nitems].value = 0; m[nitems].group = 0; nitems++;
#ifdef OGL
	m[nitems].type = NM_TYPE_RADIO; m[nitems].text = "Classic Reboot"; m[nitems].value = 0; m[nitems].group = 0; nitems++;
#endif
	m[nitems].type = NM_TYPE_RADIO; m[nitems].text = "None"; m[nitems].value = 0; m[nitems].group = 0; nitems++;
	m[nitems].type = NM_TYPE_RADIO; m[nitems].text = "X"; m[nitems].value = 0; m[nitems].group = 0; nitems++;
	m[nitems].type = NM_TYPE_RADIO; m[nitems].text = "Dot"; m[nitems].value = 0; m[nitems].group = 0; nitems++;
	m[nitems].type = NM_TYPE_RADIO; m[nitems].text = "Circle"; m[nitems].value = 0; m[nitems].group = 0; nitems++;
	m[nitems].type = NM_TYPE_RADIO; m[nitems].text = "Cross V1"; m[nitems].value = 0; m[nitems].group = 0; nitems++;
	m[nitems].type = NM_TYPE_RADIO; m[nitems].text = "Cross V2"; m[nitems].value = 0; m[nitems].group = 0; nitems++;
	m[nitems].type = NM_TYPE_RADIO; m[nitems].text = "Angle"; m[nitems].value = 0; m[nitems].group = 0; nitems++;
	m[nitems].type = NM_TYPE_TEXT; m[nitems].text = ""; nitems++;
	m[nitems].type = NM_TYPE_TEXT; m[nitems].text = "Reticle Color:"; nitems++;
	opt_ret_rgba = nitems;
	m[nitems].type = NM_TYPE_SLIDER; m[nitems].text = "Red"; m[nitems].value = (PlayerCfg.ReticleRGBA[0]/2); m[nitems].min_value = 0; m[nitems].max_value = 16; nitems++;
	m[nitems].type = NM_TYPE_SLIDER; m[nitems].text = "Green"; m[nitems].value = (PlayerCfg.ReticleRGBA[1]/2); m[nitems].min_value = 0; m[nitems].max_value = 16; nitems++;
	m[nitems].type = NM_TYPE_SLIDER; m[nitems].text = "Blue"; m[nitems].value = (PlayerCfg.ReticleRGBA[2]/2); m[nitems].min_value = 0; m[nitems].max_value = 16; nitems++;
	m[nitems].type = NM_TYPE_SLIDER; m[nitems].text = "Alpha"; m[nitems].value = (PlayerCfg.ReticleRGBA[3]/2); m[nitems].min_value = 0; m[nitems].max_value = 16; nitems++;
	m[nitems].type = NM_TYPE_TEXT; m[nitems].text = ""; nitems++;
	opt_ret_size = nitems;
	m[nitems].type = NM_TYPE_SLIDER; m[nitems].text = "Reticle Size:"; m[nitems].value = PlayerCfg.ReticleSize; m[nitems].min_value = 0; m[nitems].max_value = 4; nitems++;

	i = PlayerCfg.ReticleType;
#ifndef OGL
	if (i > 1) i--;
#endif
	m[opt_ret_type+i].value=1;

	newmenu_do1( NULL, "Reticle Options", nitems, m, NULL, NULL, 1 );

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
int graphics_config_menuset(newmenu *menu, d_event *event, void *userdata)
{
	newmenu_item *items = newmenu_get_items(menu);
	int citem = newmenu_get_citem(menu);

	userdata = userdata;

	switch (event->type)
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
	newmenu_item m[13];
	int i = 0;
#else
	newmenu_item m[3];
#endif
	int nitems = 0;

#ifdef OGL
	m[nitems].type = NM_TYPE_TEXT; m[nitems].text = "Texture Filtering:"; nitems++;
	opt_gr_texfilt = nitems;
	m[nitems].type = NM_TYPE_RADIO; m[nitems].text = "None (Classical)"; m[nitems].value = 0; m[nitems].group = 0; nitems++;
	m[nitems].type = NM_TYPE_RADIO; m[nitems].text = "Bilinear"; m[nitems].value = 0; m[nitems].group = 0; nitems++;
	m[nitems].type = NM_TYPE_RADIO; m[nitems].text = "Trilinear"; m[nitems].value = 0; m[nitems].group = 0; nitems++;
	m[nitems].type = NM_TYPE_RADIO; m[nitems].text = "Anisotropic"; m[nitems].value = 0; m[nitems].group = 0; nitems++;
	m[nitems].type = NM_TYPE_TEXT; m[nitems].text = ""; nitems++;
#endif
	opt_gr_brightness = nitems;
	m[nitems].type = NM_TYPE_SLIDER; m[nitems].text = TXT_BRIGHTNESS; m[nitems].value = gr_palette_get_gamma(); m[nitems].min_value = 0; m[nitems].max_value = 16; nitems++;
	opt_gr_reticlemenu = nitems;
	m[nitems].type = NM_TYPE_MENU; m[nitems].text = "Reticle Options"; nitems++;
#ifdef OGL
	opt_gr_alphafx = nitems;
	m[nitems].type = NM_TYPE_CHECK; m[nitems].text = "Transparency Effects"; m[nitems].value = PlayerCfg.AlphaEffects; nitems++;
	opt_gr_dynlightcolor = nitems;
	m[nitems].type = NM_TYPE_CHECK; m[nitems].text = "Colored Dynamic Light"; m[nitems].value = PlayerCfg.DynLightColor; nitems++;
	opt_gr_vsync = nitems;
	m[nitems].type = NM_TYPE_CHECK; m[nitems].text="VSync"; m[nitems].value = GameCfg.VSync; nitems++;
	opt_gr_multisample = nitems;
	m[nitems].type = NM_TYPE_CHECK; m[nitems].text="4x multisampling"; m[nitems].value = GameCfg.Multisample; nitems++;
#endif
	opt_gr_fpsindi = nitems;
	m[nitems].type = NM_TYPE_CHECK; m[nitems].text="FPS Counter"; m[nitems].value = GameCfg.FPSIndicator; nitems++;
#ifdef OGL
	m[opt_gr_texfilt+GameCfg.TexFilt].value=1;
#endif

	newmenu_do1( NULL, "Graphics Options", nitems, m, graphics_config_menuset, NULL, 1 );

#ifdef OGL
	if (GameCfg.VSync != m[opt_gr_vsync].value || GameCfg.Multisample != m[opt_gr_multisample].value)
		nm_messagebox( NULL, 1, TXT_OK, "To apply VSync or 4x Multisample\nyou need to restart the program");

	for (i = 0; i <= 3; i++)
		if (m[i+opt_gr_texfilt].value)
			GameCfg.TexFilt = i;
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
typedef struct browser
{
	char	*title;			// The title - needed for making another listbox when changing directory
	int		(*when_selected)(void *userdata, const char *filename);	// What to do when something chosen
	void	*userdata;		// Whatever you want passed to when_selected
	char	**list;			// All menu items
	char	*list_buf;		// Buffer for menu item text: hopefully reduces memory fragmentation this way
	char	**ext_list;		// List of file extensions we're looking for (if looking for a music file many types are possible)
	int		select_dir;		// Allow selecting the current directory (e.g. for Jukebox level song directory)
	int		num_files;		// Number of list items found (including parent directory and current directory if selectable)
	int		max_files;		// How many entries we can have before having to grow the array
	int		max_buf;		// How much text we can have before having to grow the buffer
	char	view_path[PATH_MAX];	// The absolute path we're currently looking at
	int		new_path;		// Whether the view_path is a new searchpath, if so, remove it when finished
} browser;

void list_dir_el(browser *b, const char *origdir, const char *fname)
{
	char *ext;
	char **i = NULL;
	
	ext = strrchr(fname, '.');
	if (ext)
		for (i = b->ext_list; *i != NULL && stricmp(ext, *i); i++) {}	// see if the file is of a type we want
	
	if ((!strcmp((PHYSFS_getRealDir(fname)==NULL?"":PHYSFS_getRealDir(fname)), b->view_path)) && (PHYSFS_isDirectory(fname) || (ext && *i))
#if defined(__MACH__) && defined(__APPLE__)
		&& stricmp(fname, "Volumes")	// this messes things up, use '..' instead
#endif
		)
		string_array_add(&b->list, &b->list_buf, &b->num_files, &b->max_files, &b->max_buf, fname);
}

int list_directory(browser *b)
{
	if (!string_array_new(&b->list, &b->list_buf, &b->num_files, &b->max_files, &b->max_buf))
		return 0;
	
	strcpy(b->list_buf, "..");		// go to parent directory
	b->list[b->num_files++] = b->list_buf;
	
	if (b->select_dir)
	{
		b->list[b->num_files] = b->list[b->num_files - 1] + strlen(b->list[b->num_files - 1]) + 1;
		strcpy(b->list[b->num_files++], "<this directory>");	// choose the directory being viewed
	}
	
	PHYSFS_enumerateFilesCallback("", (PHYSFS_EnumFilesCallback) list_dir_el, b);
	string_array_tidy(&b->list, &b->list_buf, &b->num_files, &b->max_files, &b->max_buf, 1 + (b->select_dir ? 1 : 0),
#ifdef __LINUX__
					  strcmp
#elif defined(_WIN32) || defined(macintosh)
					  stricmp
#else
					  strcasecmp
#endif
					  );
					  
	return 1;
}

int select_file_recursive(char *title, const char *orig_path, char **ext_list, int select_dir, int (*when_selected)(void *userdata, const char *filename), void *userdata);

int select_file_handler(listbox *menu, d_event *event, browser *b)
{
	char newpath[PATH_MAX];
	char **list = listbox_get_items(menu);
	int citem = listbox_get_citem(menu);
	const char *sep = PHYSFS_getDirSeparator();

	memset(newpath, 0, sizeof(char)*PATH_MAX);
	switch (event->type)
	{
#ifdef _WIN32
		case EVENT_KEY_COMMAND:
		{
			if (event_key_get(event) == KEY_CTRLED + KEY_D)
			{
				newmenu_item *m;
				char *text = NULL;
				int rval = 0;

				MALLOC(text, char, 2);
				MALLOC(m, newmenu_item, 1);
				snprintf(text, sizeof(char)*PATH_MAX, "c");
				m->type=NM_TYPE_INPUT; m->text_len = 3; m->text = text;
				rval = newmenu_do( NULL, "Enter drive letter", 1, m, NULL, NULL );
				text[1] = '\0'; 
				snprintf(newpath, sizeof(char)*PATH_MAX, "%s:%s", text, sep);
				if (!rval && strlen(text))
				{
					select_file_recursive(b->title, newpath, b->ext_list, b->select_dir, b->when_selected, b->userdata);
					// close old box.
					event->type = EVENT_WINDOW_CLOSED;
					window_close(listbox_get_window(menu));
				}
				d_free(text);
				d_free(m);
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
				
				if ((p = strstr(&newpath[strlen(newpath) - strlen(sep)], sep)))
					if (p != strstr(newpath, sep))	// if this isn't the only separator (i.e. it's not about to look at the root)
						*p = 0;
				
				p = newpath + strlen(newpath) - 1;
				while ((p > newpath) && strncmp(p, sep, strlen(sep)))	// make sure full separator string is matched (typically is)
					p--;
				
				if (p == strstr(newpath, sep))	// Look at root directory next, if not already
				{
#if defined(__MACH__) && defined(__APPLE__)
					if (!stricmp(p, "/Volumes"))
						return 1;
#endif
					if (p[strlen(sep)] != '\0')
						p[strlen(sep)] = '\0';
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
				if (strncmp(&newpath[strlen(newpath) - strlen(sep)], sep, strlen(sep)))
				{
					strncat(newpath, sep, PATH_MAX - 1 - strlen(newpath));
					newpath[PATH_MAX - 1] = '\0';
				}
				strncat(newpath, list[citem], PATH_MAX - 1 - strlen(newpath));
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

			if (list)
				d_free(list);
			if (b->list_buf)
				d_free(b->list_buf);
			d_free(b);
			break;
			
		default:
			break;
	}
	
	return 0;
}

int select_file_recursive(char *title, const char *orig_path, char **ext_list, int select_dir, int (*when_selected)(void *userdata, const char *filename), void *userdata)
{
	browser *b;
	const char *sep = PHYSFS_getDirSeparator();
	char *p;
	char new_path[PATH_MAX];
	
	MALLOC(b, browser, 1);
	if (!b)
		return 0;
	
	b->title = title;
	b->when_selected = when_selected;
	b->userdata = userdata;
	b->ext_list = ext_list;
	b->select_dir = select_dir;
	b->num_files = b->max_files = 0;
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
			while ((p > b->view_path) && strncmp(p, sep, strlen(sep)))
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
			d_free(b);
			return 0;
		}
	}
	
	if (!list_directory(b))
	{
		d_free(b);
		return 0;
	}
	
	return newmenu_listbox1(title, b->num_files, b->list, 1, 0, (int (*)(listbox *, d_event *, void *))select_file_handler, b) >= 0;
}

#define PATH_HEADER_TYPE NM_TYPE_MENU
#define BROWSE_TXT " (browse...)"

#else

int select_file_recursive(char *title, const char *orig_path, char **ext_list, int select_dir, int (*when_selected)(void *userdata, const char *filename), void *userdata)
{
	return 0;
}

#define PATH_HEADER_TYPE NM_TYPE_TEXT
#define BROWSE_TXT

#endif

int opt_sm_digivol = -1, opt_sm_musicvol = -1, opt_sm_revstereo = -1, opt_sm_mtype0 = -1, opt_sm_mtype1 = -1, opt_sm_mtype2 = -1, opt_sm_mtype3 = -1, opt_sm_redbook_playorder = -1, opt_sm_mtype3_lmpath = -1, opt_sm_mtype3_lmplayorder1 = -1, opt_sm_mtype3_lmplayorder2 = -1, opt_sm_mtype3_lmplayorder3 = -1, opt_sm_cm_mtype3_file1_b = -1, opt_sm_cm_mtype3_file1 = -1, opt_sm_cm_mtype3_file2_b = -1, opt_sm_cm_mtype3_file2 = -1, opt_sm_cm_mtype3_file3_b = -1, opt_sm_cm_mtype3_file3 = -1, opt_sm_cm_mtype3_file4_b = -1, opt_sm_cm_mtype3_file4 = -1, opt_sm_cm_mtype3_file5_b = -1, opt_sm_cm_mtype3_file5 = -1;

void set_extmusic_volume(int volume);

int get_absolute_path(char *full_path, const char *rel_path)
{
	PHYSFSX_getRealPath(rel_path, full_path);
	return 1;
}

#ifdef USE_SDLMIXER
#define SELECT_SONG(t, s)	select_file_recursive(t, GameCfg.CMMiscMusic[s], jukebox_exts, 0, (int (*)(void *, const char *))get_absolute_path, GameCfg.CMMiscMusic[s])
#endif

int sound_menuset(newmenu *menu, d_event *event, void *userdata)
{
	newmenu_item *items = newmenu_get_items(menu);
	int citem = newmenu_get_citem(menu);
	//int nitems = newmenu_get_nitems(menu);
	int replay = 0;
	int rval = 0;

	switch (event->type)
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
			if (citem == opt_sm_mtype3_lmpath)
			{
				static char *ext_list[] = { ".m3u", NULL };		// select a directory or M3U playlist
				select_file_recursive(
#ifndef _WIN32
					"Select directory or\nM3U playlist to\n play level music from",
#else
					"Select directory or\nM3U playlist to\n play level music from.\n CTRL-D to change drive",
#endif
									  GameCfg.CMLevelMusicPath, ext_list, 1,	// look in current music path for ext_list files and allow directory selection
									  (int (*)(void *, const char *))get_absolute_path, GameCfg.CMLevelMusicPath);	// just copy the absolute path
			}
			else if (citem == opt_sm_cm_mtype3_file1_b)
#ifndef _WIN32
				SELECT_SONG("Select main menu music", SONG_TITLE);
#else
				SELECT_SONG("Select main menu music.\nCTRL-D to change drive", SONG_TITLE);
#endif
			else if (citem == opt_sm_cm_mtype3_file2_b)
#ifndef _WIN32
				SELECT_SONG("Select briefing music", SONG_BRIEFING);
#else
				SELECT_SONG("Select briefing music.\nCTRL-D to change drive", SONG_BRIEFING);
#endif
			else if (citem == opt_sm_cm_mtype3_file3_b)
#ifndef _WIN32
				SELECT_SONG("Select credits music", SONG_CREDITS);
#else
				SELECT_SONG("Select credits music.\nCTRL-D to change drive", SONG_CREDITS);
#endif
			else if (citem == opt_sm_cm_mtype3_file4_b)
#ifndef _WIN32
				SELECT_SONG("Select escape sequence music", SONG_ENDLEVEL);
#else
				SELECT_SONG("Select escape sequence music.\nCTRL-D to change drive", SONG_ENDLEVEL);
#endif
			else if (citem == opt_sm_cm_mtype3_file5_b)
#ifndef _WIN32
				SELECT_SONG("Select game ending music", SONG_ENDGAME);
#else
				SELECT_SONG("Select game ending music.\nCTRL-D to change drive", SONG_ENDGAME);
#endif
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

	userdata = userdata;

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
	char old_CMLevelMusicPath[PATH_MAX+1], old_CMMiscMusic0[PATH_MAX+1];

	memset(old_CMLevelMusicPath, 0, sizeof(char)*(PATH_MAX+1));
	snprintf(old_CMLevelMusicPath, strlen(GameCfg.CMLevelMusicPath)+1, "%s", GameCfg.CMLevelMusicPath);
	memset(old_CMMiscMusic0, 0, sizeof(char)*(PATH_MAX+1));
	snprintf(old_CMMiscMusic0, strlen(GameCfg.CMMiscMusic[SONG_TITLE])+1, "%s", GameCfg.CMMiscMusic[SONG_TITLE]);

	MALLOC(m, newmenu_item, SOUND_MENU_NITEMS);
	if (!m)
		return;

	opt_sm_digivol = nitems;
	m[nitems].type = NM_TYPE_SLIDER; m[nitems].text = TXT_FX_VOLUME; m[nitems].value = GameCfg.DigiVolume; m[nitems].min_value = 0; m[nitems++].max_value = 8;

	opt_sm_musicvol = nitems;
	m[nitems].type = NM_TYPE_SLIDER; m[nitems].text = "music volume"; m[nitems].value = GameCfg.MusicVolume; m[nitems].min_value = 0; m[nitems++].max_value = 8;

	opt_sm_revstereo = nitems;
	m[nitems].type = NM_TYPE_CHECK; m[nitems].text = TXT_REVERSE_STEREO; m[nitems++].value = GameCfg.ReverseStereo;

	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "";

	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "music type:";

	opt_sm_mtype0 = nitems;
	m[nitems].type = NM_TYPE_RADIO; m[nitems].text = "no music"; m[nitems].value = (GameCfg.MusicType == MUSIC_TYPE_NONE); m[nitems].group = 0; nitems++;

#if defined(USE_SDLMIXER) || defined(_WIN32)
	opt_sm_mtype1 = nitems;
	m[nitems].type = NM_TYPE_RADIO; m[nitems].text = "built-in/addon music"; m[nitems].value = (GameCfg.MusicType == MUSIC_TYPE_BUILTIN); m[nitems].group = 0; nitems++;
#endif

	opt_sm_mtype2 = nitems;
	m[nitems].type = NM_TYPE_RADIO; m[nitems].text = "cd music"; m[nitems].value = (GameCfg.MusicType == MUSIC_TYPE_REDBOOK); m[nitems].group = 0; nitems++;

#ifdef USE_SDLMIXER
	opt_sm_mtype3 = nitems;
	m[nitems].type = NM_TYPE_RADIO; m[nitems].text = "jukebox"; m[nitems].value = (GameCfg.MusicType == MUSIC_TYPE_CUSTOM); m[nitems].group = 0; nitems++;

#endif

	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "";
#ifdef USE_SDLMIXER
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "cd music / jukebox options:";
#else
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "cd music options:";
#endif

	opt_sm_redbook_playorder = nitems;
	m[nitems].type = NM_TYPE_CHECK; m[nitems].text = "force mac cd track order"; m[nitems++].value = GameCfg.OrigTrackOrder;

#ifdef USE_SDLMIXER
	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "";

	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "jukebox options:";

	opt_sm_mtype3_lmpath = nitems;
	m[nitems].type = PATH_HEADER_TYPE; m[nitems++].text = "path for level music" BROWSE_TXT;

	m[nitems].type = NM_TYPE_INPUT; m[nitems].text = GameCfg.CMLevelMusicPath; m[nitems++].text_len = NM_MAX_TEXT_LEN-1;

	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "";

	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "level music play order:";

	opt_sm_mtype3_lmplayorder1 = nitems;
	m[nitems].type = NM_TYPE_RADIO; m[nitems].text = "continuously"; m[nitems].value = (GameCfg.CMLevelMusicPlayOrder == MUSIC_CM_PLAYORDER_CONT); m[nitems].group = 1; nitems++;

	opt_sm_mtype3_lmplayorder2 = nitems;
	m[nitems].type = NM_TYPE_RADIO; m[nitems].text = "one track per level"; m[nitems].value = (GameCfg.CMLevelMusicPlayOrder == MUSIC_CM_PLAYORDER_LEVEL); m[nitems].group = 1; nitems++;

	opt_sm_mtype3_lmplayorder3 = nitems;
	m[nitems].type = NM_TYPE_RADIO; m[nitems].text = "random"; m[nitems].value = (GameCfg.CMLevelMusicPlayOrder == MUSIC_CM_PLAYORDER_RAND); m[nitems].group = 1; nitems++;

	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "";

	m[nitems].type = NM_TYPE_TEXT; m[nitems++].text = "non-level music:";

	opt_sm_cm_mtype3_file1_b = nitems;
	m[nitems].type = PATH_HEADER_TYPE; m[nitems++].text = "main menu" BROWSE_TXT;

	opt_sm_cm_mtype3_file1 = nitems;
	m[nitems].type = NM_TYPE_INPUT; m[nitems].text = GameCfg.CMMiscMusic[SONG_TITLE]; m[nitems++].text_len = NM_MAX_TEXT_LEN-1;

	opt_sm_cm_mtype3_file2_b = nitems;
	m[nitems].type = PATH_HEADER_TYPE; m[nitems++].text = "briefing" BROWSE_TXT;

	opt_sm_cm_mtype3_file2 = nitems;
	m[nitems].type = NM_TYPE_INPUT; m[nitems].text = GameCfg.CMMiscMusic[SONG_BRIEFING]; m[nitems++].text_len = NM_MAX_TEXT_LEN-1;

	opt_sm_cm_mtype3_file3_b = nitems;
	m[nitems].type = PATH_HEADER_TYPE; m[nitems++].text = "credits" BROWSE_TXT;

	opt_sm_cm_mtype3_file3 = nitems;
	m[nitems].type = NM_TYPE_INPUT; m[nitems].text = GameCfg.CMMiscMusic[SONG_CREDITS]; m[nitems++].text_len = NM_MAX_TEXT_LEN-1;

	opt_sm_cm_mtype3_file4_b = nitems;
	m[nitems].type = PATH_HEADER_TYPE; m[nitems++].text = "escape sequence" BROWSE_TXT;

	opt_sm_cm_mtype3_file4 = nitems;
	m[nitems].type = NM_TYPE_INPUT; m[nitems].text = GameCfg.CMMiscMusic[SONG_ENDLEVEL]; m[nitems++].text_len = NM_MAX_TEXT_LEN-1;

	opt_sm_cm_mtype3_file5_b = nitems;
	m[nitems].type = PATH_HEADER_TYPE; m[nitems++].text = "game ending" BROWSE_TXT;

	opt_sm_cm_mtype3_file5 = nitems;
	m[nitems].type = NM_TYPE_INPUT; m[nitems].text = GameCfg.CMMiscMusic[SONG_ENDGAME]; m[nitems++].text_len = NM_MAX_TEXT_LEN-1;
#endif

	Assert(nitems == SOUND_MENU_NITEMS);

	newmenu_do1( NULL, "Sound Effects & Music", nitems, m, sound_menuset, NULL, 0 );

#ifdef USE_SDLMIXER
	if ( ((Game_wind != NULL) && strcmp(old_CMLevelMusicPath, GameCfg.CMLevelMusicPath)) || ((Game_wind == NULL) && strcmp(old_CMMiscMusic0, GameCfg.CMMiscMusic[SONG_TITLE])) )
	{
		songs_uninit();

		if (Game_wind)
			songs_play_level_song( Current_level_num, 0 );
		else
			songs_play_song(SONG_TITLE, 1);
	}
#endif
}

#define ADD_CHECK(n,txt,v)  do { m[n].type=NM_TYPE_CHECK; m[n].text=txt; m[n].value=v;} while (0)

void do_misc_menu()
{
	newmenu_item m[8];
	int i = 0;

	do {
		ADD_CHECK(0, "Ship auto-leveling", PlayerCfg.AutoLeveling);
		ADD_CHECK(1, "Persistent Debris",PlayerCfg.PersistentDebris);
		ADD_CHECK(2, "Screenshots w/o HUD",PlayerCfg.PRShot);
		ADD_CHECK(3, "No redundant pickup messages",PlayerCfg.NoRedundancy);
		ADD_CHECK(4, "Show Player chat only (Multi)",PlayerCfg.MultiMessages);
		ADD_CHECK(5, "Show D2-style Prox. Bomb Gauge",PlayerCfg.BombGauge);
		ADD_CHECK(6, "Free Flight controls in Automap",PlayerCfg.AutomapFreeFlight);
		ADD_CHECK(7, "No Weapon Autoselect when firing",PlayerCfg.NoFireAutoselect);

		i = newmenu_do1( NULL, "Misc Options", sizeof(m)/sizeof(*m), m, NULL, NULL, i );

		PlayerCfg.AutoLeveling			= m[0].value;
		PlayerCfg.PersistentDebris		= m[1].value;
		PlayerCfg.PRShot 			= m[2].value;
		PlayerCfg.NoRedundancy 			= m[3].value;
		PlayerCfg.MultiMessages 		= m[4].value;
		PlayerCfg.BombGauge 			= m[5].value;
		PlayerCfg.AutomapFreeFlight		= m[6].value;
		PlayerCfg.NoFireAutoselect		= m[7].value;

	} while( i>-1 );

}

#if defined(USE_UDP)
static int multi_player_menu_handler(newmenu *menu, d_event *event, int *menu_choice)
{
	newmenu_item *items = newmenu_get_items(menu);

	switch (event->type)
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
	m[num_options].type=NM_TYPE_MENU; m[num_options].text="HOST GAME"; menu_choice[num_options]=MENU_START_UDP_NETGAME; num_options++;
#ifdef USE_TRACKER
	m[num_options].type=NM_TYPE_MENU; m[num_options].text="FIND LAN/ONLINE GAMES"; menu_choice[num_options]=MENU_JOIN_LIST_UDP_NETGAME; num_options++;
#else
	m[num_options].type=NM_TYPE_MENU; m[num_options].text="FIND LAN GAMES"; menu_choice[num_options]=MENU_JOIN_LIST_UDP_NETGAME; num_options++;
#endif
	m[num_options].type=NM_TYPE_MENU; m[num_options].text="JOIN GAME MANUALLY"; menu_choice[num_options]=MENU_JOIN_MANUAL_UDP_NETGAME; num_options++;
#endif

	newmenu_do3( NULL, TXT_MULTIPLAYER, num_options, m, (int (*)(newmenu *, d_event *, void *))multi_player_menu_handler, menu_choice, 0, NULL );
}
#endif

void do_options_menu()
{
	newmenu_item *m;

	MALLOC(m, newmenu_item, 10);
	if (!m)
		return;

	m[ 0].type = NM_TYPE_MENU;   m[ 0].text="Sound effects & music...";
	m[ 1].type = NM_TYPE_TEXT;   m[ 1].text="";
	m[ 2].type = NM_TYPE_MENU;   m[ 2].text=TXT_CONTROLS_;
	m[ 3].type = NM_TYPE_TEXT;   m[ 3].text="";
	m[ 4].type = NM_TYPE_MENU;   m[ 4].text="Screen resolution...";
	m[ 5].type = NM_TYPE_MENU;   m[ 5].text="Graphics Options...";
	m[ 6].type = NM_TYPE_TEXT;   m[ 6].text="";
	m[ 7].type = NM_TYPE_MENU;   m[ 7].text="Primary autoselect ordering...";
	m[ 8].type = NM_TYPE_MENU;   m[ 8].text="Secondary autoselect ordering...";
	m[ 9].type = NM_TYPE_MENU;   m[ 9].text="Misc Options...";

	// Fall back to main event loop
	// Allows clean closing and re-opening when resolution changes
	newmenu_do3( NULL, TXT_OPTIONS, 10, m, options_menuset, NULL, 0, NULL );
}

#ifndef RELEASE
int polygon_models_viewer_handler(window *wind, d_event *event)
{
	static int view_idx = 0;
	int key = 0;
	static vms_angvec ang;

	switch (event->type)
	{
		case EVENT_WINDOW_ACTIVATED:
			key_toggle_repeat(1);
			view_idx = 0;
			ang.p = ang.b = 0;
			ang.h = F1_0/2;
			break;
		case EVENT_KEY_COMMAND:
			key = event_key_get(event);
			switch (key)
			{
				case KEY_ESC:
					window_close(wind);
					break;
				case KEY_SPACEBAR:
					view_idx ++;
					if (view_idx >= N_polygon_models) view_idx = 0;
					break;
				case KEY_BACKSP:
					view_idx --;
					if (view_idx < 0 ) view_idx = N_polygon_models - 1;
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
					ang.h = F1_0/2;
					break;
				default:
					break;
			}
			return 1;
		case EVENT_WINDOW_DRAW:
			timer_delay(F1_0/60);
			draw_model_picture(view_idx, &ang);
			gr_set_curfont(GAME_FONT);
			gr_set_fontcolor(BM_XRGB(255,255,255), -1);
			gr_printf(FSPACX(1), FSPACY(1), "ESC: leave\nSPACE/BACKSP: next/prev model (%i/%i)\nA/D: rotate y\nW/S: rotate x\nQ/E: rotate z\nR: reset orientation",view_idx,N_polygon_models-1);
			break;
		case EVENT_WINDOW_CLOSE:
			key_toggle_repeat(0);
			break;
		default:
			break;
	}
	
	return 0;
}

void polygon_models_viewer()
{
	window *wind = window_create(&grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT, (int (*)(window *, d_event *, void *))polygon_models_viewer_handler, NULL);
	if (!wind)
	{
		d_event event = { EVENT_WINDOW_CLOSE };
		polygon_models_viewer_handler(NULL, &event);
		return;
	}

	while (window_exists(wind))
		event_process();
}

int gamebitmaps_viewer_handler(window *wind, d_event *event)
{
	static int view_idx = 0;
	int key = 0;
#ifdef OGL
	float scale = 1.0;
#endif
	bitmap_index bi;
	grs_bitmap *bm;
	extern int Num_bitmap_files;

	switch (event->type)
	{
		case EVENT_WINDOW_ACTIVATED:
			key_toggle_repeat(1);
			view_idx = 0;
			break;
		case EVENT_KEY_COMMAND:
			key = event_key_get(event);
			switch (key)
			{
				case KEY_ESC:
					window_close(wind);
					break;
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
			return 1;
		case EVENT_WINDOW_DRAW:
			bi.index = view_idx;
			bm = &GameBitmaps[view_idx];
			timer_delay(F1_0/60);
			PIGGY_PAGE_IN(bi);
			gr_clear_canvas( BM_XRGB(0,0,0) );
#ifdef OGL
			scale = (bm->bm_w > bm->bm_h)?(SHEIGHT/bm->bm_w)*0.8:(SHEIGHT/bm->bm_h)*0.8;
			ogl_ubitmapm_cs((SWIDTH/2)-(bm->bm_w*scale/2),(SHEIGHT/2)-(bm->bm_h*scale/2),bm->bm_w*scale,bm->bm_h*scale,bm,-1,F1_0);
#else
			gr_bitmap((SWIDTH/2)-(bm->bm_w/2), (SHEIGHT/2)-(bm->bm_h/2), bm);
#endif
			gr_set_curfont(GAME_FONT);
			gr_set_fontcolor(BM_XRGB(255,255,255), -1);
			gr_printf(FSPACX(1), FSPACY(1), "ESC: leave\nSPACE/BACKSP: next/prev bitmap (%i/%i)",view_idx,Num_bitmap_files-1);
			break;
		case EVENT_WINDOW_CLOSE:
			key_toggle_repeat(0);
			break;
		default:
			break;
	}
	
	return 0;
}

void gamebitmaps_viewer()
{
	window *wind = window_create(&grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT, (int (*)(window *, d_event *, void *))gamebitmaps_viewer_handler, NULL);
	if (!wind)
	{
		d_event event = { EVENT_WINDOW_CLOSE };
		gamebitmaps_viewer_handler(NULL, &event);
		return;
	}

	while (window_exists(wind))
		event_process();
}

int sandbox_menuset(newmenu *menu, d_event *event, void *userdata)
{
	switch (event->type)
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

	userdata = userdata; //kill warning

	return 0;
}

void do_sandbox_menu()
{
	newmenu_item *m;

	MALLOC(m, newmenu_item, 2);
	if (!m)
		return;

	m[ 0].type = NM_TYPE_MENU;   m[ 0].text="Polygon_models viewer";
	m[ 1].type = NM_TYPE_MENU;   m[ 1].text="GameBitmaps viewer";

	newmenu_do3( NULL, "Coder's sandbox", 2, m, sandbox_menuset, NULL, 0, NULL );
}
#endif
