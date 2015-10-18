/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license below.
 * See COPYING.txt for license details.

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

#pragma once

#ifdef __cplusplus
#include <cstdint>

bool InitArgs(int argc, char **argv);

#ifdef OGL
// GL Sync methods
typedef enum {
	SYNC_GL_NONE=0,
	SYNC_GL_FENCE,
	SYNC_GL_FENCE_SLEEP,
	SYNC_GL_FINISH_AFTER_SWAP,
	SYNC_GL_FINISH_BEFORE_SWAP,
	SYNC_GL_AUTO
} SyncGLMethod;

#define OGL_SYNC_METHOD_DEFAULT		SYNC_GL_AUTO
#define OGL_SYNC_WAIT_DEFAULT		2		/* milliseconds */

#endif

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
#include <string>
#include "dxxsconf.h"
#include "pack.h"
#include "compiler-type_traits.h"

struct CArg : prohibit_void_ptr<CArg>
{
	bool CtlNoCursor;
	bool CtlNoMouse;
	bool CtlNoStickyKeys;
	bool DbgForbidConsoleGrab;
	bool DbgShowMemInfo;
	bool DbgSafelog;
	int DbgVerbose;
};

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
struct Arg : prohibit_void_ptr<Arg>
{
	int SysMaxFPS;
	std::string SysHogDir;
	std::string SysMissionDir;
	std::string SysPilot;
	std::string SysRecordDemoNameTemplate;
	bool SysShowCmdHelp;
	bool SysNoNiceFPS;
#if defined(__unix__)
	bool SysNoHogDir;
#endif
	bool SysUsePlayersDir;
	bool SysLowMem;
	bool SysAutoRecordDemo;
	bool SysWindow;
	bool SysNoBorders;
	bool SysNoTitles;
	bool SysAutoDemo;
	bool CtlNoJoystick;
	bool SndNoSound;
	bool SndNoMusic;
#ifdef USE_SDLMIXER
	bool SndDisableSdlMixer;
#else
	static constexpr tt::true_type SndDisableSdlMixer{};
#endif
	bool GfxSkipHiresFNT;
#ifdef DXX_BUILD_DESCENT_I
	bool EdiNoBm;
#endif
#ifdef DXX_BUILD_DESCENT_II
	bool SysNoMovies;
	bool GfxSkipHiresMovie;
	bool GfxSkipHiresGFX;
	int SndDigiSampleRate;
#endif
#ifdef OGL
	bool DbgUseOldTextureMerge;
	bool OglFixedFont;
	SyncGLMethod OglSyncMethod;
	int OglSyncWait;
#endif
	std::string MplUdpHostAddr;
	uint16_t MplUdpHostPort;
	uint16_t MplUdpMyPort;
#ifdef USE_TRACKER
	uint16_t MplTrackerPort;
	std::string MplTrackerAddr;
#endif
#ifdef DXX_BUILD_DESCENT_II
	std::string EdiAutoLoad;
	bool EdiSaveHoardData;
	bool EdiMacData; // also used for some read routines in non-editor build
#endif
	bool DbgNoRun;
	bool DbgRenderStats;
	std::string DbgAltTex;
	std::string DbgTexMap;
	bool DbgNoDoubleBuffer;
	bool DbgNoCompressPigBitmap;
	int DbgBpp;
#ifdef OGL
	int DbgGlIntensity4Ok;
	int DbgGlLuminance4Alpha4Ok;
	int DbgGlRGBA2Ok;
	int DbgGlReadPixelsOk;
	int DbgGlGetTexLevelParamOk;
#else
	int DbgSdlHWSurface;
	int DbgSdlASyncBlit;
#endif
};

extern struct Arg GameArg;
#endif
extern CArg CGameArg;

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
static inline const char *PLAYER_DIRECTORY_STRING(const char *s, const char *f) __attribute_format_arg(2);
static inline const char *PLAYER_DIRECTORY_STRING(const char *s, const char *)
{
	return (GameArg.SysUsePlayersDir) ? s : (s + sizeof("Players/") - 1);
}
#define PLAYER_DIRECTORY_STRING(S)	((PLAYER_DIRECTORY_STRING)("Players/" S, S))
#endif

#endif
