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
 * Functions for accessing arguments.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <SDL_stdinc.h>
#include "physfsx.h"
#include "args.h"
#include "u_mem.h"
#include "strio.h"
#include "strutil.h"
#include "digi.h"
#include "game.h"
#include "gauges.h"
#include "console.h"
#ifdef USE_UDP
#include "net_udp.h"
#endif

#define MAX_ARGS 1000
#if defined(DXX_BUILD_DESCENT_I)
#define INI_FILENAME "d1x.ini"
#elif defined(DXX_BUILD_DESCENT_II)
#define INI_FILENAME "d2x.ini"
#endif

static int Num_args=0;
static char * Args[MAX_ARGS];

struct Arg GameArg;

static int FindArg(const char *const s)
{
	int i;

	for (i=0; i<Num_args; i++ )
		if (! d_stricmp( Args[i], s))
			return i;

	return 0;
}

static void AppendIniArgs(void)
{
	PHYSFS_file *f;
	f = PHYSFSX_openReadBuffered(INI_FILENAME);

	if(f) {
		while(!PHYSFS_eof(f) && Num_args < MAX_ARGS)
		{
			char *line=fgets_unlimited(f);

			static const char separator[] = " ";
			for(char *token = strtok(line, separator); token != NULL; token = strtok(NULL, separator))
				Args[Num_args++] = d_strdup(token);

			d_free(line);
		}
		PHYSFS_close(f);
	}
}

// Utility function to get an integer provided as argument
static int get_int_arg(char *arg_name, int default_value) {
	int t;
	return ((t = FindArg(arg_name)) ? atoi(Args[t+1]) : default_value);

}
// Utility function to get a string provided as argument
static char *get_str_arg(char *arg_name, char *default_value) {
	int t;
	return ((t = FindArg(arg_name)) ? Args[t+1] : default_value);
}

