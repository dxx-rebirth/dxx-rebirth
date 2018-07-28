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

#include <cctype>
#include <locale>
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
#if !DXX_USE_OGL
#include "../texmap/scanline.h" //for select_tmap -MM
#endif
#include "event.h"
#include "rbaudio.h"
#if !defined(__linux__) && !defined(__OpenBSD__)
#include "messagebox.h"
#else
#if DXX_WORDS_NEED_ALIGNMENT
#include <sys/prctl.h>
#endif
#endif
#if DXX_USE_EDITOR
#include "editor/editor.h"
#include "editor/kdefs.h"
#include "ui.h"
#endif
#include "vers_id.h"
#if DXX_USE_UDP
#include "net_udp.h"
#endif

#include "dxxsconf.h"
#include "dsx-ns.h"
#include "compiler-begin.h"

#if DXX_USE_SDLMIXER
#include <SDL_mixer.h>
#endif

namespace dsx {

int Screen_mode=-1;					//game screen or editor screen?

#if defined(DXX_BUILD_DESCENT_I)
int HiresGFXAvailable = 0;
int MacHog = 0;	// using a Mac hogfile?
#endif

//read help from a file & print to screen
static void print_commandline_help()
{
#define DXX_COMMAND_LINE_HELP_FMT(FMT,...)	FMT
#define DXX_COMMAND_LINE_HELP_ARG(FMT,...)	, ## __VA_ARGS__

#define DXX_if_defined_placeholder1	,
#define DXX_if_defined_unwrap(A,...)	A, ## __VA_ARGS__
#define DXX_if_defined(V,F)	DXX_if_defined2(V,F)
#define DXX_if_defined2(V,F)	DXX_if_defined3(DXX_if_defined_placeholder##V, F)
#define DXX_if_defined3(V,F)	DXX_if_defined4(F, V 1, 0)
#define DXX_if_defined4(F,_,V,...)	DXX_if_defined5_##V(F)
#define DXX_if_defined5_0(F)
#define DXX_if_defined5_1(F)	DXX_if_defined_unwrap F
#define DXX_if_defined_01(V,F)	DXX_if_defined4(F,,V)

#define DXX_COMMAND_LINE_HELP_unix(V)	DXX_if_defined(__unix__, (V))
#define DXX_COMMAND_LINE_HELP_D1(V)	DXX_if_defined(DXX_BUILD_DESCENT_I, (V))
#define DXX_COMMAND_LINE_HELP_D2(V)	DXX_if_defined(DXX_BUILD_DESCENT_II, (V))
#define DXX_STRINGIZE2(X)	#X
#define DXX_STRINGIZE(X)	DXX_STRINGIZE2(X)

#if DXX_USE_OGL
#define DXX_COMMAND_LINE_HELP_OGL(V)	V
#define DXX_COMMAND_LINE_HELP_SDL(V)
#else
#define DXX_COMMAND_LINE_HELP_OGL(V)
#define DXX_COMMAND_LINE_HELP_SDL(V)	V
#endif

#define DXX_COMMAND_LINE_HELP(VERB)	\
	VERB("\n System Options:\n\n")	\
	VERB("  -nonicefps                    Don't free CPU-cycles\n")	\
	VERB("  -maxfps <n>                   Set maximum framerate to <n>\n\t\t\t\t(default: " DXX_STRINGIZE(MAXIMUM_FPS) ", available: " DXX_STRINGIZE(MINIMUM_FPS) "-" DXX_STRINGIZE(MAXIMUM_FPS) ")\n")	\
	VERB("  -hogdir <s>                   set shared data directory to <s>\n")	\
	DXX_COMMAND_LINE_HELP_unix(	\
		VERB("  -nohogdir                     don't try to use shared data directory\n")	\
	)	\
	VERB("  -add-missions-dir <s>         Add contents of location <s> to the missions directory\n")	\
	VERB("  -use_players_dir              Put player files and saved games in Players subdirectory\n")	\
	VERB("  -lowmem                       Lowers animation detail for better performance with\n\t\t\t\tlow memory\n")	\
	VERB("  -pilot <s>                    Select pilot <s> automatically\n")	\
	VERB("  -auto-record-demo             Start recording on level entry\n")	\
	VERB("  -record-demo-format           Set demo name automatically\n")	\
	VERB("  -autodemo                     Start in demo mode\n")	\
	VERB("  -window                       Run the game in a window\n")	\
	VERB("  -noborders                    Don't show borders in window mode\n")	\
	DXX_COMMAND_LINE_HELP_D1(	\
		VERB("  -notitles                     Skip title screens\n")	\
	)	\
	DXX_COMMAND_LINE_HELP_D2(	\
		VERB("  -nomovies                     Don't play movies\n")	\
	)	\
	VERB("\n Controls:\n\n")	\
	VERB("  -nocursor                     Hide mouse cursor\n")	\
	VERB("  -nomouse                      Deactivate mouse\n")	\
	VERB("  -nojoystick                   Deactivate joystick\n")	\
	VERB("  -nostickykeys                 Make CapsLock and NumLock non-sticky\n")	\
	VERB("\n Sound:\n\n")	\
	VERB("  -nosound                      Disables sound output\n")	\
	VERB("  -nomusic                      Disables music output\n")	\
	DXX_COMMAND_LINE_HELP_D2(	\
		VERB("  -sound11k                     Use 11KHz sounds\n")	\
	)	\
	DXX_if_defined_01(DXX_USE_SDLMIXER, (	\
		VERB("  -nosdlmixer                   Disable Sound output via SDL_mixer\n")	\
	))	\
	VERB("\n Graphics:\n\n")	\
	VERB("  -lowresfont                   Force use of low resolution fonts\n")	\
	DXX_COMMAND_LINE_HELP_D2(	\
		VERB("  -lowresgraphics               Force use of low resolution graphics\n")	\
		VERB("  -lowresmovies                 Play low resolution movies if available (for slow machines)\n")	\
	)	\
	DXX_COMMAND_LINE_HELP_OGL(	\
		VERB("  -gl_fixedfont                 Don't scale fonts to current resolution\n")	\
		VERB("  -gl_syncmethod <n>            OpenGL sync method (default: %i)\n", OGL_SYNC_METHOD_DEFAULT)	\
		VERB("                                    0: Disabled\n")	\
		VERB("                                    1: Fence syncs, limit GPU latency to at most one frame\n")	\
		VERB("                                    2: Like 1, but sleep during sync to reduce CPU load\n")	\
		VERB("                                    3: Immediately sync after buffer swap\n")	\
		VERB("                                    4: Immediately sync after buffer swap\n")	\
		VERB("                                    5: Auto: if VSync is enabled and ARB_sync is supported, use mode 2, otherwise mode 0\n")	\
		VERB("  -gl_syncwait <n>              Wait interval (ms) for sync mode 2 (default: " DXX_STRINGIZE(OGL_SYNC_WAIT_DEFAULT) ")\n")	\
		VERB("  -gl_darkedges                 Re-enable dark edges around filtered textures (as present in earlier versions of the engine)\n")	\
	)	\
	DXX_if_defined_01(DXX_USE_UDP, (	\
		VERB("\n Multiplayer:\n\n")	\
		VERB("  -udp_hostaddr <s>             Use IP address/Hostname <s> for manual game joining\n\t\t\t\t(default: %s)\n", UDP_MANUAL_ADDR_DEFAULT)	\
		VERB("  -udp_hostport <n>             Use UDP port <n> for manual game joining (default: %hu)\n", UDP_PORT_DEFAULT)	\
		VERB("  -udp_myport <n>               Set my own UDP port to <n> (default: %hu)\n", UDP_PORT_DEFAULT)	\
		DXX_if_defined_01(DXX_USE_TRACKER, (	\
			VERB("  -no-tracker                   Disable tracker (unless overridden by later -tracker_hostaddr)\n")	\
			VERB("  -tracker_hostaddr <n>         Address of tracker server to register/query games to/from\n\t\t\t\t(default: %s)\n", TRACKER_ADDR_DEFAULT)	\
			VERB("  -tracker_hostport <n>         Port of tracker server to register/query games to/from\n\t\t\t\t(default: %hu)\n", TRACKER_PORT_DEFAULT)	\
		))	\
	))	\
	DXX_if_defined(EDITOR, (	\
		VERB("\n Editor:\n\n")	\
		DXX_COMMAND_LINE_HELP_D1(	\
			VERB("  -nobm                         Don't load BITMAPS.TBL and BITMAPS.BIN - use internal data\n")	\
		)	\
		DXX_COMMAND_LINE_HELP_D2(	\
			VERB("  -autoload <s>                 Autoload level <s> in the editor\n")	\
			VERB("  -macdata                      Read and write Mac data files in editor (swap colors)\n")	\
			VERB("  -hoarddata                    Make the Hoard ham file from some files, then exit\n")	\
		)	\
	))	\
	VERB("\n Debug (use only if you know what you're doing):\n\n")	\
	VERB("  -debug                        Enable debugging output.\n")	\
	VERB("  -verbose                      Enable verbose output.\n")	\
	VERB("  -safelog                      Write gamelog.txt unbuffered.\n\t\t\t\tUse to keep helpful output to trace program crashes.\n")	\
	VERB("  -norun                        Bail out after initialization\n")	\
	VERB("  -no-grab                      Never grab keyboard/mouse\n")	\
	VERB("  -renderstats                  Enable renderstats info by default\n")	\
	VERB("  -text <s>                     Specify alternate .tex file\n")	\
	VERB("  -showmeminfo                  Show memory statistics\n")	\
	VERB("  -nodoublebuffer               Disable Doublebuffering\n")	\
	VERB("  -bigpig                       Use uncompressed RLE bitmaps\n")	\
	VERB("  -16bpp                        Use 16Bpp instead of 32Bpp\n")	\
	DXX_COMMAND_LINE_HELP_OGL(	\
		VERB("  -gl_oldtexmerge               Use old texmerge, uses more ram, but might be faster\n")	\
		VERB("  -gl_intensity4_ok <n>         Override DbgGlIntensity4Ok (default: 1)\n")	\
		VERB("  -gl_luminance4_alpha4_ok <n>  Override DbgGlLuminance4Alpha4Ok (default: 1)\n")	\
		VERB("  -gl_rgba2_ok <n>              Override DbgGlRGBA2Ok (default: 1)\n")	\
		VERB("  -gl_readpixels_ok <n>         Override DbgGlReadPixelsOk (default: 1)\n")	\
		VERB("  -gl_gettexlevelparam_ok <n>   Override DbgGlGetTexLevelParamOk (default: 1)\n")	\
	)	\
	DXX_COMMAND_LINE_HELP_SDL(	\
		VERB("  -tmap <s>                     Select texmapper <s> to use\n\t\t\t\t(default: c, available: c, fp, quad)\n")	\
		VERB("  -hwsurface                    Use SDL HW Surface\n")	\
		VERB("  -asyncblit                    Use queued blits over SDL. Can speed up rendering\n")	\
	)	\
	VERB("\n Help:\n\n")	\
	VERB("  -help, -h, -?, ?             View this help screen\n")	\
	VERB("\n\n")	\

	printf(DXX_COMMAND_LINE_HELP(DXX_COMMAND_LINE_HELP_FMT) DXX_COMMAND_LINE_HELP(DXX_COMMAND_LINE_HELP_ARG));
}

int Quitting = 0;

}

