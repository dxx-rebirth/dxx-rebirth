/* $Id: inferno.c,v 1.33 2002-07-22 22:59:24 btb Exp $ */
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
 * FIXME: put description here
 *
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

#include "pstypes.h"
#include "strutil.h"
#include "console.h"
#include "pa_enabl.h"       //$$POLY_ACC
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
#include "iff.h"
#include "pcx.h"
#include "palette.h"
#include "args.h"
#include "sounds.h"
#include "titles.h"
#include "player.h"
#include "text.h"
#include "ipx.h"
#include "newdemo.h"
#ifdef NETWORK
#include "network.h"
#endif
#include "modem.h"
#include "gamefont.h"
#include "kconfig.h"
#include "mouse.h"
#include "joy.h"
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
#include "compbit.h"
#include "d_io.h"

// #  include "3dfx_des.h"

//added on 9/30/98 by Matt Mueller for selectable automap modes
#include "automap.h"
//end addition -MM

#include "../texmap/scanline.h" //for select_tmap -MM

#if defined(POLY_ACC)
#include "poly_acc.h"
extern int Current_display_mode;        //$$ there's got to be a better way than hacking this.
#endif

#ifdef EDITOR
#include "editor/editor.h"
#include "editor/kdefs.h"
#include "ui.h"
#endif

#ifdef SDL_INPUT
#include <SDL/SDL.h>
#endif

#ifndef SDL_VERSION_ATLEAST
#include "oldsdl.h"
#endif

#include "vers_id.h"

void mem_init(void);
void arch_init(void);
void arch_init_start(void);

//Current version number

ubyte Version_major = 1;		//FULL VERSION
ubyte Version_minor = 2;

static const char desc_id_checksum_str[] = DESC_ID_CHKSUM_TAG "0000";	//4-byte checksum
char desc_id_exit_num = 0;

int Function_mode=FMODE_MENU;		//game or editor?
int Screen_mode=-1;					//game screen or editor screen?

//--unused-- grs_bitmap Inferno_bitmap_title;

int WVIDEO_running=0;		//debugger can set to 1 if running

#ifdef EDITOR
int Inferno_is_800x600_available = 0;
#endif

//--unused-- int Cyberman_installed=0;			// SWIFT device present
ubyte CybermouseActive=0;

int __far descent_critical_error_handler( unsigned deverr, unsigned errcode, unsigned __far * devhdr );

void check_joystick_calibration(void);


//--------------------------------------------------------------------------

extern int piggy_low_memory;


int descent_critical_error = 0;
unsigned descent_critical_deverror = 0;
unsigned descent_critical_errcode = 0;

extern int Network_allow_socket_changes;

extern void vfx_set_palette_sub(ubyte *);

extern int VR_low_res;

extern int Config_vr_type;
extern int Config_vr_resolution;
extern int Config_vr_tracking;
int grd_fades_disabled=1;

#define LINE_LEN	100

