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
 * inferno.c: Entry point of program (main procedure)
 *
 * After main initializes everything, most of the time is spent in the loop
 * while (window_get_front())
 * In this loop, the main menu is brought up first.
 *
 * main() for Inferno
 *
 */

char copyright[] = "DESCENT   COPYRIGHT (C) 1994,1995 PARALLAX SOFTWARE CORPORATION";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

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
#include "timer.h"
#include "3d.h"
#include "bm.h"
#include "inferno.h"
#include "error.h"
#include "game.h"
#include "segment.h"		//for Side_to_verts
#include "u_mem.h"
#include "screens.h"
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
#include "desc_id.h"
#include "config.h"
#include "multi.h"
#include "songs.h"
#include "cfile.h"
#include "gameseq.h"
#include "playsave.h"
#include "collide.h"
#include "newdemo.h"
#include "joy.h"
#include "../texmap/scanline.h" //for select_tmap -MM
#include "event.h"
#include "rbaudio.h"
#ifndef __LINUX__
#include "messagebox.h"
#endif

#ifdef EDITOR
#include "editor/editor.h"
#include "editor/kdefs.h"
#include "ui.h"
#endif

#include <SDL/SDL.h>

#include "vers_id.h"

char desc_id_exit_num = 0;
int Function_mode=FMODE_MENU;		//game or editor?
int Screen_mode=-1;					//game screen or editor screen?
int descent_critical_error = 0;
unsigned int descent_critical_deverror = 0;
unsigned int descent_critical_errcode = 0;

int HiresGFXAvailable = 0;

extern void arch_init(void);


//read help from a file & print to screen
void print_commandline_help()
{
	printf( "\n System Options:\n\n");
	printf( "  -fps               %s\n", "Enable FPS indicator by default");
	printf( "  -nonicefps         %s\n", "Don't free CPU-cycles");
	printf( "  -maxfps <n>        %s\n", "Set maximum framerate (1-200)");
	printf( "  -hogdir <s>        %s\n", "set shared data directory to <dir>");
	printf( "  -nohogdir          %s\n", "don't try to use shared data directory");
	printf( "  -use_players_dir   %s\n", "put player files and saved games in Players subdirectory");
	printf( "  -lowmem            %s\n", "Lowers animation detail for better performance with low memory");
	printf( "  -pilot <s>         %s\n", "Select this pilot automatically");
	printf( "  -autodemo          %s\n", "Start in demo mode");
	printf( "  -notitles          %s\n", "Skip title screens");
	printf( "  -window            %s\n", "Run the game in a window");
	printf( "  -noredundancy      %s\n", "Do not send messages when picking up redundant items");

	printf( "\n Controls:\n\n");
	printf( "  -nomouse           %s\n", "Deactivate mouse");
	printf( "  -nojoystick        %s\n", "Deactivate joystick");
	printf( "  -mouselook         %s\n", "Activate mouselook. Works in singleplayer only");
	printf( "  -grabmouse         %s\n", "Keeps the mouse from wandering out of the window");
	printf( "  -nostickykeys      %s\n", "Make CapsLock and NumLock non-sticky so they can be used as normal keys");

	printf( "\n Sound:\n\n");
	printf( "  -nosound           %s\n", "Disables sound output");
	printf( "  -nomusic           %s\n", "Disables music output");
#ifdef    USE_SDLMIXER
	printf( "  -nosdlmixer        %s\n", "Disable Sound output via SDL_mixer");
#endif // USE SDLMIXER

	printf( "\n Graphics:\n\n");
	printf( "  -lowresfont        %s\n", "Force to use LowRes fonts");
#ifdef    OGL
	printf( "  -gl_fixedfont      %s\n", "Do not scale fonts to current resolution");
#endif // OGL

#ifdef    NETWORK
	printf( "\n Multiplayer:\n\n");
	printf( "  -multimessages     %s\n", "Only show player-chat important Multiplayer messages");
	printf( "  -ipxnetwork <n>    %s\n", "Use IPX network number <n>");
	printf( "  -udp_hostaddr <n>  %s\n", "When manually joining a game use default IP Address <n> to connect to");
	printf( "  -udp_hostport <n>  %s\n", "When manually joining a game use default UDP Port <n> to connect to");
	printf( "  -udp_myport <n>	  %s\n", "When hosting/joining a game use default UDP Port <n> to send packets from");
#endif // NETWORK

#ifdef    EDITOR
	printf( "\n Editor:\n\n");
	printf( "  -nobm              %s\n", "Don't load BITMAPS.TBL and BITMAPS.BIN - use internal data");
#endif // EDITOR

	printf( "\n Debug (use only if you know what you're doing):\n\n");
	printf( "  -debug             %s\n", "Enable very verbose output");
	printf( "  -verbose           %s\n", "Shows initialization steps for tech support");
	printf( "  -norun             %s\n", "Bail out after initialization");
	printf( "  -renderstats       %s\n", "Enable renderstats info by default");
	printf( "  -text <s>          %s\n", "Specify alternate .tex file");
	printf( "  -tmap <s>          %s\n", "Select texmapper to use (c,fp,quad,i386,pent,ppro)");
	printf( "  -showmeminfo       %s\n", "Show memory statistics");
	printf( "  -nodoublebuffer    %s\n", "Disable Doublebuffering");
	printf( "  -bigpig            %s\n", "Use uncompressed RLE bitmaps");
	printf( "  -16bpp             %s\n", "Use 16Bpp instead of 32Bpp");
#ifdef    OGL
	printf( "  -gl_oldtexmerge    %s\n", "Use old texmerge, uses more ram, but _might_ be a bit faster");
	printf( "  -gl_intensity4_ok <n> %s\n", "Override DbgGlIntensity4Ok - Default: 1");
	printf( "  -gl_luminance4_alpha4_ok <n> %s\n", "Override DbgGlLuminance4Alpha4Ok - Default: 1");
	printf( "  -gl_rgba2_ok <n>   %s\n", "Override DbgGlRGBA2Ok - Default: 1");
	printf( "  -gl_readpixels_ok <n> %s\n", "Override DbgGlReadPixelsOk - Default: 1");
	printf( "  -gl_gettexlevelparam_ok <n> %s\n", "Override DbgGlGetTexLevelParamOk - Default: 1");
#else
	printf( "  -hwsurface         %s\n", "Use SDL HW Surface");
	printf( "  -asyncblit         %s\n", "Use queued blits over SDL. Can speed up rendering");
#endif // OGL

	printf( "\n Help:\n\n");
	printf( "  -help, -h, -?, ?   %s\n", "View this help screen");
	printf( "\n\n");
}

