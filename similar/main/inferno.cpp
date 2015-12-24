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
 * inferno.c: Entry point of program (main procedure)
 *
 * After main initializes everything, most of the time is spent in the loop
 * while (window_get_front())
 * In this loop, the main menu is brought up first.
 *
 * main() for Inferno
 *
 */

extern const char copyright[];

const
#if defined(DXX_BUILD_DESCENT_I)
char copyright[] = "DESCENT   COPYRIGHT (C) 1994,1995 PARALLAX SOFTWARE CORPORATION";
#elif defined(DXX_BUILD_DESCENT_II)
char copyright[] = "DESCENT II  COPYRIGHT (C) 1994-1996 PARALLAX SOFTWARE CORPORATION";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <SDL.h>

#ifdef __unix__
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include "pstypes.h"
#include "strutil.h"
#include "console.h"
#include "gr.h"
#include "key.h"
#include "3d.h"
#include "bm.h"
#include "inferno.h"
#include "dxxerror.h"
#include "player.h"
#include "game.h"
#include "segment.h"		//for Side_to_verts
#include "u_mem.h"
#include "screens.h"
#include "texmap.h"
#include "texmerge.h"
#include "menu.h"
#include "digi.h"
#include "palette.h"
#include "args.h"
#include "titles.h"
#include "text.h"
#include "gauges.h"
#include "gamefont.h"
#include "kconfig.h"
#include "newmenu.h"
#include "config.h"
#include "multi.h"
#include "songs.h"
#include "gameseq.h"
#if defined(DXX_BUILD_DESCENT_II)
#include "gamepal.h"
#include "movie.h"
#endif
#include "playsave.h"
#include "collide.h"
#include "newdemo.h"
#include "joy.h"
#include "../texmap/scanline.h" //for select_tmap -MM
#include "event.h"
#include "rbaudio.h"
#ifndef __linux__
#include "messagebox.h"
#else
#ifdef WORDS_NEED_ALIGNMENT
#include <sys/prctl.h>
#endif
#endif
#ifdef EDITOR
#include "editor/editor.h"
#include "editor/kdefs.h"
#include "ui.h"
#endif
#include "vers_id.h"
#ifdef USE_UDP
#include "net_udp.h"
#endif

#include "dxxsconf.h"
#include "compiler-begin.h"

