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
 * main() for Inferno  
 *
 */

#ifdef __GNUC__
static char copyright[] = "DESCENT   COPYRIGHT (C) 1994,1995 PARALLAX SOFTWARE CORPORATION";
static char *__reference[2]={copyright,(char *)__reference};
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <SDL/SDL.h>

#if !defined(_MSC_VER) && !defined(macintosh)
#include <unistd.h>
#endif

#ifdef __MSDOS__
#include <time.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

#if !(defined(__APPLE__) && defined(__MACH__))
#include <physfs.h>
#else
#include <physfs/physfs.h>
#endif

#ifdef __MSDOS__
#include <conio.h>
#else
#define getch() getchar()
#endif

#ifdef EDITOR
#include "editor/editor.h"
#include "editor/kdefs.h"
#endif

#ifdef QUICKSTART
#include "playsave.h"
#endif

#ifdef SCRIPT
#include "script.h"
#endif

#ifdef OGL
#include "ogl_init.h"
#endif

#include "gr.h"
#include "3d.h"
#include "inferno.h"
#include "error.h"
#include "game.h"
#include "segment.h" //for Side_to_verts
#include "u_mem.h"
#include "texmerge.h"
#include "menu.h"
#include "digi.h"
#include "args.h"
#include "titles.h"
#include "text.h"
#include "network.h"
#include "gamefont.h"
#include "kconfig.h"
#include "newmenu.h"
#include "desc_id.h"
#include "config.h"
#include "multi.h"
#include "songs.h"
#include "cfile.h"
#include "gameseq.h"
#include "timer.h"
#include "key.h"
#include "palette.h"
#include "bm.h"
#include "screens.h"
#include "hudmsg.h"
#include "playsave.h"
#include "gauges.h"
#include "physics.h"
#include "strutil.h"
#include "altsound.h"
#include "../texmap/scanline.h" //for select_tmap -MM
#include "vers_id.h"
#include "collide.h"
#include "newdemo.h"
#include "joy.h"
#include "console.h"

char desc_id_exit_num = 0;
int Function_mode=FMODE_MENU; //game or editor?
int Screen_mode=-1; //game screen or editor screen?
int descent_critical_error = 0;
unsigned int descent_critical_deverror = 0;
unsigned int descent_critical_errcode = 0;

int HiresGFXAvailable = 0;

void mem_init(void);
extern void arch_init(void);

#ifdef EDITOR
int Inferno_is_800x600_available = 0;
#endif

