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
static char rcsid[] = "$Id: ddraw.c,v 1.1.1.1 2001-01-19 03:30:15 bradleyb Exp $";
#pragma on (unreferenced)


#define WIN95
#define _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "win\ddraw.h"

#include <stdio.h>
#include <mem.h>

#include "gr.h"
#include "mono.h"
#include "error.h"
#include "winapp.h"
#include "dd.h"
#include "args.h"


//	Direct X Variables ---------------------------------------------------------

LPDIRECTDRAW			_lpDD=0;				// Direct Draw Object
LPDIRECTDRAWSURFACE	_lpDDSPrimary=0;	// Primary Display Screen (Page 1)
LPDIRECTDRAWSURFACE	_lpDDSBack=0;		// Page 2 or offscreen canvas
LPDIRECTDRAWCLIPPER	_lpDDClipper=0;	// Window Clipper Object
LPDIRECTDRAWPALETTE	_lpDDPalette=0;	// Direct Draw Palette;
DDMODEINFO				_DDModeList[16];	// Mode list for Exclusive mode.
int						_DDNumModes = 0;	// Number of display modes
BOOL						_DDFullScreen;		// Full Screen DD mode?
BOOL						_DDExclusive;		// Exclusive mode?
BOOL						_DDSysMemSurfacing=TRUE;
int						W95DisplayMode;	// Display mode.
int						W95OldDisplayMode;

dd_caps					ddDriverCaps;		// Driver Caps.

int						_DDFlags=0;			// Direct Draw Flags
int						_DDLockCounter=0;	// DirectDraw Lock Surface counter

static BOOL				DDUseEmulation=FALSE;
static HWND				DDWnd=NULL;

static int				DDVideoLocks = 0;
static int				DDSystemLocks = 0;

//	Function prototypes --------------------------------------------------------

HRESULT CALLBACK EnumDispModesCB(LPDDSURFACEDESC lpddsd, LPVOID context);
BOOL CheckDDResult(HRESULT ddresult, char *funcname);
BOOL DDRestoreCanvas(dd_grs_canvas *canvas);


//	Direct Draw Initialization
// ----------------------------------------------------------------------------