//read help from a file & print to screen
void print_commandline_help()
{
	CFILE *ifile;
	int have_binary=0;
	char line[LINE_LEN];

	ifile = cfopen("help.tex","rb");
	if (!ifile) {
		ifile = cfopen("help.txb","rb");
		if (!ifile)
			Error("Cannot load help text file.");
		have_binary = 1;
	}

	while (cfgets(line,LINE_LEN,ifile)) {

		if (have_binary) {
			int i;
			for (i = 0; i < strlen(line) - 1; i++) {
				encode_rotate_left(&(line[i]));
				line[i] = line[i] ^ BITMAP_TBL_XOR;
				encode_rotate_left(&(line[i]));
			}
		}

		if (line[0] == ';')
			continue;		//don't show comments

		printf("%s",line);

	}

	cfclose(ifile);

//	printf( " Diagnostic:\n\n");
//	printf( "  -emul           %s\n", "Certain video cards need this option in order to run game");
//	printf(	"  -ddemul         %s\n", "If -emul doesn't work, use this option");
//	printf( "\n");
#ifdef EDITOR
	printf( " Editor Options:\n\n");
	printf( "  -autoload <file>%s\n", "Autoload a level in the editor");
	printf( "  -hoarddata      %s\n","FIXME: Undocumented");
	printf( "  -macdata        %s\n","FIXME: Undocumented");
//	printf( "  -nobm           %s\n","FIXME: Undocumented");
	printf( "\n");
#endif
	printf( " D2X Options:\n\n");
	printf( "  -noredundancy   %s\n", "Do not send messages when picking up redundant items in multi");
	printf( "  -shortpackets   %s\n", "Set shortpackets to default as on");
	printf( "  -notitles       %s\n", "Do not show titlescreens on startup");
	printf( "  -ini <file>     %s\n", "option file (alternate to command line) defaults to d2x.ini, or d1x.ini");
	printf( "  -autodemo       %s\n", "Start in demo mode");
	printf( "  -bigpig         %s\n","FIXME: Undocumented");
	printf( "  -bspgen         %s\n","FIXME: Undocumented");
//	printf( "  -cdproxy        %s\n","FIXME: Undocumented");
#ifndef NDEBUG
	printf( "  -checktime      %s\n","FIXME: Undocumented");
	printf( "  -showmeminfo    %s\n","FIXME: Undocumented");
#endif
//	printf( "  -codereadonly   %s\n","FIXME: Undocumented");
//	printf( "  -cyberimpact    %s\n","FIXME: Undocumented");
	printf( "  -debug          %s\n","Enable very verbose output");
//	printf( "  -debugmode      %s\n","FIXME: Undocumented");
//	printf( "  -disallowgfx    %s\n","FIXME: Undocumented");
//	printf( "  -disallowreboot %s\n","FIXME: Undocumented");
//	printf( "  -dynamicsockets %s\n","FIXME: Undocumented");
//	printf( "  -forcegfx       %s\n","FIXME: Undocumented");
#ifdef SDL_INPUT
#if SDL_VERSION_ATLEAST(1,0,2)
	printf( "  -grabmouse      %s\n","Keeps the mouse from wandering out of the window");
#endif
#endif
//	printf( "  -hw_3dacc       %s\n","FIXME: Undocumented");
#ifndef RELEASE
	printf( "  -invulnerability %s\n","Make yourself invulnerable");
#endif
	printf( "  -ipxnetwork <num> %s\n","Use IPX network number <num>");
	printf( "  -jasen          %s\n","FIXME: Undocumented");
	printf( "  -joyslow        %s\n","FIXME: Undocumented");
//	printf( "  -logfile        %s\n","FIXME: Undocumented");
//	printf( "  -lowresmovies   %s\n","FIXME: Undocumented");
//	printf( "  -memdbg         %s\n","FIXME: Undocumented");
//	printf( "  -monodebug      %s\n","FIXME: Undocumented");
	printf( "  -nocdrom        %s\n","FIXME: Undocumented");
#ifdef __DJGPP__
	printf( "  -nocyberman     %s\n","FIXME: Undocumented");
#endif
	printf( "  -nofade         %s\n","Disable fades");
#ifdef NETWORK
	printf( "  -nomatrixcheat  %s\n","FIXME: Undocumented");
	printf( "  -norankings     %s\n","Disable multiplayer ranking system");
	printf( "  -packets <num>  %s\n","Specifies the number of packets per second\n");
//	printf( "  -showaddress    %s\n","FIXME: Undocumented");
	printf( "  -socket         %s\n","FIXME: Undocumented");
#endif
#if !defined(MACINTOSH) && !defined(WINDOWS)
	printf( "  -nomixer        %s\n","Don't crank music volume");
	printf( "  -superhires     %s\n","Allow higher-resolution modes");
#endif
//	printf( "  -nomodex        %s\n","FIXME: Undocumented");
#ifndef RELEASE
	printf( "  -nomovies       %s\n","Don't play movies");
	printf( "  -noscreens      %s\n","Skip briefing screens");
#endif
#if !defined(SHAREWARE) || ( defined(SHAREWARE) && defined(APPLE_DEMO) )
	printf( "  -noredbook      %s\n","Disable redbook audio");
#endif
	printf( "  -norun          %s\n","Bail out after initialization");
//	printf( "  -ordinaljoy     %s\n","FIXME: Undocumented");
//	printf( "  -rtscts         %s\n","Same as -ctsrts");
//	printf( "  -semiwin        %s\n","Use non-fullscreen mode");
//	printf( "  -specialdevice  %s\n","FIXME: Undocumented");
#ifdef TACTILE
	printf( "  -stickmag       %s\n","FIXME: Undocumented");
#endif
//	printf( "  -stopwatch      %s\n","FIXME: Undocumented");
	printf( "  -subtitles      %s\n","Turn on movie subtitles (English-only)");
//	printf( "  -sysram         %s\n","FIXME: Undocumented");
	printf( "  -text <file>    %s\n","Specify alternate .tex file");
//	printf( "  -tsengdebug1    %s\n","FIXME: Undocumented");
//	printf( "  -tsengdebug2    %s\n","FIXME: Undocumented");
//	printf( "  -tsengdebug3    %s\n","FIXME: Undocumented");
//	printf( "  -udp            %s\n","FIXME: Undocumented");
//	printf( "  -vidram         %s\n","FIXME: Undocumented");
	printf( "  -xcontrol       %s\n","FIXME: Undocumented");
	printf( "  -xname          %s\n","FIXME: Undocumented");
	printf( "  -xver           %s\n","FIXME: Undocumented");
	printf( "  -tmap <t>       %s\n","select texmapper to use (c,fp,i386,pent,ppro)");
	printf( "  -automap<X>x<Y> %s\n","Set automap resolution to <X> by <Y>");
	printf( "  -automap_gameres %s\n","Set automap to use the same resolution as in game");
//	printf( "  -menu<X>x<Y>    %s\n","Set menu resolution to <X> by <Y>");
//	printf( "  -menu_gameres   %s\n","Set menus to use the same resolution as in game");
	printf( "\n");

	printf( "D2X System Options:\n\n");
#ifdef __MSDOS__
	printf( "  -joy209         %s\n", "Use alternate port 209 for joystick");
#endif
#ifdef GR_SUPPORTS_FULLSCREEN_TOGGLE 
	printf( "  -fullscreen     %s\n", "Use fullscreen mode if available");
#endif
#ifdef OGL
	printf( "  -gl_texmagfilt <f> %s\n","set GL_TEXTURE_MAG_FILTER (see readme.d1x)");
	printf( "  -gl_texminfilt <f> %s\n","set GL_TEXTURE_MIN_FILTER (see readme.d1x)");
	printf( "  -gl_mipmap      %s\n","set gl texture filters to \"standard\" options for mipmapping");
	printf( "  -gl_simple      %s\n","set gl texture filters to gl_nearest for \"original\" look. (default)");
	printf( "  -gl_alttexmerge %s\n","use new texmerge, usually uses less ram (default)");
	printf( "  -gl_stdtexmerge %s\n","use old texmerge, uses more ram, but _might_ be a bit faster");
#ifdef GR_SUPPORTS_FULLSCREEN_TOGGLE 
	printf( "  -gl_voodoo      %s\n","force fullscreen mode only");
#endif
	printf( "  -gl_16bittextures %s\n","attempt to use 16bit textures");
	printf( "  -gl_reticle <r> %s\n","use OGL reticle 0=never 1=above 320x* 2=always");
	printf( "  -gl_intensity4_ok %s\n","FIXME: Undocumented");
	printf( "  -gl_luminance4_alpha4_ok %s\n","FIXME: Undocumented");
	printf( "  -gl_readpixels_ok %s\n","FIXME: Undocumented");
	printf( "  -gl_rgba2_ok    %s\n","FIXME: Undocumented");
//	printf( "  -gl_test1       %s\n","FIXME: Undocumented");
	printf( "  -gl_test2       %s\n","FIXME: Undocumented");
	printf( "  -gl_vidmem      %s\n","FIXME: Undocumented");
#ifdef OGL_RUNTIME_LOAD
	printf( "  -gl_library <l> %s\n","use alternate opengl library");
#endif
#endif
#ifdef SDL_VIDEO
	printf( "  -nosdlvidmodecheck %s\n", "Some X servers don't like checking vidmode first, so just switch");
	printf( "  -hwsurface      %s\n","FIXME: Undocumented");
#endif
#ifdef __unix__
	printf( "  -serialdevice <s> %s\n", "Set serial/modem device to <s>");
	printf( "  -serialread <r> %s\n", "Set serial/modem to read from <r>");
#endif
	printf( "\n Help:\n\n");
	printf( "  -help, -h, -?, ? %s\n", "View this help screen");
	printf( "\n");
}

