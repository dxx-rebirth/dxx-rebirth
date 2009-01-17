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
 * inferno.c: Entry point of program (main procedure)
 *
 * After main initializes everything, most of the time is spent in the loop
 * while (Function_mode != FMODE_EXIT)
 * In this loop, the main menu is brought up first.
 *
 * main() for Inferno
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

char copyright[] = "DESCENT II  COPYRIGHT (C) 1994-1996 PARALLAX SOFTWARE CORPORATION";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#ifdef __unix__
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#if !(defined(__APPLE__) && defined(__MACH__))
#include <physfs.h>
#else
#include <physfs/physfs.h>
#endif

#include "pstypes.h"
#include "strutil.h"
#include "console.h"
#include "gr.h"
#include "fix.h"
#include "vecmat.h"
#include "key.h"
#include "timer.h"
#include "3d.h"
#include "bm.h"
#include "inferno.h"
#include "error.h"
#include "game.h"
#include "segment.h"		//for Side_to_verts
#include "u_mem.h"
#include "segpoint.h"
#include "screens.h"
#include "texmap.h"
#include "texmerge.h"
#include "menu.h"
#include "wall.h"
#include "polyobj.h"
#include "effects.h"
#include "digi.h"
#include "palette.h"
#include "args.h"
#include "sounds.h"
#include "titles.h"
#include "player.h"
#include "text.h"
#include "gauges.h"
#ifdef NETWORK
#include "network.h"
#endif
#include "gamefont.h"
#include "kconfig.h"
#include "mouse.h"
#include "newmenu.h"
#include "desc_id.h"
#include "config.h"
#include "multi.h"
#include "songs.h"
#include "cfile.h"
#include "gameseq.h"
#include "gamepal.h"
#include "mission.h"
#include "movie.h"
#include "playsave.h"
#include "collide.h"
#include "newdemo.h"
#include "joy.h"
#include "../texmap/scanline.h" //for select_tmap -MM

#ifdef EDITOR
#include "editor/editor.h"
#include "editor/kdefs.h"
#include "ui.h"
#endif

#include <SDL/SDL.h>

#include "vers_id.h"

void mem_init(void);

//Current version number

ubyte Version_major = 1;		//FULL VERSION
ubyte Version_minor = 2;

//static const char desc_id_checksum_str[] = DESC_ID_CHKSUM_TAG "0000"; // 4-byte checksum
char desc_id_exit_num = 0;

int Function_mode=FMODE_MENU;		//game or editor?
int Screen_mode=-1;					//game screen or editor screen?

void show_order_form(void);

//--------------------------------------------------------------------------

int descent_critical_error = 0;
unsigned descent_critical_deverror = 0;
unsigned descent_critical_errcode = 0;

extern int Network_allow_socket_changes;
extern void piggy_init_pigfile(char *filename);
extern void arch_init(void);

#define LINE_LEN	100

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
	printf( "  -window            %s\n", "Run the game in a window");
	printf( "  -nomovies          %s\n", "Don't play movies");
	printf( "  -noredundancy      %s\n", "Do not send messages when picking up redundant items");

	printf( "\n Controls:\n\n");
	printf( "  -nomouse           %s\n", "Deactivate mouse");
	printf( "  -nojoystick        %s\n", "Deactivate joystick");
	printf( "  -mouselook         %s\n", "Activate mouselook. Works in singleplayer only");
	printf( "  -grabmouse         %s\n", "Keeps the mouse from wandering out of the window");

	printf( "\n Sound:\n\n");
	printf( "  -nosound           %s\n", "Disables sound output");
	printf( "  -nomusic           %s\n", "Disables music output");
	printf( "  -sound11k          %s\n", "Use 11KHz sounds");
#ifdef    USE_SDLMIXER
	printf( "  -nosdlmixer        %s\n", "Disable Sound output via SDL_mixer");
	printf( "  -music_ext <s>     %s\n", "Play music files with extension <s> (i.e. mp3, ogg)");
#endif // USE SDLMIXER

	printf( "\n Graphics:\n\n");
	printf( "  -lowresfont        %s\n", "Force to use LowRes fonts");
	printf( "  -lowresgraphics    %s\n", "Force to use LowRes graphics");
	printf( "  -lowresmovies      %s\n", "Play low resolution movies if available (for slow machines)");
	printf( "  -subtitles         %s\n", "Turn on movie subtitles");
#ifdef    OGL
	printf( "  -gl_fixedfont      %s\n", "Do not scale fonts to current resolution");
#endif // OGL

#ifdef    NETWORK
	printf( "\n Multiplayer:\n\n");
	printf( "  -mprofile          %s\n", "Enable multiplayer game profiles");
	printf( "  -norankings        %s\n", "Disable multiplayer ranking system");
	printf( "  -playermessages    %s\n", "View only messages from other players in multi - overrides -noredundancy");
	printf( "  -ipxnetwork <n>    %s\n", "Use IPX network number <n>");
        printf( "  -ip_baseport <n>   %s\n", "Use <n> as offset from normal port (allows multiple instances of d1x to be run on a single computer)");
	printf( "  -ip_relay          %s\n", "Relay players with closed port over host (increases traffic and lag)");
	printf( "  -ip_hostaddr <n>   %s\n", "Use <n> as host ip address");
#endif // NETWORK

