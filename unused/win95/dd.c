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
static char rcsid[] = "$Id: dd.c,v 1.1.1.1 2001-01-19 03:30:15 bradleyb Exp $";
#pragma on (unreferenced)



#define _WIN32
#define WIN95
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <windowsx.h>

#include <stdio.h>

#include "ddraw.h"

#include "pstypes.h"
#include "mono.h"
#include "args.h"
#include "error.h"
#include "winapp.h"
#include "dd.h"



//	Globals
//	----------------------------------------------------------------------------

LPDIRECTDRAW				_lpDD=0;				// Direct Draw Object

LPDIRECTDRAWSURFACE		_lpDDSPrimary=0;	//	Page 0 and 1
LPDIRECTDRAWSURFACE		_lpDDSBack=0;		//

LPDIRECTDRAWCLIPPER		_lpDDClipper=0;	// Window clipper Object
LPDIRECTDRAWPALETTE		_lpDDPalette=0;	// Palette object

BOOL							_DDFullScreen;		// Game is in window, or fullscreen
BOOL							_DDSysMemSurfacing=TRUE;

int							_DDNumModes=0;		// Number of graphic modes
DDMODEINFO					_DDModeList[16];	// Mode information list
int							_DDLockCounter=0;	// DirectDraw Lock Surface counter


int							W95DisplayMode, 	// Current display mode from list
								W95OldDisplayMode;//	Old display smode

dd_caps						ddDriverCaps;		// Direct Draw Caps?

static HWND					DDWnd=NULL;			// Current window that owns the display
static FILE					*LogFile=NULL;
int					DD_Emulation=0;	// Do we force emulated modes?

RECT					ViewportRect;					// Current Viewport


//	Function Prototypes
//	----------------------------------------------------------------------------

HRESULT CALLBACK EnumDispModesCB(LPDDSURFACEDESC lpddsd, LPVOID context);
BOOL CheckDDResult(HRESULT ddresult, char *funcname);
void DDEmulateTest(void);

#define WRITELOG(t) if (LogFile) { fprintf t; fflush(LogFile);}



//	Initialization
//	----------------------------------------------------------------------------

BOOL DDInit(int mode)
{
	LPDIRECTDRAW lpdd;
	HRESULT ddresult;
	DWORD flags;

#ifndef NDEBUG
	if (FindArg("-logfile")) LogFile = fopen("dd.log", "wt");
	else LogFile = NULL;
#endif	

//	Create Direct Draw Object (Use Emulation if Hardware is off)
	if (!_lpDD)	{
		{
			ddresult = DirectDrawCreate(NULL, &lpdd, NULL);
		
			if (ddresult == DDERR_NODIRECTDRAWHW) {
				if (FindArg("-ddemul")) {
	      	   ddresult = DirectDrawCreate( (LPVOID) DDCREATE_EMULATIONONLY, &lpdd, NULL );
					if (!CheckDDResult(ddresult, "InitDD:DirectDrawCreate emulation"))
						return FALSE;
					DD_Emulation = 1;
					logentry("DirectDraw: forcing emulation.\n");
				}
				else {
					CheckDDResult(ddresult, "InitDD:DirectDrawCreate noemulation");
					return FALSE;
				}
			}
			else if (ddresult != DD_OK) 
				return FALSE;
		}
		WRITELOG((LogFile, "System initiated.\n"));
	}
	else return TRUE;

	DDWnd = GetLibraryWindow();
	_lpDD = lpdd;

	atexit(DDKill);

//	Set cooperative mode
	if (mode == DDGR_FULLSCREEN) {
		flags = DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE;

	#ifndef NDEBUG
		if (!FindArg("-nomodex")) flags |= DDSCL_ALLOWMODEX;
	#else
		flags |= DDSCL_ALLOWMODEX;
	#endif

		if (!FindArg("-disallowreboot")) flags |= DDSCL_ALLOWREBOOT;
 
		ddresult = IDirectDraw_SetCooperativeLevel(lpdd, DDWnd, flags); 

		if (!CheckDDResult(ddresult, "DDInit::SetCooperativeLevel")) {	
			IDirectDraw_Release(lpdd);
			return FALSE;
		}

		_DDFullScreen = TRUE;			
	}
	else if (mode == DDGR_WINDOW) {
		ddresult = IDirectDraw_SetCooperativeLevel(lpdd, DDWnd,
										DDSCL_NORMAL);
		if (!CheckDDResult(ddresult, "DDInit::SetCooperativeLevel"))
			return FALSE;
		_DDFullScreen = FALSE;
		if (!DDInitClipper()) return FALSE;
	}
	else return FALSE;

// Perform Emulation test
	DDEmulateTest();

//	Direct draw initialized.  Begin post-initialization
	if (!DDEnumerateModes()) return FALSE;
//	DDSETDISPLAYMODE(SM95_640x480x8);
//	DDSetDisplayMode(W95DisplayMode, 0);

	return TRUE;
}