BOOL DDInit(int mode)
{
	LPDIRECTDRAW lpdd;
	DDCAPS ddcaps, ddcaps2;
	HRESULT ddresult;
	int num;

	DDWnd = GetLibraryWindow();

//	Create Direct Draw Object (Use Emulation if Hardware is off)
	if (!_lpDD)	{
		ddresult = DirectDrawCreate(NULL, &lpdd, NULL);
		
		if (ddresult == DDERR_NODIRECTDRAWHW) {
         ddresult = DirectDrawCreate( (LPVOID) DDCREATE_EMULATIONONLY, &lpdd, NULL );
			if (!CheckDDResult(ddresult, "InitDD:DirectDrawCreate emulation"))
				return FALSE;
			DDUseEmulation = TRUE;
			logentry("DirectDraw: forcing emulation.\n");
		}
		else if (ddresult != DD_OK) return FALSE;
		logentry("DirectDraw: DirectX API hardware compliant.\n");
	}
	else return FALSE;

	atexit(DDKill);

//	Determine hardware caps
//	Determine capture mode (fullscreen takes exclusive, window is normal)
	if (mode == DDGR_FULLSCREEN) {
		DWORD flags;

		flags = DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN;

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

		_DDExclusive = TRUE;
		_DDFullScreen = TRUE;			
	}
	else if (mode == DDGR_EXWINDOW) {
		ddresult = IDirectDraw_SetCooperativeLevel(lpdd, DDWnd,
										DDSCL_EXCLUSIVE |
										DDSCL_FULLSCREEN);
		if (!CheckDDResult(ddresult, "DDInit::SetCooperativeLevel"))
			return FALSE;
		_DDExclusive = TRUE;
		_DDFullScreen = FALSE;
	}
	else if (mode == DDGR_WINDOW) {
		ddresult = IDirectDraw_SetCooperativeLevel(lpdd, DDWnd,
										DDSCL_NORMAL);
		if (!CheckDDResult(ddresult, "DDInit::SetCooperativeLevel"))
			return FALSE;
		_DDExclusive = FALSE;
		_DDFullScreen = FALSE;
	}
	else return FALSE;
	
//	Get Display modes/Window Sizes
//	Force invalidation of all modes for now
	for (num = 0; num < 16; num++)
	{
		_DDModeList[num].rw = _DDModeList[num].w = -1;
		_DDModeList[num].rh = _DDModeList[num].h = -1;
	}

	W95DisplayMode = SM95_640x480x8;
	num = 0;
	if (mode == DDGR_FULLSCREEN) {
		ddresult = IDirectDraw_EnumDisplayModes(lpdd, 0, NULL, 0, 
									EnumDispModesCB);
		if(!CheckDDResult(ddresult, "DDInit::EnumDisplayModes")) {
			IDirectDraw_Release(lpdd);
			return FALSE;
		}
	}
	else if (mode == DDGR_EXWINDOW) {
		_DDModeList[SM95_320x200x8X].rw = 320;
		_DDModeList[SM95_320x200x8X].rh = 200;
		_DDModeList[SM95_320x200x8X].w = 640;
		_DDModeList[SM95_320x200x8X].h = 480;
		_DDModeList[SM95_320x200x8X].emul = 1;
		_DDModeList[SM95_320x200x8X].dbuf = 0;
		_DDModeList[SM95_320x200x8X].modex = 0;
		_DDModeList[SM95_320x200x8X].paged = 0; 

		_DDModeList[SM95_640x480x8].rw = 640;
		_DDModeList[SM95_640x480x8].rh = 480;
		_DDModeList[SM95_640x480x8].w = 640;
		_DDModeList[SM95_640x480x8].h = 480;
		_DDModeList[SM95_640x480x8].emul = 1;
		_DDModeList[SM95_640x480x8].dbuf = 0;
		_DDModeList[SM95_640x480x8].modex = 0;
		_DDModeList[SM95_640x480x8].paged = 0; 

		_DDModeList[SM95_800x600x8].rw = 800;
		_DDModeList[SM95_800x600x8].rh = 600;
		_DDModeList[SM95_800x600x8].w = 640;
		_DDModeList[SM95_800x600x8].h = 480;
		_DDModeList[SM95_800x600x8].emul = 1;
		_DDModeList[SM95_800x600x8].dbuf = 0;
		_DDModeList[SM95_800x600x8].modex = 0;
		_DDModeList[SM95_800x600x8].paged = 0; 
		_DDNumModes = 3;
	}
	else if (mode == DDGR_WINDOW) {	
		_DDModeList[SM95_320x200x8X].rw = 320;
		_DDModeList[SM95_320x200x8X].rh = 200;
		_DDModeList[SM95_320x200x8X].w = 640;
		_DDModeList[SM95_320x200x8X].h = 480;
		_DDModeList[SM95_320x200x8X].emul = 1;
		_DDModeList[SM95_320x200x8X].dbuf = 0;
		_DDModeList[SM95_320x200x8X].modex = 0;
		_DDModeList[SM95_320x200x8X].paged = 0; 

		_DDModeList[SM95_640x480x8].rw = 640;
		_DDModeList[SM95_640x480x8].rh = 480;
		_DDModeList[SM95_640x480x8].w = 640;
		_DDModeList[SM95_640x480x8].h = 480;
		_DDModeList[SM95_640x480x8].emul = 1;
		_DDModeList[SM95_640x480x8].dbuf = 0;
		_DDModeList[SM95_640x480x8].modex = 0;
		_DDModeList[SM95_640x480x8].paged = 0; 

		_DDModeList[SM95_800x600x8].rw = 800;
		_DDModeList[SM95_800x600x8].rh = 600;
		_DDModeList[SM95_800x600x8].w = 800;
		_DDModeList[SM95_800x600x8].h = 600;
		_DDModeList[SM95_800x600x8].emul = 1;
		_DDModeList[SM95_800x600x8].dbuf = 0;
		_DDModeList[SM95_800x600x8].modex = 0;
		_DDModeList[SM95_800x600x8].paged = 0; 
		_DDNumModes = 3;
	}
	else return FALSE;

//	Set appropriate display mode or window mode

	_lpDD = lpdd;

	memset(&ddcaps, 0, sizeof(ddcaps));
	ddcaps.dwSize = sizeof(ddcaps);
	ddcaps2.dwSize = sizeof(ddcaps);
	ddresult = IDirectDraw_GetCaps(_lpDD, &ddcaps, NULL);
	if (!CheckDDResult(ddresult, "InitDD::GetCaps")) 
		return FALSE;

	logentry("DirectDraw: VRAM free:  %d\n", ddcaps.dwVidMemFree);	
	logentry("DirectDraw: VRAM total: %d\n", ddcaps.dwVidMemTotal);

#ifndef NDEBUG
	if (FindArg("-TsengDebug1")) {
		IDirectDraw_Release(lpdd);
		return FALSE;
	}
#endif

	DDSetDisplayMode(W95DisplayMode, 0);

#ifndef NDEBUG
	if (FindArg("-TsengDebug2")) {
		IDirectDraw_Release(lpdd);
		return FALSE;
	}
#endif
	
	// If 'windowed' do this.
	if (!_DDFullScreen) 
	{
			ddresult = IDirectDraw_CreateClipper(_lpDD, 0, &_lpDDClipper, NULL);
			if (!CheckDDResult(ddresult, "DDCreateScreen::CreateClipper"))
				return FALSE;

			ddresult = IDirectDrawClipper_SetHWnd(_lpDDClipper, 0, DDWnd);
			if (!CheckDDResult(ddresult, "DDCreateScreen::SetHWnd"))
				return FALSE;

			ddresult = IDirectDrawSurface_SetClipper(_lpDDSPrimary, _lpDDClipper);
			if (!CheckDDResult(ddresult, "DDCreateScreen::SetClipper"))
				return FALSE;
	}

//	Register Optimizations

	ddcaps.dwSize = sizeof(ddcaps);
	ddcaps2.dwSize = sizeof(ddcaps);
	ddresult = IDirectDraw_GetCaps(lpdd, &ddcaps, &ddcaps2);
	if (!CheckDDResult(ddresult, "DDInit::GetCaps"))
		return FALSE;

#ifndef NDEBUG
	if (FindArg("-TsengDebug3")) {
		IDirectDraw_Release(lpdd);
		return FALSE;
	}
#endif

	if (FindArg("-vidram")) {
		logentry("DirectDraw: Forcing VRAM rendering.\n");
		_DDSysMemSurfacing = FALSE;
	}
	else if (FindArg("-sysram")) {
		logentry("DirectDraw: Forcing SRAM rendering.\n");
		_DDSysMemSurfacing = TRUE;
	}
	else if (ddcaps.dwCaps & DDCAPS_BANKSWITCHED) {
		logentry("DirectDraw: Hardware is bank-switched.  Using SRAM rendering.\n");
		_DDSysMemSurfacing = TRUE;
	}
	else {
		logentry("DirectDraw: Hardware is not bank-switched.  Using VRAM rendering.\n");
		_DDSysMemSurfacing = FALSE;
	}
		
	if (ddcaps.dwCaps	& DDCAPS_COLORKEYHWASSIST) 
		ddDriverCaps.hwcolorkey = 1;
	else 
		ddDriverCaps.hwcolorkey = 0;
	if (ddcaps.dwCaps & DDCAPS_BLTSTRETCH)
		ddDriverCaps.hwbltstretch = 1;
	else 
		ddDriverCaps.hwbltstretch = 0;
		

//@@	mprintf((0, "DD::Hardware="));
//@@	if (ddcaps.dwCaps & DDCAPS_NOHARDWARE) mprintf((0, "Off\n"));
//@@	else mprintf((0, "On\n"));
//@@
//@@	mprintf((0, "DD::VideoMem=%u bytes\n", ddcaps.dwVidMemTotal));

//@@	mprintf((0, "DD::SrcColorKey="));	
//@@	if (ddcaps.dwCKeyCaps & DDCKEYCAPS_SRCBLT) mprintf((0, "Hardware\n"));
//@@	else mprintf((0, "Emulation\n"));

	return TRUE;
}


