/* $Id: inferno.c,v 1.1.1.1 2006/03/17 19:57:28 zicodxx Exp $ */
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
#include "mono.h"
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
#include "gauges.h" // ZICO - new HUD modes
#ifdef NETWORK
#include "network.h"
#include "modem.h"
#endif
#include "gamefont.h"
#include "kconfig.h"
#include "mouse.h"
#include "newmenu.h"
#include "desc_id.h"
#include "config.h"
#include "joydefs.h"
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

// #  include "3dfx_des.h"

//added on 9/30/98 by Matt Mueller for selectable automap modes
#include "automap.h"
//end addition -MM

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

//--unused-- grs_bitmap Inferno_bitmap_title;

int WVIDEO_running=0;		//debugger can set to 1 if running

//--unused-- int Cyberman_installed=0;			// SWIFT device present
ubyte CybermouseActive=0;

#ifdef __WATCOMC__
int __far descent_critical_error_handler( unsigned deverr, unsigned errcode, unsigned __far * devhdr );
#endif

void show_order_form(void);

//--------------------------------------------------------------------------

int descent_critical_error = 0;
unsigned descent_critical_deverror = 0;
unsigned descent_critical_errcode = 0;

extern int Network_allow_socket_changes;

extern void vfx_set_palette_sub(ubyte *);

extern int VR_low_res;

extern int Config_vr_type;
extern int Config_vr_resolution;
extern int Config_vr_tracking;
extern int vertigo_present;
#ifndef NDEBUG
extern int checktime;
#endif
extern void d_mouse_init(void);

#define LINE_LEN	100

