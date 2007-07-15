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

#define MAX_ARGS 200
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

	if (FindArg("-fps"))
		GameArg.SysFPSIndicator = 1;
	else
		GameArg.SysFPSIndicator = 0;

	if (FindArg("-nonicefps"))
		GameArg.SysUseNiceFPS = 0;
	else
		GameArg.SysUseNiceFPS = 1;

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
