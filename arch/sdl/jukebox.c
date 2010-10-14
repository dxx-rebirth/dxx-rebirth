/*
 * DXX Rebirth "jukebox" code
 * MD 2211 <md2211@users.sourceforge.net>, 2007
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if !(defined(__APPLE__) && defined(__MACH__))
#include <SDL/SDL_mixer.h>
#else
#include <SDL_mixer/SDL_mixer.h>
#endif

#include "physfsx.h"
#include "args.h"
#include "dl_list.h"
#include "hudmsg.h"
#include "digi_mixer_music.h"
#include "jukebox.h"
#include "error.h"
#include "console.h"
#include "config.h"

#define MUSIC_HUDMSG_MAXLEN 40
#define JUKEBOX_HUDMSG_PLAYING "Now playing:"
#define JUKEBOX_HUDMSG_STOPPED "Jukebox stopped"

typedef struct jukebox_songs
{
	char **list;	// the actual list
	char *list_buf; // buffer containing song file path text
	int num_songs;	// number of jukebox songs
	int max_songs;	// maximum number of pointers that 'list' can hold, i.e. size of list / size of one pointer
	int max_buf;	// size of list_buf
} jukebox_songs;

static jukebox_songs JukeboxSongs = { NULL, NULL, 0, 0, 0 };
char hud_msg_buf[MUSIC_HUDMSG_MAXLEN+4];


void jukebox_unload()
{
	if (JukeboxSongs.list)
		d_free(JukeboxSongs.list);

	if (JukeboxSongs.list_buf)
		d_free(JukeboxSongs.list_buf);
	
	JukeboxSongs.num_songs = JukeboxSongs.max_songs = JukeboxSongs.max_buf = 0;
}

char *jukebox_exts[] = { ".mp3", ".ogg", ".wav", ".aif", ".mid", NULL };

void jukebox_add_song(void *data, const char *origdir, const char *fname)
{
	char *ext;
	char **i = NULL;
	
	ext = strrchr(fname, '.');
	if (ext)
		for (i = jukebox_exts; *i != NULL && stricmp(ext, *i); i++) {}	// see if the file is of a type we want

	if ((!strcmp((PHYSFS_getRealDir(fname)==NULL?"":PHYSFS_getRealDir(fname)), GameCfg.CMLevelMusicPath)) && (ext && *i))
		string_array_add(&JukeboxSongs.list, &JukeboxSongs.list_buf, &JukeboxSongs.num_songs, &JukeboxSongs.max_songs, &JukeboxSongs.max_buf, fname);
}

void read_m3u(void)
{
	FILE *fp;
	int length;
	char *buf;
	
	fp = fopen(GameCfg.CMLevelMusicPath, "rb");
	if (!fp)
		return;
	
	fseek( fp, -1, SEEK_END );
	length = ftell(fp) + 1;
	MALLOC(JukeboxSongs.list_buf, char, length + 1);
	if (!JukeboxSongs.list_buf)
	{
		fclose(fp);
		return;
	}

	fseek(fp, 0, SEEK_SET);
	if (!fread(JukeboxSongs.list_buf, length, 1, fp))
	{
		d_free(JukeboxSongs.list_buf);
		fclose(fp);
		return;
	}

	fclose(fp);		// Finished with it

	// The growing string list is allocated last, hopefully reducing memory fragmentation when it grows
	MALLOC(JukeboxSongs.list, char *, 1024);
	if (!JukeboxSongs.list)
	{
		d_free(JukeboxSongs.list_buf);
		return;
	}
	JukeboxSongs.max_songs = 1024;

	JukeboxSongs.list_buf[length] = '\0';	// make sure the last string is terminated
	JukeboxSongs.max_buf = length + 1;
	buf = JukeboxSongs.list_buf;
	
	while (buf < JukeboxSongs.list_buf + length)
	{
		while (*buf == 0 || *buf == 10 || *buf == 13)	// find new line - support DOS, Unix and Mac line endings
			buf++;
		
		if (*buf != '#')	// ignore comments / extra info
		{
			if (JukeboxSongs.num_songs >= JukeboxSongs.max_songs)
			{
				char **new_list = d_realloc(JukeboxSongs.list, JukeboxSongs.max_buf*sizeof(char *)*MEM_K);
				if (new_list == NULL)
					break;
				JukeboxSongs.max_buf *= MEM_K;
				JukeboxSongs.list = new_list;
			}
			
			JukeboxSongs.list[JukeboxSongs.num_songs++] = buf;
		}
		
		while (*buf != 0 && *buf != 10 && *buf != 13)	// find end of line
			buf++;
		
		*buf = 0;
	}
}

/* Loads music file names from a given directory or M3U playlist */
void jukebox_load()
{
	static char curpath[PATH_MAX+1];

	if (memcmp(curpath,GameCfg.CMLevelMusicPath,PATH_MAX) || (GameCfg.MusicType != MUSIC_TYPE_CUSTOM))
	{
		PHYSFS_removeFromSearchPath(curpath);
		jukebox_unload();
	}

	if (JukeboxSongs.list)
		return;

	if (GameCfg.MusicType == MUSIC_TYPE_CUSTOM)
	{
		char *p;
		const char *sep = PHYSFS_getDirSeparator();

		// build path properly.
		if (strlen(GameCfg.CMLevelMusicPath) >= strlen(sep))
		{
			char abspath[PATH_MAX+1];

			p = GameCfg.CMLevelMusicPath + strlen(GameCfg.CMLevelMusicPath) - strlen(sep);
			if (strcmp(p, sep) && stricmp(&GameCfg.CMLevelMusicPath[strlen(GameCfg.CMLevelMusicPath) - 4], ".m3u"))
				strncat(GameCfg.CMLevelMusicPath, sep, PATH_MAX - 1 - strlen(GameCfg.CMLevelMusicPath));

			if (PHYSFS_isDirectory(GameCfg.CMLevelMusicPath)) // it's a child of Sharepath, build full path
			{
				PHYSFSX_getRealPath(GameCfg.CMLevelMusicPath,abspath);
				snprintf(GameCfg.CMLevelMusicPath,sizeof(char)*PATH_MAX,"%s",abspath);
			}
		}

		// Check if it's an M3U file
		if (!PHYSFS_addToSearchPath(GameCfg.CMLevelMusicPath, 0))
		{
			if (!stricmp(&GameCfg.CMLevelMusicPath[strlen(GameCfg.CMLevelMusicPath) - 4], ".m3u"))
				read_m3u();
		}
		else
		{
			// Read directory using PhysicsFS

			if (!string_array_new(&JukeboxSongs.list, &JukeboxSongs.list_buf, &JukeboxSongs.num_songs, &JukeboxSongs.max_songs, &JukeboxSongs.max_buf))
				return;
			
			// as mountpoints are no option (yet), make sure only files originating from GameCfg.CMLevelMusicPath are aded to the list.
			PHYSFS_enumerateFilesCallback("", jukebox_add_song, NULL);
			string_array_tidy(&JukeboxSongs.list, &JukeboxSongs.list_buf, &JukeboxSongs.num_songs, &JukeboxSongs.max_songs, &JukeboxSongs.max_buf, 0,
#ifdef __LINUX__
							  strcmp
#elif defined(_WIN32) || defined(macintosh)
							  stricmp
#else
							  strcasecmp
#endif
							  );
		}
	
		if (JukeboxSongs.num_songs)
		{
			con_printf(CON_DEBUG,"Jukebox: %d music file(s) found in %s\n", JukeboxSongs.num_songs, GameCfg.CMLevelMusicPath);
			memcpy(curpath,GameCfg.CMLevelMusicPath,sizeof(char)*PATH_MAX);
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
			PHYSFS_removeFromSearchPath(GameCfg.CMLevelMusicPath);
			con_printf(CON_DEBUG,"Jukebox music could not be found!\n");
		}
	}
}