//read help from a file & print to screen
void print_commandline_help()
{
	printf( "\n System Options:\n\n");
	printf( "  -fps               %s\n", "Enable FPS indicator by default");
	printf( "  -nonicefps         %s\n", "Disable CPU cycle freeing. Higher CPU load, but game may be smoother");
	printf( "  -maxfps <n>        %s\n", "Set maximum framerate (1-80)");
	printf( "  -hogdir <dir>      %s\n", "set shared data directory to <dir>");
	printf( "  -nohogdir          %s\n", "don't try to use shared data directory");
	printf( "  -userdir <dir>     %s\n", "set user dir to <dir> instead of $HOME/.d2x-rebirth");
	printf( "  -use_players_dir   %s\n", "put player files and saved games in Players subdirectory");
	printf( "  -lowmem            %s\n", "Lowers animation detail for better performance with low memory");
	printf( "  -legacyhomers      %s\n", "Activate original homing missiles (FPS and physics dependent)");
	printf( "  -pilot <name>      %s\n", "Select this pilot automatically");
	printf( "  -autodemo          %s\n", "Start in demo mode");

	printf( "\n Controls:\n\n");
	printf( "  -nomouse           %s\n", "Deactivate mouse");
	printf( "  -nojoystick        %s\n", "Deactivate joystick");
	printf( "  -mouselook         %s\n", "Activate mouselook. Works in singleplayer only");
	printf( "  -grabmouse         %s\n", "Keeps the mouse from wandering out of the window");

	printf( "\n Sound:\n\n");
	printf( "  -nosound           %s\n", "Disables sound output");
	printf( "  -nomusic           %s\n", "Disables music output");
	printf( "  -sound11k          %s\n", "Use 11KHz sounds");
#if       !defined(SHAREWARE) || ( defined(SHAREWARE) && defined(APPLE_DEMO) )
	printf( "  -redbook           %s\n", "Enable redbook audio support");
#endif //  !defined(SHAREWARE) || ( defined(SHAREWARE) && defined(APPLE_DEMO) )

	printf( "\n Graphics:\n\n");
	printf( "  -menu<X>x<Y>       %s\n", "Set menu-resolution to <X> by <Y> instead of game-resolution");
	printf( "  -aspect<Y>x<X>     %s\n", "use specified aspect");
	printf( "  -hud <h>           %s\n", "Set hud mode.  0=normal 1-3=new");
	printf( "  -persistentdebris  %s\n", "Enable persistent debris");
#ifdef    GR_SUPPORTS_FULLSCREEN_TOGGLE
	printf( "  -window            %s\n", "Run the game in a window");
#endif // GR_SUPPORTS_FULLSCREEN_TOGGLE
	printf( "  -lowresmovies      %s\n","Play low resolution movies if available (for slow machines)");
	printf( "  -subtitles         %s\n","Turn on movie subtitles (English-only)");
	printf( "  -rearviewtime t    %s\n", "Time holding rearview key to use toggle mode (default 0.0625 seconds)");

#ifdef    OGL
	printf( "\n OpenGL:\n\n");
	printf( "  -gl_simple         %s\n", "Set gl texture filters to gl_nearest for \"original\" look. (default)");
	printf( "  -gl_mipmap         %s\n", "Set gl texture filters to \"standard\" options for mipmapping");
	printf( "  -gl_trilinear      %s\n", "Set gl texture filters to trilinear mipmapping");
	printf( "  -gl_transparency   %s\n", "Enable transparency effects");
	printf( "  -gl_reticle <r>    %s\n", "Use OGL reticle 0=never 1=above 320x* 2=always");
	printf( "  -gl_scissor_ok <r> %s\n", "Set glScissor. 0=off 1=on (default)");
	printf( "  -gl_voodoo         %s\n", "Force fullscreen mode only");
	printf( "  -fixedfont         %s\n", "Do not scale fonts to current resolution");
#endif // OGL

#ifdef    NETWORK
	printf( "\n Multiplayer:\n\n");
	printf( "  -norankings        %s\n", "Disable multiplayer ranking system");
	printf( "  -noredundancy      %s\n", "Do not send messages when picking up redundant items in multi");
	printf( "  -packets <num>     %s\n", "Specifies the number of packets per second\n");
	printf( "  -ipxnetwork <num>  %s\n", "Use IPX network number <num>");
	printf( "  -kali              %s\n", "Use Kali for networking");
        printf( "  -ip_nogetmyaddr    %s\n", "Prevent autodetection of local ip address");
        printf( "  -ip_myaddr <a>     %s\n", "Use <a> as local ip address");
        printf( "  -ip_bind_addr <a>  %s\n", "Bind to <a> instead of INADDR_ANY");
        printf( "  -ip_baseport <p>   %s\n", "Use <p> as offset from normal port (allows multiple instances of d1x to be run on a single computer)");
#endif // NETWORK

#ifndef   NDEBUG
	printf( "\n Debug:\n\n");
	printf( "  -debug             %s\n","Enable very verbose output");
	printf( "  -Verbose           %s\n", "Shows initialization steps for tech support");
	printf( "  -renderstats       %s\n", "Enable renderstats info by default");
	printf( "  -norun             %s\n","Bail out after initialization");
	printf( "  -nofade            %s\n","Disable fades");
	printf( "  -norun             %s\n","Bail out after initialization");
	printf( "  -text <file>       %s\n","Specify alternate .tex file");
#ifndef   RELEASE
	printf( "  -nomovies          %s\n","Don't play movies");
#endif // RELEASE
#ifdef    SDL_VIDEO
	printf( "  -nosdlvidmodecheck %s\n", "Some X servers don't like checking vidmode first, so just switch");
#endif // SDL_VIDEO

#ifdef    EDITOR
	printf( "\n Editor:\n\n");
	printf( "  -autoload <file>   %s\n", "Autoload a level in the editor");
	printf( "  -macdata           %s\n", "Read and write mac data files in editor (swap colors)");
	printf( "  -hoarddata         %s\n", "Make the hoard ham file from some files, then exit");
#endif // EDITOR

/*	KEPT FOR FURTHER REFERENCE
	printf( "\n Unused / Obsolete:\n\n");
	printf( "  -nobm              %s\n", "FIXME: Undocumented");
	printf( "  -bigpig            %s\n", "FIXME: Undocumented");
	printf( "  -bspgen            %s\n", "FIXME: Undocumented");
	printf( "  -cdproxy           %s\n", "FIXME: Undocumented");
	printf( "  -checktime         %s\n", "FIXME: Undocumented");
	printf( "  -showmeminfo       %s\n", "FIXME: Undocumented");
	printf( "  -codereadonly      %s\n", "FIXME: Undocumented");
	printf( "  -cyberimpact       %s\n", "FIXME: Undocumented");
	printf( "  -debugmode         %s\n", "FIXME: Undocumented");
	printf( "  -disallowgfx       %s\n", "FIXME: Undocumented");
	printf( "  -disallowreboot    %s\n", "FIXME: Undocumented");
	printf( "  -dynamicsockets    %s\n", "FIXME: Undocumented");
	printf( "  -forcegfx          %s\n", "FIXME: Undocumented");
	printf( "  -hw_3dacc          %s\n", "FIXME: Undocumented");
	printf( "  -jasen             %s\n", "FIXME: Undocumented");
	printf( "  -joyslow           %s\n", "FIXME: Undocumented");
	printf( "  -logfile           %s\n", "FIXME: Undocumented");
	printf( "  -memdbg            %s\n", "FIXME: Undocumented");
	printf( "  -monodebug         %s\n", "FIXME: Undocumented");
	printf( "  -nocyberman        %s\n", "FIXME: Undocumented");
	printf( "  -nomatrixcheat     %s\n", "FIXME: Undocumented");
	printf( "  -showaddress       %s\n", "FIXME: Undocumented");
	printf( "  -socket            %s\n", "FIXME: Undocumented");
	printf( "  -nomodex           %s\n", "FIXME: Undocumented");
	printf( "  -ordinaljoy        %s\n", "FIXME: Undocumented");
	printf( "  -specialdevice     %s\n", "FIXME: Undocumented");
	printf( "  -stickmag          %s\n", "FIXME: Undocumented");
	printf( "  -stopwatch         %s\n", "FIXME: Undocumented");
	printf( "  -sysram            %s\n", "FIXME: Undocumented");
	printf( "  -tsengdebug1       %s\n", "FIXME: Undocumented");
	printf( "  -tsengdebug2       %s\n", "FIXME: Undocumented");
	printf( "  -tsengdebug3       %s\n", "FIXME: Undocumented");
	printf( "  -vidram            %s\n", "FIXME: Undocumented");
	printf( "  -xname             %s\n", "FIXME: Undocumented");
	printf( "  -xver              %s\n", "FIXME: Undocumented");
	printf( "  -gl_intensity4_ok  %s\n", "FIXME: Undocumented");
	printf( "  -gl_luminance4_alpha4_ok %s\n", "FIXME: Undocumented");
	printf( "  -gl_readpixels_ok  %s\n", "FIXME: Undocumented");
	printf( "  -gl_rgba2_ok       %s\n", "FIXME: Undocumented");
	printf( "  -gl_test1          %s\n", "FIXME: Undocumented");
	printf( "  -gl_test2          %s\n", "FIXME: Undocumented");
	printf( "  -gl_vidmem         %s\n", "FIXME: Undocumented");
	printf( "  -hwsurface         %s\n", "FIXME: Undocumented");
	printf( "  -gl_library <l>    %s\n", "use alternate opengl library");
	printf( "  -emul              %s\n", "Certain video cards need this option in order to run game");
	printf( "  -ddemul            %s\n", "If -emul doesn't work, use this option");
	printf( "  -rtscts            %s\n", "Same as -ctsrts");
	printf( "  -semiwin           %s\n", "Use non-fullscreen mode");
	printf( "  -ihaveabrokenmouse %s\n", "try to make mouse work if it is not currently");
	printf( "  -joy209            %s\n", "Use alternate port 209 for joystick");
	printf( "  -serialdevice <s>  %s\n", "Set serial/modem device to <s>");
	printf( "  -serialread <r>    %s\n", "Set serial/modem to read from <r>");
*/
#endif // NDEBUG

	printf( "\n Help:\n\n");
	printf( "  -help, -h, -?, ?   %s\n", "View this help screen");
	printf( "\n\n");
}

