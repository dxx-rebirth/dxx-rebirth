/*
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

#ifndef _GR_H
#define _GR_H

#include "types.h"
#include "fix.h"

#define GR_FADE_LEVELS 34
#define GR_ACTUAL_FADE_LEVELS 32

extern int Gr_scanline_darkening_level;

typedef struct _grs_point {
	fix	x,y;
} grs_point;

//these are control characters that have special meaning in the font code

#define CC_COLOR			1		//next char is new foreground color
#define CC_LSPACING		2		//next char specifies line spacing
#define CC_UNDERLINE		3		//next char is underlined

//now have string versions of these control characters (can concat inside a string)

#define CC_COLOR_S	 	"\x1"		//next char is new foreground color
#define CC_LSPACING_S 	"\x2"		//next char specifies line spacing
#define CC_UNDERLINE_S	"\x3"		//next char is underlined

typedef struct _grs_font {
	short		ft_w,ft_h;		// Width and height in pixels
	short		ft_flags;		// Proportional?
	short		ft_baseline;	//
	ubyte		ft_minchar,		// The first and last chars defined by
				ft_maxchar;		// This font
	short		ft_bytewidth;	// Width in unsigned chars
	ubyte	* 	ft_data;			// Ptr to raw data.
	ubyte	**	ft_chars;		// Ptrs to data for each char (required for prop font)
	short	*	ft_widths;		// Array of widths (required for prop font)
	ubyte *  ft_kerndata;	// Array of kerning triplet data
} grs_font;

#define BM_LINEAR   0
#define BM_MODEX    1
#define BM_SVGA     2
#define BM_RGB15    3   //5 bits each r,g,b stored at 16 bits
#define BM_SVGA15   4

//@@//	Define these modes for Gameplay too, since the game was developed under
//@@//	DOS, we will adapt these modes to other systems thru rendering.
//@@#define SM_ORIGINAL		-1
//@@#define SM_320x200C     0
//@@#define SM_320x200U     1
//@@#define SM_320x240U     2
//@@#define SM_360x200U     3
//@@#define SM_360x240U     4
//@@#define SM_376x282U     5
//@@#define SM_320x400U     6
//@@#define SM_320x480U     7
//@@#define SM_360x400U     8
//@@#define SM_360x480U     9
//@@#define SM_360x360U     10
//@@#define SM_376x308U     11
//@@#define SM_376x564U     12
//@@#define SM_640x400V     13
//@@#define SM_640x480V     14
//@@#define SM_800x600V     15
//@@#define SM_1024x768V    16
//@@#define SM_640x480V15   17
//@@#define SM_800x600V15   18


#define BM_FLAG_TRANSPARENT			1
#define BM_FLAG_SUPER_TRANSPARENT	2
#define BM_FLAG_NO_LIGHTING			4
#define BM_FLAG_RLE						8			// A run-length encoded bitmap.
#define BM_FLAG_PAGED_OUT				16			// This bitmap's data is paged out.

typedef struct _grs_bitmap {
	short       bm_x,bm_y;      // Offset from parent's origin
	short       bm_w,bm_h;      // width,height
	byte       	bm_type;        // 0=Linear, 1=ModeX, 2=SVGA
	byte			bm_flags;		// bit 0 on means it has transparency.
										// bit 1 on means it has supertransparency
										// bit 2 on means it doesn't get passed through lighting.
	short	    bm_rowsize;     // unsigned char offset to next row
	unsigned char *      bm_data;	     // ptr to pixel data...
								//   Linear = *parent+(rowsize*y+x)
								//   ModeX = *parent+(rowsize*y+x/4)
								//   SVGA = *parent+(rowsize*y+x)
	unsigned short bm_selector;
	ubyte			avg_color;		//	Average color of all pixels in texture map.
	byte			unused;			//	to 4-byte align.
} grs_bitmap;

typedef struct _grs_canvas {
	grs_bitmap  cv_bitmap;      // the bitmap for this canvas
	short       cv_color;       // current color
	short       cv_drawmode;    // fill,XOR,etc.
	grs_font *  cv_font;        // the currently selected font
	short       cv_font_fg_color;   // current font foreground color (-1==Invisible)
	short       cv_font_bg_color;   // current font background color (-1==Invisible)
} grs_canvas;

typedef struct _grs_screen {     // This is a video screen
	grs_canvas  sc_canvas;      // Represents the entire screen
	short       sc_mode;        // Video mode number
	short       sc_w, sc_h;     // Actual Width and Height
	fix			sc_aspect;		//aspect ratio (w/h) for this screen
} grs_screen;


//=========================================================================
// System functions:
// setup and set mode. this creates a grs_screen structure and sets
// grd_curscreen to point to it.  grs_curcanv points to this screen's
// canvas.  Saves the current VGA state and screen mode.
						  
int gr_init(void);

// This function sets up the main screen.  It should be called whenever
// the video mode changes. 
int gr_init_screen(int mode, int w, int h, int rowsize, ubyte *data);


// These 4 functions actuall change screen colors.

extern void gr_pal_fade_out(unsigned char * pal);
extern void gr_pal_fade_in(unsigned char * pal);
extern void gr_pal_clear(void);
extern void gr_pal_setblock( int start, int number, unsigned char * pal );
extern void gr_pal_getblock( int start, int number, unsigned char * pal );


extern unsigned char *gr_video_memory;
												// All graphic modules will define this value.

//shut down the 2d.  Restore the screen mode.
int gr_close(void);

//=========================================================================
// Canvas functions:

// Makes a new canvas. allocates memory for the canvas and its bitmap,
// including the raw pixel buffer.

grs_canvas *gr_create_canvas(int w, int h);

// Creates a canvas that is part of another canvas.  this can be used to make
// a window on the screen.  the canvas structure is malloc'd; the address of
// the raw pixel data is inherited from the parent canvas.

grs_canvas *gr_create_sub_canvas(grs_canvas *canv,int x,int y,int w, int h);

// Initialize the specified canvas. the raw pixel data buffer is passed as
// a parameter. no memory allocation is performed.

gr_init_canvas(grs_canvas *canv,unsigned char *pixdata,int pixtype, int w,int h);

// Initialize the specified sub canvas. no memory allocation is performed.

gr_init_sub_canvas(grs_canvas *new,grs_canvas *src,int x,int y,int w, int h);

// Free up the canvas and its pixel data.

gr_free_canvas(grs_canvas *canv);

// Free up the canvas. do not free the pixel data, which belongs to the
// parent canvas.

gr_free_sub_canvas(grs_canvas *canv);

// Clear the current canvas to the specified color
gr_clear_canvas(int color);

//=========================================================================
// Bitmap functions:

// Allocate a bitmap and its pixel data buffer.
grs_bitmap *gr_create_bitmap(int w,int h);

// Allocated a bitmap and makes its data be raw_data that is already somewhere.
grs_bitmap *gr_create_bitmap_raw(int w, int h, unsigned char * raw_data );

// Creates a bitmap which is part of another bitmap
grs_bitmap *gr_create_sub_bitmap(grs_bitmap *bm,int x,int y,int w, int h);

// Free the bitmap and its pixel data
gr_free_bitmap(grs_bitmap *bm);

// Free the bitmap, but not the pixel data buffer
gr_free_sub_bitmap(grs_bitmap *bm);

void gr_bm_pixel( grs_bitmap * bm, int x, int y, unsigned char color );
void gr_bm_upixel( grs_bitmap * bm, int x, int y, unsigned char color );
void gr_bm_ubitblt( int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest);
void gr_bm_ubitbltm(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest);

void gr_update_buffer( void * sbuf1, void * sbuf2, void * dbuf, int size );

//=========================================================================
// Color functions:

// When this function is called, the guns are set to gr_palette, and
// the palette stays the same until gr_close is called

void gr_use_palette_table(char * filename );

//=========================================================================
// Drawing functions:

// For solid, XOR, or other fill modes.
gr_set_drawmode(int mode);

// Sets the color in the current canvas.  should be a macro
// Use: gr_setcolor(int color);
void gr_setcolor(int color);

// Draw a polygon into the current canvas in the current color and drawmode.
// verts points to an ordered list of x,y pairs.  the polygon should be
// convex; a concave polygon will be handled in some reasonable manner,
// but not necessarily shaded as a concave polygon. It shouldn't hang.
// probably good solution is to shade from minx to maxx on each scan line.
// int should really be fix
gr_poly(int nverts,int *verts);
gr_upoly(int nverts,int *verts);


// Draws a point into the current canvas in the current color and drawmode.
gr_pixel(int x,int y);
gr_upixel(int x,int y);

// Gets a pixel;
unsigned char gr_gpixel( grs_bitmap * bitmap, int x, int y );
unsigned char gr_ugpixel( grs_bitmap * bitmap, int x, int y );

// Draws a line into the current canvas in the current color and drawmode.
int gr_line(fix x0,fix y0,fix x1,fix y1);
int gr_uline(fix x0,fix y0,fix x1,fix y1);

// Draws an anti-aliased line into the current canvas in the current color and drawmode.
int gr_aaline(fix x0,fix y0,fix x1,fix y1);
int gr_uaaline(fix x0,fix y0,fix x1,fix y1);

// Draw the bitmap into the current canvas at the specified location.
gr_bitmap(int x,int y,grs_bitmap *bm);
gr_ubitmap(int x,int y,grs_bitmap *bm);
// bitmap function with transparency
gr_bitmapm( int x, int y, grs_bitmap *bm );
gr_ubitmapm( int x, int y, grs_bitmap *bm );

// Draw a rectangle into the current canvas.
gr_rect(int left,int top,int right,int bot);
gr_urect(int left,int top,int right,int bot);

// Draw a filled circle
int gr_disk(fix x,fix y,fix r);
int gr_udisk(fix x,fix y,fix r);

// Draw an outline circle
gr_circle(fix x,fix y,fix r);
gr_ucircle(fix x,fix y,fix r);

// Draw an unfilled rectangle into the current canvas
gr_box(int left,int top,int right,int bot);
gr_ubox(int left,int top,int right,int bot);

void gr_scanline( int x1, int x2, int y );
void gr_uscanline( int x1, int x2, int y );


// Reads in a font file... current font set to this one.
grs_font * gr_init_font( char * fontfile );
void gr_close_font( grs_font * font );

//remap a font, re-reading its data & palette 
void gr_remap_font( grs_font *font, char * fontname );

//remap (by re-reading) all the color fonts
void gr_remap_color_fonts();

// Writes a string using current font. Returns the next column after last char.
void gr_set_fontcolor( int fg, int bg );
void gr_set_curfont( grs_font * new );
int gr_string(int x, int y, char *s );
int gr_ustring(int x, int y, char *s );
int gr_printf( int x, int y, char * format, ... );
int gr_uprintf( int x, int y, char * format, ... );
void gr_get_string_size(char *s, int *string_width, int *string_height, int *average_width );


//	From roller.c
void rotate_bitmap(grs_bitmap *bp, grs_point *vertbuf, int light_value);

// From scale.c
void scale_bitmap(grs_bitmap *bp, grs_point *vertbuf, int orientation );

//===========================================================================
// Global variables
extern grs_canvas *grd_curcanv;             //active canvas
extern grs_screen *grd_curscreen;           //active screen
extern unsigned char Test_bitmap_data[64*64];

extern unsigned int FixDivide( unsigned int x, unsigned int y );

extern void gr_show_canvas( grs_canvas *canv );
extern void gr_set_current_canvas( grs_canvas *canv );

//flags for fonts
#define FT_COLOR			1
#define FT_PROPORTIONAL	2
#define FT_KERNED			4

extern void gr_vesa_update( grs_bitmap * source1, grs_bitmap * dest, grs_bitmap * source2 );

// Special effects
extern void gr_snow_out(int num_dots);

extern void test_rotate_bitmap(void);
extern void rotate_bitmap(grs_bitmap *bp, grs_point *vertbuf, int light_value);

extern ubyte gr_palette[256*3];
extern ubyte gr_fade_table[];
extern ubyte gr_inverse_table[32*32*32];

extern ushort gr_palette_selector;
extern ushort gr_inverse_table_selector;
extern ushort gr_fade_table_selector;

// Remaps a bitmap into the current palette. If transparent_color is between 0 and 255
// then all occurances of that color are mapped to whatever color the 2d uses for 
// transparency. This is normally used right after a call to iff_read_bitmap like
// this:
//		iff_error = iff_read_bitmap(filename,new,BM_LINEAR,newpal);
//		if (iff_error != IFF_NO_ERROR) Error("Can't load IFF file <%s>, error=%d",filename,iff_error);
//		if ( iff_has_transparency )
//			gr_remap_bitmap( new, newpal, iff_transparent_color );
//		else
//			gr_remap_bitmap( new, newpal, -1 );
extern void gr_remap_bitmap( grs_bitmap * bmp, ubyte * palette, int transparent_color, int super_transparent_color );

// Same as above, but searches using gr_find_closest_color which uses 18-bit accurracy
// instaed of 15bit when translating colors.
extern void gr_remap_bitmap_good( grs_bitmap * bmp, ubyte * palette, int transparent_color, int super_transparent_color );

extern void gr_palette_step_up( int r, int g, int b );

extern void gr_bitmap_check_transparency( grs_bitmap * bmp );

// Allocates a selector that has a base address at 'address' and length 'size'.
// Returns 0 if successful... BE SURE TO CHECK the return value since there
// is a limited number of selectors available!!!
extern int get_selector( void * address, int size, unsigned int * selector );

// Assigns a selector to a bitmap. Returns 0 if successful.  BE SURE TO CHECK
// this return value since there is a limited number of selectors!!!!!!!
extern int gr_bitmap_assign_selector( grs_bitmap * bmp );

//#define GR_GETCOLOR(r,g,b) (gr_inverse_table[( (((r)&31)<<10) | (((g)&31)<<5) | ((b)&31) )])
//#define gr_getcolor(r,g,b) (gr_inverse_table[( (((r)&31)<<10) | (((g)&31)<<5) | ((b)&31) )])
//#define BM_XRGB(r,g,b) (gr_inverse_table[( (((r)&31)<<10) | (((g)&31)<<5) | ((b)&31) )])

#define BM_RGB(r,g,b) ( (((r)&31)<<10) | (((g)&31)<<5) | ((b)&31) )
#define BM_XRGB(r,g,b) gr_find_closest_color( (r)*2,(g)*2,(b)*2 )
#define GR_GETCOLOR(r,g,b) gr_find_closest_color( (r)*2,(g)*2,(b)*2 )
#define gr_getcolor(r,g,b) gr_find_closest_color( (r)*2,(g)*2,(b)*2 )

// Given: r,g,b, each in range of 0-63, return the color index that
// best matches the input.
int gr_find_closest_color( int r, int g, int b );

extern void gr_merge_textures( ubyte * lower, ubyte * upper, ubyte * dest );
extern void gr_merge_textures_1( ubyte * lower, ubyte * upper, ubyte * dest );
extern void gr_merge_textures_2( ubyte * lower, ubyte * upper, ubyte * dest );
extern void gr_merge_textures_3( ubyte * lower, ubyte * upper, ubyte * dest );

#endif

