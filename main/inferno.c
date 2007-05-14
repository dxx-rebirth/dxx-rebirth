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
/* $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/inferno.c,v $
 * $Revision: 1.3 $
 * $Author: michaelstather $
 * $Date: 2006/03/19 14:41:26 $
 *
 * main() for Inferno  
 *
 * $Log: inferno.c,v $
 * Revision 1.3  2006/03/19 14:41:26  michaelstather
 * Cleaned up command line arguments
 * Reformatting
 *
 * Revision 1.2  2006/03/18 23:08:13  michaelstather
 * New build system by KyroMaster
 *
 * Revision 1.1.1.1  2006/03/17 19:44:36  zicodxx
 * initial import
 */

#ifdef RCS
static char rcsid[] = "$Id: inferno.c,v 1.3 2006/03/19 14:41:26 michaelstather Exp $";
#endif

#ifdef __GNUC__
static char copyright[] = "DESCENT   COPYRIGHT (C) 1994,1995 PARALLAX SOFTWARE CORPORATION";
static char *__reference[2]={copyright,(char *)__reference};
#endif


#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <SDL/SDL.h>

#ifndef _MSC_VER
#include <unistd.h>
#endif

#ifdef __MSDOS__
#include <time.h>
#endif

#ifdef __WINDOWS__
#include <windows.h>
#endif

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
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
#include "ipx.h"
#include "network.h"
#include "modem.h"
#include "gamefont.h"
#include "kconfig.h"
#include "newmenu.h"
#include "desc_id.h"
#include "config.h"
#include "joydefs.h"
#include "multi.h"
#include "songs.h"
#include "cfile.h"
#include "gameseq.h"
#include "timer.h"
#include "joy.h"
#include "key.h"
#include "mono.h"
#include "palette.h"
#include "bm.h"
#include "screens.h"
#include "arch.h"
#include "hudmsg.h"
#include "playsave.h"
#include "d_io.h"
#include "automap.h"
#include "hudlog.h"
#include "cdplay.h"
#include "ban.h"
#include "gauges.h"
#include "pingstat.h"
#include "physics.h"
#include "strutil.h"
#include "altsound.h"
#include "../texmap/scanline.h" //for select_tmap -MM
#include "vers_id.h"
#include "collide.h"
#include "newdemo.h"

void show_order_form();

static const char desc_id_checksum_str[] = DESC_ID_CHKSUM;
char desc_id_exit_num = 0;
int Function_mode=FMODE_MENU; //game or editor?
int Screen_mode=-1; //game screen or editor screen?
int descent_critical_error = 0;
unsigned int descent_critical_deverror = 0;
unsigned int descent_critical_errcode = 0;
u_int32_t menu_screen_mode=SM(640,480);
int menu_use_game_res=1;

#ifdef EDITOR
int Inferno_is_800x600_available = 0;
#endif

int init_graphics()
{
#ifdef __MSDOS__
	int result;
	result=gr_check_mode(SM(320,200));

#ifdef EDITOR
	if ( result==0 )	
		result=gr_check_mode(SM(800,600));
#endif

	switch( result )	{
		case  0:		//Mode set OK
#ifdef EDITOR
						Inferno_is_800x600_available = 1;
#endif
						break;
		case  1:		//No VGA adapter installed
						printf("%s\n", TXT_REQUIRES_VGA );
						return 1;
		case 10:		//Error allocating selector for A0000h
						printf( "%s\n",TXT_ERROR_SELECTOR );
						return 1;
		case 11:		//Not a valid mode support by gr.lib
						printf( "%s\n", TXT_ERROR_GRAPHICS );
						return 1;
#ifdef EDITOR
		case  3:		//Monitor doesn't support that VESA mode.
		case  4:		//Video card doesn't support that VESA mode.

						printf( "Your VESA driver or video hardware doesn't support 800x600 256-color mode.\n" );
						break;
		case  5:		//No VESA driver found.
						printf( "No VESA driver detected.\n" );
						break;
		case  2:		//Program doesn't support this VESA granularity
		case  6:		//Bad Status after VESA call/
		case  7:		//Not enough DOS memory to call VESA functions.
                case  8:                //Error using DPMI.
		case  9:		//Error setting logical line width.
		default:
						printf( "Error %d using 800x600 256-color VESA mode.\n", result );
						break;
#endif
	}

	#ifdef EDITOR
	if (!Inferno_is_800x600_available)	{
		printf( "The editor will not be available, press any key to start game...\n" );
		Function_mode = FMODE_MENU;
		getch();
	}
	#endif
#endif
	return 0;
}

