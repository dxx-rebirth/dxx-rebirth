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
 * Routines to manage the songs in Descent.
 *
 */

#include <iterator>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dxxerror.h"
#include "pstypes.h"
#include "songs.h"
#include "strutil.h"
#include "digi.h"
#include "rbaudio.h"
#if DXX_USE_SDLMIXER
#include "digi_mixer_music.h"
#endif
#include "jukebox.h"
#include "config.h"
#include "u_mem.h"
#include "args.h"
#include "physfsx.h"
#include "game.h"
#include "console.h"
#include <memory>

namespace dcx {

namespace {

enum class level_song_number : unsigned
{
	None = UINT32_MAX
};

class user_configured_level_songs : std::unique_ptr<bim_song_info[]>
{
	using base_type = std::unique_ptr<bim_song_info[]>;
	std::size_t count{};
public:
	constexpr user_configured_level_songs() = default;
	explicit user_configured_level_songs(const std::size_t length) :
		base_type{std::make_unique<bim_song_info[]>(length)}, count{length}
	{
	}
	auto size() const
	{
		return count;
	}
	void reset()
	{
		count = {};
		base_type::reset();
	}
	void resize(const std::size_t length)
	{
		base_type::operator=(std::make_unique<bim_song_info[]>(length));
		count = {length};
	}
	bim_song_info &operator[](const song_number i) const
	{
		return base_type::operator[](static_cast<std::size_t>(i));
	}
	user_configured_level_songs &operator=(user_configured_level_songs &&) = default;
	void operator=(std::vector<bim_song_info> &&v);
};

void user_configured_level_songs::operator=(std::vector<bim_song_info> &&v)
{
	if (v.empty())
	{
		/* If the vector is empty, use `reset` so that this object
		 * stores `nullptr` instead of a pointer to a zero-length array.
		 * Storing `nullptr` causes `operator bool()` to return false.
		 * Storing a zero-length array causes `operator bool()` to
		 * return true.
		 *
		 * A true return causes a divide-by-zero later when the object
		 * is considered true, but then reports a length of zero.
		 */
		reset();
		return;
	}
	resize(v.size());
	std::ranges::move(v, this->get());
}

static song_number build_song_number_from_level_song_number(const level_song_number i)
{
	return song_number{underlying_value(i) + underlying_value(song_number::first_level_song)};
}

static song_number Song_playing{song_number::None}; // song_number::None if no song playing, else the Descent song number
#if DXX_USE_SDL_REDBOOK_AUDIO
static int Redbook_playing = 0; // Redbook track num differs from Song_playing. We need this for Redbook repeat hooks.
#endif

user_configured_level_songs BIMSongs;
user_configured_level_songs BIMSecretSongs;

}

song_number songs_is_playing()
{
	return Song_playing;
}

//takes volume in range 0..8
//NOTE that we do not check what is playing right now (except Redbook) This is because here we don't (want) know WHAT we're playing - let the subfunctions do it (i.e. digi_win32_set_music_volume() knows if a MIDI plays or not)
void songs_set_volume(int volume)
{
#ifdef _WIN32
	digi_win32_set_midi_volume(volume);
#endif
#if DXX_USE_SDL_REDBOOK_AUDIO
	if (CGameCfg.MusicType == music_type::Redbook)
	{
		RBASetVolume(0);
		RBASetVolume(volume);
	}
#else
	(void)volume;
#endif
#if DXX_USE_SDLMIXER
	mix_set_music_volume(volume);
#endif
}

namespace {

struct deferred_physfs_pathname
{
	std::optional<const char *> dirname;
	const char *const physfs_virtual_path;
	deferred_physfs_pathname(const char *p) :
		physfs_virtual_path{p}
	{
	}
	const char *get_dirname();
};

const char *deferred_physfs_pathname::get_dirname()
{
	if (!dirname)
	{
		const auto r{PHYSFS_getRealDir(physfs_virtual_path)};
		dirname = r;
		return r;
	}
	return *dirname;
}

static bool is_valid_song_extension(const std::ranges::subrange<std::span<char>::iterator> extension)
{
	/* This is a special purpose tolower, which only works due to the special
	 * circumstances in this function.
	 * - the user-supplied `extension` may be any set of characters, including
	 *   non-letters
	 * - candidate extensions are known to be lowercase letters only, and are
	 *   not transformed
	 * - applying this transform to a user-defined input character has 3 cases:
	 *   - if the input is ASCII uppercase, it becomes ASCII lowercase
	 *   - if the input is ASCII lowercase, it is unchanged
	 *   - if the input is not a letter, it is changed to some other non-letter
	 * Since the comparison only seeks matches, and all candidate extensions
	 * are all lowercase letters, changing one non-letter to a different
	 * non-letter preserves the inequality.
	 */
	auto &&lower_letter = [](const char c) -> char {
		return c | ('a' - 'A');
	};
	using namespace std::string_view_literals;
#define SONG_STRING_VIEW_CONCAT_UDL(EXTENSION)	EXTENSION##sv
#define SONG_STRING_VIEW(EXTENSION)	SONG_STRING_VIEW_CONCAT_UDL(EXTENSION)
	const auto is_HMP{std::ranges::equal(extension, SONG_STRING_VIEW(SONG_EXT_HMP), {}, lower_letter)};
#if DXX_USE_SDLMIXER
	if (!is_HMP)
	{
		if (const auto is_MID{std::ranges::equal(extension, SONG_STRING_VIEW(SONG_EXT_MID), {}, lower_letter)})
			return is_MID;
		if (const auto is_OGG{std::ranges::equal(extension, SONG_STRING_VIEW(SONG_EXT_OGG), {}, lower_letter)})
			return is_OGG;
		if (const auto is_FLAC{std::ranges::equal(extension, SONG_STRING_VIEW(SONG_EXT_FLAC), {}, lower_letter)})
			return is_FLAC;
		if (const auto is_MP3{std::ranges::equal(extension, SONG_STRING_VIEW(SONG_EXT_MP3), {}, lower_letter)})
			return is_MP3;
	}
#endif
#undef SONG_STRING_VIEW
#undef SONG_STRING_VIEW_CONCAT_UDL
	return is_HMP;
}

template <std::size_t N>
requires(
	/* Compile-time check that the input string will not overflow the output
	 * buffer.
	 */
	requires(bim_song_info song) {
		requires(std::size(song.filename) >= N);
	}
)
static inline void assign_builtin_song(bim_song_info &song, const char (&str)[N])
{
	std::ranges::copy(str, std::ranges::begin(song.filename));
}

static void add_song(deferred_physfs_pathname &pathname, const unsigned lineno, std::vector<bim_song_info> &songs, const std::ranges::subrange<std::span<char>::iterator> entry_input)
{
	/* Some versions of `descent.sng` are not a simple list of songs, but
	 * instead have a song, whitespace, and then other text on the line.  The
	 * original parser used sscanf to extract the filename, which caused the
	 * extraction to stop on the first whitespace.  Implement an equivalent
	 * early stop here, by scanning the input range for whitespace.  When no
	 * whitespace is found, `input == entry_input`.
	 */
	const std::ranges::subrange<std::span<char>::iterator> input{
		entry_input.begin(),
		std::ranges::find_if(entry_input, [](const char c) { return std::isspace(static_cast<unsigned>(c)); })
	};
	static constexpr std::size_t maximum_song_filename_characters_to_print{20 /* arbitrary cap to balance full output versus flooding the user's terminal */};
	const auto clamp_filename_characters_count{[](const std::size_t actual_size) -> int {
		return static_cast<int>(std::min(actual_size, maximum_song_filename_characters_to_print));
	}};
	const auto get_filename_truncation_hint_text{[](const std::size_t actual_size) {
		return actual_size > maximum_song_filename_characters_to_print ? " (truncated)" : "";
	}};
	const std::size_t input_size{input.size()};
	if (const std::size_t limit{std::tuple_size<decltype(bim_song_info::filename)>::value - 1}; input_size > limit)
	{
		/* Drop excessively long filenames. */
		con_printf(CON_NORMAL, "warning: song file \"%s\"/\"%s\":%u: ignoring excessively long filename; maximum allowed length is %" DXX_PRI_size_type ", but found %" DXX_PRI_size_type " (\"%.*s\"%s)", pathname.get_dirname(), pathname.physfs_virtual_path, lineno, limit, input_size, clamp_filename_characters_count(input_size), input.data(), get_filename_truncation_hint_text(input_size));
		return;
	}
	auto &&reverse_input{std::ranges::views::reverse(input)};
	if (const auto &&ri_dot{std::ranges::find(reverse_input, '.')}; ri_dot == reverse_input.end())
	{
		/* Drop filenames that lack a dot. */
		con_printf(CON_NORMAL, "warning: song file \"%s\"/\"%s\":%u: ignoring filename with no dot (\"%.*s\"%s[%" DXX_PRI_size_type "])", pathname.get_dirname(), pathname.physfs_virtual_path, lineno, clamp_filename_characters_count(input_size), input.data(), get_filename_truncation_hint_text(input_size), input_size);
		return;
	}
	/* Due to `std::reverse_iterator` rules, `reverse_iterator::operator*()`
	 * returns `*std::prev(reverse_iterator::base())`, so `find` returns an
	 * iterator one forward of the '.'.  Therefore, `.base()` points to the
	 * first character in the extension, not to the dot.
	 */
	else if (const std::ranges::subrange extension{ri_dot.base(), input.end()}; !is_valid_song_extension(extension))
	{
		/* Drop filenames that lack a valid song extension. */
#if DXX_USE_SDLMIXER
#define DXX_ADD_SONG_EXTENSION_HINT	"s: " SONG_EXT_MID ", " SONG_EXT_OGG ", " SONG_EXT_FLAC ", " SONG_EXT_MP3 ", "
#else
#define DXX_ADD_SONG_EXTENSION_HINT	": "
#endif
		const std::size_t extension_size{extension.size()};
		con_printf(CON_NORMAL, "warning: song file \"%s\"/\"%s\":%u: ignoring filename with unacceptable filename extension (\"%.*s\"%s[%" DXX_PRI_size_type "]) on filename (\"%.*s\"%s[%" DXX_PRI_size_type "]); acceptable value" DXX_ADD_SONG_EXTENSION_HINT SONG_EXT_HMP, pathname.get_dirname(), pathname.physfs_virtual_path, lineno, clamp_filename_characters_count(extension_size), extension.data(), get_filename_truncation_hint_text(extension_size), extension_size, clamp_filename_characters_count(input_size), input.data(), get_filename_truncation_hint_text(input_size), input_size);
#undef DXX_ADD_SONG_EXTENSION_HINT
		return;
	}
	/* The input is not a `bim_song_info`, so a direct use of
	 * `emplace_back(input)` is not well-formed.
	 *
	 * Using `emplace_back(bim_song_info(...))` is well-formed, but constructs
	 * a temporary to pass to emplace_back, which then copy-constructs the
	 * vector element.
	 *
	 * Pass an instance of an object that is convertible to the element_type,
	 * and rely on copy elision to construct the converted value directly into
	 * the vector element.
	 */
	struct emplace_subrange
	{
		std::ranges::subrange<std::span<char>::iterator> input;
		operator bim_song_info() const
		{
			/* `input` does not include a terminating null, so zero initialize
			 * `r` before copying `input` into it.  Zeroing the entire buffer
			 * is cheap, and avoids a lookup of the length of the input buffer.
			 */
			bim_song_info r{};
			std::ranges::copy(input, std::ranges::begin(r.filename));
			return r;
		}
	};
	auto &e{songs.emplace_back(emplace_subrange{input})};
	assert(!e.filename.back());
	/* Check verbosity before preparing arguments, so that the dirname is not
	 * computed if the con_printf call will not use it.
	 */
	if (CON_VERBOSE <= CGameArg.DbgVerbose)
		con_printf(CON_VERBOSE, "note: song file \"%s\"/\"%s\":%u: adding filename \"%s\"", pathname.get_dirname(), pathname.physfs_virtual_path, lineno, e.filename.data());
}

}

}

