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
 * contains routine(s) to read in the configuration file which contains
 * game configuration stuff like detail level, sound card, etc
 *
 */

#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "config.h"
#include "pstypes.h"
#include "game.h"
#include "songs.h"
#include "kconfig.h"
#include "palette.h"
#include "digi.h"
#include "mission.h"
#include "u_mem.h"
#include "physfsx.h"
#include "nvparse.h"
#if DXX_USE_OGL
#include "ogl_init.h"
#endif
#include <memory>

namespace dcx {
CCfg CGameCfg;
}

namespace dsx {
Cfg GameCfg;

#define DigiVolumeStr "DigiVolume"
#define MusicVolumeStr "MusicVolume"
#define ReverseStereoStr "ReverseStereo"
#define OrigTrackOrderStr "OrigTrackOrder"
#define MusicTypeStr "MusicType"
#define CMLevelMusicPlayOrderStr "CMLevelMusicPlayOrder"
#define CMLevelMusicTrack0Str "CMLevelMusicTrack0"
#define CMLevelMusicTrack1Str "CMLevelMusicTrack1"
#define CMLevelMusicPathStr "CMLevelMusicPath"
#define CMMiscMusic0Str "CMMiscMusic0"
#define CMMiscMusic1Str "CMMiscMusic1"
#define CMMiscMusic2Str "CMMiscMusic2"
#define CMMiscMusic3Str "CMMiscMusic3"
#define CMMiscMusic4Str "CMMiscMusic4"
#define GammaLevelStr "GammaLevel"
#define LastPlayerStr "LastPlayer"
#define LastMissionStr "LastMission"
#define ResolutionXStr "ResolutionX"
#define ResolutionYStr "ResolutionY"
#define AspectXStr "AspectX"
#define AspectYStr "AspectY"
#define WindowModeStr "WindowMode"
#define TexFiltStr "TexFilt"
#define TexAnisStr "TexAnisotropy"
#if defined(DXX_BUILD_DESCENT_II)
#define MovieTexFiltStr "MovieTexFilt"
#define MovieSubtitlesStr "MovieSubtitles"
#endif
#if DXX_USE_ADLMIDI
#define ADLMIDINumChipsStr	"ADLMIDI_NumberOfChips"
#define ADLMIDIBankStr	"ADLMIDI_Bank"
#define ADLMIDIEnabledStr	"ADLMIDI_Enabled"
#endif
#define VSyncStr "VSync"
#define MultisampleStr "Multisample"
#define FPSIndicatorStr "FPSIndicator"
#define GrabinputStr "GrabInput"

int ReadConfigFile()
{
	// set defaults
	GameCfg.DigiVolume = 8;
	GameCfg.MusicVolume = 8;
	GameCfg.ReverseStereo = 0;
	GameCfg.OrigTrackOrder = 0;
#if DXX_USE_SDL_REDBOOK_AUDIO && defined(__APPLE__) && defined(__MACH__)
	GameCfg.MusicType = MUSIC_TYPE_REDBOOK;
#else
	GameCfg.MusicType = MUSIC_TYPE_BUILTIN;
#endif
	CGameCfg.CMLevelMusicPlayOrder = LevelMusicPlayOrder::Continuous;
	CGameCfg.CMLevelMusicTrack[0] = -1;
	CGameCfg.CMLevelMusicTrack[1] = -1;
	CGameCfg.CMLevelMusicPath = {};
	CGameCfg.CMMiscMusic = {};
#if defined(__APPLE__) && defined(__MACH__)
	const auto userdir = PHYSFS_getUserDir();
	GameCfg.OrigTrackOrder = 1;
#if defined(DXX_BUILD_DESCENT_I)
	CGameCfg.CMLevelMusicPlayOrder = LevelMusicPlayOrder::Level;
	CGameCfg.CMLevelMusicPath = "descent.m3u";
	snprintf(CGameCfg.CMMiscMusic[SONG_TITLE].data(), CGameCfg.CMMiscMusic[SONG_TITLE].size(), "%s%s", userdir, "Music/iTunes/iTunes Music/Insanity/Descent/02 Primitive Rage.mp3");
	snprintf(CGameCfg.CMMiscMusic[SONG_CREDITS].data(), CGameCfg.CMMiscMusic[SONG_CREDITS].size(), "%s%s", userdir, "Music/iTunes/iTunes Music/Insanity/Descent/05 The Darkness Of Space.mp3");
#elif defined(DXX_BUILD_DESCENT_II)
	CGameCfg.CMLevelMusicPath = "descent2.m3u";
	snprintf(CGameCfg.CMMiscMusic[SONG_TITLE].data(), CGameCfg.CMMiscMusic[SONG_TITLE].size(), "%s%s", userdir, "Music/iTunes/iTunes Music/Redbook Soundtrack/Descent II, Macintosh CD-ROM/02 Title.mp3");
	snprintf(CGameCfg.CMMiscMusic[SONG_CREDITS].data(), CGameCfg.CMMiscMusic[SONG_CREDITS].size(), "%s%s", userdir, "Music/iTunes/iTunes Music/Redbook Soundtrack/Descent II, Macintosh CD-ROM/03 Crawl.mp3");
#endif
	snprintf(CGameCfg.CMMiscMusic[SONG_BRIEFING].data(), CGameCfg.CMMiscMusic[SONG_BRIEFING].size(), "%s%s", userdir, "Music/iTunes/iTunes Music/Insanity/Descent/03 Outerlimits.mp3");
	snprintf(CGameCfg.CMMiscMusic[SONG_ENDLEVEL].data(), CGameCfg.CMMiscMusic[SONG_ENDLEVEL].size(), "%s%s", userdir, "Music/iTunes/iTunes Music/Insanity/Descent/04 Close Call.mp3");
	snprintf(CGameCfg.CMMiscMusic[SONG_ENDGAME].data(), CGameCfg.CMMiscMusic[SONG_ENDGAME].size(), "%s%s", userdir, "Music/iTunes/iTunes Music/Insanity/Descent/14 Insanity.mp3");
#endif
	CGameCfg.GammaLevel = 0;
	GameCfg.LastPlayer = {};
	CGameCfg.LastMission = "";
	CGameCfg.ResolutionX = 1024;
	CGameCfg.ResolutionY = 768;
	GameCfg.AspectX = 3;
	GameCfg.AspectY = 4;
	CGameCfg.WindowMode = false;
#if DXX_USE_OGL
	CGameCfg.TexFilt = opengl_texture_filter::classic;
	CGameCfg.TexAnisotropy = 0;
#endif
#if defined(DXX_BUILD_DESCENT_II)
#if DXX_USE_OGL
	GameCfg.MovieTexFilt = 0;
#endif
	GameCfg.MovieSubtitles = 0;
#endif
	CGameCfg.VSync = false;
#if DXX_USE_OGL
	CGameCfg.Multisample = 0;
#endif
	CGameCfg.FPSIndicator = 0;
	CGameCfg.Grabinput = true;


	auto infile = PHYSFSX_openReadBuffered("descent.cfg").first;
	if (!infile)
	{
		return 1;
	}

	// to be fully safe, assume the whole cfg consists of one big line
	for (PHYSFSX_gets_line_t<0> line(PHYSFS_fileLength(infile) + 1); const char *const eol = PHYSFSX_fgets(line, infile);)
	{
		const auto lb = line.begin();
		if (eol == line.end())
			continue;
		auto eq = std::find(lb, eol, '=');
		if (eq == eol)
			continue;
		auto value = std::next(eq);
		if (cmp(lb, eq, DigiVolumeStr))
			convert_integer(GameCfg.DigiVolume, value);
		else if (cmp(lb, eq, MusicVolumeStr))
			convert_integer(GameCfg.MusicVolume, value);
		else if (cmp(lb, eq, ReverseStereoStr))
			convert_integer(GameCfg.ReverseStereo, value);
		else if (cmp(lb, eq, OrigTrackOrderStr))
			convert_integer(GameCfg.OrigTrackOrder, value);
		else if (cmp(lb, eq, MusicTypeStr))
			convert_integer(GameCfg.MusicType, value);
		else if (cmp(lb, eq, CMLevelMusicPlayOrderStr))
		{
			if (auto r = convert_integer<uint8_t>(value))
				if (auto CMLevelMusicPlayOrder = *r; CMLevelMusicPlayOrder <= static_cast<uint8_t>(LevelMusicPlayOrder::Random))
					CGameCfg.CMLevelMusicPlayOrder = LevelMusicPlayOrder{CMLevelMusicPlayOrder};
		}
		else if (cmp(lb, eq, CMLevelMusicTrack0Str))
			convert_integer(CGameCfg.CMLevelMusicTrack[0], value);
		else if (cmp(lb, eq, CMLevelMusicTrack1Str))
			convert_integer(CGameCfg.CMLevelMusicTrack[1], value);
		else if (cmp(lb, eq, CMLevelMusicPathStr))
			convert_string(CGameCfg.CMLevelMusicPath, value, eol);
		else if (cmp(lb, eq, CMMiscMusic0Str))
			convert_string(CGameCfg.CMMiscMusic[SONG_TITLE], value, eol);
		else if (cmp(lb, eq, CMMiscMusic1Str))
			convert_string(CGameCfg.CMMiscMusic[SONG_BRIEFING], value, eol);
		else if (cmp(lb, eq, CMMiscMusic2Str))
			convert_string(CGameCfg.CMMiscMusic[SONG_ENDLEVEL], value, eol);
		else if (cmp(lb, eq, CMMiscMusic3Str))
			convert_string(CGameCfg.CMMiscMusic[SONG_ENDGAME], value, eol);
		else if (cmp(lb, eq, CMMiscMusic4Str))
			convert_string(CGameCfg.CMMiscMusic[SONG_CREDITS], value, eol);
		else if (cmp(lb, eq, GammaLevelStr))
		{
			convert_integer(CGameCfg.GammaLevel, value);
			gr_palette_set_gamma(CGameCfg.GammaLevel);
		}
		else if (cmp(lb, eq, LastPlayerStr))
			GameCfg.LastPlayer.copy_lower(std::span(value, std::distance(value, eol)));
		else if (cmp(lb, eq, LastMissionStr))
			convert_string(CGameCfg.LastMission, value, eol);
		else if (cmp(lb, eq, ResolutionXStr))
			convert_integer(CGameCfg.ResolutionX, value);
		else if (cmp(lb, eq, ResolutionYStr))
			convert_integer(CGameCfg.ResolutionY, value);
		else if (cmp(lb, eq, AspectXStr))
			convert_integer(GameCfg.AspectX, value);
		else if (cmp(lb, eq, AspectYStr))
			convert_integer(GameCfg.AspectY, value);
		else if (cmp(lb, eq, WindowModeStr))
			convert_integer(CGameCfg.WindowMode, value);
#if DXX_USE_OGL
		else if (cmp(lb, eq, TexFiltStr))
		{
			if (auto r = convert_integer<uint8_t>(value))
			{
				switch (const auto TexFilt = *r)
				{
					case static_cast<unsigned>(opengl_texture_filter::classic):
					case static_cast<unsigned>(opengl_texture_filter::upscale):
					case static_cast<unsigned>(opengl_texture_filter::trilinear):
						CGameCfg.TexFilt = opengl_texture_filter{TexFilt};
						break;
				}
			}
		}
		else if (cmp(lb, eq, TexAnisStr))
			convert_integer(CGameCfg.TexAnisotropy, value);
#endif
#if defined(DXX_BUILD_DESCENT_II)
#if DXX_USE_OGL
		else if (cmp(lb, eq, MovieTexFiltStr))
			convert_integer(GameCfg.MovieTexFilt, value);
#endif
		else if (cmp(lb, eq, MovieSubtitlesStr))
			convert_integer(GameCfg.MovieSubtitles, value);
#endif
#if DXX_USE_ADLMIDI
		else if (cmp(lb, eq, ADLMIDINumChipsStr))
			convert_integer(CGameCfg.ADLMIDI_num_chips, value);
		else if (cmp(lb, eq, ADLMIDIBankStr))
			convert_integer(CGameCfg.ADLMIDI_bank, value);
		else if (cmp(lb, eq, ADLMIDIEnabledStr))
			convert_integer(CGameCfg.ADLMIDI_enabled, value);
#endif
		else if (cmp(lb, eq, VSyncStr))
			convert_integer(CGameCfg.VSync, value);
#if DXX_USE_OGL
		else if (cmp(lb, eq, MultisampleStr))
			convert_integer(CGameCfg.Multisample, value);
#endif
		else if (cmp(lb, eq, FPSIndicatorStr))
			convert_integer(CGameCfg.FPSIndicator, value);
		else if (cmp(lb, eq, GrabinputStr))
			convert_integer(CGameCfg.Grabinput, value);
	}
	if ( GameCfg.DigiVolume > 8 ) GameCfg.DigiVolume = 8;
	if ( GameCfg.MusicVolume > 8 ) GameCfg.MusicVolume = 8;

	if (CGameCfg.ResolutionX >= 320 && CGameCfg.ResolutionY >= 200)
	{
		Game_screen_mode.width = CGameCfg.ResolutionX;
		Game_screen_mode.height = CGameCfg.ResolutionY;
	}

	return 0;
}

int WriteConfigFile()
{
	CGameCfg.GammaLevel = gr_palette_get_gamma();

	auto infile = PHYSFSX_openWriteBuffered("descent.cfg").first;
	if (!infile)
	{
		return 1;
	}
	PHYSFSX_printf(infile, "%s=%d\n", DigiVolumeStr, GameCfg.DigiVolume);
	PHYSFSX_printf(infile, "%s=%d\n", MusicVolumeStr, GameCfg.MusicVolume);
	PHYSFSX_printf(infile, "%s=%d\n", ReverseStereoStr, GameCfg.ReverseStereo);
	PHYSFSX_printf(infile, "%s=%d\n", OrigTrackOrderStr, GameCfg.OrigTrackOrder);
	PHYSFSX_printf(infile, "%s=%d\n", MusicTypeStr, GameCfg.MusicType);
	PHYSFSX_printf(infile, "%s=%d\n", CMLevelMusicPlayOrderStr, static_cast<int>(CGameCfg.CMLevelMusicPlayOrder));
	PHYSFSX_printf(infile, "%s=%d\n", CMLevelMusicTrack0Str, CGameCfg.CMLevelMusicTrack[0]);
	PHYSFSX_printf(infile, "%s=%d\n", CMLevelMusicTrack1Str, CGameCfg.CMLevelMusicTrack[1]);
	PHYSFSX_printf(infile, "%s=%s\n", CMLevelMusicPathStr, CGameCfg.CMLevelMusicPath.data());
	PHYSFSX_printf(infile, "%s=%s\n", CMMiscMusic0Str, CGameCfg.CMMiscMusic[SONG_TITLE].data());
	PHYSFSX_printf(infile, "%s=%s\n", CMMiscMusic1Str, CGameCfg.CMMiscMusic[SONG_BRIEFING].data());
	PHYSFSX_printf(infile, "%s=%s\n", CMMiscMusic2Str, CGameCfg.CMMiscMusic[SONG_ENDLEVEL].data());
	PHYSFSX_printf(infile, "%s=%s\n", CMMiscMusic3Str, CGameCfg.CMMiscMusic[SONG_ENDGAME].data());
	PHYSFSX_printf(infile, "%s=%s\n", CMMiscMusic4Str, CGameCfg.CMMiscMusic[SONG_CREDITS].data());
	PHYSFSX_printf(infile, "%s=%d\n", GammaLevelStr, CGameCfg.GammaLevel);
	PHYSFSX_printf(infile, "%s=%s\n", LastPlayerStr, static_cast<const char *>(InterfaceUniqueState.PilotName));
	PHYSFSX_printf(infile, "%s=%s\n", LastMissionStr, static_cast<const char *>(CGameCfg.LastMission));
	PHYSFSX_printf(infile, "%s=%i\n", ResolutionXStr, SM_W(Game_screen_mode));
	PHYSFSX_printf(infile, "%s=%i\n", ResolutionYStr, SM_H(Game_screen_mode));
	PHYSFSX_printf(infile, "%s=%i\n", AspectXStr, GameCfg.AspectX);
	PHYSFSX_printf(infile, "%s=%i\n", AspectYStr, GameCfg.AspectY);
	PHYSFSX_printf(infile, "%s=%i\n", WindowModeStr, CGameCfg.WindowMode);
#if DXX_USE_OGL
	PHYSFSX_printf(infile, "%s=%i\n", TexFiltStr, static_cast<unsigned>(CGameCfg.TexFilt));
	PHYSFSX_printf(infile, "%s=%i\n", TexAnisStr, CGameCfg.TexAnisotropy);
#endif
#if defined(DXX_BUILD_DESCENT_II)
#if DXX_USE_OGL
	PHYSFSX_printf(infile, "%s=%i\n", MovieTexFiltStr, GameCfg.MovieTexFilt);
#endif
	PHYSFSX_printf(infile, "%s=%i\n", MovieSubtitlesStr, GameCfg.MovieSubtitles);
#endif
#if DXX_USE_ADLMIDI
	PHYSFSX_printf(infile, "%s=%i\n", ADLMIDINumChipsStr, CGameCfg.ADLMIDI_num_chips);
	PHYSFSX_printf(infile, "%s=%i\n", ADLMIDIBankStr, CGameCfg.ADLMIDI_bank);
	PHYSFSX_printf(infile, "%s=%i\n", ADLMIDIEnabledStr, CGameCfg.ADLMIDI_enabled);
#endif
	PHYSFSX_printf(infile, "%s=%i\n", VSyncStr, CGameCfg.VSync);
#if DXX_USE_OGL
	PHYSFSX_printf(infile, "%s=%i\n", MultisampleStr, CGameCfg.Multisample);
#endif
	PHYSFSX_printf(infile, "%s=%i\n", FPSIndicatorStr, CGameCfg.FPSIndicator);
	PHYSFSX_printf(infile, "%s=%i\n", GrabinputStr, CGameCfg.Grabinput);
	return 0;
}

}
