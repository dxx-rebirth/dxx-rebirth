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
#include "key.h"
#include "mono.h"
#include "palette.h"
#include "bm.h"
#include "screens.h"
#include "hudmsg.h"
#include "playsave.h"
#include "d_io.h"
#include "ban.h"
#include "gauges.h"
#include "physics.h"
#include "strutil.h"
#include "altsound.h"
#include "../texmap/scanline.h" //for select_tmap -MM
#include "vers_id.h"
#include "collide.h"
#include "newdemo.h"
#include "joy.h"


static const char desc_id_checksum_str[] = DESC_ID_CHKSUM;
char desc_id_exit_num = 0;
int Function_mode=FMODE_MENU; //game or editor?
int Screen_mode=-1; //game screen or editor screen?
int descent_critical_error = 0;
unsigned int descent_critical_deverror = 0;
unsigned int descent_critical_errcode = 0;
void mem_init(void);

#ifdef EDITOR
int Inferno_is_800x600_available = 0;
#endif
extern void d_mouse_init(void);

void show_commandline_help()
{
	printf( "\n System Options:\n\n");
	printf( "  -fps               %s\n", "Enable FPS indicator by default");
	printf( "  -nicefps           %s\n", "Free CPU-cycles. Less CPU load, but game may become choppy");
	printf( "  -maxfps <n>        %s\n", "Set maximum framerate (1-80)");
	printf( "  -missiondir <s>    %s\n", "Set alternate mission dir to <d> instead of missions/");
	printf( "  -lowmem            %s\n", "Lowers animation detail for better performance with low memory");
	printf( "  -legacyhomers      %s\n", "Activate original homing missiles (FPS and physics dependent)");
	printf( "  -pilot <s>         %s\n", "Select this pilot-file automatically");
	printf( "  -autodemo          %s\n", "Start in demo mode");
	printf( "  -window            %s\n", "Run the game in a window");

	printf( "\n Controls:\n\n");
	printf( "  -nomouse           %s\n", "Deactivate mouse");
	printf( "  -nojoystick        %s\n", "Deactivate joystick");
	printf( "  -mouselook         %s\n", "Activate mouselook. Works in singleplayer only");
	printf( "  -grabmouse         %s\n", "Keeps the mouse from wandering out of the window");

	printf( "\n Sound:\n\n");
	printf( "  -nosound           %s\n", "Disables sound output");
	printf( "  -nomusic           %s\n", "Disables music output");

	printf( "\n Graphics:\n\n");
	printf( "  -aspect<Y>x<X>     %s\n", "use specified aspect");
	printf( "  -hud <n>           %s\n", "Set hud mode.  0=normal 1-3=new");
        printf( "  -hudlines <n>      %s\n", "Number of hud messages to show");
	printf( "  -hiresfont         %s\n", "use high resolution fonts if available");
	printf( "  -persistentdebris  %s\n", "Enable persistent debris. Works in singleplayer only");
	printf( "  -noreticle         %s\n", "Disable reticle");

#ifdef    OGL
	printf( "\n OpenGL:\n\n");
	printf( "  -gl_mipmap         %s\n", "Set gl texture filters to \"standard\" options for mipmapping");
	printf( "  -gl_trilinear      %s\n", "Set gl texture filters to trilinear mipmapping");
	printf( "  -gl_transparency   %s\n", "Enable transparency effects");
	printf( "  -gl_reticle <n>    %s\n", "Use OGL reticle 0=never 1=above 320x* 2=always");
	printf( "  -gl_voodoo         %s\n", "Force fullscreen mode only");
	printf( "  -fixedfont         %s\n", "Do not scale fonts to current resolution");
#endif // OGL

#ifdef    NETWORK
	printf( "\n Multiplayer:\n\n");
	printf( "  -mprofile <s>      %s\n", "Use multi game profile <f>");
	printf( "  -nobans            %s\n", "Don't use saved bans");
	printf( "  -savebans          %s\n", "Automatically save new bans");
	printf( "  -noredundancy      %s\n", "Do not send messages when picking up redundant items in multiplayer");
	printf( "  -playermessages    %s\n", "View only messages from other players in multi - overrides -noredundancy");
	printf( "  -ipxnetwork <n>    %s\n", "Use IPX network number <n>");
	printf( "  -ip_hostaddr <n>   %s\n", "Use <n> as host ip address");
        printf( "  -ip_nogetmyaddr    %s\n", "Prevent autodetection of local ip address");
        printf( "  -ip_myaddr <n>     %s\n", "Use <n> as local ip address");
        printf( "  -ip_baseport <n>   %s\n", "Use <n> as offset from normal port (allows multiple instances of d1x to be run on a single computer)");
#endif // NETWORK

#ifdef    EDITOR
	printf( "\n Editor:\n\n");
	printf( "  -nobm              %s\n", "Don't load BITMAPS.TBL and BITMAPS.BIN - use internal data");
#endif // EDITOR

#ifndef   NDEBUG
	printf( "\n Debug:\n\n");
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
	printf( "  -gl_setgammaramp_ok <n> %s\n", "Override DbgGlSetGammaRampOk - Default: 0");
	printf( "  -gl_vidmem <n>     %s\n", "Limit Texture cache to <n> MB");
#else
	printf( "  -hwsurface         %s\n", "Use HW Surface");
#endif // OGL

#endif // NDEBUG

	printf( "\n Help:\n\n");
	printf( "  -help, -h, -?, ?   %s\n", "View this help screen");
	printf( "\n\n");
}