namespace dsx {

namespace {

// Set up everything for our music
// NOTE: you might think this is done once per runtime but it's not! It's done for EACH song so that each mission can have it's own descent.sng structure. We COULD optimize that by only doing this once per mission.
static void songs_init()
{
	using namespace std::string_view_literals;

	BIMSongs.reset();
	BIMSecretSongs.reset();

	// try dxx-r.sng - a songfile specifically for dxx which level authors CAN use (dxx does not care if descent.sng contains MP3/OGG/etc. as well) besides the normal descent.sng containing files other versions of the game cannot play. this way a mission can contain a DOS-Descent compatible OST (hmp files) as well as a OST using MP3, OGG, etc.
	auto fp = PHYSFSX_openReadBuffered("dxx-r.sng").first;

	const bool canUseExtensions{!!fp};	// can use extensions ONLY if dxx-r.sng
	if (!fp) // try to open regular descent.sng
		fp = PHYSFSX_openReadBuffered("descent.sng").first;

	if (!fp) // No descent.sng available. Define a default song-set
	{
		constexpr std::size_t predef{30}; // define 30 songs - period

		user_configured_level_songs builtin_songs{predef};

		assign_builtin_song(builtin_songs[song_number::title], "descent.hmp");
		assign_builtin_song(builtin_songs[song_number::briefing], "briefing.hmp");
		assign_builtin_song(builtin_songs[song_number::credits], "credits.hmp");
		assign_builtin_song(builtin_songs[song_number::endlevel], "endlevel.hmp");	// can't find it? give a warning
		assign_builtin_song(builtin_songs[song_number::endgame], "endgame.hmp");	// ditto

		for (const auto i : constant_xrange<unsigned, 0u, predef>{})
		{
			auto &s{builtin_songs[build_song_number_from_level_song_number(level_song_number{i})]};
			const auto d{s.filename.data()};
			const auto fs{s.filename.size()};
			std::snprintf(d, fs, "game%02u.hmp", i + 1);
			if (PHYSFSX_exists_ignorecase(d))
				continue;
			std::snprintf(d, fs, "game%u.hmp", i);
			if (PHYSFSX_exists_ignorecase(d))
				continue;
			else
			{
				s = {};	// music not available
				break;
			}
		}
		BIMSongs = std::move(builtin_songs);
		/* BIMSecretSongs remains unset.  There is no built-in default song
		 * list for secret levels.
		 */
	}
	else
	{
		const auto use_secret_songs{[](const bool canUseExtensions, const PHYSFSX_fgets_t::result &result) {
			if (!canUseExtensions)
				return canUseExtensions;
			/* If extensions are allowed, check whether this line uses an
			 * extension directive.
			 */
			if (constexpr std::string_view secret_label{"!Rebirth.secret "sv}; result.size() > secret_label.size() &&
				std::ranges::equal(std::span(result.begin(), secret_label.size()), secret_label))
				return canUseExtensions;
			return false;
		}};
		std::vector<bim_song_info> main_songs, secret_songs;
		unsigned lineno{0};
		deferred_physfs_pathname deferred_pathname{fp.filename};
		for (PHYSFSX_gets_line_t<81> inputline; const auto result{PHYSFSX_fgets(inputline, fp)};)
		{
			++lineno;
			if (result.empty())
				continue;
			add_song(deferred_pathname, lineno, use_secret_songs(canUseExtensions, result) ? secret_songs : main_songs, result);
		}
#if defined(DXX_BUILD_DESCENT_I)
		// HACK: If Descent.hog is patched from 1.0 to 1.5, descent.sng is turncated. So let's patch it up here
		constexpr std::size_t truncated_song_count = 12;
		if (!canUseExtensions &&
			main_songs.size() == truncated_song_count &&
			PHYSFS_fileLength(fp) == 422)
		{
			main_songs.resize(truncated_song_count + 15);
			for (const auto i : constant_xrange<unsigned, truncated_song_count, 27>{})
			{
				auto &s = main_songs[i];
				snprintf(std::data(s.filename), std::size(s.filename), "game%02u.hmp", i - 4);
			}
		}
#endif
		BIMSongs = std::move(main_songs);
		BIMSecretSongs = std::move(secret_songs);
	}

	fp.reset();

	if (CGameArg.SndNoMusic)
		CGameCfg.MusicType = music_type::None;

	// If SDL_Mixer is not supported (or deactivated), switch to no-music type if SDL_mixer-related music type was selected
	else if (CGameArg.SndDisableSdlMixer)
	{
#ifndef _WIN32
		if (CGameCfg.MusicType == music_type::Builtin)
			CGameCfg.MusicType = music_type::None;
#endif
		if (CGameCfg.MusicType == music_type::Custom)
			CGameCfg.MusicType = music_type::None;
	}

	switch (CGameCfg.MusicType)
	{
		case music_type::None:
		case music_type::Builtin:
			break;
#if DXX_USE_SDL_REDBOOK_AUDIO
		case music_type::Redbook:
			RBAInit();
			break;
#endif
		case music_type::Custom:
			jukebox_load();
			break;
	}

	songs_set_volume(CGameCfg.MusicVolume);
}

}

}

