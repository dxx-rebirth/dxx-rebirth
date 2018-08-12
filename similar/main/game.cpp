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
 * Game loop for Inferno
 *
 */

#include "dxxsconf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <SDL.h>
#include <ctime>
#if DXX_USE_SCREENSHOT_FORMAT_PNG
#include <png.h>
#include "vers_id.h"
#endif

#if DXX_USE_OGL
#include "ogl_init.h"
#endif

#include "pstypes.h"
#include "console.h"
#include "gr.h"
#include "inferno.h"
#include "game.h"
#include "key.h"
#include "config.h"
#include "object.h"
#include "physics.h"
#include "dxxerror.h"
#include "joy.h"
#include "iff.h"
#include "pcx.h"
#include "timer.h"
#include "render.h"
#include "laser.h"
#include "screens.h"
#include "textures.h"
#include "slew.h"
#include "gauges.h"
#include "texmap.h"
#include "3d.h"
#include "effects.h"
#include "menu.h"
#include "player.h"
#include "gameseg.h"
#include "wall.h"
#include "ai.h"
#include "fuelcen.h"
#include "digi.h"
#include "u_mem.h"
#include "palette.h"
#include "morph.h"
#include "lighting.h"
#include "newdemo.h"
#include "collide.h"
#include "weapon.h"
#include "sounds.h"
#include "args.h"
#include "gameseq.h"
#include "automap.h"
#include "text.h"
#include "powerup.h"
#include "fireball.h"
#include "newmenu.h"
#include "gamefont.h"
#include "endlevel.h"
#include "kconfig.h"
#include "mouse.h"
#include "switch.h"
#include "controls.h"
#include "songs.h"
#include "rbaudio.h"

#include "multi.h"
#include "cntrlcen.h"
#include "pcx.h"
#include "state.h"
#include "piggy.h"
#include "multibot.h"
#include "fvi.h"
#include "ai.h"
#include "robot.h"
#include "playsave.h"
#include "maths.h"
#include "hudmsg.h"
#if defined(DXX_BUILD_DESCENT_II)
#include <climits>
#include "gamepal.h"
#include "movie.h"
#endif
#include "event.h"
#include "window.h"

#if DXX_USE_EDITOR
#include "editor/editor.h"
#include "editor/esegment.h"
#endif

#include "d_enumerate.h"
#include "compiler-range_for.h"
#include "partial_range.h"
#include "segiter.h"

static fix64 last_timer_value=0;
static fix64 sync_timer_value=0;
fix ThisLevelTime=0;

grs_canvas	Screen_3d_window;							// The rectangle for rendering the mine to

namespace dcx {
int	force_cockpit_redraw=0;
int	PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd;

int	Game_suspended=0; //if non-zero, nothing moves but player
int	Game_mode = GM_GAME_OVER;
int	Global_missile_firing_count = 0;
}

//	Function prototypes for GAME.C exclusively.

namespace dsx {
static window_event_result GameProcessFrame(void);
static bool FireLaser(player_info &);
static void powerup_grab_cheat_all();

#if defined(DXX_BUILD_DESCENT_II)
d_flickering_light_state Flickering_light_state;
static void slide_textures(void);
static void flicker_lights(d_flickering_light_state &fls, fvmsegptridx &vmsegptridx);
#endif

// Cheats
game_cheats cheats;

//	==============================================================================================

//this is called once per game
void init_game()
{
	init_objects();

	init_special_effects();
	Clear_window = 2;		//	do portal only window clear.
}

}

namespace dcx {

void reset_palette_add()
{
	PaletteRedAdd 		= 0;
	PaletteGreenAdd	= 0;
	PaletteBlueAdd		= 0;
}

screen_mode Game_screen_mode{640, 480};

}

namespace dsx {

//initialize the various canvases on the game screen
//called every time the screen mode or cockpit changes
void init_cockpit()
{
	//Initialize the on-screen canvases

	if (Screen_mode != SCREEN_GAME)
		return;

	if ( Screen_mode == SCREEN_EDITOR )
		PlayerCfg.CockpitMode[1] = CM_FULL_SCREEN;

#if !DXX_USE_OGL
	if (PlayerCfg.CockpitMode[1] != CM_LETTERBOX)
	{
#if defined(DXX_BUILD_DESCENT_II)
		int HiresGFXAvailable = !GameArg.GfxSkipHiresGFX;
#endif
		auto full_screen_mode = HiresGFXAvailable ? screen_mode{640, 480} : screen_mode{320, 200};
		if (Game_screen_mode != full_screen_mode) {
			PlayerCfg.CockpitMode[1] = CM_FULL_SCREEN;
		}
	}
#endif

	gr_set_default_canvas();

	switch( PlayerCfg.CockpitMode[1] ) {
		case CM_FULL_COCKPIT:
			game_init_render_sub_buffers(0, 0, SWIDTH, (SHEIGHT*2)/3);
			break;

		case CM_REAR_VIEW:
		{
			unsigned x1 = 0, y1 = 0, x2 = SWIDTH, y2 = (SHEIGHT*2)/3;
			int mode = PlayerCfg.CockpitMode[1];
#if defined(DXX_BUILD_DESCENT_II)
			mode += (HIRESMODE?(Num_cockpits/2):0);
#endif

			PIGGY_PAGE_IN(cockpit_bitmap[mode]);
			auto &bm = GameBitmaps[cockpit_bitmap[mode].index];
			gr_bitblt_find_transparent_area(bm, x1, y1, x2, y2);
			game_init_render_sub_buffers(x1*(static_cast<float>(SWIDTH)/bm.bm_w), y1*(static_cast<float>(SHEIGHT)/bm.bm_h), (x2-x1+1)*(static_cast<float>(SWIDTH)/bm.bm_w), (y2-y1+2)*(static_cast<float>(SHEIGHT)/bm.bm_h));
			break;
		}

		case CM_FULL_SCREEN:
			game_init_render_sub_buffers(0, 0, SWIDTH, SHEIGHT);
			break;

		case CM_STATUS_BAR:
			game_init_render_sub_buffers( 0, 0, SWIDTH, (HIRESMODE?(SHEIGHT*2)/2.6:(SHEIGHT*2)/2.72) );
			break;

		case CM_LETTERBOX:	{
			const unsigned gsm_height = SM_H(Game_screen_mode);
			const unsigned w = SM_W(Game_screen_mode);
			const unsigned h = (gsm_height * 3) / 4; // true letterbox size (16:9)
			const unsigned x = 0;
			const unsigned y = (gsm_height - h) / 2;

			const uint8_t color = 0;
			auto &canvas = *grd_curcanv;
			gr_rect(canvas, x, 0, w, gsm_height - h, color);
			gr_rect(canvas, x, gsm_height - h, w, gsm_height, color);

			game_init_render_sub_buffers( x, y, w, h );
			break;
		}
	}

	gr_set_default_canvas();
}

}

//selects a given cockpit (or lack of one).  See types in game.h
void select_cockpit(cockpit_mode_t mode)
{
	if (mode != PlayerCfg.CockpitMode[1]) {		//new mode
		PlayerCfg.CockpitMode[1]=mode;
		init_cockpit();
	}
}

namespace dcx {

//force cockpit redraw next time. call this if you've trashed the screen
void reset_cockpit()
{
	force_cockpit_redraw=1;
	last_drawn_cockpit = -1;
}

void game_init_render_sub_buffers( int x, int y, int w, int h )
{
	gr_clear_canvas(*grd_curcanv, 0);
	gr_init_sub_canvas(Screen_3d_window, grd_curscreen->sc_canvas, x, y, w, h);
}

}

namespace dsx {

//called to change the screen mode. Parameter sm is the new mode, one of
//SMODE_GAME or SMODE_EDITOR. returns mode acutally set (could be other
//mode if cannot init requested mode)
int set_screen_mode(int sm)
{
	if ( (Screen_mode == sm) && !((sm==SCREEN_GAME) && (grd_curscreen->get_screen_mode() != Game_screen_mode)) && !(sm==SCREEN_MENU) )
	{
		return 1;
	}

#if DXX_USE_EDITOR
	Canv_editor = NULL;
#endif

	Screen_mode = sm;

#if SDL_MAJOR_VERSION == 1
	switch( Screen_mode )
	{
		case SCREEN_MENU:
			if  (grd_curscreen->get_screen_mode() != Game_screen_mode)
				if (gr_set_mode(Game_screen_mode))
					Error("Cannot set screen mode.");
			break;

		case SCREEN_GAME:
			if  (grd_curscreen->get_screen_mode() != Game_screen_mode)
				if (gr_set_mode(Game_screen_mode))
					Error("Cannot set screen mode.");
			break;
#if DXX_USE_EDITOR
		case SCREEN_EDITOR:
		{
			const screen_mode editor_mode{800, 600};
			if (grd_curscreen->get_screen_mode() != editor_mode)
			{
				int gr_error;
				if ((gr_error = gr_set_mode(editor_mode)) != 0) { //force into game scrren
					Warning("Cannot init editor screen (error=%d)",gr_error);
					return 0;
				}
			}
		}
			break;
#endif
#if defined(DXX_BUILD_DESCENT_II)
		case SCREEN_MOVIE:
		{
			const screen_mode movie_mode{MOVIE_WIDTH, MOVIE_HEIGHT};
			if (grd_curscreen->get_screen_mode() != movie_mode)
			{
				if (gr_set_mode(movie_mode))
					Error("Cannot set screen mode for game!");
				gr_palette_load( gr_palette );
			}
		}
			break;
#endif
		default:
			Error("Invalid screen mode %d",sm);
	}
#endif
	return 1;
}

}

