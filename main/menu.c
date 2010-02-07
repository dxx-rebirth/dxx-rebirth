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
#include "net_ipx.h"
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
#include "config.h"
#include "gauges.h"
#include "hudmsg.h" //for HUD_max_num_disp
#include "multi.h"
#ifdef USE_IPX
#include "net_ipx.h"
#endif
#ifdef USE_UDP
#include "net_udp.h"
#endif
#ifdef EDITOR
#include "editor/editor.h"
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
    MENU_LOAD_LEVEL,
    MENU_CONFIG,
    MENU_REJOIN_NETGAME,
    MENU_DIFFICULTY,
    MENU_HELP,
    MENU_NEW_PLAYER,
    #if defined(USE_UDP) || defined (USE_IPX)
        MENU_MULTIPLAYER,
    #endif

    MENU_SHOW_CREDITS,
    MENU_ORDER_INFO,
    MENU_PLAY_SONG,

    #ifdef USE_UDP
    MENU_START_UDP_NETGAME,
    MENU_JOIN_MANUAL_UDP_NETGAME,
    MENU_JOIN_LIST_UDP_NETGAME,
    #endif
    #ifdef USE_IPX
    MENU_START_IPX_NETGAME,
    MENU_JOIN_IPX_NETGAME,
    MENU_START_KALI_NETGAME, // xKali support (not Windows Kali! Windows Kali is over IPX!)
    MENU_JOIN_KALI_NETGAME,
    #endif
};

//ADD_ITEM("Start netgame...", MENU_START_NETGAME, -1 );
//ADD_ITEM("Send net message...", MENU_SEND_NET_MESSAGE, -1 );

#define ADD_ITEM(t,value,key)  do { m[num_options].type=NM_TYPE_MENU; m[num_options].text=t; menu_choice[num_options]=value;num_options++; } while (0)

// Function Prototypes added after LINTING
void do_option(int select);
void do_new_game_menu(void);
void do_multi_player_menu();
extern void newmenu_close();
extern void ReorderPrimary();
extern void ReorderSecondary();

//pairs of chars describing ranges
char playername_allowed_chars[] = "azAZ09__--";

