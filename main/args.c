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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "u_mem.h"
#include "strio.h"
#include "strutil.h"
#include "args.h"
#include "game.h"
#include "gauges.h"

#ifdef OGL
#if defined(__APPLE__) && defined(__MACH__)
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif
#endif

#define MAX_ARGS 1000
#define INI_FILENAME "d1x.ini"

int Num_args=0;
char * Args[MAX_ARGS];

struct Arg GameArg;

int FindArg( char * s )	{
	int i;

#ifndef NDEBUG
	printf("FindArg: %s\n",s);
#endif

	for (i=0; i<Num_args; i++ )
		if (! strcasecmp( Args[i], s))
			return i;

	return 0;
}

int FindResArg(char *prefix, int *sw, int *sh)
{
	int i;
	int w, h;
	char *endptr;
	int prefixlen = strlen(prefix);

	for (i = 0; i < Num_args; ++i)
		if (Args[i][0] == '-' && !strnicmp(Args[i] + 1, prefix, prefixlen))
		{
			w = strtol(Args[i] + 1 + prefixlen, &endptr, 10);
			if (w > 0 && endptr && endptr[0] == 'x')
			{
				h = strtol(endptr + 1, &endptr, 10);
				if (h > 0 && endptr[0] == '\0')
				{
					*sw = w;
					*sh = h;
					return i;
				}
			}
		}

	return 0;

}

void AppendIniArgs(void)
{
	FILE *f;
	char *line,*word;

	f=fopen(INI_FILENAME,"rt");
	
	if(f) {
		while(!feof(f) && Num_args < MAX_ARGS)
		{
			line=fsplitword(f,'\n');
			word=splitword(line,' ');
			
			Args[Num_args++] = strdup(word);
			
			if(line)
				Args[Num_args++] = strdup(line);
			
			free(line); free(word);
		}
		fclose(f);
	}
}

// Utility function to get an integer provided as argument
int get_int_arg(char *arg_name, int default_value) {
	int t;
	return ((t = FindArg(arg_name)) ? atoi(Args[t+1]) : default_value);

}
// Utility function to get a string provided as argument
char *get_str_arg(char *arg_name, char *default_value) {
	int t;
	return ((t = FindArg(arg_name)) ? Args[t+1] : default_value);
}