namespace dcx {

namespace {

class game_world_time_paused
{
	unsigned time_paused;
public:
	explicit operator bool() const
	{
		return time_paused;
	}
	void increase_pause_count();
	void decrease_pause_count();
};

}

static game_world_time_paused time_paused;

void game_world_time_paused::increase_pause_count()
{
	if (time_paused==0) {
		const fix64 time = timer_update();
		last_timer_value = time - last_timer_value;
		if (last_timer_value < 0) {
			last_timer_value = 0;
		}
	}
	time_paused++;
}

void game_world_time_paused::decrease_pause_count()
{
	Assert(time_paused > 0);
	--time_paused;
	if (time_paused==0) {
		const fix64 time = timer_update();
		last_timer_value = time - last_timer_value;
	}
}

void start_time()
{
	time_paused.decrease_pause_count();
}

void stop_time()
{
	time_paused.increase_pause_count();
}

pause_game_world_time::pause_game_world_time()
{
	stop_time();
}

pause_game_world_time::~pause_game_world_time()
{
	start_time();
}

static void game_flush_common_inputs()
{
	event_flush();
	key_flush();
	joy_flush();
	mouse_flush();
}

}

namespace dsx {

void game_flush_inputs()
{
	Controls = {};
	game_flush_common_inputs();
}

}

namespace dcx {

void game_flush_respawn_inputs()
{
	static_cast<control_info::fire_controls_t &>(Controls.state) = {};
}

/*
 * timer that every sets d_tick_step true and increments d_tick_count every 1000/DESIGNATED_GAME_FPS ms.
 */
void calc_d_tick()
{
	static fix timer = 0;
	auto t = timer + FrameTime;

	d_tick_step = t >= DESIGNATED_GAME_FRAMETIME;
	if (d_tick_step)
	{
		d_tick_count++;
		if (d_tick_count > 1000000)
			d_tick_count = 0;
		t -= DESIGNATED_GAME_FRAMETIME;
	}
	timer = t;
}

void reset_time()
{
	last_timer_value = timer_update();
}

}

void calc_frame_time()
{
	fix last_frametime = FrameTime;

	const auto vsync = CGameCfg.VSync;
	const auto bound = f1_0 / (likely(vsync) ? MAXIMUM_FPS : CGameArg.SysMaxFPS);
	const auto may_sleep = !CGameArg.SysNoNiceFPS && !vsync;
	for (;;)
	{
		const auto timer_value = timer_update();
		FrameTime = timer_value - last_timer_value;
		if (FrameTime > 0 && timer_value - sync_timer_value >= bound)
		{
			last_timer_value = timer_value;

			sync_timer_value += bound;
			if (sync_timer_value + bound < timer_value) {
				sync_timer_value = timer_value;
			}
			break;
		}
		if (Game_mode & GM_MULTI)
			multi_do_frame(); // during long wait, keep packets flowing
		if (may_sleep)
			timer_delay_ms(1);
	}

	if ( cheats.turbo )
		FrameTime *= 2;

	if (FrameTime < 0)				//if bogus frametime...
		FrameTime = (last_frametime==0?1:last_frametime);		//...then use time from last frame

	GameTime64 += FrameTime;

	calc_d_tick();
#ifdef NEWHOMER
        calc_d_homer_tick();
#endif
}

namespace dsx {

void move_player_2_segment(const vmsegptridx_t seg,int side)
{
	const auto &&console = vmobjptridx(ConsoleObject);
	compute_segment_center(vcvertptr, console->pos, seg);
	auto vp = compute_center_point_on_side(vcvertptr, seg, side);
	vm_vec_sub2(vp, console->pos);
	vm_vector_2_matrix(console->orient, vp, nullptr, nullptr);
	obj_relink(vmobjptr, vmsegptr, console, seg);
}

}