void songs_uninit()
{
	songs_stop_all();
#if DXX_USE_SDLMIXER
	jukebox_unload();
#endif
	BIMSecretSongs.reset();
	BIMSongs.reset();
}

//stop any songs - builtin, redbook or jukebox - that are currently playing
void songs_stop_all(void)
{
#ifdef _WIN32
	digi_win32_stop_midi_song();	// Stop midi song, if playing
#endif
#if DXX_USE_SDL_REDBOOK_AUDIO
	RBAStop();
#endif
#if DXX_USE_SDLMIXER
	mix_stop_music();
#endif

	Song_playing = song_number::None;
}

void songs_pause(void)
{
#ifdef _WIN32
	digi_win32_pause_midi_song();
#endif
#if DXX_USE_SDL_REDBOOK_AUDIO
	if (CGameCfg.MusicType == music_type::Redbook)
		RBAPause();
#endif
#if DXX_USE_SDLMIXER
	mix_pause_music();
#endif
}

void songs_resume(void)
{
#ifdef _WIN32
	digi_win32_resume_midi_song();
#endif
#if DXX_USE_SDL_REDBOOK_AUDIO
	if (CGameCfg.MusicType == music_type::Redbook)
		RBAResume();
#endif
#if DXX_USE_SDLMIXER
	mix_resume_music();
#endif
}