namespace dsx {

int Screen_mode=-1;					//game screen or editor screen?

#if defined(DXX_BUILD_DESCENT_I)
int HiresGFXAvailable = 0;
int MacHog = 0;	// using a Mac hogfile?
#endif

//read help from a file & print to screen
static void print_commandline_help()
{
	printf( "\n System Options:\n\n");
	printf( "  -nonicefps                    Don't free CPU-cycles\n");
	printf( "  -maxfps <n>                   Set maximum framerate to <n>\n\t\t\t\t(default: %i, available: %i-%i)\n", MAXIMUM_FPS, MINIMUM_FPS, MAXIMUM_FPS);
	printf( "  -hogdir <s>                   set shared data directory to <s>\n");
#if defined(__unix__)
	printf( "  -nohogdir                     don't try to use shared data directory\n");
#endif
	printf( "  -add-missions-dir <s>         Add contents of location <s> to the missions directory\n");
	printf( "  -use_players_dir              Put player files and saved games in Players subdirectory\n");
	printf( "  -lowmem                       Lowers animation detail for better performance with\n\t\t\t\tlow memory\n");
	printf( "  -pilot <s>                    Select pilot <s> automatically\n");
	printf( "  -auto-record-demo             Start recording on level entry\n");
	printf( "  -record-demo-format           Set demo name automatically\n");
	printf( "  -autodemo                     Start in demo mode\n");
	printf( "  -window                       Run the game in a window\n");
	printf( "  -noborders                    Don't show borders in window mode\n");
#if defined(DXX_BUILD_DESCENT_I)
	printf( "  -notitles                     Skip title screens\n");
#elif defined(DXX_BUILD_DESCENT_II)
	printf( "  -nomovies                     Don't play movies\n");
#endif

	printf( "\n Controls:\n\n");
	printf( "  -nocursor                     Hide mouse cursor\n");
	printf( "  -nomouse                      Deactivate mouse\n");
	printf( "  -nojoystick                   Deactivate joystick\n");
	printf( "  -nostickykeys                 Make CapsLock and NumLock non-sticky\n");

	printf( "\n Sound:\n\n");
	printf( "  -nosound                      Disables sound output\n");
	printf( "  -nomusic                      Disables music output\n");
#if defined(DXX_BUILD_DESCENT_II)
	printf( "  -sound11k                     Use 11KHz sounds\n");
#endif
#ifdef    USE_SDLMIXER
	printf( "  -nosdlmixer                   Disable Sound output via SDL_mixer\n");
#endif // USE SDLMIXER

	printf( "\n Graphics:\n\n");
	printf( "  -lowresfont                   Force use of low resolution fonts\n");
#if defined(DXX_BUILD_DESCENT_II)
	printf( "  -lowresgraphics               Force use of low resolution graphics\n");
	printf( "  -lowresmovies                 Play low resolution movies if available (for slow machines)\n");
#endif
#ifdef    OGL
	printf( "  -gl_fixedfont                 Don't scale fonts to current resolution\n");
	printf( "  -gl_syncmethod <n>            OpenGL sync method (default: %i)\n", OGL_SYNC_METHOD_DEFAULT);
	printf( "                                    0: Disabled\n");
	printf( "                                    1: Fence syncs, limit GPU latency to at most one frame\n");
	printf( "                                    2: Like 1, but sleep during sync to reduce CPU load\n");
	printf( "                                    3: Immediately sync after buffer swap\n");
	printf( "                                    4: Immediately sync after buffer swap\n");
	printf( "                                    5: Auto. use mode 2 if available, 0 otherwise\n");
	printf( "  -gl_syncwait <n>              Wait interval (ms) for sync mode 2 (default: %i)\n", OGL_SYNC_WAIT_DEFAULT);
#endif // OGL

#if defined(USE_UDP)
	printf( "\n Multiplayer:\n\n");
	printf( "  -udp_hostaddr <s>             Use IP address/Hostname <s> for manual game joining\n\t\t\t\t(default: %s)\n", UDP_MANUAL_ADDR_DEFAULT);
	printf( "  -udp_hostport <n>             Use UDP port <n> for manual game joining (default: %i)\n", UDP_PORT_DEFAULT);
	printf( "  -udp_myport <n>               Set my own UDP port to <n> (default: %i)\n", UDP_PORT_DEFAULT);
	printf( "  -no-tracker                   Disable tracker (unless overridden by later -tracker_hostaddr)\n");
#ifdef USE_TRACKER
	printf( "  -tracker_hostaddr <n>         Address of tracker server to register/query games to/from\n\t\t\t\t(default: %s)\n", TRACKER_ADDR_DEFAULT);
	printf( "  -tracker_hostport <n>         Port of tracker server to register/query games to/from\n\t\t\t\t(default: %i)\n", TRACKER_PORT_DEFAULT);
#endif // USE_TRACKER
#endif // defined(USE_UDP)

#ifdef    EDITOR
	printf( "\n Editor:\n\n");
#if defined(DXX_BUILD_DESCENT_I)
	printf( "  -nobm                         Don't load BITMAPS.TBL and BITMAPS.BIN - use internal data\n");
#elif defined(DXX_BUILD_DESCENT_II)
	printf( "  -autoload <s>                 Autoload level <s> in the editor\n");
	printf( "  -macdata                      Read and write Mac data files in editor (swap colors)\n");
	printf( "  -hoarddata                    Make the Hoard ham file from some files, then exit\n");
#endif
#endif // EDITOR

	printf( "\n Debug (use only if you know what you're doing):\n\n");
	printf( "  -debug                        Enable debugging output.\n");
	printf( "  -verbose                      Enable verbose output.\n");
	printf( "  -safelog                      Write gamelog.txt unbuffered.\n\t\t\t\tUse to keep helpful output to trace program crashes.\n");
	printf( "  -norun                        Bail out after initialization\n");
	printf( "  -no-grab                      Never grab keyboard/mouse\n");
	printf( "  -renderstats                  Enable renderstats info by default\n");
	printf( "  -text <s>                     Specify alternate .tex file\n");
	printf( "  -tmap <s>                     Select texmapper <s> to use\n\t\t\t\t(default: c, available: c, fp, quad)\n");
	printf( "  -showmeminfo                  Show memory statistics\n");
	printf( "  -nodoublebuffer               Disable Doublebuffering\n");
	printf( "  -bigpig                       Use uncompressed RLE bitmaps\n");
	printf( "  -16bpp                        Use 16Bpp instead of 32Bpp\n");
#ifdef    OGL
	printf( "  -gl_oldtexmerge               Use old texmerge, uses more ram, but might be faster\n");
	printf( "  -gl_intensity4_ok <n>         Override DbgGlIntensity4Ok (default: 1)\n");
	printf( "  -gl_luminance4_alpha4_ok <n>  Override DbgGlLuminance4Alpha4Ok (default: 1)\n");
	printf( "  -gl_rgba2_ok <n>              Override DbgGlRGBA2Ok (default: 1)\n");
	printf( "  -gl_readpixels_ok <n>         Override DbgGlReadPixelsOk (default: 1)\n");
	printf( "  -gl_gettexlevelparam_ok <n>   Override DbgGlGetTexLevelParamOk (default: 1)\n");
#else
	printf( "  -hwsurface                    Use SDL HW Surface\n");
	printf( "  -asyncblit                    Use queued blits over SDL. Can speed up rendering\n");
#endif // OGL

	printf( "\n Help:\n\n");
	printf( "  -help, -h, -?, ?             View this help screen\n");
	printf( "\n\n");
}

int Quitting = 0;

}