// To proceed tru our playlist. Usually used for continous play, but can loop as well.
void jukebox_hook_next()
{
	if (!JukeboxSongs.list || GameCfg.CMLevelMusicTrack[0] == -1) return;

	GameCfg.CMLevelMusicTrack[0]++;
	if (GameCfg.CMLevelMusicTrack[0] + 1 > GameCfg.CMLevelMusicTrack[1])
		GameCfg.CMLevelMusicTrack[0] = 0;

	jukebox_play();
}

// Play tracks from Jukebox directory. Play track specified in GameCfg.CMLevelMusicTrack[0] and loop depending on GameCfg.CMLevelMusicPlayOrder
int jukebox_play()
{
	char *music_filename;

	if (!JukeboxSongs.list)
		return 0;

	if (GameCfg.CMLevelMusicTrack[0] < 0 || GameCfg.CMLevelMusicTrack[0] + 1 > GameCfg.CMLevelMusicTrack[1])
		return 0;

	music_filename = JukeboxSongs.list[GameCfg.CMLevelMusicTrack[0]];
	if (!music_filename)
		return 0;

	if (!mix_play_file(music_filename, ((GameCfg.CMLevelMusicPlayOrder == MUSIC_CM_PLAYORDER_CONT)?0:1), ((GameCfg.CMLevelMusicPlayOrder == MUSIC_CM_PLAYORDER_CONT)?jukebox_hook_next:NULL)))
		return 0;	// whoops, got an error

	// Formatting a pretty message
	if (strlen(music_filename) >= MUSIC_HUDMSG_MAXLEN) {
		strcpy(hud_msg_buf, "...");
		strncat(hud_msg_buf, &music_filename[strlen(music_filename) - MUSIC_HUDMSG_MAXLEN], MUSIC_HUDMSG_MAXLEN);
		hud_msg_buf[MUSIC_HUDMSG_MAXLEN+3] = '\0';
	} else {
		strcpy(hud_msg_buf, music_filename);
	}

	HUD_init_message(HM_DEFAULT, "%s %s", JUKEBOX_HUDMSG_PLAYING, hud_msg_buf);

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
	if (!(*JukeboxSongs.list)) {
		con_printf(CON_DEBUG,"* No songs have been found\n");
	}
	else {
		for (i = 0; i < GameCfg.CMLevelMusicTrack[1]; i++)
			con_printf(CON_DEBUG,"* %s\n", JukeboxSongs.list[i]);
	}
}
