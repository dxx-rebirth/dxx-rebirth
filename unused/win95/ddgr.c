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


#pragma off (unreferenced)
static char rcsid[] = "$Id: ddgr.c,v 1.1.1.1 2001-01-19 03:30:15 bradleyb Exp $";
#pragma on (unreferenced)


#define WIN95
#define _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "ddraw.h"

#include <stdlib.h>
#include <mem.h>

#include "winapp.h"
#include "mem.h"
#include "error.h"
#include "gr.h"
#include "dd.h"
#include "mono.h"


//	Canvas Globals
//	----------------------------------------------------------------------------
int dd_gr_initialized = 0;

dd_grs_canvas *dd_grd_screencanv = NULL;
dd_grs_canvas *dd_grd_backcanv = NULL;
dd_grs_canvas *dd_grd_curcanv = NULL;

static int ddgr_atexit_called = 0;

extern int _DDLockCounter;
extern BOOL _DDSysMemSurfacing;
extern int W95OldDisplayMode;

extern RECT ViewportRect;  		// Superhack! (from descentw.c.  Too much trouble
											// to move!)


//	dd_gr_create_canvas
//	----------------------------------------------------------------------------
dd_grs_canvas *dd_gr_create_canvas(int w, int h)
{
	dd_grs_canvas *ddnew;
	grs_canvas *new;
	DDSCAPS ddsc;

	ddnew = (dd_grs_canvas *)malloc( sizeof(dd_grs_canvas) );
	ddnew->xoff = ddnew->yoff = 0;

	new = &ddnew->canvas;

	new->cv_bitmap.bm_x = 0;
	new->cv_bitmap.bm_y = 0;
	new->cv_bitmap.bm_w = w;
	new->cv_bitmap.bm_h = h;
	new->cv_bitmap.bm_flags = 0;
	new->cv_bitmap.bm_type = BM_LINEAR;
	new->cv_bitmap.bm_rowsize = 0;
	new->cv_bitmap.bm_data = NULL;

	new->cv_color = 0;
	new->cv_drawmode = 0;
	new->cv_font = NULL;
	new->cv_font_fg_color = 0;
	new->cv_font_bg_color = 0;

	ddnew->lpdds = DDCreateSurface(w, h, 0);
	if (!ddnew->lpdds) 
		Error("dd_gr_create_canvas: Unable to create DD Surface");
	ddnew->lock_count = 0;

	IDirectDrawSurface_GetCaps(ddnew->lpdds, &ddsc); 

	if (ddDriverCaps.offscreen.sysmem) ddnew->sram = 1;
	else ddnew->sram = 0;		

	return ddnew;
}


void dd_gr_init_canvas(dd_grs_canvas *canv, int pixtype, int w, int h)
{
	grs_canvas *new;
	DDSCAPS ddsc;

	canv->xoff = canv->yoff = 0;

	new = &canv->canvas;

	new->cv_bitmap.bm_x = 0;
	new->cv_bitmap.bm_y = 0;
	new->cv_bitmap.bm_w = w;
	new->cv_bitmap.bm_h = h;
	new->cv_bitmap.bm_flags = 0;
	new->cv_bitmap.bm_type = BM_LINEAR;
	new->cv_bitmap.bm_rowsize = 0;
	new->cv_bitmap.bm_data = NULL;

	new->cv_color = 0;
	new->cv_drawmode = 0;
	new->cv_font = NULL;
	new->cv_font_fg_color = 0;
	new->cv_font_bg_color = 0;

	canv->lpdds = DDCreateSurface(w,h,0);
	if (!canv->lpdds) 
		Error("dd_gr_create_canvas: Unable to create DD Surface");
		
	canv->lock_count = 0;
	IDirectDrawSurface_GetCaps(canv->lpdds, &ddsc);		

	if (ddsc.dwCaps & DDSCAPS_VIDEOMEMORY) canv->sram = 0;
	else if (ddDriverCaps.offscreen.sysmem) canv->sram = 1;
	else canv->sram = 0;		

}


