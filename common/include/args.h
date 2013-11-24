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

#ifdef __cplusplus

extern void InitArgs(int argc, char **argv);
extern void args_exit();

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
#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
#include "dxxsconf.h"

typedef struct Arg
{
	int SysShowCmdHelp;
	int SysNoNiceFPS;
	int SysMaxFPS;
	int SysNoHogDir;
	const char *SysHogDir;
	int SysUsePlayersDir;
	int SysLowMem;
	const char *SysPilot;
	int SysWindow;
	int SysNoBorders;
	int SysAutoDemo;
#ifdef DXX_BUILD_DESCENT_I
	int SysNoTitles;
#endif
#ifdef DXX_BUILD_DESCENT_II
	int SysNoMovies;
#endif
	int CtlNoCursor;
	int CtlNoMouse;
	int CtlNoJoystick;
	int CtlNoStickyKeys;
	int SndNoSound;
	int SndNoMusic;
#ifdef USE_SDLMIXER
	int SndDisableSdlMixer;
#endif
#ifdef DXX_BUILD_DESCENT_II
	int SndDigiSampleRate;
	int GfxSkipHiresMovie;
	int GfxSkipHiresGFX;
#endif
	int GfxSkipHiresFNT;
#ifdef OGL
	int OglFixedFont;
#endif
	const char *MplUdpHostAddr;
	int MplUdpHostPort;
	int MplUdpMyPort;
#ifdef USE_TRACKER
	const char *MplTrackerAddr;
	int MplTrackerPort;
#endif
#ifdef DXX_BUILD_DESCENT_I
	int EdiNoBm;
#endif
#ifdef DXX_BUILD_DESCENT_II
	const char *EdiAutoLoad;
	int EdiSaveHoardData;
	int EdiMacData; // also used for some read routines in non-editor build
#endif
	int DbgVerbose;
	int DbgSafelog;
	int DbgNoRun;
	int DbgRenderStats;
	const char *DbgAltTex;
	const char *DbgTexMap;
	int DbgShowMemInfo;
	int DbgNoDoubleBuffer;
	int DbgNoCompressPigBitmap;
	int DbgBpp;
#ifdef OGL
	int DbgUseOldTextureMerge;
	int DbgGlIntensity4Ok;
	int DbgGlLuminance4Alpha4Ok;
	int DbgGlRGBA2Ok;
	int DbgGlReadPixelsOk;
	int DbgGlGetTexLevelParamOk;
#else
	int DbgSdlHWSurface;
	int DbgSdlASyncBlit;
#endif
} Arg;

extern struct Arg GameArg;

static inline const char *PLAYER_DIRECTORY_STRING(const char *s, const char *f) __attribute_format_arg(2);
static inline const char *PLAYER_DIRECTORY_STRING(const char *s, const char *f)
{
	return (GameArg.SysUsePlayersDir) ? s : (s + sizeof("Players/") - 1);
}
#define PLAYER_DIRECTORY_STRING(S)	((PLAYER_DIRECTORY_STRING)("Players/" S, S))
#endif

#endif

#endif