namespace dcx {

// Default event handler for everything except the editor
int standard_handler(const d_event &event)
{
	int key;

	if (Quitting)
	{
		window *wind = window_get_front();
		if (!wind)
			return 0;
	
		if (wind == Game_wind)
		{
			int choice;
			Quitting = 0;
			choice=nm_messagebox( NULL, 2, TXT_YES, TXT_NO, TXT_ABORT_GAME );
			if (choice != 0)
				return 0;
			else
			{
				GameArg.SysAutoDemo = 0;
				Quitting = 1;
			}
		}
		
		// Close front window, let the code flow continue until all windows closed or quit cancelled
		if (!window_close(wind))
			Quitting = 0;
		
		return 1;
	}

	switch (event.type)
	{
		case EVENT_MOUSE_BUTTON_DOWN:
		case EVENT_MOUSE_BUTTON_UP:
			// No window selecting
			// We stay with the current one until it's closed/hidden or another one is made
			// Not the case for the editor
			break;

		case EVENT_KEY_COMMAND:
			key = event_key_get(event);

			switch (key)
			{
#ifdef macintosh
				case KEY_COMMAND + KEY_SHIFTED + KEY_3:
#endif
				case KEY_PRINT_SCREEN:
				{
					gr_set_current_canvas(NULL);
					save_screen_shot(0);
					return 1;
				}

				case KEY_ALTED+KEY_ENTER:
				case KEY_ALTED+KEY_PADENTER:
					if (Game_wind)
						if (Game_wind == window_get_front())
							return 0;
					gr_toggle_fullscreen();
					return 1;

#if defined(__APPLE__) || defined(macintosh)
				case KEY_COMMAND+KEY_Q:
					// Alt-F4 already taken, too bad
					Quitting = 1;
					return 1;
#endif
				case KEY_SHIFTED + KEY_ESC:
					con_showup();
					return 1;
			}
			break;

		case EVENT_WINDOW_DRAW:
		case EVENT_IDLE:
			//see if redbook song needs to be restarted
			RBACheckFinishedHook();
			return 1;

		case EVENT_QUIT:
#ifdef EDITOR
			if (SafetyCheck())
#endif
				Quitting = 1;
			return 1;

		default:
			break;
	}

	return 0;
}

}

