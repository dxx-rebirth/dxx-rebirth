/* $Id: inferno.c,v 1.93 2004-12-01 12:48:13 btb Exp $ */
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

#include <physfs.h>

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
#include "newdemo.h"
#ifdef NETWORK
#include "network.h"
#include "modem.h"
#endif
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

#ifndef __MSDOS__
#include <SDL.h>
#endif

#include "vers_id.h"

void mem_init(void);
void arch_init(void);
void arch_init_start(void);

//Current version number

ubyte Version_major = 1;		//FULL VERSION
ubyte Version_minor = 2;

//static const char desc_id_checksum_str[] = DESC_ID_CHKSUM_TAG "0000"; // 4-byte checksum
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

#ifdef __WATCOMC__
int __far descent_critical_error_handler( unsigned deverr, unsigned errcode, unsigned __far * devhdr );
#endif

void check_joystick_calibration(void);

void show_order_form(void);

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
			Warning("Cannot load help text file.");
		have_binary = 1;
	}

	if (ifile)
	{
        char *end;
        
		while ((end = cfgets(line,LINE_LEN,ifile))) {

            // This is the only use of cfgets that needs the CR
            *end++ = '\n';
			if (have_binary)
				decode_text_line (line);

			if (line[0] == ';')
				continue;		//don't show comments

			printf("%s",line);

		}

		cfclose(ifile);

	}

//	printf( " Diagnostic:\n\n");
//	printf( "  -emul           %s\n", "Certain video cards need this option in order to run game");
//	printf(	"  -ddemul         %s\n", "If -emul doesn't work, use this option");
//	printf( "\n");
#ifdef EDITOR
	printf( " Editor Options:\n\n");
	printf( "  -autoload <file>%s\n", "Autoload a level in the editor");
	printf( "  -hoarddata      %s\n","FIXME: Undocumented");
//	printf( "  -nobm           %s\n","FIXME: Undocumented");
	printf( "\n");
#endif
	printf( " D2X Options:\n\n");
	printf( "  -noredundancy   %s\n", "Do not send messages when picking up redundant items in multi");
	printf( "  -shortpackets   %s\n", "Set shortpackets to default as on");
#ifdef OGL // currently only does anything on ogl build, so don't advertise othewise.
	printf("  -renderstats    %s\n", "Enable renderstats info by default");
#endif
	printf( "  -maxfps <n>     %s\n", "Set maximum framerate (1-100)");
	printf( "  -notitles       %s\n", "Do not show titlescreens on startup");
	printf( "  -hogdir <dir>   %s\n", "set shared data directory to <dir>");
#ifdef __unix__
	printf( "  -nohogdir       %s\n", "don't try to use shared data directory");
	printf( "  -userdir <dir>  %s\n", "set user dir to <dir> instead of $HOME/.d2x");
#endif
	printf( "  -ini <file>     %s\n", "option file (alternate to command line), defaults to d2x.ini");
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
	printf( "  -grabmouse      %s\n","Keeps the mouse from wandering out of the window");
#endif
//	printf( "  -hw_3dacc       %s\n","FIXME: Undocumented");
#ifndef RELEASE
	printf( "  -invulnerability %s\n","Make yourself invulnerable");
#endif
	printf( "  -ipxnetwork <num> %s\n","Use IPX network number <num>");
	printf( "  -jasen          %s\n","FIXME: Undocumented");
	printf( "  -joyslow        %s\n","FIXME: Undocumented");
#ifdef NETWORK
	printf( "  -kali           %s\n","use Kali for networking");
#endif
//	printf( "  -logfile        %s\n","FIXME: Undocumented");
	printf( "  -lowresmovies   %s\n","Play low resolution movies if available (for slow machines)");
#if defined(EDITOR) || !defined(MACDATA)
	printf( "  -macdata        %s\n","Read (and, for editor, write) mac data files (swap colors)");
#endif
//	printf( "  -memdbg         %s\n","FIXME: Undocumented");
//	printf( "  -monodebug      %s\n","FIXME: Undocumented");
	printf( "  -nocdrom        %s\n","FIXME: Undocumented");
