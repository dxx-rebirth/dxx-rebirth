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

#include <span>
#include <type_traits>
#include "dxxsconf.h"
#include <cstdint>

#if DXX_USE_OGL
// GL Sync methods
enum SyncGLMethod : uint8_t {
	SYNC_GL_NONE=0,
	SYNC_GL_FENCE,
	SYNC_GL_FENCE_SLEEP,
	SYNC_GL_FINISH_AFTER_SWAP,
	SYNC_GL_FINISH_BEFORE_SWAP,
	SYNC_GL_AUTO
};

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
#include "dsx-ns.h"
#include "pack.h"

namespace dcx {

enum class sound_sample_rate : uint16_t;
enum class digi_mixer_method : uint8_t;

struct CArg : prohibit_void_ptr<CArg>
{
	bool CtlNoCursor;
	bool CtlNoMouse;
	bool CtlNoStickyKeys;
	bool DbgForbidConsoleGrab;
	bool DbgShowMemInfo;
	bool DbgSafelog;
	bool SysShowCmdHelp;
	bool SysLowMem;
	int8_t SysUsePlayersDir;
	bool SysAutoRecordDemo;
	bool SysWindow;
	bool SysAutoDemo;
	bool GfxSkipHiresFNT;
	bool SndNoSound;
	bool SndNoMusic;
	bool SysNoBorders;
	bool SysNoTitles;
#if DXX_USE_SDLMIXER
	bool SndDisableSdlMixer;
	digi_mixer_method SndMixerMethod;
#else
	static constexpr std::true_type SndDisableSdlMixer{};
#endif
#if DXX_MAX_JOYSTICKS
	bool CtlNoJoystick;
#else
	static constexpr std::true_type CtlNoJoystick{};
#endif
#if DXX_USE_OGL
	bool OglFixedFont;
	SyncGLMethod OglSyncMethod;
	bool OglDarkEdges;
	bool DbgUseOldTextureMerge;
	bool DbgGlIntensity4Ok;
	bool DbgGlReadPixelsOk;
	bool DbgGlGetTexLevelParamOk;
	bool DbgGlLuminance4Alpha4Ok;
	bool DbgGlRGBA2Ok;
#if DXX_USE_STEREOSCOPIC_RENDER
	bool OglStereo;
	uint8_t OglStereoView;
#endif
	unsigned OglSyncWait;
#else
	bool DbgSdlHWSurface;
	bool DbgSdlASyncBlit;
#endif
	bool DbgNoRun;
	bool DbgNoDoubleBuffer;
	bool DbgNoCompressPigBitmap;
	bool DbgRenderStats;
	uint8_t DbgBpp;
	int8_t DbgVerbose;
#if DXX_USE_SHAREPATH
	bool SysNoHogDir;
#endif
	bool SysNoNiceFPS;
	int SysMaxFPS;
	int SysRenderZoomAdjustment;
	uint16_t MplUdpHostPort;
	uint16_t MplUdpMyPort;
#if DXX_USE_TRACKER
	uint16_t MplTrackerPort;
	std::string MplTrackerAddr;
#endif
	std::string SysMissionDir;
	std::string SysHogDir;
	std::string SysPilot;
	std::string SysRecordDemoNameTemplate;
	std::string MplUdpHostAddr;
	std::string DbgAltTex;
#if !DXX_USE_OGL
	std::string DbgTexMap;
#endif
};
extern CArg CGameArg;
}

#ifdef dsx
namespace dsx {
struct Arg : prohibit_void_ptr<Arg>
{
#ifdef DXX_BUILD_DESCENT_I
	bool EdiNoBm;
#endif
#ifdef DXX_BUILD_DESCENT_II
	bool SysNoMovies;
	bool GfxSkipHiresMovie;
	bool GfxSkipHiresGFX;
	sound_sample_rate SndDigiSampleRate;
	std::string EdiAutoLoad;
	bool EdiSaveHoardData;
	bool EdiMacData; // also used for some read routines in non-editor build
#endif
};

extern struct Arg GameArg;

bool InitArgs(std::span<char *> argv);
}

namespace dcx {
#define PLAYER_DIRECTORY_TEXT	"Players/"

__attribute_format_arg(1)
static inline const char *PLAYER_DIRECTORY_STRING(const char *s);
static inline const char *PLAYER_DIRECTORY_STRING(const char *s)
{
	return &s[CGameArg.SysUsePlayersDir + sizeof(PLAYER_DIRECTORY_TEXT) - 1];
}
#define PLAYER_DIRECTORY_STRING(S)	((PLAYER_DIRECTORY_STRING)(PLAYER_DIRECTORY_TEXT S))
}
#endif