namespace dsx {

#define PROGNAME argv[0]

//	DESCENT by Parallax Software
//	DESCENT II by Parallax Software
//	(varies based on preprocessor options)
//		Descent Main

static int main(int argc, char *argv[])
{
	mem_init();
#ifdef __linux__
	error_init(NULL);
#ifdef WORDS_NEED_ALIGNMENT
	prctl(PR_SET_UNALIGN, PR_UNALIGN_NOPRINT, 0, 0, 0);
#endif
#else
	error_init(msgbox_error);
	set_warn_func(msgbox_warning);
#endif
	if (!PHYSFSX_init(argc, argv))
		return 1;
	con_init();  // Initialise the console

	setbuf(stdout, NULL); // unbuffered output via printf
#ifdef _WIN32
	freopen( "CON", "w", stdout );
	freopen( "CON", "w", stderr );
#endif

	if (CGameArg.SysShowCmdHelp) {
		print_commandline_help();

		return(0);
	}

	printf("\nType %s -help' for a list of command-line options.\n\n", PROGNAME);

	PHYSFSX_listSearchPathContent();
	
	if (!PHYSFSX_checkSupportedArchiveTypes())
		return(0);

#if defined(DXX_BUILD_DESCENT_I)
	if (! PHYSFSX_contfile_init("descent.hog", 1))
#define DXX_NAME_NUMBER	"1"
#define DXX_HOGFILE_NAMES	"descent.hog"
#elif defined(DXX_BUILD_DESCENT_II)
	if (! PHYSFSX_contfile_init("descent2.hog", 1) && ! PHYSFSX_contfile_init("d2demo.hog", 1))
#define DXX_NAME_NUMBER	"2"
#define DXX_HOGFILE_NAMES	"descent2.hog or d2demo.hog"
#endif
	{
#if defined(__unix__) && !defined(__APPLE__)
#define DXX_HOGFILE_PROGRAM_DATA_DIRECTORY	\
			      "\t$HOME/.d" DXX_NAME_NUMBER "x-rebirth\n"	\
			      "\t" SHAREPATH "\n"
#else
#define DXX_HOGFILE_PROGRAM_DATA_DIRECTORY	\
				  "\tDirectory containing D" DXX_NAME_NUMBER "X\n"
#endif
#if (defined(__APPLE__) && defined(__MACH__)) || defined(macintosh)
#define DXX_HOGFILE_APPLICATION_BUNDLE	\
				  "\tIn 'Resources' inside the application bundle\n"
#else
#define DXX_HOGFILE_APPLICATION_BUNDLE	""
#endif
#define DXX_MISSING_HOGFILE_ERROR_TEXT	\
		"Could not find a valid hog file (" DXX_HOGFILE_NAMES ")\nPossible locations are:\n"	\
		DXX_HOGFILE_PROGRAM_DATA_DIRECTORY	\
		"\tIn a subdirectory called 'Data'\n"	\
		DXX_HOGFILE_APPLICATION_BUNDLE	\
		"Or use the -hogdir option to specify an alternate location."
		UserError(DXX_MISSING_HOGFILE_ERROR_TEXT);
	}

#if defined(DXX_BUILD_DESCENT_I)
	switch (PHYSFSX_fsize("descent.hog"))
	{
		case D1_MAC_SHARE_MISSION_HOGSIZE:
		case D1_MAC_MISSION_HOGSIZE:
			MacHog = 1;	// used for fonts and the Automap
			break;
	}
#endif

	load_text();

	//print out the banner title
#if defined(DXX_BUILD_DESCENT_I)
	con_printf(CON_NORMAL, "%s  %s", DESCENT_VERSION, g_descent_build_datetime); // D1X version
	con_printf(CON_NORMAL, "This is a MODIFIED version of Descent, based on %s.", BASED_VERSION);
	con_printf(CON_NORMAL, "%s\n%s",TXT_COPYRIGHT,TXT_TRADEMARK);
	con_printf(CON_NORMAL, "Copyright (C) 2005-2013 Christian Beckhaeuser");
#elif defined(DXX_BUILD_DESCENT_II)
	con_printf(CON_NORMAL, "%s%s  %s", DESCENT_VERSION, PHYSFSX_exists(MISSION_DIR "d2x.hog",1) ? "  Vertigo Enhanced" : "", g_descent_build_datetime); // D2X version
	con_printf(CON_NORMAL, "This is a MODIFIED version of Descent 2, based on %s.", BASED_VERSION);
	con_printf(CON_NORMAL, "%s\n%s",TXT_COPYRIGHT,TXT_TRADEMARK);
	con_printf(CON_NORMAL, "Copyright (C) 1999 Peter Hawkins, 2002 Bradley Bell, 2005-2013 Christian Beckhaeuser");
#endif

	if (CGameArg.DbgVerbose)
		con_puts(CON_VERBOSE, TXT_VERBOSE_1);
	
	ReadConfigFile();

	PHYSFSX_addArchiveContent();

	arch_init();

	select_tmap(GameArg.DbgTexMap);

#if defined(DXX_BUILD_DESCENT_II)
	Lighting_on = 1;
#endif

	con_printf(CON_VERBOSE, "Going into graphics mode...");
	gr_set_mode(Game_screen_mode);

	// Load the palette stuff. Returns non-zero if error.
	con_printf(CON_DEBUG, "Initializing palette system..." );
#if defined(DXX_BUILD_DESCENT_I)
	gr_use_palette_table( "PALETTE.256" );
#elif defined(DXX_BUILD_DESCENT_II)
	gr_use_palette_table(D2_DEFAULT_PALETTE );
#endif

	con_printf(CON_DEBUG, "Initializing font system..." );
	gamefont_init();	// must load after palette data loaded.

#if defined(DXX_BUILD_DESCENT_II)
	con_printf( CON_DEBUG, "Initializing movie libraries..." );
	init_movies();		//init movie libraries
#endif

	show_titles();

	set_screen_mode(SCREEN_MENU);

	con_printf( CON_DEBUG, "\nDoing gamedata_init..." );
	gamedata_init();

#if defined(DXX_BUILD_DESCENT_II)
	#ifdef EDITOR
	if (GameArg.EdiSaveHoardData) {
		save_hoard_data();
		exit(1);
	}
	#endif
#endif

	if (GameArg.DbgNoRun)
		return(0);

	con_printf( CON_DEBUG, "\nInitializing texture caching system..." );
	texmerge_init( 10 );		// 10 cache bitmaps

#if defined(DXX_BUILD_DESCENT_II)
	piggy_init_pigfile("groupa.pig");	//get correct pigfile
#endif

	con_printf( CON_DEBUG, "\nRunning game..." );
	init_game();

	get_local_player().callsign = {};

#if defined(DXX_BUILD_DESCENT_I)
	key_flush();
#elif defined(DXX_BUILD_DESCENT_II)
	//	If built with editor, option to auto-load a level and quit game
	//	to write certain data.
	#ifdef	EDITOR
	if (!GameArg.EdiAutoLoad.empty()) {
		Players[0].callsign = "dummy";
	} else
	#endif
#endif
	{
		if (!CGameArg.SysPilot.empty())
		{
			char filename[32] = "";
			unsigned j;

			snprintf(filename, sizeof(filename), PLAYER_DIRECTORY_STRING("%.12s"), CGameArg.SysPilot.c_str());
			for (j = GameArg.SysUsePlayersDir? 8 : 0; filename[j] != '\0'; j++) {
				switch (filename[j]) {
					case ' ':
						filename[j] = '\0';
				}
			}
			if (j < sizeof(filename) - 4 && (j <= 4 || strcmp(&filename[j - 4], ".plr"))) // if player hasn't specified .plr extension in argument, add it
			{
				strcpy(&filename[j], ".plr");
				j += 4;
			}
			if(PHYSFSX_exists(filename,0))
			{
				filename[j - 4] = 0;
				char *b = GameArg.SysUsePlayersDir? &filename[8] : filename;
				get_local_player().callsign.copy(b, std::distance(b, end(filename)));
				read_player_file();
				WriteConfigFile();
			}
		}
	}

#if defined(DXX_BUILD_DESCENT_II)
#ifdef EDITOR
	if (!GameArg.EdiAutoLoad.empty()) {
		/* Any number >= FILENAME_LEN works */
		Level_names[0].copy_if(GameArg.EdiAutoLoad.c_str(), GameArg.EdiAutoLoad.size());
		LoadLevel(1, 1);
	}
	else
#endif
#endif
	{
		Game_mode = GM_GAME_OVER;
		DoMenu();
	}

	while (window_get_front())
		// Send events to windows and the default handler
		event_process();
	
	// Tidy up - avoids a crash on exit
	{
		window *wind;

		show_menus();
		while ((wind = window_get_front()))
			window_close(wind);
	}

	WriteConfigFile();
	show_order_form();

	con_printf( CON_DEBUG, "\nCleanup..." );
	close_game();
	texmerge_close();
	gamedata_close();
	gamefont_close();
	free_text();
	newmenu_free_background();
	Current_mission.reset();
	PHYSFSX_removeArchiveContent();

	return(0);		//presumably successful exit
}

}

int main(int argc, char *argv[])
{
	return dsx::main(argc, argv);
}
