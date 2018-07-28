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

#pragma once

#include <SDL_version.h>
#if SDL_MAJOR_VERSION == 2
#include <SDL_video.h>
#endif
#include <cstdint>
#include <memory>
#include <utility>
#include "palette.h"
#include "maths.h"

// some defines for transparency and blending
#define TRANSPARENCY_COLOR   255            // palette entry of transparency color -- 255 on the PC
#define GR_FADE_LEVELS       34u
#define GR_FADE_OFF          GR_FADE_LEVELS // yes, max means OFF - don't screw that up
enum gr_blend {
	GR_BLEND_NORMAL,		// normal blending
	GR_BLEND_ADDITIVE_A,	// additive alpha blending
	GR_BLEND_ADDITIVE_C,	// additive color blending
};

#define SWIDTH  (grd_curscreen->get_screen_width())
#define SHEIGHT (grd_curscreen->get_screen_height())

#if defined(DXX_BUILD_DESCENT_I)
namespace dsx {
extern int HiresGFXAvailable;
}
#define HIRESMODE HiresGFXAvailable		// descent.pig either contains hires or lowres graphics, not both
#elif defined(DXX_BUILD_DESCENT_II)
#define HIRESMODE (SWIDTH >= 640 && SHEIGHT >= 480 && !GameArg.GfxSkipHiresGFX)
#endif
#define MAX_BMP_SIZE(width, height) (4 + ((width) + 2) * (height))

#define SCRNS_DIR "screenshots/"

//these are control characters that have special meaning in the font code

#define CC_COLOR        1   //next char is new foreground color
#define CC_LSPACING     2   //next char specifies line spacing
#define CC_UNDERLINE    3   //next char is underlined

//now have string versions of these control characters (can concat inside a string)

#define CC_COLOR_S      "\x1"   //next char is new foreground color
#define CC_LSPACING_S   "\x2"   //next char specifies line spacing
#define CC_UNDERLINE_S  "\x3"   //next char is underlined

enum bm_mode : uint8_t
{
	linear,
	rgb15 = 3,	//5 bits each r,g,b stored at 16 bits
#if DXX_USE_OGL
	ogl = 5,
#endif /* def OGL */
};

#define BM_FLAG_TRANSPARENT         1
#define BM_FLAG_SUPER_TRANSPARENT   2
#define BM_FLAG_NO_LIGHTING         4
#define BM_FLAG_RLE                 8   // A run-length encoded bitmap.
#define BM_FLAG_PAGED_OUT           16  // This bitmap's data is paged out.
#define BM_FLAG_RLE_BIG             32  // for bitmaps that RLE to > 255 per row (i.e. cockpits)

#ifdef __cplusplus
#include "dxxsconf.h"
#include "dsx-ns.h"
#include "compiler-array.h"

struct grs_bitmap;
struct grs_canvas;
#define GRS_FONT_SIZE 28    // how much space it takes up on disk
struct grs_font;
struct grs_point;

union screen_mode;

class grs_screen;

namespace dcx {

struct grs_main_canvas;
typedef std::unique_ptr<grs_main_canvas> grs_canvas_ptr;

struct grs_subcanvas;
typedef std::unique_ptr<grs_subcanvas> grs_subcanvas_ptr;

// Free the bitmap and its pixel data
class grs_main_bitmap;
typedef std::unique_ptr<grs_main_bitmap> grs_bitmap_ptr;

#if SDL_MAJOR_VERSION == 1
uint_fast32_t gr_list_modes(array<screen_mode, 50> &modes);
#elif SDL_MAJOR_VERSION == 2
extern SDL_Window *g_pRebirthSDLMainWindow;
#endif

}

#ifdef dsx
namespace dsx {
int gr_set_mode(screen_mode mode);
void gr_set_mode_from_window_size();

int gr_init();
#if DXX_USE_OGL
void gr_set_attributes();
#endif
void gr_close();
}
#endif