void dd_gr_reinit_canvas(dd_grs_canvas *canv)
{
	grs_canvas *new;
	DDSURFACEDESC ddsd;
	LPDIRECTDRAWSURFACE lpdds;	

	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
	IDirectDrawSurface_GetSurfaceDesc(canv->lpdds, &ddsd);

	lpdds = canv->lpdds;

	memset(canv, 0, sizeof(dd_grs_canvas));
	canv->lpdds = lpdds;

	canv->lock_count = 0;
	canv->xoff = canv->yoff = 0;

	new = &canv->canvas;

	new->cv_bitmap.bm_x = 0;
	new->cv_bitmap.bm_y = 0;
	new->cv_bitmap.bm_w = ddsd.dwWidth;
	new->cv_bitmap.bm_h = ddsd.dwHeight;
	new->cv_bitmap.bm_flags = 0;
	new->cv_bitmap.bm_type = BM_LINEAR;
	new->cv_bitmap.bm_rowsize = 0;
	new->cv_bitmap.bm_data = NULL;

	new->cv_color = 0;
	new->cv_drawmode = 0;
	new->cv_font = NULL;
	new->cv_font_fg_color = 0;
	new->cv_font_bg_color = 0;
}


//	dd_gr_free_canvas
//	----------------------------------------------------------------------------
void dd_gr_free_canvas(dd_grs_canvas *canvas)
{
	if (canvas == dd_grd_curcanv) {	//bad!! freeing current canvas!
		DebugBreak();
		gr_set_current_canvas(NULL);
	}

	DDFreeSurface(canvas->lpdds);
	free(canvas);
}


//	dd_gr_create_sub_canvas
//	----------------------------------------------------------------------------
dd_grs_canvas *dd_gr_create_sub_canvas(dd_grs_canvas *cvs, 
									int x, int y, int w, int h)
{
   dd_grs_canvas *ddnew;
	grs_canvas *canv;
	grs_canvas *new;

	canv = &cvs->canvas;

	if (x+w > canv->cv_bitmap.bm_w) {Int3(); w=canv->cv_bitmap.bm_w-x;}
	if (y+h > canv->cv_bitmap.bm_h) {Int3(); h=canv->cv_bitmap.bm_h-y;}

   ddnew = (dd_grs_canvas *)malloc( sizeof(dd_grs_canvas) );
	ddnew->xoff = cvs->xoff;
	ddnew->yoff = cvs->yoff;
	ddnew->lock_count = 0;
	ddnew->lpdds = cvs->lpdds;
	new = &ddnew->canvas;

	new->cv_bitmap.bm_x = x+canv->cv_bitmap.bm_x;
	new->cv_bitmap.bm_y = y+canv->cv_bitmap.bm_y;
	new->cv_bitmap.bm_w = w;
	new->cv_bitmap.bm_h = h;
	new->cv_bitmap.bm_flags = 0;
	new->cv_bitmap.bm_type = canv->cv_bitmap.bm_type;
	new->cv_bitmap.bm_rowsize = 0;

	new->cv_bitmap.bm_data = 0;

	new->cv_color = canv->cv_color;
   new->cv_drawmode = canv->cv_drawmode;
   new->cv_font = canv->cv_font;
	new->cv_font_fg_color = canv->cv_font_fg_color;
	new->cv_font_bg_color = canv->cv_font_bg_color;

	ddnew->sram = cvs->sram;

   return ddnew;
}


//	dd_gr_free_sub_canvas
//	----------------------------------------------------------------------------
void dd_gr_free_sub_canvas(dd_grs_canvas *cvs)
{
	free(cvs);
}


//	dd_gr_hacks
//	----------------------------------------------------------------------------
void dd_gr_dup_hack(dd_grs_canvas *hacked, dd_grs_canvas *src)
{
	hacked->canvas.cv_bitmap.bm_data = src->canvas.cv_bitmap.bm_data;
	hacked->canvas.cv_bitmap.bm_rowsize = src->canvas.cv_bitmap.bm_rowsize;
}

void dd_gr_dup_unhack(dd_grs_canvas *hacked)
{
	hacked->canvas.cv_bitmap.bm_data = 0;
	hacked->canvas.cv_bitmap.bm_rowsize = 0;
}


