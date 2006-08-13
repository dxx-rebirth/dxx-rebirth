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
#ifndef _MSC_VER
#include <unistd.h>
#endif
#ifdef __MSDOS__
#include <time.h>
#endif
//added on 1/11/99 by DPH for win32
#ifdef __WINDOWS__
#include <windows.h>
#endif
//end this section addition - dph
//added on 12/14/98 by Matt Mueller - override res in d1x.ini with command line args
#include <limits.h>
//end addition -MM
#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif


#include "gr.h"
#include "3d.h"
#include "inferno.h"
#include "error.h"
#include "game.h"
#include "segment.h"		//for Side_to_verts
#include "u_mem.h"
#include "texmerge.h"
#include "menu.h"
#include "digi.h"
#include "args.h"
#include "titles.h"
#include "text.h"
#include "ipx.h"
#include "newdemo.h"
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

//added on 9/30/98 by Matt Mueller for selectable automap modes
#include "automap.h"
//end addition -MM

//added on 11/01/98 by Matt Mueller
#include "hudlog.h"
//end addition -MM
//added on 11/15/98 by Victor Racels to ease cd stuff
#include "cdplay.h"
//end this section addition - VR

//added on 2/3/99 by Victor Rachels to add readbans
#include "ban.h"
//end this section addition - VR

//added 02/07/99 Matt Mueller - for -hud command line
#include "gauges.h"
//end addition -MM

//added 2/9/99 by Victor Rachels for pingstats
#include "pingstat.h"
//end this section addition - VR

//added 3/12/99 by Victor Rachels for faster sporb turning
#include "physics.h"
//end this section addition - VR

//added 6/15/99 by Owen Evans to fix compile warnings
#include "strutil.h"
//end section -OE

//added 11/13/99 by Victor Rachels for alternate sounds
#include "altsound.h"
//end this section addition - VR

#include "../texmap/scanline.h" //for select_tmap -MM

#include "d_delay.h" //for SUPPORTS_NICEFPS

#ifdef __MSDOS__
#include <conio.h>
#else
#define getch() getchar()
#endif

#ifdef EDITOR
#include "editor/editor.h"
#include "editor/kdefs.h"
#endif

#include "vers_id.h"

#ifdef QUICKSTART
#include "playsave.h"
#endif
#ifdef SCRIPT
#include "script.h"
#endif
#ifdef OGL
#include "ogl_init.h"
#endif

#include <SDL/SDL.h>


void check_joystick_calibration();
void show_order_form();

static const char desc_id_checksum_str[] = DESC_ID_CHKSUM;
char desc_id_exit_num = 0;

int Function_mode=FMODE_MENU;		//game or editor?
int Screen_mode=-1;					//game screen or editor screen?

int descent_critical_error = 0;
unsigned int descent_critical_deverror = 0;
unsigned int descent_critical_errcode = 0;

u_int32_t menu_screen_mode=SM(640,480); // ZICO - the way menus should be - old: SM(320,200); // mode used for menus -- jb
int menu_use_game_res=0; // ZICO - NO, D1X shouldn't use game res for menu!!!

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