namespace dcx {

grs_canvas_ptr gr_create_canvas(uint16_t w, uint16_t h);

grs_subcanvas_ptr gr_create_sub_canvas(grs_canvas &canv,uint16_t x,uint16_t y,uint16_t w, uint16_t h);

// Initialize the specified canvas. the raw pixel data buffer is passed as
// a parameter. no memory allocation is performed.

void gr_init_canvas(grs_canvas &canv,unsigned char *pixdata, bm_mode pixtype, uint16_t w, uint16_t h);

// Initialize the specified sub canvas. no memory allocation is performed.

void gr_init_sub_canvas(grs_canvas &n, grs_canvas &src, uint16_t x, uint16_t y, uint16_t w, uint16_t h);

// Clear the current canvas to the specified color
void gr_clear_canvas(grs_canvas &, color_t color);

//=========================================================================
// Bitmap functions:

// these are the two workhorses, the others just use these
void gr_init_bitmap(grs_bitmap &bm, bm_mode mode, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t bytesperline, const uint8_t* data);
void gr_init_sub_bitmap (grs_bitmap &bm, grs_bitmap &bmParent, uint16_t x, uint16_t y, uint16_t w, uint16_t h);

void gr_init_bitmap_alloc(grs_main_bitmap &bm, bm_mode mode, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t bytesperline);
void gr_free_bitmap_data(grs_bitmap &bm);

// Allocate a bitmap and its pixel data buffer.
grs_bitmap_ptr gr_create_bitmap(uint16_t w,uint16_t h);

// Free the bitmap, but not the pixel data buffer
class grs_subbitmap;
typedef std::unique_ptr<grs_subbitmap> grs_subbitmap_ptr;

// Creates a bitmap which is part of another bitmap
grs_subbitmap_ptr gr_create_sub_bitmap(grs_bitmap &bm, uint16_t x, uint16_t y, uint16_t w, uint16_t h);

// Free the bitmap's data

#if !DXX_USE_OGL
void gr_bm_ubitblt(grs_canvas &dest, unsigned w, unsigned h, int dx, int dy, int sx, int sy, const grs_bitmap &src);
void gr_bm_ubitbltm(grs_canvas &dest, unsigned w, unsigned h, unsigned dx, unsigned dy, unsigned sx, unsigned sy, const grs_bitmap &src);
#define gr_bm_pixel(C,B,X,Y,C2) gr_bm_pixel(B,X,Y,C2)
#define gr_settransblend(A,B,C)	gr_settransblend(A,B)
#endif
void gr_bm_pixel(grs_canvas &, grs_bitmap &bm, uint_fast32_t x, uint_fast32_t y, uint8_t color);
void gr_set_bitmap_data(grs_bitmap &bm, const uint8_t *data);
}

#ifdef dsx
namespace dsx {

//=========================================================================
// Color functions:

// When this function is called, the guns are set to gr_palette, and
// the palette stays the same until gr_close is called

void gr_use_palette_table(const char * filename);

}
#endif

//=========================================================================
// Drawing functions:

namespace dcx {

// Sets transparency and blending function
void gr_settransblend(grs_canvas &, int fade_level, gr_blend blend_func);

// Draws a point into the current canvas in the current color and drawmode.
void gr_pixel(grs_bitmap &, unsigned x, unsigned y, uint8_t color);
void gr_upixel(grs_bitmap &, unsigned x, unsigned y, uint8_t color);

// Gets a pixel;
unsigned char gr_gpixel(const grs_bitmap &bitmap, unsigned x, unsigned y);
unsigned char gr_ugpixel(const grs_bitmap &bitmap, int x, int y);

// Draws a line into the current canvas in the current color and drawmode.
void gr_line(grs_canvas &, fix x0,fix y0,fix x1,fix y1, uint8_t color);
void gr_uline(grs_canvas &canvas, fix x0,fix y0,fix x1,fix y1, uint8_t color);

// Draw the bitmap into the current canvas at the specified location.
void gr_bitmap(grs_canvas &, unsigned x,unsigned y,grs_bitmap &bm);
void gr_ubitmap(grs_canvas &, grs_bitmap &bm);
void show_fullscr(grs_canvas &, grs_bitmap &bm);

// Find transparent area in bitmap
void gr_bitblt_find_transparent_area(const grs_bitmap &bm, unsigned &minx, unsigned &miny, unsigned &maxx, unsigned &maxy);

// bitmap function with transparency
#if !DXX_USE_OGL
void gr_bitmapm(grs_canvas &, unsigned x, unsigned y, const grs_bitmap &bm);
void gr_ubitmapm(grs_canvas &, unsigned x, unsigned y, grs_bitmap &bm);
#endif

// Draw a rectangle into the current canvas.
void gr_rect(grs_canvas &, int left,int top,int right,int bot, uint8_t color);
void gr_urect(grs_canvas &, int left,int top,int right,int bot, uint8_t color);

// Draw a filled circle
int gr_disk(grs_canvas &, fix x,fix y,fix r, uint8_t color);

// Draw an outline circle
int gr_ucircle(grs_canvas &, fix x,fix y,fix r, uint8_t color);

// Draw an unfilled rectangle into the current canvas
#if DXX_USE_EDITOR
void gr_box(grs_canvas &, uint_fast32_t left,uint_fast32_t top,uint_fast32_t right,uint_fast32_t bot, uint8_t color);
#endif
void gr_ubox(grs_canvas &, int left,int top,int right,int bot, uint8_t color);

void gr_scanline(grs_canvas &canvas, int x1, int x2, unsigned y, uint8_t color);
#if !DXX_USE_OGL
void gr_uscanline(grs_canvas &canvas, unsigned x1, unsigned x2, unsigned y, uint8_t color);
#endif
void gr_close_font(std::unique_ptr<grs_font> font);

struct font_delete;
typedef std::unique_ptr<grs_font, font_delete> grs_font_ptr;

// Reads in a font file... current font set to this one.
grs_font_ptr gr_init_font(grs_canvas &canvas, const char * fontfile);

}

