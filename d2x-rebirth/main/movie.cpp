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
 * Movie Playing Stuff
 *
 */

#include <string.h>
#ifndef macintosh
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
# ifndef _MSC_VER
#  include <unistd.h>
# endif
#endif // ! macintosh
#include <ctype.h>

#include "movie.h"
#include "window.h"
#include "console.h"
#include "config.h"
#include "physfsx.h"
#include "key.h"
#include "mouse.h"
#include "digi.h"
#include "songs.h"
#include "inferno.h"
#include "palette.h"
#include "strutil.h"
#include "dxxerror.h"
#include "u_mem.h"
#include "gr.h"
#include "gamefont.h"
#include "menu.h"
#include "libmve.h"
#include "text.h"
#include "screens.h"
#include "physfsrwops.h"
#if DXX_USE_OGL
#include "ogl_init.h"
#endif
#include "args.h"

#include "compiler-range_for.h"
#include "partial_range.h"

namespace {

// Subtitle data
struct subtitle {
	typename std::conditional<sizeof(char *) == sizeof(uint32_t), uint16_t, uint32_t>::type first_frame, last_frame;
	const char *msg;
};

#define MAX_ACTIVE_SUBTITLES 3

struct d_subtitle_state
{
	unsigned Num_subtitles = 0;
	std::unique_ptr<char[]> subtitle_raw_data;
	std::array<subtitle, 500> Subtitles;
};

static int init_subtitles(d_subtitle_state &SubtitleState, const char *filename);

// Movielib data

constexpr std::array<std::array<char, 8>, 3> movielib_files{{
	{"intro"}, {"other"}, {"robots"}
}};

struct loaded_movie_t
{
	std::array<char, FILENAME_LEN + 2> filename;
};

static loaded_movie_t extra_robot_movie_mission;

static RWops_ptr RoboFile;

// Function Prototypes
static movie_play_status RunMovie(const char *filename, const char *subtitles, int highres_flag, int allow_abort,int dx,int dy);

static void draw_subtitles(const d_subtitle_state &, int frame_num);

//-----------------------------------------------------------------------

struct movie_pause_window : window
{
	using window::window;
	virtual window_event_result event_handler(const d_event &) override;
};

}

void *MovieMemoryAllocate(std::size_t size)
{
	return d_malloc(size);
}

void MovieMemoryFree(void *p)
{
	d_free(p);
}

unsigned int MovieFileRead(void *handle, void *buf, unsigned int count)
{
	const unsigned numread = SDL_RWread(reinterpret_cast<SDL_RWops *>(handle), buf, 1, count);
	return (numread == count);
}

//-----------------------------------------------------------------------


//filename will actually get modified to be either low-res or high-res
//returns status.  see values in movie.h
movie_play_status PlayMovie(const char *subtitles, const char *filename, int must_have)
{
	char name[FILENAME_LEN],*p;

	if (GameArg.SysNoMovies)
		return movie_play_status::skipped;

	strcpy(name,filename);

	if ((p=strchr(name,'.')) == NULL)		//add extension, if missing
		strcat(name,".MVE");

	// Stop all digital sounds currently playing.
	digi_stop_digi_sounds();

	// Stop all songs
	songs_stop_all();

	// MD2211: if using SDL_Mixer, we never reinit the sound system
	if (CGameArg.SndDisableSdlMixer)
		digi_close();

	// Start sound
	MVE_sndInit(!CGameArg.SndNoSound ? 1 : -1);

	const auto ret = RunMovie(name, subtitles, !GameArg.GfxSkipHiresMovie, must_have, -1, -1);

	// MD2211: if using SDL_Mixer, we never reinit the sound system
	if (!CGameArg.SndNoSound
		&& CGameArg.SndDisableSdlMixer
	)
		digi_init();

	Screen_mode = -1;		//force screen reset
	return ret;
}