void show_cmdline_help() {
	printf( "%s\n", TXT_COMMAND_LINE_0 );
	printf( "  -<X>x<Y>       %s\n", "Change screen resolution. Options: 320x100;320x200;320x240;320x400;640x400;640x480;800x600;1024x768");
	printf( "%s\n", TXT_COMMAND_LINE_5 );
	printf( "%s\n", TXT_COMMAND_LINE_6 );
	printf( "%s\n", TXT_COMMAND_LINE_8 );
	printf( "%s\n", TXT_COMMAND_LINE_9);
	printf( "%s\n", TXT_COMMAND_LINE_11);
	printf( "%s\n", TXT_COMMAND_LINE_12);
	printf( "%s\n", TXT_COMMAND_LINE_14);
	printf( "%s\n", TXT_COMMAND_LINE_15);
	printf( "%s\n", TXT_COMMAND_LINE_16);
	printf( "%s\n", TXT_COMMAND_LINE_17);
	printf( "\n%s\n",TXT_PRESS_ANY_KEY3);
	getch();
	printf( "\n");
	printf( " D1X-Rebirth options:\n");
	printf( "  -mprofile <f>   %s\n", "Use multi game profile <f>");
	printf( "  -missiondir <d> %s\n", "Set alternate mission dir to <d>");
	printf( "  -mission <f>    %s\n", "Use mission <f> to start game");
	printf( "  -startnetgame   %s\n", "Start a network game immediately");
	printf( "  -joinnetgame    %s\n", "Skip to join menu screen");
	printf( "  -nobans         %s\n", "Don't use saved bans");
	printf( "  -savebans       %s\n", "Automatically save new bans");
	printf( "  -pingstats      %s\n", "Show pingstats on hud");
	printf( "  -noredundancy   %s\n", "Do not send messages when picking up redundant items in multiplayer");
	printf( "  -playermessages %s\n", "View only messages from other players in multi");
//	printf( "  -shortpackets   %s\n", "Set shortpackets to default as on");
//	printf( "  -pps <n>        %s\n", "Set packets per second to default at <n>");
//	printf( "  -ackmsg         %s\n", "Turn on packet acknowledgement debug msgs");
	printf( "  -pilot <pilot>  %s\n", "Select this pilot automatically");
	printf( "  -cockpit <n>    %s\n", "Set initial cockpit");
	printf( "                  %s\n", "0=full 2=status bar 3=full screen");
	printf( "  -hud <h>        %s\n", "Set hud mode.  0=normal 1-3=new");
	printf( "  -fps            %s\n", "Enable FPS indicator by default");
	printf( "  -demo <f>       %s\n", "Start playing demo <f>");
	printf( "  -maxfps <n>     %s\n", "Set maximum framerate (1-100)");
	printf( "  -notitles       %s\n", "Do not show titlescreens on startup");
	printf( "  -ini <file>     %s\n", "option file (alternate to command line)");
	printf( "  -handicap <n>   %s\n", "Start game with <n> shields. Must be < 100 for multi");
	printf( "  -hudlog         %s\n", "Start hudlog immediatly");
/*
#ifdef SUPPORTS_NICEFPS
        printf( "  -nicefps        %s\n", "Free cpu while waiting for next frame");
        printf( "  -niceautomap    %s\n", "Free cpu while doing automap");
#endif
        printf( "  -automap<X>x<Y> %s\n", "Set automap resolution to <X> by <Y>");
        printf( "  -automap_gameres %s\n", "Set automap to use the same resolution as in game");*/ // ZICO - not really needed anymore
	printf( "  -menu<X>x<Y>    %s\n", "Set menu resolution to <X> by <Y>");
	printf( "  -menu_gameres   %s\n", "Set menus to use the same resolution as in game");
	printf( "  -hudlog_multi   %s\n", "Start hudlog upon entering multiplayer games");
	printf( "  -hudlogdir <d>  %s\n", "Log hud messages in directory <d>");
	printf( "  -hudlines <l>   %s\n", "Number of hud messages to show");
	printf( "  -msgcolorlevel <c> %s\n", "Level of colorization for hud messages (0-3)");
	printf( "  -nocdaudio      %s\n", "Disable cd audio");
	printf( "  -playlist \"...\" %s\n", "Set the cd audio playlist to tracks \"a b c ... f g\"");
	printf( "  -fastext        %s\n", "Fast external control");
	printf( "  -font320 <f>    %s\n", "font to use for res 320x* and above (default font3-1.fnt)");
	printf( "  -font640 <f>    %s\n", "font to use for res 640x* and above (default pc6x8.fnt)");
	printf( "  -font800 <f>    %s\n", "font to use for res 800x* and above");
	printf( "  -font1024 <f>   %s\n", "font to use for res 1024x* and above (default pc8x16.fnt)");
#ifdef OGL
	printf( "  -fixedfont      %s\n", "do not scale fonts to current resolution");
#endif
	printf( "  -hiresfont      %s\n", "use high resolution fonts if available");
	printf( "  -tmap <t>       %s\n","select texmapper to use (c,fp,i386,pent,ppro)");
	printf( "  -mouselook      %s\n","Activate fast mouselook. Works in singleplayer only"); // ZICO - added for mouselook
	printf( "  -grabmouse      %s\n","Keeps the mouse from wandering out of the window"); // ZICO - added for mouse capture
	printf( "\n");
	printf( "\n%s\n",TXT_PRESS_ANY_KEY3);
	getch();
	printf( " System options:\n");
#ifdef GR_SUPPORTS_FULLSCREEN_TOGGLE 
	printf( "  -window         %s\n", "Run the game in a window"); // ZICO - from window to fullscreen
#endif
	printf( "  -aspect <y> <x>  %s\n",  ";use specified aspect - example: -aspect 16 9, -aspect 16 7.5 etc.");
#ifdef OGL
	printf( "  -gl_texmaxfilt <f> %s\n","set GL_TEXTURE_MAX_FILTER (see readme.d1x)");
	printf( "  -gl_texminfilt <f> %s\n","set GL_TEXTURE_MIN_FILTER (see readme.d1x)");
	printf( "  -gl_mipmap      %s\n","set gl texture filters to \"standard\" options for mipmapping");
	printf( "  -gl_trilinear   %s\n","set gl texture filters to trilinear mipmapping");
	printf( "  -gl_simple      %s\n","set gl texture filters to gl_nearest for \"original\" look. (default)");
	printf( "  -gl_alttexmerge %s\n","use new texmerge, usually uses less ram (default)");
	printf( "  -gl_stdtexmerge %s\n","use old texmerge, uses more ram, but _might_ be a bit faster");
	printf( "  -gl_voodoo      %s\n","force fullscreen mode only");
	printf( "  -gl_16bittextures %s\n","attempt to use 16bit textures");
	printf( "  -gl_reticle <r> %s\n","use OGL reticle 0=never 1=above 320x* 2=always");
#ifdef OGL_RUNTIME_LOAD
	printf( "  -gl_library <l> %s\n","use alternate opengl library");
#endif
#ifdef __WINDOWS__
	printf( "  -gl_refresh <r> %s\n","set refresh rate (in fullscreen mode)");
#endif
#endif
#ifdef SDL_VIDEO
	printf( "  -nosdlvidmodecheck %s\n", "Some X servers don't like checking vidmode first, so just switch");
#endif
#ifdef __LINUX__
	printf( "  -serialdevice <s> %s\n", "Set serial/modem device to <s>");
	printf( "  -serialread <r>   %s\n", "Set serial/modem to read from <r>");
#endif
	printf( "\n");
}