//	Direct Draw Destruction
// ----------------------------------------------------------------------------

void DDKill()
{
//	Perform cleanup for full screen case and window case
	DDKillScreen();
	if (_lpDDPalette) IDirectDrawPalette_Release(_lpDDPalette);
	if (_lpDD) IDirectDraw_Release(_lpDD);

	_DDExclusive = _DDFullScreen = FALSE;
	_DDNumModes = 0;
	_DDLockCounter = 0;
	_lpDD = NULL;
}


//	Direct Draw Surface Creation
//		Create Screen (Page 0 and possibly Page 1 or offscreen buffer)
// ----------------------------------------------------------------------------

BOOL DDCreateScreen(int flags)
{
	DDSCAPS ddscaps;
	DDCAPS ddcaps, ddcaps2;
	DDSURFACEDESC ddsd;
	HRESULT ddresult;

	memset(&ddcaps, 0, sizeof(ddcaps));
	memset(&ddcaps2, 0, sizeof(ddcaps2));
	ddcaps.dwSize = sizeof(ddcaps);
	ddcaps2.dwSize = sizeof(ddcaps);
	ddresult = IDirectDraw_GetCaps(_lpDD, &ddcaps, &ddcaps2);
	if (!CheckDDResult(ddresult, "DDCreateScreen::GetCaps"))
		return FALSE;


	logentry("DirectDraw HW Caps:  %x\nDirectDraw HEL Caps: %x\n",ddcaps.dwCaps,ddcaps2.dwCaps);
	if (ddcaps.dwCaps & DDCAPS_BANKSWITCHED) {
		logentry("DirectDraw: Hardware is bank-switched.  Using SRAM rendering.\n");
		_DDSysMemSurfacing = TRUE;
	}
	else {
		logentry("DirectDraw: Hardware is not bank-switched.  Using VRAM rendering.\n");
		_DDSysMemSurfacing = FALSE;
	}

//	Determine GFX caps.
	if (ddcaps.dwCaps	& DDCAPS_COLORKEYHWASSIST) 
		ddDriverCaps.hwcolorkey = 1;
	else 
		ddDriverCaps.hwcolorkey = 0;
	if (ddcaps.dwCaps & DDCAPS_BLTSTRETCH)
		ddDriverCaps.hwbltstretch = 1;
	else 
		ddDriverCaps.hwbltstretch = 0;
	
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);

	if (_DDFullScreen && GRMODEINFO(paged)) {
	//	We should use page flipping
		ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
		ddsd.dwBackBufferCount = 1;
		ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE |
										DDSCAPS_FLIP |
										DDSCAPS_COMPLEX;
		ddresult = IDirectDraw_CreateSurface(_lpDD, &ddsd, &_lpDDSPrimary, NULL);
		if (!CheckDDResult(ddresult, "DDCreateScreen::CreateSurface -fullscreen"))
			return FALSE;
			
		ddscaps.dwCaps = DDSCAPS_BACKBUFFER;
		ddresult = IDirectDrawSurface_GetAttachedSurface(_lpDDSPrimary,
													&ddscaps, &_lpDDSBack);
		if (!CheckDDResult(ddresult, "DDCreateScreen::GetAttachedSurface"))
			return FALSE;
	}
	else {
	// We just create a primary and offscreen buffer
		if (GRMODEINFO(emul) && !_lpDDSPrimary) {
		// make sure we don't reinitialize the screen if we already made it
		//	beforehand for windowed version
			ddsd.dwFlags = DDSD_CAPS;
			ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
			ddresult = IDirectDraw_CreateSurface(_lpDD, &ddsd, &_lpDDSPrimary, NULL);
			if (!CheckDDResult(ddresult, "DDCreateScreen::CreateSurface -windowed"))
				return FALSE;
		}
		else if (!GRMODEINFO(emul)) {
		// If we aren't emulating
			ddsd.dwFlags = DDSD_CAPS;
			ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
			ddresult = IDirectDraw_CreateSurface(_lpDD, &ddsd, &_lpDDSPrimary, NULL);
			if (!CheckDDResult(ddresult, "DDCreateScreen::CreateSurface -windowed"))
				return FALSE;
		}

		if (GRMODEINFO(emul)) {
			_lpDDSBack = DDCreateSurface(_DDModeList[W95DisplayMode].rw, 
										_DDModeList[W95DisplayMode].rh, 1);
			if (!_lpDDSBack) {
				mprintf((0,"Call to create DDSBackBuffer failed."));
				return FALSE;
			}
		}
		else _lpDDSBack = NULL;	
	
	}