namespace dcx {

// Default event handler for everything except the editor
window_event_result standard_handler(const d_event &event)
{
	int key;

	if (Quitting)
	{
		window *wind = window_get_front();
		if (!wind)
			return window_event_result::ignored;	// finished quitting
	
		if (wind == Game_wind)
		{
			int choice;
			Quitting = 0;
			choice=nm_messagebox( NULL, 2, TXT_YES, TXT_NO, TXT_ABORT_GAME );
			if (choice != 0)
				return window_event_result::handled;	// aborted quitting
			else
			{
				CGameArg.SysAutoDemo = false;
				Quitting = 1;
			}
		}
		
		// Close front window, let the code flow continue until all windows closed or quit cancelled
		if (!window_close(wind))
		{
			Quitting = 0;
			return window_event_result::handled;
		}
		
		return window_event_result::deleted;	// tell the event system we deleted some window
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
#if DXX_USE_SCREENSHOT
#ifdef macintosh
				case KEY_COMMAND + KEY_SHIFTED + KEY_3:
#endif
				case KEY_PRINT_SCREEN:
				{
					gr_set_default_canvas();
					save_screen_shot(0);
					return window_event_result::handled;
				}
#endif

				case KEY_ALTED+KEY_ENTER:
				case KEY_ALTED+KEY_PADENTER:
					if (Game_wind)
						if (Game_wind == window_get_front())
							return window_event_result::ignored;
					gr_toggle_fullscreen();
#if SDL_MAJOR_VERSION == 2
					{
						/* Hack to force the canvas to adjust to the new
						 * dimensions.  Without this, the canvas
						 * continues to use the old window size until
						 * the hack of calling `init_cockpit` from
						 * `game_handler` fixes the dimensions.  If the
						 * window became bigger, the game fails to draw
						 * in the full new area.  If the window became
						 * smaller, part of the game is outside the
						 * cropped area.
						 *
						 * If the automap is open, the view is still
						 * wrong, since the automap uses its own private
						 * canvas.  That will need to be fixed
						 * separately.  Ideally, the whole window
						 * system would be reworked to provide a general
						 * notification to every interested canvas when
						 * the top level window resizes.
						 */
						auto sm = Screen_mode;
						Screen_mode = SCREEN_GAME;
						init_cockpit();
						Screen_mode = sm;
					}
#endif
					return window_event_result::handled;

#if defined(__APPLE__) || defined(macintosh)
				case KEY_COMMAND+KEY_Q:
					// Alt-F4 already taken, too bad
					Quitting = 1;
					return window_event_result::handled;
#endif
				case KEY_SHIFTED + KEY_ESC:
					con_showup();
					return window_event_result::handled;
			}
			break;

		case EVENT_WINDOW_DRAW:
		case EVENT_IDLE:
			//see if redbook song needs to be restarted
#if DXX_USE_SDL_REDBOOK_AUDIO
			RBACheckFinishedHook();
#endif
			return window_event_result::handled;

		case EVENT_QUIT:
#if DXX_USE_EDITOR
			if (SafetyCheck())
#endif
				Quitting = 1;
			return window_event_result::handled;

		default:
			break;
	}