extern fix fixed_frametime;
extern int framerate_on;

extern void vfx_set_palette_sub(ubyte *);

int Inferno_verbose = 0;

int start_net_immediately = 0;
int start_with_mission = 0;
char *start_with_mission_name;

int main(int argc,char **argv)
//end this section addition - dph
{
	int i,t;
	char start_demo[13];
	int screen_width = 640;
	int screen_height = 480;
	u_int32_t screen_mode = SM(640,480);

	error_init(NULL);

	setbuf(stdout, NULL);	// unbuffered output via printf
		
	InitArgs( argc,argv );

	if ( FindArg( "-verbose" ) )
		Inferno_verbose = 1;

	#ifndef NDEBUG
        if ( FindArg( "-showmeminfo" ) )
                show_mem_info = 1;              // Make memory statistics show
        #endif

	// Things to initialize before anything else
	arch_init_start();

	load_text();

	printf(DESCENT_VERSION "\n"
	       "This is a MODIFIED version of DESCENT which is NOT supported by Parallax or\n"
	       "Interplay. Use at your own risk!\n");
	printf("Based on: DESCENT   %s\n", VERSION_NAME);
	printf("%s\n%s\n",TXT_COPYRIGHT,TXT_TRADEMARK);

	if (FindArg( "-?" ) || FindArg( "-help" ) || FindArg( "--help" ) || FindArg( "?" ) ) {
		show_cmdline_help();
                set_exit_message("");
		return 0;
	}

	printf("\n%s\n", TXT_HELP);

	if ((t = FindArg( "-altsounds" ))) {
		load_alt_sounds(Args[t+1]);
		atexit(free_alt_sounds);
	}

	if ((t = FindArg( "-missiondir" )))
		cfile_use_alternate_hogdir(Args[t+1]);
	else
		cfile_use_alternate_hogdir(DESCENT_DATA_PATH);

	if ((t=FindArg("-tmap"))){
		select_tmap(Args[t+1]);
	}
	else {
		select_tmap(NULL);
	}

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
	
	if ((t=FindArg("-hudlogdir")))
	hud_log_setdir(Args[t+1]);
	
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

	if((t=FindArg( "-mission" ))) {
		start_with_mission = 1;
		sprintf(start_with_mission_name,"%s",Args[t+1]);
		removeext(start_with_mission_name,start_with_mission_name);
		if(strlen(start_with_mission_name)>8)
			start_with_mission_name[9]=0;
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

	if ( FindArg( "-fps" ))
		framerate_on = 1;

	if ((t = FindArg( "-demo" ))) {
		strncpy(start_demo, Args[t + 1], 12);
		start_demo[12] = 0;
		Auto_demo = 1;
	} 
	else {
		start_demo[0] = 0;
	}

	if ( FindArg( "-autodemo" ))
		Auto_demo = 1;

	#ifndef RELEASE
	if ( FindArg( "-noscreens" ) )
		Skip_briefing_screens = 1;
	#endif

	if (Inferno_verbose) printf ("%s", TXT_VERBOSE_1);
        ReadConfigFile();

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
	if (!FindArg("-noserial"))	{
		serial_active = 1;
	}
	else {
		serial_active = 0;
	}
#endif
	{
//added on 12/14/98 by Matt Mueller - override res in d1x.ini with command line args
	int i, argnum=INT_MAX;
//end addition -MM
	int vr_mode = VR_NONE;
	int screen_compatible = 1;
	int use_double_buffer = 0;

//added/edited on 12/14/98 by Matt Mueller - override res in d1x.ini with command line args
//added on 9/30/98 by Matt Mueller clean up screen mode code, and add higher resolutions
#define SCREENMODE(X,Y,C) if ( (i=FindArg( "-" #X "x" #Y ))&&(i<argnum))  {argnum=i; screen_mode = SM( X , Y );if (Inferno_verbose) printf( "Using " #X "x" #Y " ...\n" );screen_width = X;screen_height = Y;use_double_buffer = 1;screen_compatible = C;}
//aren't #defines great? :)

	SCREENMODE(320,100,0);
	SCREENMODE(320,200,1);
//end addition/edit -MM
	SCREENMODE(320,240,0);
	SCREENMODE(320,400,0);
	SCREENMODE(640,400,0);
	SCREENMODE(640,480,0);
	SCREENMODE(800,600,0);
	SCREENMODE(1024,768,0);
	SCREENMODE(1152,864,0);
	SCREENMODE(1280,960,0);
	SCREENMODE(1280,1024,0);
	SCREENMODE(1600,1200,0);
//end addition -MM
		
//added ifdefs on 9/30/98 by Matt Mueller to fix high res in linux
	use_double_buffer = 0;
//end addition -MM
//added 3/24/99 by Owen Evans for screen res changing
	Game_screen_mode = screen_mode;
//end added -OE
	game_init_render_buffers(screen_mode, screen_width, screen_height, use_double_buffer, vr_mode, screen_compatible);
	}
	{
//added/edited on 12/14/98 by Matt Mueller - override res in d1x.ini with command line args
	int i, argnum=INT_MAX;
//added on 9/30/98 by Matt Mueller for selectable automap modes - edited 11/21/99 whee, more fun with defines.
#define SMODE(V,VV,VG,X,Y) if ( (i=FindArg( "-" #V #X "x" #Y )) && (i<argnum))  {argnum=i; VV = SM( X , Y );VG=0;}
#define SMODE_GR(V,VG) if ((i=FindArg("-" #V "_gameres"))){if (i<argnum) VG=1;}
#define SMODE_PRINT(V,VV,VG) if (Inferno_verbose) { if (VG) printf( #V " using game resolution ...\n"); else printf( #V " using %ix%i ...\n",SM_W(VV),SM_H(VV) ); }
//aren't #defines great? :)
//end addition/edit -MM
#define S_MODE(V,VV,VG) argnum=INT_MAX;SMODE(V,VV,VG,320,200);SMODE(V,VV,VG,320,240);SMODE(V,VV,VG,320,400);SMODE(V,VV,VG,640,400);SMODE(V,VV,VG,640,480);SMODE(V,VV,VG,800,600);SMODE(V,VV,VG,1024,768);SMODE(V,VV,VG,1280,1024);SMODE(V,VV,VG,1600,1200);SMODE_GR(V,VG);SMODE_PRINT(V,VV,VG);

	S_MODE(automap,automap_mode,automap_use_game_res);
	S_MODE(menu,menu_screen_mode,menu_use_game_res);
	}
//end addition -MM
	
#ifdef NETWORK
	control_invul_time = 0;
#endif

	i = FindArg( "-xcontrol" );
	if ( i > 0 ) {
		kconfig_init_external_controls( strtol(Args[i+1], NULL, 0), strtol(Args[i+2], NULL, 0) );
	}

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
		ogl_set_screen_mode();
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
		texmerge_init( 10 );		// if we are low on mem, only use 10 cache bitmaps
	else
		texmerge_init( 9999 );		// otherwise, use as much as possible (its still limited by the #define in texmerge.c, so it won't actually use 9999) -MM
	mprintf( (0, "\nRunning game...\n" ));
#ifdef SCRIPT
	script_init();
#endif
	set_screen_mode(SCREEN_MENU);

	init_game();
	set_detail_level_parameters(Detail_level);

	Players[Player_num].callsign[0] = '\0';
	if (!Auto_demo) 	{
		key_flush();
#ifdef QUICKSTART
		strcpy(Players[Player_num].callsign, config_last_player);
		read_player_file();
		Auto_leveling_on = Default_leveling_on;
		write_player_file();
#else
//added/changed on 10/31/98  by Victor Rachels to add -pilot and exit on esc
                if((i=FindArg("-pilot")))
                 {
                  char filename[15];
                  sprintf(filename,"%.8s.plr",Args[i+1]);
                  strlwr(filename);
                   if(!access(filename,4))
                    {
                     strcpy(Players[Player_num].callsign,Args[i+1]);
                     strupr(Players[Player_num].callsign);
                     read_player_file();
                     Auto_leveling_on = Default_leveling_on;
                     WriteConfigFile();
                    }
                   else          //pilot doesn't exist. get pilot.
                    if(!RegisterPlayer())
                     Function_mode = FMODE_EXIT;
                 }
                else
                 if(!RegisterPlayer())               //get player's name
                  Function_mode = FMODE_EXIT;
//end this section addition - Victor Rachels
#endif
	}

	gr_palette_fade_out( NULL, 32, 0 );

	//kconfig_load_all();

	Game_mode = GM_GAME_OVER;

	if (Auto_demo)	{
		newdemo_start_playback((start_demo[0] ? start_demo : "descent.dem"));
		if (Newdemo_state == ND_STATE_PLAYBACK )
			Function_mode = FMODE_GAME;
	}

#ifndef SHAREWARE
	t = build_mission_list(0);	    // This also loads mission 0.
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
			if ( Auto_demo < 0 ) {
				show_title_screen( "descent.pcx", 3 ); // show w/o fade,keywait
                                RegisterPlayer();               //get player's name
				Auto_demo = 0;
			} else if ( Auto_demo )        {
				if (start_demo[0])
					newdemo_start_playback(start_demo);
				else
					newdemo_start_playback(NULL);		// Randomly pick a file
				if (Newdemo_state != ND_STATE_PLAYBACK)	
					Error("No demo files were found for autodemo mode!");
			} else {
				check_joystick_calibration();
				DoMenu();									 	
#ifdef EDITOR
				if ( Function_mode == FMODE_EDITOR )	{
					create_new_mine();
					SetPlayerFromCurseg();
				}
#endif
			}
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

//killed at 7/11/99 by adb - first run all atexit()'s
//--killed #if defined(_MSC_VER) && defined(_DEBUG)
//--killed	_CrtDumpMemoryLeaks();
//--killed #endif
//end changes - adb

#ifdef __WINDOWS__
	digi_stop_current_song(); // ZICO - stop all midis // hack for some onboard soundcards
#endif
	digi_close(); // ZICO - stop all sounds // hack for some onboard soundcards

	return(0);		//presumably successful exit

}


void check_joystick_calibration()	{
	int x1, y1, x2, y2, c;
	fix t1;

	if ( (Config_control_type!=CONTROL_JOYSTICK) &&
		  (Config_control_type!=CONTROL_FLIGHTSTICK_PRO) &&
		  (Config_control_type!=CONTROL_THRUSTMASTER_FCS) &&
		  (Config_control_type!=CONTROL_GRAVIS_GAMEPAD)
		) return;

	joy_get_pos( &x1, &y1 );

	t1 = timer_get_fixed_seconds();
	while( timer_get_fixed_seconds() < t1 + F1_0/100 )
		;

	joy_get_pos( &x2, &y2 );

	// If joystick hasn't moved...
	if ( (abs(x2-x1)<30) &&  (abs(y2-y1)<30) )	{
		if ( (abs(x1)>30) || (abs(x2)>30) ||  (abs(y1)>30) || (abs(y2)>30) )	{
			c = nm_messagebox( NULL, 2, TXT_CALIBRATE, TXT_SKIP, TXT_JOYSTICK_NOT_CEN );
			if ( c==0 )	{
				joydefs_calibrate();
			}
		}
	}

}

void show_order_form()
{
	show_title_screen(
#ifdef SHAREWARE
	"order01.pcx",
#else
	"warning.pcx",
#endif
	1);
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