#ifdef __DJGPP__
	printf( "  -nocyberman     %s\n","FIXME: Undocumented");
#endif
#ifndef NDEBUG
	printf( "  -nofade         %s\n","Disable fades");
#endif
#ifdef NETWORK
	printf( "  -nomatrixcheat  %s\n","FIXME: Undocumented");
	printf( "  -norankings     %s\n","Disable multiplayer ranking system");
	printf( "  -packets <num>  %s\n","Specifies the number of packets per second\n");
//	printf( "  -showaddress    %s\n","FIXME: Undocumented");
	printf( "  -socket         %s\n","FIXME: Undocumented");
#endif
#if !defined(MACINTOSH) && !defined(WINDOWS)
	printf( "  -nomixer        %s\n","Don't crank music volume");
//	printf( "  -superhires     %s\n","Allow higher-resolution modes");
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
//	printf( "  -vidram         %s\n","FIXME: Undocumented");
	printf( "  -xcontrol       %s\n","FIXME: Undocumented");
	printf( "  -xname          %s\n","FIXME: Undocumented");
	printf( "  -xver           %s\n","FIXME: Undocumented");
	printf( "  -tmap <t>       %s\n","select texmapper to use (c,fp,i386,pent,ppro)");
#ifdef __MSDOS__
	printf( "  -<X>x<Y>        %s\n", "Change screen resolution. Options:");
	printf( "                     320x100;320x200;320x240;320x400;640x400;640x480;800x600;1024x768\n");
#else
	printf( "  -<X>x<Y>        %s\n", "Change screen resolution to <X> by <Y>");
#endif
	printf("  -niceautomap    %s\n", "Free cpu while doing automap");
	printf( "  -automap<X>x<Y> %s\n","Set automap resolution to <X> by <Y>");
	printf( "  -automap_gameres %s\n","Set automap to use the same resolution as in game");
//	printf( "  -menu<X>x<Y>    %s\n","Set menu resolution to <X> by <Y>");
//	printf( "  -menu_gameres   %s\n","Set menus to use the same resolution as in game");
	printf("  -rearviewtime t %s\n", "time holding rearview key to use toggle mode (default 0.0625 seconds)");
	printf( "\n");

	printf( "D2X System Options:\n\n");
#ifdef __MSDOS__
	printf("  -ihaveabrokenmouse %s\n", "try to make mouse work if it is not currently");
	printf( "  -joy209         %s\n", "Use alternate port 209 for joystick");
#endif
#ifdef GR_SUPPORTS_FULLSCREEN_TOGGLE
	printf( "  -fullscreen     %s\n", "Use fullscreen mode if available");
#endif
#ifdef OGL
	printf( "  -gl_texmagfilt <f> %s\n","set GL_TEXTURE_MAG_FILTER (see readme.d1x)");
	printf( "  -gl_texminfilt <f> %s\n","set GL_TEXTURE_MIN_FILTER (see readme.d1x)");
	printf("  -gl_mipmap      %s\n", "set gl texture filters to \"standard\" (bilinear) mipmapping");
	printf("  -gl_trilinear   %s\n", "set gl texture filters to trilinear mipmapping");
	printf( "  -gl_simple      %s\n","set gl texture filters to gl_nearest for \"original\" look. (default)");
	printf("  -gl_anisotropy <f> %s\n", "set maximum degree of anisotropy to <f>");
	printf( "  -gl_alttexmerge %s\n","use new texmerge, usually uses less ram (default)");
	printf( "  -gl_stdtexmerge %s\n","use old texmerge, uses more ram, but _might_ be a bit faster");
#ifdef GR_SUPPORTS_FULLSCREEN_TOGGLE
	printf( "  -gl_voodoo      %s\n","force fullscreen mode only");
#endif
	printf( "  -gl_16bittextures %s\n","attempt to use 16bit textures");
	printf("  -gl_16bpp       %s\n", "attempt to use 16bit screen mode");
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
#ifdef WGL_VIDEO
	printf("  -gl_refresh <r> %s\n", "set refresh rate (in fullscreen mode)");
