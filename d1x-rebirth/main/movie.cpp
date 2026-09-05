/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * See COPYING.txt for license details.
 */

/*
 *
 * D1X Movie Playing Stuff (PSX extra movies)
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

#include "movie.h"
#include "window.h"
#include "console.h"
#include "config.h"
#include "physfsx.h"
#include "joy.h"
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
#include "game.h"
#endif
#include "args.h"

#include "compiler-range_for.h"
#include "d_zip.h"

namespace d1x {

namespace {

// Function Prototypes
static movie_play_status RunMovie(const char *filename, int highres_flag, play_movie_warn_missing, MVE_play_sounds audio_enabled, int dx, int dy);

//-----------------------------------------------------------------------

struct movie_pause_window : window
{
	movie_pause_window(grs_canvas &src) :
		window(src, 0, 0, src.cv_bitmap.bm_w, src.cv_bitmap.bm_h)
	{
	}
	virtual window_event_result event_handler(const d_event &) override;
};

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
	(void)subtitles;

	if (GameArg.SysNoMovies)
		return movie_play_status::skipped;

	// Stop all digital sounds currently playing.
	digi_stop_digi_sounds();

	// Stop all songs
	songs_stop_all();

	// MD2211: if using SDL_Mixer, we never reinit the sound system
	if (CGameArg.SndDisableSdlMixer)
		digi_close();

	const auto ret{RunMovie(name, 1, must_have, !CGameArg.SndNoSound ? MVE_play_sounds::enabled : MVE_play_sounds::silent, -1, -1)};

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

	(void)sw;
	(void)sh;
	if ((static_cast<float>(SWIDTH) / bufw) < (static_cast<float>(SHEIGHT) / bufh))
		scale = static_cast<float>(SWIDTH) / bufw;
	else
		scale = static_cast<float>(SHEIGHT) / bufh;

	if (dstx == -1) // center it
		dstx = (SWIDTH/2)-((bufw*scale)/2);
	if (dsty == -1) // center it
		dsty = (SHEIGHT/2)-((bufh*scale)/2);

#if DXX_USE_OGL
	glDisable (GL_BLEND);

#if DXX_USE_STEREOSCOPIC_RENDER
	if (VR_stereo != StereoFormat::None)
		ogl_ubitmapm_cs(*grd_curcanv, dstx, dsty, bufw, bufh, source_bm, ogl_colors::white, true);
	else
#endif
	ogl_ubitblt_i(
		bufw*scale, bufh*scale,
		dstx, dsty,
		bufw, bufh, 0, 0, source_bm, grd_curcanv->cv_bitmap, opengl_texture_filter::classic);

	glEnable (GL_BLEND);
#else
	{
		const int dstw = static_cast<int>(bufw * scale);
		const int dsth = static_cast<int>(bufh * scale);
		const std::array<grs_point, 3> vertbuf{{
			{i2f(dstx), i2f(dsty)},
			{0, 0},
			{i2f(dstx + dstw), i2f(dsty + dsth)}
		}};
		scale_bitmap(source_bm, vertbuf, 0, grd_curcanv->cv_bitmap);
	}
#endif
}

//our routine to set the pallete, called from the movie code
void MovieSetPalette(const unsigned char *p, unsigned start, unsigned count)
{
	if (count == 0)
		return;

	//Set color 0 to be black
	gr_palette[0] = {};

	//Set color 255 to be white (for any text overlay)
	gr_palette[255] = rgb_t{
		.r = 50,
		.g = 50,
		.b = 50,
	};

	// Clamp range to protect reserved colors 0 and 255
	if (start == 0)
	{
		++start;
		if (count > 0)
			--count;
	}
	if (start + count > 255)
		count = 255 - start;
	if (count == 0)
		return;

	//movie libs palette into our array
	memcpy(&gr_palette[start],p+start*3,count*3);

	gr_palette_load(gr_palette);
}

namespace {

struct movie : window, mixin_trackable_window
{
	MVE_StepStatus result{MVE_StepStatus::EndOfFile};
	uint8_t paused{};
	int frame_num{};
	const MVESTREAM_ptr_t pMovie;
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

#if DXX_MAX_BUTTONS_PER_JOYSTICK
		case event_type::joystick_button_down:
		{
			const auto btn = event_joystick_get_button(event);
#if SDL_MAJOR_VERSION == 2
			if (btn == SDL_CONTROLLER_BUTTON_B || btn == SDL_CONTROLLER_BUTTON_BACK)
#else
			if (btn == 1)
#endif
				return window_event_result::close;
			break;
		}
#endif

		case event_type::window_draw:
			{
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
movie_play_status RunMovie(const char *const filename, const int hires_flag, const play_movie_warn_missing warn_missing, const MVE_play_sounds audio_enabled, const int dx, const int dy)
{

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

#if DXX_USE_OGL
	set_screen_mode(SCREEN_MOVIE);
	const palette_array_t pal_save{gr_palette};
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
	gr_palette = pal_save;
	reset_computed_colors();
	gr_palette_load(pal_save);
#endif

	return movie_play_status::started;
}

}

}