namespace dcx {

namespace {

#if DXX_USE_SCREENSHOT_FORMAT_PNG
struct RAIIpng_struct
{
	png_struct *png_ptr;
	png_info *info_ptr = nullptr;
	RAIIpng_struct(png_struct *const p) :
		png_ptr(p)
	{
	}
	~RAIIpng_struct()
	{
		png_destroy_write_struct(&png_ptr, &info_ptr);
	}
};

struct d_screenshot : RAIIpng_struct
{
	DXX_INHERIT_CONSTRUCTORS(d_screenshot,RAIIpng_struct);
	/* error handling callbacks */
	[[noreturn]]
	static void png_error_cb(png_struct *png, const char *str);
	static void png_warn_cb(png_struct *png, const char *str);
	/* output callbacks */
	static void png_write_cb(png_struct *png, uint8_t *buf, png_size_t);
	static void png_flush_cb(png_struct *png);
	class png_exception : std::exception
	{
	};
};

void d_screenshot::png_error_cb(png_struct *const png, const char *const str)
{
	/* libpng requires that this function not return to its caller, and
	 * will abort the program if this requirement is violated.  However,
	 * throwing an exception that unwinds out past libpng is permitted.
	 */
	(void)png;
	con_printf(CON_URGENT, "libpng error: %s", str);
	throw png_exception();
}

void d_screenshot::png_warn_cb(png_struct *const png, const char *const str)
{
	(void)png;
	con_printf(CON_URGENT, "libpng warning: %s", str);
}

void d_screenshot::png_write_cb(png_struct *const png, uint8_t *const buf, const png_size_t size)
{
	const auto file = reinterpret_cast<PHYSFS_File *>(png_get_io_ptr(png));
	PHYSFS_write(file, buf, size, 1);
}

void d_screenshot::png_flush_cb(png_struct *const png)
{
	(void)png;
}

void record_screenshot_time(const struct tm &tm, png_struct *const png_ptr, png_info *const info_ptr)
{
#ifdef PNG_tIME_SUPPORTED
	png_time pt{};
	pt.year = tm.tm_year + 1900;
	pt.month = tm.tm_mon + 1;
	pt.day = tm.tm_mday;
	pt.hour = tm.tm_hour;
	pt.minute = tm.tm_min;
	pt.second = tm.tm_sec;
	png_set_tIME(png_ptr, info_ptr, &pt);
#else
	(void)png_ptr;
	(void)info_ptr;
	con_printf(CON_NORMAL, "libpng configured without support for time chunk: screenshot will lack time record.");
#endif
}

#ifdef PNG_TEXT_SUPPORTED
void record_screenshot_text_metadata(png_struct *const png_ptr, png_info *const info_ptr)
{
	array<png_text, 6> text_fields{};
	char descent_version[80];
	char descent_build_datetime[21];
	std::string current_mission_path;
	ntstring<MISSION_NAME_LEN> current_mission_name;
	char current_level_number[4];
	char viewer_segment[sizeof("65536")];
	unsigned idx = 0;
	char key_descent_version[] = "Rebirth.version";
	{
		auto &t = text_fields[idx++];
		auto &text = descent_version;
		t.key = key_descent_version;
		text[sizeof(text) - 1] = 0;
		strncpy(text, g_descent_version, sizeof(text) - 1);
		t.text = text;
		t.compression = PNG_TEXT_COMPRESSION_NONE;
	}
	char key_descent_build_datetime[] = "Rebirth.build_datetime";
	{
		auto &t = text_fields[idx++];
		auto &text = descent_build_datetime;
		t.key = key_descent_build_datetime;
		text[sizeof(text) - 1] = 0;
		strncpy(text, g_descent_build_datetime, sizeof(text) - 1);
		t.text = text;
		t.compression = PNG_TEXT_COMPRESSION_NONE;
	}
	char key_current_mission_path[] = "Rebirth.mission.pathname";
	char key_current_mission_name[] = "Rebirth.mission.textname";
	char key_viewer_segment[] = "Rebirth.viewer_segment";
	char key_current_level_number[] = "Rebirth.current_level_number";
	if (const auto current_mission = Current_mission.get())
	{
		{
			auto &t = text_fields[idx++];
			t.key = key_current_mission_path;
			current_mission_path = current_mission->path;
			t.text = &current_mission_path[0];
			t.compression = PNG_TEXT_COMPRESSION_NONE;
		}
		{
			auto &t = text_fields[idx++];
			t.key = key_current_mission_name;
			current_mission_name = current_mission->mission_name;
			t.text = current_mission_name.data();
			t.compression = PNG_TEXT_COMPRESSION_NONE;
		}
		{
			auto &t = text_fields[idx++];
			t.key = key_current_level_number;
			t.text = current_level_number;
			t.compression = PNG_TEXT_COMPRESSION_NONE;
			snprintf(current_level_number, sizeof(current_level_number), "%i", Current_level_num);
		}
		if (const auto viewer = Viewer)
		{
			auto &t = text_fields[idx++];
			t.key = key_viewer_segment;
			t.text = viewer_segment;
			t.compression = PNG_TEXT_COMPRESSION_NONE;
			snprintf(viewer_segment, sizeof(viewer_segment), "%hu", viewer->segnum);
		}
	}
	png_set_text(png_ptr, info_ptr, text_fields.data(), idx);
}
#endif

#if DXX_USE_OGL
#define write_screenshot_png(F,T,B,P)	write_screenshot_png(F,T,B)
#endif
unsigned write_screenshot_png(PHYSFS_File *const file, const struct tm *const tm, const grs_bitmap &bitmap, const palette_array_t &pal)
{
	const unsigned bm_w = ((bitmap.bm_w + 3) & ~3);
	const unsigned bm_h = ((bitmap.bm_h + 3) & ~3);
#if DXX_USE_OGL
	const unsigned bufsize = bm_w * bm_h * 3;
	const auto buf = std::make_unique<uint8_t[]>(bufsize);
	const auto begin_byte_buffer = buf.get();
	glReadPixels(0, 0, bm_w, bm_h, GL_RGB, GL_UNSIGNED_BYTE, begin_byte_buffer);
#else
	const unsigned bufsize = bitmap.bm_rowsize * bm_h;
	const auto begin_byte_buffer = bitmap.bm_mdata;
#endif
	d_screenshot ss(png_create_write_struct(PNG_LIBPNG_VER_STRING, &ss, &d_screenshot::png_error_cb, &d_screenshot::png_warn_cb));
	if (!ss.png_ptr)
	{
		con_puts(CON_URGENT, "Cannot save screenshot: libpng png_create_write_struct failed");
		return 1;
	}
	/* Assert that Rebirth type rgb_t is layout compatible with
	 * libpng type png_color, so that the Rebirth palette_array_t
	 * can be safely reinterpret_cast to an array of png_color.
	 * Without this, it would be necessary to copy each rgb_t
	 * palette entry into a libpng png_color.
	 */
	static_assert(sizeof(png_color) == sizeof(rgb_t), "size mismatch");
	static_assert(offsetof(png_color, red) == offsetof(rgb_t, r), "red offsetof mismatch");
	static_assert(offsetof(png_color, green) == offsetof(rgb_t, g), "green offsetof mismatch");
	static_assert(offsetof(png_color, blue) == offsetof(rgb_t, b), "blue offsetof mismatch");
	try {
		ss.info_ptr = png_create_info_struct(ss.png_ptr);
		if (tm)
			record_screenshot_time(*tm, ss.png_ptr, ss.info_ptr);
		png_set_write_fn(ss.png_ptr, file, &d_screenshot::png_write_cb, &d_screenshot::png_flush_cb);
#if DXX_USE_OGL
		const auto color_type = PNG_COLOR_TYPE_RGB;
#else
		png_set_PLTE(ss.png_ptr, ss.info_ptr, reinterpret_cast<const png_color *>(pal.data()), pal.size());
		const auto color_type = PNG_COLOR_TYPE_PALETTE;
#endif
		png_set_IHDR(ss.png_ptr, ss.info_ptr, bm_w, bm_h, 8 /* always 256 colors */, color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
#ifdef PNG_TEXT_SUPPORTED
		record_screenshot_text_metadata(ss.png_ptr, ss.info_ptr);
#endif
		png_write_info(ss.png_ptr, ss.info_ptr);
		array<png_byte *, 1024> row_pointers;
		const auto rpb = row_pointers.begin();
		auto o = rpb;
		const auto end_byte_buffer = begin_byte_buffer + bufsize;
#if DXX_USE_OGL
		/* OpenGL glReadPixels returns an image with origin in bottom
		 * left.  Write rows from end back to beginning, since PNG
		 * expects origin in top left.  If rows were written in memory
		 * order, the image would be vertically flipped.
		 */
		const uint_fast32_t stride = bm_w * 3;	/* Without palette, written data is 3-byte-sized RGB tuples of color */
		for (auto p = end_byte_buffer; p != begin_byte_buffer;)
#else
		const uint_fast32_t stride = bm_w;	/* With palette, written data is byte-sized indices into a color table */
		/* SDL canvas uses an image with origin in top left.  Write rows
		 * in memory order, since this matches the PNG layout.
		 */
		for (auto p = begin_byte_buffer; p != end_byte_buffer;)
#endif
		{
#if DXX_USE_OGL
			p -= stride;
#else
			p += stride;
#endif
			*o++ = p;
			if (o == row_pointers.end())
			{
				/* Internal capacity exhausted.  Flush rows and rewind
				 * to the beginning of the array.
				 */
				o = rpb;
				png_write_rows(ss.png_ptr, o, row_pointers.size());
			}
		}
		/* Flush any trailing rows */
		if (const auto len = o - rpb)
			png_write_rows(ss.png_ptr, rpb, len);
		png_write_end(ss.png_ptr, ss.info_ptr);
		return 0;
	} catch (const d_screenshot::png_exception &) {
		/* Destructor unwind will handle the exception.  This catch is
		 * only required to prevent further propagation.
		 *
		 * Return nonzero to instruct the caller to delete the failed
		 * file.
		 */
		return 1;
	}
}
#endif

}

#if DXX_USE_SCREENSHOT
void save_screen_shot(int automap_flag)
{
#if DXX_USE_OGL
	if (!CGameArg.DbgGlReadPixelsOk)
	{
		if (!automap_flag)
			HUD_init_message_literal(HM_DEFAULT, "glReadPixels not supported on your configuration");
		return;
	}
#endif
#if DXX_USE_SCREENSHOT_FORMAT_PNG
#define DXX_SCREENSHOT_FILE_EXTENSION	"png"
#elif DXX_USE_SCREENSHOT_FORMAT_LEGACY
#if DXX_USE_OGL
#define DXX_SCREENSHOT_FILE_EXTENSION	"tga"
#else
#define DXX_SCREENSHOT_FILE_EXTENSION	"pcx"
#endif
#endif

	if (!PHYSFSX_exists(SCRNS_DIR,0))
		PHYSFS_mkdir(SCRNS_DIR); //try making directory

	pause_game_world_time p;
	unsigned tm_sec;
	unsigned tm_min;
	unsigned tm_hour;
	unsigned tm_mday;
	unsigned tm_mon;
	unsigned tm_year;
	const auto t = time(nullptr);
	struct tm *tm = nullptr;
	if (t == static_cast<time_t>(-1) || !(tm = gmtime(&t)))
		tm_year = tm_mon = tm_mday = tm_hour = tm_min = tm_sec = 0;
	else
	{
		tm_sec = tm->tm_sec;
		tm_min = tm->tm_min;
		tm_hour = tm->tm_hour;
		tm_mday = tm->tm_mday;
		tm_mon = tm->tm_mon + 1;
		tm_year = tm->tm_year + 1900;
	}
	/* Colon is not legal in Windows filenames, so use - to separate
	 * hour:minute:second.
	 */
#define DXX_SCREENSHOT_TIME_FORMAT_STRING	SCRNS_DIR "%04u-%02u-%02u.%02u-%02u-%02u"
#define DXX_SCREENSHOT_TIME_FORMAT_VALUES	tm_year, tm_mon, tm_mday, tm_hour, tm_min, tm_sec
	/* Reserve extra space for the trailing -NN disambiguation.  This
	 * is only used if multiple screenshots get the same time, so it is
	 * unlikely to be seen in practice and very unlikely to be exhausted
	 * (unless the clock is frozen or returns invalid times).
	 */
	char savename[sizeof(SCRNS_DIR) + sizeof("2000-01-01.00-00-00.NN.ext")];
	snprintf(savename, sizeof(savename), DXX_SCREENSHOT_TIME_FORMAT_STRING "." DXX_SCREENSHOT_FILE_EXTENSION, DXX_SCREENSHOT_TIME_FORMAT_VALUES);
	for (unsigned savenum = 0; PHYSFS_exists(savename) && savenum != 100; ++savenum)
	{
		snprintf(savename, sizeof(savename), DXX_SCREENSHOT_TIME_FORMAT_STRING ".%02u." DXX_SCREENSHOT_FILE_EXTENSION, DXX_SCREENSHOT_TIME_FORMAT_VALUES, savenum);
#undef DXX_SCREENSHOT_TIME_FORMAT_VALUES
#undef DXX_SCREENSHOT_TIME_FORMAT_STRING
#undef DXX_SCREENSHOT_FILE_EXTENSION
	}
	unsigned write_error;
	if (const auto file = PHYSFSX_openWriteBuffered(savename))
	{
	if (!automap_flag)
		HUD_init_message(HM_DEFAULT, "%s '%s'", TXT_DUMPING_SCREEN, &savename[sizeof(SCRNS_DIR) - 1]);

#if DXX_USE_OGL
#if !DXX_USE_OGLES
	glReadBuffer(GL_FRONT);
#endif
#if DXX_USE_SCREENSHOT_FORMAT_PNG
	write_error = write_screenshot_png(file, tm, grd_curscreen->sc_canvas.cv_bitmap, void /* unused */);
#elif DXX_USE_SCREENSHOT_FORMAT_LEGACY
	write_bmp(file, grd_curscreen->get_screen_width(), grd_curscreen->get_screen_height());
	/* write_bmp never fails */
	write_error = 0;
#endif
#else
	grs_canvas &screen_canv = grd_curscreen->sc_canvas;
	palette_array_t pal;

	const auto &&temp_canv = gr_create_canvas(screen_canv.cv_bitmap.bm_w, screen_canv.cv_bitmap.bm_h);
	gr_ubitmap(*temp_canv, screen_canv.cv_bitmap);

	gr_palette_read(pal);		//get actual palette from the hardware
	// Correct palette colors
	range_for (auto &i, pal)
	{
		i.r <<= 2;
		i.g <<= 2;
		i.b <<= 2;
	}
#if DXX_USE_SCREENSHOT_FORMAT_PNG
	write_error = write_screenshot_png(file, tm, grd_curscreen->sc_canvas.cv_bitmap, pal);
#elif DXX_USE_SCREENSHOT_FORMAT_LEGACY
	write_error = pcx_write_bitmap(file, &temp_canv->cv_bitmap, pal);
#endif
#endif
	}
	else
	{
		if (!automap_flag)
			HUD_init_message(HM_DEFAULT, "Failed to open screenshot file for writing: %s", &savename[sizeof(SCRNS_DIR) - 1]);
		else
			con_printf(CON_URGENT, "Failed to open screenshot file for writing: %s", savename);
		return;
	}
	if (write_error)
		PHYSFS_delete(savename);
}
#endif

//initialize flying
void fly_init(object_base &obj)
{
	obj.control_type = CT_FLYING;
	obj.movement_type = MT_PHYSICS;

	obj.mtype.phys_info.velocity = {};
	obj.mtype.phys_info.thrust = {};
	obj.mtype.phys_info.rotvel = {};
	obj.mtype.phys_info.rotthrust = {};
}

}

namespace dsx {

//	------------------------------------------------------------------------------------
static void do_cloak_stuff(void)
{
	range_for (auto &&e, enumerate(partial_range(Players, N_players)))
	{
		auto &plobj = *vmobjptr(e.value.objnum);
		auto &player_info = plobj.ctype.player_info;
		auto &pl_flags = player_info.powerup_flags;
		if (pl_flags & PLAYER_FLAGS_CLOAKED)
		{
			if (GameTime64 > player_info.cloak_time+CLOAK_TIME_MAX)
			{
				pl_flags &= ~PLAYER_FLAGS_CLOAKED;
				auto &i = e.idx;
				if (i == Player_num) {
					multi_digi_play_sample(SOUND_CLOAK_OFF, F1_0);
					maybe_drop_net_powerup(POW_CLOAK, 1, 0);
					if ( Newdemo_state != ND_STATE_PLAYBACK )
						multi_send_decloak(); // For demo recording
				}
			}
		}
	}
}

//	------------------------------------------------------------------------------------
static void do_invulnerable_stuff(player_info &player_info)
{
	auto &pl_flags = player_info.powerup_flags;
	if (pl_flags & PLAYER_FLAGS_INVULNERABLE)
	{
		if (GameTime64 > player_info.invulnerable_time + INVULNERABLE_TIME_MAX)
		{
			pl_flags &= ~PLAYER_FLAGS_INVULNERABLE;
			if (auto &FakingInvul = player_info.FakingInvul)
			{
				FakingInvul = 0;
				return;
			}
				multi_digi_play_sample(SOUND_INVULNERABILITY_OFF, F1_0);
				if (Game_mode & GM_MULTI)
				{
					maybe_drop_net_powerup(POW_INVULNERABILITY, 1, 0);
				}
		}
	}
}

#if defined(DXX_BUILD_DESCENT_I)
static inline void do_afterburner_stuff(object_array &)
{
}
#elif defined(DXX_BUILD_DESCENT_II)
ubyte	Last_afterburner_state = 0;
static fix Last_afterburner_charge;
fix64	Time_flash_last_played;

#define AFTERBURNER_LOOP_START	((GameArg.SndDigiSampleRate==SAMPLE_RATE_22K)?32027:(32027/2))		//20098
#define AFTERBURNER_LOOP_END		((GameArg.SndDigiSampleRate==SAMPLE_RATE_22K)?48452:(48452/2))		//25776

static void do_afterburner_stuff(object_array &objects)
{
	auto &vmobjptr = objects.vmptr;
	auto &vcobjptridx = objects.vcptridx;
	static sbyte func_play = 0;

	auto &player_info = get_local_plrobj().ctype.player_info;
	const auto have_afterburner = player_info.powerup_flags & PLAYER_FLAGS_AFTERBURNER;
	if (!have_afterburner)
		Afterburner_charge = 0;

	const auto plobj = vcobjptridx(get_local_player().objnum);
	if (Endlevel_sequence || Player_dead_state != player_dead_state::no)
	{
		digi_kill_sound_linked_to_object(plobj);
		if (Game_mode & GM_MULTI && func_play)
		{
			multi_send_sound_function (0,0);
			func_play = 0;
		}
	}

	if ((Controls.state.afterburner != Last_afterburner_state && Last_afterburner_charge) || (Last_afterburner_state && Last_afterburner_charge && !Afterburner_charge)) {
		if (Afterburner_charge && Controls.state.afterburner && have_afterburner) {
			digi_link_sound_to_object3(SOUND_AFTERBURNER_IGNITE, plobj, 1, F1_0, sound_stack::allow_stacking, vm_distance{i2f(256)}, AFTERBURNER_LOOP_START, AFTERBURNER_LOOP_END);
			if (Game_mode & GM_MULTI)
			{
				multi_send_sound_function (3,SOUND_AFTERBURNER_IGNITE);
				func_play = 1;
			}
		} else {
			digi_kill_sound_linked_to_object(plobj);
			digi_link_sound_to_object2(SOUND_AFTERBURNER_PLAY, plobj, 0, F1_0, sound_stack::allow_stacking, vm_distance{i2f(256)});
			if (Game_mode & GM_MULTI)
			{
			 	multi_send_sound_function (0,0);
				func_play = 0;
			}
		}
	}

	//@@if (Controls.state.afterburner && Afterburner_charge)
	//@@	afterburner_shake();

	Last_afterburner_state = Controls.state.afterburner;
	Last_afterburner_charge = Afterburner_charge;
}
#endif

//	Amount to diminish guns towards normal, per second.
#define	DIMINISH_RATE 16 // gots to be a power of 2, else change the code in diminish_palette_towards_normal