void do_joystick_init()
{
 

	if (!FindArg( "-nojoystick" ))	{
		con_printf(CON_VERBOSE, "\n%s", TXT_VERBOSE_6);
		joy_init();
		if ( FindArg( "-joyslow" ))	{
			con_printf(CON_VERBOSE, "\n%s", TXT_VERBOSE_7);
			joy_set_slow_reading(JOY_SLOW_READINGS);
		}
		if ( FindArg( "-joypolled" ))	{
			con_printf(CON_VERBOSE, "\n%s", TXT_VERBOSE_8);
			joy_set_slow_reading(JOY_POLLED_READINGS);
		}
		if ( FindArg( "-joybios" ))	{
			con_printf(CON_VERBOSE, "\n%s", TXT_VERBOSE_9);
			joy_set_slow_reading(JOY_BIOS_READINGS);
		}

	//	Added from Descent v1.5 by John.  Adapted by Samir.
	} else {
		con_printf(CON_VERBOSE, "\n%s", TXT_VERBOSE_10);
	}
}

//set this to force game to run in low res
int disable_high_res=0;

void do_register_player(ubyte *title_pal)
{
	Players[Player_num].callsign[0] = '\0';

	if (!Auto_demo) 	{

		key_flush();

		//now, before we bring up the register player menu, we need to 
		//do some stuff to make sure the palette is ok.  First, we need to
		//get our current palette into the 2d's array, so the remapping will
		//work.  Second, we need to remap the fonts.  Third, we need to fill 
		//in part of the fade tables so the darkening of the menu edges works

		memcpy(gr_palette,title_pal,sizeof(gr_palette));
		remap_fonts_and_menus(1);
		RegisterPlayer();		//get player's name
	}

}