void MovieShowFrame(const uint8_t *buf, int dstx, int dsty, int bufw, int bufh, int sw, int sh)
{
	grs_bitmap source_bm;
	static palette_array_t old_pal;
	float scale = 1.0;

	if (old_pal != gr_palette)
	{
		old_pal = gr_palette;
		return;
	}
	old_pal = gr_palette;

	source_bm.bm_x = source_bm.bm_y = 0;
	source_bm.bm_w = source_bm.bm_rowsize = bufw;
	source_bm.bm_h = bufh;
	source_bm.set_type(bm_mode::linear);
	source_bm.clear_flags();
	source_bm.bm_data = buf;

	if (dstx == -1 && dsty == -1) // Fullscreen movie so set scale to fit the actual screen size
	{
		if ((static_cast<float>(SWIDTH)/SHEIGHT) < (static_cast<float>(sw)/bufh))
			scale = (static_cast<float>(SWIDTH)/sw);
		else
			scale = (static_cast<float>(SHEIGHT)/bufh);
	}
	else // Other (robot) movie so set scale to min. screen dimension
	{
		if ((static_cast<float>(SWIDTH)/bufw) < (static_cast<float>(SHEIGHT)/bufh))
			scale = (static_cast<float>(SWIDTH)/sw);
		else
			scale = (static_cast<float>(SHEIGHT)/sh);
	}

	if (dstx == -1) // center it
		dstx = (SWIDTH/2)-((bufw*scale)/2);
	if (dsty == -1) // center it
		dsty = (SHEIGHT/2)-((bufh*scale)/2);

#if DXX_USE_OGL
	glDisable (GL_BLEND);

	ogl_ubitblt_i(
		bufw*scale, bufh*scale,
		dstx, dsty,
		bufw, bufh, 0, 0, source_bm, grd_curcanv->cv_bitmap, (GameCfg.MovieTexFilt) ? opengl_texture_filter::trilinear : opengl_texture_filter::classic);

	glEnable (GL_BLEND);
#else
	gr_bm_ubitbltm(*grd_curcanv, bufw, bufh, dstx, dsty, 0, 0, source_bm);
#endif
}

//our routine to set the pallete, called from the movie code
void MovieSetPalette(const unsigned char *p, unsigned start, unsigned count)
{
	if (count == 0)
		return;

	//Color 0 should be black, and we get color 255
	Assert(start>=1 && start+count-1<=254);

	//Set color 0 to be black
	gr_palette[0].r = gr_palette[0].g = gr_palette[0].b = 0;

	//Set color 255 to be our subtitle color
	gr_palette[255].r = gr_palette[255].g = gr_palette[255].b = 50;

	//movie libs palette into our array
	memcpy(&gr_palette[start],p+start*3,count*3);
}

