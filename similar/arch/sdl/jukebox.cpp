/*
 * This file is part of the DXX-Rebirth project <https://www.dxx-rebirth.com/>.
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
#include "physfs_list.h"
#include "digi.h"

#include "partial_range.h"
#include <memory>

namespace dcx {

#define MUSIC_HUDMSG_MAXLEN 40
#define JUKEBOX_HUDMSG_PLAYING "Now playing:"
#define JUKEBOX_HUDMSG_STOPPED "Jukebox stopped"

namespace {

struct m3u_bytes
{
	using range_type = partial_range_t<char *>;
	using ptr_range_type = partial_range_t<char **>;
	using alloc_type = std::unique_ptr<char *[]>;
	range_type range = {nullptr, nullptr};
	ptr_range_type ptr_range = {nullptr, nullptr};
	alloc_type alloc;
	m3u_bytes() = default;
	m3u_bytes(m3u_bytes &&) = default;
	m3u_bytes(range_type &&r, ptr_range_type &&p, alloc_type &&b) :
		range(std::move(r)),
		ptr_range(std::move(p)),
		alloc(std::move(b))
	{
	}
};

class FILE_deleter
{
public:
	void operator()(FILE *const p) const
	{
		fclose(p);
	}
};

class list_deleter : PHYSFS_list_deleter
{
public:
	/* When `list_pointers` is a PHYSFS allocation, `buf` is nullptr.
	 * When `list_pointers` is a new[char *[]] allocation, `buf`
	 * points to the same location as `list_pointers`.
	 */
	std::unique_ptr<char *[]> buf;
	void operator()(char **list)
	{
		if (buf)
		{
			assert(buf.get() == list);
			buf.reset();
		}
		else
			this->PHYSFS_list_deleter::operator()(list);
	}
};

class list_pointers : public PHYSFSX_uncounted_list_template<list_deleter>
{
	typedef PHYSFSX_uncounted_list_template<list_deleter> base_ptr;
public:
	using base_ptr::reset;
	void set_combined(std::unique_ptr<char *[]> &&buf)
		noexcept(
			noexcept(std::declval<base_ptr>().reset(buf.get())) &&
			noexcept(std::declval<list_deleter>().buf = std::move(buf))
		)
	{
		this->base_ptr::reset(buf.get());
		get_deleter().buf = std::move(buf);
	}
	void reset(PHYSFSX_uncounted_list list)
		noexcept(noexcept(std::declval<base_ptr>().reset(list.release())))
	{
		this->base_ptr::reset(list.release());
	}
};

class jukebox_songs
{
public:
	void unload();
	list_pointers list;	// the actual list
	unsigned num_songs;	// number of jukebox songs
	static const std::size_t max_songs = 1024;	// maximum number of pointers that 'list' can hold, i.e. size of list / size of one pointer
};

}

static jukebox_songs JukeboxSongs;

void jukebox_songs::unload()
{
	num_songs = 0;
	list.reset();
}

void jukebox_unload()
{
	JukeboxSongs.unload();
}

const std::array<file_extension_t, 5> jukebox_exts{{
	SONG_EXT_HMP,
	SONG_EXT_MID,
	SONG_EXT_OGG,
	SONG_EXT_FLAC,
	SONG_EXT_MP3
}};

/* Open an m3u using fopen, not PHYSFS.  If the path seems to be under
 * PHYSFS, that will be preferred over a raw filesystem path.
 */
static std::unique_ptr<FILE, FILE_deleter> open_m3u_from_disk(const char *const cfgpath)
{
	std::array<char, PATH_MAX> absbuf;
	return std::unique_ptr<FILE, FILE_deleter>(fopen(
	// it's a child of Sharepath, build full path
		(PHYSFSX_exists(cfgpath, 0)
			? (PHYSFSX_getRealPath(cfgpath, absbuf), absbuf.data())
			: cfgpath), "rb")
	);
}