int MakeNewPlayerFile(int allow_abort)
{
	int x;
	char filename[14];
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
	
	sprintf( filename, GameArg.SysUsePlayersDir? "Players/%s.plr" : "%s.plr", text );
	
	if (PHYSFS_exists(filename))
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
	
	switch (((d_event_keycommand *)event)->keycode)
	{
		case KEY_CTRLED+KEY_D:
			if (citem > 0)
			{
				int x = 1;
				x = nm_messagebox( NULL, 2, TXT_YES, TXT_NO, "%s %s?", TXT_DELETE_PILOT, items[citem]+((items[citem][0]=='$')?1:0) );
				if (x==0)	{
					char * p;
					char plxfile[PATH_MAX], efffile[PATH_MAX];
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
						if (cfexist(plxfile))
							PHYSFS_delete(plxfile);
						// delete EFF file
						sprintf(efffile, GameArg.SysUsePlayersDir? "Players/%.8s.eff" : "%.8s.eff", items[citem]);
						if (cfexist(efffile))
							PHYSFS_delete(efffile);
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

int fname_sort_func(char **e0, char **e1);

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
		// Read the last player's name from config file, not lastplr.txt
		strncpy( Players[Player_num].callsign, GameCfg.LastPlayer, CALLSIGN_LEN );
		
		if (GameCfg.LastPlayer[0]==0)
			allow_abort_flag = 0;
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
		
		m[i++] = *f;
		p = strchr(*f, '.');
		if (p)
			*p = '\0';		// chop the .plr
		if ((p - *f) > 8)
			*f[8] = 0;		// sorry guys, can only have up to eight chars for the player name
	}
	
	// Sort by name, except the <Create New Player> string
	qsort(&m[1], NumItems - 1, sizeof(char *), (int (*)( const void *, const void * ))fname_sort_func);

	for ( i=0; i<NumItems; i++ )
		if (!stricmp(Players[Player_num].callsign, m[i]) )
			citem = i;

	newmenu_listbox1(TXT_SELECT_PILOT, NumItems, m, allow_abort_flag, citem, (int (*)(listbox *, d_event *, void *))player_menu_handler, list);
	
	return 1;
}

int main_menu_handler(newmenu *menu, d_event *event, int *menu_choice )
{
	int curtime;

	switch (event->type)
	{
		case EVENT_WINDOW_ACTIVATED:
			if ( Players[Player_num].callsign[0]==0 )
				RegisterPlayer();
			else
				keyd_time_when_last_pressed = timer_get_fixed_seconds();		// .. 20 seconds from now!
			break;
			
		case EVENT_KEY_COMMAND:
			// Don't allow them to hit ESC in the main menu.
			if (((d_event_keycommand *)event)->keycode==KEY_ESC)
				return 1;
			
		case EVENT_IDLE:
			curtime = timer_get_fixed_seconds();
			if ( keyd_time_when_last_pressed+i2f(45) < curtime || GameArg.SysAutoDemo  )
			{
				if (curtime < 0) curtime = 0;
				keyd_time_when_last_pressed = curtime;			// Reset timer so that disk won't thrash if no demos.
				newdemo_start_playback(NULL);		// Randomly pick a file
				if (Newdemo_state == ND_STATE_PLAYBACK)
				{
					Function_mode = FMODE_GAME;
					return -2;
				}
			}
			break;
			
		default:
			break;
	}

	return 0;
}

static int main_menu_choice = 0;

//	-----------------------------------------------------------------------------
//	Create the main menu.
void create_main_menu(newmenu_item *m, int *menu_choice, int *callers_num_options)
{
	int	num_options;

	#ifndef DEMO_ONLY
	num_options = 0;

	set_screen_mode (SCREEN_MENU);

	ADD_ITEM(TXT_NEW_GAME,MENU_NEW_GAME,KEY_N);

	ADD_ITEM(TXT_LOAD_GAME,MENU_LOAD_GAME,KEY_L);
#if defined(USE_UDP) || defined(USE_IPX)
	ADD_ITEM(TXT_MULTIPLAYER_,MENU_MULTIPLAYER,-1);
#endif

	ADD_ITEM(TXT_OPTIONS_, MENU_CONFIG, -1 );
	ADD_ITEM(TXT_CHANGE_PILOTS,MENU_NEW_PLAYER,unused);
	ADD_ITEM(TXT_VIEW_DEMO,MENU_DEMO_PLAY,0);
	ADD_ITEM(TXT_VIEW_SCORES,MENU_VIEW_SCORES,KEY_V);
	if (!cfexist("warning.pcx")) /* SHAREWARE */
		ADD_ITEM(TXT_ORDERING_INFO,MENU_ORDER_INFO,-1);
	ADD_ITEM(TXT_CREDITS,MENU_SHOW_CREDITS,-1);
	#endif
	ADD_ITEM(TXT_QUIT,MENU_QUIT,KEY_Q);

	#ifndef RELEASE
	if (!(Game_mode & GM_MULTI ))	{
		//m[num_options].type=NM_TYPE_TEXT;
		//m[num_options++].text=" Debug options:";

		ADD_ITEM("  Load level...",MENU_LOAD_LEVEL ,KEY_N);
		#ifdef EDITOR
		ADD_ITEM("  Editor", MENU_EDITOR, KEY_E);
		#endif
	}

	ADD_ITEM( "  Play song", MENU_PLAY_SONG, -1 );
	#endif

	*callers_num_options = num_options;
}

//returns number of item chosen
int DoMenu() 
{
	int menu_choice[25];
	newmenu_item m[25];
	int num_options = 0;

	memset(&menu_choice, 0, sizeof(int)*25);
	memset(&m, 0, sizeof(newmenu_item)*25);

	create_main_menu(m, menu_choice, &num_options); // may have to change, eg, maybe selected pilot and no save games.

	if (main_menu_choice < 0 )
		main_menu_choice = 0;
	main_menu_choice = newmenu_do2( "", NULL, num_options, m, (int (*)(newmenu *, d_event *, void *))main_menu_handler, NULL, main_menu_choice, Menu_pcx_name);
	if ( main_menu_choice > -1 ) do_option(menu_choice[main_menu_choice]);

	return main_menu_choice;
}

extern void show_order_form(void);	// John didn't want this in inferno.h so I just externed it.

int select_song_callback(listbox *lb, d_event *event, void *userdata)
{
	int citem = listbox_get_citem(lb);
	
	userdata = userdata;
	if (event->type != EVENT_NEWMENU_SELECTED)
		return 0;
	
	if (citem > -1)
		songs_play_song( citem, 0 );
	
	return 1;	// stay in menu until user escapes
}

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
			select_demo();
			break;
		case MENU_LOAD_GAME:
			state_restore_all(0);
			break;
		#ifdef EDITOR
		case MENU_EDITOR:
			Function_mode = FMODE_EDITOR;
			init_cockpit();

			create_new_mine();
			SetPlayerFromCurseg();

			keyd_editor_mode = 1;
			editor();
			if ( Function_mode == FMODE_GAME ) {
				Game_mode = GM_EDITOR;
				editor_reset_stuff_on_level();
				N_players = 1;
			}
			break;
		#endif
		case MENU_VIEW_SCORES:
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
			Function_mode = FMODE_EXIT;
			break;
		case MENU_NEW_PLAYER:
			RegisterPlayer();		//1 == allow escape out of menu
			break;

#ifndef RELEASE

		case MENU_PLAY_SONG:
		{
			char * m[MAX_NUM_SONGS];
			int i;

			for (i=0;i<Num_songs;i++)
				m[i] = Songs[i].filename;
			newmenu_listbox( "Select Song", Num_songs, m, 1, select_song_callback, NULL );
			break;
		}
		case MENU_LOAD_LEVEL:
			if (Current_mission || select_mission(0, "Load Level\n\nSelect mission"))
			{
				newmenu_item m;
				char text[10]="";
				int new_level_num;
	
				m.type=NM_TYPE_INPUT; m.text_len = 10; m.text = text;
	
				newmenu_do( NULL, "Enter level to load", 1, &m, NULL, NULL );
	
				new_level_num = atoi(m.text);
	
				if (new_level_num!=0 && new_level_num>=Last_secret_level && new_level_num<=Last_level)  {
					StartNewGame(new_level_num);
				}
			}
			break;

#endif //ifndef RELEASE


#ifdef USE_UDP
		case MENU_START_UDP_NETGAME:
			multi_protocol = MULTI_PROTO_UDP;
			net_udp_start_game();
			break;
		case MENU_JOIN_MANUAL_UDP_NETGAME:
			multi_protocol = MULTI_PROTO_UDP;
			net_udp_manual_join_game();
			break;
		case MENU_JOIN_LIST_UDP_NETGAME:
			multi_protocol = MULTI_PROTO_UDP;
			//net_udp_list_join_game();
			break;
#endif
#ifdef USE_IPX
		case MENU_START_IPX_NETGAME:
			multi_protocol = MULTI_PROTO_IPX;
			ipxdrv_set(NETPROTO_IPX);
			net_ipx_start_game();
			break;
		case MENU_JOIN_IPX_NETGAME:
			multi_protocol = MULTI_PROTO_IPX;
			ipxdrv_set(NETPROTO_IPX);
			net_ipx_join_game();
			break;
		case MENU_START_KALI_NETGAME:
			multi_protocol = MULTI_PROTO_IPX;
			ipxdrv_set(NETPROTO_KALINIX);
			net_ipx_start_game();
			break;
		case MENU_JOIN_KALI_NETGAME:
			multi_protocol = MULTI_PROTO_IPX;
			ipxdrv_set(NETPROTO_KALINIX);
			net_ipx_join_game();
			break;
#endif
#if defined(USE_UDP) || defined(USE_IPX)
		case MENU_MULTIPLAYER:
			do_multi_player_menu();
			break;
#endif
		case MENU_CONFIG:
			do_options_menu();
			break;
		case MENU_SHOW_CREDITS:
			songs_stop_all();
			credits_show(NULL);
			break;
		default:
			Error("Unknown option %d in do_option",select);
			break;
	}

}

