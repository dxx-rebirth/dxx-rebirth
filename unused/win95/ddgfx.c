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
static char rcsid[] = "$Id: ddgfx.c,v 1.1.1.1 2001-01-19 03:30:15 bradleyb Exp $";
#pragma on (unreferenced)


#define WIN95
#define _WIN32
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdlib.h>
#include "win\ddraw.h"
#include "dd.h"


#include "fix.h"
#include "gr.h"
#include "rle.h"
#include "mono.h"
#include "error.h"
#include "args.h"
#include "winapp.h"


#define MAX_GFX_BITMAPS		512


// Data!

static BOOL dd_gfx_initialized = 0;
static unsigned short gfxBitmapHandleCur = 1;

static struct _gfxBitmap {
	LPDIRECTDRAWSURFACE lpdds;
	int w;
	int h;
} gfxBitmap[MAX_GFX_BITMAPS];


//	Functions

int dd_gfx_init()
{
	int i;
	
	if (dd_gfx_initialized) return 1;

	for (i = 0; i < MAX_GFX_BITMAPS; i++)
		gfxBitmap[i].lpdds = NULL;

	gfxBitmapHandleCur = 1;

	atexit(dd_gfx_close);


	if (FindArg("-disallowgfx")) dd_gfx_initialized = 0;
	else if (FindArg("-forcegfx")) dd_gfx_initialized = 1;
	else {
		if (ddDriverCaps.hwcolorkey) 
			logentry("Card supports HW colorkeying.\n");

		if (ddDriverCaps.hwbltstretch)
			logentry("Card supports HW bitmap stretching.\n");
		
		if (ddDriverCaps.hwcolorkey) dd_gfx_initialized = 1;
		else dd_gfx_initialized = 0;
	}

	return 0;
}


int dd_gfx_close()
{
	int i;

	if (!dd_gfx_initialized) return 1;

	for (i = 0; i < MAX_GFX_BITMAPS; i++)
	{
		if (gfxBitmap[i].lpdds)
			IDirectDrawSurface_Release(gfxBitmap[i].lpdds);
	}
	
	dd_gfx_initialized =0;

	return 0;
}


unsigned short dd_gfx_createbitmap2D(int w, int h)
{
	int i, force;
	unsigned short handle;

	if (!dd_gfx_initialized) return 0;

	for (i = 0; i < MAX_GFX_BITMAPS; i++)
		if (!gfxBitmap[i].lpdds) {
			handle = (unsigned short)(i);
			break;
		}

	if (i == MAX_GFX_BITMAPS) return 0;

//	Only do this if we can benefit from it.
//	if (ddDriverCaps.hwcolorkey && ddDriverCaps.hwbltstretch) 

	if (FindArg("-forcegfx")) 	force = 1;
	else force = 0;

	if (ddDriverCaps.hwcolorkey || force) 
	{
		LPDIRECTDRAWSURFACE lpdds;
		DDSURFACEDESC ddsd;
		HRESULT ddresult;
		DDCOLORKEY ddck;

		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
		ddsd.dwWidth = w;
		ddsd.dwHeight = h;

		ddresult = IDirectDraw_CreateSurface(_lpDD, &ddsd, &lpdds, NULL);

		if (ddresult != DD_OK) {
			mprintf((0, "DDGFX: Failed to create vidmem 2d bitmap (%x).\n", ddresult));
			return 0;
		}
      ddck.dwColorSpaceLowValue = TRANSPARENCY_COLOR;
      ddck.dwColorSpaceHighValue = TRANSPARENCY_COLOR;
      IDirectDrawSurface_SetColorKey(lpdds, DDCKEY_SRCBLT, &ddck);

		gfxBitmap[handle].lpdds = lpdds;
		gfxBitmap[handle].w = w;
		gfxBitmap[handle].h = h; 
		handle++;									// Make it a valid handle	
	}
	else handle = 0;

	return handle;
}	