	return window_event_result::ignored;
}

}

namespace dsx {

#define PROGNAME argv[0]
#define DXX_RENAME_IDENTIFIER2(I,N)	I##$##N
#define DXX_RENAME_IDENTIFIER(I,N)	DXX_RENAME_IDENTIFIER2(I,N)
#define argc	DXX_RENAME_IDENTIFIER(argc_gc, DXX_git_commit)
#define argv	DXX_RENAME_IDENTIFIER(argv_gd, DXX_git_describe)

//	DESCENT by Parallax Software
//	DESCENT II by Parallax Software
//	(varies based on preprocessor options)
//		Descent Main

static int main(int argc, char *argv[])
{
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

	printf("\nType '%s -help' for a list of command-line options.\n\n", PROGNAME);

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
					DXX_HOGFILE_SHAREPATH_INDENTED
#ifdef SHAREPATH
#define DXX_HOGFILE_SHAREPATH_INDENTED	\
			      "\t" SHAREPATH "\n"
#else
#define DXX_HOGFILE_SHAREPATH_INDENTED
#endif
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
	con_puts(CON_NORMAL, "This is a MODIFIED version of Descent, based on " BASED_VERSION ".");
	con_printf(CON_NORMAL, "%s\n%s",TXT_COPYRIGHT,TXT_TRADEMARK);
	con_puts(CON_NORMAL, "Copyright (C) 2005-2013 Christian Beckhaeuser, 2013-2017 Kp");
#elif defined(DXX_BUILD_DESCENT_II)
	con_printf(CON_NORMAL, "%s%s  %s", DESCENT_VERSION, PHYSFSX_exists(MISSION_DIR "d2x.hog",1) ? "  Vertigo Enhanced" : "", g_descent_build_datetime); // D2X version
	con_puts(CON_NORMAL, "This is a MODIFIED version of Descent 2, based on " BASED_VERSION ".");
	con_printf(CON_NORMAL, "%s\n%s",TXT_COPYRIGHT,TXT_TRADEMARK);
	con_puts(CON_NORMAL, "Copyright (C) 1999 Peter Hawkins, 2002 Bradley Bell, 2005-2013 Christian Beckhaeuser, 2013-2017 Kp");
#endif

	if (CGameArg.DbgVerbose)
	{
		{
			PHYSFS_Version vc, vl;
			PHYSFS_VERSION(&vc);
			PHYSFS_getLinkedVersion(&vl);
			con_printf(CON_VERBOSE, "D" DXX_NAME_NUMBER "X-Rebirth built with PhysFS %u.%u.%u; loaded with PhysFS %u.%u.%u", vc.major, vc.minor, vc.patch, vl.major, vl.minor, vl.patch);
		}
		{
			SDL_version vc;
			SDL_VERSION(&vc);
#if SDL_MAJOR_VERSION == 1
			const auto vl = SDL_Linked_Version();
#else
			SDL_version vlv;
			const auto vl = &vlv;
			SDL_GetVersion(vl);
#endif
			con_printf(CON_VERBOSE, "D" DXX_NAME_NUMBER "X-Rebirth built with libSDL %u.%u.%u; loaded with libSDL %u.%u.%u", vc.major, vc.minor, vc.patch, vl->major, vl->minor, vl->patch);
		}
#if DXX_USE_SDLMIXER
		{
			SDL_version vc;
			SDL_MIXER_VERSION(&vc);
			const auto vl = Mix_Linked_Version();
			con_printf(CON_VERBOSE, "D" DXX_NAME_NUMBER "X-Rebirth built with SDL_mixer %u.%u.%u; loaded with SDL_mixer %u.%u.%u", vc.major, vc.minor, vc.patch, vl->major, vl->minor, vl->patch);
		}
#endif
		con_puts(CON_VERBOSE, TXT_VERBOSE_1);
	}
	
	ReadConfigFile();

	PHYSFSX_addArchiveContent();

	arch_init();

#if !DXX_USE_OGL
	select_tmap(CGameArg.DbgTexMap);

#if defined(DXX_BUILD_DESCENT_II)
	Lighting_on = 1;
#endif
#endif

	con_puts(CON_VERBOSE, "Going into graphics mode...");
	gr_set_mode_from_window_size();

	// Load the palette stuff. Returns non-zero if error.
	con_puts(CON_DEBUG, "Initializing palette system...");
#if defined(DXX_BUILD_DESCENT_I)
	gr_use_palette_table( "PALETTE.256" );
#elif defined(DXX_BUILD_DESCENT_II)
	gr_use_palette_table(D2_DEFAULT_PALETTE );
#endif

	con_puts(CON_DEBUG, "Initializing font system...");
	gamefont_init();	// must load after palette data loaded.

#if defined(DXX_BUILD_DESCENT_II)
	con_puts(CON_DEBUG, "Initializing movie libraries...");
	init_movies();		//init movie libraries
#endif

	show_titles();

	set_screen_mode(SCREEN_MENU);
#ifdef DEBUG_MEMORY_ALLOCATIONS
	/* Memdebug runs before global destructors, so it incorrectly
	 * reports as leaked any allocations that would be freed by a global
	 * destructor.  This local will force the newmenu globals to be
	 * reset before memdebug scans, which prevents memdebug falsely
	 * reporting them as leaked.
	 *
	 * External tools, such as Valgrind, know to run global destructors
	 * before checking for leaks, so this hack is only necessary when
	 * memdebug is used.
	 */
	struct hack_free_global_backgrounds
	{
		~hack_free_global_backgrounds()
		{
			newmenu_free_background();
		}
	};
	hack_free_global_backgrounds hack_free_global_background;
#endif

	con_puts(CON_DEBUG, "Doing gamedata_init...");
	gamedata_init();

#if defined(DXX_BUILD_DESCENT_II)
#if DXX_USE_EDITOR
	if (GameArg.EdiSaveHoardData) {
		save_hoard_data();
		exit(1);
	}
	#endif
#endif

	if (CGameArg.DbgNoRun)
		return(0);

	con_puts(CON_DEBUG, "Initializing texture caching system...");
	texmerge_init();		// 10 cache bitmaps

#if defined(DXX_BUILD_DESCENT_II)
	piggy_init_pigfile("groupa.pig");	//get correct pigfile
#endif

	con_puts(CON_DEBUG, "Running game...");
	init_game();

	get_local_player().callsign = {};

#if defined(DXX_BUILD_DESCENT_I)
	key_flush();
#elif defined(DXX_BUILD_DESCENT_II)
	//	If built with editor, option to auto-load a level and quit game
	//	to write certain data.
	#ifdef	EDITOR
	if (!GameArg.EdiAutoLoad.empty()) {
		Players[0u].callsign = "dummy";
	} else
	#endif
#endif
	{
		if (!CGameArg.SysPilot.empty())
		{
			char filename[sizeof(PLAYER_DIRECTORY_TEXT) + CALLSIGN_LEN + 4];

			/* Step over the literal PLAYER_DIRECTORY_TEXT when it is
			 * present.  Point at &filename[0] when
			 * PLAYER_DIRECTORY_TEXT is absent.
			 */
			const auto b = &filename[-CGameArg.SysUsePlayersDir];
			snprintf(filename, sizeof(filename), PLAYER_DIRECTORY_STRING("%.12s"), CGameArg.SysPilot.c_str());
			/* The pilot name is never used after this.  Clear it to
			 * free the allocated memory, if any.
			 */
			CGameArg.SysPilot.clear();
			auto p = b;
			for (const auto &facet = std::use_facet<std::ctype<char>>(std::locale::classic()); char &c = *p; ++p)
			{
				c = facet.tolower(static_cast<uint8_t>(c));
			}
			auto j = p - filename;
			if (j < sizeof(filename) - 4 && (j <= 4 || strcmp(&filename[j - 4], ".plr"))) // if player hasn't specified .plr extension in argument, add it
			{
				strcpy(&filename[j], ".plr");
				j += 4;
			}
			if(PHYSFSX_exists(filename,0))
			{
				get_local_player().callsign.copy(b, std::distance(b, &filename[j - 4]));
				read_player_file();
				WriteConfigFile();
			}
		}
	}

#if defined(DXX_BUILD_DESCENT_II)
#if DXX_USE_EDITOR
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

	con_puts(CON_DEBUG, "Cleanup...");
	close_game();
	texmerge_close();
	gamedata_close();
	gamefont_close();
	Current_mission.reset();
	PHYSFSX_removeArchiveContent();

	return(0);		//presumably successful exit
}

}

int main(int argc, char *argv[])
{
	mem_init();
#if defined(__linux__) || defined(__OpenBSD__)
#if DXX_WORDS_NEED_ALIGNMENT
	prctl(PR_SET_UNALIGN, PR_UNALIGN_NOPRINT, 0, 0, 0);
#endif
#else
	set_warn_func(msgbox_warning);
#endif
#ifdef WIN32
	void d_set_exception_handler();
	d_set_exception_handler();
#endif
	return dsx::main(argc, argv);
}

#undef argv
#undef argc
