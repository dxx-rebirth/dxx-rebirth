/*
 * This file is part of the DXX-Rebirth project <http://www.dxx-rebirth.com/>.
 * It is copyright by its individual contributors, as recorded in the
 * project's Git history.  See COPYING.txt at the top level for license
 * terms and a link to the Git history.
 */
/*
 * DXX Rebirth "jukebox" code
 * MD 2211 <md2211@users.sourceforge.net>, 2007
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "hudmsg.h"
#include "songs.h"
#include "jukebox.h"
#include "dxxerror.h"
#include "console.h"
#include "config.h"
#include "strutil.h"
#include "u_mem.h"

#include "compiler-make_unique.h"
#include "partial_range.h"

#define MUSIC_HUDMSG_MAXLEN 40
#define JUKEBOX_HUDMSG_PLAYING "Now playing:"
#define JUKEBOX_HUDMSG_STOPPED "Jukebox stopped"

namespace {

class list_deleter : std::default_delete<char *[]>, PHYSFS_list_deleter
{
	typedef std::default_delete<char *[]> base_deleter;
public:
	std::unique_ptr<char[]> buf;	// buffer containing song file path text
	void operator()(char **list)
	{
		if (buf)
		{
			buf.reset();
			this->base_deleter::operator()(list);
		}
		else
			this->PHYSFS_list_deleter::operator()(list);
	}
};

class list_pointers : public std::unique_ptr<char *[], list_deleter>
{
	typedef std::unique_ptr<char *[], list_deleter> base_ptr;
public:
	using base_ptr::reset;
	void reset(char **list, std::unique_ptr<char[]> &&buf)
		noexcept(
			noexcept(std::declval<base_ptr>().reset(list)) &&
			noexcept(std::declval<list_deleter>().buf = std::move(buf))
		)
	{
		this->base_ptr::reset(list);
		get_deleter().buf = std::move(buf);
	}
	void reset(PHYSFS_list_t list)
		noexcept(noexcept(std::declval<base_ptr>().reset(list.release())))
	{
		this->base_ptr::reset(list.release());
	}
};

class jukebox_songs
{
	void quick_unload();
public:
	~jukebox_songs();
	void unload();
	list_pointers list;	// the actual list
	int num_songs;	// number of jukebox songs
	static const std::size_t max_songs = 1024;	// maximum number of pointers that 'list' can hold, i.e. size of list / size of one pointer
};

}

static jukebox_songs JukeboxSongs;

jukebox_songs::~jukebox_songs()
{
	quick_unload();
}

void jukebox_songs::quick_unload()
{
	list.reset();
}

void jukebox_songs::unload()
{
	quick_unload();
	num_songs = 0;
}

void jukebox_unload()
{
	JukeboxSongs.unload();
}

const array<file_extension_t, 6> jukebox_exts{SONG_EXT_HMP, SONG_EXT_MID, SONG_EXT_OGG, SONG_EXT_FLAC, SONG_EXT_MP3};

static int read_m3u(void)
{
	FILE *fp;
	std::size_t length;
	char *buf;
	array<char, PATH_MAX> absbuf;
	if (PHYSFSX_exists(GameCfg.CMLevelMusicPath.data(), 0)) // it's a child of Sharepath, build full path
		PHYSFSX_getRealPath(GameCfg.CMLevelMusicPath.data(), buf = absbuf.data());
	else
		buf = GameCfg.CMLevelMusicPath.data();
	fp = fopen(buf, "rb");
	if (!fp)
		return 0;
	
	fseek( fp, -1, SEEK_END );
	length = ftell(fp) + 1;
	auto list_buf = make_unique<char[]>(length + 1);
	fseek(fp, 0, SEEK_SET);
	if (!fread(list_buf.get(), length, 1, fp))
	{
		fclose(fp);
		return 0;
	}

	fclose(fp);		// Finished with it

	// The growing string list is allocated last, hopefully reducing memory fragmentation when it grows
	list_buf[length] = '\0';	// make sure the last string is terminated
	auto &&range = unchecked_partial_range(list_buf.get(), length);
	const auto eol = [](char c) {
		return c == '\n' || c == '\r' || !c;
	};
	JukeboxSongs.list.reset(new char *[JukeboxSongs.max_songs], std::move(list_buf));
	for (auto buf = range.begin(); buf != range.end(); ++buf)
	{
		for (; buf != range.end() && eol(*buf);)	// find new line - support DOS, Unix and Mac line endings
			buf++;
		if (buf == range.end())
			break;
		if (*buf != '#')	// ignore comments / extra info
		{
			JukeboxSongs.list[JukeboxSongs.num_songs++] = buf;
			if (JukeboxSongs.num_songs >= JukeboxSongs.max_songs)
				break;
		}
		for (; buf != range.end(); ++buf)	// find end of line
			if (eol(*buf))
			{
				*buf = 0;
				break;
			}
		if (buf == range.end())
			break;
	}
	return 1;
}

/* Loads music file names from a given directory or M3U playlist */
void jukebox_load()
{
	jukebox_unload();

	// Check if it's an M3U file
	size_t musiclen = strlen(GameCfg.CMLevelMusicPath.data());
	if (musiclen > 4 && !d_stricmp(&GameCfg.CMLevelMusicPath[musiclen - 4], ".m3u"))
		read_m3u();
	else	// a directory
	{
		int new_path = 0;
		const char *sep = PHYSFS_getDirSeparator();
		size_t seplen = strlen(sep);
		int i;

		// stick a separator on the end if necessary.
		if (musiclen >= seplen)
		{
			auto p = &GameCfg.CMLevelMusicPath[musiclen - seplen];
			if (strcmp(p, sep))
				GameCfg.CMLevelMusicPath.copy_if(musiclen, sep, seplen);
		}

		// Read directory using PhysicsFS
		if (PHYSFS_isDirectory(GameCfg.CMLevelMusicPath.data()))	// find files in relative directory
			JukeboxSongs.list.reset(PHYSFSX_findFiles(GameCfg.CMLevelMusicPath.data(), jukebox_exts));
		else
		{
			new_path = PHYSFSX_isNewPath(GameCfg.CMLevelMusicPath.data());
			PHYSFS_addToSearchPath(GameCfg.CMLevelMusicPath.data(), 0);

			// as mountpoints are no option (yet), make sure only files originating from GameCfg.CMLevelMusicPath are aded to the list.
			JukeboxSongs.list.reset(PHYSFSX_findabsoluteFiles("", GameCfg.CMLevelMusicPath.data(), jukebox_exts));
		}

		if (!JukeboxSongs.list)
		{
			if (new_path)
				PHYSFS_removeFromSearchPath(GameCfg.CMLevelMusicPath.data());
			return;
		}
		
		for (i = 0; JukeboxSongs.list[i]; i++) {}
		JukeboxSongs.num_songs = i;

		if (new_path)
			PHYSFS_removeFromSearchPath(GameCfg.CMLevelMusicPath.data());
	}

	if (JukeboxSongs.num_songs)
	{
		con_printf(CON_DEBUG,"Jukebox: %d music file(s) found in %s", JukeboxSongs.num_songs, GameCfg.CMLevelMusicPath.data());
		if (GameCfg.CMLevelMusicTrack[1] != JukeboxSongs.num_songs)
		{
			GameCfg.CMLevelMusicTrack[1] = JukeboxSongs.num_songs;
			GameCfg.CMLevelMusicTrack[0] = 0; // number of songs changed so start from beginning.
		}
	}
	else
	{
		GameCfg.CMLevelMusicTrack[0] = -1;
		GameCfg.CMLevelMusicTrack[1] = -1;
		con_printf(CON_DEBUG,"Jukebox music could not be found!");
	}
}

