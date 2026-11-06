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
#include <utility>
#include <charconv>
#include <string_view>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cctype>

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
#include "strutil.h"
#include "nvparse.h"
#if DXX_USE_OGL
#include "ogl_init.h"
#endif
#include <memory>

namespace dcx {

#define DXX_DESCENT_CFG_NAME	"descent.cfg"

CCfg CGameCfg;

namespace {

static char dxx_descent_cfg_name[]{DXX_DESCENT_CFG_NAME};

/* This is declared for use in `decltype`, and never defined. */
template <std::size_t N>
	std::integral_constant<std::size_t, N> field_value_length(const std::array<char, N> &);

}

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
#if DXX_BUILD_DESCENT == 2
#define MovieTexFiltStr "MovieTexFilt"
#define MovieSubtitlesStr "MovieSubtitles"
#endif
#if DXX_USE_ADLMIDI
#define ADLMIDINumChipsStr	"ADLMIDI_NumberOfChips"
#define ADLMIDIBankStr	"ADLMIDI_Bank"
#define ADLMIDIEnabledStr	"ADLMIDI_Enabled"
#define DXX_DESCENT_CFG_ADLMIDI_BLOCK(VERB_d)	\
	VERB_d(ADLMIDINumChipsStr, CGameCfg.ADLMIDI_num_chips)	\
	VERB_d(ADLMIDIBankStr, CGameCfg.ADLMIDI_bank)	\
	VERB_d(ADLMIDIEnabledStr, CGameCfg.ADLMIDI_enabled)	\

#else
#define DXX_DESCENT_CFG_ADLMIDI_BLOCK(VERB_d)	/* ADLMIDI is disabled - expand to nothing */
#endif
#define VSyncStr "VSync"
#define MultisampleStr "Multisample"
#define FPSIndicatorStr "FPSIndicator"
#define GrabinputStr "GrabInput"

namespace {

template <std::integral T>
static std::optional<T> parse_config_number(std::string_view text)
{
	if constexpr (std::is_same_v<T, bool>)
	{
		const auto value = parse_config_number<uint8_t>(text);
		return value && *value <= 1 ? std::optional<bool>{*value != 0} : std::nullopt;
	}
	else
	{
		while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())))
			text.remove_prefix(1);
		if (text.empty())
			return std::nullopt;
		if (text.front() == '+')
		{
			text.remove_prefix(1);
			if (text.empty() || text.front() == '-')
				return std::nullopt;
		}
		bool negative_zero = false;
		if constexpr (std::is_unsigned_v<T>)
		{
			if (text.front() == '-')
			{
				negative_zero = true;
				text.remove_prefix(1);
			}
		}
		T value{};
		const auto end = text.data() + text.size();
		const auto result = std::from_chars(text.data(), end, value);
		if (result.ec != std::errc{} || result.ptr != end || (negative_zero && value != 0))
			return std::nullopt;
		return value;
	}
}

template <std::integral T>
static void read_config_number(T &value, const std::string_view text)
{
	if (const auto parsed = parse_config_number<T>(text))
		value = *parsed;
}

#if DXX_BUILD_DESCENT == 1
#define D2X_DESCENT_CFG_TEXT(VERB_d)	/* For Descent 1, expand to nothing */

#elif DXX_BUILD_DESCENT == 2
#define D2X_DESCENT_CFG_TEXT(VERB_d)	/* For Descent 2, expand to values specific to D2. */	\
	VERB_d(MovieTexFiltStr, GameCfg.MovieTexFilt)	\
	VERB_d(MovieSubtitlesStr, GameCfg.MovieSubtitles)	\

#endif