void sdl_close()
{
	SDL_Quit();
}

extern fix fixed_frametime;
extern void vfx_set_palette_sub(ubyte *);
#define PROGNAME argv[0]

int main(int argc,char **argv)
{
	int t;

	mem_init();

	error_init(NULL);

	setbuf(stdout, NULL); // unbuffered output via printf

	ReadConfigFile();

	InitArgs( argc,argv );

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

	if (GameArg.SysShowCmdHelp) {
		show_commandline_help();
                set_exit_message("");
		return 0;
	}

	printf("\nType %s -help' for a list of command-line options.\n", PROGNAME);

	cfile_use_alternate_hogdir(GameArg.SysMissionDir);

	select_tmap(GameArg.DbgTexMap);

#ifdef NETWORK
	readbans();
#endif

	if (GameArg.DbgVerbose)
		printf ("%s", TXT_VERBOSE_1);

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

#ifdef NETWORK
	control_invul_time = 0;
#endif

	if (GameArg.DbgVerbose)
		printf( "\n%s\n\n", TXT_INITIALIZING_GRAPHICS);
	if ((t=gr_init( SM_ORIGINAL ))!=0)
		Error(TXT_CANT_INIT_GFX,t);
	// Load the palette stuff. Returns non-zero if error.
	mprintf( (0, "Going into graphics mode..." ));
	gr_set_mode(Game_screen_mode);

	mprintf( (0, "\nInitializing palette system..." ));
	gr_use_palette_table( "PALETTE.256" );
	mprintf( (0, "\nInitializing font system..." ));
	gamefont_init(); // must load after palette data loaded.
	songs_play_song( SONG_TITLE, 1 );

	show_title_screen( "iplogo1.pcx", 1 );
	show_title_screen( "logo.pcx", 1 );
	show_title_screen( "descent.pcx", 1 );

#ifdef SHAREWARE
	bm_init_use_tbl();
#else
#ifdef EDITOR
	if (!GameArg.EdiNoBm)
		bm_init_use_tbl();
	else
		bm_init();
#else
		bm_init();
#endif
#endif

	if (GameArg.DbgNoRun)
		return(0);

	mprintf( (0, "\nInitializing 3d system..." ));
	g3_init();
	mprintf( (0, "\nInitializing texture caching system..." ));
	texmerge_init( 10 );		// 10 cache bitmaps

	mprintf( (0, "\nRunning game...\n" ));
#ifdef SCRIPT
	script_init();
#endif
	set_screen_mode(SCREEN_MENU);

	init_game();
	set_detail_level_parameters(Detail_level);

	Players[Player_num].callsign[0] = '\0';

	key_flush();

	if(GameArg.SysPilot)
	{
		char filename[15];
		int j;
		snprintf(filename, 12, GameArg.SysPilot);
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

	gr_palette_fade_out( NULL, 32, 0 );

	Game_mode = GM_GAME_OVER;

#ifndef SHAREWARE
	t = build_mission_list(0); // This also loads mission 0.
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

			game();

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