namespace {

struct movie : window
{
	MVE_StepStatus result = MVE_StepStatus::EndOfFile;
	int frame_num = 0;
	int paused = 0;
	const MVESTREAM_ptr_t pMovie;
	d_subtitle_state SubtitleState;
	movie(grs_canvas &src, int x, int y, int w, int h, MVESTREAM_ptr_t mvestream) :
		window(src, x, y, w, h),
		pMovie(std::move(mvestream))
	{
	}
	virtual window_event_result event_handler(const d_event &) override;
};

window_event_result movie_pause_window::event_handler(const d_event &event)
{
	switch (event.type)
	{
		case EVENT_MOUSE_BUTTON_DOWN:
			if (event_mouse_get_button(event) != 0)
				return window_event_result::ignored;
			DXX_BOOST_FALLTHROUGH;
		case EVENT_KEY_COMMAND:
			if (const auto result = call_default_handler(event); result == window_event_result::ignored)
				return window_event_result::close;
			else
				return result;

		case EVENT_WINDOW_DRAW:
		{
			const char *msg = TXT_PAUSE;
			int h;
			int y;

			gr_set_default_canvas();
			auto &canvas = *grd_curcanv;
			const auto &game_font = *GAME_FONT;
			gr_get_string_size(game_font, msg, nullptr, &h, nullptr);

			y = (grd_curscreen->get_screen_height() - h) / 2;

			gr_set_fontcolor(canvas, 255, -1);

			gr_ustring(canvas, game_font, 0x8000, y, msg);
			break;
		}

		default:
			break;
	}
	return window_event_result::ignored;
}

window_event_result movie::event_handler(const d_event &event)
{
	int key;
	switch (event.type)
	{
		case EVENT_WINDOW_ACTIVATED:
			paused = 0;
			break;

		case EVENT_WINDOW_DEACTIVATED:
			paused = 1;
			MVE_rmHoldMovie();
			break;

		case EVENT_KEY_COMMAND:
			key = event_key_get(event);

			// If ESCAPE pressed, then quit movie.
			if (key == KEY_ESC) {
				return window_event_result::close;
			}

			// If PAUSE pressed, then pause movie
			if ((key == KEY_PAUSE) || (key == KEY_COMMAND + KEY_P))
			{
				if (auto pause_window = window_create<movie_pause_window>(grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT))
				{
					(void)pause_window;
					MVE_rmHoldMovie();
				}
				return window_event_result::handled;
			}
			break;

		case EVENT_WINDOW_DRAW:
			if (!paused)
			{
				result = MVE_rmStepMovie(*pMovie.get());
				if (result == MVE_StepStatus::EndOfFile)
					return window_event_result::close;
				if (result != MVE_StepStatus::Continue)
				{
					return window_event_result::handled;
				}
			}

			draw_subtitles(SubtitleState, frame_num);

			gr_palette_load(gr_palette);

			if (!paused)
				frame_num++;
			break;

		case EVENT_WINDOW_CLOSE:
			break;
		default:
			break;
	}
	return window_event_result::ignored;
}

//returns status.  see movie.h
movie_play_status RunMovie(const char *const filename, const char *const subtitles, const int hires_flag, const int must_have, const int dx, const int dy)
{
	int track = 0;
#if DXX_USE_OGL
	palette_array_t pal_save;
#endif

	// Open Movie file.  If it doesn't exist, no movie, just return.

	auto filehndl = PHYSFSRWOPS_openRead(filename);
	if (!filehndl)
	{
		con_printf(must_have ? CON_URGENT : CON_VERBOSE, "Failed to open movie <%s>: %s", filename, PHYSFS_getLastError());
		return movie_play_status::skipped;
	}
	MVESTREAM_ptr_t mvestream;
	if (MVE_rmPrepMovie(mvestream, filehndl.get(), dx, dy, track))
	{
		return movie_play_status::skipped;
	}
	const auto reshow = hide_menus();
	auto wind = window_create<movie>(grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT, std::move(mvestream));
	bool exists = true;
	wind->track(&exists);
	init_subtitles(wind->SubtitleState, subtitles);

#if DXX_USE_OGL
	set_screen_mode(SCREEN_MOVIE);
	gr_copy_palette(pal_save, gr_palette);
	gr_palette_load(gr_palette);
	(void)hires_flag;
#else
	gr_set_mode(hires_flag ? screen_mode{640, 480} : screen_mode{320, 200});
#endif

	while (exists)
		event_process();
	wind = nullptr;

	filehndl.reset();                           // Close Movie File
	if (reshow)
		show_menus();

	// Restore old graphic state

	Screen_mode=-1;  //force reset of screen mode
#if DXX_USE_OGL
	gr_copy_palette(gr_palette, pal_save);
	gr_palette_load(pal_save);
#endif

	return movie_play_status::started;
}

}

//returns 1 if frame updated ok
int RotateRobot(MVESTREAM_ptr_t &pMovie)
{
	auto err = MVE_rmStepMovie(*pMovie.get());
	gr_palette_load(gr_palette);

	if (err == MVE_StepStatus::EndOfFile)     //end of movie, so reset
	{
		SDL_RWseek(RoboFile.get(), 0, SEEK_SET);
		if (MVE_rmPrepMovie(pMovie, RoboFile.get(), SWIDTH/2.3, SHEIGHT/2.3, 0))
		{
			Int3();
			return 0;
		}
		err = MVE_rmStepMovie(*pMovie.get());
	}
	if (err != MVE_StepStatus::Continue)
	{
		Int3();
		return 0;
	}

	return 1;
}


