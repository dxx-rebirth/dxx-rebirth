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

#include <cstdint>
#include <memory>
#include "pstypes.h"
#include "maths.h"
#include "palette.h"
#include "dxxsconf.h"
#include "fmtcheck.h"
#include "pack.h"
#include "compiler-array.h"

#ifdef DXX_BUILD_DESCENT_I
extern int HiresGFXAvailable;
#endif

// some defines for transparency and blending
#define TRANSPARENCY_COLOR   255            // palette entry of transparency color -- 255 on the PC
#define GR_FADE_LEVELS       34
#define GR_FADE_OFF          GR_FADE_LEVELS // yes, max means OFF - don't screw that up
#define GR_BLEND_NORMAL      0              // normal blending
#define GR_BLEND_ADDITIVE_A  1              // additive alpha blending
#define GR_BLEND_ADDITIVE_C  2              // additive color blending

#define GWIDTH  grd_curcanv->cv_bitmap.bm_w
#define GHEIGHT grd_curcanv->cv_bitmap.bm_h
#define SWIDTH  (grd_curscreen->get_screen_width())
#define SHEIGHT (grd_curscreen->get_screen_height())

#if defined(DXX_BUILD_DESCENT_I)
#define HIRESMODE HiresGFXAvailable		// descent.pig either contains hires or lowres graphics, not both
#endif
#if defined(DXX_BUILD_DESCENT_II)
#define HIRESMODE (SWIDTH >= 640 && SHEIGHT >= 480 && !GameArg.GfxSkipHiresGFX)
#endif
#define MAX_BMP_SIZE(width, height) (4 + ((width) + 2) * (height))

#define SCRNS_DIR "screenshots/"

struct grs_point
{
	fix x,y;
};

//these are control characters that have special meaning in the font code

#define CC_COLOR        1   //next char is new foreground color
#define CC_LSPACING     2   //next char specifies line spacing
#define CC_UNDERLINE    3   //next char is underlined

//now have string versions of these control characters (can concat inside a string)

#define CC_COLOR_S      "\x1"   //next char is new foreground color
#define CC_LSPACING_S   "\x2"   //next char specifies line spacing
#define CC_UNDERLINE_S  "\x3"   //next char is underlined

#define BM_LINEAR   0
#define BM_MODEX    1
#define BM_SVGA     2
#define BM_RGB15    3   //5 bits each r,g,b stored at 16 bits
#define BM_SVGA15   4
#ifdef OGL
#define BM_OGL      5
#endif /* def OGL */

#define SM(w,h) ((((u_int32_t)w)<<16)+(((u_int32_t)h)&0xFFFF))
#define SM_W(m) (m>>16)
#define SM_H(m) (m&0xFFFF)
#define SM_ORIGINAL		0

#define BM_FLAG_TRANSPARENT         1
#define BM_FLAG_SUPER_TRANSPARENT   2
#define BM_FLAG_NO_LIGHTING         4
#define BM_FLAG_RLE                 8   // A run-length encoded bitmap.
#define BM_FLAG_PAGED_OUT           16  // This bitmap's data is paged out.
#define BM_FLAG_RLE_BIG             32  // for bitmaps that RLE to > 255 per row (i.e. cockpits)

struct grs_bitmap : prohibit_void_ptr<grs_bitmap>
{
	uint16_t bm_x,bm_y; // Offset from parent's origin
	uint16_t bm_w,bm_h; // width,height
	sbyte   bm_type;    // 0=Linear, 1=ModeX, 2=SVGA
	sbyte   bm_flags;   // bit 0 on means it has transparency.
	                    // bit 1 on means it has supertransparency
	                    // bit 2 on means it doesn't get passed through lighting.
	short   bm_rowsize; // unsigned char offset to next row
	union {
		const uint8_t *bm_data;     // ptr to pixel data...
	                                //   Linear = *parent+(rowsize*y+x)
	                                //   ModeX = *parent+(rowsize*y+x/4)
	                                //   SVGA = *parent+(rowsize*y+x)
		uint8_t *bm_mdata;
	};
	const uint8_t *get_bitmap_data() const { return bm_data; }
	uint8_t *get_bitmap_data() { return bm_mdata; }
	unsigned short      bm_handle;  //for application.  initialized to 0
	ubyte   avg_color;  //  Average color of all pixels in texture map.
	array<fix, 3> avg_color_rgb; // same as above but real rgb value to be used to textured objects that should emit light
	struct grs_bitmap  *bm_parent;
#ifdef OGL
	struct ogl_texture *gltexture;
#endif /* def OGL */
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
	short       ft_bytewidth;   // Width in unsigned chars
	ubyte     * ft_data;        // Ptr to raw data.
	std::unique_ptr<ubyte *[]>    ft_chars;       // Ptrs to data for each char (required for prop font)
	short     * ft_widths;      // Array of widths (required for prop font)
	ubyte     * ft_kerndata;    // Array of kerning triplet data
#ifdef OGL
	// These fields do not participate in disk i/o!
	std::unique_ptr<grs_bitmap[]> ft_bitmaps;
	grs_bitmap ft_parent_bitmap;
#endif /* def OGL */
};

