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



#ifndef _DD_H
#define _DD_H

#include "pstypes.h"
#include "gr.h"
#include "ddraw.h"

//	Render Modes supported by the Windows version
#ifndef _VGA_H

#define SM_ORIGINAL		-1
#define SM_320x200C     0
#define SM_320x200U     1
#define SM_320x240U     2
#define SM_360x200U     3
#define SM_360x240U     4
#define SM_376x282U     5
#define SM_320x400U     6
#define SM_320x480U     7
#define SM_360x400U     8
#define SM_360x480U     9
#define SM_360x360U     10
#define SM_376x308U     11
#define SM_376x564U     12
#define SM_640x400V     13
#define SM_640x480V     14
#define SM_800x600V     15
#define SM_1024x768V    16
#define SM_640x480V15   17
#define SM_800x600V15   18

#endif

#define SM95_320x200x8	0
#define SM95_320x200x8X	1
#define SM95_640x480x8	2
#define SM95_320x400x8	3
#define SM95_640x400x8	4 
#define SM95_800x600x8	5
#define SM95_1024x768x8	6
#define SM95_640x480x8P	7

// DDraw interface ------------------------------------------------------------

typedef struct DDMODEINFO {
	int w, h, bpp;
	int rw, rh;
	int emul					:1;				// Emulated Display Mode (Double Buffer/Single Page)
	int dbuf					:1;				// Double Buffered Duel Page Game Mode
	int modex				:1;				// Mode X Duel page mode. 
	int paged				:1;				// Duel Page Standard Mode
} DDMODEINFO;

typedef struct _dd_grs_canvas {
	LPDIRECTDRAWSURFACE lpdds;
	int lock_count;
	int sram;
	int xoff, yoff;
	grs_canvas canvas;
} dd_grs_canvas;

typedef struct _dd_caps {
	int hwcolorkey;
	int hwbltstretch;
	int hwbltqueue;

	struct {
		int sysmem;
	} offscreen;

} dd_caps;



extern LPDIRECTDRAW			_lpDD;				// Direct Draw Object
extern LPDIRECTDRAWSURFACE	_lpDDSPrimary;		// Primary Display Screen (Page 1)
extern LPDIRECTDRAWSURFACE	_lpDDSBack;			// Page 2 or offscreen canvas
extern LPDIRECTDRAWCLIPPER	_lpDDClipper;		// Window Clipper Object
extern LPDIRECTDRAWPALETTE	_lpDDPalette;		// Direct Draw Palette;
extern DDMODEINFO				_DDModeList[16];	// Possible display modes
extern int						_DDNumModes;		// Number of Display modes
extern BOOL						_DDFullScreen;		// Full Screen Mode?
extern BOOL						_DDExclusive;		// Is application exclusive?
extern int						_DDFlags;			// Direct Draw Screen Flags

extern LPDIRECTDRAWSURFACE _lpDDSMask;			// Cockpit Mask!

extern int 			WIN95_GR_current_mode;		// WIN95_GR_current mode of render.
extern int			W95DisplayMode;				// Current Display Mode


extern BOOL DDInit(int mode);
extern void DDKill();
extern BOOL DDCreateScreen();
extern void DDKillScreen();
extern ubyte *DDLockSurface(LPDIRECTDRAWSURFACE lpdds, RECT *rect, int *pitch);
extern void DDUnlockSurface(LPDIRECTDRAWSURFACE lpdds, char *data);
extern void DDSetDisplayMode(int display_mode,int flags);
extern void DDSetPalette(LPDIRECTDRAWPALETTE lpDDPal);
extern void DDFlip();
extern LPDIRECTDRAWSURFACE DDCreateSurface(int w, int h, int vram);
extern LPDIRECTDRAWSURFACE DDCreateSysMemSurface(int width, int height);
extern LPDIRECTDRAWPALETTE DDCreatePalette(ubyte *pal);
extern LPDIRECTDRAWPALETTE DDGetPalette(LPDIRECTDRAWSURFACE lpdds);
extern void DDFreeSurface(LPDIRECTDRAWSURFACE lpdds);
extern int DDCheckMode(int mode);


//	ddgr interface	-------------------------------------------------------------

#define DDGR_FULLSCREEN		1
#define DDGR_EXWINDOW		2
#define DDGR_WINDOW			3
#define DDGR_SCREEN_PAGING	1


extern dd_grs_canvas *dd_grd_screencanv;		// Primary Screen	Canvas
extern dd_grs_canvas *dd_grd_backcanv;			// Back Screen	Canvas
extern dd_grs_canvas *dd_grd_curcanv;			// Current Canvas
extern dd_caps	ddDriverCaps;						// Direct Draw Caps

extern void dd_gr_init();
extern void dd_gr_close();
extern void dd_gr_init_screen();
extern void dd_gr_screen_lock();
extern void dd_gr_screen_unlock();
extern void dd_gr_flip();

extern dd_grs_canvas *dd_gr_create_canvas(int w, int h);
extern dd_grs_canvas *dd_gr_create_sub_canvas(dd_grs_canvas *cvs, int x, int y, int w, int h);
extern void dd_gr_init_canvas(dd_grs_canvas *canv, int pixtype, int w, int h);
extern void dd_gr_free_sub_canvas(dd_grs_canvas *cvs);
extern void dd_gr_free_canvas(dd_grs_canvas *canvas);
extern void dd_gr_set_current_canvas(dd_grs_canvas *canvas);
extern void dd_gr_disable_current_canvas();
extern void dd_gr_init_sub_canvas(dd_grs_canvas *new, dd_grs_canvas *src, 
											int x, int y, int w, int h);
extern void dd_gr_clear_canvas(int color);
extern void dd_gr_reinit_canvas(dd_grs_canvas *canv);


//	dd_gr_blt functions
extern void dd_gr_blt_notrans(dd_grs_canvas *srccanv,
					int sx, int sy, int swidth, int sheight,
					dd_grs_canvas *destcanv, 
					int dx, int dy, int dwidth, int dheight);
extern void dd_gr_blt_display(dd_grs_canvas *srccanv,
					int sx, int sy, int swidth, int sheight,
					int dx, int dy, int dwidth, int dheight);
extern void dd_gr_blt_screen(dd_grs_canvas *srccanv,
					int sx, int sy, int swidth, int sheight,
					int dx, int dy, int dwidth, int dheight);

//	dd_gfx functions
extern int dd_gfx_init();
extern int dd_gfx_close();
extern int dd_gfx_resetbitmap2Dcache();
extern unsigned short dd_gfx_createbitmap2D(int w, int h);
extern int dd_gfx_loadbitmap2D(unsigned short handle, void *buf, int rleflag);
extern int dd_gfx_destroybitmap2D(unsigned short handle);
extern void dd_gfx_blt2D(unsigned short handle, int x, int y, int x2, int y2,
						fix u0, fix v0, fix u1, fix v1);



//	Macros --------------------------------------------------------------------

//	ddgr and gr hooks

#define DDSETDISPLAYMODE(c) (	W95DisplayMode = c )

#ifndef NDEBUG
#define DDGRLOCK(c) (dd_gr_lock_d(c, __FILE__, __LINE__));
#else
#define DDGRLOCK(c) (dd_gr_lock(c));
#endif
#define DDGRUNLOCK(c) (dd_gr_unlock(c));
#define DDGRRESTORE (dd_gr_restore_display());
#define DDGRSCREENLOCK ( dd_gr_screen_lock() );
#define DDGRSCREENUNLOCK ( dd_gr_screen_unlock());

#define GRMODEINFO(c) (_DDModeList[W95DisplayMode].c)


#endif