void delete_player_saved_games(char * name)
{
	int i;
	char filename[FILENAME_LEN + 9];
	
	for (i=0;i<10; i++)
	{
		sprintf( filename, GameArg.SysUsePlayersDir? "Players/%s.sg%x" : "%s.sg%x", name, i );
		
		PHYSFS_delete(filename);
	}
}

int fname_sort_func(char **e0, char **e1)
{
	return stricmp(*e0, *e1);
}

int demo_menu_keycommand( listbox *lb, d_event *event )
{
	char **items = listbox_get_items(lb);
	int citem = listbox_get_citem(lb);
	
	switch (((d_event_keycommand *)event)->keycode)
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
			// return 1;		// later - when the listbox uses the main loop
			break;
			
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
	char *types[] = { ".dem", NULL };
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
	qsort(list, NumItems, sizeof(char *), (int (*)( const void *, const void * ))fname_sort_func);

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

		strcpy(num_text,"1");

		choice = newmenu_do( NULL, TXT_SELECT_START_LEV, n_items, m, NULL, NULL );

		if (choice==-1 || m[1].text[0]==0)
			return;

		new_level_num = atoi(m[1].text);

		if (!(new_level_num>0 && new_level_num<=player_highest_level)) {
			m[0].text = TXT_ENTER_TO_CONT;
			nm_messagebox( NULL, 1, TXT_OK, TXT_INVALID_LEVEL); 
			goto try_again;
		}
	}

	Difficulty_level = PlayerCfg.DefaultDifficulty;

	if (!do_difficulty_menu())
		return;

	StartNewGame(new_level_num);

}

void do_sound_menu();
void input_config();
void change_res();
void do_graphics_menu();
void do_misc_menu();

