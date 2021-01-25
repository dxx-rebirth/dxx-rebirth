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
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

/*
 *
 * prototype definitions for descent.cfg reading/writing
 *
 */

#pragma once

#if defined(DXX_BUILD_DESCENT_I) || defined(DXX_BUILD_DESCENT_II)
#include "player-callsign.h"
#endif

#ifdef __cplusplus
#include "mission.h"
#include "pack.h"
#include "ntstring.h"
#include <array>

namespace dcx {

enum class opengl_texture_filter : uint8_t;

// play-order definitions for custom music
/* These values are written to a file as integers, so they must not be
 * renumbered.
 */
enum class LevelMusicPlayOrder : uint8_t
{
	Continuous,
	Level,
	Random,
};

struct CCfg : prohibit_void_ptr<CCfg>
{
	uint16_t ResolutionX;
	uint16_t ResolutionY;
#if DXX_USE_ADLMIDI
	int ADLMIDI_num_chips = 6;
	/* See common/include/adlmidi_dynamic.h for the symbolic name and for other
	 * values.
	 */
	int ADLMIDI_bank = 31;
	bool ADLMIDI_enabled;
#endif
	bool VSync;
	bool Grabinput;
	bool WindowMode;
	opengl_texture_filter TexFilt;
	bool TexAnisotropy;
	bool Multisample;
	bool FPSIndicator;
	uint8_t GammaLevel;
	LevelMusicPlayOrder CMLevelMusicPlayOrder;
	std::array<int, 2> CMLevelMusicTrack;
	ntstring<MISSION_NAME_LEN> LastMission;
	ntstring<PATH_MAX - 1> CMLevelMusicPath;
	std::array<ntstring<PATH_MAX - 1>, 5> CMMiscMusic;
};

extern struct CCfg CGameCfg;
}

#ifdef dsx
namespace dsx {
struct Cfg : prohibit_void_ptr<Cfg>
{
	int MusicType;
	int AspectX;
	int AspectY;
	uint8_t DigiVolume;
	uint8_t MusicVolume;
	bool ReverseStereo;
	bool OrigTrackOrder;
#ifdef DXX_BUILD_DESCENT_II
	bool MovieSubtitles;
	int MovieTexFilt;
#endif
	callsign_t LastPlayer;
};
extern struct Cfg GameCfg;

//#ifdef USE_SDLMIXER
//#define EXT_MUSIC_ON (GameCfg.SndEnableRedbook || GameCfg.JukeboxOn)
//#else
//#define EXT_MUSIC_ON (GameCfg.SndEnableRedbook)		// JukeboxOn shouldn't do anything if it's not supported
//#endif

extern int ReadConfigFile(void);
extern int WriteConfigFile(void);
}
#endif

#endif
