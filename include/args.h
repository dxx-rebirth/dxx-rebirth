/* $Id: args.h,v 1.1.1.1 2006/03/17 20:01:30 zicodxx Exp $ */
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
extern char *Args[];
extern int FindArg(char *s);
extern int FindResArg(char *prefix, int *sw, int *sh);
extern void AppendIniArgs(void);
extern void InitArgs(int argc, char **argv);
extern int Inferno_verbose;

// Struct that keeps all variables used by FindArg
// Prefixes are:
//   Sys - System Options
//   Ctl - Control Options
//   Snd - Sound Options
//   Gfx - Graphics Options
//   Ogl - OpenGL Options
//   Mpl - Multiplayer Options
//   Edi - Editor Options
//   Dbg - Debugging/Undocumented Options
typedef struct Arg
{
	int SysFPSIndicator;
	int SysUseNiceFPS;
	int SysMaxFPS;
	char *SysHogDir;
	int SysNoHogDir;
	char *SysUserDir;
	int SysUsePlayersDir;
	int SysLowMem;
	int SysLegacyHomers;
	char *SysPilot;
	int SysWindow;
	int SysAutoDemo;
	int CtlNoMouse;
	int CtlNoJoystick;
	int CtlMouselook;
	int CtlGrabMouse;
	int SndNoSound;
	int SndNoMusic;
	int SndDigiSampleRate;
	int SndEnableRedbook;
	float GfxAspectX;
	float GfxAspectY;
	int GfxGaugeHudMode;
	int GfxPersistentDebris;
	int GfxMovieHires;
	int GfxMovieSubtitles;
#ifdef OGL
	int OglTexMagFilt;
	int OglTexMinFilt;
	int OglAlphaEffects;
	int OglReticle;
	int OglScissorOk;
	int OglVoodooHack;
	int OglFixedFont;
#endif
#ifdef EDITOR
	int EdiMacData;
#endif
} __attribute__ ((packed)) Arg;

extern struct Arg GameArg;

#endif
