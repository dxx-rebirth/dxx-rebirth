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



#ifndef _DESW_H
#define _DESW_H


#ifdef WINDOWS

#define DESCENT_VIEWPORT_WIDTH	640
#define DESCENT_VIEWPORT_HEIGHT	480
#define DESCENT_RENDER_WIDTH		320
#define DESCENT_RENDER_HEIGHT		200


#define WIN95
#define _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "win\dd.h"
#include "win\winapp.h"
#include "pstypes.h"
#include "fix.h"
#include "gr.h"

#undef	DEFAULT_PALETTE


#define WINAPP_NAME "Descent II"

typedef struct GAME_CONTEXT {
	BOOL paused;
	BOOL active;
};

typedef struct SCREEN_CONTEXT {
	char *bkg_filename;
} SCREEN_CONTEXT;


typedef struct PALETTE {
	WORD version;
	WORD num_entries;
	PALETTEENTRY entries[256];
} PALETTE;

typedef struct RGBBITMAPINFO {
	BITMAPINFOHEADER	bmiHeader;
	RGBQUAD				rgb[256];
} RGBBITMAPINFO;



//	Other Structures

#define MSG_QUIT	0
#define MSG_SHUTDOWN 1
#define MSG_NORMAL 2


typedef struct WinJoystickDesc {
	char title[32];
	char cal_ztitle[16];
	char cal_rtitle[16];
	char cal_utitle[16];
	char cal_vtitle[16];
	char cal_zmsg[6][16];
	char cal_rmsg[6][16];
	char cal_umsg[6][16];
	char cal_vmsg[6][16];
} WinJoystickDesc;


//	Globals

extern HWND 		_hAppWnd;						// Descent Window
extern HINSTANCE 	_hAppInstance;
extern int			_DDraw;							// Direct X Implementation
extern BOOL			_AppActive;
extern BOOL 		SOS_DLLInit;
extern BOOL			_RedrawScreen;
extern SCREEN_CONTEXT _SCRContext;


extern dd_grs_canvas *dd_VR_offscreen_buffer;
extern dd_grs_canvas dd_VR_screen_pages[2];
extern dd_grs_canvas dd_VR_render_buffer[2];
extern dd_grs_canvas dd_VR_render_sub_buffer[2];



//	WinG Stuff

extern HPALETTE	_hAppPalette;					// Application Palette


//	Other Globals

extern RECT			ViewportRect;					// Viewport rect for window
extern char			*_OffscreenCanvasBits;		// Pointer to DIB Bits.
extern fix			WinFrameTime;					// Time per frame
extern int			Platform_system;				// Tells us the platform


//	Functions

extern void WinDelay(int msecs);

//	misc functions
extern void WErrorPrint(char *msg);
extern dd_grs_canvas *get_current_game_screen();
extern BOOL SOSInit();
extern void SOSUnInit();


#define MOUSE_DEFAULT_CURSOR 1
#define MOUSE_WAIT_CURSOR 2

extern void LoadCursorWin(int cursor);
extern void ShowCursorW();
extern void HideCursorW();


//	Macros
#define CanvasWidth(C) ((C).bmiHeader.biWidth)
#define CanvasHeight(C) (((C).bmiHeader.biHeight > 0) ? \
							(C).bmiHeader.biHeight : -(C).bmiHeader.biHeight)

#define DebugMessageBox(c) (MessageBox(NULL,c,"Message",MB_OK))

#define DEFINE_SCREEN(fn) (_SCRContext.bkg_filename = fn)

#define WINNT_PLATFORM 1
#define WIN95_PLATFORM 0


#endif


#endif