//	Create 8-bit palette
	{
		ubyte pal[768];
		memset(pal, 0, 768);
		
		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		IDirectDrawSurface_GetSurfaceDesc(_lpDDSPrimary, &ddsd);

		logentry("Primary surface pixel format: %x, %d\n", ddsd.ddpfPixelFormat.dwFlags, ddsd.ddpfPixelFormat.dwRGBBitCount); 

		_lpDDPalette = DDCreatePalette(pal);
		Assert(_lpDDPalette != NULL);
		DDSetPalette(_lpDDPalette);
	}

	return TRUE;
}


//	DirectDraw Screen Kill
//	----------------------------------------------------------------------------

void DDKillScreen()
{
	if (_lpDDClipper) IDirectDrawClipper_Release(_lpDDClipper);
	if (_lpDDPalette) IDirectDrawPalette_Release(_lpDDPalette);
	if (_lpDDSBack) IDirectDrawSurface_Release(_lpDDSBack);
	if (_lpDDSPrimary) {

		if (!GRMODEINFO(modex)) {
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

		IDirectDrawSurface_Release(_lpDDSPrimary);
	}

	_lpDDClipper = NULL;
	_lpDDPalette = NULL;
	_lpDDSBack = NULL;
	_lpDDSPrimary = NULL;
}	


void DDKillEmulatedScreen()
{
	if (_lpDDSBack) IDirectDrawSurface_Release(_lpDDSBack);

	_lpDDSBack = NULL;
}


//	DDSetDisplayMode
//	----------------------------------------------------------------------------

void DDSetDisplayMode(int display_mode, int flags)
{
	HRESULT ddresult;

	W95DisplayMode = display_mode;
	W95OldDisplayMode = display_mode;
	if (_DDFullScreen) {
		logentry("Setting screen display mode to (%dx%dx%d::%dx%dx%d).\n", _DDModeList[W95DisplayMode].w, _DDModeList[W95DisplayMode].h,	_DDModeList[W95DisplayMode].bpp,_DDModeList[W95DisplayMode].rw, _DDModeList[W95DisplayMode].rh,	_DDModeList[W95DisplayMode].bpp);
	
		DDKillScreen();
		ddresult = IDirectDraw_SetDisplayMode(_lpDD, _DDModeList[W95DisplayMode].w,
										_DDModeList[W95DisplayMode].h,
										_DDModeList[W95DisplayMode].bpp);
		if (!CheckDDResult(ddresult, "DDInit::SetDisplayMode")) {
			Error("Unable to set display mode: %d.\n", W95DisplayMode);
		}
		DDCreateScreen(flags);
	}
	else {
		RECT rect;
		DWORD dwStyle;

		DDKillEmulatedScreen();

		dwStyle = GetWindowLong(DDWnd, GWL_STYLE);
		dwStyle &= ~WS_POPUP;
		dwStyle |= WS_OVERLAPPED | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX;

		SetRect(&rect, 0, 0, _DDModeList[W95DisplayMode].w,
				_DDModeList[W95DisplayMode].h);	
		SetWindowLong(DDWnd, GWL_STYLE, dwStyle);
		AdjustWindowRectEx(&rect, 
						GetWindowLong(DDWnd, GWL_STYLE),
						GetMenu(DDWnd)!=NULL,
						GetWindowLong(DDWnd, GWL_EXSTYLE));

		SetWindowPos(DDWnd, NULL, 0,0,
					rect.right-rect.left,
					rect.bottom-rect.top,
				SWP_NOMOVE | 
				SWP_NOZORDER | 
				SWP_NOACTIVATE);
		SetWindowPos(DDWnd, HWND_NOTOPMOST, 0, 0, 0, 0, 
				SWP_NOSIZE |
				SWP_NOMOVE |
				SWP_NOACTIVATE);

		DDCreateScreen(flags);
	}
}


//	DDRestoreScreen
//		Restore screens 
//	----------------------------------------------------------------------------
int DDRestoreScreen()
{
	mprintf((1, "Need to Restore DDraw Page0 and Page1.\n"));
	if (!_lpDDSPrimary || IDirectDrawSurface_Restore(_lpDDSPrimary) !=DD_OK) {
		mprintf((1, "Warning: Unable to restore Primary Surface.\n"));
		return 0;
	}
	if (!GRMODEINFO(paged) && _lpDDSBack) {
		if (IDirectDrawSurface_Restore(_lpDDSBack) != DD_OK) {
			mprintf((1, "Warning: Unable to restore Back Surface.\n"));
			return 0;
		}
	}
	else if (!_DDFullScreen) {
		if (!_lpDDSBack || IDirectDrawSurface_Restore(_lpDDSBack) != DD_OK) {
			mprintf((1, "Warning: Unable to restore Back Surface.\n"));
			return 0;
		}
	}

	return 1;
}


//	DDFlip
//		Flip Screens using DirectDraw::Flip
//	----------------------------------------------------------------------------
void DDFlip()
{
	HRESULT ddresult;

	ddresult = IDirectDrawSurface_Flip(_lpDDSPrimary, NULL, 0);
	if (ddresult != DD_OK) {
		mprintf((1, "DDFlip:: Unable to flip screen (%X)\n", ddresult));
		Int3();									// Bad flipping
	}
}


//	DDCreateSurface
//		Create an offscreen surface hopefully in video memory
//	----------------------------------------------------------------------------
LPDIRECTDRAWSURFACE DDCreateSurface(int width, int height, BOOL vram)
{
	DDSURFACEDESC ddsd;
	HRESULT ddresult;
	LPDIRECTDRAWSURFACE lpdds;
	DDCOLORKEY ddck;

	if (_DDSysMemSurfacing && !vram) return DDCreateSysMemSurface(width, height);

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
	ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
	ddsd.dwWidth = width;
	ddsd.dwHeight = height;

//	logentry("Creating %dx%d sysram/vidram surface.\n", width, height);
	ddresult = IDirectDraw_CreateSurface(_lpDD, &ddsd, &lpdds, NULL);
	if (ddresult != DD_OK) {
		logentry("DDRAW::CreateSurface err: %x\n", ddresult);		
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

//	logentry("Creating %dx%d sysram surface.\n", width, height);
	ddresult = IDirectDraw_CreateSurface(_lpDD, &ddsd, &lpdds, NULL);
	if (ddresult != DD_OK) {
		logentry("DDRAW::CreateSysMemSurface err: %x\n", ddresult);		
		return NULL;
	}

	return lpdds;
}


//	DDGetPalette
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
	

//	DDCreatePalette
//	----------------------------------------------------------------------------
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
								DDPCAPS_8BIT | DDPCAPS_ALLOW256, 
								pe, 
								&lpddpal, NULL);
	if (ddresult != DD_OK) {
		mprintf((1, "DDERR: CreatePalette %x.\n", ddresult));
		return NULL;
	}
	
	return lpddpal;
}