#ifdef NETWORK
void do_network_init()
{
	if (!FindArg( "-nonetwork" ))	{
		int socket=0, t;
		int ipx_error;

		con_printf(CON_VERBOSE, "\n%s ", TXT_INITIALIZING_NETWORK);
		if ((t=FindArg("-socket")))
			socket = atoi( Args[t+1] );
		//@@if ( FindArg("-showaddress") ) showaddress=1;
		if ((ipx_error=ipx_init(IPX_DEFAULT_SOCKET+socket))==0)	{
  			con_printf(CON_VERBOSE, "%s %d.\n", TXT_IPX_CHANNEL, socket );
			Network_active = 1;
		} else {
			switch( ipx_error )	{
			case 3: 	con_printf(CON_VERBOSE, "%s\n", TXT_NO_NETWORK); break;
			case -2: con_printf(CON_VERBOSE, "%s 0x%x.\n", TXT_SOCKET_ERROR, IPX_DEFAULT_SOCKET+socket); break;
			case -4: con_printf(CON_VERBOSE, "%s\n", TXT_MEMORY_IPX ); break;
			default:
				con_printf(CON_VERBOSE, "%s %d", TXT_ERROR_IPX, ipx_error );
			}
			con_printf(CON_VERBOSE, "%s\n",TXT_NETWORK_DISABLED);
			Network_active = 0;		// Assume no network
		}
		ipx_read_user_file( "descent.usr" );
		ipx_read_network_file( "descent.net" );
		//@@if ( FindArg( "-dynamicsockets" ))
		//@@	Network_allow_socket_changes = 1;
		//@@else
		//@@	Network_allow_socket_changes = 0;
	} else {
		con_printf(CON_VERBOSE, "%s\n", TXT_NETWORK_DISABLED);
		Network_active = 0;		// Assume no network
	}
}
#endif

#define PROGNAME argv[0]

extern char Language[];

//can we do highres menus?
extern int MenuHiresAvailable;

#ifdef D2_OEM
int intro_played = 0;
#endif

int Inferno_verbose = 0;

//added on 11/18/98 by Victor Rachels to add -mission and -startgame
int start_net_immediately = 0;
//int start_with_mission = 0;
//char *start_with_mission_name;
//end this section addition

int open_movie_file(char *filename,int must_have);

#if defined(POLY_ACC)
#define MENU_HIRES_MODE SM_640x480x15xPA
#else
#define MENU_HIRES_MODE SM(640,480)
#endif

//	DESCENT II by Parallax Software
//		Descent Main

//extern ubyte gr_current_pal[];

#ifdef	EDITOR
int	Auto_exit = 0;
char	Auto_file[128] = "";
#endif