int options_menuset(newmenu *menu, d_event *event, void *userdata)
{
	switch (event->type)
	{
		case EVENT_NEWMENU_CHANGED:
			if ( newmenu_get_citem(menu)==4)
			{
				gr_palette_set_gamma(newmenu_get_items(menu)[4].value);
			}
			break;
			
		case EVENT_NEWMENU_SELECTED:
			switch(newmenu_get_citem(menu))
			{
				case  0: do_sound_menu();		break;
				case  2: input_config();		break;
				case  5: change_res();			break;
				case  6: do_graphics_menu();		break;
				case  8: ReorderPrimary();		break;
				case  9: ReorderSecondary();		break;
				case 10: do_misc_menu();		break;
			}
			return 1;	// stay in menu until escape
			break;
			
		case EVENT_WINDOW_CLOSE:
			write_player_file();
			break;
			
		default:
			break;
	}

	userdata++;		//kill warning
	
	return 0;
}

int gcd(int a, int b)
{
	if (!b)
		return a; 

	return gcd(b, a%b);
}

int change_res_poll() { return 0; }

void change_res()
{
	newmenu_item m[15];
	u_int32_t modes[15];
	int i = 0, mc = 0, num_presets = 0;
	char customres[16], aspect[16];
	int fullscreenc;
	u_int32_t screen_mode = 0, aspect_mode = 0;

	// the list of pre-defined resolutions
	if (gr_check_mode(SM(320,200)))		{ m[mc].type = NM_TYPE_RADIO; m[mc].text = "320x200 (16x10)"; m[mc].value = (Game_screen_mode == SM(320,200)); m[mc].group = 0; modes[mc] = SM(320,200); mc++; }
	if (gr_check_mode(SM(640,480)))		{ m[mc].type = NM_TYPE_RADIO; m[mc].text = "640x480 (4x3)"; m[mc].value = (Game_screen_mode == SM(640,480)); m[mc].group = 0; modes[mc] = SM(640,480); mc++; }
	if (gr_check_mode(SM(800,600)))		{ m[mc].type = NM_TYPE_RADIO; m[mc].text = "800x600 (4x3)"; m[mc].value = (Game_screen_mode == SM(800,600)); m[mc].group = 0; modes[mc] = SM(800,600); mc++; }
	if (gr_check_mode(SM(1024,768)))	{ m[mc].type = NM_TYPE_RADIO; m[mc].text = "1024x768 (4x3)"; m[mc].value = (Game_screen_mode == SM(1024,768)); m[mc].group = 0; modes[mc] = SM(1024,768); mc++; }
	if (gr_check_mode(SM(1280,800)))	{ m[mc].type = NM_TYPE_RADIO; m[mc].text = "1280x800 (16x10)"; m[mc].value = (Game_screen_mode == SM(1280,800)); m[mc].group = 0; modes[mc] = SM(1280,800); mc++; }
	if (gr_check_mode(SM(1280,1024)))	{ m[mc].type = NM_TYPE_RADIO; m[mc].text = "1280x1024 (5x4)"; m[mc].value = (Game_screen_mode == SM(1280,1024)); m[mc].group = 0; modes[mc] = SM(1280,1024); mc++; }
	if (gr_check_mode(SM(1440,900)))	{ m[mc].type = NM_TYPE_RADIO; m[mc].text = "1440x900 (16x10)"; m[mc].value = (Game_screen_mode == SM(1440,900)); m[mc].group = 0; modes[mc] = SM(1440,900); mc++; }
	if (gr_check_mode(SM(1600,1200)))	{ m[mc].type = NM_TYPE_RADIO; m[mc].text = "1600x1200 (4x3)"; m[mc].value = (Game_screen_mode == SM(1600,1200)); m[mc].group = 0; modes[mc] = SM(1600,1200); mc++; }
	if (gr_check_mode(SM(1920,1200)))	{ m[mc].type = NM_TYPE_RADIO; m[mc].text = "1920x1200 (16x10)"; m[mc].value = (Game_screen_mode == SM(1920,1200)); m[mc].group = 0; modes[mc] = SM(1920,1200); mc++; }
	num_presets = mc;

	// now see which field is true and break there
	m[mc].value=0;
	for (i = 0; i < mc; i++)
		if (m[mc].value)
			break;

	// the field for custom resolution and aspect
	m[mc].type = NM_TYPE_RADIO; m[mc].text = "use custom values"; m[mc].value = (i == mc); m[mc].group = 0; modes[mc] = 0; mc++;
	m[mc].type = NM_TYPE_TEXT; m[mc].text = "resolution:"; mc++;
	sprintf(customres, "%ix%i", SM_W(Game_screen_mode), SM_H(Game_screen_mode));
	m[mc].type = NM_TYPE_INPUT; m[mc].text = customres; m[mc].text_len = 11; modes[mc] = 0; mc++;
	m[mc].type = NM_TYPE_TEXT; m[mc].text = "aspect:"; mc++;
	sprintf(aspect, "%ix%i", GameCfg.AspectY, GameCfg.AspectX);
	m[mc].type = NM_TYPE_INPUT; m[mc].text = aspect; m[mc].text_len = 11; modes[mc] = 0; mc++;

	// fullscreen toggle
	fullscreenc = mc; m[mc].type = NM_TYPE_CHECK; m[mc].text = "Fullscreen"; m[mc].value = gr_check_fullscreen(); mc++;

	// create the menu
	i = newmenu_do1(NULL, "Screen Resolution", mc, m, change_res_poll, NULL, 0);

	// menu is done, now do what we need to do

	// now check for fullscreen toggle and apply if necessary
	if (m[fullscreenc].value != gr_check_fullscreen())
		gr_toggle_fullscreen();

	// check which preset field was selected
	for (i = 0; (m[i].value == 0) && (i < num_presets); i++);

	if (modes[i]==0) // no preset selected, use custom values and set screen_mode and aspect
	{
		if (!strchr(customres, 'x'))
			return;

		screen_mode = SM(atoi(customres), atoi(strchr(customres, 'x')+1));
		if (SM_W(screen_mode) < 320 || SM_H(screen_mode) < 200) // oh oh - the resolution is too small. Revert!
		{
			nm_messagebox( TXT_WARNING, 1, "OK", "Entered resolution is too small.\nReverting ..." );
			return;
		}
		if (strchr(aspect, 'x')) // we even have a custom aspect set up
		{
			aspect_mode = SM(atoi(aspect), atoi(strchr(aspect, 'x')+1));
			GameCfg.AspectY = SM_W(aspect_mode)/gcd(SM_W(aspect_mode),SM_H(aspect_mode));
			GameCfg.AspectX = SM_H(aspect_mode)/gcd(SM_W(aspect_mode),SM_H(aspect_mode));
		}
	}
	else // a preset field is selected - set screen_mode and aspect
	{
		screen_mode = modes[i];
		GameCfg.AspectY = SM_W(screen_mode)/gcd(SM_W(screen_mode),SM_H(screen_mode));
		GameCfg.AspectX = SM_H(screen_mode)/gcd(SM_W(screen_mode),SM_H(screen_mode));
	}


	// clean up and apply everything
	newmenu_close();
	set_screen_mode(SCREEN_MENU);
	Game_screen_mode = screen_mode;
	gr_set_mode(Game_screen_mode);
	init_cockpit();
	game_init_render_buffers(SM_W(screen_mode), SM_H(screen_mode), VR_NONE);
	window_close(window_get_front());	// close options dialog - it will be messy with a different resolution
	//do_options_menu();				// reopen it. D'OH: Can't yet, not until we have a main event loop for it
}