//	dd_gr_init
//	----------------------------------------------------------------------------
void dd_gr_init()
{
	Assert(!dd_gr_initialized);

	if (!dd_grd_screencanv) 
		dd_grd_screencanv = (dd_grs_canvas *)malloc(sizeof(dd_grs_canvas));
	if (!dd_grd_backcanv) 
		dd_grd_backcanv = (dd_grs_canvas *)malloc(sizeof(dd_grs_canvas));
	

	dd_grd_screencanv->lpdds = NULL;
	dd_grd_backcanv->lpdds = NULL;	

	if (!ddgr_atexit_called) {
		atexit(dd_gr_close);
		ddgr_atexit_called = 1;
	}
	dd_gr_initialized = 1;
}


//	dd_gr_close
//	----------------------------------------------------------------------------
void dd_gr_close()
{
	Assert(dd_gr_initialized);

	if (dd_grd_screencanv) free(dd_grd_screencanv);
	if (dd_grd_backcanv) free(dd_grd_backcanv);

	dd_grd_screencanv = dd_grd_backcanv = NULL;
	dd_gr_initialized = 0;
}	


//	dd_gr_init_screen
//	----------------------------------------------------------------------------
void dd_gr_init_screen()
{
	grs_canvas *new;

// Define screen canvas
	if (dd_grd_screencanv->lpdds != NULL && !IDirectDrawSurface_IsLost(dd_grd_screencanv->lpdds)) {
		gr_palette_clear();
		dd_gr_set_current_canvas(NULL);
		dd_gr_clear_canvas(BM_XRGB(0,0,0));
		if (_DDModeList[W95OldDisplayMode].modex) {
			dd_gr_flip();
			dd_gr_clear_canvas(BM_XRGB(0,0,0));
			dd_gr_flip();
		}
	}

//	Capture the surface's palette.
	DDSetDisplayMode(W95DisplayMode, 0);
	grwin_set_winpalette(_lpDD, _lpDDPalette);
	gr_palette_clear();

	gr_init_screen(BM_LINEAR, 
					GRMODEINFO(rw), GRMODEINFO(rh), 
					0, 0, 0, NULL);
	
	dd_grd_screencanv->lock_count = 0;
	memcpy(&dd_grd_screencanv->canvas, &grd_curscreen->sc_canvas, sizeof(grs_canvas));

//	NEW!!!  
//	Scheme 1:
//		The 'Emulated' Method.
//		We will define the screen canvas as the Windows equiv to 
//		DOS display memory.   This will be our Direct Draw Back canvas.

	if (GRMODEINFO(emul) || GRMODEINFO(modex)) {
		dd_grd_screencanv->lpdds = _lpDDSBack;
		dd_grd_backcanv->lpdds = NULL;
		dd_grd_curcanv = NULL;
	}
	
//	Scheme 2:
//		The Page Flipping Full Screen Method
//		The screen canvas is the actual display
//		The back canvas is our scratch page
//		This does not apply to Mode X modes
	
	else if (GRMODEINFO(paged) && !GRMODEINFO(modex)) {
		dd_grd_screencanv->lpdds = _lpDDSPrimary;

		new = &dd_grd_backcanv->canvas;

		new->cv_bitmap.bm_x = 0;
		new->cv_bitmap.bm_y = 0;
		new->cv_bitmap.bm_w = _DDModeList[W95DisplayMode].rw;
		new->cv_bitmap.bm_h = _DDModeList[W95DisplayMode].rh;
		new->cv_bitmap.bm_flags = 0;
		new->cv_bitmap.bm_type = BM_LINEAR;
		new->cv_bitmap.bm_rowsize = 0;
		new->cv_bitmap.bm_data = NULL;

		new->cv_color = 0;
		new->cv_drawmode = 0;
		new->cv_font = NULL;
		new->cv_font_fg_color = 0;
		new->cv_font_bg_color = 0;
	
		dd_grd_backcanv->lpdds = _lpDDSBack;	
		dd_grd_backcanv->lock_count = 0;
		dd_grd_curcanv = NULL;
	}

//	Scheme 3:
//		No page flipping (just 1 page), and no use for back surface.

	else if (GRMODEINFO(dbuf) && !GRMODEINFO(paged)) {
		dd_grd_screencanv->lpdds = _lpDDSPrimary;
		dd_grd_backcanv->lpdds = NULL;
		dd_grd_curcanv = NULL;
	}

//	Bad Scheme

	else {
		Int3();									// An illegal hacked graphic mode
	}

	dd_gr_set_current_canvas(NULL);
	dd_gr_clear_canvas(BM_XRGB(0,0,0));
//	if (GRMODEINFO(modex)) {
//		dd_gr_flip();
//		dd_gr_clear_canvas(BM_XRGB(0,0,0));
//		dd_gr_flip();
//	}
}


