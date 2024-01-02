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
 * Definitions for graphics lib.
 *
 */

#pragma once

#include <cassert>
#include "fwd-gr.h"
#include "pstypes.h"
#include "maths.h"
#include "palette.h"
#include "dxxsconf.h"
#include "dsx-ns.h"
#include "pack.h"
#include <array>

#if DXX_USE_SDLIMAGE || !DXX_USE_OGL
#include <memory>
#include <SDL_video.h>
#endif

namespace dcx {

enum class gr_fade_level : uint8_t
{
	off = GR_FADE_LEVELS // yes, max means OFF - don't screw that up
};

struct grs_point
{
	fix x,y;
};

struct grs_bitmap : prohibit_void_ptr<grs_bitmap>
{
	uint16_t bm_x,bm_y; // Offset from parent's origin
	uint16_t bm_w,bm_h; // width,height
private:
	bm_mode bm_type;
	uint8_t bm_flags;
public:
	bm_mode get_type() const
	{
		return bm_type;
	}
	void set_type(bm_mode t)
	{
		bm_type = t;
	}
	uint8_t get_flags() const
	{
		return bm_flags;
	}
	uint8_t get_flag_mask(const uint8_t mask) const
	{
		return get_flags() & mask;
	}
	void set_flags(const uint8_t f)
	{
		bm_flags = f;
	}
	void clear_flags()
	{
		set_flags(0);
	}
	void set_flag_mask(const bool set, const uint8_t mask)
	{
		const auto f = get_flags();
		set_flags(set ? f | mask : f & ~mask);
	}
	void add_flags(const uint8_t f)
	{
		bm_flags |= f;
	}
	                    // bit 1 on means it has supertransparency
	                    // bit 2 on means it doesn't get passed through lighting.
	short   bm_rowsize; // unsigned char offset to next row
	std::array<fix, 3> avg_color_rgb; // same as above but real rgb value to be used to textured objects that should emit light
	union {
		const color_palette_index *bm_data{};     // ptr to pixel data...
	                                //   Linear = *parent+(rowsize*y+x)
	                                //   ModeX = *parent+(rowsize*y+x/4)
	                                //   SVGA = *parent+(rowsize*y+x)
		color_palette_index *bm_mdata;
	};
	const color_palette_index *get_bitmap_data() const { return bm_data; }
	color_palette_index *get_bitmap_data() { return bm_mdata; }
	struct grs_bitmap  *bm_parent{};
#if DXX_USE_OGL
	struct ogl_texture *gltexture{};
#else
	uint8_t avg_color;  //  Average color of all pixels in texture map.
#endif /* def OGL */
};

// Free the bitmap and its pixel data
class grs_main_bitmap : public grs_bitmap
{
public:
	grs_main_bitmap();
	grs_main_bitmap(const grs_main_bitmap &) = delete;
	grs_main_bitmap &operator=(const grs_main_bitmap &) = delete;
	grs_main_bitmap(grs_main_bitmap &&r) :
		grs_bitmap{std::move(static_cast<grs_bitmap &>(r))}
	{
		r.bm_data = nullptr;
#if DXX_USE_OGL
		r.gltexture = nullptr;
#endif
	}
	grs_main_bitmap &operator=(grs_main_bitmap &&r)
	{
		if (grs_bitmap *const that = &r; this == that)
			return *this;
		reset();
		grs_bitmap::operator=(std::move(static_cast<grs_bitmap &>(r)));
		r.bm_data = nullptr;
#if DXX_USE_OGL
		r.gltexture = nullptr;
#endif
		return *this;
	}
	~grs_main_bitmap()
	{
		reset();
	}
	void reset()
	{
		gr_free_bitmap_data(*this);
	}
};

struct grs_canvas : prohibit_void_ptr<grs_canvas>
{
	grs_canvas(const grs_canvas &) = delete;
	grs_canvas &operator=(const grs_canvas &) = delete;
	grs_canvas(grs_canvas &&) = default;
	grs_canvas &operator=(grs_canvas &&) = default;
	~grs_canvas()
	{
		/* `grd_curcanv` points to the currently active canvas.  If it
		 * points to `this` when the destructor runs, then any further
		 * attempts to use the referenced canvas will access an
		 * out-of-scope variable, leading to undefined behavior.  Assert
		 * that `grd_curcanv` does not point to the expiring variable so
		 * that potential misuses are detected before they manifest as
		 * undefined behavior.
		 *
		 * If you get an assertion failure here, enable
		 * `DXX_DEBUG_CURRENT_CANVAS_ORIGIN` to trace where the canvas
		 * was most recently set.
		 *
		 * Eventually, `grd_curcanv` will be removed and this test will
		 * become obsolete.
		 */
		assert(this != grd_curcanv);
		/* If the canvas is reset before next use, then no crash.
		 * If the canvas is not reset, then crash instead of accessing
		 * freed memory.
		 */
		if (grd_curcanv == this)
			grd_curcanv = nullptr;
	}
	grs_bitmap  cv_bitmap;      // the bitmap for this canvas
	const grs_font *  cv_font;        // the currently selected font
	color_palette_index cv_font_fg_color;   // current font foreground color (255==Invisible)
	color_palette_index cv_font_bg_color;   // current font background color (255==Invisible)
	gr_fade_level cv_fade_level;  // transparency level
protected:
	grs_canvas() = default;
};

//=========================================================================
// System functions:
// setup and set mode. this creates a grs_screen structure and sets
// grd_curscreen to point to it.  grs_curcanv points to this screen's
// canvas.  Saves the current VGA state and screen mode.

union screen_mode
{
private:
	uint32_t wh;
public:
	struct {
		uint16_t width, height;
	};
	constexpr bool operator==(const screen_mode &rhs) const
	{
		return wh == rhs.wh;
	}
	screen_mode() = default;
	constexpr screen_mode(uint16_t &&w, uint16_t &&h) :
		width{w}, height{h}
	{
	}
};

static inline uint16_t SM_W(const screen_mode &s)
{
	return s.width;
}

static inline uint16_t SM_H(const screen_mode &s)
{
	return s.height;
}

// Makes a new canvas. allocates memory for the canvas and its bitmap,
// including the raw pixel buffer.

struct grs_main_canvas : grs_canvas, prohibit_void_ptr<grs_main_canvas>
{
	using prohibit_void_ptr<grs_main_canvas>::operator &;
	grs_main_canvas &operator=(grs_main_canvas &) = delete;
	grs_main_canvas &operator=(grs_main_canvas &&) = default;
	~grs_main_canvas();
};

// Creates a canvas that is part of another canvas.  This can be used to make
// a window on the screen.  The address of the raw pixel data is inherited from
// the parent canvas.
struct grs_subcanvas : grs_canvas, prohibit_void_ptr<grs_subcanvas>
{
	using prohibit_void_ptr<grs_subcanvas>::operator &;
};

#if DXX_USE_SDLIMAGE || !DXX_USE_OGL
struct RAII_SDL_Surface
{
	struct deleter
	{
		void operator()(SDL_Surface *s) const
		{
			SDL_FreeSurface(s);
		}
	};
	std::unique_ptr<SDL_Surface, deleter> surface;
	constexpr RAII_SDL_Surface() = default;
	RAII_SDL_Surface(RAII_SDL_Surface &&) = default;
	RAII_SDL_Surface &operator=(RAII_SDL_Surface &&) = default;
	explicit RAII_SDL_Surface(SDL_Surface *const s) :
		surface{s}
	{
	}
};
#endif

class grs_screen : prohibit_void_ptr<grs_screen>
{    // This is a video screen
	screen_mode sc_mode;
public:
	grs_screen &operator=(grs_screen &) = delete;
	grs_screen &operator=(grs_screen &&) = default;
#if DXX_USE_OGL
	/* OpenGL builds allocate the backing data for the canvas, so use
	 * grs_main_canvas to ensure it is freed when appropriate.
	 */
	grs_main_canvas  sc_canvas;  // Represents the entire screen
#else
	/* SDL builds borrow the backing data from the SDL_Surface, so use
	 * grs_subcanvas for the canvas because the memory is owned by SDL and will
	 * be freed by SDL_FreeSurface.  Store the associated SDL_Surface alongside
	 * the canvas, so that it is freed at the same time.
	 */
	RAII_SDL_Surface sdl_surface;
	grs_subcanvas sc_canvas;
#endif
	fix     sc_aspect;      //aspect ratio (w/h) for this screen
	uint16_t get_screen_width() const
	{
		return SM_W(sc_mode);
	}
	uint16_t get_screen_height() const
	{
		return SM_H(sc_mode);
	}
	screen_mode get_screen_mode() const
	{
		return sc_mode;
	}
	void set_screen_width_height(uint16_t w, uint16_t h)
	{
		sc_mode.width = w;
		sc_mode.height = h;
	}
};

//=========================================================================
// Canvas functions:

// Free the bitmap, but not the pixel data buffer
class grs_subbitmap : public grs_bitmap
{
};

//font structure
struct grs_font : public prohibit_void_ptr<grs_font>
{
	uint16_t    ft_w;           // Width in pixels
	uint16_t    ft_h;           // Height in pixels
	int16_t     ft_flags;       // Proportional?
	int16_t     ft_baseline;    //
	uint8_t     ft_minchar;     // First char defined by this font
	uint8_t     ft_maxchar;     // Last char defined by this font
	std::array<char, 13> ft_filename{};
	const uint8_t *ft_data = nullptr;        // Ptr to raw data.
	const uint8_t *const *ft_chars = nullptr;       // Ptrs to data for each char (required for prop font)
	const uint16_t *ft_widths = nullptr;     // Array of widths (required for prop font)
	const uint8_t *ft_kerndata = nullptr;    // Array of kerning triplet data
	std::unique_ptr<uint8_t[]> ft_allocdata;
#if DXX_USE_OGL
	// These fields do not participate in disk i/o!
	std::unique_ptr<grs_bitmap[]> ft_bitmaps;
	grs_main_bitmap ft_parent_bitmap;
#endif /* def OGL */
};

}