int input_menuset(newmenu *menu, d_event *event, void *userdata)
{
	int i;
	newmenu_item *items = newmenu_get_items(menu);
	int citem = newmenu_get_citem(menu);

	userdata = userdata;

	switch (event->type)
	{
		case EVENT_NEWMENU_CHANGED:
			switch (citem)
			{
				case 0:
				case 1:
				case 2:
				case 3:
					PlayerCfg.ControlType = citem;
					if (PlayerCfg.ControlType == 2) PlayerCfg.ControlType = CONTROL_MOUSE;
					if (PlayerCfg.ControlType == 3) PlayerCfg.ControlType = CONTROL_JOYMOUSE;
					kc_set_controls();
					break;
				
				case 10:	PlayerCfg.JoystickSensitivityX = items[citem].value; break;
				case 11:	PlayerCfg.JoystickSensitivityY = items[citem].value; break;
				case 12:	PlayerCfg.JoystickDeadzone = items[citem].value; break;
				case 15:	PlayerCfg.MouseSensitivityX = items[citem].value; break;
				case 16:	PlayerCfg.MouseSensitivityY = items[citem].value; break;
				case 17:	PlayerCfg.MouseFilter = items[citem].value; break;
			}
			break;
			
		case EVENT_NEWMENU_SELECTED:
			i = PlayerCfg.ControlType;
			if (i == CONTROL_MOUSE) i = 2;
			if (i == CONTROL_JOYMOUSE) i = 3;

			switch (citem)
			{
				case 5:
					kconfig(i, items[i].text);
					break;
				case 6:
					kconfig(0, "KEYBOARD");
					break;
				case 7:
					kconfig(4, "WEAPON KEYS");
					break;
				case 19:
					show_help();
					break;
				case 20:
					show_netgame_help();
					break;
				case 21:
					show_newdemo_help();
					break;
			}
			return 1;		// stay in menu
			break;
			
		default:
			break;
	}
	
	return 0;
}

