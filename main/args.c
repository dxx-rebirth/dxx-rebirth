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
#include <GL/glu.h>
#endif

#define MAX_ARGS 1000
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

	f=fopen("d1x.ini","rt");
	
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

// All FindArg calls should be here to keep the code clean
void ReadCmdArgs(void)
{
	int t = 0, x = 0, y = 0;

	// System Options

	if (FindArg( "-help" ) || FindArg( "-h" ) || FindArg( "-?" ) || FindArg( "?" ))
		GameArg.SysShowCmdHelp = 1;
	else
		GameArg.SysShowCmdHelp = 0;

	if (FindArg("-fps"))
		GameArg.SysFPSIndicator = 1;
	else
		GameArg.SysFPSIndicator = 0;

	if (FindArg("-nicefps"))
		GameArg.SysUseNiceFPS = 1;
	else
		GameArg.SysUseNiceFPS = 0;

	if ((t = FindArg("-maxfps"))) {
		t=atoi(Args[t+1]);
		if (t>0&&t<=80)
			GameArg.SysMaxFPS=t;
		else
			GameArg.SysMaxFPS=80;
	}
	else
		GameArg.SysMaxFPS=80;

	if ((t = FindArg("-missiondir")))
		GameArg.SysMissionDir = Args[t+1];
	else
		GameArg.SysMissionDir = DESCENT_DATA_PATH "missions/";

	if (FindArg("-lowmem"))
		GameArg.SysLowMem = 1;
	else
		GameArg.SysLowMem = 0;

	if (FindArg("-legacyhomers"))
		GameArg.SysLegacyHomers = 1;
	else
		GameArg.SysLegacyHomers = 0;

	if ((t = FindArg("-pilot")))
		GameArg.SysPilot = Args[t+1];
	else
		GameArg.SysPilot = NULL;

	if (FindArg("-autodemo"))
		GameArg.SysAutoDemo = 1;
	else
		GameArg.SysAutoDemo = 0;

	if (FindArg("-window"))
		GameArg.SysWindow = 1;
	else
		GameArg.SysWindow = 0;

	// Control Options

	if (FindArg("-nomouse"))
		GameArg.CtlNoMouse = 1;
	else
		GameArg.CtlNoMouse = 0;

	if (FindArg("-nojoystick"))
		GameArg.CtlNoJoystick = 1;
	else
		GameArg.CtlNoJoystick = 0;

	if (FindArg("-mouselook"))
		GameArg.CtlMouselook = 1;
	else
		GameArg.CtlMouselook = 0;

	if (FindArg("-grabmouse"))
		GameArg.CtlGrabMouse = 1;
	else
		GameArg.CtlGrabMouse = 0;

	// Sound Options

	if (FindArg("-nosound"))
		GameArg.SndNoSound = 1;
	else
		GameArg.SndNoSound = 0;

	if (FindArg("-nomusic"))
		GameArg.SndNoMusic = 1;
	else
		GameArg.SndNoMusic = 0;

#ifdef USE_SDLMIXER
	GameArg.SndSdlMixer = FindArg("-sdlmixer");
	GameArg.SndJukebox = (t = FindArg("-jukebox") ? Args[t+1] : NULL);
	GameArg.SndExternalMusic = (t = FindArg("-music_ext") ? Args[t+1] : NULL);
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

	if ((t=FindArg("-hud"))){
		t=atoi(Args[t+1]);
		if(t>=0 && t<GAUGE_HUD_NUMMODES)
			GameArg.GfxGaugeHudMode = t;
		else
			GameArg.GfxGaugeHudMode = 0;
	}
	else
		GameArg.GfxGaugeHudMode = 0;

	if ((t=FindArg("-hudlines")))
	{
		t=atoi(Args[t+1]);
		if(t>0 && t<=HUD_MAX_NUM)
			GameArg.GfxHudMaxNumDisp = t;
		else
			GameArg.GfxHudMaxNumDisp = 3;
	}
	else
		GameArg.GfxHudMaxNumDisp = 3;

	if (FindArg("-hiresfont"))
		GameArg.GfxUseHiresFont = 1;
	else
		GameArg.GfxUseHiresFont = 0;

	if (FindArg("-persistentdebris"))
		GameArg.GfxPersistentDebris = 1;
	else
		GameArg.GfxPersistentDebris = 0;

	if (FindArg("-noreticle"))
		GameArg.GfxNoReticle = 1;
	else
		GameArg.GfxNoReticle = 0;

#ifdef OGL
	// OpenGL Options

	if (FindArg("-gl_mipmap"))
	{
		GameArg.OglTexMagFilt = GL_LINEAR;
		GameArg.OglTexMinFilt = GL_LINEAR_MIPMAP_NEAREST;
	}
	else if (FindArg("-gl_trilinear"))
	{
		GameArg.OglTexMagFilt = GL_LINEAR;
		GameArg.OglTexMinFilt = GL_LINEAR_MIPMAP_LINEAR;
	}
	else
	{
		GameArg.OglTexMagFilt = GL_NEAREST;
		GameArg.OglTexMinFilt = GL_NEAREST;
	}

	if (FindArg("-gl_transparency"))
		GameArg.OglAlphaEffects = 1;
	else
		GameArg.OglAlphaEffects = 0;

	if ((t=FindArg("-gl_reticle")))
		GameArg.OglReticle = atoi(Args[t+1]);
	else
		GameArg.OglReticle = 0;

	if (FindArg("-gl_voodoo"))
		GameArg.OglVoodooHack = 1;
	else
		GameArg.OglVoodooHack = 0;

	if (FindArg("-fixedfont"))
		GameArg.OglFixedFont = 1;
	else
		GameArg.OglFixedFont = 0;
#endif

	// Multiplayer Options

	if (FindArg("-mprofile"))
		GameArg.MplGameProfile = 1;
	else
		GameArg.MplGameProfile = 0;

	if (FindArg("-nobans"))
		GameArg.MplNoBans = 1;
	else
		GameArg.MplNoBans = 0;

	if (FindArg("-savebans"))
		GameArg.MplSaveBans = 1;
	else
		GameArg.MplSaveBans = 0;

	if (FindArg("-noredundancy"))
		GameArg.MplNoRedundancy = 1;
	else
		GameArg.MplNoRedundancy = 0;

	if (FindArg("-playermessages"))
		GameArg.MplPlayerMessages = 1;
	else
		GameArg.MplPlayerMessages = 0;

	if ((t=FindArg("-ipxnetwork")) && Args[t+1])
		GameArg.MplIpxNetwork = Args[t+1];
	else
		GameArg.MplIpxNetwork = NULL;

	if ((t=FindArg("-ipxbasesocket")) && Args[t+1])
		GameArg.MplIPXSocketOffset = atoi(Args[t+1]);
	else
		GameArg.MplIPXSocketOffset = 0;

	if ((t=FindArg("-ip_hostaddr")))
		GameArg.MplIpHostAddr = Args[t+1];
	else
		GameArg.MplIpHostAddr = "";

	if (FindArg("-ip_nogetmyaddr"))
		GameArg.MplIpNoGetMyAddr = 1;
	else
		GameArg.MplIpNoGetMyAddr = 0;

	if ((t=FindArg("-ip_myaddr")))
		GameArg.MplIpMyAddr = Args[t+1];
	else
		GameArg.MplIpMyAddr = NULL;

	if ((t=FindArg("-ip_baseport")))
		GameArg.MplIpBasePort = atoi(Args[t+1]);
	else
		GameArg.MplIpBasePort = 0;

	// Editor Options

	if (FindArg("-nobm"))
		GameArg.EdiNoBm = 1;
	else
		GameArg.EdiNoBm = 0;

	// Debug Options

	if (FindArg("-verbose"))
		GameArg.DbgVerbose = 1;
	else
		GameArg.DbgVerbose = 0;

	if (FindArg("-norun"))
		GameArg.DbgNoRun = 1;
	else
		GameArg.DbgNoRun = 0;

	if (FindArg("-renderstats"))
		GameArg.DbgRenderStats = 1;
	else
		GameArg.DbgRenderStats = 0;

	if ((t=FindArg("-text")))
		GameArg.DbgAltTex = Args[t+1];
	else
		GameArg.DbgAltTex = NULL;

	if ((t=FindArg("-tmap")))
		GameArg.DbgTexMap = Args[t+1];
	else
		GameArg.DbgTexMap = NULL;

	if (FindArg( "-showmeminfo" ))
		GameArg.DbgShowMemInfo = 1;
	else
		GameArg.DbgShowMemInfo = 0;

	if (FindArg("-nodoublebuffer"))
		GameArg.DbgUseDoubleBuffer = 0;
	else
		GameArg.DbgUseDoubleBuffer = 1;

	if (FindArg("-bigpig"))
		GameArg.DbgBigPig = 0;
	else
		GameArg.DbgBigPig = 1;

#ifdef OGL
	if (FindArg("-gl_oldtexmerge"))
		GameArg.DbgAltTexMerge = 0;
	else
		GameArg.DbgAltTexMerge = 1;

	if (FindArg("-gl_16bpp"))
		GameArg.DbgGlBpp = 16;
	else
		GameArg.DbgGlBpp = 32;

	if ((t=FindArg("-gl_intensity4_ok")))
		GameArg.DbgGlIntensity4Ok = atoi(Args[t+1]);
	else
		GameArg.DbgGlIntensity4Ok = 1;

	if ((t=FindArg("-gl_luminance4_alpha4_ok")))
		GameArg.DbgGlLuminance4Alpha4Ok = atoi(Args[t+1]);
	else
		GameArg.DbgGlLuminance4Alpha4Ok = 1;

	if ((t=FindArg("-gl_rgba2_ok")))
		GameArg.DbgGlRGBA2Ok = atoi(Args[t+1]);
	else
		GameArg.DbgGlRGBA2Ok = 1;

	if ((t=FindArg("-gl_readpixels_ok")))
		GameArg.DbgGlReadPixelsOk = atoi(Args[t+1]);
	else
		GameArg.DbgGlReadPixelsOk = 1;

	if ((t=FindArg("-gl_gettexlevelparam_ok")))
		GameArg.DbgGlGetTexLevelParamOk = atoi(Args[t+1]);
	else
		GameArg.DbgGlGetTexLevelParamOk = 1;

	if ((t=FindArg("-gl_setgammaramp_ok")))
		GameArg.DbgGlSetGammaRampOk = atoi(Args[t+1]);
	else
		GameArg.DbgGlSetGammaRampOk = 0;

	if ((t=FindArg("-gl_vidmem")))
		GameArg.DbgGlMemTarget = atoi(Args[t+1])*1024*1024;
	else
		GameArg.DbgGlMemTarget = -1;
#else
	if (FindArg("-hwsurface"))
		GameArg.DbgSdlHWSurface = 1;
	else
		GameArg.DbgSdlHWSurface = 0;
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