static inline void gr_set_bitmap_flags(grs_bitmap &bm, uint8_t flags)
{
	bm.set_flags(flags);
}

static inline void gr_set_transparent(grs_bitmap &bm, bool bTransparent)
{
	bm.set_flag_mask(bTransparent, BM_FLAG_TRANSPARENT);
}

namespace dcx {

static inline void gr_set_font_fg_color(grs_canvas &canvas, color_palette_index fg_color)
{
	canvas.cv_font_fg_color = fg_color;
}

static inline void gr_set_font_bg_color(grs_canvas &canvas, color_palette_index bg_color)
{
	canvas.cv_font_bg_color = bg_color;
}

#define gr_set_fontcolor(C,F,B)	\
	( DXX_BEGIN_COMPOUND_STATEMENT {	\
		auto &gr_set_fontcolor = C;	\
		gr_set_font_fg_color(gr_set_fontcolor, F);	\
		gr_set_font_bg_color(gr_set_fontcolor, B);	\
		} DXX_END_COMPOUND_STATEMENT )

struct font_delete
{
	void operator()(grs_font *p) const
	{
		gr_close_font(std::unique_ptr<grs_font>(p));
	}
};
}

static inline void (gr_set_current_canvas)(grs_canvas_ptr &canv DXX_DEBUG_CURRENT_CANVAS_FILE_LINE_COMMA_L_DECL_VARS)
{
	(gr_set_current_canvas)(canv.get() DXX_DEBUG_CURRENT_CANVAS_FILE_LINE_COMMA_L_PASS_VARS);
}

static inline void (gr_set_current_canvas)(grs_subcanvas_ptr &canv DXX_DEBUG_CURRENT_CANVAS_FILE_LINE_COMMA_L_DECL_VARS)
{
	(gr_set_current_canvas)(canv.get() DXX_DEBUG_CURRENT_CANVAS_FILE_LINE_COMMA_L_PASS_VARS);
}