// To proceed tru our playlist. Usually used for continous play, but can loop as well.
static void jukebox_hook_next()
{
	if (!JukeboxSongs.list || GameCfg.CMLevelMusicTrack[0] == -1) return;

	if (GameCfg.CMLevelMusicPlayOrder == MUSIC_CM_PLAYORDER_RAND)
		GameCfg.CMLevelMusicTrack[0] = d_rand() % GameCfg.CMLevelMusicTrack[1]; // simply a random selection - no check if this song has already been played. But that's how I roll!
	else
		GameCfg.CMLevelMusicTrack[0]++;
	if (GameCfg.CMLevelMusicTrack[0] + 1 > GameCfg.CMLevelMusicTrack[1])
		GameCfg.CMLevelMusicTrack[0] = 0;

	jukebox_play();
}

// Play tracks from Jukebox directory. Play track specified in GameCfg.CMLevelMusicTrack[0] and loop depending on GameCfg.CMLevelMusicPlayOrder
int jukebox_play()
{
	const char *music_filename;
	uint_fast32_t size_full_filename = 0;

	if (!JukeboxSongs.list)
		return 0;

	if (GameCfg.CMLevelMusicTrack[0] < 0 || GameCfg.CMLevelMusicTrack[0] + 1 > GameCfg.CMLevelMusicTrack[1])
		return 0;

	music_filename = JukeboxSongs.list[GameCfg.CMLevelMusicTrack[0]];
	if (!music_filename)
		return 0;

	size_t size_music_filename = strlen(music_filename);
	size_t musiclen = strlen(GameCfg.CMLevelMusicPath.data());
	size_full_filename = musiclen + size_music_filename + 1;
	RAIIdmem<char[]> full_filename;
	CALLOC(full_filename, char[], size_full_filename);
	const char *LevelMusicPath;
	if (musiclen > 4 && !d_stricmp(&GameCfg.CMLevelMusicPath[musiclen - 4], ".m3u"))	// if it's from an M3U playlist
		LevelMusicPath = "";
	else											// if it's from a specified path
		LevelMusicPath = GameCfg.CMLevelMusicPath.data();
	snprintf(full_filename, size_full_filename, "%s%s", LevelMusicPath, music_filename);

	int played = songs_play_file(full_filename, ((GameCfg.CMLevelMusicPlayOrder == MUSIC_CM_PLAYORDER_LEVEL)?1:0), ((GameCfg.CMLevelMusicPlayOrder == MUSIC_CM_PLAYORDER_LEVEL)?NULL:jukebox_hook_next));
	full_filename = NULL;
	if (!played)
	{
		return 0;	// whoops, got an error
	}

	// Formatting a pretty message
	const char *prefix = "...";
	if (size_music_filename >= MUSIC_HUDMSG_MAXLEN) {
		music_filename += size_music_filename - MUSIC_HUDMSG_MAXLEN;
	} else {
		prefix += 3;
	}

	HUD_init_message(HM_DEFAULT, "%s %s%s", JUKEBOX_HUDMSG_PLAYING, prefix, music_filename);

	return 1;
}

char *jukebox_current() {
	return JukeboxSongs.list[GameCfg.CMLevelMusicTrack[0]];
}

int jukebox_is_loaded() { return (JukeboxSongs.list != NULL); }
int jukebox_is_playing() { return GameCfg.CMLevelMusicTrack[0] + 1; }
int jukebox_numtracks() { return GameCfg.CMLevelMusicTrack[1]; }

void jukebox_list() {
	int i;
	if (!JukeboxSongs.list) return;
	if (!JukeboxSongs.list[0]) {
		con_printf(CON_DEBUG,"* No songs have been found");
	}
	else {
		for (i = 0; i < GameCfg.CMLevelMusicTrack[1]; i++)
			con_printf(CON_DEBUG,"* %s", JukeboxSongs.list[i]);
	}
}
