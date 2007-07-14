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
 * Prototypes for accessing arguments.
 *
 */



#ifndef _ARGS_H
#define _ARGS_H

extern int Num_args;
extern char * Args[];
extern int FindArg( char * s );
extern int FindResArg(char *prefix, int *sw, int *sh);
extern void InitArgs( int argc, char **argv );
extern int Inferno_verbose;

// Struct that keeps all variables used by FindArg
//   Sys - System Options
//   Ctl - Control Options
//   Snd - Sound Options
//   Gfx - Graphics Options
//   Ogl - OpenGL Options
//   Mpl - Multiplayer Options
//   Dbg - Debugging/Undocumented Options
typedef struct Arg
{
	int SysFPSIndicator;
	int SysUseNiceFPS;
	int SysMaxFPS;
	char *SysMissionDir;
	int SysLowMem;
	int SysLegacyHomers;
	char *SysPilot;
	int SysAutoDemo;
	int CtlMouselook;
	int CtlGrabMouse;
} __attribute__ ((packed)) Arg;

extern struct Arg GameArg;

#endif