int dd_gfx_loadbitmap2D(unsigned short handle, void *buf, int rleflag)
{
	HRESULT ddresult;
	DDSURFACEDESC ddsd;
	char *ptr;
	int pitch, i;
	grs_bitmap tbmp;

	Assert(handle != 0);

	if (!dd_gfx_initialized) return 0;

	handle--;									// Convert to valid handle

RedoLock:

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);

	ddresult = IDirectDrawSurface_Lock(gfxBitmap[handle].lpdds, NULL,	&ddsd, DDLOCK_WAIT, NULL);
	if (ddresult != DD_OK) {
		if (ddresult == DDERR_SURFACELOST) {
			ddresult = IDirectDrawSurface_Restore(gfxBitmap[handle].lpdds);
			if (ddresult != DD_OK) {
				if (ddresult != DDERR_WRONGMODE) {
					logentry("DDGFX::Restore::Surface err: %x\n", ddresult);		
					return 1;
				}
				else {
					LPDIRECTDRAWSURFACE lpdds;
					DDCOLORKEY ddck;

					IDirectDrawSurface_Release(lpdds);
					gfxBitmap[handle].lpdds = NULL;

					memset(&ddsd, 0, sizeof(ddsd));
					ddsd.dwSize = sizeof(ddsd);
					ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
					ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
					ddsd.dwWidth = gfxBitmap[handle].w;
					ddsd.dwHeight = gfxBitmap[handle].h;

					ddresult = IDirectDraw_CreateSurface(_lpDD, &ddsd, &lpdds, NULL);

					if (ddresult != DD_OK) {
						mprintf((0, "DDGFX: Failed to restore vidmem 2d bitmap.\n"));
						return 1;
					}
   				ddck.dwColorSpaceLowValue = TRANSPARENCY_COLOR;
   				ddck.dwColorSpaceHighValue = TRANSPARENCY_COLOR;
   				IDirectDrawSurface_SetColorKey(lpdds, DDCKEY_SRCBLT, &ddck);
					gfxBitmap[handle].lpdds = lpdds;
				}
			}
			
			goto RedoLock;
		}
		else {
			mprintf((0, "DDGFX:Unable to lock surface!!\n"));
			return 1;
		}
	}

//	Locked!
	ptr = ddsd.lpSurface;
	pitch = ddsd.lPitch;	

	gr_init_bitmap(&tbmp, BM_LINEAR, 0,0,gfxBitmap[handle].w, gfxBitmap[handle].h, gfxBitmap[handle].w, buf);
	if (rleflag) tbmp.bm_flags = BM_FLAG_RLE;

	for(i = 0; i < gfxBitmap[handle].h; i++) 
	{
		extern ubyte scale_rle_data[];
		if (rleflag) {
			decode_row(&tbmp,i);
			memcpy(ptr+(i*pitch), scale_rle_data, gfxBitmap[handle].w);
		} 
	  	else memcpy(ptr+(i*pitch), (ubyte*)(buf)+(i*gfxBitmap[handle].w), gfxBitmap[handle].w);
	}
	
	IDirectDrawSurface_Unlock(gfxBitmap[handle].lpdds, ptr);

//	Unlocked...

	return 0;
}

	
int dd_gfx_destroybitmap2D(unsigned short handle)
{
	if (!dd_gfx_initialized) return 0;

	if (handle == 0) return 1;
	handle--;									// Convert to valid handle

	if (gfxBitmap[handle].lpdds) {
		IDirectDrawSurface_Release(gfxBitmap[handle].lpdds);
		gfxBitmap[handle].lpdds = NULL;
		return 0;
	}
	else return 1;
}		


int dd_gfx_resetbitmap2Dcache()
{
	int i;

	if (!dd_gfx_initialized) return 0;

	for (i = 0; i < MAX_GFX_BITMAPS; i++)
		dd_gfx_destroybitmap2D(i+1);

	return 0;
}
		
		
void dd_gfx_blt2D(unsigned short handle, int x, int y, int x2, int y2,
						fix u0, fix v0, fix u1, fix v1)
{
	RECT drect, srect;

	if (!dd_gfx_initialized) return;

	Assert(handle != 0);
	handle--;									// Convert to valid handle

	SetRect(&drect, x,y,x2,y2);
	SetRect(&srect, f2i(u0), f2i(v0), f2i(u1), f2i(v1));

	IDirectDrawSurface_Blt(dd_grd_curcanv->lpdds, &drect,
					gfxBitmap[handle].lpdds, 
					&srect, DDBLT_WAIT| DDBLT_KEYSRC,
					NULL);
}			