#define key_ismod(k)  ((k&0xff)==KEY_LALT || (k&0xff)==KEY_RALT || (k&0xff)==KEY_LSHIFT || (k&0xff)==KEY_RSHIFT || (k&0xff)==KEY_LCTRL || (k&0xff)==KEY_RCTRL || (k&0xff)==KEY_LMETA || (k&0xff)==KEY_RMETA)

int Quitting = 0;

// Default event handler for everything except the editor
int standard_handler(d_event *event)
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
				Quitting = 1;
		}
		
		// Close front window, let the code flow continue until all windows closed or quit cancelled
		window_close(wind);
		
		return 1;
	}

	switch (event->type)
	{
		case EVENT_MOUSE_BUTTON_DOWN:
		case EVENT_MOUSE_BUTTON_UP:
			// No window selecting
			// We stay with the current one until it's closed/hidden or another one is made
			// Not the case for the editor
			break;

		case EVENT_KEY_COMMAND:
			key = ((d_event_keycommand *)event)->keycode;

			// Don't let modifier(s) on their own do something unless we explicitly want that
			// (e.g. if it's a game control like fire primary)
			if (key_ismod(key))
				return 1;

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
					gr_toggle_fullscreen();
					return 1;

#ifndef NDEBUG
				case KEY_BACKSP:
					Int3();
					return 1;
#endif

#if defined(__APPLE__) || defined(macintosh)
				case KEY_COMMAND+KEY_Q:
					// Alt-F4 already taken, too bad
					Quitting = 1;
					return 1;
#endif
			}
			break;

		case EVENT_IDLE:
			//see if redbook song needs to be restarted
			RBACheckFinishedHook();
			return 1;

		case EVENT_QUIT:
			Quitting = 1;
			return 1;

		default:
			break;
	}

	return 0;
}

int MacHog = 0;	// using a Mac hogfile?
jmp_buf LeaveEvents;
#define PROGNAME argv[0]

//	DESCENT by Parallax Software
//		Descent Main