int dd_gr_restore_canvas(dd_grs_canvas *canvas)
{
	if (!DDRestoreSurface(canvas->lpdds)) {
	//	Recreate surface
		DDFreeSurface(canvas->lpdds);
		if (canvas->sram) {
			canvas->lpdds = DDCreateSysMemSurface(canvas->canvas.cv_bitmap.bm_w,
											canvas->canvas.cv_bitmap.bm_h);
		}
		else {
			canvas->lpdds = DDCreateSurface(canvas->canvas.cv_bitmap.bm_w,
											canvas->canvas.cv_bitmap.bm_h, TRUE);
		}
 		if (!canvas->lpdds) return 0;
	}
	return 1;
}
	
	

//	dd_gr_screen_lock
//		copies dd_gr_screencanv to grd_curscreen->sc_canvas
//	----------------------------------------------------------------------------
void dd_gr_screen_lock()
{
	if (dd_grd_screencanv->lpdds == _lpDDSPrimary && GRMODEINFO(modex)) 
		Int3();									// Can't do this in ModeX!!
	dd_gr_lock(dd_grd_screencanv);
	memcpy(&grd_curscreen->sc_canvas, &dd_grd_screencanv->canvas, sizeof(grs_canvas)); 
}


//	dd_gr_screen_unlock
//		copies grd_curscreen->sc_canvas to dd_gr_screencanv
//	----------------------------------------------------------------------------
void dd_gr_screen_unlock()
{
	memcpy(&dd_grd_screencanv->canvas, &grd_curscreen->sc_canvas, sizeof(grs_canvas)); 
	dd_gr_unlock(dd_grd_screencanv);
}


void dd_gr_lock(dd_grs_canvas *canv)
{
	RECT rect;
	ubyte *data;
	grs_bitmap *bmp;
	int rowsize;

	if (canv->lock_count == 0) 
	{
		bmp = &canv->canvas.cv_bitmap;
		SetRect(&rect,bmp->bm_x,bmp->bm_y,bmp->bm_x+bmp->bm_w, bmp->bm_y+bmp->bm_h);

		if (!dd_gr_restore_canvas(canv)) 
			Error("Failed to lock canvas (restore err)!\n");

		data = DDLockSurface(canv->lpdds, &rect, &rowsize);
		canv->canvas.cv_bitmap.bm_rowsize = (short)rowsize;
		
		if (!data) 
			Error("Failed to lock canvas! You may need to use the -emul option.\n");

		canv->canvas.cv_bitmap.bm_data = data;

		if (canv == dd_grd_curcanv) {
			gr_set_current_canvas(&canv->canvas);
		}
	}	
	canv->lock_count++;
}


void dd_gr_lock_d(dd_grs_canvas *canv, char *filename, int line)
{
	RECT rect;
	ubyte *data;
	grs_bitmap *bmp;
	int rowsize;

	if (canv->lock_count == 0) 
	{
		bmp = &canv->canvas.cv_bitmap;
		SetRect(&rect,bmp->bm_x,bmp->bm_y,bmp->bm_x+bmp->bm_w, bmp->bm_y+bmp->bm_h);

		if (!dd_gr_restore_canvas(canv)) 
		#ifndef NDEBUG
			Error("Failed to lock canvas (restore err) (%s line %d)\n", filename, line);
		#else
			Error("Failed to lock canvas (restore err)!\n");
		#endif	
		
		data = DDLockSurface(canv->lpdds, &rect, &rowsize);
		canv->canvas.cv_bitmap.bm_rowsize = (short)rowsize;
		
		if (!data) 
		#ifndef NDEBUG
			Error("Failed to lock canvas (%s line %d)\n", filename, line);
		#else
			Error("Failed to lock canvas! You may ned to use the -emul option.\n");
		#endif	

		canv->canvas.cv_bitmap.bm_data = data;

		if (canv == dd_grd_curcanv) {
			gr_set_current_canvas(&canv->canvas);
		}
	}

	canv->lock_count++;
}


void dd_gr_unlock(dd_grs_canvas *canv)
{
	if (canv->lock_count == 1) 
	{
		DDUnlockSurface(canv->lpdds, canv->canvas.cv_bitmap.bm_data);
	}
	canv->lock_count--;
}