//	DDSetPalette
//	----------------------------------------------------------------------------
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
			Error("Unable to attach palette to primary surface: %x.", ddresult);
		}
	}
}


//	DDLock and DDUnlock Canvas
//		This is required to access any canvas.
//	----------------------------------------------------------------------------
#ifndef NDEBUG
void DDLockCanvas_D(dd_grs_canvas *canvas, char *filename, int line)
{
	HRESULT ddresult;
	DDSURFACEDESC ddsd;
	RECT rect;
	grs_bitmap *bmp;


	bmp = &canvas->canvas.cv_bitmap;
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	
	if (canvas->lock_count == 0) {
	// Obtain info about a rectangle on the surface
		SetRect(&rect, bmp->bm_x, bmp->bm_y, 
				bmp->bm_x+bmp->bm_w, bmp->bm_y+bmp->bm_h);

	RetryLock:

		ddresult = IDirectDrawSurface_Lock(canvas->lpdds, 	
										&rect,
										&ddsd,
										DDLOCK_WAIT,
										NULL);
		if (ddresult != DD_OK) {
			if (ddresult == DDERR_SURFACELOST) {
				if (!DDRestoreCanvas(canvas))
					Error("Unable to restore surface for lock:%x (%s line %d)\n", ddresult, filename, line);
				else goto RetryLock;
			}
			else {
				while (canvas->lock_count) DDUnlockCanvas(canvas);
				Error("Unable to lock canvas: %x (%s line %d)\n", ddresult, filename, line);
			}
		}
		bmp->bm_data = (unsigned char *)ddsd.lpSurface;
		bmp->bm_rowsize = (short)ddsd.lPitch;

//		if (canvas->sram && !GRMODEINFO(modex)) {
//		//	Manually calculate?
//			bmp->bm_data = bmp->bm_data + (bmp->bm_y*bmp->bm_rowsize);
//			bmp->bm_data += bmp->bm_x;
//		} 

		_DDLockCounter++;
	}
	canvas->lock_count++;
}
#endif