#ifdef dsx
namespace dsx {

#if defined(DXX_BUILD_DESCENT_I)
#define DXX_SDL_WINDOW_CAPTION	"Descent"
#define DXX_SDL_WINDOW_ICON_BITMAP	"d1x-rebirth.bmp"
#elif defined(DXX_BUILD_DESCENT_II)
#define DXX_SDL_WINDOW_CAPTION	"Descent II"
#define DXX_SDL_WINDOW_ICON_BITMAP	"d2x-rebirth.bmp"
void gr_copy_palette(palette_array_t &gr_palette, const palette_array_t &pal);
#endif

}
#endif

// Writes a string using current font. Returns the next column after last char.
namespace dcx {

//remap (by re-reading) all the color fonts
void gr_remap_color_fonts();
void gr_set_curfont(grs_canvas &, const grs_font *);
void gr_set_fontcolor(grs_canvas &, int fg_color, int bg_color);
void gr_string(grs_canvas &, const grs_font &, int x, int y, const char *s, int w, int h);
void gr_string(grs_canvas &, const grs_font &, int x, int y, const char *s);
void gr_ustring(grs_canvas &, const grs_font &, int x, int y, const char *s);
template <void (&)(grs_canvas &, const grs_font &, int, int, const char *)>
void gr_printt(grs_canvas &, const grs_font &, int x, int y, const char *format, ...) __attribute_format_printf(5, 6);
#define gr_printfs(...)	gr_printt<gr_string>(__VA_ARGS__)
#define gr_printfus(...)	gr_printt<gr_ustring>(__VA_ARGS__)
#define gr_printf(A1,A2,A3,A4,F,...)	dxx_call_printf_checked(gr_printfs,gr_string,(A1,A2,A3,A4),(F),##__VA_ARGS__)
#define gr_uprintf(A1,A2,A3,A4,F,...)	dxx_call_printf_checked(gr_printfus,gr_ustring,(A1,A2,A3,A4),(F),##__VA_ARGS__)
std::pair<const char *, unsigned> gr_get_string_wrap(const grs_font &, const char *s, unsigned limit);
void gr_get_string_size(const grs_font &, const char *s, int *string_width, int *string_height, int *average_width);
void gr_get_string_size(const grs_font &, const char *s, int *string_width, int *string_height, int *average_width, const unsigned max_chars_per_line);

// From scale.c
void scale_bitmap(const grs_bitmap &bp, const array<grs_point, 3> &vertbuf, int orientation, grs_bitmap &);

//===========================================================================
// Global variables
extern grs_canvas *grd_curcanv;             //active canvas
extern std::unique_ptr<grs_screen> grd_curscreen;           //active screen

/* Define `DXX_DEBUG_CURRENT_CANVAS_ORIGIN` to a positive integer N to
 * save __FILE__+__LINE__ of the N most recent callers.  If
 * `DXX_DEBUG_CURRENT_CANVAS_ORIGIN` is not a positive integer, the
 * file+line is not passed to the canvas update functions.
 *
 * Use this to trace which functions update the global canvas pointer
 * without requiring a debugger trap on every update.
 */
#define DXX_DEBUG_CURRENT_CANVAS_ORIGIN	0

#if DXX_DEBUG_CURRENT_CANVAS_ORIGIN > 0
#define DXX_DEBUG_CURRENT_CANVAS_FILE_LINE_COMMA_N_DECL_VARS	const char *const file, const unsigned line
#define DXX_DEBUG_CURRENT_CANVAS_FILE_LINE_COMMA_L_DECL_VARS	, DXX_DEBUG_CURRENT_CANVAS_FILE_LINE_COMMA_N_DECL_VARS
#define DXX_DEBUG_CURRENT_CANVAS_FILE_LINE_COMMA_N_PASS_VARS	file, line
#define DXX_DEBUG_CURRENT_CANVAS_FILE_LINE_COMMA_L_PASS_VARS	, DXX_DEBUG_CURRENT_CANVAS_FILE_LINE_COMMA_N_PASS_VARS
#define gr_set_default_canvas()	gr_set_default_canvas(__FILE__, __LINE__)
#define gr_set_current_canvas(C)	gr_set_current_canvas(C, __FILE__, __LINE__)
#else
#define DXX_DEBUG_CURRENT_CANVAS_FILE_LINE_COMMA_N_DECL_VARS
#define DXX_DEBUG_CURRENT_CANVAS_FILE_LINE_COMMA_L_DECL_VARS
#define DXX_DEBUG_CURRENT_CANVAS_FILE_LINE_COMMA_N_PASS_VARS
#define DXX_DEBUG_CURRENT_CANVAS_FILE_LINE_COMMA_L_PASS_VARS
#endif
void (gr_set_default_canvas)(DXX_DEBUG_CURRENT_CANVAS_FILE_LINE_COMMA_N_DECL_VARS);
void (gr_set_current_canvas)(grs_canvas & DXX_DEBUG_CURRENT_CANVAS_FILE_LINE_COMMA_L_DECL_VARS);
void (gr_set_current_canvas)(std::nullptr_t DXX_DEBUG_CURRENT_CANVAS_FILE_LINE_COMMA_L_DECL_VARS) = delete;
void gr_set_current_canvas2(grs_canvas * DXX_DEBUG_CURRENT_CANVAS_FILE_LINE_COMMA_L_DECL_VARS);

static inline void gr_set_current_canvas_inline(grs_canvas *const canv DXX_DEBUG_CURRENT_CANVAS_FILE_LINE_COMMA_L_DECL_VARS)
{
	if (canv)
		(gr_set_current_canvas)(*canv DXX_DEBUG_CURRENT_CANVAS_FILE_LINE_COMMA_L_PASS_VARS);
	else
		(gr_set_default_canvas)(DXX_DEBUG_CURRENT_CANVAS_FILE_LINE_COMMA_N_PASS_VARS);
}

static inline void (gr_set_current_canvas)(grs_canvas *const canv DXX_DEBUG_CURRENT_CANVAS_FILE_LINE_COMMA_L_DECL_VARS)
{
#ifdef DXX_HAVE_BUILTIN_CONSTANT_P
	if (dxx_builtin_constant_p(!canv))
		gr_set_current_canvas_inline(canv DXX_DEBUG_CURRENT_CANVAS_FILE_LINE_COMMA_L_PASS_VARS);
	else
#endif
		gr_set_current_canvas2(canv DXX_DEBUG_CURRENT_CANVAS_FILE_LINE_COMMA_L_PASS_VARS);
}

//flags for fonts
#define FT_COLOR        1
#define FT_PROPORTIONAL 2
#define FT_KERNED       4

extern palette_array_t gr_palette;
typedef array<array<color_t, 256>, GR_FADE_LEVELS> gft_array1;
extern gft_array1 gr_fade_table;
}