void input_config()
{
	newmenu_item m[22];
	int i;
	int nitems = 22;

	m[0].type = NM_TYPE_RADIO;  m[0].text = "KEYBOARD"; m[0].value = 0; m[0].group = 0;
	m[1].type = NM_TYPE_RADIO;  m[1].text = "JOYSTICK"; m[1].value = 0; m[1].group = 0;
	m[2].type = NM_TYPE_RADIO;  m[2].text = "MOUSE";    m[2].value = 0; m[2].group = 0;
	m[3].type = NM_TYPE_RADIO;  m[3].text = "JOYSTICK & MOUSE"; m[3].value = 0; m[3].group = 0;
	m[4].type = NM_TYPE_TEXT;   m[4].text = "";
	m[5].type = NM_TYPE_MENU;   m[5].text = TXT_CUST_ABOVE;
	m[6].type = NM_TYPE_MENU;   m[6].text = TXT_CUST_KEYBOARD;
	m[7].type = NM_TYPE_MENU;   m[7].text = "CUSTOMIZE WEAPON KEYS";
	m[8].type = NM_TYPE_TEXT;   m[8].text = "";
	m[9].type = NM_TYPE_TEXT;   m[9].text = "Joystick";
	m[10].type = NM_TYPE_SLIDER; m[10].text="X Sensitivity"; m[10].value=PlayerCfg.JoystickSensitivityX; m[10].min_value = 0; m[10].max_value = 16;
	m[11].type = NM_TYPE_SLIDER; m[11].text="Y Sensitivity"; m[11].value=PlayerCfg.JoystickSensitivityY; m[11].min_value = 0; m[11].max_value = 16;
	m[12].type = NM_TYPE_SLIDER; m[12].text="Deadzone"; m[12].value=PlayerCfg.JoystickDeadzone; m[12].min_value=0; m[12].max_value = 16;
	m[13].type = NM_TYPE_TEXT;   m[13].text = "";
	m[14].type = NM_TYPE_TEXT;   m[14].text = "Mouse";
	m[15].type = NM_TYPE_SLIDER; m[15].text="X Sensitivity"; m[15].value=PlayerCfg.MouseSensitivityX; m[15].min_value = 0; m[15].max_value = 16;
	m[16].type = NM_TYPE_SLIDER; m[16].text="Y Sensitivity"; m[16].value=PlayerCfg.MouseSensitivityY; m[16].min_value = 0; m[16].max_value = 16;
	m[17].type = NM_TYPE_CHECK;  m[17].text="Mouse Smoothing/Filtering"; m[17].value=PlayerCfg.MouseFilter;
	m[18].type = NM_TYPE_TEXT;   m[18].text = "";
	m[19].type = NM_TYPE_MENU;   m[19].text = "GAME SYSTEM KEYS";
	m[20].type = NM_TYPE_MENU;   m[20].text = "NETGAME SYSTEM KEYS";
	m[21].type = NM_TYPE_MENU;   m[21].text = "DEMO SYSTEM KEYS";

	i = PlayerCfg.ControlType;
	if (i == CONTROL_MOUSE) i = 2;
	if (i==CONTROL_JOYMOUSE) i = 3;
	m[i].value = 1;

	newmenu_do1(NULL, TXT_CONTROLS, nitems, m, input_menuset, NULL, 5);
}

void do_graphics_menu()
{
	newmenu_item m[9];
	int i = 0, j = 0;

	do {
		m[0].type = NM_TYPE_TEXT;   m[0].text="Texture Filtering:";
		m[1].type = NM_TYPE_RADIO;  m[1].text = "None (Classical)";       m[1].value = 0; m[1].group = 0;
		m[2].type = NM_TYPE_RADIO;  m[2].text = "Bilinear";               m[2].value = 0; m[2].group = 0;
		m[3].type = NM_TYPE_RADIO;  m[3].text = "Trilinear";              m[3].value = 0; m[3].group = 0;
		m[4].type = NM_TYPE_TEXT;   m[4].text="";
		m[5].type = NM_TYPE_CHECK;  m[5].text="Transparency Effects";     m[5].value = PlayerCfg.OglAlphaEffects;
		m[6].type = NM_TYPE_CHECK;  m[6].text="Vectorial Reticle";        m[6].value = PlayerCfg.OglReticle;
		m[7].type = NM_TYPE_CHECK;  m[7].text="VSync";                    m[7].value = GameCfg.VSync;
		m[8].type = NM_TYPE_CHECK;  m[8].text="4x multisampling";         m[8].value = GameCfg.Multisample;

		m[GameCfg.TexFilt+1].value=1;

		i = newmenu_do1( NULL, "Graphics Options", sizeof(m)/sizeof(*m), m, NULL, NULL, i );

		if (GameCfg.VSync != m[7].value || GameCfg.Multisample != m[8].value)
			nm_messagebox( NULL, 1, TXT_OK, "To apply VSync or 4x Multisample\nyou need to restart the program");

		for (j = 0; j <= 2; j++)
			if (m[j+1].value)
				GameCfg.TexFilt = j;
		PlayerCfg.OglAlphaEffects = m[5].value;
		PlayerCfg.OglReticle = m[6].value;
		GameCfg.VSync = m[7].value;
		GameCfg.Multisample = m[8].value;
		gr_set_attributes();
		gr_set_mode(Game_screen_mode);
	} while( i>-1 );
}