void DDLockCanvas(dd_grs_canvas *canvas)
{
	HRESULT ddresult;
	DDSURFACEDESC ddsd;
	RECT rect;
	grs_bitmap *bmp;


	bmp = &canvas->canvas.cv_bitmap;
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	
	if (canvas->lock_count == 0) {
	// Obtain info about a rectangle on the surface
		SetRect(&rect, bmp->bm_x, bmp->bm_y, 
				bmp->bm_x+bmp->bm_w, bmp->bm_y+bmp->bm_h);

	RetryLock:

		ddresult = IDirectDrawSurface_Lock(canvas->lpdds, 	
										&rect,
										&ddsd,
										DDLOCK_WAIT,
										NULL);
		if (ddresult != DD_OK) {
			if (ddresult == DDERR_SURFACELOST) {
				if (!DDRestoreCanvas(canvas))
					Error("Unable to restore surface for lock:");
				else goto RetryLock;
			}
			else {
				while (canvas->lock_count) DDUnlockCanvas(canvas);
				Error("Unable to lock canvas: %x\n", ddresult);
			}
		}
		bmp->bm_data = (unsigned char *)ddsd.lpSurface;
		bmp->bm_rowsize = (short)ddsd.lPitch;

//		if (canvas->sram && !GRMODEINFO(modex)) {
//		//	Manually calculate?
//			bmp->bm_data = bmp->bm_data + (bmp->bm_y*bmp->bm_rowsize);
//			bmp->bm_data += bmp->bm_x;
//		} 

		_DDLockCounter++;
	}
	canvas->lock_count++;
}