void sdl_close()
{
	SDL_Quit();
}

//set this to force game to run in low res
int disable_high_res=0;

void do_register_player(ubyte *title_pal)
{
	Players[Player_num].callsign[0] = '\0';

	key_flush();

	//now, before we bring up the register player menu, we need to
	//do some stuff to make sure the palette is ok.  First, we need to
	//get our current palette into the 2d's array, so the remapping will
	//work.  Second, we need to remap the fonts.  Third, we need to fill
	//in part of the fade tables so the darkening of the menu edges works

	memcpy(gr_palette,title_pal,sizeof(gr_palette));
	remap_fonts_and_menus(1);
	RegisterPlayer();		//get player's name
	read_player_file(); // read out now so we are able to use game resolution in menu
}

#define PROGNAME argv[0]

extern char Language[];

//can we do highres menus?
extern int MenuHiresAvailable;

int Inferno_verbose = 0;

//added on 11/18/98 by Victor Rachels to add -mission and -startgame
int start_net_immediately = 0;
//int start_with_mission = 0;
//char *start_with_mission_name;
//end this section addition

u_int32_t MENU_HIRES_MODE=SM(640,480); //#define MENU_HIRES_MODE SM(640,480)
int menu_use_game_res=1;

//	DESCENT II by Parallax Software
//		Descent Main