static m3u_bytes read_m3u_bytes_from_disk(const char *const cfgpath)
{
	const auto &&f = open_m3u_from_disk(cfgpath);
	if (!f)
		return {};
	const auto fp = f.get();
	fseek(fp, -1, SEEK_END);
	const std::size_t length = ftell(fp) + 1;
	const auto juke_max_songs = JukeboxSongs.max_songs;
	if (length >= PATH_MAX * juke_max_songs)
		return {};
	fseek(fp, 0, SEEK_SET);
	/* A file consisting only of single character records and newline
	 * separators, with no junk newlines, comments, or final terminator,
	 * will need one pointer per two bytes of file, rounded up.  Any
	 * file that uses longer records, which most will use, will need
	 * fewer pointers.  This expression usually overestimates, sometimes
	 * substantially.  However, it is still more conservative than the
	 * previous expression, which was to allocate exactly
	 * `JukeboxSongs.max_songs` pointers without regard to the file size
	 * or contents.
	 */
	const auto required_alloc_size = 1 + (length / 2);
	const auto max_songs = std::min(required_alloc_size, juke_max_songs);
	/* Use T=`char*[]` to ensure alignment.  Place pointers before file
	 * contents to keep the pointer array aligned.
	 */
	auto &&list_buf = std::make_unique<char*[]>(max_songs + 1 + (length / sizeof(char *)));
	const auto p = reinterpret_cast<char *>(list_buf.get() + max_songs);
	p[length] = '\0';	// make sure the last string is terminated
	return fread(p, length, 1, fp)
		? m3u_bytes(
			unchecked_partial_range(p, length),
			unchecked_partial_range(list_buf.get(), max_songs),
			std::move(list_buf)
		)
		: m3u_bytes();
}