void DDUnlockCanvas(dd_grs_canvas *canvas)
{
	HRESULT ddresult;
	grs_bitmap *bmp;

	bmp = &canvas->canvas.cv_bitmap;
	
	if (canvas->lock_count == 1) {
//		if (canvas->sram && !GRMODEINFO(modex)) {
//			bmp->bm_data = bmp->bm_data - bmp->bm_x;
//			bmp->bm_data = bmp->bm_data - (bmp->bm_y*bmp->bm_rowsize);
//		}
		ddresult = IDirectDrawSurface_Unlock(canvas->lpdds, 
								canvas->canvas.cv_bitmap.bm_data);
		if (ddresult != DD_OK) {	
				Error("Unable to unlock canvas: %x\n", ddresult);
				exit(1);
		}

		canvas->canvas.cv_bitmap.bm_data = NULL;
		canvas->canvas.cv_bitmap.bm_rowsize = 0;
		_DDLockCounter--;
	}
	canvas->lock_count--;
}


void DDLockDebug()
{
	logentry("VRAM locks: %d.  SRAM locks: %d\n", DDVideoLocks, DDSystemLocks);
}



//	DDFreeSurface
//	----------------------------------------------------------------------------
void DDFreeSurface(LPDIRECTDRAWSURFACE lpdds)
{
	HRESULT ddresult;

	Assert(lpdds != NULL);

	ddresult = IDirectDrawSurface_Release(lpdds);
	if (ddresult != DD_OK) {
		logentry("DDRAW::FreeSurface err: %x\n", ddresult);		
		Error("DDFreeSurface: Unable to free surface.");
	}
}