//	dd_gr_set_current_canvas
//		This function should do this:
//			set desired canvas to dd_grd_curcanv
//			call gr_set_current_canvas
//		
//		If desired canvas is NULL, then set current canvas to screen
//			set grd_curscreen->sc_canvas.cv_bitmap, etc to dd_grd_screencanv info
//			call gr_set_current_canvas(NULL)
//				this will use what is in grd_curscreen->etc to do the proper thing
//
//		Note: Must lock desired canvas before calling this.
//				This causes an address to be delivered to the canvas.
//				Else address is invalid and will crash.

void dd_gr_set_current_canvas(dd_grs_canvas *canvas)
{
	if (canvas == NULL) {
		dd_grd_curcanv = dd_grd_screencanv;
		gr_set_current_canvas(&dd_grd_screencanv->canvas);
	}
	else {
		dd_grd_curcanv = canvas;
		gr_set_current_canvas(&dd_grd_curcanv->canvas);
	}
}


//	dd_gr_init_sub_canvas
//		perform gr_init_sub_canvas.
//		same surface but reset lock count.
void dd_gr_init_sub_canvas(dd_grs_canvas *new, dd_grs_canvas *src, 
									int x, int y, int w, int h)
{
	gr_init_sub_canvas(&new->canvas, &src->canvas, x, y, w, h);
	new->xoff = src->xoff;
	new->yoff = src->yoff;
	new->lpdds = src->lpdds;
	new->lock_count = 0;
	new->sram = src->sram;
}


//	dd_gr_flip()
//		performs a general 'flip' of canvases.
//		If we are in a slow display mode, we will copy the screen canvas
//		to the display
//		If we are in a FullScreen mode, then we will just perform a
//		Direct Draw Flip.
void dd_gr_flip()
{
	if (GRMODEINFO(emul)) {
		dd_gr_blt_display(dd_grd_screencanv, 
						0,0,0,0,	
						ViewportRect.left, ViewportRect.top, 
						ViewportRect.right-ViewportRect.left, 
						ViewportRect.bottom-ViewportRect.top);
	}
	else if (_DDFullScreen) {
		DDFlip();
	}
	else {			 
		Int3();									// Illegal display mode!
	}
}


//	dd_gr_restore_display
//		blts the screen canvas to the display 
//		(for Slow modes which emulate a DOS display)
void dd_gr_restore_display()
{
	if (GRMODEINFO(emul)) {
	//	We use a offscreen buffer as grd_screencanv, so just
	// blt this to the display

		IDirectDrawSurface_Blt(_lpDDSPrimary,
					NULL,
					dd_grd_screencanv->lpdds,
					NULL,
					DDBLT_WAIT,
					NULL);
	}
	else if (GRMODEINFO(modex)) {
//		dd_gr_flip();
	}
}


//	DD Blt Functions
//
//	dd_gr_blt_notrans
//		blts one canvas region to another canvas with scaling and
//		no color keying
void dd_gr_blt_notrans(dd_grs_canvas *srccanv,
					int sx, int sy, int swidth, int sheight,
					dd_grs_canvas *destcanv, 
					int dx, int dy, int dwidth, int dheight)
{
	RECT srect, drect;
	RECT *psrect, *pdrect;
	grs_bitmap *sbmp, *dbmp;

	psrect = &srect;
	pdrect = &drect;
	sbmp = &srccanv->canvas.cv_bitmap;
	dbmp = &destcanv->canvas.cv_bitmap;

	if (swidth || sheight) {
		SetRect(psrect, sx+sbmp->bm_x, sy+sbmp->bm_y, sx+sbmp->bm_x+swidth, 
					sy+sbmp->bm_y+sheight);
	}
	else {
		SetRect(psrect, sbmp->bm_x, sbmp->bm_y, sbmp->bm_w+sbmp->bm_x,
							sbmp->bm_h+sbmp->bm_y);
	}

	if (dwidth || dheight) {
		SetRect(pdrect, dx+dbmp->bm_x, dy+dbmp->bm_y, dx+dbmp->bm_x+dwidth, 
					dy+dbmp->bm_y+dheight);
	}
	else {
		SetRect(pdrect, dbmp->bm_x, dbmp->bm_y, dbmp->bm_x+dbmp->bm_w, 
					dbmp->bm_y+dbmp->bm_h);
	}

	if (_DDFullScreen && !GRMODEINFO(emul)) {
		IDirectDrawSurface_BltFast(destcanv->lpdds, dx+dbmp->bm_x, dy+dbmp->bm_y,		
							srccanv->lpdds, psrect, 
							DDBLTFAST_NOCOLORKEY | DDBLTFAST_WAIT);
	}		
	else {
		IDirectDrawSurface_Blt(destcanv->lpdds,
					pdrect,
					srccanv->lpdds,
					psrect,
					DDBLT_WAIT,
					NULL);
	}
}