void show_commandline_help()
{
	printf( "\n System Options:\n\n");
	printf( "  -fps               %s\n", "Enable FPS indicator by default");
	printf( "  -nonicefps         %s\n", "Don't free CPU-cycles");
	printf( "  -maxfps <n>        %s\n", "Set maximum framerate (1-200)");
	printf( "  -hogdir <s>        %s\n", "Set shared data directory to <dir>");
	printf( "  -nohogdir          %s\n", "Don't try to use shared data directory");
	printf( "  -use_players_dir   %s\n", "Put player files and saved games in Players subdirectory");
	printf( "  -lowmem            %s\n", "Lowers animation detail for better performance with low memory");
	printf( "  -pilot <s>         %s\n", "Select this pilot-file automatically");
	printf( "  -autodemo          %s\n", "Start in demo mode");
	printf( "  -notitles          %s\n", "Skip title screens");
	printf( "  -window            %s\n", "Run the game in a window");
	printf( "  -noredundancy      %s\n", "Do not send messages when picking up redundant items");

	printf( "\n Controls:\n\n");
	printf( "  -nomouse           %s\n", "Deactivate mouse");
	printf( "  -nojoystick        %s\n", "Deactivate joystick");
	printf( "  -mouselook         %s\n", "Activate mouselook. Works in singleplayer only");
	printf( "  -grabmouse         %s\n", "Keeps the mouse from wandering out of the window");

	printf( "\n Sound:\n\n");
	printf( "  -nosound           %s\n", "Disables sound output");
	printf( "  -nomusic           %s\n", "Disables music output");
#ifdef    USE_SDLMIXER
	printf( "  -nosdlmixer        %s\n", "Disable Sound output via SDL_mixer");
	printf( "  -music_ext <s>     %s\n", "Play music files with extension <s> (i.e. mp3, ogg)");
#endif // USE SDLMIXER

	printf( "\n Graphics:\n\n");
	printf( "  -lowresfont        %s\n", "Force to use LowRes fonts");
#ifdef    OGL
	printf( "  -gl_fixedfont      %s\n", "Do not scale fonts to current resolution");
#endif // OGL

#ifdef    NETWORK
	printf( "\n Multiplayer:\n\n");
	printf( "  -mprofile          %s\n", "Enable multiplayer game profiles");
	printf( "  -playermessages    %s\n", "View only messages from other players in multi - overrides -noredundancy");
	printf( "  -ipxnetwork <n>    %s\n", "Use IPX network number <n>");
        printf( "  -ip_baseport <n>   %s\n", "Use <n> as offset from normal port (allows multiple instances of d1x to be run on a single computer)");
	printf( "  -ip_relay          %s\n", "Relay players with closed port over host (increases traffic and lag)");
	printf( "  -ip_hostaddr <n>   %s\n", "Use <n> as host ip address");
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
#ifdef    OGL
	printf( "  -gl_oldtexmerge    %s\n", "Use old texmerge, uses more ram, but _might_ be a bit faster");
	printf( "  -gl_16bpp          %s\n", "Use 16Bpp Color Depth");
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

void error_messagebox(char *s)
{
	error_init(NULL, NULL);	// don't try to show a messagebox if there's an error with showing one!
	nm_messagebox( TXT_SORRY, 1, TXT_OK, s );
}

int MacHog = 0;	// using a Mac hogfile?
#define PROGNAME argv[0]

int main(int argc,char *argv[])
{
	mem_init();

	error_init(NULL, NULL);
	PHYSFSX_init(argc, argv);
	con_init();  // Initialise the console

	setbuf(stdout, NULL); // unbuffered output via printf

	ReadConfigFile();

	if (! cfile_init("descent.hog", 1))
		Error("Could not find a valid hog file (descent.hog)\nPossible locations are:\n"
#if defined(__unix__) && !defined(__APPLE__)
			  "\t$HOME/.d1x-rebirth\n"
			  "\t" SHAREPATH "\n"
#else
			  "\tDirectory containing D1X\n"
#endif
			  "\tIn a subdirectory called 'Data'\n"
			  "Or use the -hogdir option to specify an alternate location.");

	switch (cfile_size("descent.hog"))
	{
		case D1_MAC_SHARE_MISSION_HOGSIZE:
		case D1_MAC_MISSION_HOGSIZE:
			MacHog = 1;	// used for fonts and the Automap
			break;
	}
	
	load_text();

	con_printf(CON_NORMAL,DESCENT_VERSION "\n"
	       "This is a MODIFIED version of DESCENT which is NOT supported by Parallax or\n"
	       "Interplay. Use at your own risk! Copyright (c) 2005 Christian Beckhaeuser\n");
	con_printf(CON_NORMAL,"Based on: DESCENT   %s\n", VERSION_NAME);
	con_printf(CON_NORMAL,"%s\n%s\n",TXT_COPYRIGHT,TXT_TRADEMARK);

	if (GameArg.SysShowCmdHelp) {
		show_commandline_help();
                set_exit_message("");
		return 0;
	}

	printf("\nType %s -help' for a list of command-line options.\n", PROGNAME);

	select_tmap(GameArg.DbgTexMap);

	if (GameArg.DbgVerbose)
		con_printf(CON_VERBOSE,"%s", TXT_VERBOSE_1);

	{
		char **i, **list;

		list = PHYSFS_getSearchPath();
		for (i = list; *i != NULL; i++)
			con_printf(CON_VERBOSE, "PHYSFS: [%s] is in the search path.\n", *i);
		PHYSFS_freeList(list);

		list = PHYSFS_enumerateFiles("");
		for (i = list; *i != NULL; i++)
			con_printf(CON_DEBUG, "PHYSFS: * We've got [%s].\n", *i);
		PHYSFS_freeList(list);
	}

	if (cfile_init("d1xrdata.zip", 0))
		con_printf(CON_NORMAL, "Added d1xrdata.zip for additional content\n");

	arch_init();

#ifdef _WIN32
	freopen( "CON", "w", stdout );
	freopen( "CON", "w", stderr );
#endif

#ifdef NETWORK
	control_invul_time = 0;
#endif

	// Load the palette stuff. Returns non-zero if error.
	con_printf( CON_DEBUG, "Going into graphics mode..." );
	gr_set_mode(Game_screen_mode);

	con_printf( CON_DEBUG, "\nInitializing palette system..." );
	gr_use_palette_table( "PALETTE.256" );
	con_printf( CON_DEBUG, "\nInitializing font system..." );
	gamefont_init(); // must load after palette data loaded.
	songs_play_song( SONG_TITLE, 1 );

	gamedata_init();

	if (GameArg.DbgNoRun)
		return(0);

	error_init(error_messagebox, NULL);

	con_printf( CON_DEBUG, "\nInitializing texture caching system..." );
	texmerge_init( 10 );		// 10 cache bitmaps

	con_printf( CON_DEBUG, "\nRunning game...\n" );
	set_screen_mode(SCREEN_MENU);

	init_game();

	Players[Player_num].callsign[0] = '\0';

	if (!GameArg.SysNoTitles)
	{
		char    publisher[16];

		strcpy(publisher, "macplay.pcx");	// Mac Shareware
		if (!PHYSFS_exists(publisher))
			strcpy(publisher, "mplaycd.pcx");	// Mac Registered
		if (!PHYSFS_exists(publisher))
			strcpy(publisher, "iplogo1.pcx");	// PC. Only down here because it's lowres ;-)
		
		show_title_screen( publisher, 1 );
		show_title_screen( (((SWIDTH>=640&&SHEIGHT>=480) && cfexist("logoh.pcx"))?"logoh.pcx":"logo.pcx"), 1 );
		show_title_screen( (((SWIDTH>=640&&SHEIGHT>=480) && cfexist("descenth.pcx"))?"descenth.pcx":"descent.pcx"), 1 );
	}

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
		else //pilot doesn't exist. get pilot.
			RegisterPlayer();
	}
	else
		RegisterPlayer();


	Game_mode = GM_GAME_OVER;

	while (Function_mode != FMODE_EXIT)
	{
		switch( Function_mode ) {
			case FMODE_MENU:
				DoMenu();
#ifdef EDITOR
				if ( Function_mode == FMODE_EDITOR ) {
					create_new_mine();
					SetPlayerFromCurseg();
				}
#endif
			break;
			case FMODE_GAME:
#ifdef EDITOR
				keyd_editor_mode = 0;
#endif
	
				game();
	
				if ( Function_mode == FMODE_MENU )
					songs_play_song( SONG_TITLE, 1 );
				break;
#ifdef EDITOR
			case FMODE_EDITOR:
				keyd_editor_mode = 1;
				editor();
				if ( Function_mode == FMODE_GAME ) {
					Game_mode = GM_EDITOR;
					editor_reset_stuff_on_level();
					N_players = 1;
				}
				break;
#endif
			default:
				Error("Invalid function mode %d",Function_mode);
		}
	}

	WriteConfigFile();
#ifdef SHAREWARE
	show_order_form();
#endif

	con_printf( CON_DEBUG, "\nCleanup...\n" );
	error_init(NULL, NULL);		// clear error func (won't have newmenu stuff loaded)
	close_game();
	texmerge_close();
	gamedata_close();
	gamefont_close();
	free_text();
	args_exit();
	newmenu_close();
	free_mission();

	return(0); //presumably successful exit
}

void quit_request()
{
	set_warn_func(warn_printf);
	exit(0);
}

void macintosh_quit()
{
	Function_mode = FMODE_EXIT;
}