//	DDRestoreCanvas
//	----------------------------------------------------------------------------
BOOL DDRestoreCanvas(dd_grs_canvas *canvas)
{
	HRESULT ddresult;
	
	Assert(canvas->lpdds != NULL);
	
	ddresult = IDirectDrawSurface_Restore(canvas->lpdds);
	if (ddresult != DD_OK) {
		if (ddresult != DDERR_WRONGMODE) {
			logentry("DDRAW::RestoreCanvas::Surface err: %x\n", ddresult);		
			return FALSE;
		}
		mprintf((0, "Recreating surfaces:\n"));
	// Must recreate canvas
		if (canvas->lpdds == _lpDDSPrimary || canvas->lpdds == _lpDDSBack) {
			mprintf((0, "DDRestoreCanvas::Screen memory was lost!\n"));
			exit(1);
		}
		if (canvas->sram) {
		// force sysmem canvas!
			canvas->lpdds = DDCreateSysMemSurface(canvas->canvas.cv_bitmap.bm_w,
											canvas->canvas.cv_bitmap.bm_h);
		}
		else {
			canvas->lpdds = DDCreateSurface(canvas->canvas.cv_bitmap.bm_w, 
											canvas->canvas.cv_bitmap.bm_h,
											_DDSysMemSurfacing);
		}					
	}
	return TRUE;
}



//	CheckDDResult
//	----------------------------------------------------------------------------
BOOL CheckDDResult(HRESULT ddresult, char *funcname)
{
	char buf[256];

	if (ddresult != DD_OK) {
		sprintf(buf, "DirectDraw error %x detected in\r\n\t%s", ddresult, funcname);
		logentry(buf);
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
	else if (width == 320 && height == 200 && bpp==8) 
		mode = SM95_320x200x8X;
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

	if (mode == SM95_320x200x8X) {
		_DDModeList[mode].modex = 1;
		_DDModeList[mode].dbuf = 1;
		_DDModeList[mode].paged = 1;
	}
	else if (mode == SM95_640x400x8) {
	//	Support a range of emulated modes
	//	320x200x8 Double Pixeled
		_DDModeList[SM95_320x200x8].rw = 320;
		_DDModeList[SM95_320x200x8].rh = 200;
		_DDModeList[SM95_320x200x8].emul = 1;
		_DDModeList[SM95_320x200x8].dbuf = 0;
		_DDModeList[SM95_320x200x8].modex = 0;
		_DDModeList[SM95_320x200x8].paged = 0; 
		_DDModeList[SM95_320x200x8].w = 640;
		_DDModeList[SM95_320x200x8].h = 400;
		_DDModeList[SM95_320x200x8].bpp = 8;
		
		_DDModeList[SM95_320x400x8].rw = 320;
		_DDModeList[SM95_320x400x8].rh = 400;
		_DDModeList[SM95_320x400x8].emul = 1;
		_DDModeList[SM95_320x400x8].dbuf = 0;
		_DDModeList[SM95_320x400x8].modex = 0;
		_DDModeList[SM95_320x400x8].paged = 0; 
		_DDModeList[SM95_320x400x8].w = 640;
		_DDModeList[SM95_320x400x8].h = 400;
		_DDModeList[SM95_320x400x8].bpp = 8;

		_DDNumModes+=2;

		_DDModeList[mode].dbuf = 1;
	}
	else {
		_DDModeList[mode].dbuf = 1;
	}

   _DDModeList[mode].w   = width;
   _DDModeList[mode].h   = height;
   _DDModeList[mode].bpp = bpp;

    _DDNumModes++;

    return DDENUMRET_OK;
}


int DDCheckMode(int mode)
{
	if (_DDModeList[mode].w==-1 && _DDModeList[mode].h==-1) return 1;
	else return 0;
}