void DDEmulateTest(void)
{
// Assume DirectDraw is init and we have exclusive access
	HRESULT ddresult;
	RECT rect;

	DD_Emulation = 0;

	if (FindArg("-emul")) {
		DD_Emulation = 1;
		return;
	}

	ddresult = IDirectDraw_SetDisplayMode(_lpDD, 640,480,8);
	if (ddresult != DD_OK) {
		DD_Emulation = 1;
		return;
	}
	else {
		DDSURFACEDESC ddsd;
		LPDIRECTDRAWSURFACE lpdds;

		ZeroMemory(&ddsd, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
							
		ddresult = IDirectDraw_CreateSurface(_lpDD, &ddsd, &lpdds, NULL);
		if (ddresult != DD_OK) {
			DD_Emulation = 1;
			return;
		}		

	// Performing lock test!
		ZeroMemory(&ddsd, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		SetRect(&rect, 10,10,584,200);
		ddresult = IDirectDrawSurface_Lock(lpdds, &rect, &ddsd, DDLOCK_WAIT, NULL);
		if (ddresult != DD_OK) {
			IDirectDrawSurface_Release(lpdds);
			DD_Emulation = 1;
			return;
		}
		else {
			IDirectDrawSurface_Unlock(lpdds, ddsd.lpSurface);
			IDirectDrawSurface_Release(lpdds);
			DD_Emulation = 0;
		}
		
	// Cleanup
//		IDirectDrawSurface_Unlock(lpdds, ddsd.lpSurface);
//		IDirectDrawSurface_Release(lpdds);
	}
}

	
BOOL DDEnumerateModes(void)
{
	HRESULT ddresult;
	int i;

//	Invalidate all modes
	for (i = 0; i < 16; i++)
	{
		_DDModeList[i].rw = _DDModeList[i].w = -1;
		_DDModeList[i].rh = _DDModeList[i].h = -1;
	}

//	If we're full screen, enumerate modes.
	if (_DDFullScreen) {
		ddresult = IDirectDraw_EnumDisplayModes(_lpDD, 0, NULL, 0, 
									EnumDispModesCB);
		if(!CheckDDResult(ddresult, "DDInit::EnumDisplayModes")) {
			IDirectDraw_Release(_lpDD);
			return FALSE;
		}
	}
	else {
		_DDModeList[SM95_640x480x8].rw = 640;
		_DDModeList[SM95_640x480x8].rh = 480;
		_DDModeList[SM95_640x480x8].w = 640;
		_DDModeList[SM95_640x480x8].h = 480;
		_DDModeList[SM95_640x480x8].emul = 1;
		_DDModeList[SM95_640x480x8].dbuf = 0;
		_DDModeList[SM95_640x480x8].modex = 0;
		_DDModeList[SM95_640x480x8].paged = 0; 
	}
	 
	return TRUE;
}				


BOOL DDInitClipper(void) 
{
	return FALSE;
}


//	Destruction
//	----------------------------------------------------------------------------

void DDKill(void)
{
	DDKillScreen();

//	if (_lpDDPalette) IDirectDrawPalette_Release(_lpDDPalette);
	if (_lpDDClipper) IDirectDrawClipper_Release(_lpDDClipper);
	if (_lpDD) IDirectDraw_Release(_lpDD);

	_DDFullScreen = FALSE;
	_DDNumModes = _DDLockCounter = 0;
	_lpDDClipper = NULL;
	_lpDD = NULL;

	WRITELOG((LogFile, "System closed.\n"));

	if (LogFile) fclose(LogFile);
}


//	Screen functions
//	----------------------------------------------------------------------------

void DDSetDisplayMode(int display_mode, int flags)
{
	HRESULT ddresult;

	W95DisplayMode = display_mode;

	if (_DDFullScreen) {
	// Change literal display mode of screen
		DDKillScreen();
		WRITELOG((LogFile, "Setting display mode to (%dx%dx%d).\n", GRMODEINFO(w), GRMODEINFO(h), GRMODEINFO(bpp)));		

		ddresult = IDirectDraw_SetDisplayMode(_lpDD, 
							_DDModeList[W95DisplayMode].w, 
							_DDModeList[W95DisplayMode].h,
							_DDModeList[W95DisplayMode].bpp);

		if (!CheckDDResult(ddresult, "DDInit::SetDisplayMode")) {
			Error("Unable to set display mode: %d [%dx%dx%d].( Err: %x ) \n", W95DisplayMode, _DDModeList[W95DisplayMode].w, 
							_DDModeList[W95DisplayMode].h,
							_DDModeList[W95DisplayMode].bpp,
							ddresult);
		}
		if (!DDCreateScreen()) exit(1);
		Sleep(1000);			
	}
	else {
		Error("Windowed display modes not currently supported.");
	}

	W95OldDisplayMode = display_mode;
}


void DDResizeViewport()
{
	RECT rect;
	POINT p1, p2;

	GetClientRect(GetLibraryWindow(), &rect);
	p1.x = rect.left;
	p1.y = rect.top;
	p2.x = rect.right;
	p2.y = rect.bottom; 
	ClientToScreen(GetLibraryWindow(), &p1);
	ClientToScreen(GetLibraryWindow(), &p2);

	if (_DDFullScreen)  {
		p1.x = p1.y = 0;
		p2.x = _DDModeList[W95DisplayMode].w;
		p2.y = _DDModeList[W95DisplayMode].h;
	}

	ViewportRect.left = p1.x;
	ViewportRect.top = p1.y;
	ViewportRect.right = p2.x;
	ViewportRect.bottom = p2.y;

	mprintf((0, "New Viewport: [%d,%d,%d,%d]\n", p1.x, p1.y, p2.x, p2.y));
}


BOOL DDCreateScreen(void)
{
	DDSCAPS 			ddscaps;
	DDCAPS  			ddcaps, ddcaps2;
	DDSURFACEDESC 	ddsd;
	HRESULT 			ddresult;

//	Determine capabilites of this display mode
	memset(&ddcaps, 0, sizeof(ddcaps));
	memset(&ddcaps2, 0, sizeof(ddcaps2));
	ddcaps.dwSize = sizeof(ddcaps);
	ddcaps2.dwSize = sizeof(ddcaps);

	ddresult = IDirectDraw_GetCaps(_lpDD, &ddcaps, &ddcaps2);
	if (!CheckDDResult(ddresult, "DDCreateScreen::GetCaps"))
		return FALSE;

	WRITELOG((LogFile, "HAL: %8x\tHEL: %8x\n", ddcaps.dwCaps, ddcaps2.dwCaps));
	WRITELOG((LogFile, "PAL: %8x\tDDS: %8x\n", ddcaps.dwPalCaps, ddcaps.ddsCaps.dwCaps));

//	This makes the game plenty faster (3 fps extra on Highres)	
	ddDriverCaps.offscreen.sysmem = 1;

	if (ddcaps.dwCaps	& DDCAPS_COLORKEYHWASSIST) {
		WRITELOG((LogFile, "Mode supports hardware colorkeying.\n"));
		ddDriverCaps.hwcolorkey = 1;
	}
	else ddDriverCaps.hwcolorkey = 0;

	if (ddcaps.dwCaps & DDCAPS_BLTSTRETCH) {
		WRITELOG((LogFile, "Mode supports hardware blt stretching.\n"));
		ddDriverCaps.hwbltstretch = 1;
	}
	else ddDriverCaps.hwbltstretch = 0;


//	Create the screen surfaces
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	_lpDDSBack = NULL;

	if (GRMODEINFO(paged)) 
	{
	//	Create a flipping surface if we can

		ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
		ddsd.dwBackBufferCount = 1;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
							
//		ddsd.ddpfPixelFormat.dwFlags = DDPF_PALETTEINDEXED8 | DDPF_RGB;
		ddresult = IDirectDraw_CreateSurface(_lpDD, &ddsd, &_lpDDSPrimary, NULL);
		if (!CheckDDResult(ddresult, "DDCreateScreen::CreateSurface -paged"))
			return FALSE;
	
		ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
		ddresult = IDirectDrawSurface_GetAttachedSurface(_lpDDSPrimary,
													&ddscaps, &_lpDDSBack);
		if (!CheckDDResult(ddresult, "DDCreateScreen::GetAttachedSurface"))
			return FALSE;
	}
	else 
	{
	// Normal primary surface (non-emulated)
		ddsd.dwFlags = DDSD_CAPS;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
//		ddsd.ddpfPixelFormat.dwFlags = DDPF_PALETTEINDEXED8 | DDPF_RGB;
		ddresult = IDirectDraw_CreateSurface(_lpDD, &ddsd, &_lpDDSPrimary, NULL);
		if (!CheckDDResult(ddresult, "DDCreateScreen::CreateSurface"))
			return FALSE;
	}
	
	if (GRMODEINFO(emul)) 
	{
	//	Create emulated 2nd page for window modes that don't let us render to the 
	//	screen directly.
		_lpDDSBack = DDCreateSurface(_DDModeList[W95DisplayMode].rw, 
										_DDModeList[W95DisplayMode].rh, 1);
		if (!_lpDDSBack) {
			mprintf((0,"Call to create DDSBackBuffer failed."));
			return FALSE;
		}
	}

	if (GRMODEINFO(bpp) == 8) 
	{
	//	for 8-bit palettized modes, assert that we use the RGB pixel format.	
	// Also create a base palette for this 8-bit mode.
		ubyte pal[768];
		memset(pal, 0, 768);
		
		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		IDirectDrawSurface_GetSurfaceDesc(_lpDDSPrimary, &ddsd);

		WRITELOG((LogFile, "Surface pixel format: %x, %d\n", ddsd.ddpfPixelFormat.dwFlags, ddsd.ddpfPixelFormat.dwRGBBitCount)); 

		if ((_lpDDPalette = DDCreatePalette(pal))!= NULL) 
			DDSetPalette(_lpDDPalette);
		else Error("Failed to create palette for screen.\n");
	}

	return TRUE;
}


void DDClearDisplay();

void DDKillScreen()
{
	WRITELOG((LogFile, "Killing display mode (%dx%dx%d).\n", _DDModeList[W95OldDisplayMode].w, _DDModeList[W95OldDisplayMode].h, _DDModeList[W95OldDisplayMode].bpp));

//	if (_lpDDPalette) IDirectDrawPalette_Release(_lpDDPalette);
	if (_lpDDSBack && !GRMODEINFO(paged)) IDirectDrawSurface_Release(_lpDDSBack);
	if (_lpDDSPrimary) {

		if (!GRMODEINFO(modex)) {
			DDClearDisplay();
		}

		IDirectDrawSurface_Release(_lpDDSPrimary);
 
	}

	_lpDDPalette = NULL;
	_lpDDSBack = NULL;
	_lpDDSPrimary = NULL;
}


void DDClearDisplay()
{
	DDBLTFX ddbltfx;
	HRESULT ddresult;

	memset(&ddbltfx, 0, sizeof(DDBLTFX));
	ddbltfx.dwSize = sizeof( ddbltfx );
	ddbltfx.dwFillColor = (WORD)(BM_XRGB(0,0,0));

	ddresult = IDirectDrawSurface_Blt(
                     _lpDDSPrimary,  			// dest surface
                     NULL,                 	// dest rect
                     NULL,                  // src surface
                     NULL,                  // src rect
                     DDBLT_COLORFILL | DDBLT_WAIT,
                     &ddbltfx);
}


void DDFlip()
{
	HRESULT ddresult;

	if (IDirectDrawSurface_GetFlipStatus(_lpDDSBack, DDGFS_ISFLIPDONE) == DDERR_WASSTILLDRAWING) return;

	ddresult = IDirectDrawSurface_Flip(_lpDDSPrimary, NULL, DDFLIP_WAIT);
	if (ddresult != DD_OK) {
		WRITELOG((LogFile, "DDFlip:: Unable to flip screen (%X)\n", ddresult));
		Int3();									// Bad flipping
	}
}



//	DirectDrawSurface Routines
//	----------------------------------------------------------------------------

LPDIRECTDRAWSURFACE DDCreateSurface(int width, int height, BOOL vram)
{
	DDSURFACEDESC ddsd;
	HRESULT ddresult;
	LPDIRECTDRAWSURFACE lpdds;

	if (ddDriverCaps.offscreen.sysmem && !vram) 
		return DDCreateSysMemSurface(width, height);

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
	ddsd.dwWidth = width;
	ddsd.dwHeight = height;

	ddresult = IDirectDraw_CreateSurface(_lpDD, &ddsd, &lpdds, NULL);
	if (ddresult != DD_OK) {
		WRITELOG((LogFile, "CreateSurface err: %x\n", ddresult));
		return NULL;
	}

	return lpdds;
}


LPDIRECTDRAWSURFACE DDCreateSysMemSurface(int width, int height)
{
	DDSURFACEDESC ddsd;
	HRESULT ddresult;
	LPDIRECTDRAWSURFACE lpdds;

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
	ddsd.dwWidth = width;
	ddsd.dwHeight = height;

	ddresult = IDirectDraw_CreateSurface(_lpDD, &ddsd, &lpdds, NULL);
	if (ddresult != DD_OK) {
		logentry("DDRAW::CreateSysMemSurface err: %x\n", ddresult);		
		return NULL;
	}

	return lpdds;
}


void DDFreeSurface(LPDIRECTDRAWSURFACE lpdds)
{
	HRESULT ddresult;

	Assert(lpdds != NULL);

	ddresult = IDirectDrawSurface_Release(lpdds);
	if (ddresult != DD_OK) {
		Error("DDFreeSurface: Unable to free surface: err %x", ddresult);
	}
}


BOOL DDRestoreSurface(LPDIRECTDRAWSURFACE lpdds)
{
	HRESULT ddresult;
	
	Assert(lpdds != NULL);
	if (IDirectDrawSurface_IsLost(lpdds) == DD_OK) return TRUE;	
	ddresult = IDirectDrawSurface_Restore(lpdds);
	if (ddresult != DD_OK) 
		return FALSE;

	return TRUE;
}



//	Locking and Unlocking of DD Surfaces

ubyte *DDLockSurface(LPDIRECTDRAWSURFACE lpdds, RECT *rect, int *pitch )
{
	HRESULT ddresult;
	DDSURFACEDESC ddsd;
 	int try_count = 0;

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);

RetryLock:

	ddresult = IDirectDrawSurface_Lock(lpdds, rect, &ddsd, DDLOCK_WAIT, NULL);

	if (ddresult != DD_OK) 
	{
		if (ddresult == DDERR_SURFACELOST) {
			if (!DDRestoreSurface(lpdds) || try_count) {
				WRITELOG((LogFile, "Unable to restore surface for lock.\n"));
				return NULL;
			}
			else {							 
				try_count++;
				goto RetryLock;
			}
		}
		else {
			WRITELOG((LogFile, "Lock error: %x.\n", ddresult));
			return NULL;
		}
	}
	
	_DDLockCounter++;
			
	*pitch = (int)ddsd.lPitch;
	return (ubyte *)ddsd.lpSurface;	
}


void DDUnlockSurface(LPDIRECTDRAWSURFACE lpdds, char *data)
{
	HRESULT ddresult;
	
	if (_DDLockCounter < 1) return;

	ddresult = IDirectDrawSurface_Unlock(lpdds, data);
	if (ddresult != DD_OK) {	
		Error("Unable to unlock canvas: %x\n", ddresult);
	}

	_DDLockCounter--;
}



//	DirectDrawPalette Utilities
//	----------------------------------------------------------------------------

LPDIRECTDRAWPALETTE DDGetPalette(LPDIRECTDRAWSURFACE lpdds)
{
	HRESULT ddresult;
	LPDIRECTDRAWPALETTE lpddp;

	ddresult = IDirectDrawSurface_GetPalette(lpdds, &lpddp);
	if (ddresult != DD_OK) {
		mprintf((1, "DDERR: GetPalette %x.\n", ddresult));
		return NULL;
	}
	return lpddp;
}
	

LPDIRECTDRAWPALETTE DDCreatePalette(ubyte *pal)
{
	HRESULT ddresult;
	LPDIRECTDRAWPALETTE lpddpal;
	PALETTEENTRY pe[256];
	int i;

	for (i = 0; i < 256; i++)
	{
		pe[i].peRed = pal[i*3];
		pe[i].peGreen = pal[i*3+1];
		pe[i].peBlue = pal[i*3+2];
		pe[i].peFlags = 0;
	}

	ddresult = IDirectDraw_CreatePalette(_lpDD, 
								DDPCAPS_8BIT | DDPCAPS_ALLOW256 | DDPCAPS_INITIALIZE, 
								pe, 
								&lpddpal, NULL);
	if (ddresult != DD_OK) {
		mprintf((1, "DDERR: CreatePalette %x.\n", ddresult));
		return NULL;
	}
	
	return lpddpal;
}


void DDSetPalette(LPDIRECTDRAWPALETTE lpDDPal)
{
	HRESULT ddresult;

	ddresult = IDirectDrawSurface_SetPalette(_lpDDSPrimary, lpDDPal);
	if (ddresult != DD_OK) {
		if (ddresult == DDERR_SURFACELOST) {
			IDirectDrawSurface_Restore(_lpDDSPrimary);
			IDirectDrawSurface_SetPalette(_lpDDSPrimary, lpDDPal);
		}
		else {
			Error("This driver requires you to set your desktop\n to 256 color mode before running Descent II.");
		}
	}
}



//	Utilities
//	----------------------------------------------------------------------------

int DDCheckMode(int mode)
{
	if (_DDModeList[mode].w==-1 && _DDModeList[mode].h==-1) return 1;
	else return 0;
}


BOOL CheckDDResult(HRESULT ddresult, char *funcname)
{
	char buf[256];

	if (ddresult != DD_OK) {
		sprintf(buf, "DirectDraw error %x detected in\r\n\t%s\n", ddresult, funcname);
		WRITELOG((LogFile, buf));
		MessageBox(NULL, buf, "DESCENT2::DDRAW", MB_OK);
		return FALSE;
	}
	else return TRUE;
}



//	EnumDispModesCB
//	----------------------------------------------------------------------------
HRESULT CALLBACK EnumDispModesCB(LPDDSURFACEDESC lpddsd, LPVOID context)
{
	DWORD width, height,bpp;
	int mode;
	DWORD modex;	

	width = lpddsd->dwWidth;
	height = lpddsd->dwHeight;
	bpp = lpddsd->ddpfPixelFormat.dwRGBBitCount;
	modex = lpddsd->ddsCaps.dwCaps;

	modex = modex & DDSCAPS_MODEX;

	if (width == 640 && height == 480 && bpp==8)   
		mode = SM95_640x480x8;
	else if (width == 640 && height == 400 && bpp==8)
		mode = SM95_640x400x8;
	else if (width == 320 && height == 200 && bpp==8) {
		mode = SM95_320x200x8X;
		if (DD_Emulation) return DDENUMRET_OK;
	}
	else if (width == 800 && height == 600 && bpp==8) 
		mode = SM95_800x600x8;
	else if (width == 1024 && height == 768 && bpp==8)
		mode = SM95_1024x768x8;
	else
		return DDENUMRET_OK;

	_DDModeList[mode].rw 		= width;
	_DDModeList[mode].rh 		= height;
	_DDModeList[mode].emul 		= 0;
	_DDModeList[mode].modex		= 0;
	_DDModeList[mode].paged		= 0;
	_DDModeList[mode].dbuf		= 0;
   _DDModeList[mode].w   		= width;
   _DDModeList[mode].h   		= height;
   _DDModeList[mode].bpp 		= bpp;

	if (mode == SM95_320x200x8X) {
		_DDModeList[mode].modex = 1;
		_DDModeList[mode].dbuf = 1;
		_DDModeList[mode].paged = 1;
	}
	else _DDModeList[mode].dbuf = 1;

	if (DD_Emulation) _DDModeList[mode].emul = 1;

   _DDNumModes++;

	WRITELOG((LogFile, "Register mode (%dx%dx%d) (paged=%d) (dbuf=%d).\n", width, height, bpp, _DDModeList[mode].paged, _DDModeList[mode].dbuf));	

   return DDENUMRET_OK;
}