#endif
#endif
#ifdef SDL_VIDEO
	printf( "  -nosdlvidmodecheck %s\n", "Some X servers don't like checking vidmode first, so just switch");
	printf( "  -hwsurface      %s\n","FIXME: Undocumented");
#endif
#ifdef NETWORK
	printf("  -udp            %s\n", "Specify options for udp/ip:");
	printf("    @<shift>      %s\n", "  shift udp port base offset");
	printf("    =<HOST_LIST>  %s\n", "  broadcast both local and to HOST_LIST");
	printf("    +<HOST_LIST>  %s\n", "  broadcast only to HOST_LIST");
	printf("                  %s\n", "   HOSTS can be any IP or hostname")
		;
	printf("                  %s\n", "   HOSTS can also be in the form of <address>:<shift>");
	printf("                  %s\n", "   separate multiple HOSTS with a ,");
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

#define PROGNAME argv[0]

extern char Language[];

//can we do highres menus?
extern int MenuHiresAvailable;

int intro_played = 0;

int Inferno_verbose = 0;

//added on 11/18/98 by Victor Rachels to add -mission and -startgame
int start_net_immediately = 0;
//int start_with_mission = 0;
//char *start_with_mission_name;
//end this section addition

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

int main(int argc, char *argv[])
{
	int i, t;
	ubyte title_pal[768];

	PHYSFS_init(argv[0]);
	PHYSFS_permitSymbolicLinks(1);

	con_init();  // Initialise the console
	mem_init();

	error_init(NULL, NULL);

	PHYSFS_addToSearchPath(PHYSFS_getBaseDir(), 1);
	InitArgs( argc,argv );

	if ((t = FindArg("-userdir"))
#ifdef __unix__
	 || 1	// or if it's a unix platform
#endif
	 )
	{
		// This stuff below seems overly complicated - brad

		char *path = Args[t+1];
		char fullPath[PATH_MAX + 5];

#ifdef __unix__
		if (!t)
			path = "~/.d2x";
#endif
		PHYSFS_removeFromSearchPath(PHYSFS_getBaseDir());
		
		if (path[0] == '~') // yes, this tilde can be put before non-unix paths.
		{
			const char *home = PHYSFS_getUserDir();
			
			strcpy(fullPath, home);	// prepend home to the path
			path++;
			if (*path == *PHYSFS_getDirSeparator())
				path++;
			strncat(fullPath, path, PATH_MAX + 5 - strlen(home));
		}
		else
			strncpy(fullPath, path, PATH_MAX + 5);
		
		PHYSFS_setWriteDir(fullPath);
		if (!PHYSFS_getWriteDir())  // need to make it
		{
			PHYSFS_mkdir(fullPath);
			PHYSFS_setWriteDir(fullPath);
		}
			
		PHYSFS_addToSearchPath(PHYSFS_getWriteDir(), 1);
		AppendArgs();
	}

	if (!PHYSFS_getWriteDir())
	{
		PHYSFS_setWriteDir(PHYSFS_getBaseDir());
		if (!PHYSFS_getWriteDir())
			Error("can't set write dir\n");
		else
			PHYSFS_addToSearchPath(PHYSFS_getWriteDir(), 0);
	}
	
	if (FindArg("-debug"))
		con_threshold.value = (float)2;
	else if (FindArg("-verbose"))
		con_threshold.value = (float)1;

	//tell cfile where hogdir is
	if ((t=FindArg("-hogdir")))
		PHYSFS_addToSearchPath(Args[t + 1], 1);
#ifdef __unix__
	else if (!FindArg("-nohogdir"))
		PHYSFS_addToSearchPath(SHAREPATH, 1);
#endif

	if (! cfile_init("descent2.hog"))
		if (! cfile_init("d2demo.hog"))
			Warning("Could not find a valid hog file (descent2.hog or d2demo.hog)\nPossible locations are:\n"
#ifdef __unix__
			      "\t$HOME/.d2x\n"
			      "\t" SHAREPATH "\n"
#else
				  "\tCurrent directory\n"
#endif
				  "Or use the -hogdir option to specify an alternate location.");
	load_text();

	//print out the banner title
	con_printf(CON_NORMAL, "\nDESCENT 2 %s v%d.%d",VERSION_TYPE,Version_major,Version_minor);
	#ifdef VERSION_NAME
	con_printf(CON_NORMAL, "  %s", VERSION_NAME);
	#endif
	if (cfexist(MISSION_DIR "d2x.hog"))
		con_printf(CON_NORMAL, "  Vertigo Enhanced");

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

	{
		char **i, **list;

		for (i = PHYSFS_getSearchPath(); *i != NULL; i++)
			con_printf(CON_VERBOSE, "PHYSFS: [%s] is in the search path.\n", *i);

		list = PHYSFS_getCdRomDirs();
		for (i = list; *i != NULL; i++)
			con_printf(CON_VERBOSE, "PHYSFS: cdrom dir [%s] is available.\n", *i);
		PHYSFS_freeList(list);

		list = PHYSFS_enumerateFiles("");
		for (i = list; *i != NULL; i++)
			con_printf(CON_DEBUG, "PHYSFS: * We've got [%s].\n", *i);
		PHYSFS_freeList(list);
	}

	//(re)added Mar 30, 2003 Micah Lieske - Allow use of 22K sound samples again.
	if(FindArg("-sound22k"))
	{
		digi_sample_rate = SAMPLE_RATE_22K;
	}

	if(FindArg("-sound11k"))
	{
		digi_sample_rate = SAMPLE_RATE_11K;
	}

	arch_init_start();

	arch_init();

	//con_printf(CON_VERBOSE, "\n%s...", "Checking for Descent 2 CD-ROM");

	if ((t = FindArg("-rearviewtime")))
	{
		float f = atof(Args[t + 1]);
		Rear_view_leave_time = f * f1_0;
	}
	con_printf(CON_VERBOSE, "Rear_view_leave_time=0x%x (%f sec)\n", Rear_view_leave_time, Rear_view_leave_time / (float)f1_0);

	//added/edited 8/18/98 by Victor Rachels to set maximum fps <= 100
	if ((t = FindArg( "-maxfps" ))) {
		t=atoi(Args[t+1]);
		if (t>0&&t<=80)
			maxfps=t;
	}
	//end addition - Victor Rachels

#ifdef SUPPORTS_NICEFPS
	if (FindArg("-nicefps"))
		use_nice_fps = 1;
	if (FindArg("-niceautomap"))
		nice_automap = 1;
#endif

	if (FindArg("-renderstats"))
		gr_renderstats = 1;

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

	do_joystick_init();

#if defined(POLY_ACC)
    Current_display_mode = -1;
    game_init_render_buffers(SM_640x480x15xPA, 640, 480, VR_NONE, VRF_COMPATIBLE_MENUS+VRF_ALLOW_COCKPIT );
#else

	if (!VR_offscreen_buffer)	//if hasn't been initialied (by headset init)
		set_display_mode(0);		//..then set default display mode
#endif

	{
		int screen_width = 640;
		int screen_height = 480;
		int vr_mode = VR_NONE;
		int screen_flags = VRF_USE_PAGING;

		if (FindResArg("", &screen_width, &screen_height))
		{
			/* stuff below mirrors values from display_mode_info in
			 * menu.c which is used by set_display_mode. In fact,
			 * set_display_mode should probably be rewritten to allow
			 * arbitrary resolutions, and then we get rid of this
			 * stuff here.
			 */
			switch (SM(screen_width, screen_height))
			{
			case SM(320, 200):
			case SM(640, 480):
				screen_flags = VRF_ALLOW_COCKPIT + VRF_COMPATIBLE_MENUS;
				break;
			case SM(320, 400):
				screen_flags = VRF_USE_PAGING;
				break;
			case SM(640, 400):
			case SM(800, 600):
			case SM(1024, 768):
			case SM(1280, 1024):
				screen_flags = VRF_COMPATIBLE_MENUS;
				break;
			}

			con_printf(CON_VERBOSE, "Using %ix%i ...\n", screen_width, screen_height);
		}

// added ifdef on 9/30/98 by Matt Mueller to fix high res in linux
#ifdef __MSDOS__
		if (FindArg("-nodoublebuffer"))
#endif
// end addition -MM
		{
			con_printf(CON_VERBOSE, "Double-buffering disabled...\n");
			screen_flags &= ~VRF_USE_PAGING;
		}

		// added 3/24/99 by Owen Evans for screen res changing
		Game_screen_mode = SM(screen_width, screen_height);
		// end added -OE
		game_init_render_buffers(Game_screen_mode, screen_width, screen_height, vr_mode, screen_flags);

	}
	{
// added/edited on 12/14/98 by Matt Mueller - override res in d1x.ini with command line args
		int i, argnum = INT_MAX, w, h;
// added on 9/30/98 by Matt Mueller for selectable automap modes - edited 11/21/99 whee, more fun with defines. - edited 03/31/02 to use new FindResArg.
#define SMODE(V,VV,VG) if ((i=FindResArg(#V, &w, &h)) && (i < argnum)) { argnum = i; VV = SM(w, h); VG = 0; }
#define SMODE_GR(V,VG) if ((i=FindArg("-" #V "_gameres"))){if (i<argnum) VG=1;}
#define SMODE_PRINT(V,VV,VG) if (VG) con_printf(CON_VERBOSE, #V " using game resolution ...\n"); else con_printf(CON_VERBOSE, #V " using %ix%i ...\n",SM_W(VV),SM_H(VV) );
// aren't #defines great? :)
// end addition/edit -MM
#define S_MODE(V,VV,VG) argnum = INT_MAX; SMODE(V, VV, VG); SMODE_GR(V, VG); SMODE_PRINT(V, VV, VG);

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

	if (FindArg( "-lowresmovies" ))
		MovieHires = 0;

	if ((t=gr_init())!=0)				//doesn't do much
		Error(TXT_CANT_INIT_GFX,t);

   #ifdef _3DFX
   _3dfx_Init();
   #endif

	// Load the palette stuff. Returns non-zero if error.
	con_printf(CON_DEBUG, "Initializing palette system...\n" );
	gr_use_palette_table(DEFAULT_PALETTE );

	con_printf(CON_DEBUG, "Initializing font system...\n" );
	gamefont_init();	// must load after palette data loaded.

	con_printf( CON_DEBUG, "Initializing movie libraries...\n" );
	init_movies();		//init movie libraries

#if 0
	con_printf(CON_VERBOSE, "Going into graphics mode...\n");
#if defined(POLY_ACC)
	gr_set_mode(SM_640x480x15xPA);
#else
	gr_set_mode(MovieHires?SM(640,480):SM(320,200));
#endif
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

		{	//show bundler screens
			char filename[FILENAME_LEN];

			played=MOVIE_NOT_PLAYED;	//default is not played

            played = PlayMovie("pre_i.mve",0);

			if (!played) {
                strcpy(filename,MenuHires?"pre_i1b.pcx":"pre_i1.pcx");

				while (PHYSFS_exists(filename))
				{
					show_title_screen( filename, 1, 0 );
                    filename[5]++;
				}
			}
		}

		#ifndef SHAREWARE
			init_subtitles("intro.tex");
			played = PlayMovie("intro.mve",MOVIE_REQUIRED);
			close_subtitles();
		#endif

		if (played != MOVIE_NOT_PLAYED)
			intro_played = 1;
		else {						//didn't get intro movie, try titles

			played = PlayMovie("titles.mve",MOVIE_REQUIRED);

			if (played == MOVIE_NOT_PLAYED)
			{
				char filename[FILENAME_LEN];

#if defined(POLY_ACC)
				gr_set_mode(SM_640x480x15xPA);
#else
				gr_set_mode(MenuHires?SM(640,480):SM(320,200));
#endif
#ifdef OGL
				set_screen_mode(SCREEN_MENU);
#endif
				con_printf( CON_DEBUG, "\nPlaying title song..." );
				songs_play_song( SONG_TITLE, 1);
				song_playing = 1;
				con_printf( CON_DEBUG, "\nShowing logo screens..." );

				strcpy(filename, MenuHires?"iplogo1b.pcx":"iplogo1.pcx"); // OEM
				if (! cfexist(filename))
					strcpy(filename, "iplogo1.pcx"); // SHAREWARE
				if (! cfexist(filename))
					strcpy(filename, "mplogo.pcx"); // MAC SHAREWARE
				show_title_screen(filename, 1, 1 );

				strcpy(filename, MenuHires?"logob.pcx":"logo.pcx"); // OEM
				if (! cfexist(filename))
					strcpy(filename, "logo.pcx"); // SHAREWARE
				if (! cfexist(filename))
					strcpy(filename, "plogo.pcx"); // MAC SHAREWARE
				show_title_screen(filename, 1, 1 );
			}
		}

		{	//show bundler movie or screens

			char filename[FILENAME_LEN];
			PHYSFS_file *movie_handle;

			played=MOVIE_NOT_PLAYED;	//default is not played

			//check if OEM movie exists, so we don't stop the music if it doesn't
			movie_handle = PHYSFS_openRead("oem.mve");
			if (movie_handle)
			{
				PHYSFS_close(movie_handle);
				played = PlayMovie("oem.mve",0);
				song_playing = 0;		//movie will kill sound
			}

			if (!played) {
				strcpy(filename,MenuHires?"oem1b.pcx":"oem1.pcx");

				while (PHYSFS_exists(filename))
				{
					show_title_screen( filename, 1, 0 );
					filename[3]++;
				}
			}
		}

		if (!song_playing)
			songs_play_song( SONG_TITLE, 1);
			
	}

	PA_DFX (pa_splash());

	con_printf( CON_DEBUG, "\nShowing loading screen..." );
	{
		//grs_bitmap title_bm;
		int pcx_error;
		char filename[14];

		strcpy(filename, MenuHires?"descentb.pcx":"descent.pcx");
		if (! cfexist(filename))
			strcpy(filename, MenuHires?"descntob.pcx":"descento.pcx"); // OEM
		if (! cfexist(filename))
			strcpy(filename, "descentd.pcx"); // SHAREWARE
		if (! cfexist(filename))
			strcpy(filename, "descentb.pcx"); // MAC SHAREWARE

#if defined(POLY_ACC)
		gr_set_mode(SM_640x480x15xPA);
#else
		gr_set_mode(MenuHires?SM(640,480):SM(320,200));
#endif
#ifdef OGL
		set_screen_mode(SCREEN_MENU);
#endif

		FontHires = FontHiresAvailable && MenuHires;

		if ((pcx_error=pcx_read_fullscr( filename, title_pal ))==PCX_ERROR_NONE)	{
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
			/* keep the mouse from wandering in SDL */
			if (FindArg("-grabmouse"))
			    SDL_WM_GrabInput(SDL_GRAB_ON);
#endif

			game();

#ifdef SDL_INPUT
			/* give control back to the WM */
			if (FindArg("-grabmouse"))
			    SDL_WM_GrabInput(SDL_GRAB_OFF);
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

	#ifndef RELEASE
	if (!FindArg( "-notitles" ))
	#endif
		show_order_form();

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
#ifndef EDITOR

	int pcx_error;
	unsigned char title_pal[768];
	char	exit_screen[16];

	gr_set_current_canvas( NULL );
	gr_palette_clear();

	key_flush();

	strcpy(exit_screen, MenuHires?"ordrd2ob.pcx":"ordrd2o.pcx"); // OEM
	if (! cfexist(exit_screen))
		strcpy(exit_screen, MenuHires?"orderd2b.pcx":"orderd2.pcx"); // SHAREWARE, prefer mac if hires
	if (! cfexist(exit_screen))
		strcpy(exit_screen, MenuHires?"orderd2.pcx":"orderd2b.pcx"); // SHAREWARE, have to rescale
	if (! cfexist(exit_screen))
		strcpy(exit_screen, MenuHires?"warningb.pcx":"warning.pcx"); // D1
	if (! cfexist(exit_screen))
		return; // D2 registered

	if ((pcx_error=pcx_read_fullscr( exit_screen, title_pal ))==PCX_ERROR_NONE) {
		//vfx_set_palette_sub( title_pal );
		gr_palette_fade_in( title_pal, 32, 0 );
		gr_update();
		while (!key_inkey() && !mouse_button_state(0)) {} //key_getch();
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
