/* $Id: args.c,v 1.1.1.1 2006/03/17 19:58:51 zicodxx Exp $ */
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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: args.c,v 1.1.1.1 2006/03/17 19:58:51 zicodxx Exp $";
#endif

#include <stdlib.h>
#include <string.h>

#include "physfsx.h"
#include "args.h"
#include "u_mem.h"
#include "strio.h"
#include "strutil.h"
#include "digi.h"

#define MAX_ARGS 200

int Num_args=0;
char * Args[MAX_ARGS];

struct Arg GameArg;

int FindArg(char *s)
{
	int i;

#ifndef NDEBUG
	printf("FindArg: %s\n",s);
#endif

	for (i=0; i<Num_args; i++ )
		if (! stricmp( Args[i], s))
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
	PHYSFS_file *f;
	char *line,*word;
	
	f = PHYSFSX_openReadBuffered("d2x.ini");
	
	if(f) {
		while(!PHYSFS_eof(f) && Num_args < MAX_ARGS)
		{
			line=fgets_unlimited(f);
			word=splitword(line,' ');
			
			Args[Num_args++] = d_strdup(word);
			
			if(line)
				Args[Num_args++] = d_strdup(line);
			
			d_free(line); d_free(word);
		}
		PHYSFS_close(f);
	}
}

// All FindArg calls should be here to keep the code clean
void ReadCmdArgs(void)
{
	int t;

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

	if ((t=FindArg("-hogdir")))
		GameArg.SysHogDir = Args[t+1];
	else
	{
		GameArg.SysHogDir = NULL;
		if (FindArg("-nohogdir"))
			GameArg.SysNoHogDir = 1;
		else
			GameArg.SysNoHogDir = 0;
	}

	if ((t = FindArg("-userdir")))
		GameArg.SysUserDir = Args[t+1];
	else
		GameArg.SysUserDir = NULL;

	if (FindArg("-use_players_dir"))
		GameArg.SysUsePlayersDir = 1;
	else
		GameArg.SysUsePlayersDir = 0;

	if (FindArg("-lowmem"))
		GameArg.SysLowMem = 1;
	else
		GameArg.SysLowMem = 0;

	if (FindArg("-legacyhomers"))
		GameArg.SysLegacyHomers = 1;
	else
		GameArg.SysLegacyHomers = 0;

	if ((t = FindArg( "-pilot" )))
		GameArg.SysPilot = Args[t+1];
	else
		GameArg.SysPilot = NULL;

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

	if (FindArg("-sound11k"))
		GameArg.SndDigiSampleRate = SAMPLE_RATE_11K;
	else
		GameArg.SndDigiSampleRate = SAMPLE_RATE_22K;

	if (FindArg("-redbook"))
		GameArg.SndEnableRedbook = 1;
	else
		GameArg.SndEnableRedbook = 0;

	// Editor Options

	if (FindArg("-macdata"))
		GameArg.EdiMacData = 1;
	else
		GameArg.EdiMacData = 0;
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
			strlwr( Args[i]  );  // Convert all args to lowercase
	}

	AppendIniArgs();
	ReadCmdArgs();

	atexit(args_exit);
}