static int read_m3u(void)
{
	auto &&m3u = read_m3u_bytes_from_disk(CGameCfg.CMLevelMusicPath.data());
	auto &list_buf = m3u.alloc;
	if (!list_buf)
		return 0;

	// The growing string list is allocated last, hopefully reducing memory fragmentation when it grows
	const auto eol = [](char c) {
		return c == '\n' || c == '\r' || !c;
	};
	JukeboxSongs.list.set_combined(std::move(list_buf));
	const auto &range = m3u.range;
	auto pp = m3u.ptr_range.begin();
	for (auto buf = range.begin(); buf != range.end(); ++buf)
	{
		for (; buf != range.end() && eol(*buf);)	// find new line - support DOS, Unix and Mac line endings
			buf++;
		if (buf == range.end())
			break;
		if (*buf != '#')	// ignore comments / extra info
		{
			*pp++ = buf;
			if (pp == m3u.ptr_range.end())
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
	JukeboxSongs.num_songs = std::distance(m3u.ptr_range.begin(), pp);
	return 1;
}

/* Loads music file names from a given directory or M3U playlist */
void jukebox_load()
{
	jukebox_unload();

	// Check if it's an M3U file
	auto &cfgpath = CGameCfg.CMLevelMusicPath;
	size_t musiclen = strlen(cfgpath.data());
	if (musiclen > 4 && !d_stricmp(&cfgpath[musiclen - 4], ".m3u"))
		read_m3u();
	else	// a directory
	{
		class PHYSFS_path_deleter
		{
		public:
			void operator()(const char *const p) const noexcept
			{
				PHYSFS_unmount(p);
			}
		};
		std::unique_ptr<const char, PHYSFS_path_deleter> new_path;
		const char *sep = PHYSFS_getDirSeparator();
		size_t seplen = strlen(sep);

		// stick a separator on the end if necessary.
		if (musiclen >= seplen)
		{
			auto p = &cfgpath[musiclen - seplen];
			if (strcmp(p, sep))
				cfgpath.copy_if(musiclen, sep, seplen);
		}

		const auto p = cfgpath.data();
		// Read directory using PhysicsFS
		if (PHYSFS_isDirectory(p))	// find files in relative directory
			JukeboxSongs.list.reset(PHYSFSX_findFiles(p, jukebox_exts));
		else
		{
			if (!PHYSFS_getMountPoint(p))
				new_path.reset(p);
			PHYSFS_mount(p, nullptr, 0);

			// as mountpoints are no option (yet), make sure only files originating from GameCfg.CMLevelMusicPath are aded to the list.
			JukeboxSongs.list.reset(PHYSFSX_findabsoluteFiles("", p, jukebox_exts));
		}

		if (!JukeboxSongs.list)
		{
			return;
		}
		JukeboxSongs.num_songs = std::distance(JukeboxSongs.list.begin(), JukeboxSongs.list.end());
	}

	if (JukeboxSongs.num_songs)
	{
		con_printf(CON_DEBUG,"Jukebox: %d music file(s) found in %s", JukeboxSongs.num_songs, cfgpath.data());
		if (CGameCfg.CMLevelMusicTrack[1] != JukeboxSongs.num_songs)
		{
			CGameCfg.CMLevelMusicTrack[1] = JukeboxSongs.num_songs;
			CGameCfg.CMLevelMusicTrack[0] = 0; // number of songs changed so start from beginning.
		}
	}
	else
	{
		CGameCfg.CMLevelMusicTrack[0] = -1;
		CGameCfg.CMLevelMusicTrack[1] = -1;
		con_puts(CON_DEBUG,"Jukebox music could not be found!");
	}
}

// To proceed tru our playlist. Usually used for continous play, but can loop as well.
static void jukebox_hook_next()
{
	if (!JukeboxSongs.list || CGameCfg.CMLevelMusicTrack[0] == -1)
		return;

	if (CGameCfg.CMLevelMusicPlayOrder == LevelMusicPlayOrder::Random)
		CGameCfg.CMLevelMusicTrack[0] = d_rand() % CGameCfg.CMLevelMusicTrack[1]; // simply a random selection - no check if this song has already been played. But that's how I roll!
	else
		CGameCfg.CMLevelMusicTrack[0]++;
	if (CGameCfg.CMLevelMusicTrack[0] + 1 > CGameCfg.CMLevelMusicTrack[1])
		CGameCfg.CMLevelMusicTrack[0] = 0;

	jukebox_play();
}

}

namespace dsx {

// Play tracks from Jukebox directory. Play track specified in GameCfg.CMLevelMusicTrack[0] and loop depending on CGameCfg.CMLevelMusicPlayOrder
int jukebox_play()
{
	const char *music_filename;
	uint_fast32_t size_full_filename = 0;

	if (!JukeboxSongs.list)
		return 0;

	if (CGameCfg.CMLevelMusicTrack[0] < 0 ||
		CGameCfg.CMLevelMusicTrack[0] + 1 > CGameCfg.CMLevelMusicTrack[1])
		return 0;

	music_filename = JukeboxSongs.list[CGameCfg.CMLevelMusicTrack[0]];
	if (!music_filename)
		return 0;

	size_t size_music_filename = strlen(music_filename);
	auto &cfgpath = CGameCfg.CMLevelMusicPath;
	size_t musiclen = strlen(cfgpath.data());
	size_full_filename = musiclen + size_music_filename + 1;
	RAIIdmem<char[]> full_filename;
	CALLOC(full_filename, char[], size_full_filename);
	const char *LevelMusicPath;
	if (musiclen > 4 && !d_stricmp(&cfgpath[musiclen - 4], ".m3u"))	// if it's from an M3U playlist
		LevelMusicPath = "";
	else											// if it's from a specified path
		LevelMusicPath = cfgpath.data();
	snprintf(full_filename.get(), size_full_filename, "%s%s", LevelMusicPath, music_filename);

	int played = songs_play_file(full_filename.get(), (CGameCfg.CMLevelMusicPlayOrder == LevelMusicPlayOrder::Level ? 1 : 0), (CGameCfg.CMLevelMusicPlayOrder == LevelMusicPlayOrder::Level ? nullptr : jukebox_hook_next));
	full_filename.reset();
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

}