// All FindArg calls should be here to keep the code clean
static void ReadCmdArgs(void)
{
	// System Options

	GameArg.SysShowCmdHelp 		= (FindArg( "-help" ) || FindArg( "-h" ) || FindArg( "-?" ) || FindArg( "?" ));
	GameArg.SysUseNiceFPS 		= !FindArg("-nonicefps");

	GameArg.SysMaxFPS = get_int_arg("-maxfps", MAXIMUM_FPS);
	if (GameArg.SysMaxFPS <= MINIMUM_FPS)
		GameArg.SysMaxFPS = MINIMUM_FPS;
	else if (GameArg.SysMaxFPS > MAXIMUM_FPS)
		GameArg.SysMaxFPS = MAXIMUM_FPS;

	GameArg.SysHogDir = get_str_arg("-hogdir", NULL);
	if (GameArg.SysHogDir == NULL)
		GameArg.SysNoHogDir = FindArg("-nohogdir");

	GameArg.SysUsePlayersDir 	= FindArg("-use_players_dir");
	GameArg.SysLowMem 		= FindArg("-lowmem");
	GameArg.SysPilot 		= get_str_arg("-pilot", NULL);
	GameArg.SysWindow 		= FindArg("-window");
	GameArg.SysNoBorders 		= FindArg("-noborders");
#if defined(DXX_BUILD_DESCENT_I)
	GameArg.SysNoTitles 		= FindArg("-notitles");
#elif defined(DXX_BUILD_DESCENT_II)
	GameArg.SysNoMovies 		= FindArg("-nomovies");
#endif
	GameArg.SysAutoDemo 		= FindArg("-autodemo");

	// Control Options

	GameArg.CtlNoCursor 		= FindArg("-nocursor");
	GameArg.CtlNoMouse 		= FindArg("-nomouse");
	GameArg.CtlNoJoystick 		= FindArg("-nojoystick");
	GameArg.CtlNoStickyKeys		= FindArg("-nostickykeys");
	static char sdl_disable_lock_keys[] = "SDL_DISABLE_LOCK_KEYS=0";
	if (GameArg.CtlNoStickyKeys) // Must happen before SDL_Init!
		sdl_disable_lock_keys[sizeof(sdl_disable_lock_keys) - 1] = '1';
	SDL_putenv(sdl_disable_lock_keys);

	// Sound Options

	GameArg.SndNoSound 		= FindArg("-nosound");
	GameArg.SndNoMusic 		= FindArg("-nomusic");
#if defined(DXX_BUILD_DESCENT_II)
	GameArg.SndDigiSampleRate 	= (FindArg("-sound11k") ? SAMPLE_RATE_11K : SAMPLE_RATE_22K);
#endif

#ifdef USE_SDLMIXER
	GameArg.SndDisableSdlMixer 	= FindArg("-nosdlmixer");
#else
	GameArg.SndDisableSdlMixer	= 1;
#endif


	// Graphics Options

	GameArg.GfxHiresFNTAvailable	= !FindArg("-lowresfont");
#if defined(DXX_BUILD_DESCENT_II)
	GameArg.GfxHiresGFXAvailable	= !FindArg("-lowresgraphics");
	GameArg.GfxMovieHires 		= !FindArg( "-lowresmovies" );
#endif

#ifdef OGL
	// OpenGL Options

	GameArg.OglFixedFont 		= FindArg("-gl_fixedfont");
#endif

	// Multiplayer Options

#ifdef USE_UDP
	GameArg.MplUdpHostAddr		= get_str_arg("-udp_hostaddr", UDP_MANUAL_ADDR_DEFAULT);
	GameArg.MplUdpHostPort		= get_int_arg("-udp_hostport", 0);
	GameArg.MplUdpMyPort		= get_int_arg("-udp_myport", 0);
#ifdef USE_TRACKER
	GameArg.MplTrackerAddr		= get_str_arg("-tracker_hostaddr", TRACKER_ADDR_DEFAULT);
	GameArg.MplTrackerPort		= get_int_arg("-tracker_hostport", TRACKER_PORT_DEFAULT);
#endif
#endif

#if defined(DXX_BUILD_DESCENT_I)
	GameArg.EdiNoBm 		= FindArg("-nobm");
#elif defined(DXX_BUILD_DESCENT_II)
#ifdef EDITOR
	// Editor Options

	GameArg.EdiAutoLoad 		= get_str_arg("-autoload", NULL);
	GameArg.EdiMacData 		= FindArg("-macdata");
	GameArg.EdiSaveHoardData 	= FindArg("-hoarddata");
#endif
#endif

	// Debug Options

	if (FindArg("-debug"))		GameArg.DbgVerbose = CON_DEBUG;
	else if (FindArg("-verbose"))	GameArg.DbgVerbose = CON_VERBOSE;
	else				GameArg.DbgVerbose = CON_NORMAL;

	GameArg.DbgSafelog 		= FindArg("-safelog");
	GameArg.DbgNoRun 		= FindArg("-norun");
	GameArg.DbgRenderStats 		= FindArg("-renderstats");
	GameArg.DbgAltTex 		= get_str_arg("-text", NULL);
	GameArg.DbgTexMap 		= get_str_arg("-tmap", NULL);
	GameArg.DbgShowMemInfo 		= FindArg("-showmeminfo");
	GameArg.DbgUseDoubleBuffer 	= !FindArg("-nodoublebuffer");
	GameArg.DbgBigPig 		= !FindArg("-bigpig");
	GameArg.DbgBpp 			= (FindArg("-16bpp") ? 16 : 32);

#ifdef OGL
	GameArg.DbgAltTexMerge 		= !FindArg("-gl_oldtexmerge");
	GameArg.DbgGlIntensity4Ok 	= get_int_arg("-gl_intensity4_ok", 1);
	GameArg.DbgGlLuminance4Alpha4Ok = get_int_arg("-gl_luminance4_alpha4_ok", 1);
	GameArg.DbgGlRGBA2Ok 		= get_int_arg("-gl_rgba2_ok", 1);
	GameArg.DbgGlReadPixelsOk 	= get_int_arg("-gl_readpixels_ok", 1);
	GameArg.DbgGlGetTexLevelParamOk = get_int_arg("-gl_gettexlevelparam_ok", 1);
#else
	GameArg.DbgSdlHWSurface = FindArg("-hwsurface");
	GameArg.DbgSdlASyncBlit = FindArg("-asyncblit");
#endif
}

void args_exit(void)
{
	int i;
	for (i=0; i< Num_args; i++ )
		d_free(Args[i]);
}

void InitArgs( int argc,char **argv )
{
	int i;

	Num_args=0;

	for (i=0; i<argc; i++ )
		Args[Num_args++] = d_strdup( argv[i] );


	for (i=0; i< Num_args; i++ ) {
		if ( Args[i][0] == '-' )
			d_strlwr( Args[i]  );  // Convert all args to lowercase
	}

	AppendIniArgs();
	ReadCmdArgs();
}