void songs_pause_resume(void)
{
#if DXX_USE_SDL_REDBOOK_AUDIO
	if (CGameCfg.MusicType == music_type::Redbook)
		RBAPauseResume();
#endif
#if DXX_USE_SDLMIXER
	mix_pause_resume_music();
#endif
}

#if defined(DXX_BUILD_DESCENT_I)
/*
 * This list may not be exhaustive!!
 */
#define D1_MAC_OEM_DISCID       0xde0feb0e // Descent CD that came with the Mac Performa 6400, hope mine isn't scratched [too much]

#if DXX_USE_SDL_REDBOOK_AUDIO
#define REDBOOK_ENDLEVEL_TRACK		4
#define REDBOOK_ENDGAME_TRACK		(RBAGetNumberOfTracks())
#define REDBOOK_FIRST_LEVEL_TRACK	(songs_have_cd() ? 6 : 1)
#endif
#elif defined(DXX_BUILD_DESCENT_II)
/*
 * Some of these have different Track listings!
 * Which one is the "correct" order?
 */
#define D2_1_DISCID         0x7d0ff809 // Descent II
#define D2_2_DISCID         0xe010a30e // Descent II
#define D2_3_DISCID         0xd410070d // Descent II
#define D2_4_DISCID         0xc610080d // Descent II
#define D2_DEF_DISCID       0x87102209 // Definitive collection Disc 2
#define D2_OEM_DISCID       0xac0bc30d // Destination: Quartzon
#define D2_OEM2_DISCID      0xc40c0a0d // Destination: Quartzon
#define D2_VERTIGO_DISCID   0x53078208 // Vertigo
#define D2_VERTIGO2_DISCID  0x64071408 // Vertigo + DMB
#define D2_MAC_DISCID       0xb70ee40e // Macintosh
#define D2_IPLAY_DISCID     0x22115710 // iPlay for Macintosh