void show_commandline_help()
{
	printf( "\n System Options:\n\n");
	printf( "  -fps               %s\n", "Enable FPS indicator by default");
	printf( "  -nonicefps         %s\n", "Disable CPU cycle freeing. Higher CPU load, but game may be smoother");
	printf( "  -maxfps <n>        %s\n", "Set maximum framerate (1-80)");
	printf( "  -missiondir <d>    %s\n", "Set alternate mission dir to <d> instead of missions/");
	printf( "  -hudlog            %s\n", "Start hudlog immediately");
	printf( "  -lowmem            %s\n", "Lowers animation detail for better performance with low memory");
	printf( "  -legacyhomers      %s\n", "Activate original homing missiles (FPS and physics independent)");

	printf( "\n Controls:\n\n");
	printf( "  -NoJoystick        %s\n", "Disables joystick support");
	printf( "  -mouselook         %s\n", "Activate mouselook. Works in singleplayer only");
	printf( "  -grabmouse         %s\n", "Keeps the mouse from wandering out of the window");

	printf( "\n Sound:\n\n");
	printf( "  -Volume <v>        %s\n", "Sets sound volume to v, where v is between 0 and 100");
	printf( "  -NoSound           %s\n", "Disables sound drivers");
	printf( "  -NoMusic           %s\n", "Disables music; sound effects remain enabled");

	printf( "\n Graphics:\n\n");
	printf( "  -menu<X>x<Y>       %s\n", "Set menu-resolution to <X> by <Y> instead of game-resolution");
	printf( "  -aspect<Y>x<X>     %s\n", "use specified aspect");
	printf( "  -cockpit <n>       %s\n", "Set initial cockpit. 0=full 2=status bar 3=full screen");
	printf( "  -hud <h>           %s\n", "Set hud mode.  0=normal 1-3=new");
        printf( "  -hudlines <l>      %s\n", "Number of hud messages to show");
	printf( "  -hiresfont         %s\n", "use high resolution fonts if available");
	printf( "  -persistentdebris  %s\n", "Enable persistent debris");
#ifdef    GR_SUPPORTS_FULLSCREEN_TOGGLE
	printf( "  -window            %s\n", "Run the game in a window");
#endif // GR_SUPPORTS_FULLSCREEN_TOGGLE

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

	printf( "\n Quickstart:\n\n");
	printf( "  -ini <file>        %s\n", "Option file (alternate to command line), defaults to d1x.ini");
	printf( "  -notitles          %s\n", "Do not show titlescreens on startup");
	printf( "  -pilot <pilot>     %s\n", "Select this pilot-file automatically");
	printf( "  -autodemo          %s\n", "Start in demo mode");

#ifdef    NETWORK
	printf( "\n Multiplayer:\n\n");
	printf( "  -mprofile <f>      %s\n", "Use multi game profile <f>");
	printf( "  -startnetgame      %s\n", "Start an IPX network game immediately");
	printf( "  -joinnetgame       %s\n", "Skip to join IPX menu screen");
	printf( "  -nobans            %s\n", "Don't use saved bans");
	printf( "  -savebans          %s\n", "Automatically save new bans");
	printf( "  -pingstats         %s\n", "Show pingstats on hud");
	printf( "  -noredundancy      %s\n", "Do not send messages when picking up redundant items in multiplayer");
	printf( "  -playermessages    %s\n", "View only messages from other players in multi - overrides -noredundancy");
	printf( "  -handicap <n>      %s\n", "Start game with <n> shields. Must be < 100 for multi");
	printf( "  -hudlog_multi      %s\n", "Start hudlog upon entering multiplayer games");
        printf( "  -msgcolorlevel <c> %s\n", "Level of colorization for hud messages\n\t\t\t0=none(old style)\n\t\t\t1=color names in talk messages only(default)\n\t\t\t2=also color names in kill/join/etc messages\n\t\t\t3=talk messages are fully colored, not just names");
#ifdef    SUPPORTS_NET_IP
        printf( "  -ip_nogetmyaddr    %s\n", "Prevent autodetection of local ip address");
        printf( "  -ip_myaddr <a>     %s\n", "Use <a> as local ip address");
        printf( "  -ip_bind_addr <a>  %s\n", "Bind to <a> instead of INADDR_ANY");
        printf( "  -ip_baseport <p>   %s\n", "Use <p> as offset from normal port (allows multiple instances of d1x to be run on a single computer)");
#endif // SUPPORTS_NET_IP
#endif // NETWORK

#ifndef   NDEBUG
	printf( "\n Debug:\n\n");
	printf( "  -Verbose           %s\n", "Shows initialization steps for tech support");
	printf( "  -norun             %s\n", "Bail out after initialization");
	printf( "  -font320 <f>       %s\n", "Font to use for res 320x* and above (default font3-1.fnt)");
	printf( "  -font640 <f>       %s\n", "Font to use for res 640x* and above");
	printf( "  -font800 <f>       %s\n", "Font to use for res 800x* and above");
	printf( "  -font1024 <f>      %s\n", "Font to use for res 1024x* and above");
#ifdef    OGL
	printf( "  -gl_texmaxfilt <f> %s\n", "Set GL_TEXTURE_MAX_FILTER");
	printf( "  -gl_texminfilt <f> %s\n", "Set GL_TEXTURE_MIN_FILTER");
	printf( "  -gl_alttexmerge    %s\n", "Use new texmerge, usually uses less ram (default)");
	printf( "  -gl_stdtexmerge    %s\n", "Use old texmerge, uses more ram, but _might_ be a bit faster");
	printf( "  -gl_16bittextures  %s\n", "Attempt to use 16bit textures");
#ifdef    OGL_RUNTIME_LOAD
	printf( "  -gl_library <l>    %s\n", "Use alternate opengl library");
#endif // OGL_RUNTIME_LOAD
#else  // ifndef OGL
	printf( "  -tmap <t>          %s\n", "Select texmapper to use (c,fp,quad,i386,pent,ppro)");
#endif // OGL
#ifdef    __SDL__
	printf( "  -nosdlvidmodecheck %s\n", "Some X servers don't like checking vidmode first, so just switch");
#endif // __SDL__

/*	KEPT FOR FURTHER REFERENCE
	printf( "\n Unused / Obsolete:\n\n");
	printf( "  -nocdaudio         %s\n", "Disable cd audio");
	printf( "  -playlist \"...\"    %s\n", "Set the cd audio playlist to tracks \"a b c ... f g\"");
	printf( "  -serialdevice <s>  %s\n", "Set serial/modem device to <s>");
	printf( "  -serialread <r>    %s\n", "Set serial/modem to read from <r>");
*/
#endif // NDEBUG

	printf( "\n Help:\n\n");
	printf( "  -help, -h, -?, ?   %s\n", "View this help screen");
	printf( "\n\n");
}