#ifdef    EDITOR
	printf( "\n Editor:\n\n");
	printf( "  -autoload <s>      %s\n", "Autoload a level in the editor");
	printf( "  -macdata           %s\n", "Read and write mac data files in editor (swap colors)");
	printf( "  -hoarddata         %s\n", "Make the hoard ham file from some files, then exit");
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
#endif

	printf( "\n Help:\n\n");
	printf( "  -help, -h, -?, ?   %s\n", "View this help screen");
	printf( "\n\n");
}

void error_messagebox(char *s)
{
	nm_messagebox( TXT_SORRY, 1, TXT_OK, s );
}

#define PROGNAME argv[0]

extern char Language[];

int Inferno_verbose = 0;

//	DESCENT II by Parallax Software
//		Descent Main

//extern ubyte gr_current_pal[];

#ifdef	EDITOR
char	Auto_file[128] = "";
#endif

int main(int argc, char *argv[])
{
	mem_init();
	error_init(NULL, NULL);
	PHYSFSX_init(argc, argv);
	con_init();  // Initialise the console

	con_printf (CON_VERBOSE, "%s", TXT_VERBOSE_1);
	ReadConfigFile();

	if (! cfile_init("descent2.hog", 1)) {
		if (! cfile_init("d2demo.hog", 1))
		{
			Error("Could not find a valid hog file (descent2.hog or d2demo.hog)\nPossible locations are:\n"
#if defined(__unix__) && !defined(__APPLE__)
			      "\t$HOME/.d2x-rebirth\n"
			      "\t" SHAREPATH "\n"
#else
				  "\tDirectory containing D2X\n"
#endif
				  "\tIn a subdirectory called 'Data'\n"
				  "Or use the -hogdir option to specify an alternate location.");
		}
	}

	load_text();

	//print out the banner title
	con_printf(CON_NORMAL, "\nDESCENT 2 %s v%d.%d",VERSION_TYPE,Version_major,Version_minor);
	#if 1	//def VERSION_NAME
	con_printf(CON_NORMAL, "  %s", DESCENT_VERSION);	// D2X version
	#endif
	if (cfexist(MISSION_DIR "d2x.hog")) {
		con_printf(CON_NORMAL, "  Vertigo Enhanced");
	}

	con_printf(CON_NORMAL, "  %s %s\n", __DATE__,__TIME__);
	con_printf(CON_NORMAL, "%s\n%s\n",TXT_COPYRIGHT,TXT_TRADEMARK);
	con_printf(CON_NORMAL, "This is a MODIFIED version of Descent 2. Copyright (c) 1999 Peter Hawkins\n");
	con_printf(CON_NORMAL, "                                         Copyright (c) 2002 Bradley Bell\n");
	con_printf(CON_NORMAL, "                                         Copyright (c) 2005 Christian Beckhaeuser\n");


	if (GameArg.SysShowCmdHelp) {
		print_commandline_help();
		set_exit_message("");

		return(0);
	}

	printf("\n");
	printf(TXT_HELP, PROGNAME);		//help message has %s for program name
	printf("\n");

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

	arch_init();

#ifdef _WIN32
	freopen( "CON", "w", stdout );
	freopen( "CON", "w", stderr );
#endif

	select_tmap(GameArg.DbgTexMap);

	Lighting_on = 1;

	con_printf(CON_VERBOSE, "Going into graphics mode...\n");
	gr_set_mode(Game_screen_mode);

	// Load the palette stuff. Returns non-zero if error.
	con_printf(CON_DEBUG, "Initializing palette system...\n" );
	gr_use_palette_table(D2_DEFAULT_PALETTE );

	con_printf(CON_DEBUG, "Initializing font system...\n" );
	gamefont_init();	// must load after palette data loaded.

	con_printf( CON_DEBUG, "Initializing movie libraries...\n" );
	init_movies();		//init movie libraries

	show_titles();

	set_screen_mode(SCREEN_MENU);

	con_printf( CON_DEBUG, "\nDoing gamedata_init..." );
	gamedata_init();

	#ifdef EDITOR
	if (GameArg.EdiSaveHoardData) {
		save_hoard_data();
		exit(1);
	}
	#endif

	if (GameArg.DbgNoRun)
		return(0);

	error_init(error_messagebox, NULL);
	
	con_printf( CON_DEBUG, "\nInitializing texture caching system..." );
	texmerge_init( 10 );		// 10 cache bitmaps

	piggy_init_pigfile("groupa.pig");	//get correct pigfile 

	con_printf( CON_DEBUG, "\nRunning game...\n" );
	init_game();

	//	If built with editor, option to auto-load a level and quit game
	//	to write certain data.
	#ifdef	EDITOR
	if (GameArg.EdiAutoLoad) {
		strcpy(Auto_file, GameArg.EdiAutoLoad);
		strcpy(Players[0].callsign, "dummy");
	} else
	#endif
	{
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
	}

	Game_mode = GM_GAME_OVER;

	while (Function_mode != FMODE_EXIT)
	{
		switch( Function_mode )	{
			case FMODE_MENU:
				set_screen_mode(SCREEN_MENU);
#ifdef EDITOR
				if (GameArg.EdiAutoLoad) {
					strcpy((char *)&Level_names[0], Auto_file);
					LoadLevel(1, 1);
					Function_mode = FMODE_EXIT;
					break;
				}
#endif
	
				DoMenu();
#ifdef EDITOR
				if ( Function_mode == FMODE_EDITOR )	{
					create_new_mine();
					SetPlayerFromCurseg();
					load_palette(NULL,1,0);
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
	show_order_form();

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

	return(0);		//presumably successful exit
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
