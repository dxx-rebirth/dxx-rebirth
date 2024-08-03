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
#include "timer.h"
#include "physfsrwops.h"
#if DXX_USE_OGL
#include "ogl_init.h"
#endif
#include "args.h"

#include "compiler-range_for.h"
#include "partial_range.h"
#include "d_zip.h"

namespace d2x {

namespace {

// Subtitle data
struct subtitle {
	typename std::conditional<sizeof(char *) == sizeof(uint32_t), uint16_t, uint32_t>::type first_frame, last_frame;
	const char *msg;
};

#define MAX_ACTIVE_SUBTITLES 3

struct d_loaded_subtitle_state
{
	unsigned Num_subtitles{};
	std::unique_ptr<char[]> subtitle_raw_data;
	std::array<subtitle, 500> Subtitles
#ifndef NDEBUG
		= {}
#endif
		;
};

static int init_subtitles(d_loaded_subtitle_state &SubtitleState, std::span<const char> filename);

// Movielib data

constexpr std::array<std::array<char, 8>, 3> movielib_files{{
	{"intro"}, {"other"}, {"robots"}
}};

// Function Prototypes
static movie_play_status RunMovie(const char *filename, std::span<const char> subtitles, int highres_flag, play_movie_warn_missing, MVE_play_sounds audio_enabled, int dx, int dy);

static void draw_subtitles(const d_loaded_subtitle_state &, int frame_num);

//-----------------------------------------------------------------------

struct movie_pause_window : window
{
	movie_pause_window(grs_canvas &src) :
		window(src, 0, 0, src.cv_bitmap.bm_w, src.cv_bitmap.bm_h)
	{
	}
	virtual window_event_result event_handler(const d_event &) override;
};

static constexpr bool valid_palette_color_range(const unsigned start, const unsigned count)
{
	if (start == 0)
		return false;
	if (start > 254)
		return false;
	if (count > 254)
		return false;
	if (start + count > 255)
		return false;
	return true;
}

}

unsigned MovieFileRead(SDL_RWops *const handle, const std::span<uint8_t> buf)
{
	const auto count{buf.size()};
	const auto numread{SDL_RWread(handle, buf.data(), 1, count)};
	return (numread == count);
}

//-----------------------------------------------------------------------

//filename will actually get modified to be either low-res or high-res
//returns status.  see values in movie.h
movie_play_status PlayMovie(const std::span<const char> subtitles, const char *const name, const play_movie_warn_missing must_have)
{
	if (GameArg.SysNoMovies)
		return movie_play_status::skipped;

	// Stop all digital sounds currently playing.
	digi_stop_digi_sounds();

	// Stop all songs
	songs_stop_all();

	// MD2211: if using SDL_Mixer, we never reinit the sound system
	if (CGameArg.SndDisableSdlMixer)
		digi_close();

	const auto ret{RunMovie(name, subtitles, !GameArg.GfxSkipHiresMovie, must_have, !CGameArg.SndNoSound ? MVE_play_sounds::enabled : MVE_play_sounds::silent, -1, -1)};

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
	float scale{1.0f};

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
	static_assert(valid_palette_color_range(1, 254));
	static_assert(!valid_palette_color_range(1, 255));
	static_assert(!valid_palette_color_range(1, 256));
	static_assert(!valid_palette_color_range(0, 254));
	static_assert(!valid_palette_color_range(2, 254));
	if (!valid_palette_color_range(start, count))
	{
		/* Color 0 is reserved by the movie subsystem to be black.
		 * Color 255 is reserved to be the subtitle color.
		 * Colors above 255 are not supported.
		 */
		con_printf(CON_URGENT, "Rebirth: error: movie palette tried to write color range [%u, %u]", start, count);
		return;
	}

	//Set color 0 to be black
	gr_palette[0] = {};

	//Set color 255 to be our subtitle color
	gr_palette[255] = rgb_t{
		.r = 50,
		.g = 50,
		.b = 50,
	};

	//movie libs palette into our array
	memcpy(&gr_palette[start],p+start*3,count*3);
}

namespace {

struct movie : window, mixin_trackable_window
{
	MVE_StepStatus result{MVE_StepStatus::EndOfFile};
	uint8_t paused{};
	int frame_num{};
	const MVESTREAM_ptr_t pMovie;
	d_loaded_subtitle_state SubtitleState;
	movie(grs_canvas &src, MVESTREAM_ptr_t mvestream) :
		window(src, 0, 0, src.cv_bitmap.bm_w, src.cv_bitmap.bm_h),
		pMovie(std::move(mvestream))
	{
	}
	virtual window_event_result event_handler(const d_event &) override;
};

window_event_result movie_pause_window::event_handler(const d_event &event)
{
	switch (event.type)
	{
		case event_type::mouse_button_down:
			if (event_mouse_get_button(event) != mbtn::left)
				return window_event_result::ignored;
			[[fallthrough]];
		case event_type::key_command:
			if (const auto result{call_default_handler(event)}; result == window_event_result::ignored)
				return window_event_result::close;
			else
				return result;
		case event_type::idle:
			timer_delay(F1_0 / 4);
			break;

		case event_type::window_draw:
		{
			const char *msg{TXT_PAUSE};

			gr_set_default_canvas();
			auto &canvas{*grd_curcanv};
			const auto &game_font{*GAME_FONT};
			const auto h{gr_get_string_size(game_font, msg).height};

			const auto y{(grd_curscreen->get_screen_height() - h) / 2};

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
	switch (event.type)
	{
		case event_type::window_activated:
			paused = 0;
			break;

		case event_type::window_deactivated:
			paused = 1;
			MVE_rmHoldMovie();
			break;

		case event_type::key_command:
			{
				const auto key{event_key_get(event)};

			// If ESCAPE pressed, then quit movie.
			if (key == KEY_ESC) {
				return window_event_result::close;
			}

			// If PAUSE pressed, then pause movie
			if ((key == KEY_PAUSE) || (key == KEY_COMMAND + KEY_P))
			{
				if (auto pause_window{window_create<movie_pause_window>(w_canv)})
				{
					(void)pause_window;
					MVE_rmHoldMovie();
				}
				return window_event_result::handled;
			}
			break;
			}

		case event_type::window_draw:
			{
				const auto f{frame_num};
			if (!paused)
			{
				result = MVE_rmStepMovie(*pMovie);
				if (result == MVE_StepStatus::EndOfFile)
					return window_event_result::close;
				if (result != MVE_StepStatus::Continue)
				{
					return window_event_result::handled;
				}
				++frame_num;
			}

			draw_subtitles(SubtitleState, f);
			}

			gr_palette_load(gr_palette);

			break;

		case event_type::window_close:
			break;
		default:
			break;
	}
	return window_event_result::ignored;
}

//returns status.  see movie.h
movie_play_status RunMovie(const char *const filename, const std::span<const char> subtitles, const int hires_flag, const play_movie_warn_missing warn_missing, const MVE_play_sounds audio_enabled, const int dx, const int dy)
{
#if DXX_USE_OGL
	palette_array_t pal_save;
#endif

	// Open Movie file.  If it doesn't exist, no movie, just return.

	auto &&[filehndl, physfserr]{PHYSFSRWOPS_openRead(filename)};
	if (!filehndl)
	{
		con_printf(warn_missing == play_movie_warn_missing::verbose ? CON_VERBOSE : CON_URGENT, "Failed to open movie <%s>: %s", filename, PHYSFS_getErrorByCode(physfserr));
		return movie_play_status::skipped;
	}
	MVESTREAM_ptr_t mvestream;
	if (!(mvestream = MVE_rmPrepMovie(audio_enabled, dx, dy, std::move(filehndl))))
	{
		return movie_play_status::skipped;
	}
	const auto reshow{hide_menus()};
	auto wind{window_create<movie>(grd_curscreen->sc_canvas, std::move(mvestream))};
	init_subtitles(wind->SubtitleState, subtitles);

#if DXX_USE_OGL
	set_screen_mode(SCREEN_MOVIE);
	gr_copy_palette(pal_save, gr_palette);
	gr_palette_load(gr_palette);
	(void)hires_flag;
#else
	gr_set_mode(hires_flag ? screen_mode{640, 480} : screen_mode{320, 200});
#endif

	for (const auto exists{wind->track()}; *exists;)
		event_process();
	wind = nullptr;

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
int RotateRobot(MVESTREAM *const pMovie)
{
	auto err{MVE_rmStepMovie(*pMovie)};
	gr_palette_load(gr_palette);

	if (err == MVE_StepStatus::EndOfFile)     //end of movie, so reset
	{
		SDL_RWseek(pMovie->movie->stream.get(), 0, SEEK_SET);
		mve_reset(pMovie);
		err = MVE_rmStepMovie(*pMovie);
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
}

const MVESTREAM *InitRobotMovie(const char *filename, MVESTREAM_ptr_t &pMovie)
{
	if (GameArg.SysNoMovies)
		return nullptr;

	con_printf(CON_DEBUG, "RoboFile=%s", filename);

	auto &&[RoboFile, physfserr]{PHYSFSRWOPS_openRead(filename)};
	if (!RoboFile)
	{
		con_printf(CON_URGENT, "Failed to open movie <%s>: %s", filename, PHYSFS_getErrorByCode(physfserr));
		return nullptr;
	}
	//tell movies to play no sound for robots
	if (!(pMovie = MVE_rmPrepMovie(MVE_play_sounds::silent, SWIDTH / 2.3, SHEIGHT / 2.3, std::move(RoboFile))))
	{
		Int3();
		return nullptr;
	}
	return pMovie.get();
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

static int init_subtitles(d_loaded_subtitle_state &SubtitleState, const std::span<const char> filename)
{
	if (filename.empty())
		return 0;
	char *p;
	int have_binary{};

	SubtitleState.Num_subtitles = 0;

	if (!GameCfg.MovieSubtitles)
	{
		con_puts(CON_VERBOSE, "Rebirth: movie subtitles are disabled");
		return 0;
	}

	auto &&[ifile, physfserr]{PHYSFSX_openReadBuffered(filename.data())};		//try text version

	if (!ifile) {								//no text version, try binary version
		std::array<char, FILENAME_LEN> filename2;
		if (!change_filename_extension(filename2, filename.data(), "txb"))
		{
			con_printf(CON_NORMAL, "Rebirth: skipping subtitles because cannot open \"%s\" (\"%s\")", filename.data(), PHYSFS_getErrorByCode(physfserr));
			return 0;
		}
		auto &&[ifile2, physfserr2]{PHYSFSX_openReadBuffered_updateCase(filename2.data())};
		if (!ifile2)
		{
			con_printf(CON_VERBOSE, "Rebirth: skipping subtitles because cannot open \"%s\" or \"%s\" (\"%s\", \"%s\")", filename.data(), filename2.data(), PHYSFS_getErrorByCode(physfserr), PHYSFS_getErrorByCode(physfserr2));
			return 0;
		}
		ifile = std::move(ifile2);
		have_binary = 1;
		con_printf(CON_VERBOSE, "Rebirth: found encoded subtitles in \"%s\"", filename2.data());
	}
	else
		con_printf(CON_VERBOSE, "Rebirth: found text subtitles in \"%s\"", filename.data());

	const auto size{PHYSFS_fileLength(ifile)};
	const auto subtitle_raw_data{(SubtitleState.subtitle_raw_data = std::make_unique<char[]>(size + 1)).get()};
	const auto read_count{PHYSFSX_readBytes(ifile, subtitle_raw_data, size)};
	ifile.reset();

	if (read_count != size) {
		con_puts(CON_VERBOSE, "Rebirth: skipping subtitles because cannot read full subtitle file");
		return 0;
	}

	subtitle_raw_data[size] = 0;
	p = subtitle_raw_data;

	while (p && p < subtitle_raw_data+size) {
		char *const endp{strchr(p,'\n')};
		if (endp) {
			if (endp[-1] == '\r')
				endp[-1] = 0;		//handle 0d0a pair
			*endp = 0;			//string termintor
		}

		if (have_binary)
			decode_text_line(p);

		if (*p != ';') {
			const auto Num_subtitles{SubtitleState.Num_subtitles};
			auto &Subtitles{SubtitleState.Subtitles};
			auto &s{Subtitles[SubtitleState.Num_subtitles++]};
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
static void draw_subtitles(const d_loaded_subtitle_state &SubtitleState, const int frame_num)
{
	static std::array<uint16_t, MAX_ACTIVE_SUBTITLES> active_subtitles;
	static int next_subtitle;
	static unsigned num_active_subtitles;
	int y;
	uint8_t must_erase{};

	if (frame_num == 0) {
		num_active_subtitles = 0;
		next_subtitle = 0;
		gr_set_curfont(*grd_curcanv, *GAME_FONT);
		gr_set_fontcolor(*grd_curcanv, 255, -1);
	}

	//get rid of any subtitles that have expired
	auto &Subtitles{SubtitleState.Subtitles};
	for (int t{}; t < num_active_subtitles;)
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
	const auto &&line_spacing{LINE_SPACING(*grd_curcanv->cv_font, *GAME_FONT)};
	y = grd_curcanv->cv_bitmap.bm_h - (line_spacing * (MAX_ACTIVE_SUBTITLES + 2));

	//erase old subtitles if necessary
	if (must_erase) {
		gr_rect(*grd_curcanv, 0,y,grd_curcanv->cv_bitmap.bm_w-1,grd_curcanv->cv_bitmap.bm_h-1, color_palette_index{0});
	}

	//now draw the current subtitles
	range_for (const auto &t, partial_range(active_subtitles, num_active_subtitles))
		{
			gr_string(*grd_curcanv, *grd_curcanv->cv_font, 0x8000, y, Subtitles[t].msg);
			y += line_spacing;
		}
}

[[nodiscard]]
static PHYSFS_ErrorCode init_movie(const char *movielib, char resolution, int required, LoadedMovie &movie)
{
	std::array<char, FILENAME_LEN + 2> filename;
	if (static_cast<std::size_t>(snprintf(filename.data(), filename.size(), "%.8s-%c.mvl", movielib, resolution)) > filename.size())
		return PHYSFS_ERR_BAD_FILENAME;
	const auto r{PHYSFSX_addRelToSearchPath(filename.data(), movie.pathname, physfs_search_path::prepend)};
	if (r != PHYSFS_ERR_OK)
	{
		movie.pathname[0] = 0;
		con_printf(required ? CON_URGENT : CON_VERBOSE, "Failed to open movielib <%s>: %s", filename.data(), PHYSFS_getErrorByCode(r));
	}
	return r;
}

static std::pair<PHYSFS_ErrorCode, movie_resolution> init_movie(const char *movielib, int required, LoadedMovie &movie)
{
	if (!GameArg.GfxSkipHiresMovie)
	{
		if (const auto r{init_movie(movielib, 'h', required, movie)}; r == PHYSFS_ERR_OK)
			return {r, movie_resolution::high};
	}
	return {init_movie(movielib, 'l', required, movie), movie_resolution::low};
}

}

//find and initialize the movie libraries
std::unique_ptr<BuiltinMovies> init_movies()
{
	if (GameArg.SysNoMovies)
		return nullptr;
	return std::make_unique<BuiltinMovies>();
}

BuiltinMovies::BuiltinMovies()
{
	for (auto &&[f, m] : zip(movielib_files, movies))
		init_movie(f.data(), 1, m);
}

LoadedMovie::~LoadedMovie()
{
	const auto movielib{pathname.data()};
	if (!*movielib)
		return;
	if (!PHYSFS_unmount(movielib))
		con_printf(CON_URGENT, "Cannot close movielib <%s>: %s", movielib, PHYSFS_getLastError());
	else
		con_printf(CON_VERBOSE, "Unloaded movielib <%s>", movielib);
}

std::unique_ptr<LoadedMovieWithResolution> init_extra_robot_movie(const char *movielib)
{
	if (GameArg.SysNoMovies)
		return nullptr;
	auto r{std::make_unique<LoadedMovieWithResolution>()};
	if (const auto &&[code, resolution]{init_movie(movielib, 0, *r)}; code != PHYSFS_ERR_OK)
		return nullptr;
	else
	{
		r->resolution = resolution;
		return r;
	}
}

}