 //adds to rgb values for palette flash
void PALETTE_FLASH_ADD(int _dr, int _dg, int _db)
{
	int	maxval;

	PaletteRedAdd += _dr;
	PaletteGreenAdd += _dg;
	PaletteBlueAdd += _db;

#if defined(DXX_BUILD_DESCENT_II)
	if (Flash_effect)
		maxval = 60;
	else
#endif
		maxval = MAX_PALETTE_ADD;

	if (PaletteRedAdd > maxval)
		PaletteRedAdd = maxval;

	if (PaletteGreenAdd > maxval)
		PaletteGreenAdd = maxval;

	if (PaletteBlueAdd > maxval)
		PaletteBlueAdd = maxval;

	if (PaletteRedAdd < -maxval)
		PaletteRedAdd = -maxval;

	if (PaletteGreenAdd < -maxval)
		PaletteGreenAdd = -maxval;

	if (PaletteBlueAdd < -maxval)
		PaletteBlueAdd = -maxval;
}

}

static void diminish_palette_color_toward_zero(int& palette_color_add, const int& dec_amount)
{
	if (palette_color_add > 0 ) {
		if (palette_color_add < dec_amount)
			palette_color_add = 0;
		else
			palette_color_add -= dec_amount;
	} else if (palette_color_add < 0 ) {
		if (palette_color_add > -dec_amount )
			palette_color_add = 0;
		else
			palette_color_add += dec_amount;
	}
}

namespace dsx {

//	------------------------------------------------------------------------------------
//	Diminish palette effects towards normal.
static void diminish_palette_towards_normal(void)
{
	int	dec_amount = 0;
	float brightness_correction = 1-(static_cast<float>(gr_palette_get_gamma())/64); // to compensate for brightness setting of the game

	// Diminish at DIMINISH_RATE units/second.
	if (FrameTime < (F1_0/DIMINISH_RATE))
	{
		static fix diminish_timer = 0;
		diminish_timer += FrameTime;
		if (diminish_timer >= (F1_0/DIMINISH_RATE))
		{
			diminish_timer -= (F1_0/DIMINISH_RATE);
			dec_amount = 1;
		}
	}
	else
	{
		dec_amount = f2i(FrameTime*DIMINISH_RATE); // one second = DIMINISH_RATE counts
		if (dec_amount == 0)
			dec_amount++; // make sure we decrement by something
	}

#if defined(DXX_BUILD_DESCENT_II)
	if (Flash_effect) {
		int	force_do = 0;
		static fix Flash_step_up_timer = 0;

		// Part of hack system to force update of palette after exiting a menu.
		if (Time_flash_last_played) {
			force_do = 1;
			PaletteRedAdd ^= 1; // Very Tricky! In gr_palette_step_up, if all stepups same as last time, won't do anything!
		}

		if (Time_flash_last_played + F1_0/8 < GameTime64) {
			digi_play_sample( SOUND_CLOAK_OFF, Flash_effect/4);
			Time_flash_last_played = GameTime64;
		}

		Flash_effect -= FrameTime;
		Flash_step_up_timer += FrameTime;
		if (Flash_effect < 0)
			Flash_effect = 0;

		if (force_do || (Flash_step_up_timer >= F1_0/26)) // originally time interval based on (d_rand() > 4096)
		{
			Flash_step_up_timer -= (F1_0/26);
			if ( (Newdemo_state==ND_STATE_RECORDING) && (PaletteRedAdd || PaletteGreenAdd || PaletteBlueAdd) )
				newdemo_record_palette_effect(PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd);

			gr_palette_step_up( PaletteRedAdd*brightness_correction, PaletteGreenAdd*brightness_correction, PaletteBlueAdd*brightness_correction );

			return;
		}

	}
#endif

	diminish_palette_color_toward_zero(PaletteRedAdd, dec_amount);
	diminish_palette_color_toward_zero(PaletteGreenAdd, dec_amount);
	diminish_palette_color_toward_zero(PaletteBlueAdd, dec_amount);

	if ( (Newdemo_state==ND_STATE_RECORDING) && (PaletteRedAdd || PaletteGreenAdd || PaletteBlueAdd) )
		newdemo_record_palette_effect(PaletteRedAdd, PaletteGreenAdd, PaletteBlueAdd);

	gr_palette_step_up( PaletteRedAdd*brightness_correction, PaletteGreenAdd*brightness_correction, PaletteBlueAdd*brightness_correction );
}

}

namespace {

int	Redsave, Bluesave, Greensave;

}

#if defined(DXX_BUILD_DESCENT_II)
static
#endif
void palette_save(void)
{
	Redsave = PaletteRedAdd; Bluesave = PaletteBlueAdd; Greensave = PaletteGreenAdd;
}

namespace dsx {
void palette_restore(void)
{
	float brightness_correction = 1-(static_cast<float>(gr_palette_get_gamma())/64);

	PaletteRedAdd = Redsave; PaletteBlueAdd = Bluesave; PaletteGreenAdd = Greensave;
	gr_palette_step_up( PaletteRedAdd*brightness_correction, PaletteGreenAdd*brightness_correction, PaletteBlueAdd*brightness_correction );

#if defined(DXX_BUILD_DESCENT_II)
	//	Forces flash effect to fixup palette next frame.
	Time_flash_last_played = 0;
#endif
}

//	--------------------------------------------------------------------------------------------------
bool allowed_to_fire_laser(const player_info &player_info)
{
	if (Player_dead_state != player_dead_state::no)
	{
		Global_missile_firing_count = 0;
		return 0;
	}

	auto &Next_laser_fire_time = player_info.Next_laser_fire_time;
	//	Make sure enough time has elapsed to fire laser
	if (Next_laser_fire_time > GameTime64)
		return 0;

	return 1;
}

int allowed_to_fire_flare(player_info &player_info)
{
	auto &Next_flare_fire_time = player_info.Next_flare_fire_time;
	if (Next_flare_fire_time > GameTime64)
		return 0;

#if defined(DXX_BUILD_DESCENT_II)
	if (player_info.energy < Weapon_info[weapon_id_type::FLARE_ID].energy_usage)
#define	FLARE_BIG_DELAY	(F1_0*2)
		Next_flare_fire_time = GameTime64 + FLARE_BIG_DELAY;
	else
#endif
		Next_flare_fire_time = GameTime64 + F1_0/4;

	return 1;
}

int allowed_to_fire_missile(const player_info &player_info)
{
	auto &Next_missile_fire_time = player_info.Next_missile_fire_time;
	//	Make sure enough time has elapsed to fire missile
	if (Next_missile_fire_time > GameTime64)
		return 0;

	return 1;
}
}

#if defined(DXX_BUILD_DESCENT_II)
void full_palette_save(void)
{
	palette_save();
	reset_palette_add();
	gr_palette_load( gr_palette );
}
#endif

#if DXX_USE_SDLMIXER
#define EXT_MUSIC_TEXT "Jukebox/Audio CD"
#else
#define EXT_MUSIC_TEXT "Audio CD"
#endif

static int free_help(newmenu *, const d_event &event, newmenu_item *items)
{
	if (event.type == EVENT_WINDOW_CLOSE)
	{
		std::default_delete<newmenu_item[]>()(items);
	}
	return 0;
}

#if (defined(__APPLE__) || defined(macintosh))
#define _DXX_HELP_MENU_SAVE_LOAD(VERB)	\
	DXX_MENUITEM(VERB, TEXT, "Alt-F2/F3 (\x85-SHIFT-s/o)\t  SAVE/LOAD GAME", HELP_AF2_3)	\
	DXX_MENUITEM(VERB, TEXT, "Alt-Shift-F2/F3 (\x85-s/o)\t  Quick Save/Load", HELP_ASF2_3)
#define _DXX_HELP_MENU_PAUSE(VERB) DXX_MENUITEM(VERB, TEXT, "Pause (\x85-P)\t  Pause", HELP_PAUSE)

#if DXX_USE_SDL_REDBOOK_AUDIO
#define _DXX_HELP_MENU_AUDIO_REDBOOK(VERB)	\
	DXX_MENUITEM(VERB, TEXT, "\x85-E\t  Eject Audio CD", HELP_ASF9)	\

#else
#define _DXX_HELP_MENU_AUDIO_REDBOOK(VERB)
#endif

#define _DXX_HELP_MENU_AUDIO(VERB)	\
	_DXX_HELP_MENU_AUDIO_REDBOOK(VERB)	\
	DXX_MENUITEM(VERB, TEXT, "\x85-Up/Down\t  Play/Pause " EXT_MUSIC_TEXT, HELP_ASF10)	\
	DXX_MENUITEM(VERB, TEXT, "\x85-Left/Right\t  Previous/Next Song", HELP_ASF11_12)
#define _DXX_HELP_MENU_HINT_CMD_KEY(VERB, PREFIX)	\
	DXX_MENUITEM(VERB, TEXT, "", PREFIX##_SEP_HINT_CMD)	\
	DXX_MENUITEM(VERB, TEXT, "(Use \x85-# for F#. e.g. \x85-1 for F1)", PREFIX##_HINT_CMD)
#define _DXX_NETHELP_SAVELOAD_GAME(VERB)	\
	DXX_MENUITEM(VERB, TEXT, "Alt-F2/F3 (\x85-SHIFT-s/\x85-o)\t  SAVE/LOAD COOP GAME", NETHELP_SAVELOAD)
#else
#define _DXX_HELP_MENU_SAVE_LOAD(VERB)	\
	DXX_MENUITEM(VERB, TEXT, "Alt-F2/F3\t  SAVE/LOAD GAME", HELP_AF2_3)	\
	DXX_MENUITEM(VERB, TEXT, "Alt-Shift-F2/F3\t  Fast Save", HELP_ASF2_3)
#define _DXX_HELP_MENU_PAUSE(VERB)	DXX_MENUITEM(VERB, TEXT, TXT_HELP_PAUSE, HELP_PAUSE)

#if DXX_USE_SDL_REDBOOK_AUDIO
#define _DXX_HELP_MENU_AUDIO_REDBOOK(VERB)	\
	DXX_MENUITEM(VERB, TEXT, "Alt-Shift-F9\t  Eject Audio CD", HELP_ASF9)	\

#else
#define _DXX_HELP_MENU_AUDIO_REDBOOK(VERB)
#endif

#define _DXX_HELP_MENU_AUDIO(VERB)	\
	_DXX_HELP_MENU_AUDIO_REDBOOK(VERB)	\
	DXX_MENUITEM(VERB, TEXT, "Alt-Shift-F10\t  Play/Pause " EXT_MUSIC_TEXT, HELP_ASF10)	\
	DXX_MENUITEM(VERB, TEXT, "Alt-Shift-F11/F12\t  Previous/Next Song", HELP_ASF11_12)
#define _DXX_HELP_MENU_HINT_CMD_KEY(VERB, PREFIX)
#define _DXX_NETHELP_SAVELOAD_GAME(VERB)	\
	DXX_MENUITEM(VERB, TEXT, "Alt-F2/F3\t  SAVE/LOAD COOP GAME", NETHELP_SAVELOAD)
#endif

#if defined(DXX_BUILD_DESCENT_II)
#define _DXX_HELP_MENU_D2_DXX_F4(VERB)	DXX_MENUITEM(VERB, TEXT, TXT_HELP_F4, HELP_F4)
#define _DXX_HELP_MENU_D2_DXX_FEATURES(VERB)	\
	DXX_MENUITEM(VERB, TEXT, "Shift-F1/F2\t  Cycle left/right window", HELP_SF1_2)	\
	DXX_MENUITEM(VERB, TEXT, "Shift-F4\t  GuideBot menu", HELP_SF4)	\
	DXX_MENUITEM(VERB, TEXT, "Alt-Shift-F4\t  Rename GuideBot", HELP_ASF4)	\
	DXX_MENUITEM(VERB, TEXT, "Shift-F5/F6\t  Drop primary/secondary", HELP_SF5_6)	\
	DXX_MENUITEM(VERB, TEXT, "Shift-number\t  GuideBot commands", HELP_GUIDEBOT_COMMANDS)
#define _DXX_NETHELP_DROPFLAG(VERB)	\
	DXX_MENUITEM(VERB, TEXT, "ALT-0\t  DROP FLAG", NETHELP_DROPFLAG)
#else
#define _DXX_HELP_MENU_D2_DXX_F4(VERB)
#define _DXX_HELP_MENU_D2_DXX_FEATURES(VERB)
#define _DXX_NETHELP_DROPFLAG(VERB)
#endif

#define DXX_HELP_MENU(VERB)	\
	DXX_MENUITEM(VERB, TEXT, TXT_HELP_ESC, HELP_ESC)	\
	DXX_MENUITEM(VERB, TEXT, "SHIFT-ESC\t  SHOW GAME LOG", HELP_LOG)	\
	DXX_MENUITEM(VERB, TEXT, "F1\t  THIS SCREEN", HELP_HELP)	\
	DXX_MENUITEM(VERB, TEXT, TXT_HELP_F2, HELP_F2)	\
	_DXX_HELP_MENU_SAVE_LOAD(VERB)	\
	DXX_MENUITEM(VERB, TEXT, "F3\t  SWITCH COCKPIT MODES", HELP_F3)	\
	_DXX_HELP_MENU_D2_DXX_F4(VERB)	\
	DXX_MENUITEM(VERB, TEXT, TXT_HELP_F5, HELP_F5)	\
	DXX_MENUITEM(VERB, TEXT, "ALT-F7\t  SWITCH HUD MODES", HELP_AF7)	\
	_DXX_HELP_MENU_PAUSE(VERB)	\
	DXX_MENUITEM(VERB, TEXT, TXT_HELP_PRTSCN, HELP_PRTSCN)	\
	DXX_MENUITEM(VERB, TEXT, TXT_HELP_1TO5, HELP_1TO5)	\
	DXX_MENUITEM(VERB, TEXT, TXT_HELP_6TO10, HELP_6TO10)	\
	_DXX_HELP_MENU_D2_DXX_FEATURES(VERB)	\
	_DXX_HELP_MENU_AUDIO(VERB)	\
	_DXX_HELP_MENU_HINT_CMD_KEY(VERB, HELP)	\

enum {
	DXX_HELP_MENU(ENUM)
};

void show_help()
{
	const unsigned nitems = DXX_HELP_MENU(COUNT);
	auto m = new newmenu_item[nitems];
	DXX_HELP_MENU(ADD);
	newmenu_dotiny(NULL, TXT_KEYS, nitems, m, 0, free_help, m);
}

#undef DXX_HELP_MENU

#define DXX_NETHELP_MENU(VERB)	\
	DXX_MENUITEM(VERB, TEXT, "F1\t  THIS SCREEN", NETHELP_HELP)	\
	_DXX_NETHELP_DROPFLAG(VERB)	\
	_DXX_NETHELP_SAVELOAD_GAME(VERB)	\
	DXX_MENUITEM(VERB, TEXT, "ALT-F4\t  SHOW PLAYER NAMES ON HUD", NETHELP_HUDNAMES)	\
	DXX_MENUITEM(VERB, TEXT, "F7\t  TOGGLE KILL LIST", NETHELP_TOGGLE_KILL_LIST)	\
	DXX_MENUITEM(VERB, TEXT, "F8\t  SEND MESSAGE", NETHELP_SENDMSG)	\
	DXX_MENUITEM(VERB, TEXT, "(SHIFT-)F9 to F12\t  (DEFINE)SEND MACRO", NETHELP_MACRO)	\
	DXX_MENUITEM(VERB, TEXT, "PAUSE\t  SHOW NETGAME INFORMATION", NETHELP_GAME_INFO)	\
	DXX_MENUITEM(VERB, TEXT, "SHIFT-PAUSE\t  SHOW NETGAME INFO & RULES", NETHELP_GAME_INFORULES)	\
	_DXX_HELP_MENU_HINT_CMD_KEY(VERB, NETHELP)	\
	DXX_MENUITEM(VERB, TEXT, "", NETHELP_SEP1)	\
	DXX_MENUITEM(VERB, TEXT, "MULTIPLAYER MESSAGE COMMANDS:", NETHELP_COMMAND_HEADER)	\
	DXX_MENUITEM(VERB, TEXT, "(*): TEXT\t  SEND TEXT TO PLAYER/TEAM (*)", NETHELP_DIRECT_MESSAGE)	\
	DXX_MENUITEM(VERB, TEXT, "/Handicap: (*)\t  SET YOUR STARTING SHIELDS TO (*) [10-100]", NETHELP_COMMAND_HANDICAP)	\
	DXX_MENUITEM(VERB, TEXT, "/move: (*)\t  MOVE PLAYER (*) TO OTHER TEAM (Host-only)", NETHELP_COMMAND_MOVE)	\
	DXX_MENUITEM(VERB, TEXT, "/kick: (*)\t  KICK PLAYER (*) FROM GAME (Host-only)", NETHELP_COMMAND_KICK)	\
	DXX_MENUITEM(VERB, TEXT, "/KillReactor\t  BLOW UP THE MINE (Host-only)", NETHELP_COMMAND_KILL_REACTOR)	\

enum {
	DXX_NETHELP_MENU(ENUM)
};

void show_netgame_help()
{
	const unsigned nitems = DXX_NETHELP_MENU(COUNT);
	auto m = new newmenu_item[nitems];
	DXX_NETHELP_MENU(ADD);
	newmenu_dotiny(NULL, TXT_KEYS, nitems, m, 0, free_help, m);
}

#undef DXX_NETHELP_MENU

#define DXX_NEWDEMO_HELP_MENU(VERB)	\
	DXX_MENUITEM(VERB, TEXT, "ESC\t  QUIT DEMO PLAYBACK", DEMOHELP_QUIT)	\
	DXX_MENUITEM(VERB, TEXT, "F1\t  THIS SCREEN", DEMOHELP_HELP)	\
	DXX_MENUITEM(VERB, TEXT, TXT_HELP_F2, DEMOHELP_F2)	\
	DXX_MENUITEM(VERB, TEXT, "F3\t  SWITCH COCKPIT MODES", DEMOHELP_F3)	\
	DXX_MENUITEM(VERB, TEXT, "F4\t  TOGGLE PERCENTAGE DISPLAY", DEMOHELP_F4)	\
	DXX_MENUITEM(VERB, TEXT, "UP\t  PLAY", DEMOHELP_PLAY)	\
	DXX_MENUITEM(VERB, TEXT, "DOWN\t  PAUSE", DEMOHELP_PAUSE)	\
	DXX_MENUITEM(VERB, TEXT, "RIGHT\t  ONE FRAME FORWARD", DEMOHELP_FRAME_FORWARD)	\
	DXX_MENUITEM(VERB, TEXT, "LEFT\t  ONE FRAME BACKWARD", DEMOHELP_FRAME_BACKWARD)	\
	DXX_MENUITEM(VERB, TEXT, "SHIFT-RIGHT\t  FAST FORWARD", DEMOHELP_FAST_FORWARD)	\
	DXX_MENUITEM(VERB, TEXT, "SHIFT-LEFT\t  FAST BACKWARD", DEMOHELP_FAST_BACKWARD)	\
	DXX_MENUITEM(VERB, TEXT, "CTRL-RIGHT\t  JUMP TO END", DEMOHELP_JUMP_END)	\
	DXX_MENUITEM(VERB, TEXT, "CTRL-LEFT\t  JUMP TO START", DEMOHELP_JUMP_START)	\
	_DXX_HELP_MENU_HINT_CMD_KEY(VERB, DEMOHELP)	\

enum {
	DXX_NEWDEMO_HELP_MENU(ENUM)
};

void show_newdemo_help()
{
	const unsigned nitems = DXX_NEWDEMO_HELP_MENU(COUNT);
	auto m = new newmenu_item[nitems];
	DXX_NEWDEMO_HELP_MENU(ADD);
	newmenu_dotiny(NULL, "DEMO PLAYBACK CONTROLS", nitems, m, 0, free_help, m);
}

#undef DXX_NEWDEMO_HELP_MENU

#define LEAVE_TIME 0x4000		//how long until we decide key is down	(Used to be 0x4000)

enum class leave_type : uint_fast8_t
{
	none,
	maybe_on_release,
	wait_for_release,
	on_press,
};

static leave_type leave_mode;

static void end_rear_view()
{
	Rear_view = 0;
	if (PlayerCfg.CockpitMode[1] == CM_REAR_VIEW)
		select_cockpit(PlayerCfg.CockpitMode[0]);
	if (Newdemo_state == ND_STATE_RECORDING)
		newdemo_record_restore_rearview();
}

static void check_end_rear_view()
{
	leave_mode = leave_type::none;
	if (Rear_view)
		end_rear_view();
}

//deal with rear view - switch it on, or off, or whatever
void check_rear_view()
{
	static fix64 entry_time;

	if (Newdemo_state == ND_STATE_PLAYBACK)
		return;

	const auto rear_view = Controls.state.rear_view;
	switch (leave_mode)
	{
		case leave_type::none:
			if (!rear_view)
				return;
			if (Rear_view)
				end_rear_view();
			else
			{
				Rear_view = 1;
				leave_mode = leave_type::maybe_on_release;		//means wait for another key
				entry_time = timer_query();
				if (PlayerCfg.CockpitMode[1] == CM_FULL_COCKPIT)
					select_cockpit(CM_REAR_VIEW);
				if (Newdemo_state == ND_STATE_RECORDING)
					newdemo_record_rearview();
			}
			return;
		case leave_type::maybe_on_release:
			if (rear_view)
			{
				if (timer_query() - entry_time > LEAVE_TIME)
					leave_mode = leave_type::wait_for_release;
			}
			else
				leave_mode = leave_type::on_press;
			return;
		case leave_type::wait_for_release:
			if (!rear_view)
				check_end_rear_view();
			return;
		case leave_type::on_press:
			if (rear_view)
			{
				Controls.state.rear_view = 0;
				check_end_rear_view();
			}
			return;
		default:
			break;
	}
}

void reset_rear_view(void)
{
	if (Rear_view) {
		if (Newdemo_state == ND_STATE_RECORDING)
			newdemo_record_restore_rearview();
	}

	Rear_view = 0;
	select_cockpit(PlayerCfg.CockpitMode[0]);
}

int cheats_enabled()
{
	return cheats.enabled;
}

//turns off all cheats & resets cheater flag
void game_disable_cheats()
{
#if defined(DXX_BUILD_DESCENT_II)
	if (cheats.homingfire)
		weapons_homing_all_reset();
#endif

	cheats = {};
}

//	game_setup()
// ----------------------------------------------------------------------------

namespace dsx {

window *game_setup(void)
{

	PlayerCfg.CockpitMode[1] = PlayerCfg.CockpitMode[0];
	last_drawn_cockpit = -1;	// Force cockpit to redraw next time a frame renders.
	Endlevel_sequence = 0;

	const auto game_wind = window_create(grd_curscreen->sc_canvas, 0, 0, SWIDTH, SHEIGHT, game_handler, unused_window_userdata);
	if (!game_wind)
		return NULL;

	reset_palette_add();
	init_cockpit();
	init_gauges();
	netplayerinfo_on = 0;

#if DXX_USE_EDITOR
	if (!Cursegp)
	{
		Cursegp = imsegptridx(segment_first);
		Curside = 0;
	}
	
	if (vcsegptr(ConsoleObject->segnum)->segnum == segment_none)      //segment no longer exists
		obj_relink(vmobjptr, vmsegptr, vmobjptridx(ConsoleObject), Cursegp);

	if (!check_obj_seg(vcvertptr, ConsoleObject))
		move_player_2_segment(Cursegp,Curside);
#endif

	Viewer = ConsoleObject;
	fly_init(*ConsoleObject);
	Game_suspended = 0;
	reset_time();
	FrameTime = 0;			//make first frame zero

	fix_object_segs();
	if (CGameArg.SysAutoRecordDemo && Newdemo_state == ND_STATE_NORMAL)
		newdemo_start_recording();
	return game_wind;
}

}

window *Game_wind = NULL;

namespace dsx {

// Event handler for the game
window_event_result game_handler(window *,const d_event &event, const unused_window_userdata_t *)
{
	auto result = window_event_result::ignored;

	switch (event.type)
	{
		case EVENT_WINDOW_ACTIVATED:
			set_screen_mode(SCREEN_GAME);

			event_toggle_focus(1);
			key_toggle_repeat(0);
			game_flush_inputs();

			if (time_paused)
				start_time();

			if (!((Game_mode & GM_MULTI) && (Newdemo_state != ND_STATE_PLAYBACK)))
				digi_resume_digi_sounds();

			if (!((Game_mode & GM_MULTI) && (Newdemo_state != ND_STATE_PLAYBACK)))
				palette_restore();
			
			reset_cockpit();
			break;

		case EVENT_WINDOW_DEACTIVATED:
			if (!(((Game_mode & GM_MULTI) && (Newdemo_state != ND_STATE_PLAYBACK)) && (!Endlevel_sequence)) )
				stop_time();

			if (!((Game_mode & GM_MULTI) && (Newdemo_state != ND_STATE_PLAYBACK)))
				digi_pause_digi_sounds();

			if (!((Game_mode & GM_MULTI) && (Newdemo_state != ND_STATE_PLAYBACK)))
				full_palette_save();

			event_toggle_focus(0);
			key_toggle_repeat(1);
			break;

		case EVENT_JOYSTICK_BUTTON_UP:
		case EVENT_JOYSTICK_BUTTON_DOWN:
		case EVENT_JOYSTICK_MOVED:
		case EVENT_MOUSE_BUTTON_UP:
		case EVENT_MOUSE_BUTTON_DOWN:
		case EVENT_MOUSE_MOVED:
		case EVENT_KEY_COMMAND:
		case EVENT_KEY_RELEASE:
		case EVENT_IDLE:
			return ReadControls(event);

		case EVENT_WINDOW_DRAW:
			if (!time_paused)
			{
				calc_frame_time();
				result = GameProcessFrame();
			}

			if (!Automap_active)		// efficiency hack
			{
				if (force_cockpit_redraw) {			//screen need redrawing?
					init_cockpit();
					force_cockpit_redraw=0;
				}
				game_render_frame();
			}
			break;

		case EVENT_WINDOW_CLOSE:
			digi_stop_digi_sounds();

			if ( (Newdemo_state == ND_STATE_RECORDING) || (Newdemo_state == ND_STATE_PAUSED) )
				newdemo_stop_recording();

			multi_leave_game();

			if ( Newdemo_state == ND_STATE_PLAYBACK )
				newdemo_stop_playback();

			songs_play_song( SONG_TITLE, 1 );

			game_disable_cheats();
			Game_mode = GM_GAME_OVER;
#if DXX_USE_EDITOR
			if (!EditorWindow)		// have to do it this way because of the necessary longjmp. Yuck.
#endif
				show_menus();
			event_toggle_focus(0);
			key_toggle_repeat(1);
			Game_wind = nullptr;
			return window_event_result::ignored;
			break;

		default:
			break;
	}

	return result;
}

// Initialise game, actually runs in main event loop
void game()
{
	hide_menus();
	Game_wind = game_setup();
}

}

//called at the end of the program
void close_game()
{
	close_gauges();
	restore_effect_bitmap_icons();
}

#if defined(DXX_BUILD_DESCENT_II)
namespace dsx {
object *Missile_viewer=NULL;
object_signature_t Missile_viewer_sig;

array<unsigned, 2> Marker_viewer_num{{~0u, ~0u}};
array<unsigned, 2> Coop_view_player{{UINT_MAX, UINT_MAX}};

//returns ptr to escort robot, or NULL
imobjptridx_t find_escort()
{
	range_for (const auto &&o, vmobjptridx)
	{
		if (o->type == OBJ_ROBOT && Robot_info[get_robot_id(o)].companion)
			return imobjptridx_t(o);
	}
	return object_none;
}

//if water or fire level, make occasional sound
static void do_ambient_sounds()
{
	int has_water,has_lava;
	int sound;

	const auto s2_flags = vcsegptr(ConsoleObject->segnum)->s2_flags;
	has_lava = (s2_flags & S2F_AMBIENT_LAVA);
	has_water = (s2_flags & S2F_AMBIENT_WATER);

	if (has_lava) {							//has lava
		sound = SOUND_AMBIENT_LAVA;
		if (has_water && (d_rand() & 1))	//both, pick one
			sound = SOUND_AMBIENT_WATER;
	}
	else if (has_water)						//just water
		sound = SOUND_AMBIENT_WATER;
	else
		return;

	if (((d_rand() << 3) < FrameTime)) {						//play the sound
		fix volume = d_rand() + f1_0/2;
		digi_play_sample(sound,volume);
	}
}
}
#endif

void game_leave_menus(void)
{
	if (!Game_wind)
		return;
	for (;;) // go through all windows and actually close them if they want to
	{
		const auto wind = window_get_front();
		if (!wind)
			break;
		if (wind == Game_wind)
			break;
		if (!window_close(wind))
			break;
	}
}

namespace dsx {

window_event_result GameProcessFrame()
{
	auto &plrobj = get_local_plrobj();
	auto &player_info = plrobj.ctype.player_info;
	auto &local_player_shields_ref = plrobj.shields;
	fix player_shields = local_player_shields_ref;
	const auto player_was_dead = Player_dead_state;
	auto result = window_event_result::ignored;

	update_player_stats();
	diminish_palette_towards_normal();		//	Should leave palette effect up for as long as possible by putting right before render.
	do_afterburner_stuff(Objects);
	do_cloak_stuff();
	do_invulnerable_stuff(player_info);
#if defined(DXX_BUILD_DESCENT_II)
	init_ai_frame(player_info.powerup_flags);
	result = do_final_boss_frame();

	auto &pl_flags = player_info.powerup_flags;
	if (pl_flags & PLAYER_FLAGS_HEADLIGHT_ON)
	{
		static int turned_off=0;
		auto &energy = player_info.energy;
		energy -= (FrameTime*3/8);
		if (energy < i2f(10)) {
			if (!turned_off) {
				pl_flags &= ~PLAYER_FLAGS_HEADLIGHT_ON;
				turned_off = 1;
				if (Game_mode & GM_MULTI)
					multi_send_flags(Player_num);
			}
		}
		else
			turned_off = 0;

		if (energy <= 0)
		{
			energy = 0;
			pl_flags &= ~PLAYER_FLAGS_HEADLIGHT_ON;
			if (Game_mode & GM_MULTI)
				multi_send_flags(Player_num);
		}
	}
#endif

#if DXX_USE_EDITOR
	check_create_player_path();
	player_follow_path(vmobjptr(ConsoleObject));
#endif

	if (Game_mode & GM_MULTI)
	{
		result = std::max(multi_do_frame(), result);
		if (Netgame.PlayTimeAllowed && ThisLevelTime>=i2f((Netgame.PlayTimeAllowed*5*60)))
			multi_check_for_killgoal_winner();
	}

	result = std::max(dead_player_frame(), result);
	if (Newdemo_state != ND_STATE_PLAYBACK)
		result = std::max(do_controlcen_dead_frame(), result);
	if (result == window_event_result::close)
		return result;	// skip everything else - don't set Player_dead_state again

#if defined(DXX_BUILD_DESCENT_II)
	process_super_mines_frame();
	do_seismic_stuff();
	do_ambient_sounds();
#endif

	if ((Game_mode & GM_MULTI) && Netgame.PlayTimeAllowed)
		ThisLevelTime +=FrameTime;

	digi_sync_sounds();

	if (Endlevel_sequence) {
		result = std::max(do_endlevel_frame(), result);
		powerup_grab_cheat_all();
		do_special_effects();
		return result;					//skip everything else
	}

	if ((Newdemo_state != ND_STATE_PLAYBACK) || (Newdemo_vcr_state != ND_STATE_PAUSED)) {
		do_special_effects();
		wall_frame_process();
	}

	if (Control_center_destroyed) {
		if (Newdemo_state==ND_STATE_RECORDING )
			newdemo_record_control_center_destroyed();
	}

	flash_frame();

	if ( Newdemo_state == ND_STATE_PLAYBACK )
	{
		result = std::max(newdemo_playback_one_frame(), result);
		if ( Newdemo_state != ND_STATE_PLAYBACK )
		{
			Assert(result == window_event_result::close);
			return window_event_result::close;	// Go back to menu
		}
	}
	else
	{ // Note the link to above!
#ifndef NEWHOMER
		player_info.homing_object_dist = -1; // Assume not being tracked.  Laser_do_weapon_sequence modifies this.
#endif
		result = std::max(object_move_all(), result);
		powerup_grab_cheat_all();

		if (Endlevel_sequence)	//might have been started during move
			return result;

		fuelcen_update_all();

		do_ai_frame_all();

		auto laser_firing_count = FireLaser(player_info);
		if (auto &Auto_fire_fusion_cannon_time = player_info.Auto_fire_fusion_cannon_time)
		{
			if (player_info.Primary_weapon != primary_weapon_index_t::FUSION_INDEX)
				Auto_fire_fusion_cannon_time = 0;
			else if ((laser_firing_count = (GameTime64 + FrameTime/2 >= Auto_fire_fusion_cannon_time)))
			{
				Auto_fire_fusion_cannon_time = 0;
			} else if (d_tick_step) {
				const auto rx = (d_rand() - 16384) / 8;
				const auto rz = (d_rand() - 16384) / 8;
				const auto &&console = vmobjptr(ConsoleObject);
				auto &rotvel = console->mtype.phys_info.rotvel;
				rotvel.x += rx;
				rotvel.z += rz;

				const auto bump_amount = player_info.Fusion_charge > F1_0*2 ? player_info.Fusion_charge * 4 : F1_0 * 4;
				bump_one_object(console, make_random_vector(), bump_amount);
			}
		}

		if (laser_firing_count)
			do_laser_firing_player(plrobj);
		delayed_autoselect(player_info);
	}

	if (Do_appearance_effect) {
		Do_appearance_effect = 0;
		create_player_appearance_effect(*ConsoleObject);
	}

#if defined(DXX_BUILD_DESCENT_II)
	omega_charge_frame(player_info);
	slide_textures();
	flicker_lights(Flickering_light_state, vmsegptridx);

	//if the player is taking damage, give up guided missile control
	if (local_player_shields_ref != player_shields)
		release_guided_missile(Player_num);
#endif

	// Check if we have to close in-game menus for multiplayer
	if ((Game_mode & GM_MULTI) && (get_local_player().connected == CONNECT_PLAYING))
	{
		if (Endlevel_sequence || (Player_dead_state != player_was_dead) || (local_player_shields_ref < player_shields) || (Control_center_destroyed && Countdown_seconds_left < 10))
                        game_leave_menus();
	}

	return result;
}

#if defined(DXX_BUILD_DESCENT_II)
void compute_slide_segs()
{
	range_for (const auto &&segp, vmsegptr)
	{
		uint8_t slide_textures = 0;
		for (int sidenum=0;sidenum<6;sidenum++) {
			const auto &side = segp->sides[sidenum];
			const auto &ti = TmapInfo[side.tmap_num];
			if (!(ti.slide_u || ti.slide_v))
				continue;
			if (IS_CHILD(segp->children[sidenum]) && side.wall_num == wall_none)
				/* If a wall exists, it could be visible at start or
				 * become visible later, so always enable sliding for
				 * walls.
				 */
				continue;
			slide_textures |= 1 << sidenum;
		}
		segp->slide_textures = slide_textures;
	}
}

template <fix uvl::*p>
static void update_uv(array<uvl, 4> &uvls, uvl &i, fix a)
{
	if (!a)
		return;
	const auto ip = (i.*p += a);
	if (ip > f2_0)
		range_for (auto &j, uvls)
			j.*p -= f1_0;
	else if (ip < -f2_0)
		range_for (auto &j, uvls)
			j.*p += f1_0;
}

//	-----------------------------------------------------------------------------
static void slide_textures(void)
{
	range_for (const auto &&segp, vmsegptr)
	{
		if (const auto slide_seg = segp->slide_textures)
		{
			for (int sidenum=0;sidenum<6;sidenum++) {
				if (slide_seg & (1 << sidenum))
				{
					auto &side = segp->sides[sidenum];
					const auto &ti = TmapInfo[side.tmap_num];
					const auto tiu = ti.slide_u;
					const auto tiv = ti.slide_v;
					if (tiu || tiv)
					{
						const auto frametime = FrameTime;
						const auto ua = fixmul(frametime, tiu << 8);
						const auto va = fixmul(frametime, tiv << 8);
						auto &uvls = side.uvls;
						range_for (auto &i, uvls)
						{
							update_uv<&uvl::u>(uvls, i, ua);
							update_uv<&uvl::v>(uvls, i, va);
						}
					}
				}
			}
		}
	}
}

constexpr std::integral_constant<fix, INT32_MIN> flicker_timer_disabled{};

static void flicker_lights(d_flickering_light_state &fls, fvmsegptridx &vmsegptridx)
{
	range_for (auto &f, partial_range(fls.Flickering_lights, fls.Num_flickering_lights))
	{
		if (f.timer == flicker_timer_disabled)		//disabled
			continue;
		const auto &&segp = vmsegptridx(f.segnum);
		const auto sidenum = f.sidenum;
		{
			auto &side = segp->sides[sidenum];
			if (!(TmapInfo[side.tmap_num].lighting || TmapInfo[side.tmap_num2 & 0x3fff].lighting))
				continue;
		}

		//make sure this is actually a light
		if (! (WALL_IS_DOORWAY(GameBitmaps, Textures, vcwallptr, segp, segp, sidenum) & WID_RENDER_FLAG))
			continue;

		if ((f.timer -= FrameTime) < 0)
		{
			while (f.timer < 0)
				f.timer += f.delay;
			f.mask = ((f.mask & 0x80000000) ? 1 : 0) + (f.mask << 1);
			if (f.mask & 1)
				add_light(segp, sidenum);
			else
				subtract_light(segp, sidenum);
		}
	}
}

//returns ptr to flickering light structure, or NULL if can't find
static std::pair<d_flickering_light_state::Flickering_light_array_t::iterator, d_flickering_light_state::Flickering_light_array_t::iterator> find_flicker(d_flickering_light_state &fls, const vmsegidx_t segnum, const unsigned sidenum)
{
	//see if there's already an entry for this seg/side
	const auto &&pr = partial_range(fls.Flickering_lights, fls.Num_flickering_lights);
	const auto &&predicate = [segnum, sidenum](const flickering_light &f) {
		return f.segnum == segnum && f.sidenum == sidenum;	//found it!
	};
	const auto &&pe = pr.end();
	return {std::find_if(pr.begin(), pe, predicate), pe};
}

static void update_flicker(d_flickering_light_state &fls, const vmsegidx_t segnum, const unsigned sidenum, const fix timer)
{
	const auto &&i = find_flicker(fls, segnum, sidenum);
	if (i.first != i.second)
		i.first->timer = timer;
}

//turn flickering off (because light has been turned off)
void disable_flicker(d_flickering_light_state &fls, const vmsegidx_t segnum, const unsigned sidenum)
{
	update_flicker(fls, segnum, sidenum, flicker_timer_disabled);
}

//turn flickering off (because light has been turned on)
void enable_flicker(d_flickering_light_state &fls, const vmsegidx_t segnum, const unsigned sidenum)
{
	update_flicker(fls, segnum, sidenum, 0);
}
#endif

//	-----------------------------------------------------------------------------
//	Fire Laser:  Registers a laser fire, and performs special stuff for the fusion
//				    cannon.
bool FireLaser(player_info &player_info)
{
	if (!Controls.state.fire_primary)
		return false;
	if (!allowed_to_fire_laser(player_info))
		return false;
	auto &Primary_weapon = player_info.Primary_weapon;
	if (!Weapon_info[Primary_weapon_to_weapon_info[Primary_weapon]].fire_count)
		/* Retail data sets fire_count=1 for all primary weapons */
		return false;

	if (Primary_weapon == primary_weapon_index_t::FUSION_INDEX)
	{
		auto &energy = player_info.energy;
		auto &Auto_fire_fusion_cannon_time = player_info.Auto_fire_fusion_cannon_time;
		if (energy < F1_0 * 2 && Auto_fire_fusion_cannon_time == 0)
		{
			return false;
		} else {
			static fix64 Fusion_next_sound_time = 0;

			if (player_info.Fusion_charge == 0)
				energy -= F1_0*2;

			const auto Fusion_charge = (player_info.Fusion_charge += FrameTime);
			energy -= FrameTime;

			if (energy <= 0)
			{
				energy = 0;
				Auto_fire_fusion_cannon_time = GameTime64 -1;	//	Fire now!
			} else
				Auto_fire_fusion_cannon_time = GameTime64 + FrameTime/2 + 1;		//	Fire the fusion cannon at this time in the future.

			{
				int dg, db;
				const int dr = Fusion_charge >> 11;
				if (Fusion_charge < F1_0*2)
					dg = 0, db = dr;
				else
					dg = dr, db = 0;
				PALETTE_FLASH_ADD(dr, dg, db);
			}

			if (Fusion_next_sound_time > GameTime64 + F1_0/8 + D_RAND_MAX/4) // GameTime64 is smaller than max delay - player in new level?
				Fusion_next_sound_time = GameTime64 - 1;

			if (Fusion_next_sound_time < GameTime64) {
				if (Fusion_charge > F1_0*2) {
					digi_play_sample( 11, F1_0 );
#if defined(DXX_BUILD_DESCENT_I)
					if(Game_mode & GM_MULTI)
						multi_send_play_sound(11, F1_0, sound_stack::allow_stacking);
#endif
					const auto cobjp = vmobjptridx(ConsoleObject);
					apply_damage_to_player(cobjp, cobjp, d_rand() * 4, 0);
				} else {
					create_awareness_event(vmobjptr(ConsoleObject), player_awareness_type_t::PA_WEAPON_ROBOT_COLLISION);
					multi_digi_play_sample(SOUND_FUSION_WARMUP, F1_0);
				}
				Fusion_next_sound_time = GameTime64 + F1_0/8 + d_rand()/4;
			}
		}
	}
	return true;
}


//	-------------------------------------------------------------------------------------------------------
//	If player is close enough to objnum, which ought to be a powerup, pick it up!
//	This could easily be made difficulty level dependent.
static void powerup_grab_cheat(object &player, const vmobjptridx_t powerup)
{
	fix	powerup_size;
	fix	player_size;

	Assert(powerup->type == OBJ_POWERUP);

	powerup_size = powerup->size;
	player_size = player.size;

	const auto dist = vm_vec_dist_quick(powerup->pos, player.pos);

	if ((dist < 2*(powerup_size + player_size)) && !(powerup->flags & OF_SHOULD_BE_DEAD)) {
		collide_live_local_player_and_powerup(powerup);
	}
}

//	-------------------------------------------------------------------------------------------------------
//	Make it easier to pick up powerups.
//	For all powerups in this segment, pick them up at up to twice pickuppable distance based on dot product
//	from player to powerup and player's forward vector.
//	This has the effect of picking them up more easily left/right and up/down, but not making them disappear
//	way before the player gets there.
void powerup_grab_cheat_all(void)
{
	if (Endlevel_sequence)
		return;
	if (Player_dead_state != player_dead_state::no)
		return;
	const auto &&console = vmobjptr(ConsoleObject);
	range_for (const auto objnum, objects_in(vmsegptr(console->segnum), vmobjptridx, vmsegptr))
		if (objnum->type == OBJ_POWERUP)
			powerup_grab_cheat(console, objnum);
}

}

int	Last_level_path_created = -1;

#ifdef SHOW_EXIT_PATH

//	------------------------------------------------------------------------------------------------------------------
//	Create path for player from current segment to goal segment.
//	Return true if path created, else return false.
static int mark_player_path_to_segment(fvmobjptridx &vmobjptridx, fvmsegptridx &vmsegptridx, segnum_t segnum)
{
	int		player_hide_index=-1;

	if (Last_level_path_created == Current_level_num) {
		return 0;
	}

	Last_level_path_created = Current_level_num;

	auto objp = vmobjptridx(ConsoleObject);
	const auto &&cr = create_path_points(objp, objp->segnum, segnum, Point_segs_free_ptr, 100, create_path_random_flag::nonrandom, create_path_safety_flag::unsafe, segment_none);
	const unsigned player_path_length = cr.second;
	if (cr.first == create_path_result::early)
		return 0;

	player_hide_index = Point_segs_free_ptr - Point_segs;
	Point_segs_free_ptr += player_path_length;

	if (Point_segs_free_ptr - Point_segs + MAX_PATH_LENGTH*2 > MAX_POINT_SEGS) {
		ai_reset_all_paths();
		return 0;
	}

	for (int i=1; i<player_path_length; i++) {
		vms_vector	seg_center;

		seg_center = Point_segs[player_hide_index+i].point;

		const auto &&obj = obj_create(OBJ_POWERUP, POW_ENERGY, vmsegptridx(Point_segs[player_hide_index+i].segnum), seg_center, &vmd_identity_matrix, Powerup_info[POW_ENERGY].size, CT_POWERUP, MT_NONE, RT_POWERUP);
		if (obj == object_none) {
			Int3();		//	Unable to drop energy powerup for path
			return 1;
		}

		obj->rtype.vclip_info.vclip_num = Powerup_info[get_powerup_id(obj)].vclip_num;
		obj->rtype.vclip_info.frametime = Vclip[obj->rtype.vclip_info.vclip_num].frame_time;
		obj->rtype.vclip_info.framenum = 0;
		obj->lifeleft = F1_0*100 + d_rand() * 4;
	}

	return 1;
}

//	Return true if it happened, else return false.
int create_special_path(void)
{
	//	---------- Find exit doors ----------
	range_for (const auto &&segp, vcsegptridx)
	{
		for (int j=0; j<MAX_SIDES_PER_SEGMENT; j++)
			if (segp->children[j] == segment_exit)
			{
				return mark_player_path_to_segment(vmobjptridx, vmsegptridx, segp);
			}
	}

	return 0;
}

#endif


#if defined(DXX_BUILD_DESCENT_II)
namespace dsx {
/*
 * reads a flickering_light structure from a PHYSFS_File
 */
void flickering_light_read(flickering_light &fl, PHYSFS_File *fp)
{
	fl.segnum = PHYSFSX_readShort(fp);
	fl.sidenum = PHYSFSX_readShort(fp);
	fl.mask = PHYSFSX_readInt(fp);
	fl.timer = PHYSFSX_readFix(fp);
	fl.delay = PHYSFSX_readFix(fp);
}

void flickering_light_write(const flickering_light &fl, PHYSFS_File *fp)
{
	PHYSFS_writeSLE16(fp, fl.segnum);
	PHYSFS_writeSLE16(fp, fl.sidenum);
	PHYSFS_writeULE32(fp, fl.mask);
	PHYSFSX_writeFix(fp, fl.timer);
	PHYSFSX_writeFix(fp, fl.delay);
}
}
#endif