#define DXX_DESCENT_CFG_TEXT(VERB_d,VERB_s)	\
	VERB_d(DigiVolumeStr, CGameCfg.DigiVolume)	\
	VERB_d(MusicVolumeStr, CGameCfg.MusicVolume)	\
	VERB_d(ReverseStereoStr, CGameCfg.ReverseStereo)	\
	VERB_d(OrigTrackOrderStr, CGameCfg.OrigTrackOrder)	\
	VERB_d(MusicTypeStr, underlying_value(CGameCfg.MusicType))	\
	VERB_d(CMLevelMusicPlayOrderStr, underlying_value(CGameCfg.CMLevelMusicPlayOrder))	\
	VERB_d(CMLevelMusicTrack0Str, CGameCfg.CMLevelMusicTrack[0])	\
	VERB_d(CMLevelMusicTrack1Str, CGameCfg.CMLevelMusicTrack[1])	\
	VERB_s(CMLevelMusicPathStr, CGameCfg.CMLevelMusicPath)	\
	VERB_s(CMMiscMusic0Str, CGameCfg.CMMiscMusic[song_number::title])	\
	VERB_s(CMMiscMusic1Str, CGameCfg.CMMiscMusic[song_number::briefing])	\
	VERB_s(CMMiscMusic2Str, CGameCfg.CMMiscMusic[song_number::endlevel])	\
	VERB_s(CMMiscMusic3Str, CGameCfg.CMMiscMusic[song_number::endgame])	\
	VERB_s(CMMiscMusic4Str, CGameCfg.CMMiscMusic[song_number::credits])	\
	VERB_d(GammaLevelStr, CGameCfg.GammaLevel)	\
	VERB_s(LastPlayerStr, InterfaceUniqueState.PilotName.a)	\
	VERB_s(LastMissionStr, CGameCfg.LastMission)	\
	VERB_d(ResolutionXStr, SM_W(Game_screen_mode))	\
	VERB_d(ResolutionYStr, SM_H(Game_screen_mode))	\
	VERB_d(AspectXStr, CGameCfg.AspectX)	\
	VERB_d(AspectYStr, CGameCfg.AspectY)	\
	VERB_d(WindowModeStr, CGameCfg.WindowMode)	\
	VERB_d(TexFiltStr, underlying_value(CGameCfg.TexFilt))	\
	VERB_d(TexAnisStr, CGameCfg.TexAnisotropy)	\
	D2X_DESCENT_CFG_TEXT(VERB_d)	\
	DXX_DESCENT_CFG_ADLMIDI_BLOCK(VERB_d)	\
	VERB_d(VSyncStr, CGameCfg.VSync)	\
	VERB_d(MultisampleStr, CGameCfg.Multisample)	\
	VERB_d(FPSIndicatorStr, CGameCfg.FPSIndicator)	\
	VERB_d(GrabinputStr, CGameCfg.Grabinput)	\

struct descent_cfg_file_prepared_text
{
#define DXX_DESCENT_CFG_FIELD_NAME_LENGTH(NAME)	+ sizeof(NAME) /* includes the trailing null, so there is no need to add 1 for the equal sign */ + 1 /* trailing newline */
#define DXX_DESCENT_CFG_FIELD_LENGTH_d(NAME, VALUE)	DXX_DESCENT_CFG_FIELD_NAME_LENGTH(NAME) + 1 /* for negative sign, if needed */ + number_to_text_length<std::numeric_limits<typename std::remove_reference_t<decltype(VALUE)>>::max()> + 1 /* for newline */
#define DXX_DESCENT_CFG_FIELD_LENGTH_s(NAME, VALUE)	DXX_DESCENT_CFG_FIELD_NAME_LENGTH(NAME) + decltype(field_value_length(VALUE))::value
	/* Declare a function to bring the macro's variable names into scope so
	 * that `DXX_DESCENT_CFG_TEXT` can refer to them.  It is never defined.
	 */
	static auto configuration_text_file_length(const CCfg &CGameCfg, const Cfg &GameCfg) -> std::integral_constant<std::size_t, DXX_DESCENT_CFG_TEXT(DXX_DESCENT_CFG_FIELD_LENGTH_d, DXX_DESCENT_CFG_FIELD_LENGTH_s)>;
	using buffer_type = std::array<char, decltype(configuration_text_file_length(std::declval<CCfg>(), std::declval<Cfg>()))::value>;
#undef DXX_DESCENT_CFG_FIELD_LENGTH_s
#undef DXX_DESCENT_CFG_FIELD_LENGTH_d
#undef DXX_DESCENT_CFG_FIELD_NAME_LENGTH
	std::size_t written;
	buffer_type buf;
};

