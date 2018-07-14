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
#include "fmtcheck.h"
#include "pack.h"
#include "compiler-array.h"
#include "compiler-exchange.h"

struct grs_point
{
	fix x,y;
};

struct grs_bitmap : prohibit_void_ptr<grs_bitmap>
{
	uint16_t bm_x,bm_y; // Offset from parent's origin
	uint16_t bm_w,bm_h; // width,height
private:
	uint8_t bm_type;
	uint8_t bm_flags;
public:
	uint8_t get_type() const
	{
		return bm_type;
	}
	void set_type(uint8_t t)
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
	array<fix, 3> avg_color_rgb; // same as above but real rgb value to be used to textured objects that should emit light
	union {
		const uint8_t *bm_data;     // ptr to pixel data...
	                                //   Linear = *parent+(rowsize*y+x)
	                                //   ModeX = *parent+(rowsize*y+x/4)
	                                //   SVGA = *parent+(rowsize*y+x)
		uint8_t *bm_mdata;
	};
	const uint8_t *get_bitmap_data() const { return bm_data; }
	uint8_t *get_bitmap_data() { return bm_mdata; }
	struct grs_bitmap  *bm_parent;
#if DXX_USE_OGL
	struct ogl_texture *gltexture;
#endif /* def OGL */
	ubyte   avg_color;  //  Average color of all pixels in texture map.
};

//font structure
struct grs_font : public prohibit_void_ptr<grs_font>
{
	short       ft_w;           // Width in pixels
	short       ft_h;           // Height in pixels
	short       ft_flags;       // Proportional?
	short       ft_baseline;    //
	ubyte       ft_minchar;     // First char defined by this font
	ubyte       ft_maxchar;     // Last char defined by this font
	array<char, 13> ft_filename;
	const uint8_t *ft_data;        // Ptr to raw data.
	const uint8_t *const *ft_chars;       // Ptrs to data for each char (required for prop font)
	const int16_t *ft_widths;      // Array of widths (required for prop font)
	const uint8_t *ft_kerndata;    // Array of kerning triplet data
	std::unique_ptr<uint8_t[]> ft_allocdata;
#if DXX_USE_OGL
	// These fields do not participate in disk i/o!
	std::unique_ptr<grs_bitmap[]> ft_bitmaps;
	grs_bitmap ft_parent_bitmap;
#endif /* def OGL */
};

struct grs_canvas : prohibit_void_ptr<grs_canvas>
{
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
	short       cv_font_fg_color;   // current font foreground color (-1==Invisible)
	short       cv_font_bg_color;   // current font background color (-1==Invisible)
	unsigned cv_fade_level;  // transparency level
};


//=========================================================================
// System functions:
// setup and set mode. this creates a grs_screen structure and sets
// grd_curscreen to point to it.  grs_curcanv points to this screen's
// canvas.  Saves the current VGA state and screen mode.

#ifdef __cplusplus
union screen_mode
{
private:
	uint32_t wh;
public:
	struct {
		uint16_t width, height;
	};
	bool operator==(const screen_mode &rhs)
	{
		return wh == rhs.wh;
	}
	bool operator!=(const screen_mode &rhs)
	{
		return !(*this == rhs);
	}
	screen_mode() = default;
	constexpr screen_mode(uint16_t &&w, uint16_t &&h) :
		width(w), height(h)
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

class grs_screen : prohibit_void_ptr<grs_screen>
{    // This is a video screen
	screen_mode sc_mode;
public:
	grs_canvas  sc_canvas;  // Represents the entire screen
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

// Makes a new canvas. allocates memory for the canvas and its bitmap,
// including the raw pixel buffer.

namespace dcx {

struct grs_main_canvas : grs_canvas
{
	~grs_main_canvas();
};

// Creates a canvas that is part of another canvas.  this can be used to make
// a window on the screen.  the canvas structure is malloc'd; the address of
// the raw pixel data is inherited from the parent canvas.

struct grs_subcanvas : grs_canvas {};

// Free the bitmap and its pixel data
class grs_main_bitmap : public grs_bitmap
{
public:
	grs_main_bitmap();
	grs_main_bitmap(const grs_main_bitmap &) = delete;
	grs_main_bitmap &operator=(const grs_main_bitmap &) = delete;
	grs_main_bitmap(grs_main_bitmap &&r) :
		grs_bitmap(std::move(static_cast<grs_bitmap &>(r)))
	{
		r.bm_data = nullptr;
#if DXX_USE_OGL
		r.gltexture = nullptr;
#endif
	}
	grs_main_bitmap &operator=(grs_main_bitmap &&r)
	{
		grs_bitmap::operator=(std::move(static_cast<grs_bitmap &>(r)));
		bm_data = exchange(r.bm_data, nullptr);
#if DXX_USE_OGL
		gltexture = exchange(r.gltexture, nullptr);
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

// Free the bitmap, but not the pixel data buffer
class grs_subbitmap : public grs_bitmap
{
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
struct font_delete
{
	void operator()(grs_font *p) const
	{
		gr_close_font(std::unique_ptr<grs_font>(p));
	}
};
}

// Writes a string using current font. Returns the next column after last char.
static inline void gr_set_curfont(grs_canvas &canvas, const grs_font_ptr &p)
{
	gr_set_curfont(canvas, p.get());
}

static inline void (gr_set_current_canvas)(grs_canvas_ptr &canv DXX_DEBUG_CURRENT_CANVAS_FILE_LINE_COMMA_L_DECL_VARS)
{
	(gr_set_current_canvas)(canv.get() DXX_DEBUG_CURRENT_CANVAS_FILE_LINE_COMMA_L_PASS_VARS);
}

static inline void (gr_set_current_canvas)(grs_subcanvas_ptr &canv DXX_DEBUG_CURRENT_CANVAS_FILE_LINE_COMMA_L_DECL_VARS)
{
	(gr_set_current_canvas)(canv.get() DXX_DEBUG_CURRENT_CANVAS_FILE_LINE_COMMA_L_PASS_VARS);
}

#endif