//extern ubyte gr_current_pal[];

#ifdef	EDITOR
int	Auto_exit = 0;
char	Auto_file[128] = "";
#endif

int main(int argc, char *argv[])
{
	int t;
	ubyte title_pal[768];

	con_init();  // Initialise the console
	mem_init();

	error_init(NULL, NULL);
	PHYSFSX_init(argc, argv);
	
	if (FindArg("-debug"))
		con_threshold.value = (float)2;
	else if (FindArg("-verbose"))
		con_threshold.value = (float)1;

	if (! cfile_init("descent2.hog", 1))
		if (! cfile_init("d2demo.hog", 1))
			Error("Could not find a valid hog file (descent2.hog or d2demo.hog)\nPossible locations are:\n"
#ifdef __unix__
			      "\t$HOME/.d2x-rebirth\n"
			      "\t" SHAREPATH "\n"
#else
				  "\tCurrent directory\n"
#endif
				  "Or use the -hogdir option to specify an alternate location.");
	load_text();

	//print out the banner title
	con_printf(CON_NORMAL, "\nDESCENT 2 %s v%d.%d",VERSION_TYPE,Version_major,Version_minor);
	#if 1	//def VERSION_NAME
	con_printf(CON_NORMAL, "  %s", DESCENT_VERSION);	// D2X version
	#endif
	if (cfexist(MISSION_DIR "d2x.hog")) {
		con_printf(CON_NORMAL, "  Vertigo Enhanced");
		vertigo_present = 1;
	}

	con_printf(CON_NORMAL, "  %s %s\n", __DATE__,__TIME__);
	con_printf(CON_NORMAL, "%s\n%s\n",TXT_COPYRIGHT,TXT_TRADEMARK);
	con_printf(CON_NORMAL, "This is a MODIFIED version of Descent 2. Copyright (c) 1999 Peter Hawkins\n");
	con_printf(CON_NORMAL, "                                         Copyright (c) 2002 Bradley Bell\n");
	con_printf(CON_NORMAL, "                                         Copyright (c) 2005 Christian Beckhaeuser\n");


	if (FindArg( "-help" ) || FindArg( "-h" ) || FindArg( "-?" ) || FindArg( "?" ) ) {
		print_commandline_help();
		set_exit_message("");

#ifdef __MINGW32__
		exit(0);  /* mingw hangs on this return.  dunno why */
#endif
		return(0);
	}

	con_printf(CON_NORMAL, "\n");
	con_printf(CON_NORMAL, TXT_HELP, PROGNAME);		//help message has %s for program name
	con_printf(CON_NORMAL, "\n");

	{
		char **i, **list;

		list = PHYSFS_getSearchPath();
		for (i = list; *i != NULL; i++)
			con_printf(CON_VERBOSE, "PHYSFS: [%s] is in the search path.\n", *i);
		PHYSFS_freeList(list);

		list = PHYSFS_getCdRomDirs();
		for (i = list; *i != NULL; i++)
			con_printf(CON_VERBOSE, "PHYSFS: cdrom dir [%s] is available.\n", *i);
		PHYSFS_freeList(list);

		list = PHYSFS_enumerateFiles("");
		for (i = list; *i != NULL; i++)
			con_printf(CON_DEBUG, "PHYSFS: * We've got [%s].\n", *i);
		PHYSFS_freeList(list);
	}

	if (SDL_Init(SDL_INIT_VIDEO)<0)
		Error("SDL library initialisation failed: %s.",SDL_GetError());

	key_init();

	if (!GameArg.SndNoSound)
		digi_init();
	
	if (!GameArg.CtlNoMouse)
		d_mouse_init();

	if (!GameArg.CtlNoJoystick)
		joy_init();
	
	atexit(sdl_close);

	//con_printf(CON_VERBOSE, "\n%s...", "Checking for Descent 2 CD-ROM");

	if ((t = FindArg("-rearviewtime")))
	{
		float f = atof(Args[t + 1]);
		Rear_view_leave_time = f * f1_0;
	}
	con_printf(CON_VERBOSE, "Rear_view_leave_time=0x%x (%f sec)\n", Rear_view_leave_time, Rear_view_leave_time / (float)f1_0);

	if (FindArg("-persistentdebris"))
		persistent_debris=1;

	if (FindArg("-renderstats"))
		gr_renderstats = 1;

	#ifndef NDEBUG
	if ( FindArg( "-checktime") )
		checktime = 1;
	#endif

	if ((t=FindArg("-tmap"))){
		select_tmap(Args[t+1]);
	}else
		select_tmap(NULL);

	if ((t=FindArg("-hud"))){ // ZICO - new HUD modes
		t=atoi(Args[t+1]);
		if(t>=0 && t<GAUGE_HUD_NUMMODES)
                 Gauge_hud_mode = t;
	}

	Lighting_on = 1;

	#ifdef EDITOR
	if (gr_check_mode(SM(800, 600)) != 0)
	{
		con_printf(CON_NORMAL, "The editor will not be available, press any key to start game...\n" );
		Function_mode = FMODE_MENU;
	}
	#endif

	if (!WVIDEO_running)
		con_printf(CON_DEBUG,"WVIDEO_running = %d\n",WVIDEO_running);

	con_printf (CON_VERBOSE, "%s", TXT_VERBOSE_1);
	ReadConfigFile();

	if (!VR_offscreen_buffer)	//if hasn't been initialied (by headset init)
		set_display_mode(0);		//..then set default display mode

	{
		int screen_width = 640;
		int screen_height = 480;
//		int screen_flags = 0;

// added ifdef on 9/30/98 by Matt Mueller to fix high res in linux
#if !defined (NDEBUG) && !defined (OGL)
		if (FindArg("-nodoublebuffer"))
// end addition -MM
// later modified by kreatordxx to allow double buffering to be disabled for any supported platform
		{
			con_printf(CON_VERBOSE, "Double-buffering disabled...\n");
			Game_double_buffer = 0;
		}
#endif

		// added 3/24/99 by Owen Evans for screen res changing
		Game_screen_mode = SM(screen_width, screen_height);
		// end added -OE
//		game_init_render_buffers(screen_width, screen_height, VR_NONE, screen_flags);

	}

	{
// added/edited on 12/14/98 by Matt Mueller - override res in d1x.ini with command line args
		int i, argnum = INT_MAX, w, h;
// added on 9/30/98 by Matt Mueller for selectable automap modes - edited 11/21/99 whee, more fun with defines. - edited 03/31/02 to use new FindResArg.
#define SMODE(V,VV,VG) if ((i=FindResArg(#V, &w, &h)) && (i < argnum)) { argnum = i; VV = SM(w, h); VG = 0;  if(w<640||h<480) disable_high_res=1; }
#define SMODE_GR(V,VG) if ((i=FindArg("-" #V "_gameres"))){if (i<argnum) VG=1;}
#define SMODE_PRINT(V,VV,VG) if (VG) con_printf(CON_VERBOSE, #V " using game resolution ...\n"); else con_printf(CON_VERBOSE, #V " using %ix%i ...\n",SM_W(VV),SM_H(VV) );
// aren't #defines great? :)
// end addition/edit -MM
#define S_MODE(V,VV,VG) argnum = INT_MAX; SMODE(V, VV, VG); SMODE_GR(V, VG); SMODE_PRINT(V, VV, VG);


		S_MODE(automap,automap_mode,automap_use_game_res);
		S_MODE(menu,MENU_HIRES_MODE,menu_use_game_res);
	 }
//end addition -MM

	con_printf(CON_VERBOSE, "\n%s\n\n", TXT_INITIALIZING_GRAPHICS);
	if (FindArg("-nofade"))
		grd_fades_disabled=1;

	//determine whether we're using high-res menus & movies
	if (FindArg("-nohires") || FindArg("-nohighres") || (gr_check_mode(MENU_HIRES_MODE) != 0) || disable_high_res)
		MovieHires = MenuHires = MenuHiresAvailable = 0;
	else
		//NOTE LINK TO ABOVE!
		MovieHires = MenuHires = MenuHiresAvailable = 1;

	if (FindArg( "-lowresmovies" ))
		MovieHires = 0;

	if ((t=gr_init(0))!=0)				//doesn't do much
		Error(TXT_CANT_INIT_GFX,t);

	gr_set_mode(MENU_SCREEN_MODE);

	// Load the palette stuff. Returns non-zero if error.
	con_printf(CON_DEBUG, "Initializing palette system...\n" );
	gr_use_palette_table(D2_DEFAULT_PALETTE );

	con_printf(CON_DEBUG, "Initializing font system...\n" );
	gamefont_init();	// must load after palette data loaded.

	con_printf( CON_DEBUG, "Initializing movie libraries...\n" );
	init_movies();		//init movie libraries

#if 0
	con_printf(CON_VERBOSE, "Going into graphics mode...\n");
	gr_set_mode(MovieHires?SM(640,480):SM(320,200));
#endif

	show_titles();

	con_printf( CON_DEBUG, "\nShowing loading screen..." );
	show_loading_screen(title_pal); // title_pal is needed (see below)

	con_printf( CON_DEBUG , "\nDoing bm_init..." );
	#ifdef EDITOR
	if (!bm_init_use_tbl())
	#endif
		bm_init();

	#ifdef EDITOR
	if (FindArg("-hoarddata") != 0) {
		save_hoard_data();
		exit(1);
	}
	#endif

	//the bitmap loading code changes gr_palette, so restore it
	memcpy(gr_palette,title_pal,sizeof(gr_palette));

	if ( FindArg( "-norun" ) )
		return(0);

	con_printf( CON_DEBUG, "\nInitializing 3d system..." );
	g3_init();

	con_printf( CON_DEBUG, "\nInitializing texture caching system..." );
	texmerge_init( 10 );		// 10 cache bitmaps

	con_printf( CON_DEBUG, "\nRunning game...\n" );
	set_screen_mode(SCREEN_MENU);

	init_game();

	//	If built with editor, option to auto-load a level and quit game
	//	to write certain data.
	#ifdef	EDITOR
	{	int t;
	if ( (t = FindArg( "-autoload" )) ) {
		Auto_exit = 1;
		strcpy(Auto_file, Args[t+1]);
	}
		
	}

	if (Auto_exit) {
		strcpy(Players[0].callsign, "dummy");
	} else
	#endif
	{
// 		do_register_player(title_pal);
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
			strlwr(filename);
			if(!strstr(filename,".plr")) // if player hasn't specified .plr extension in argument, add it
				strcat(filename,".plr");
			if(cfexist(filename))
			{
				strcpy(strstr(filename,".plr"),"\0");
				strcpy(Players[Player_num].callsign, GameArg.SysUsePlayersDir? &filename[8] : filename);
				read_player_file();
				WriteConfigFile();
				remap_fonts_and_menus(1);
			}
			else //pilot doesn't exist. get pilot.
				do_register_player(title_pal);
		}
		else
			do_register_player(title_pal);
	}

	gr_palette_fade_out( title_pal, 32, 0 );

	Game_mode = GM_GAME_OVER;

	while (Function_mode != FMODE_EXIT)
	{
		switch( Function_mode )	{
		case FMODE_MENU:
			set_screen_mode(SCREEN_MENU);
			#ifdef EDITOR
			if (Auto_exit) {
				strcpy((char *)&Level_names[0], Auto_file);
				LoadLevel(1, 1);
				Function_mode = FMODE_EXIT;
				break;
			}
			#endif

			gr_palette_clear();		//I'm not sure why we need this, but we do
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
#ifdef __WATCOMC__
			_harderr( (void*)descent_critical_error_handler );		// Reinstall game error handler
#endif
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

	#ifndef NDEBUG
	if ( FindArg( "-showmeminfo" ) )
		show_mem_info = 1;		// Make memory statistics show
	#endif

#ifdef _WIN32
	digi_stop_current_song(); // ZICO - stop all midis // hack for some onboard soundcards
	digi_close(); // ZICO - stop all sounds // hack for some onboard soundcards
#endif

	return(0);		//presumably successful exit
}

void quit_request()
{
#ifdef NETWORK
//	void network_abort_game();
//	if(Network_status)
//		network_abort_game();
#endif
	exit(0);
}