void set_extmusic_volume(int volume);

int sound_menuset(newmenu *menu, d_event *event, void *userdata)
{
	newmenu_item *items = newmenu_get_items(menu);

	switch (event->type)
	{
		case EVENT_NEWMENU_CHANGED:
			switch (newmenu_get_citem(menu))
			{
				case 0:
					GameCfg.DigiVolume = items[0].value;
					digi_set_digi_volume( (GameCfg.DigiVolume*32768)/8 );
					digi_play_sample_once( SOUND_DROP_BOMB, F1_0 );
					break;

				case 1:
					GameCfg.MusicVolume = items[1].value;
					if (EXT_MUSIC_ON)
						set_extmusic_volume(GameCfg.MusicVolume);
					else
						digi_set_midi_volume( (GameCfg.MusicVolume*128)/8 );
					break;
			}
			break;
			
		default:
			break;
	}

	userdata = userdata;

	return 0;
}

void do_sound_menu()
{
#ifdef USE_SDLMIXER
	newmenu_item m[10];
#else
	newmenu_item m[7];
#endif
	int i = 0;
	int nitems;

	do {
		if (GameCfg.SndEnableRedbook && GameCfg.JukeboxOn)
			GameCfg.JukeboxOn = 0;

		nitems = 0;

		m[nitems].type = NM_TYPE_SLIDER; m[nitems].text=TXT_FX_VOLUME; m[nitems].value=GameCfg.DigiVolume; m[nitems].min_value=0; m[nitems++].max_value=8; 
		m[nitems].type = NM_TYPE_SLIDER; m[nitems].text="music volume"; m[nitems].value=GameCfg.MusicVolume; m[nitems].min_value=0; m[nitems++].max_value=8;
		m[nitems].type = NM_TYPE_TEXT; m[nitems++].text="";
		m[nitems].type = NM_TYPE_RADIO; m[nitems].text = "MIDI Music enabled"; m[nitems].value = (!GameCfg.SndEnableRedbook && !GameCfg.JukeboxOn); m[nitems].group = 0; nitems++;
		m[nitems].type = NM_TYPE_RADIO;  m[nitems].text="CD Music enabled"; m[nitems].value=GameCfg.SndEnableRedbook; m[nitems].group = 0; nitems++;
#ifdef USE_SDLMIXER
		m[nitems].type = NM_TYPE_RADIO; m[nitems].text="jukebox enabled"; m[nitems].value=GameCfg.JukeboxOn; m[nitems].group = 0; nitems++;
		m[nitems].type = NM_TYPE_TEXT; m[nitems++].text="path to music for jukebox:";
		m[nitems].type = NM_TYPE_INPUT; m[nitems].text = GameCfg.JukeboxPath; m[nitems++].text_len = NM_MAX_TEXT_LEN-1;
#endif
		m[nitems].type = NM_TYPE_CHECK;  m[nitems].text="Force Mac Descent CD track order"; m[nitems++].value=GameCfg.OrigTrackOrder;
		m[nitems].type = NM_TYPE_CHECK;  m[nitems].text=TXT_REVERSE_STEREO; m[nitems++].value=GameCfg.ReverseStereo;
		
		i = newmenu_do1( NULL, "Sound Effects & Music", nitems, m, sound_menuset, NULL, i );

		GameCfg.ReverseStereo = m[nitems - 1].value;
		GameCfg.OrigTrackOrder = m[nitems - 2].value;
		if ((GameCfg.SndEnableRedbook != m[4].value)
#ifdef USE_SDLMIXER
			|| (GameCfg.JukeboxOn != m[5].value) || (GameCfg.JukeboxOn)
#endif
			)
		{
			GameCfg.SndEnableRedbook = m[4].value;
#ifdef USE_SDLMIXER
			GameCfg.JukeboxOn = m[5].value;
#endif
			ext_music_stop();
			ext_music_unload();
			ext_music_select_system(GameCfg.JukeboxOn ? EXT_MUSIC_JUKEBOX : EXT_MUSIC_REDBOOK);
			if (Function_mode == FMODE_GAME)
				songs_play_level_song( Current_level_num );
			else
				songs_play_song(SONG_TITLE, 1);
		}

	} while( i>-1 );
}

#define ADD_CHECK(n,txt,v)  do { m[n].type=NM_TYPE_CHECK; m[n].text=txt; m[n].value=v;} while (0)