static bool cfg_file_unchanged(const descent_cfg_file_prepared_text &current_cfg_text)
{
	const auto &&[f, physfserr]{PHYSFSX_openReadBuffered_updateCase(dxx_descent_cfg_name)};
	if (!f)
	{
		/* If the file cannot be read, assume that a new file must be written. */
		con_printf(physfserr == PHYSFS_ERR_NOT_FOUND ? CON_VERBOSE : CON_NORMAL, "Failed to read \"" DXX_DESCENT_CFG_NAME "\": %s", PHYSFS_getErrorByCode(physfserr));
		return false;
	}
	const auto existing_file_size{PHYSFS_fileLength(f)};
	if (!std::cmp_equal(existing_file_size, current_cfg_text.written))
	{
		/* The file is of a different length, so its contents must be
		 * different.  A new file must be written.
		 */
		con_printf(CON_VERBOSE, "Replacing configuration file \"" DXX_DESCENT_CFG_NAME "\" due to size mismatch: current=%" DXX_PRI_size_type "; new=%" DXX_PRI_size_type, static_cast<std::size_t>(existing_file_size), current_cfg_text.written);
		return false;
	}
	descent_cfg_file_prepared_text::buffer_type previous_cfg_text;
	if (const auto rb{PHYSFS_readBytes(f, previous_cfg_text.data(), previous_cfg_text.size())}; rb != existing_file_size)
	{
		/* Log this at a higher level because it is strange for the file to
		 * change size between when `PHYSFS_fileLength` examines it and when
		 * `PHYSFS_readBytes` reads it.
		 */
		con_printf(CON_NORMAL, "Replacing configuration file \"" DXX_DESCENT_CFG_NAME "\" due to read count mismatch: expected=%" DXX_PRI_size_type "; read=%" DXX_PRI_size_type, static_cast<std::size_t>(existing_file_size), static_cast<std::size_t>(rb));
		return false;
	}
	const auto pcbegin{previous_cfg_text.begin()};
	const auto pcend{std::next(pcbegin, existing_file_size)};
	const auto ccbegin{current_cfg_text.buf.begin()};
	const auto ccend{std::next(ccbegin, existing_file_size)};
	/* The two ranges are of equal length, so `in1 == pcend` if and only if `in2 == ccend`. */
	if (const auto mm{std::ranges::mismatch(pcbegin, pcend, ccbegin, ccend)}; mm.in1 != pcend)
	{
		con_printf(CON_VERBOSE, "Replacing configuration file \"" DXX_DESCENT_CFG_NAME "\" due to content mismatch at offset %" DXX_PRI_size_type, mm.in1 - pcbegin);
		return false;
	}
	con_puts(CON_VERBOSE, "Retaining configuration file \"" DXX_DESCENT_CFG_NAME "\"");
	return true;
}

static std::optional<descent_cfg_file_prepared_text> build_cfg_file_text(const CCfg &CGameCfg, const Cfg &GameCfg)
{
#if DXX_BUILD_DESCENT == 1
	(void)GameCfg;	/* Only Descent 2 uses this variable. */
#endif
	descent_cfg_file_prepared_text result;
	std::span<char> output{result.buf};
	const auto append = [&output](const std::string_view text) {
		if (text.size() > output.size())
			return false;
		std::ranges::copy(text, output.begin());
		output = output.subspan(text.size());
		return true;
	};
	const auto number = [&output, &append](const auto value) {
		if constexpr (std::is_same_v<decltype(value), const bool>)
			return append(value ? "1" : "0");
		else
		{
			const auto end = output.data() + output.size();
			const auto converted = std::to_chars(output.data(), end, value);
			if (converted.ec != std::errc{})
				return false;
			output = std::span(converted.ptr, end);
			return true;
		}
	};
#define DXX_DESCENT_CFG_WRITE_d(NAME, VALUE) \
	if (!append(NAME "=") || !number(VALUE) || !append("\n")) return std::nullopt;
#define DXX_DESCENT_CFG_WRITE_s(NAME, VALUE) \
	if (!append(NAME "=") || !append((VALUE).data()) || !append("\n")) return std::nullopt;
	DXX_DESCENT_CFG_TEXT(DXX_DESCENT_CFG_WRITE_d, DXX_DESCENT_CFG_WRITE_s)
#undef DXX_DESCENT_CFG_WRITE_s
#undef DXX_DESCENT_CFG_WRITE_d
	result.written = result.buf.size() - output.size();
	return result;
}

}