#if DXX_USE_SDL_REDBOOK_AUDIO
#define REDBOOK_TITLE_TRACK         2
#define REDBOOK_CREDITS_TRACK       3
#define REDBOOK_FIRST_LEVEL_TRACK   (songs_have_cd() ? 4 : 1)
#endif
#endif

// 0 otherwise
namespace dsx {
namespace {
#if DXX_USE_SDL_REDBOOK_AUDIO
static int songs_have_cd()
{
	unsigned long discid;

	if (CGameCfg.OrigTrackOrder)
		return 1;

	if (!(CGameCfg.MusicType == music_type::Redbook))
		return 0;

	discid = RBAGetDiscID();
	con_printf(CON_DEBUG, "CD-ROM disc ID is 0x%08lx", discid);

	switch (discid) {
#if defined(DXX_BUILD_DESCENT_I)
		case D1_MAC_OEM_DISCID:	// Doesn't work with your Mac Descent CD? Please tell!
			return 1;
#elif defined(DXX_BUILD_DESCENT_II)
	case D2_1_DISCID:
	case D2_2_DISCID:
	case D2_3_DISCID:
	case D2_4_DISCID:
	case D2_DEF_DISCID:
	case D2_OEM_DISCID:
	case D2_OEM2_DISCID:
	case D2_VERTIGO_DISCID:
	case D2_VERTIGO2_DISCID:
	case D2_MAC_DISCID:
	case D2_IPLAY_DISCID:
		return 1;
#endif
		default:
			return 0;
	}
}
#endif

#if defined(DXX_BUILD_DESCENT_I)
#if DXX_USE_SDL_REDBOOK_AUDIO
static void redbook_repeat_func()
{
	pause_game_world_time p;
	RBAPlayTracks(Redbook_playing, 0, redbook_repeat_func);
}
#endif
#elif defined(DXX_BUILD_DESCENT_II)
#if DXX_USE_SDL_REDBOOK_AUDIO || DXX_USE_SDLMIXER
static void play_credits_track()
{
	pause_game_world_time p;
	songs_play_song(song_number::credits, 1);
}
#endif
#endif

#if DXX_USE_SDL_REDBOOK_AUDIO
static void play_redbook_track_if_available(const song_number songnum, const int redbook_track, const int num_tracks, const int repeat)
{
	if (redbook_track > num_tracks)
		return;
#if defined(DXX_BUILD_DESCENT_I)
	constexpr auto repeat_func{&redbook_repeat_func};
#elif defined(DXX_BUILD_DESCENT_II)
	constexpr auto repeat_func{&play_credits_track};
#endif
	if (RBAPlayTracks(redbook_track, redbook_track, repeat ? repeat_func : nullptr))
	{
		Redbook_playing = redbook_track;
		Song_playing = songnum;
	}
}
#endif

}

}