void DeInitRobotMovie(MVESTREAM_ptr_t &pMovie)
{
	pMovie.reset();
	RoboFile.reset();                           // Close Movie File
}


int InitRobotMovie(const char *filename, MVESTREAM_ptr_t &pMovie)
{
	if (GameArg.SysNoMovies)
		return 0;

	con_printf(CON_DEBUG, "RoboFile=%s", filename);

	MVE_sndInit(-1);        //tell movies to play no sound for robots

	RoboFile = PHYSFSRWOPS_openRead(filename);

	if (!RoboFile)
	{
		con_printf(CON_URGENT, "Can't open movie <%s>: %s", filename, PHYSFS_getLastError());
		return 0;
	}
	if (MVE_rmPrepMovie(pMovie, RoboFile.get(), SWIDTH/2.3, SHEIGHT/2.3, 0)) {
		Int3();
		return 0;
	}

	return 1;
}

namespace {

/*
 *		Subtitle system code
 */

//search for next field following whitespace
static char *next_field (char *p)
{
	while (*p && !isspace(*p))
		p++;

	if (!*p)
		return NULL;

	while (*p && isspace(*p))
		p++;

	if (!*p)
		return NULL;

	return p;
}

static int init_subtitles(d_subtitle_state &SubtitleState, const char *const filename)
{
	if (!filename)
		return 0;
	int size,read_count;
	char *p;
	int have_binary = 0;

	SubtitleState.Num_subtitles = 0;

	if (!GameCfg.MovieSubtitles)
	{
		con_puts(CON_VERBOSE, "Rebirth: movie subtitles are disabled");
		return 0;
	}

	auto ifile = PHYSFSX_openReadBuffered(filename);		//try text version

	if (!ifile) {								//no text version, try binary version
		char filename2[FILENAME_LEN];
		change_filename_extension(filename2, filename, ".txb");
		ifile = PHYSFSX_openReadBuffered(filename2);
		if (!ifile)
		{
			con_printf(CON_VERBOSE, "Rebirth: skipping subtitles because cannot open \"%s\" or \"%s\"", filename, filename2);
			return 0;
		}
		have_binary = 1;
		con_printf(CON_VERBOSE, "Rebirth: found encoded subtitles in \"%s\"", filename2);
	}
	else
		con_printf(CON_VERBOSE, "Rebirth: found text subtitles in \"%s\"", filename);

	size = PHYSFS_fileLength(ifile);

	const auto subtitle_raw_data = (SubtitleState.subtitle_raw_data = std::make_unique<char[]>(size + 1)).get();
	read_count = PHYSFS_read(ifile, subtitle_raw_data, 1, size);
	ifile.reset();

	if (read_count != size) {
		con_puts(CON_VERBOSE, "Rebirth: skipping subtitles because cannot read full subtitle file");
		return 0;
	}

	subtitle_raw_data[size] = 0;
	p = subtitle_raw_data;

	while (p && p < subtitle_raw_data+size) {
		char *endp;

		endp = strchr(p,'\n');
		if (endp) {
			if (endp[-1] == '\r')
				endp[-1] = 0;		//handle 0d0a pair
			*endp = 0;			//string termintor
		}

		if (have_binary)
			decode_text_line(p);

		if (*p != ';') {
			const auto Num_subtitles = SubtitleState.Num_subtitles;
			auto &Subtitles = SubtitleState.Subtitles;
			auto &s = Subtitles[SubtitleState.Num_subtitles++];
			s.first_frame = atoi(p);
			p = next_field(p); if (!p) continue;
			s.last_frame = atoi(p);
			p = next_field(p); if (!p) continue;
			s.msg = p;

			if (Num_subtitles)
			{
				assert(s.first_frame >= Subtitles[Num_subtitles - 1].first_frame);
			}
			assert(s.last_frame >= s.first_frame);
		}

		p = endp+1;
	}
	return 1;
}

//draw the subtitles for this frame
static void draw_subtitles(const d_subtitle_state &SubtitleState, const int frame_num)
{
	static int active_subtitles[MAX_ACTIVE_SUBTITLES];
	static int next_subtitle;
	static unsigned num_active_subtitles;
	int y;
	int must_erase=0;

	if (frame_num == 0) {
		num_active_subtitles = 0;
		next_subtitle = 0;
		gr_set_curfont(*grd_curcanv, *GAME_FONT);
		gr_set_fontcolor(*grd_curcanv, 255, -1);
	}

	//get rid of any subtitles that have expired
	auto &Subtitles = SubtitleState.Subtitles;
	for (int t=0;t<num_active_subtitles;)
		if (frame_num > Subtitles[active_subtitles[t]].last_frame) {
			int t2;
			for (t2=t;t2<num_active_subtitles-1;t2++)
				active_subtitles[t2] = active_subtitles[t2+1];
			num_active_subtitles--;
			must_erase = 1;
		}
		else
			t++;

	//get any subtitles new for this frame
	while (next_subtitle < SubtitleState.Num_subtitles && frame_num >= Subtitles[next_subtitle].first_frame) {
		if (num_active_subtitles >= MAX_ACTIVE_SUBTITLES)
			Error("Too many active subtitles!");
		active_subtitles[num_active_subtitles++] = next_subtitle;
		next_subtitle++;
	}

	//find y coordinate for first line of subtitles
	const auto &&line_spacing = LINE_SPACING(*grd_curcanv->cv_font, *GAME_FONT);
	y = grd_curcanv->cv_bitmap.bm_h - (line_spacing * (MAX_ACTIVE_SUBTITLES + 2));

	//erase old subtitles if necessary
	if (must_erase) {
		gr_rect(*grd_curcanv, 0,y,grd_curcanv->cv_bitmap.bm_w-1,grd_curcanv->cv_bitmap.bm_h-1, color_palette_index{0});
	}

	//now draw the current subtitles
	range_for (const auto &t, partial_range(active_subtitles, num_active_subtitles))
		if (t != -1)
		{
			gr_string(*grd_curcanv, *grd_curcanv->cv_font, 0x8000, y, Subtitles[t].msg);
			y += line_spacing;
		}
}

static int init_movie(const char *movielib, char resolution, int required, loaded_movie_t &movie)
{
	snprintf(&movie.filename[0], movie.filename.size(), "%s-%c.mvl", movielib, resolution);
	auto r = PHYSFSX_contfile_init(&movie.filename[0], 0);
	if (!r)
	{
		if (required || CGameArg.DbgVerbose)
			con_printf(CON_URGENT, "Can't open movielib <%s>: %s", &movie.filename[0], PHYSFS_getLastError());
		movie.filename[0] = 0;
	}
	return r;
}

static void init_movie(const char *movielib, int required, loaded_movie_t &movie)
{
	if (!GameArg.GfxSkipHiresMovie)
	{
		if (init_movie(movielib, 'h', required, movie))
			return;
	}
	init_movie(movielib, 'l', required, movie);
}

}

//find and initialize the movie libraries
void init_movies()
{
	if (GameArg.SysNoMovies)
		return;

	range_for (auto &i, movielib_files)
	{
		loaded_movie_t m;
		init_movie(&i[0], 1, m);
	}
}

void close_extra_robot_movie()
{
	const auto movielib = &extra_robot_movie_mission.filename[0];
	if (!*movielib)
		return;
	if (!PHYSFSX_contfile_close(movielib))
	{
		con_printf(CON_URGENT, "Can't close movielib <%s>: %s", movielib, PHYSFS_getLastError());
	}
	*movielib = 0;
}

void init_extra_robot_movie(const char *movielib)
{
	if (GameArg.SysNoMovies)
		return;
	init_movie(movielib, 0, extra_robot_movie_mission);
}