int main(int argc,char **argv)
{
	int i,t;
	ubyte title_pal[768];
	int screen_width = 640;
	int screen_height = 480;
	u_int32_t screen_mode = SM(640,480);

	con_init();  // Initialise the console
	mem_init();

	error_init(NULL, NULL);

#ifdef __unix__
	{
		char *home = getenv("HOME");

		if (home) {
			char buf[PATH_MAX + 5];
			
			strcpy(buf, home);
			strcat(buf, "/.d2x");
			if (chdir(buf)) {
				d_mkdir(buf);
				if (chdir(buf))
					fprintf(stderr, "Cannot change to $HOME/.d2x\n");
			}
		}
	}
#endif

	InitArgs( argc,argv );

	if ( FindArg( "-debug") )
	{
		con_threshold.value = (float)2;

	} else
		if ( FindArg( "-verbose" ) ) 
		{
			con_threshold.value = (float)1;
		}

	arch_init_start();

	arch_init();

#ifdef __unix__
	//tell cfile where hogdir is
	cfile_use_alternate_hogdir(SHAREPATH);
#endif

	//tell cfile about our counter
	cfile_set_critical_error_counter_ptr(&descent_critical_error);

	#ifdef SHAREWARE
		cfile_init("d2demo.hog");			//specify name of hogfile
	#else
	#define HOGNAME "descent2.hog"
	if (! cfile_init(HOGNAME)) {		//didn't find HOG.  Check on CD
		#ifdef RELEASE
			Error("Could not find required file <%s>",HOGNAME);
		#endif
	}
	#endif
	
	load_text();

	//print out the banner title
	con_printf(CON_NORMAL, "\nDESCENT 2 %s v%d.%d",VERSION_TYPE,Version_major,Version_minor);
	con_printf(CON_NORMAL, "  %s %s\n", __DATE__,__TIME__);
	con_printf(CON_NORMAL, "%s\n%s\n",TXT_COPYRIGHT,TXT_TRADEMARK);	
	con_printf(CON_NORMAL, "This is a MODIFIED version of Descent 2. Copyright (c) 1999 Peter Hawkins\n");
	con_printf(CON_NORMAL, "                                         Copyright (c) 2002 Bradley Bell\n");


	if (FindArg( "-?" ) || FindArg( "-help" ) || FindArg( "?" ) || FindArg( "-h" ) ) {
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

	con_printf(CON_VERBOSE, "\n%s...", "Checking for Descent 2 CD-ROM");

	if ( FindArg( "-autodemo" ))
		Auto_demo = 1;

#ifndef RELEASE
	if ( FindArg( "-noscreens" ) )
		Skip_briefing_screens = 1;
#endif

	if ((t=FindArg("-tmap"))){
		select_tmap(Args[t+1]);
	}else
		select_tmap(NULL);

	Lighting_on = 1;

//	if (init_graphics()) return 1;

	#ifdef EDITOR
	if (!Inferno_is_800x600_available)	{
		con_printf(CON_NORMAL, "The editor will not be available, press any key to start game...\n" );
		Function_mode = FMODE_MENU;
	}
	#endif

	if (!WVIDEO_running)
		con_printf(CON_DEBUG,"WVIDEO_running = %d\n",WVIDEO_running);

	con_printf (CON_VERBOSE, "%s", TXT_VERBOSE_1);
	ReadConfigFile();

#ifdef NETWORK
	do_network_init();
#endif

#if defined(POLY_ACC)
    Current_display_mode = -1;
    game_init_render_buffers(SM_640x480x15xPA, 640, 480, VR_NONE, VRF_COMPATIBLE_MENUS+VRF_ALLOW_COCKPIT );
#else

	if (!VR_offscreen_buffer)	//if hasn't been initialied (by headset init)
		set_display_mode(0);		//..then set default display mode
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
#define SCREENMODE(X,Y,C) if ( (i=FindArg( "-" #X "x" #Y ))&&(i<argnum))  {argnum=i; screen_mode = SM( X , Y ); con_printf(CON_VERBOSE, "Using " #X "x" #Y " ...\n" );screen_width = X;screen_height = Y;use_double_buffer = 1;screen_compatible = C;}
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
#ifdef __MSDOS__
		if ( FindArg( "-nodoublebuffer" ) )	{
			con_printf(CON_VERBOSE, "Double-buffering disabled...\n" );
#endif
			use_double_buffer = 0;
#ifdef __MSDOS__
		}
#endif
//end addition -MM

		//added 3/24/99 by Owen Evans for screen res changing
//		Game_Screen_mode = screen_mode;
		//end added -OE
		game_init_render_buffers(screen_mode, screen_width, screen_height, vr_mode, screen_compatible);
                
	}
	{
//added/edited on 12/14/98 by Matt Mueller - override res in d1x.ini with command line args
		int i, argnum=INT_MAX;
//added on 9/30/98 by Matt Mueller for selectable automap modes - edited 11/21/99 whee, more fun with defines.
#define SMODE(V,VV,VG,X,Y) if ( (i=FindArg( "-" #V #X "x" #Y )) && (i<argnum))  {argnum=i; VV = SM( X , Y );VG=0;}
#define SMODE_GR(V,VG) if ((i=FindArg("-" #V "_gameres"))){if (i<argnum) VG=1;}
#define SMODE_PRINT(V,VV,VG) if (VG) con_printf(CON_VERBOSE, #V " using game resolution ...\n"); else con_printf(CON_VERBOSE, #V " using %ix%i ...\n",SM_W(VV),SM_H(VV) );
//aren't #defines great? :)
//end addition/edit -MM
#define S_MODE(V,VV,VG) argnum=INT_MAX;SMODE(V,VV,VG,320,200);SMODE(V,VV,VG,320,240);SMODE(V,VV,VG,320,400);SMODE(V,VV,VG,640,400);SMODE(V,VV,VG,640,480);SMODE(V,VV,VG,800,600);SMODE(V,VV,VG,1024,768);SMODE(V,VV,VG,1280,1024);SMODE(V,VV,VG,1600,1200);SMODE_GR(V,VG);SMODE_PRINT(V,VV,VG);

		S_MODE(automap,automap_mode,automap_use_game_res);
//		S_MODE(menu,menu_screen_mode,menu_use_game_res);
	 }
//end addition -MM

	i = FindArg( "-xcontrol" );
	if ( i > 0 )	{
		kconfig_init_external_controls( strtol(Args[i+1], NULL, 0), strtol(Args[i+2], NULL, 0) );
	}

	con_printf(CON_VERBOSE, "\n%s\n\n", TXT_INITIALIZING_GRAPHICS);
	if (FindArg("-nofade"))
		grd_fades_disabled=1;
	
	//determine whether we're using high-res menus & movies
#if !defined(POLY_ACC)
	if (FindArg("-nohires") || FindArg("-nohighres") || (gr_check_mode(MENU_HIRES_MODE) != 0) || disable_high_res)
		MovieHires = MenuHires = MenuHiresAvailable = 0;
	else
#endif
		//NOTE LINK TO ABOVE!
		MenuHires = MenuHiresAvailable = 1;

	if ((t=gr_init())!=0)				//doesn't do much
		Error(TXT_CANT_INIT_GFX,t);

   #ifdef _3DFX
   _3dfx_Init();
   #endif

	// Load the palette stuff. Returns non-zero if error.
	con_printf(CON_DEBUG, "\nInitializing palette system..." );
   gr_use_palette_table(DEFAULT_PALETTE );

	con_printf(CON_DEBUG, "\nInitializing font system..." );
	gamefont_init();	// must load after palette data loaded.

	con_printf( CON_DEBUG, "\nInitializing movie libraries..." );
	init_movies();		//init movie libraries

	con_printf(CON_VERBOSE, "\nGoing into graphics mode...\n");
#if defined(POLY_ACC)
	gr_set_mode(SM_640x480x15xPA);
#else
	gr_set_mode(MovieHires?SM(640,480):SM(320,200));
#endif

	#ifndef RELEASE
	if ( FindArg( "-notitles" ) ) 
		songs_play_song( SONG_TITLE, 1);
	else
	#endif
	{	//NOTE LINK TO ABOVE!
#ifndef SHAREWARE
		int played=MOVIE_NOT_PLAYED;	//default is not played
#endif
		int song_playing = 0;

		#ifdef D2_OEM
		#define MOVIE_REQUIRED 0
		#else
		#define MOVIE_REQUIRED 1
		#endif

#ifdef D2_OEM   //$$POLY_ACC, jay.
		{	//show bundler screens
			FILE *tfile;
			char filename[FILENAME_LEN];

			played=MOVIE_NOT_PLAYED;	//default is not played

            played = PlayMovie("pre_i.mve",0);

			if (!played) {
                strcpy(filename,MenuHires?"pre_i1b.pcx":"pre_i1.pcx");

				while ((tfile=fopen(filename,"rb")) != NULL) {
					fclose(tfile);
					show_title_screen( filename, 1, 0 );
                    filename[5]++;
				}
			}
		}
#endif

		#ifndef SHAREWARE
			init_subtitles("intro.tex");
			played = PlayMovie("intro.mve",MOVIE_REQUIRED);
			close_subtitles();
		#endif

		#ifdef D2_OEM
		if (played != MOVIE_NOT_PLAYED)
			intro_played = 1;
		else {						//didn't get intro movie, try titles

			played = PlayMovie("titles.mve",MOVIE_REQUIRED);

			if (played == MOVIE_NOT_PLAYED) {
#if defined(POLY_ACC)
            gr_set_mode(SM_640x480x15xPA);
#else
				gr_set_mode(MenuHires?SM_640x480V:SM_320x200C);
#endif
				con_printf( CON_DEBUG, "\nPlaying title song..." );
				songs_play_song( SONG_TITLE, 1);
				song_playing = 1;
				con_printf( CON_DEBUG, "\nShowing logo screens..." );
				show_title_screen( MenuHires?"iplogo1b.pcx":"iplogo1.pcx", 1, 1 );
				show_title_screen( MenuHires?"logob.pcx":"logo.pcx", 1, 1 );
			}
		}

		{	//show bundler movie or screens

			FILE *tfile;
			char filename[FILENAME_LEN];
			int movie_handle;

			played=MOVIE_NOT_PLAYED;	//default is not played

			//check if OEM movie exists, so we don't stop the music if it doesn't
			movie_handle = open_movie_file("oem.mve",0);
			if (movie_handle != -1) {
				close(movie_handle);
				played = PlayMovie("oem.mve",0);
				song_playing = 0;		//movie will kill sound
			}

			if (!played) {
				strcpy(filename,MenuHires?"oem1b.pcx":"oem1.pcx");

				while ((tfile=fopen(filename,"rb")) != NULL) {
					fclose(tfile);
					show_title_screen( filename, 1, 0 );
					filename[3]++;
				}
			}
		}
		#endif

		if (!song_playing)
			songs_play_song( SONG_TITLE, 1);
			
	}

   PA_DFX (pa_splash());

	con_printf( CON_DEBUG, "\nShowing loading screen..." );
	{
		//grs_bitmap title_bm;
		int pcx_error;
		char filename[14];

		#ifdef SHAREWARE
		strcpy(filename, "descentd.pcx");
		#else
		#ifdef D2_OEM
		strcpy(filename, MenuHires?"descntob.pcx":"descento.pcx");
		#else
		strcpy(filename, MenuHires?"descentb.pcx":"descent.pcx");
		#endif
		#endif

#if defined(POLY_ACC)
		gr_set_mode(SM_640x480x15xPA);
#else
		gr_set_mode(MenuHires?SM(640,480):SM(320,200));
#endif

		FontHires = MenuHires;

		if ((pcx_error=pcx_read_bitmap( filename, &grd_curcanv->cv_bitmap, grd_curcanv->cv_bitmap.bm_type, title_pal ))==PCX_ERROR_NONE)	{
			//vfx_set_palette_sub( title_pal );
			gr_palette_clear();
			gr_palette_fade_in( title_pal, 32, 0 );
		} else
			Error( "Couldn't load pcx file '%s', PCX load error: %s\n",filename, pcx_errormsg(pcx_error));
	}

	con_printf( CON_DEBUG , "\nDoing bm_init..." );
	#ifdef EDITOR
		bm_init_use_tbl();
	#else
		bm_init();
	#endif

	#ifdef EDITOR
	if (FindArg("-hoarddata") != 0) {
		#define MAX_BITMAPS_PER_BRUSH 30
		grs_bitmap * bm[MAX_BITMAPS_PER_BRUSH];
		grs_bitmap icon;
		int nframes;
		short nframes_short;
		ubyte palette[256*3];
		FILE *ofile;
		int iff_error,i;
		char *sounds[] = {"selforb.raw","selforb.r22",		//SOUND_YOU_GOT_ORB			
								"teamorb.raw","teamorb.r22",		//SOUND_FRIEND_GOT_ORB			
								"enemyorb.raw","enemyorb.r22",	//SOUND_OPPONENT_GOT_ORB	
								"OPSCORE1.raw","OPSCORE1.r22"};	//SOUND_OPPONENT_HAS_SCORED

		ofile = fopen("hoard.ham","wb");

	   iff_error = iff_read_animbrush("orb.abm",bm,MAX_BITMAPS_PER_BRUSH,&nframes,palette);
		Assert(iff_error == IFF_NO_ERROR);
		nframes_short = nframes;
		fwrite(&nframes_short,sizeof(nframes_short),1,ofile);
		fwrite(&bm[0]->bm_w,sizeof(short),1,ofile);
		fwrite(&bm[0]->bm_h,sizeof(short),1,ofile);
		fwrite(palette,3,256,ofile);
		for (i=0;i<nframes;i++)
			fwrite(bm[i]->bm_data,1,bm[i]->bm_w*bm[i]->bm_h,ofile);

	   iff_error = iff_read_animbrush("orbgoal.abm",bm,MAX_BITMAPS_PER_BRUSH,&nframes,palette);
		Assert(iff_error == IFF_NO_ERROR);
		Assert(bm[0]->bm_w == 64 && bm[0]->bm_h == 64);
		nframes_short = nframes;
		fwrite(&nframes_short,sizeof(nframes_short),1,ofile);
		fwrite(palette,3,256,ofile);
		for (i=0;i<nframes;i++)
			fwrite(bm[i]->bm_data,1,bm[i]->bm_w*bm[i]->bm_h,ofile);

		for (i=0;i<2;i++) {
			iff_error = iff_read_bitmap(i?"orbb.bbm":"orb.bbm",&icon,BM_LINEAR,palette);
			Assert(iff_error == IFF_NO_ERROR);
			fwrite(&icon.bm_w,sizeof(short),1,ofile);
			fwrite(&icon.bm_h,sizeof(short),1,ofile);
			fwrite(palette,3,256,ofile);
			fwrite(icon.bm_data,1,icon.bm_w*icon.bm_h,ofile);
		}

		for (i=0;i<sizeof(sounds)/sizeof(*sounds);i++) {
			FILE *ifile;
			int size;
			ubyte *buf;
			ifile = fopen(sounds[i],"rb");
			Assert(ifile != NULL);
			size = ffilelength(ifile);
			buf = d_malloc(size);
			fread(buf,1,size,ifile);
			fwrite(&size,sizeof(size),1,ofile);
			fwrite(buf,1,size,ofile);
			d_free(buf);
			fclose(ifile);
		}

		fclose(ofile);

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
		do_register_player(title_pal);

	gr_palette_fade_out( title_pal, 32, 0 );

	Game_mode = GM_GAME_OVER;

	if (Auto_demo)	{
		newdemo_start_playback("descent.dem");		
		if (Newdemo_state == ND_STATE_PLAYBACK )
			Function_mode = FMODE_GAME;
	}

	//do this here because the demo code can do a longjmp when trying to
	//autostart a demo from the main menu, never having gone into the game
	setjmp(LeaveGame);

	while (Function_mode != FMODE_EXIT)
	{
		switch( Function_mode )	{
		case FMODE_MENU:
			set_screen_mode(SCREEN_MENU);
			if ( Auto_demo )	{
				newdemo_start_playback(NULL);		// Randomly pick a file
				if (Newdemo_state != ND_STATE_PLAYBACK)	
					Error("No demo files were found for autodemo mode!");
			} else {
				#ifdef EDITOR
				if (Auto_exit) {
					strcpy((char *)&Level_names[0], Auto_file);
					LoadLevel(1, 1);
					Function_mode = FMODE_EXIT;
					break;
				}
				#endif

				check_joystick_calibration();
				gr_palette_clear();		//I'm not sure why we need this, but we do
				DoMenu();									 	
				#ifdef EDITOR
				if ( Function_mode == FMODE_EDITOR )	{
					create_new_mine();
					SetPlayerFromCurseg();
					load_palette(NULL,1,0);
				}
				#endif
			}
			break;
		case FMODE_GAME:
			#ifdef EDITOR
				keyd_editor_mode = 0;
			#endif

#ifdef SDL_INPUT
#if SDL_VERSION_ATLEAST(1,0,2)
			/* keep the mouse from wandering in SDL */
			if (FindArg("-grabmouse"))
			    SDL_WM_GrabInput(SDL_GRAB_ON);
#endif
#endif

			game();

#ifdef SDL_INPUT
#if SDL_VERSION_ATLEAST(1,0,2)
			/* give control back to the WM */
			if (FindArg("-grabmouse"))
			    SDL_WM_GrabInput(SDL_GRAB_OFF);
#endif
#endif

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

#if 0 /* ????? */
	#ifndef RELEASE
	if (!FindArg( "-notitles" ))
	#endif
#endif

	#ifndef NDEBUG
	if ( FindArg( "-showmeminfo" ) )
		show_mem_info = 1;		// Make memory statistics show
	#endif

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
#if !defined(EDITOR) && (defined(SHAREWARE) || defined(D2_OEM))

	int pcx_error;
	char title_pal[768];
	char	exit_screen[16];

	gr_set_current_canvas( NULL );
	gr_palette_clear();

	key_flush();		

	#ifdef D2_OEM
		strcpy(exit_screen, MenuHires?"ordrd2ob.pcx":"ordrd2o.pcx");
	#else
	#if defined(SHAREWARE)
		strcpy(exit_screen, "orderd2.pcx");
	#else
		strcpy(exit_screen, MenuHires?"warningb.pcx":"warning.pcx");
	#endif
	#endif

	if ((pcx_error=pcx_read_bitmap( exit_screen, &grd_curcanv->cv_bitmap, grd_curcanv->cv_bitmap.bm_type, title_pal ))==PCX_ERROR_NONE) {
		//vfx_set_palette_sub( title_pal );
		gr_palette_fade_in( title_pal, 32, 0 );
		key_getch();
		gr_palette_fade_out( title_pal, 32, 0 );		
	}
	else
		Int3();		//can't load order screen

	key_flush();		

#endif
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