// play a filename as music, depending on filename extension.
int songs_play_file(const char *filename, int repeat, void (*hook_finished_track)())
{
	songs_stop_all();
#if defined(_WIN32) || DXX_USE_SDLMIXER
	const char *fptr = strrchr(filename, '.');
	if (fptr == NULL)
		return 0;

	++ fptr;
	if (!d_stricmp(fptr, SONG_EXT_HMP))
	{
#if defined(_WIN32)
		return digi_win32_play_midi_song( filename, repeat );
#elif DXX_USE_SDLMIXER
		return mix_play_file( filename, repeat, hook_finished_track );
#else
		return 0;
#endif
	}
#if DXX_USE_SDLMIXER
	else if ( !d_stricmp(fptr, SONG_EXT_MID) ||
			!d_stricmp(fptr, SONG_EXT_OGG) ||
			!d_stricmp(fptr, SONG_EXT_FLAC) ||
			!d_stricmp(fptr, SONG_EXT_MP3) )
	{
		return mix_play_file( filename, repeat, hook_finished_track );
	}
#endif
#endif
	return 0;
}

namespace dsx {

void songs_play_song(const song_number songnum, const int repeat)
{
	songs_init();

	switch (CGameCfg.MusicType)
	{
		case music_type::Builtin:
		{
			if (underlying_value(songnum) >= BIMSongs.size())
				break;
			// EXCEPTION: If song_number::endlevel is not available, continue playing level song.
			auto &s = BIMSongs[songnum];
			if (songnum == song_number::endlevel && Song_playing >= song_number::first_level_song && !PHYSFSX_exists_ignorecase(s.filename.data()))
				return;

			Song_playing = song_number::None;
			if (songs_play_file(std::data(s.filename), repeat, nullptr))
				Song_playing = songnum;
			break;
		}
#if DXX_USE_SDL_REDBOOK_AUDIO
		case music_type::Redbook:
		{
			const auto num_tracks{RBAGetNumberOfTracks()};

#if defined(DXX_BUILD_DESCENT_I)
			Song_playing = song_number::None;
			if (songnum < song_number::endgame)
			{
				play_redbook_track_if_available(songnum, underlying_value(songnum) + 2, num_tracks, repeat);
			}
			else if (songnum == song_number::endgame) // The endgame track is the last track
			{
				play_redbook_track_if_available(songnum, REDBOOK_ENDGAME_TRACK, num_tracks, repeat);
			}
			else if (songnum > song_number::endgame)
			{
				play_redbook_track_if_available(songnum, underlying_value(songnum) + 1, num_tracks, repeat);
			}
#elif defined(DXX_BUILD_DESCENT_II)
			// keep playing current music if chosen song is unavailable (e.g. SONG_ENDLEVEL)
			if (songnum == song_number::title)
			{
				play_redbook_track_if_available(songnum, REDBOOK_TITLE_TRACK, num_tracks, repeat);
			}
			else if (songnum == song_number::credits)
			{
				play_redbook_track_if_available(songnum, REDBOOK_CREDITS_TRACK, num_tracks, repeat);
			}
#endif
			break;
		}
#endif
#if DXX_USE_SDLMIXER
		case music_type::Custom:
		{
			// EXCEPTION: If song_number::endlevel is undefined, continue playing level song.
			if (songnum == song_number::endlevel && Song_playing >= song_number::first_level_song && !CGameCfg.CMMiscMusic[songnum].front())
				return;

			Song_playing = song_number::None;
#if defined(DXX_BUILD_DESCENT_I)
			int play = songs_play_file(CGameCfg.CMMiscMusic[songnum].data(), repeat, NULL);
#elif defined(DXX_BUILD_DESCENT_II)
			const auto use_credits_track{songnum == song_number::title && CGameCfg.OrigTrackOrder};
			int play = songs_play_file(CGameCfg.CMMiscMusic[songnum].data(),
							  // Play the credits track after the title track and loop the credits track if original CD track order was chosen
							  use_credits_track ? 0 : repeat,
							  use_credits_track ? play_credits_track : NULL);
#endif
			if (play)
				Song_playing = songnum;
			break;
		}
#endif
		default:
			Song_playing = song_number::None;
			break;
	}

	return;
}

#if DXX_USE_SDL_REDBOOK_AUDIO
namespace {
static void redbook_first_song_func()
{
	pause_game_world_time p;
	Song_playing = song_number::None; // Playing Redbook tracks will not modify Song_playing. To repeat we must reset this so songs_play_level_song does not think we want to re-play the same song again.
	songs_play_level_song(1, 0);
}
}
#endif

// play track given by levelnum (depending on the music type and it's playing behaviour) or increment/decrement current track number via offset value

void songs_play_level_song(int levelnum, int offset)
{
	Assert( levelnum != 0 );

	songs_init();
	const level_song_number level_songnum{(levelnum > 0) ? static_cast<unsigned>(levelnum - 1) : static_cast<unsigned>(-levelnum)};

	switch (CGameCfg.MusicType)
	{
		case music_type::Builtin:
		{
			if (offset)
				return;

			Song_playing = song_number::None;
			if (levelnum < 0)
			{
				/* Secret songs are processed separately and do not need
				 * to exclude non-levels.  Secret songs start at offset 0, so a
				 * test for `size == 0` is sufficient.
				 */
				if (const auto size{BIMSecretSongs.size()})
				{
					const song_number secretsongnum{static_cast<unsigned>((-levelnum - 1) % size)};
					if (songs_play_file(BIMSecretSongs[secretsongnum].filename.data(), 1, nullptr))
						Song_playing = secretsongnum;
				}
				break;
			}
			/* Level songs start at offset `song_number::first_level_song`, so
			 * require that the song list contain more than that many entries.
			 * A list with at most `song_number::first_level_song` entries only
			 * has non-level songs, so nothing can be played for level songs.
			 */
			if (const auto size{BIMSongs.size()}; size > static_cast<std::size_t>(song_number::first_level_song))
			{
				/* Compute `count_level_songs` to exclude songs assigned to
				 * non-levels, such as title, briefing, etc.
				 */
				const auto count_level_songs{size - static_cast<std::size_t>(song_number::first_level_song)};
				const song_number songnum{build_song_number_from_level_song_number(static_cast<level_song_number>(underlying_value(level_songnum) % count_level_songs))};
				if (songs_play_file(BIMSongs[songnum].filename.data(), 1, nullptr))
					Song_playing = songnum;
			}
			break;
		}
#if DXX_USE_SDL_REDBOOK_AUDIO
		case music_type::Redbook:
		{
			const auto n_tracks{RBAGetNumberOfTracks()};
			int tracknum;
			const song_number songnum{build_song_number_from_level_song_number(level_songnum)};

			if (!offset)
			{
				// we have just been told to play the same as we do already -> ignore
				if (Song_playing >= song_number::first_level_song && songnum == Song_playing)
					return;

#if defined(DXX_BUILD_DESCENT_I)
				const int bn_tracks{n_tracks};
#elif defined(DXX_BUILD_DESCENT_II)
				const int bn_tracks{n_tracks + 1};
#endif
				if (bn_tracks < REDBOOK_FIRST_LEVEL_TRACK)
					/* If there are insufficient tracks to have any level songs, break */
					break;
				tracknum = REDBOOK_FIRST_LEVEL_TRACK + (
					(bn_tracks <= REDBOOK_FIRST_LEVEL_TRACK)
					/* If there is exactly one level track, pick level track 0.
					 */
					? 0
					/* Otherwise, pick the appropriate song, and wrap back to
					 * the start if there are too few tracks to give each level
					 * a unique song.
					 */
					: (underlying_value(level_songnum) % (bn_tracks - REDBOOK_FIRST_LEVEL_TRACK))
				);
			}
			else
			{
				tracknum = Redbook_playing+offset;
				if (tracknum < REDBOOK_FIRST_LEVEL_TRACK)
					tracknum = n_tracks - (REDBOOK_FIRST_LEVEL_TRACK - tracknum) + 1;
				else if (tracknum > n_tracks)
					tracknum = REDBOOK_FIRST_LEVEL_TRACK + (tracknum - n_tracks) - 1;
			}

			Song_playing = song_number::None;
			if (RBAEnabled() && (tracknum <= n_tracks))
			{
#if defined(DXX_BUILD_DESCENT_I)
				const auto have_cd{songs_have_cd()};
				const auto play{RBAPlayTracks(tracknum, !have_cd ? n_tracks : tracknum, have_cd ? redbook_repeat_func : redbook_first_song_func)};
#elif defined(DXX_BUILD_DESCENT_II)
				const auto play{RBAPlayTracks(tracknum, n_tracks, redbook_first_song_func)};
#endif
				if (play)
				{
					Song_playing = songnum;
					Redbook_playing = tracknum;
				}
			}
			break;
		}
#endif
#if DXX_USE_SDLMIXER
		case music_type::Custom:
		{
			if (CGameCfg.CMLevelMusicPlayOrder == LevelMusicPlayOrder::Random)
				CGameCfg.CMLevelMusicTrack[0] = d_rand() % CGameCfg.CMLevelMusicTrack[1]; // simply a random selection - no check if this song has already been played. But that's how I roll!
			else if (!offset)
			{
				if (CGameCfg.CMLevelMusicPlayOrder == LevelMusicPlayOrder::Continuous)
				{
					static level_song_number last_songnum{level_song_number::None};

					if (Song_playing >= song_number::first_level_song)
						return;

					// As soon as we start a new level, go to next track
					if (last_songnum != level_song_number::None && level_songnum != last_songnum)
						((CGameCfg.CMLevelMusicTrack[0] + 1 >= CGameCfg.CMLevelMusicTrack[1])
							? CGameCfg.CMLevelMusicTrack[0] = 0
							: CGameCfg.CMLevelMusicTrack[0]++);
					last_songnum = level_songnum;
				}
				else if (CGameCfg.CMLevelMusicPlayOrder == LevelMusicPlayOrder::Level)
					CGameCfg.CMLevelMusicTrack[0] = underlying_value(level_songnum) % CGameCfg.CMLevelMusicTrack[1];
			}
			else
			{
				CGameCfg.CMLevelMusicTrack[0] += offset;
				if (CGameCfg.CMLevelMusicTrack[0] < 0)
					CGameCfg.CMLevelMusicTrack[0] = CGameCfg.CMLevelMusicTrack[1] + CGameCfg.CMLevelMusicTrack[0];
				if (CGameCfg.CMLevelMusicTrack[0] + 1 > CGameCfg.CMLevelMusicTrack[1])
					CGameCfg.CMLevelMusicTrack[0] = CGameCfg.CMLevelMusicTrack[0] - CGameCfg.CMLevelMusicTrack[1];
			}

			Song_playing = song_number::None;
			if (jukebox_play())
				Song_playing = build_song_number_from_level_song_number(level_songnum);

			break;
		}
#endif
		default:
			Song_playing = song_number::None;
			break;
	}

#if defined(DXX_BUILD_DESCENT_I)
	// If we couldn't play the song, most likely because it wasn't specified, play no music.
	if (Song_playing == song_number::None)
		songs_stop_all();
#endif
	return;
}

}