#define GRS_FONT_SIZE 28    // how much space it takes up on disk

struct grs_canvas : prohibit_void_ptr<grs_canvas>
{
	grs_bitmap  cv_bitmap;      // the bitmap for this canvas
	const grs_font *  cv_font;        // the currently selected font
	uint8_t     cv_color;       // current color
	short       cv_drawmode;    // fill,XOR,etc.
	short       cv_font_fg_color;   // current font foreground color (-1==Invisible)
	short       cv_font_bg_color;   // current font background color (-1==Invisible)
	int         cv_fade_level;  // transparency level
	ubyte       cv_blend_func;  // blending function to use
};


//=========================================================================
// System functions:
// setup and set mode. this creates a grs_screen structure and sets
// grd_curscreen to point to it.  grs_curcanv points to this screen's
// canvas.  Saves the current VGA state and screen mode.

#ifdef __cplusplus

class grs_screen : prohibit_void_ptr<grs_screen>
{    // This is a video screen
	unsigned short   sc_w, sc_h;     // Actual Width and Height
public:
	grs_canvas  sc_canvas;  // Represents the entire screen
	fix     sc_aspect;      //aspect ratio (w/h) for this screen
	uint_fast32_t get_screen_width() const
	{
		return sc_w;
	}
	uint_fast32_t get_screen_height() const
	{
		return sc_h;
	}
	uint_fast32_t get_screen_width_height() const
	{
		return SM(get_screen_width(), get_screen_height());
	}
	void set_screen_width_height(uint16_t w, uint16_t h)
	{
		sc_w = w;
		sc_h = h;
	}
};

int gr_init(int mode);

int gr_list_modes( array<uint32_t, 50> &gsmodes );
int gr_set_mode(u_int32_t mode);
void gr_set_attributes(void);

//shut down the 2d.  Restore the screen mode.
void gr_close(void);

//=========================================================================
// Canvas functions:

// Makes a new canvas. allocates memory for the canvas and its bitmap,
// including the raw pixel buffer.

struct grs_main_canvas : grs_canvas
{
	~grs_main_canvas();
};
typedef std::unique_ptr<grs_main_canvas> grs_canvas_ptr;

grs_canvas_ptr gr_create_canvas(uint16_t w, uint16_t h);

// Creates a canvas that is part of another canvas.  this can be used to make
// a window on the screen.  the canvas structure is malloc'd; the address of
// the raw pixel data is inherited from the parent canvas.

struct grs_subcanvas : grs_canvas {};
typedef std::unique_ptr<grs_subcanvas> grs_subcanvas_ptr;

grs_subcanvas_ptr gr_create_sub_canvas(grs_canvas &canv,uint16_t x,uint16_t y,uint16_t w, uint16_t h);

// Initialize the specified canvas. the raw pixel data buffer is passed as
// a parameter. no memory allocation is performed.

void gr_init_canvas(grs_canvas &canv,unsigned char *pixdata, uint8_t pixtype, uint16_t w, uint16_t h);

// Initialize the specified sub canvas. no memory allocation is performed.

void gr_init_sub_canvas(grs_canvas &n, grs_canvas &src, uint16_t x, uint16_t y, uint16_t w, uint16_t h);

// Clear the current canvas to the specified color
void gr_clear_canvas(color_t color);

//=========================================================================
// Bitmap functions:

// these are the two workhorses, the others just use these
void gr_init_bitmap(grs_bitmap &bm, uint8_t mode, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t bytesperline, unsigned char * data);
void gr_init_sub_bitmap (grs_bitmap &bm, grs_bitmap &bmParent, uint16_t x, uint16_t y, uint16_t w, uint16_t h);

void gr_init_bitmap_alloc(grs_bitmap &bm, uint8_t mode, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t bytesperline);
void gr_free_bitmap_data(grs_bitmap &bm);

// Free the bitmap and its pixel data
class grs_main_bitmap : public grs_bitmap
{
public:
	grs_main_bitmap() = default;
	grs_main_bitmap(const grs_main_bitmap &) = delete;
	grs_main_bitmap &operator=(const grs_main_bitmap &) = delete;
	~grs_main_bitmap()
	{
		gr_free_bitmap_data(*this);
	}
};

typedef std::unique_ptr<grs_main_bitmap> grs_bitmap_ptr;

// Allocate a bitmap and its pixel data buffer.
grs_bitmap_ptr gr_create_bitmap(uint16_t w,uint16_t h);