extern fix fixed_frametime;
extern int framerate_on;
extern void vfx_set_palette_sub(ubyte *);
extern int mouselook;
extern int newhomers;
#ifndef RELEASE
extern int invulnerability;
#endif

int Inferno_verbose = 0;
int start_net_immediately = 0;

int main(int argc,char **argv)
{
	int i,t;
	u_int32_t screen_mode = SM(640,480);

	error_init(NULL);

	setbuf(stdout, NULL); // unbuffered output via printf

	ReadConfigFile();

	InitArgs( argc,argv );

	if ( FindArg( "-verbose" ) )
		Inferno_verbose = 1;

	// Things to initialize before anything else
	arch_init_start();

	if (!cfexist(DESCENT_DATA_PATH "descent.hog") || !cfexist(DESCENT_DATA_PATH "descent.pig"))
		Error("Could not find valid descent.hog and/or descent.pig in\n"
#ifdef __unix__
				"\t" DESCENT_DATA_PATH "\n"
#else
				"\tcurrent directory\n"
#endif
				);

	load_text();

	printf(DESCENT_VERSION "\n"
	       "This is a MODIFIED version of DESCENT which is NOT supported by Parallax or\n"
	       "Interplay. Use at your own risk! Copyright (c) 2005 Christian Beckhaeuser\n");
	printf("Based on: DESCENT   %s\n", VERSION_NAME);
	printf("%s\n%s\n",TXT_COPYRIGHT,TXT_TRADEMARK);

	if (FindArg( "-help" ) || FindArg( "-h" ) || FindArg( "-?" ) || FindArg( "?" ) ) {
		show_commandline_help();
                set_exit_message("");
		return 0;
	}

	printf("\nType 'd1x-rebirth-gl/sdl -help' for a list of command-line options.\n");

	if ((t = FindArg( "-missiondir" )))
		cfile_use_alternate_hogdir(Args[t+1]);
	else
		cfile_use_alternate_hogdir(DESCENT_DATA_PATH "missions/");

	if ((t=FindArg("-tmap")))
		select_tmap(Args[t+1]);
	else
		select_tmap(NULL);

	if ((t=FindArg("-cockpit"))){
		t=atoi(Args[t+1]);
		if(t==0 || t==2 || t==3)
			Cockpit_mode = t;
	}

	if ((t=FindArg("-hud"))){
		t=atoi(Args[t+1]);
		if(t>=0 && t<GAUGE_HUD_NUMMODES)
			Gauge_hud_mode = t;
	}

#ifdef NETWORK
	if (FindArg("-pingstats"))
		ping_stats_on = 1;
#endif

	if ((t=FindArg("-msgcolorlevel"))){
		extern int gr_message_color_level;
		t=atoi(Args[t+1]);
		if(t>=0 && t<=3)
                gr_message_color_level = t;
	}
		 
	if ((t=FindArg("-hudlines"))){
		t=atoi(Args[t+1]);
		if(t>0 && t<=HUD_MAX_NUM)
                HUD_max_num_disp = t;
	}

	if (FindArg("-hudlog_multi"))
	HUD_log_multi_autostart = 1;
	
	if (FindArg("-hudlog"))
	HUD_log_autostart = 1;

	if (FindArg("-noredundancy"))
		MSG_Noredundancy = 1;

	if (FindArg("-playermessages"))
		MSG_Playermessages = 1;

	if ((t = FindArg( "-handicap" ))) {
		t = i2f(atoi(Args[t+1]));
		if(t < F1_0)
			t= F1_0;
		else if (t > F1_0*200)
			t = F1_0*200;
		handicap=t;
	}

	if ((t = FindArg( "-maxfps" ))) {
		t=atoi(Args[t+1]);
		if (t>0&&t<=80)
			maxfps=t;
	}

#ifdef NETWORK
	if(FindArg( "-startnetgame" ))
		start_net_immediately = 1;

	if(FindArg( "-joinnetgame" ))
		start_net_immediately = 2;

	if(FindArg( "-ackmsg" ))
		ackdebugmsg = 1;

	readbans();
#endif

	if(FindArg( "-fastext" ))
		extfaster=1;

	if (FindArg("-mouselook"))
		mouselook=1;
	else
		mouselook=0;

	if (FindArg("-legacyhomers"))
		newhomers = 0;

	if (FindArg("-persistentdebris"))
		persistent_debris=1;

	if ( FindArg( "-fps" ))
		framerate_on = 1;

	if ( FindArg( "-nonicefps" ))
		use_nice_fps = 0;

	if ( FindArg( "-autodemo" ))
		Auto_demo = 1;

	#ifndef RELEASE
	if ( FindArg( "-noscreens" ) )
		Skip_briefing_screens = 1;

	if ( FindArg( "-invulnerability") )
		invulnerability = 1;
	#endif

	if (Inferno_verbose)
		printf ("%s", TXT_VERBOSE_1);

	arch_init();

        cd_init();

	if (init_graphics()) return 1;

	//------------ Init sound ---------------
	if (!FindArg( "-nosound" ))	{
		if (digi_init())	{
#ifdef ALLEGRO
			printf( "\nFailure initializing sound: %s\n", allegro_error);
                        printf( "Make sure your soundcard is installed and not in use. \nIf the problem persists, check your allegro.cfg.\n");
#else
			printf( "\nFailure initializing sound.\n");
#endif
#ifndef __linux__
			key_getch();
#endif
		}
	} 
	else {
		if (Inferno_verbose) printf( "\n%s",TXT_SOUND_DISABLED );
	}

#ifdef NETWORK
	if (!FindArg("-noserial"))
		serial_active = 1;
	else
		serial_active = 0;
#endif
	Game_screen_mode = screen_mode;
// 	game_init_render_buffers(screen_width, screen_height, VR_NONE);

	{
		int i, argnum=INT_MAX, w, h;
#define SMODE(V,VV,VG) if ((i=FindResArg(#V, &w, &h)) && (i < argnum)) { argnum = i; VV = SM(w, h); VG = 0; }
#define SMODE_GR(V,VG) if ((i=FindArg("-" #V "_gameres"))){if (i<argnum) VG=1;}
#define SMODE_PRINT(V,VV,VG) if (Inferno_verbose) { if (VG) printf( #V " using game resolution ...\n"); else printf( #V " using %ix%i ...\n",SM_W(VV),SM_H(VV) ); }
#define S_MODE(V,VV,VG) argnum = INT_MAX; SMODE(V, VV, VG); SMODE_GR(V, VG); SMODE_PRINT(V, VV, VG);

		S_MODE(automap,automap_mode,automap_use_game_res);
		S_MODE(menu,menu_screen_mode,menu_use_game_res);
	}
//end addition -MM
	
#ifdef NETWORK
	control_invul_time = 0;
#endif

	if (Inferno_verbose)
		printf( "\n%s\n\n", TXT_INITIALIZING_GRAPHICS);
	if ((t=gr_init( SM_ORIGINAL ))!=0)
		Error(TXT_CANT_INIT_GFX,t);
	// Load the palette stuff. Returns non-zero if error.
	mprintf( (0, "Going into graphics mode..." ));
	gr_set_mode(MENU_SCREEN_MODE);
#ifdef OGL
	/* hack to fix initial screens with ogl */
	{
 		int old_screen_mode = Screen_mode;
		Screen_mode = MENU_SCREEN_MODE;
		Screen_mode = old_screen_mode;
	}
#endif	
	mprintf( (0, "\nInitializing palette system..." ));
	gr_use_palette_table( "PALETTE.256" );
	mprintf( (0, "\nInitializing font system..." ));
	gamefont_init(); // must load after palette data loaded.
	songs_play_song( SONG_TITLE, 1 );

#ifndef QUICKSTART
#ifndef SHAREWARE
	if ( !FindArg( "-notitles" ) ) 
#endif
	{
		show_title_screen( "iplogo1.pcx", 1 );
		show_title_screen( "logo.pcx", 1 );
	}
#endif

	show_title_screen( "descent.pcx", 2 );

#ifdef SHAREWARE
	bm_init_use_tbl();
#else
#ifdef EDITOR
	if ( !FindArg("-nobm") )
		bm_init_use_tbl();
	else
		bm_init();
#else
		bm_init();
#endif
#endif

	if ( FindArg( "-norun" ) )
		return(0);

	mprintf( (0, "\nInitializing 3d system..." ));
	g3_init();
	mprintf( (0, "\nInitializing texture caching system..." ));
	if (FindArg( "-lowmem" ))
		texmerge_init( 10 ); // if we are low on mem, only use 10 cache bitmaps
	else
		texmerge_init( 9999 ); // otherwise, use as much as possible (its still limited by the #define in texmerge.c, so it won't actually use 9999) -MM
	mprintf( (0, "\nRunning game...\n" ));
#ifdef SCRIPT
	script_init();
#endif
	set_screen_mode(SCREEN_MENU);

	init_game();
	set_detail_level_parameters(Detail_level);

	Players[Player_num].callsign[0] = '\0';

	key_flush();
#ifdef QUICKSTART
	strcpy(Players[Player_num].callsign, config_last_player);
	read_player_file();
	Auto_leveling_on = Default_leveling_on;
	write_player_file();
#else
	if((i=FindArg("-pilot")))
	{
		char filename[15];
		int j;
		snprintf(filename, 12, Args[i+1]);
		for (j=0; filename[j] != '\0'; j++) {
			switch (filename[j]) {
				case ' ':
					filename[j] = '\0';
			}
		}
		strlwr(filename);
		if(!strstr(filename,".plr")) // if player hasn't specified .plr extension in argument, add it
			strcat(filename,".plr");
		if(!access(filename,4))
		{
			strcpy(strstr(filename,".plr"),"\0");
			strcpy(Players[Player_num].callsign,filename);
			read_player_file();
			Auto_leveling_on = Default_leveling_on;
			WriteConfigFile();
		}
		else //pilot doesn't exist. get pilot.
			if(!RegisterPlayer())
				Function_mode = FMODE_EXIT;
	}
	else
		if(!RegisterPlayer())
			Function_mode = FMODE_EXIT;
#endif

	gr_palette_fade_out( NULL, 32, 0 );

	Game_mode = GM_GAME_OVER;

#ifndef SHAREWARE
	t = build_mission_list(0); // This also loads mission 0.
#endif


#ifdef QUICKSTART
	Difficulty_level = Player_default_difficulty;
	Skip_briefing_screens = 1;

	{
		int default_mission = 0;
		for (i=0;i<t;i++) {
			if ( !strcasecmp( Mission_list[i].mission_name, config_last_mission ) )
				default_mission = i;
                }
		load_mission(default_mission);
	}
	Function_mode = FMODE_GAME;
	StartNewGame(1);
	game();
#endif
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

			/* keep the mouse from wandering in SDL */
			if (FindArg("-grabmouse"))
				SDL_WM_GrabInput(SDL_GRAB_ON);

			game();

			/* give control back to the WM */
			if (FindArg("-grabmouse"))
				SDL_WM_GrabInput(SDL_GRAB_OFF);

			if ( Function_mode == FMODE_MENU )
				songs_play_song( SONG_TITLE, 1 );
			break;
		#ifdef EDITOR
		case FMODE_EDITOR:
			keyd_editor_mode = 1;
			editor();
#ifdef __WATCOMC__
			_harderr( (void *)descent_critical_error_handler );		// Reinstall game error handler
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

#ifndef ROCKWELL_CODE
        #ifdef SHAREWARE
                show_order_form();
	#endif
#endif

	return(0); //presumably successful exit

}

void quit_request()
{
#ifdef NETWORK
	void network_abort_game();
	if(Network_status)
		network_abort_game();
#endif
	exit(0);
}