extern uint16_t gr_palette_selector;
extern uint16_t gr_inverse_table_selector;
extern uint16_t gr_fade_table_selector;

// Remaps a bitmap into the current palette. If transparent_color is
// between 0 and 255 then all occurances of that color are mapped to
// whatever color the 2d uses for transparency. This is normally used
// right after a call to iff_read_bitmap like this:
//		iff_error = iff_read_bitmap(filename,new,bm_mode::linear,newpal);
//		if (iff_error != IFF_NO_ERROR) Error("Can't load IFF file <%s>, error=%d",filename,iff_error);
//		if (iff_has_transparency)
//			gr_remap_bitmap(new, newpal, iff_transparent_color);
//		else
//			gr_remap_bitmap(new, newpal, -1);

// Same as above, but searches using gr_find_closest_color which uses
// 18-bit accurracy instead of 15bit when translating colors.
namespace dcx {
void gr_remap_bitmap_good(grs_bitmap &bmp, palette_array_t &palette, uint_fast32_t transparent_color, uint_fast32_t super_transparent_color);

void gr_palette_step_up(int r, int g, int b);

#define BM_RGB(r,g,b) ((((r)&31)<<10) | (((g)&31)<<5) | ((b)&31))
#define BM_XRGB(r,g,b) gr_find_closest_color((r)*2,(g)*2,(b)*2)

// Given: r,g,b, each in range of 0-63, return the color index that
// best matches the input.
color_t gr_find_closest_color(int r, int g, int b);
int gr_find_closest_color_15bpp(int rgb);
void gr_flip();

/*
 * must return 0 if windowed, 1 if fullscreen
 */
int gr_check_fullscreen();

/*
 * returns state after toggling (ie, same as if you had called
 * check_fullscreen immediatly after)
 */
void gr_toggle_fullscreen();

void ogl_do_palfx();
void ogl_init_pixel_buffers(unsigned w, unsigned h);
void ogl_close_pixel_buffers();
}
void ogl_cache_polymodel_textures(unsigned model_num);

#endif