// Free the bitmap, but not the pixel data buffer
class grs_subbitmap : public grs_bitmap
{
};

typedef std::unique_ptr<grs_subbitmap> grs_subbitmap_ptr;

// Creates a bitmap which is part of another bitmap
grs_subbitmap_ptr gr_create_sub_bitmap(grs_bitmap &bm, uint16_t x, uint16_t y, uint16_t w, uint16_t h);

// Free the bitmap's data
void gr_init_bitmap_data (grs_bitmap &bm);

void gr_bm_pixel(grs_bitmap &bm, uint_fast32_t x, uint_fast32_t y, uint8_t color );
void gr_bm_ubitblt(unsigned w, unsigned h, int dx, int dy, int sx, int sy, const grs_bitmap &src, grs_bitmap &dest);
void gr_bm_ubitbltm(unsigned w, unsigned h, unsigned dx, unsigned dy, unsigned sx, unsigned sy, const grs_bitmap &src, grs_bitmap &dest);

static inline void gr_set_bitmap_flags(grs_bitmap &bm, uint8_t flags)
{
	bm.bm_flags = flags;
}

static inline void gr_set_transparent(grs_bitmap &bm, bool bTransparent)
{
	auto bm_flags = bm.bm_flags;
	gr_set_bitmap_flags(bm, bTransparent ? bm_flags | BM_FLAG_TRANSPARENT : bm_flags & ~BM_FLAG_TRANSPARENT);
}

void gr_set_bitmap_data(grs_bitmap &bm, unsigned char *data);

//=========================================================================
// Color functions:

// When this function is called, the guns are set to gr_palette, and
// the palette stays the same until gr_close is called

void gr_use_palette_table(const char * filename );

//=========================================================================
// Drawing functions:

// Sets the color in the current canvas.
void gr_setcolor(color_t color);
// Sets transparency and blending function
void gr_settransblend(int fade_level, ubyte blend_func);

// Draws a point into the current canvas in the current color and drawmode.
void gr_pixel(int x,int y);
void gr_upixel(int x,int y);

// Gets a pixel;
unsigned char gr_gpixel(const grs_bitmap &bitmap, int x, int y );
unsigned char gr_ugpixel(const grs_bitmap &bitmap, int x, int y );

// Draws a line into the current canvas in the current color and drawmode.
void gr_line(fix x0,fix y0,fix x1,fix y1);
void gr_uline(fix x0,fix y0,fix x1,fix y1);

// Draw the bitmap into the current canvas at the specified location.
void gr_bitmap(unsigned x,unsigned y,grs_bitmap &bm);
void gr_ubitmap(grs_bitmap &bm);
void show_fullscr(grs_bitmap &bm);

// Find transparent area in bitmap
void gr_bitblt_find_transparent_area(const grs_bitmap &bm, unsigned &minx, unsigned &miny, unsigned &maxx, unsigned &maxy);

// bitmap function with transparency
void gr_bitmapm(unsigned x, unsigned y, const grs_bitmap &bm);
void gr_ubitmapm(unsigned x, unsigned y, grs_bitmap &bm);

// Draw a rectangle into the current canvas.
void gr_rect(int left,int top,int right,int bot);
void gr_urect(int left,int top,int right,int bot);

// Draw a filled circle
int gr_disk(fix x,fix y,fix r);

// Draw an outline circle
int gr_circle(fix x,fix y,fix r);
int gr_ucircle(fix x,fix y,fix r);

// Draw an unfilled rectangle into the current canvas
void gr_box(uint_fast32_t left,uint_fast32_t top,uint_fast32_t right,uint_fast32_t bot);
void gr_ubox(int left,int top,int right,int bot);

void gr_scanline( int x1, int x2, int y );
void gr_uscanline( int x1, int x2, int y );

void gr_close_font(std::unique_ptr<grs_font> font);

struct font_delete
{
	void operator()(grs_font *p) const
	{
		gr_close_font(std::unique_ptr<grs_font>(p));
	}
};

typedef std::unique_ptr<grs_font, font_delete> grs_font_ptr;

// Reads in a font file... current font set to this one.
grs_font_ptr gr_init_font( const char * fontfile );

#if defined(DXX_BUILD_DESCENT_I)
#define DXX_SDL_WINDOW_CAPTION	"Descent"
#define DXX_SDL_WINDOW_ICON_BITMAP	"d1x-rebirth.bmp"

static inline void gr_remap_color_fonts() {}
static inline void gr_remap_mono_fonts() {}
#elif defined(DXX_BUILD_DESCENT_II)
#define DXX_SDL_WINDOW_CAPTION	"Descent II"
#define DXX_SDL_WINDOW_ICON_BITMAP	"d2x-rebirth.bmp"
void gr_copy_palette(palette_array_t &gr_palette, const palette_array_t &pal);