int main(int argc, char *argv[])
{
	mem_init();
#ifdef __LINUX__
	error_init(NULL, NULL);
#else
	error_init(msgbox_error, NULL);
	set_warn_func(msgbox_warning);
#endif
	PHYSFSX_init(argc, argv);
	con_init();  // Initialise the console

	setbuf(stdout, NULL); // unbuffered output via printf
#ifdef _WIN32
	freopen( "CON", "w", stdout );
	freopen( "CON", "w", stderr );
#endif

	if (GameArg.SysShowCmdHelp) {
		print_commandline_help();
		set_exit_message("");

		return(0);
	}

	printf("\nType %s -help' for a list of command-line options.\n\n", PROGNAME);

	PHYSFSX_listSearchPathContent();
	
	if (!PHYSFSX_checkSupportedArchiveTypes())
		return(0);

	if (! cfile_init("descent.hog", 1))
		Error("Could not find a valid hog file (descent.hog)\nPossible locations are:\n"
#if defined(__unix__) && !defined(__APPLE__)
			  "\t$HOME/.d1x-rebirth\n"
			  "\t" SHAREPATH "\n"
#else
			  "\tDirectory containing D1X\n"
#endif
			  "\tIn a subdirectory called 'Data'\n"
#if (defined(__APPLE__) && defined(__MACH__)) || defined(macintosh)
			  "\tIn 'Resources' inside the application bundle\n"
#endif
			  "Or use the -hogdir option to specify an alternate location.");
	
	switch (cfile_size("descent.hog"))
	{
		case D1_MAC_SHARE_MISSION_HOGSIZE:
		case D1_MAC_MISSION_HOGSIZE:
			MacHog = 1;	// used for fonts and the Automap
			break;
	}

	load_text();

	//print out the banner title
	con_printf(CON_NORMAL,DESCENT_VERSION "\n"
			   "This is a MODIFIED version of DESCENT which is NOT supported by Parallax or\n"
			   "Interplay. Use at your own risk! Copyright (c) 2005 Christian Beckhaeuser\n");
	con_printf(CON_NORMAL,"Based on: DESCENT   %s\n", VERSION_NAME);
	con_printf(CON_NORMAL,"%s\n%s\n\n",TXT_COPYRIGHT,TXT_TRADEMARK);

	if (GameArg.DbgVerbose)
		con_printf(CON_VERBOSE,"%s%s", TXT_VERBOSE_1, "\n");
	
	ReadConfigFile();

	PHYSFSX_addArchiveContent();

	arch_init();

	select_tmap(GameArg.DbgTexMap);

#ifdef NETWORK
	control_invul_time = 0;
#endif

	con_printf(CON_VERBOSE, "Going into graphics mode...\n");
	gr_set_mode(Game_screen_mode);

	// Load the palette stuff. Returns non-zero if error.
	con_printf(CON_DEBUG, "Initializing palette system...\n" );
	gr_use_palette_table( "PALETTE.256" );

	con_printf(CON_DEBUG, "Initializing font system...\n" );
	gamefont_init();	// must load after palette data loaded.

	set_default_handler(standard_handler);

	songs_play_song( SONG_TITLE, 1 );
	show_titles();

	set_screen_mode(SCREEN_MENU);

	con_printf( CON_DEBUG, "\nDoing gamedata_init..." );
	gamedata_init();

	if (GameArg.DbgNoRun)
		return(0);

	con_printf( CON_DEBUG, "\nInitializing texture caching system..." );
	texmerge_init( 10 );		// 10 cache bitmaps

	con_printf( CON_DEBUG, "\nRunning game...\n" );
	init_game();

	Players[Player_num].callsign[0] = '\0';

	key_flush();

	if(GameArg.SysPilot)
	{
		char filename[32] = "";
		int j;

		if (GameArg.SysUsePlayersDir)
			strcpy(filename, "Players/");
		strncat(filename, GameArg.SysPilot, 12);
		filename[8 + 12] = '\0';	// unfortunately strncat doesn't put the terminating 0 on the end if it reaches 'n'
		for (j = GameArg.SysUsePlayersDir? 8 : 0; filename[j] != '\0'; j++) {
			switch (filename[j]) {
				case ' ':
					filename[j] = '\0';
			}
		}
		if(!strstr(filename,".plr")) // if player hasn't specified .plr extension in argument, add it
			strcat(filename,".plr");
		if(cfexist(filename))
		{
			strcpy(strstr(filename,".plr"),"\0");
			strcpy(Players[Player_num].callsign, GameArg.SysUsePlayersDir? &filename[8] : filename);
			read_player_file();
			WriteConfigFile();
		}
	}


	Game_mode = GM_GAME_OVER;
	DoMenu();

	setjmp(LeaveEvents);
	while (window_get_front())
		// Send events to windows and the default handler
		event_process();

	WriteConfigFile();
	show_order_form();

	con_printf( CON_DEBUG, "\nCleanup...\n" );
	close_game();
	texmerge_close();
	gamedata_close();
	gamefont_close();
	free_text();
	args_exit();
	newmenu_free_background();
	free_mission();
	PHYSFSX_removeArchiveContent();

	return(0);		//presumably successful exit
}