//	dd_gr_blt_display	 (no keying)
//		blts a canvas region to the display.
//		for windowed modes, this is the only way to blt
//		a canvas to the display.
void dd_gr_blt_display(dd_grs_canvas *srccanv,
					int sx, int sy, int swidth, int sheight,
					int dx, int dy, int dwidth, int dheight)
{
	RECT srect, drect;
	RECT *psrect, *pdrect;
	HRESULT result;
	grs_bitmap *sbmp;

	if (srccanv->lpdds == _lpDDSPrimary) {
		Int3();									// This will crash the system
	}


	psrect = &srect;
	pdrect = &drect;
	sbmp = &srccanv->canvas.cv_bitmap;

	if (swidth || sheight) {
		SetRect(psrect, sx+sbmp->bm_x, sy+sbmp->bm_y, sx+sbmp->bm_x+swidth-1, 
					sy+sbmp->bm_y+sheight-1);
	}
	else {
		SetRect(psrect, sbmp->bm_x, sbmp->bm_y, sbmp->bm_w+sbmp->bm_x-1,
							sbmp->bm_h+sbmp->bm_y-1);
	}

	if (dwidth || dheight) {
		SetRect(pdrect, dx, dy, dx+dwidth-1, dy+dheight-1);
	}
	else pdrect = NULL;

//	We are blting to the display which is _lpDDSPrimary.
	result = IDirectDrawSurface_Blt(_lpDDSPrimary,
					pdrect,
					srccanv->lpdds,
					psrect,
					DDBLT_WAIT,
					NULL);
	if (result != DD_OK) 
		Error("DDERR: Blt: (%d)\n", (result & 0x0000ffff));
}


//	dd_gr_blt_screen	 (no keying)
//		blts canvas to screen canvas
void dd_gr_blt_screen(dd_grs_canvas *srccanv,
					int sx, int sy, int swidth, int sheight,
					int dx, int dy, int dwidth, int dheight)
{
	dd_gr_blt_notrans(srccanv, sx, sy, swidth, sheight,
							dd_grd_screencanv, dx, dy, dwidth, dheight);
}


//	dd_gr_clear_canvas
//		clears a canvas the 'fast' way.
void dd_gr_clear_canvas(int color)
{
    DDBLTFX     ddbltfx;
    HRESULT     ddresult;
	 RECT 		 drect;
	 grs_bitmap	 *bmp;

	 bmp = &dd_grd_curcanv->canvas.cv_bitmap;

	 UpdateWindow(GetLibraryWindow());

    ddbltfx.dwSize = sizeof( ddbltfx );
    ddbltfx.dwFillColor = (DWORD)color;

    Assert(_DDLockCounter == 0);

	 SetRect(&drect, bmp->bm_x, bmp->bm_y, bmp->bm_x+bmp->bm_w, 
					bmp->bm_y+bmp->bm_h);

    ddresult = IDirectDrawSurface_Blt(
                            dd_grd_curcanv->lpdds,  // dest surface
                            &drect,                 // dest rect
                            NULL,                   // src surface
                            NULL,                   // src rect
                            DDBLT_COLORFILL | DDBLT_WAIT,
                            &ddbltfx);
	 if (ddresult != DD_OK) {
		if (ddresult == DDERR_SURFACELOST) {
			if (!dd_gr_restore_canvas(dd_grd_curcanv))
				Error("Direct Draw GR library Blt Clear Restore error.");
		}		
		else Error("Direct Draw GR library Blt Clear error: %x", ddresult);
	 }

	if (!_DDFullScreen) {
		Assert(_DDLockCounter == 0);		
		DDGRRESTORE;
	}
}