// All FindArg calls should be here to keep the code clean
void ReadCmdArgs(void)
{
	int t = 0, x = 0, y = 0;

	// System Options

	GameArg.SysShowCmdHelp 		= (FindArg( "-help" ) || FindArg( "-h" ) || FindArg( "-?" ) || FindArg( "?" ));
	GameArg.SysFPSIndicator 	= FindArg("-fps");
	GameArg.SysUseNiceFPS 		= FindArg("-nicefps");

	GameArg.SysMaxFPS = get_int_arg("-maxfps", MAXIMUM_FPS);
	if (GameArg.SysMaxFPS <= 0 || GameArg.SysMaxFPS > MAXIMUM_FPS)
		GameArg.SysMaxFPS = MAXIMUM_FPS;

	GameArg.SysMissionDir 		= get_str_arg("-missiondir", DESCENT_DATA_PATH "missions/");
	GameArg.SysUsePlayersDir 	= FindArg("-use_players_dir");
	GameArg.SysLowMem 		= FindArg("-lowmem");
	GameArg.SysLegacyHomers 	= FindArg("-legacyhomers");
	GameArg.SysPilot 		= get_str_arg("-pilot", NULL);
	GameArg.SysWindow 		= FindArg("-window");
	GameArg.SysNoTitles 		= FindArg("-notitles");
	GameArg.SysAutoDemo 		= FindArg("-autodemo");

	// Control Options

	GameArg.CtlNoMouse 		= FindArg("-nomouse");
	GameArg.CtlNoJoystick 		= FindArg("-nojoystick");
	GameArg.CtlMouselook 		= FindArg("-mouselook");
	GameArg.CtlGrabMouse 		= FindArg("-grabmouse");

	// Sound Options

	GameArg.SndNoSound 		= FindArg("-nosound");
	GameArg.SndNoMusic 		= FindArg("-nomusic");

#ifdef USE_SDLMIXER
	GameArg.SndSdlMixer 		= FindArg("-sdlmixer");
	GameArg.SndJukebox 		= get_str_arg("-jukebox", NULL);
	GameArg.SndExternalMusic 	= get_str_arg("-music_ext", NULL);
#endif

	// Graphics Options

	if ((t=FindResArg("aspect", &y, &x)))
	{
		GameArg.GfxAspectY = y;
		GameArg.GfxAspectX = x;
	}
	else
	{
		GameArg.GfxAspectY = 4;
		GameArg.GfxAspectX = 3;
	}

	GameArg.GfxGaugeHudMode = get_int_arg("-hud", 0);
	if (GameArg.GfxGaugeHudMode <= 0 || GameArg.GfxGaugeHudMode > GAUGE_HUD_NUMMODES-1)
		GameArg.GfxGaugeHudMode = 0;

	GameArg.GfxHudMaxNumDisp = get_int_arg("-hudlines", 3);
	if (GameArg.GfxHudMaxNumDisp <= 0 || GameArg.GfxHudMaxNumDisp > HUD_MAX_NUM)
		GameArg.GfxHudMaxNumDisp = 3;

	GameArg.GfxPersistentDebris 	= FindArg("-persistentdebris");
	GameArg.GfxUseHiresFont		= FindArg("-hiresfont" );
	GameArg.GfxNoReticle	 	= FindArg("-noreticle");

#ifdef OGL
	// OpenGL Options

	if (FindArg("-gl_trilinear"))
	{
		GameArg.OglTexMagFilt = GL_LINEAR;
		GameArg.OglTexMinFilt = GL_LINEAR_MIPMAP_LINEAR;
	}
	else if (FindArg("-gl_mipmap"))
	{
		GameArg.OglTexMagFilt = GL_LINEAR;
		GameArg.OglTexMinFilt = GL_LINEAR_MIPMAP_NEAREST;
	}
	else
	{
		GameArg.OglTexMagFilt = GL_NEAREST;
		GameArg.OglTexMinFilt = GL_NEAREST;
	}

	GameArg.OglAlphaEffects 	= FindArg("-gl_transparency");
	GameArg.OglVoodooHack 		= FindArg("-gl_voodoo");
	GameArg.OglFixedFont 		= FindArg("-gl_fixedfont");
	GameArg.OglReticle		= get_int_arg("-gl_reticle", 0);
	GameArg.OglPrShot		= FindArg("-gl_prshot");

#endif

	// Multiplayer Options

	GameArg.MplGameProfile 		= FindArg("-mprofile");
	GameArg.MplNoRedundancy 	= FindArg("-noredundancy");
	GameArg.MplPlayerMessages 	= FindArg("-playermessages");
	GameArg.MplIpxNetwork 		= get_str_arg("-ipxnetwork", NULL);
	GameArg.MplIpHostAddr 		= get_str_arg("-ip_hostaddr", "");
	GameArg.MplIpBasePort 		= get_int_arg("-ip_baseport", 0);
	GameArg.MplIpNoRelay	 	= FindArg("-ip_norelay");

	// Editor Options

	GameArg.EdiNoBm 		= FindArg("-nobm");

	// Debug Options

	GameArg.DbgVerbose 		= FindArg("-verbose");
	GameArg.DbgNoRun 		= FindArg("-norun");
	GameArg.DbgRenderStats 		= FindArg("-renderstats");
	GameArg.DbgAltTex 		= get_str_arg("-text", NULL);
	GameArg.DbgTexMap 		= get_str_arg("-tmap", NULL);
	GameArg.DbgShowMemInfo 		= FindArg("-showmeminfo");
	GameArg.DbgUseDoubleBuffer 	= !FindArg("-nodoublebuffer");
	GameArg.DbgBigPig 		= !FindArg("-bigpig");

#ifdef OGL
	GameArg.DbgAltTexMerge 		= !FindArg("-gl_oldtexmerge");
	GameArg.DbgGlBpp 		= (FindArg("-gl_16bpp") ? 16 : 32);
	GameArg.DbgGlIntensity4Ok 	= get_int_arg("-gl_intensity4_ok", 1);
	GameArg.DbgGlLuminance4Alpha4Ok = get_int_arg("-gl_luminance4_alpha4_ok", 1);
	GameArg.DbgGlRGBA2Ok 		= get_int_arg("-gl_rgba2_ok", 1);
	GameArg.DbgGlReadPixelsOk 	= get_int_arg("-gl_readpixels_ok", 1);
	GameArg.DbgGlGetTexLevelParamOk = get_int_arg("-gl_gettexlevelparam_ok", 1);
#else
	GameArg.DbgSdlHWSurface = FindArg("-hwsurface");
#endif
}

void args_exit(void)
{
	int i;
	for (i=0; i< Num_args; i++ )
		free(Args[i]);
}

void InitArgs( int argc,char **argv )
{
	int i;
	
	Num_args=0;
	
	for (i=0; i<argc; i++ )
		Args[Num_args++] = strdup( argv[i] );

	for (i=0; i< Num_args; i++ ) {
		if ( Args[i][0] == '-' )
			strlwr( Args[i]  );  // Convert all args to lowercase
	}

	AppendIniArgs();
	ReadCmdArgs();

	atexit(args_exit);
}