void ReadConfigFile(CCfg &CGameCfg, Cfg &GameCfg)
{
	// set defaults
	CGameCfg.DigiVolume = 8;
	CGameCfg.MusicVolume = 8;
	CGameCfg.ReverseStereo = false;
	CGameCfg.OrigTrackOrder = false;
#if DXX_USE_SDL_REDBOOK_AUDIO && defined(__APPLE__) && defined(__MACH__)
	CGameCfg.MusicType = music_type::Redbook;
#else
	CGameCfg.MusicType = music_type::Builtin;
#endif
	CGameCfg.CMLevelMusicPlayOrder = LevelMusicPlayOrder::Continuous;
	CGameCfg.CMLevelMusicTrack[0] = -1;
	CGameCfg.CMLevelMusicTrack[1] = -1;
	CGameCfg.CMLevelMusicPath = {};
	CGameCfg.CMMiscMusic = {};
#if defined(__APPLE__) && defined(__MACH__)
	CGameCfg.OrigTrackOrder = true;
	const auto userdir = PHYSFS_getUserDir();
#if DXX_BUILD_DESCENT == 1
	CGameCfg.CMLevelMusicPlayOrder = LevelMusicPlayOrder::Level;
	CGameCfg.CMLevelMusicPath = "descent.m3u";
	snprintf(CGameCfg.CMMiscMusic[song_number::title].data(), CGameCfg.CMMiscMusic[song_number::title].size(), "%s%s", userdir, "Music/iTunes/iTunes Music/Insanity/Descent/02 Primitive Rage.mp3");
	snprintf(CGameCfg.CMMiscMusic[song_number::credits].data(), CGameCfg.CMMiscMusic[song_number::credits].size(), "%s%s", userdir, "Music/iTunes/iTunes Music/Insanity/Descent/05 The Darkness Of Space.mp3");
#elif DXX_BUILD_DESCENT == 2
	CGameCfg.CMLevelMusicPath = "descent2.m3u";
	snprintf(CGameCfg.CMMiscMusic[song_number::title].data(), CGameCfg.CMMiscMusic[song_number::title].size(), "%s%s", userdir, "Music/iTunes/iTunes Music/Redbook Soundtrack/Descent II, Macintosh CD-ROM/02 Title.mp3");
	snprintf(CGameCfg.CMMiscMusic[song_number::credits].data(), CGameCfg.CMMiscMusic[song_number::credits].size(), "%s%s", userdir, "Music/iTunes/iTunes Music/Redbook Soundtrack/Descent II, Macintosh CD-ROM/03 Crawl.mp3");
#endif
	snprintf(CGameCfg.CMMiscMusic[song_number::briefing].data(), CGameCfg.CMMiscMusic[song_number::briefing].size(), "%s%s", userdir, "Music/iTunes/iTunes Music/Insanity/Descent/03 Outerlimits.mp3");
	snprintf(CGameCfg.CMMiscMusic[song_number::endlevel].data(), CGameCfg.CMMiscMusic[song_number::endlevel].size(), "%s%s", userdir, "Music/iTunes/iTunes Music/Insanity/Descent/04 Close Call.mp3");
	snprintf(CGameCfg.CMMiscMusic[song_number::endgame].data(), CGameCfg.CMMiscMusic[song_number::endgame].size(), "%s%s", userdir, "Music/iTunes/iTunes Music/Insanity/Descent/14 Insanity.mp3");
#endif
	CGameCfg.GammaLevel = 0;
	GameCfg.LastPlayer = {};
	CGameCfg.LastMission = "";
	CGameCfg.ResolutionX = 1024;
	CGameCfg.ResolutionY = 768;
	CGameCfg.AspectX = 3;
	CGameCfg.AspectY = 4;
	CGameCfg.WindowMode = false;
#if DXX_USE_OGL
	CGameCfg.TexFilt = opengl_texture_filter::classic;
#endif
	CGameCfg.TexAnisotropy = 0;
#if DXX_BUILD_DESCENT == 2
	GameCfg.MovieTexFilt = 0;
	GameCfg.MovieSubtitles = 0;
#endif
	CGameCfg.VSync = false;
	CGameCfg.Multisample = 0;
	CGameCfg.FPSIndicator = 0;
	CGameCfg.Grabinput = true;

	auto &&[infile, physfserr]{PHYSFSX_openReadBuffered_updateCase(dxx_descent_cfg_name)};
	if (!infile)
	{
		/* "File not found" is less important, and happens naturally on a first
		 * install.  Log it at a lower verbosity level.
		 */
		con_printf(physfserr == PHYSFS_ERR_NOT_FOUND ? CON_VERBOSE : CON_NORMAL, "Failed to read \"" DXX_DESCENT_CFG_NAME "\": %s", PHYSFS_getErrorByCode(physfserr));
		return;
	}

	// to be fully safe, assume the whole cfg consists of one big line
	for (PHYSFSX_gets_line_t<0> line(PHYSFS_fileLength(infile) + 1); const auto rc{PHYSFSX_fgets(line, infile)};)
	{
		const auto eol{rc.end()};
		const auto lb{rc.begin()};
		const auto eq{std::ranges::find(lb, eol, '=')};
		if (eq == eol)
			continue;
		const auto value{std::next(eq)};
		const std::string_view numeric_value{value, eol};
		if (const std::ranges::subrange name{lb, eq}; compare_nonterminated_name(name, DigiVolumeStr))
		{
			if (const auto r = parse_config_number<uint8_t>(numeric_value))
				if (const auto v = *r; v < 8)
					CGameCfg.DigiVolume = v;
		}
		else if (compare_nonterminated_name(name, MusicVolumeStr))
		{
			if (const auto r = parse_config_number<uint8_t>(numeric_value))
				if (const auto v = *r; v < 8)
					CGameCfg.MusicVolume = v;
		}
		else if (compare_nonterminated_name(name, ReverseStereoStr))
			read_config_number(CGameCfg.ReverseStereo, numeric_value);
		else if (compare_nonterminated_name(name, OrigTrackOrderStr))
			read_config_number(CGameCfg.OrigTrackOrder, numeric_value);
		else if (compare_nonterminated_name(name, MusicTypeStr))
		{
			if (const auto r = parse_config_number<uint8_t>(numeric_value))
				switch (const music_type v{*r})
				{
					case music_type::None:
					case music_type::Builtin:
#if DXX_USE_SDL_REDBOOK_AUDIO
					case music_type::Redbook:
#endif
					case music_type::Custom:
						CGameCfg.MusicType = v;
						break;
				}
		}
		else if (compare_nonterminated_name(name, CMLevelMusicPlayOrderStr))
		{
			if (auto r = parse_config_number<uint8_t>(numeric_value))
				if (auto CMLevelMusicPlayOrder = *r; CMLevelMusicPlayOrder <= underlying_value(LevelMusicPlayOrder::Random))
					CGameCfg.CMLevelMusicPlayOrder = LevelMusicPlayOrder{CMLevelMusicPlayOrder};
		}
		else if (compare_nonterminated_name(name, CMLevelMusicTrack0Str))
			read_config_number(CGameCfg.CMLevelMusicTrack[0], numeric_value);
		else if (compare_nonterminated_name(name, CMLevelMusicTrack1Str))
			read_config_number(CGameCfg.CMLevelMusicTrack[1], numeric_value);
		else if (compare_nonterminated_name(name, CMLevelMusicPathStr))
			convert_string(CGameCfg.CMLevelMusicPath, value, eol);
		else if (compare_nonterminated_name(name, CMMiscMusic0Str))
			convert_string(CGameCfg.CMMiscMusic[song_number::title], value, eol);
		else if (compare_nonterminated_name(name, CMMiscMusic1Str))
			convert_string(CGameCfg.CMMiscMusic[song_number::briefing], value, eol);
		else if (compare_nonterminated_name(name, CMMiscMusic2Str))
			convert_string(CGameCfg.CMMiscMusic[song_number::endlevel], value, eol);
		else if (compare_nonterminated_name(name, CMMiscMusic3Str))
			convert_string(CGameCfg.CMMiscMusic[song_number::endgame], value, eol);
		else if (compare_nonterminated_name(name, CMMiscMusic4Str))
			convert_string(CGameCfg.CMMiscMusic[song_number::credits], value, eol);
		else if (compare_nonterminated_name(name, GammaLevelStr))
		{
			read_config_number(CGameCfg.GammaLevel, numeric_value);
			gr_palette_set_gamma(CGameCfg.GammaLevel);
		}
		else if (compare_nonterminated_name(name, LastPlayerStr))
			GameCfg.LastPlayer.copy_lower(std::span(value, std::distance(value, eol)));
		else if (compare_nonterminated_name(name, LastMissionStr))
			convert_string(CGameCfg.LastMission, value, eol);
		else if (compare_nonterminated_name(name, ResolutionXStr))
			read_config_number(CGameCfg.ResolutionX, numeric_value);
		else if (compare_nonterminated_name(name, ResolutionYStr))
			read_config_number(CGameCfg.ResolutionY, numeric_value);
		else if (compare_nonterminated_name(name, AspectXStr))
			read_config_number(CGameCfg.AspectX, numeric_value);
		else if (compare_nonterminated_name(name, AspectYStr))
			read_config_number(CGameCfg.AspectY, numeric_value);
		else if (compare_nonterminated_name(name, WindowModeStr))
			read_config_number(CGameCfg.WindowMode, numeric_value);
		else if (compare_nonterminated_name(name, TexFiltStr))
		{
			if (auto r = parse_config_number<uint8_t>(numeric_value))
			{
				switch (const auto TexFilt = *r)
				{
#if DXX_USE_OGL
					case static_cast<unsigned>(opengl_texture_filter::classic):
					case static_cast<unsigned>(opengl_texture_filter::upscale):
					case static_cast<unsigned>(opengl_texture_filter::trilinear):
#else
					default:
						/* In SDL-only builds, accept any value and save it.
						 * The value will not be used, but it will be written
						 * back to the configuration file, to avoid deleting
						 * settings for players who use both SDL-only and
						 * OpenGL-enabled builds.
						 */
#endif
						CGameCfg.TexFilt = opengl_texture_filter{TexFilt};
						break;
				}
			}
		}
		else if (compare_nonterminated_name(name, TexAnisStr))
			read_config_number(CGameCfg.TexAnisotropy, numeric_value);
#if DXX_BUILD_DESCENT == 2
		else if (compare_nonterminated_name(name, MovieTexFiltStr))
			read_config_number(GameCfg.MovieTexFilt, numeric_value);
		else if (compare_nonterminated_name(name, MovieSubtitlesStr))
			read_config_number(GameCfg.MovieSubtitles, numeric_value);
#endif
#if DXX_USE_ADLMIDI
		else if (compare_nonterminated_name(name, ADLMIDINumChipsStr))
			read_config_number(CGameCfg.ADLMIDI_num_chips, numeric_value);
		else if (compare_nonterminated_name(name, ADLMIDIBankStr))
			read_config_number(CGameCfg.ADLMIDI_bank, numeric_value);
		else if (compare_nonterminated_name(name, ADLMIDIEnabledStr))
			read_config_number(CGameCfg.ADLMIDI_enabled, numeric_value);
#endif
		else if (compare_nonterminated_name(name, VSyncStr))
			read_config_number(CGameCfg.VSync, numeric_value);
		else if (compare_nonterminated_name(name, MultisampleStr))
			read_config_number(CGameCfg.Multisample, numeric_value);
		else if (compare_nonterminated_name(name, FPSIndicatorStr))
			read_config_number(CGameCfg.FPSIndicator, numeric_value);
		else if (compare_nonterminated_name(name, GrabinputStr))
			read_config_number(CGameCfg.Grabinput, numeric_value);
	}

	if (CGameCfg.ResolutionX >= 320 && CGameCfg.ResolutionY >= 200)
	{
		Game_screen_mode.width = CGameCfg.ResolutionX;
		Game_screen_mode.height = CGameCfg.ResolutionY;
	}
}

int WriteConfigFile(const CCfg &CGameCfg, const Cfg &GameCfg)
{
	const auto current_cfg_text{build_cfg_file_text(CGameCfg, GameCfg)};
	if (!current_cfg_text)
	{
		con_puts(CON_NORMAL, "Failed to prepare configuration text");
		return 1;
	}
	if (cfg_file_unchanged(*current_cfg_text))
		return 0;
	auto &&[outfile, physfserr]{PHYSFSX_openWriteBuffered(DXX_DESCENT_CFG_NAME)};
	if (!outfile)
	{
		con_printf(CON_NORMAL, "Failed to write \"" DXX_DESCENT_CFG_NAME "\": %s", PHYSFS_getErrorByCode(physfserr));
		return 1;
	}
	PHYSFSX_writeBytes(outfile, current_cfg_text->buf.data(), current_cfg_text->written);
	return 0;
}

}