//remap (by re-reading) all the color fonts
void gr_remap_color_fonts();
void gr_remap_mono_fonts();
#endif

// Writes a string using current font. Returns the next column after last char.
void gr_set_curfont(const grs_font *);
static inline void gr_set_curfont(const grs_font_ptr &p)
{
	gr_set_curfont(p.get());
}
void gr_set_fontcolor( int fg_color, int bg_color );
void gr_string(int x, int y, const char *s );
void gr_ustring(int x, int y, const char *s );
void gr_printf( int x, int y, const char * format, ... ) __attribute_format_printf(3, 4);
#define gr_printf(A1,A2,F,...)	dxx_call_printf_checked(gr_printf,gr_string,(A1,A2),(F),##__VA_ARGS__)
void gr_uprintf( int x, int y, const char * format, ... ) __attribute_format_printf(3, 4);
#define gr_uprintf(A1,A2,F,...)	dxx_call_printf_checked(gr_uprintf,gr_ustring,(A1,A2),(F),##__VA_ARGS__)
void gr_get_string_size(const char *s, int *string_width, int *string_height, int *average_width );


// From scale.c
void scale_bitmap(const grs_bitmap &bp, const array<grs_point, 3> &vertbuf, int orientation );

//===========================================================================
// Global variables
extern grs_canvas *grd_curcanv;             //active canvas
extern std::unique_ptr<grs_screen> grd_curscreen;           //active screen

void gr_set_default_canvas();
void gr_set_current_canvas(grs_canvas &);
void _gr_set_current_canvas(grs_canvas *);

static inline void _gr_set_current_canvas_inline(grs_canvas *canv)
{
	if (canv)
		gr_set_current_canvas(*canv);
	else
		gr_set_default_canvas();
}

static inline void gr_set_current_canvas(grs_canvas *canv)
{
#ifdef DXX_HAVE_BUILTIN_CONSTANT_P
	if (__builtin_constant_p(!canv))
		_gr_set_current_canvas_inline(canv);
	else
#endif
		_gr_set_current_canvas(canv);
}

static inline void gr_set_current_canvas(grs_canvas_ptr &canv)
{
	gr_set_current_canvas(canv.get());
}

static inline void gr_set_current_canvas(grs_subcanvas_ptr &canv)
{
	gr_set_current_canvas(canv.get());
}

//flags for fonts
#define FT_COLOR        1
#define FT_PROPORTIONAL 2
#define FT_KERNED       4

extern palette_array_t gr_palette;
typedef array<color_t, 256> gft_array0;
typedef array<gft_array0, GR_FADE_LEVELS> gft_array1;
extern gft_array1 gr_fade_table;

extern ushort gr_palette_selector;
extern ushort gr_inverse_table_selector;
extern ushort gr_fade_table_selector;

// Remaps a bitmap into the current palette. If transparent_color is
// between 0 and 255 then all occurances of that color are mapped to
// whatever color the 2d uses for transparency. This is normally used
// right after a call to iff_read_bitmap like this:
//		iff_error = iff_read_bitmap(filename,new,BM_LINEAR,newpal);
//		if (iff_error != IFF_NO_ERROR) Error("Can't load IFF file <%s>, error=%d",filename,iff_error);
//		if ( iff_has_transparency )
//			gr_remap_bitmap( new, newpal, iff_transparent_color );
//		else
//			gr_remap_bitmap( new, newpal, -1 );
void gr_remap_bitmap( grs_bitmap * bmp, palette_array_t &palette, int transparent_color, int super_transparent_color );

// Same as above, but searches using gr_find_closest_color which uses
// 18-bit accurracy instead of 15bit when translating colors.
void gr_remap_bitmap_good(grs_bitmap &bmp, palette_array_t &palette, uint_fast32_t transparent_color, uint_fast32_t super_transparent_color);

void gr_palette_step_up( int r, int g, int b );

extern void gr_bitmap_check_transparency( grs_bitmap * bmp );

#define BM_RGB(r,g,b) ( (((r)&31)<<10) | (((g)&31)<<5) | ((b)&31) )
#define BM_XRGB(r,g,b) gr_find_closest_color( (r)*2,(g)*2,(b)*2 )

// Given: r,g,b, each in range of 0-63, return the color index that
// best matches the input.
color_t gr_find_closest_color( int r, int g, int b );
int gr_find_closest_color_15bpp( int rgb );

extern void gr_flip(void);

/*
 * must return 0 if windowed, 1 if fullscreen
 */
int gr_check_fullscreen(void);

/*
 * returns state after toggling (ie, same as if you had called
 * check_fullscreen immediatly after)
 */
void gr_toggle_fullscreen();
void ogl_do_palfx(void);
void ogl_init_pixel_buffers(unsigned w, unsigned h);
void ogl_close_pixel_buffers(void);
void ogl_cache_polymodel_textures(int model_num);;

#endif