void do_misc_menu()
{
	newmenu_item m[4];
	int i = 0;

	do {
		ADD_CHECK(0, "Ship auto-leveling", PlayerCfg.AutoLeveling);
		ADD_CHECK(1, "Show reticle", PlayerCfg.ReticleOn);
		ADD_CHECK(2, "Persistent Debris",PlayerCfg.PersistentDebris);
		ADD_CHECK(3, "Screenshots w/o HUD",PlayerCfg.PRShot);

		i = newmenu_do1( NULL, "Misc Options", sizeof(m)/sizeof(*m), m, NULL, NULL, i );
			
		PlayerCfg.AutoLeveling		= m[0].value;
		PlayerCfg.ReticleOn		= m[1].value;
		PlayerCfg.PersistentDebris	= m[2].value;
		PlayerCfg.PRShot 		= m[3].value;

	} while( i>-1 );

}

#if defined(USE_UDP) || defined(USE_IPX)
void do_multi_player_menu()
{
	int menu_choice[12];
	newmenu_item m[12];
	int choice = 0, num_options = 0;
	int old_game_mode;

	do {
		old_game_mode = Game_mode;
		num_options = 0;

#ifdef USE_UDP
		m[num_options].type=NM_TYPE_TEXT; m[num_options].text="UDP:"; num_options++;
		m[num_options].type=NM_TYPE_MENU; m[num_options].text="HOST GAME"; menu_choice[num_options]=MENU_START_UDP_NETGAME; num_options++;
		m[num_options].type=NM_TYPE_MENU; m[num_options].text="FIND LAN GAMES"; menu_choice[num_options]=MENU_JOIN_LIST_UDP_NETGAME; num_options++;
		//m[num_options].type=NM_TYPE_MENU; m[num_options].text="FIND LAN/ONLINE GAMES"; menu_choice[num_options]=MENU_JOIN_LIST_UDP_NETGAME; num_options++;
		m[num_options].type=NM_TYPE_MENU; m[num_options].text="JOIN GAME MANUALLY"; menu_choice[num_options]=MENU_JOIN_MANUAL_UDP_NETGAME; num_options++;
#endif

#ifdef USE_IPX
		m[num_options].type=NM_TYPE_TEXT; m[num_options].text=""; num_options++;
		m[num_options].type=NM_TYPE_TEXT; m[num_options].text="IPX:"; num_options++;
		m[num_options].type=NM_TYPE_MENU; m[num_options].text="HOST GAME"; menu_choice[num_options]=MENU_START_IPX_NETGAME; num_options++;
		m[num_options].type=NM_TYPE_MENU; m[num_options].text="JOIN GAME"; menu_choice[num_options]=MENU_JOIN_IPX_NETGAME; num_options++;
#ifdef __LINUX__
		m[num_options].type=NM_TYPE_TEXT; m[num_options].text=""; num_options++;
		m[num_options].type=NM_TYPE_TEXT; m[num_options].text="XKALI:"; num_options++;
		m[num_options].type=NM_TYPE_MENU; m[num_options].text="HOST GAME"; menu_choice[num_options]=MENU_START_KALI_NETGAME; num_options++;
		m[num_options].type=NM_TYPE_MENU; m[num_options].text="JOIN GAME"; menu_choice[num_options]=MENU_JOIN_KALI_NETGAME; num_options++;
#endif
#endif

		choice = newmenu_do1( NULL, TXT_MULTIPLAYER, num_options, m, NULL, NULL, choice );
		
		if ( choice > -1 )
			do_option(menu_choice[choice]);
	
		if (old_game_mode != Game_mode)
			break;          // leave menu

	} while( choice > -1 );
}
#endif

void do_options_menu()
{
	newmenu_item m[11];

	m[ 0].type = NM_TYPE_MENU;   m[ 0].text="Sound effects & music...";
	m[ 1].type = NM_TYPE_TEXT;   m[ 1].text="";
	m[ 2].type = NM_TYPE_MENU;   m[ 2].text=TXT_CONTROLS_;
	m[ 3].type = NM_TYPE_TEXT;   m[ 3].text="";

	m[ 4].type = NM_TYPE_SLIDER;
	m[ 4].text = TXT_BRIGHTNESS;
	m[ 4].value = gr_palette_get_gamma();
	m[ 4].min_value = 0;
	m[ 4].max_value = 16;

	m[ 5].type = NM_TYPE_MENU;   m[ 5].text="Screen resolution...";
#ifdef OGL
	m[ 6].type = NM_TYPE_MENU;   m[ 6].text="Graphics Options...";
#else
	m[ 6].type = NM_TYPE_TEXT;   m[ 6].text="";
#endif

	m[ 7].type = NM_TYPE_TEXT;   m[ 7].text="";
	m[ 8].type = NM_TYPE_MENU;   m[ 8].text="Primary autoselect ordering...";
	m[ 9].type = NM_TYPE_MENU;   m[ 9].text="Secondary autoselect ordering...";
	m[10].type = NM_TYPE_MENU;   m[10].text="Misc Options...";

	newmenu_do1( NULL, TXT_OPTIONS, sizeof(m)/sizeof(*m), m, options_menuset, NULL, 0 );
}
