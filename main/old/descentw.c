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
static char rcsid[] = "$Id: descentw.c,v 1.1.1.1 2001-01-19 03:30:14 bradleyb Exp $";
#pragma on (unreferenced)

#include "desw.h"

#include <mmsystem.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#include "winapp.h"
#include "error.h"
#include "gr.h"
#include "digi.h"
#include "key.h"
#include "inferno.h"
#include "gamepal.h"
#include "game.h"
#include "songs.h"
#include "mono.h"
#include "screens.h"
#include "config.h"
#include "newdemo.h"
#include "text.h"
#include "menu.h"
#include "vers_id.h"
#include "mvelibw.h"
#include "modem.h"
#include "cntrlcen.h"

#define D2_ICON 50

#define MIN_VIRTUAL_MEM_FREE		(16 * (1024*1024))
#define MIN_PHYSICAL_MEM_FREE		(16 * (1024*1024))
#define SOUND_22K_CUTOFF			(16 *1024*1024)		// 8.0 MB


//	Globals --------------------------------------------------------------------

HWND 			_hAppWnd;						// Descent Window
HINSTANCE 	_hAppInstance;
int			_DDraw = 0;						// No DirectX
char			*_OffscreenCanvasBits;		// Pointer to Offscreen Canvas Bits.
BOOL			_AppActive = FALSE;			// Is application active?
BOOL			_AppPaused = FALSE;			// Shall we run game?
BOOL			_AppInit = FALSE;				// Is application initialized?
BOOL			_RedrawScreen=FALSE;			// Shall we force a redraw of the screen?
SCREEN_CONTEXT _SCRContext;

HANDLE		hDescent2Mutex = NULL;		// handle to determine whether D2 is already running
HANDLE 		hDescent2Mutex2 = NULL;

BOOL			AllowActivates = FALSE;

//	HACK, to keep original CD audio vol.
DWORD CD_audio_desktop_vol = 0;
int CD_audio_desktop_dev = -1;

//	EXTERNS

int Platform_system=0;


extern int Mono_initialized;
extern int WinEnableInt3;
extern int DD_Emulation;
extern int Skip_briefing_screens;		// So we can skip the briefing 
extern int digi_system_initialized;
extern int framerate_on;
extern int Current_display_mode;
extern ubyte gr_palette_faded_out;
extern int piggy_low_memory;				// FROM PIGGY.C!
extern RECT ViewportRect;
extern int Joystick_calibrating;

// Game State and other Variables ---------------------------------------------

static char			WinErrorMessage[512];
static BOOL			WinErrorTrap = FALSE;
static BOOL			GameShutdown = FALSE;
//static BOOL			GameInitialized = FALSE;
static BOOL			WinEnableMovies = TRUE;


//	External Functions ---------------------------------------------------------

extern void grwin_gdi_realizepal(HDC hdc);
extern void grwin_cleanup_palette();

extern void GameLoop(int, int);
extern void game_setup(void);
extern void check_joystick_calibration();
extern void DDResizeViewport(void);
extern void SetWinMonoInfo(HWND hWnd, HINSTANCE hInstance);

extern void InitCD(char *arg);
extern void	InitVideo(void);
extern void	InitIO(void);
extern void	InitData(void);
extern void	InitSound(void);
extern void	InitNetwork(void);
extern void InitDescent(void);
extern void InitPilot(void);	


//	Function Prototypes --------------------------------------------------------

int RunGame(int argc, char *argv[]);
int RestoreGameSurfaces(void);
void ValidateSystem(void);
void MakeCodeWritable(void);
int ParseArgs(char *argv[], LPSTR lpCmdLine);
void WErrorCatch(char *message);

void DoRegisterCheck(void);

void AppExit();
void AppPaint(HWND hWnd, HDC hdc);
void AppSize(HWND hWnd);
void AppActivate(HWND hWnd, UINT wParam);
void AppQueryPalette(HWND hWnd);
void AppPaletteChange(HWND hWnd, UINT wParam);
void AppDisplayChange(HWND hWnd, int w, int h, int bpp);
BOOL AppHandleSystemKeys(UINT msg, UINT wParam, UINT lParam);

BOOL GameCheckMultiReactor();

LRESULT WINAPI _export DescentWndProc(HWND hWnd,UINT msg,UINT wParam,
													 LPARAM lParam);



// Initialization and Destruction
//	----------------------------------------------------------------------------

BOOL AppInit(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int nCmdShow)
{
//	Create Application Window and Initialize WinG stuff.
	WNDCLASS wc;

#ifdef RELEASE
	WinEnableInt3=0;
#else
	WinEnableInt3=1;
#endif

#ifndef NDEBUG
	if (FindArg("-logfile")) loginit("descentw.log");
#endif

	ValidateSystem();

	if (!hPrev) {
		wc.hCursor				= LoadCursor(NULL, IDC_ARROW);
		wc.hIcon					= LoadIcon(hInst, MAKEINTRESOURCE(D2_ICON));
		wc.lpszMenuName		= NULL;
		wc.lpszClassName 		= "DescentWndClass";
		wc.hbrBackground 		= (HBRUSH)GetStockObject(BLACK_BRUSH);
		wc.hInstance 			= hInst;
		wc.style					= CS_DBLCLKS;
		wc.lpfnWndProc			= (WNDPROC)DescentWndProc;
		wc.cbWndExtra			= 0;
		wc.cbClsExtra			= 0;

		if (!RegisterClass(&wc)) return FALSE;
	}

//	Arrgh, make entire code segment writable for now.
//	-codereadonly forces read only code (debug purposes)

	if (!FindArg("-codereadonly")) 
		MakeCodeWritable();

#ifndef RELEASE
	cinit();
#endif

	atexit(AppExit);

	_hAppInstance = hInst;
	_hAppWnd = CreateWindowEx(WS_EX_APPWINDOW,
					"DescentWndClass", 
					WINAPP_NAME,
					WS_POPUP | WS_SYSMENU | WS_BORDER,
					0, 0,
					GetSystemMetrics(SM_CXSCREEN),
					GetSystemMetrics(SM_CYSCREEN),
					NULL, 
					NULL, 
					hInst, 
					NULL);

	Assert(_hAppWnd != 0);

	ShowWindow(_hAppWnd, SW_SHOWNORMAL);
	UpdateWindow(_hAppWnd);

//	Tell Descent Libraries about Windows
	HideCursorW();
//	LoadCursorWin(MOUSE_WAIT_CURSOR);
//	ShowCursorW();

#ifdef RELEASE
#ifndef NDEBUG 
	if (FindArg("-monodebug")) Mono_initialized = 1;
	else // link to below line!
#endif	
	Mono_initialized = 0;
#else
	Mono_initialized = 1;
#endif

	minit();

	SetLibraryWinInfo(_hAppWnd, _hAppInstance);

	#ifndef NDEBUG
		mopen( 0, 9, 1, 78, 15, "Debug Spew");
		mopen( 1, 2, 1, 78,  5, "Errors & Serious Warnings");
	#endif

	return TRUE;
}


void AppExit()
{
//	HACK do this when the application is closing! restore the CD audio volume.
	if (CD_audio_desktop_dev != -1) 
		auxSetVolume(CD_audio_desktop_dev, CD_audio_desktop_vol);

	grwin_cleanup_palette();

	LoadCursorWin(MOUSE_DEFAULT_CURSOR);
	ShowCursorW();

#ifndef RELEASE
	if (WinErrorTrap) {
		cprintf("\n\n%s", WinErrorMessage);
		cgetch();
	}
	else {
		CloseWindow(_hAppWnd);
		cprintf("\n\nDescent II for Win95 done...\n");
		cgetch();
	}
	cclose();
#else 
	if (WinErrorTrap)
		MessageBox(NULL, WinErrorMessage, "Descent II error", MB_OK);
#endif

#ifndef NDEBUG
	logclose();
#endif

	if (hDescent2Mutex2) 
		CloseHandle(hDescent2Mutex2);

	if (hDescent2Mutex) 
		CloseHandle(hDescent2Mutex);		// Destroy Mutex Object!
}

extern char CDROM_dir[30];

void DoRegisterCheck()
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	BOOL flag;
	char filename[64];

	strcpy(filename, CDROM_dir);
	strcat(filename, "regcard.exe");

	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOW;

	flag = CreateProcess(
			NULL,
			filename, NULL, NULL,
			FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL,
			&si, &pi);			
	if (flag) {
		DWORD dwval;
		dwval = WaitForInputIdle(pi.hProcess, INFINITE);

		while (WaitForSingleObject(pi.hProcess, 0) != WAIT_OBJECT_0)
		{
		}

		mprintf((1, "JOY.CPL process finished?\n"));
		ShowWindow(_hAppWnd, SW_MAXIMIZE);
		SetForegroundWindow(_hAppWnd);
	}
}


//	Game -----------------------------------------------------------------------

char commandline_help[] = 
			"Command-line options (case is not significant):\n"
			"\n"
			" General Options:\n"
			"\n"
			"	-LowMem		Lower animation detail for better performance with low memory\n"
			"	-NoLowMem	Force high animation detail even when low memory\n"
			"	-Subtitles		Turn on movie subtitles (English-only)\n"
			"\n"
			" Input devices: -SpecialDevice\n"
			"\n"
			"	-IForce <n>	Use Immersion tactile feedback joystick on port <n>\n"
			"\n"
			" Sound & Music:\n"
			"\n"
			"	-NoSound	Turns off sound & music\n"
			"	-NoMusic		Disables music; sound effects remain enabled\n"
			"	-Sound22K	Use 22KHz sounds, even if machine has less than 16 MB\n"
			"	-Sound11K	Use 11KHz sounds, even if machine has more than 16 MB\n"
			"\n"
			" Network & Modem:\n"
			"\n"
			"	-CtsRts		Enables CTS/RTS handshaking during null-modem games\n"
			"	-NoNetwork	Disables network drivers\n"
			"	-NoSerial		Disables serial drivers\n"
			"	-Packets #	Specifies the packets per second where # is the number of packets\n"
			"	-Shortpackets	Turn on short packets\n"
			"	-Norankings	Disable multiplayer ranking system\n"
			"	-Noredunancy	suppresses duplicate messages such as \"You already have ....\"\n"
			"\n"
			"Diagnostic:\n"
			"	-emul		Certain video cards need this option in order to run game.\n"
			"	-ddemul		If -emul doesn't work, use this option.\n";

print_commandline_help()
{
	ShowCursorW();
	MessageBox(NULL, commandline_help, "Descent II Command-line Options", MB_OK);
}

int RunGame(int argc, char *argv[])
{
	MSG msg;
	int val;

	setbuf(stdout, NULL);
	error_init(WErrorCatch, NULL);

#ifndef RELEASE
	if (FindArg("-stopwatch"))
		timer_init(1); 
	else
		timer_init(0);
#else
	timer_init(0);
#endif

//	InitArgs(argc, argv);

	InitCD(argv[0]);
	
#ifdef RELEASE
	DoRegisterCheck();
#endif

	InitVideo();
	InitIO();
	InitData();
	InitSound();

	if (!FindArg("-nonetwork"))
		InitNetwork();

	if (!FindArg("-noserial"))
		serial_active = 1;

	AllowActivates = TRUE;	

	InitDescent();

	InitPilot();	

	if (Auto_demo)	{
		newdemo_start_playback("DESCENT.DEM");		
		if (Newdemo_state == ND_STATE_PLAYBACK )
			Function_mode = FMODE_GAME;
	}

//	do this here because the demo code can do a longjmp when trying to
//		autostart a demo from the main menu, never having gone into the game
	setjmp(LeaveGame);

	while (Function_mode != FMODE_EXIT)
	{
		val = DoMessageStuff(&msg);

		DDGRRESTORE;

		switch (Function_mode) 
		{
			case FMODE_MENU:
			{
				set_screen_mode(SCREEN_MENU);
				if (Auto_demo) {
					DEFINE_SCREEN(NULL);
					newdemo_start_playback(NULL);
					if (Newdemo_state != ND_STATE_PLAYBACK)	
						Error("No demo files were found for autodemo mode!");
				}
				else {
					DEFINE_SCREEN(Menu_pcx_name);
					mprintf((1, "%s\n", Menu_pcx_name));
					gr_palette_clear();
					DoMenu();
				}
				break;
			}
			case FMODE_GAME:
			{
				HideCursorW();
				DEFINE_SCREEN(NULL);
				game();
				digi_reset();
				digi_reset();
				if ( Function_mode == FMODE_MENU )
					songs_play_song( SONG_TITLE, 1 );
				mprintf((0, "Function mode = %d\n", Function_mode));
			}
		}

	}	

	dd_gr_set_current_canvas(NULL);
	dd_gr_clear_canvas(BM_XRGB(0,0,0));

	WriteConfigFile();

	return msg.wParam;
}


//	Window Message Functions 
//	----------------------------------------------------------------------------

void AppPaint(HWND hWnd, HDC hdc)
{
//	grwin_gdi_realizepal(hdc);
}

				
void AppSize(HWND hWnd)
{
	if (IsIconic(hWnd)) {
		_AppPaused = TRUE;
		InvalidateRect(hWnd, NULL, TRUE);
	}
	else if (GetForegroundWindow() == hWnd) {
		_AppPaused = FALSE;
	}
	  
	DDResizeViewport();
	mprintf((0, "Resized window (%d,%d,%d,%d).\n", ViewportRect.left, ViewportRect.top, ViewportRect.right, ViewportRect.bottom));	
}


void AppActivate(HWND hWnd, UINT wParam)
{
	_AppActive = (BOOL)(wParam) && GetForegroundWindow()==hWnd && !IsIconic(hWnd);
			
	if (_AppActive) {
		mprintf((0, "Descent II is gaining Window focus.\n"));
		cprintf("Descent II is gaining Window focus.\n");
	}
	else {
		mprintf((0, "Descent II is losing Window focus.\n"));
		cprintf("Descent II is losing Window focus.\n");
	}

// Must now unpause if paused and app is active now.
	mprintf((0, "AppActivate: Active: %d, Paused: %d\n", _AppActive, _AppPaused));

	if (_AppActive && AllowActivates && !_AppPaused) 
	{
		D2Restore();
	}
	else if (AllowActivates) {
		D2Shutdown();
	}

}


void AppQueryPalette(HWND hWnd)
{
//	if (!_DDFullScreen && _lpDDPalette && _lpDDSPrimary) {
//		gr_palette_load(gr_palette);
//	}
	mprintf((0, "QUERY PALETTE!\n"));
}


void AppCreate(HWND hWnd)
{
	HMENU hMenu;

// Remove system menu components not good
	hMenu = GetSystemMenu(hWnd, FALSE);
	
	RemoveMenu(hMenu, SC_SIZE, MF_BYCOMMAND);
	RemoveMenu(hMenu, SC_MOVE, MF_BYCOMMAND);
}	


void AppPaletteChange(HWND hWnd, UINT wParam)
{
	if ((HWND)wParam != hWnd) {
		if (!_DDFullScreen) {
			mprintf((0, "Palette changed!\n"));
			_AppPaused = TRUE;
			mprintf((0, "Descent 2 is paused.\n"));
		}	
	}
}

//static LPDIRECTDRAWSURFACE Page0Buffer = 0;

void AppDisplayChange(HWND hWnd, int w, int h, int bpp)
{	  
//	mprintf((0, "Windows feels a display change coming...\n"));
}


BOOL AppHandleSystemKeys(UINT msg, UINT wParam, UINT lParam)
{		 
//	Intercept ALT keystrokes

	if (msg == WM_SYSKEYUP) {
		mprintf((0,"ALT key down!"));
	}

	send_key_msg(msg, wParam, lParam);

	if (wParam == VK_TAB || wParam == VK_ESCAPE) return FALSE;

	return TRUE;
}


int RestoreGameSurfaces()
{
	if (dd_VR_offscreen_buffer) {
		if (!dd_VR_offscreen_buffer->lpdds) return 1;
		if (IDirectDrawSurface_Restore(dd_VR_offscreen_buffer->lpdds) != DD_OK)	 {
			mprintf((1, "Warning: Unable to restore dd_VR_offscreen_buffer!\n"));
			return 0;
		}
		mprintf((0, "Restored dd_VR_offscreen_buffer\n"));
	}

	return 1;
}


LRESULT WINAPI _export DescentWndProc(HWND hWnd,
								UINT msg, 
								UINT wParam,
								LPARAM lParam)
{
	//HANDLE h;
	//static int nTimer = 0;

	switch (msg)
	{
		case WM_CREATE:
			AppCreate(hWnd);
			return 0;

		case WM_SYSCOMMAND:
			if((wParam&0xFFF0)==SC_SCREENSAVE || (wParam&0xFFF0)==SC_MONITORPOWER)
            return 0;

			if ((wParam&0xfff0)== SC_KEYMENU && _AppActive) {
				mprintf((0, "W32: Bypassing system menu.\n"));
				return 0;
			}
			break;

		case WM_SIZE:
		case WM_MOVE:
			AppSize(hWnd);
			return 0;

		case WM_ACTIVATEAPP:					// Application is losing focus.
			AppActivate(hWnd, wParam);
			return 0;

		case WM_QUERYNEWPALETTE:
			AppQueryPalette(hWnd);
			break;

		case WM_PALETTECHANGED:
			AppPaletteChange(hWnd, wParam);
			break;

		case WM_DISPLAYCHANGE:				// Screen Mode is changing.
			AppDisplayChange(hWnd, LOWORD(lParam), HIWORD(lParam), wParam);
			break;

		case WM_SYSKEYUP:
		case WM_SYSKEYDOWN:
			if (AppHandleSystemKeys(msg, wParam,lParam))
				return 0;
			break;

		case WM_KEYDOWN:
		case WM_KEYUP:
			if (wParam == VK_SNAPSHOT) {
				mprintf((0, "Capture screen begins...\n"));
				save_screen_shot(0);
				if (!clipboard_screenshot()) 
					mprintf((1, "Failed to create clipboard screenshot!\n"));
			}
//		#ifndef NDEBUG
//			if (msg == WM_KEYDOWN && wParam == 0x49) ipx_debug_hold_packets = 1;
//			if (msg == WM_KEYUP && wParam == 0x49) ipx_debug_hold_packets = 0;
//		#endif
			send_key_msg(msg, wParam, lParam);
			break;
	
		case WM_PAINT:
//			hdc = BeginPaint(hWnd, &ps);			
//			AppPaint(hWnd, hdc);
//			EndPaint(hWnd, &ps);
//			return 0;
			break;

		case MM_JOY1MOVE:
		case MM_JOY1BUTTONDOWN:
		case MM_JOY1BUTTONUP:
			if (!_AppPaused && (_AppActive || !_DDFullScreen))	{
				joy_handler_win(hWnd, msg, wParam, lParam);
			}
			break;			

		case WM_DESTROY:
			mprintf((0, "Killing main window...\n"));
			PostQuitMessage(0);
			break;
	}

//	digi_midi_debug();
	mouse_win_callback(msg, wParam, lParam);
	
	return DefWindowProc(hWnd, msg, wParam, lParam);
}


static char *WinArgs[64];

//	Main Program
//	----------------------------------------------------------------------------

int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, 
						 int nCmdShow)
{
	int retval = 1;
	int argc;


	argc = ParseArgs(WinArgs, szCmdLine);
	InitArgs(argc, WinArgs);

	if (FindArg( "-?" ) || FindArg( "-help" ) || FindArg( "?" ) ) {
		print_commandline_help();
		set_exit_message("");
		return(0);
	}

	if (AppInit(hInst, hPrev, szCmdLine, nCmdShow)) {
		retval = RunGame(argc, WinArgs);
	}

	return retval;
}



//	Low Level Routines
//	----------------------------------------------------------------------------

static char *NoMovieArg = "-nomovies";
static char *PathArg = "descentw.exe";

int ParseArgs(char *argv[], LPSTR lpCmdLine)
{
	int argc;
	int i;

	argc = 0;
	i = 0;

	argv[argc++] = PathArg;

//	while (lpCmdLine[i] == '-' || lpCmdLine[i] == '/' )

	while (lpCmdLine[i])
	{
		if (lpCmdLine[i] == ' ') i++;
		else {
			argv[argc++] = &lpCmdLine[i];

			while (lpCmdLine[i]!=' ' && lpCmdLine[i]!= 0) {
				//mprintf((0, "%c", lpCmdLine[i]));
				i++;
			}
			lpCmdLine[i++] = 0;
//			mprintf((0, "Argument %s.\n", argv[argc-1]));
		}
	}

	if (!WinEnableMovies) 
		argv[argc++] = NoMovieArg;

	return argc;
}
					

extern void rls_stretch_scanline_asm();


void MakeCodeWritable(void)
{
	typedef DWORD __stdcall translator(DWORD);

	HMODULE 	modCode;
	BYTE 		*pImageBase = 0;
	DWORD		version;

	modCode = GetModuleHandle(0);
	version = GetVersion();

//	For Win32s we have to use undocumented features
	if ((version < 0x80000000) || ((BYTE)version >=0x04)) {  
	//	 Win NT, Win95.
		pImageBase = (BYTE *)modCode;
	}
	else {
		MessageBox(NULL, "Descent II cannot run under Windows 3.x", "Descent II", MB_OK);
		exit(1);

	}

	if (pImageBase) {
		DWORD oldRights;
		//char buf[256];
		IMAGE_OPTIONAL_HEADER *pHeader = 
			(IMAGE_OPTIONAL_HEADER *)(pImageBase + ((IMAGE_DOS_HEADER *)pImageBase)->e_lfanew + 
											 sizeof(IMAGE_NT_SIGNATURE) + sizeof(IMAGE_FILE_HEADER));

//@@		if (!VirtualProtect(pImageBase+pHeader->BaseOfCode, pHeader->SizeOfCode,
//@@							 	  PAGE_READWRITE, &oldRights))  {
//@@				exit(1);
//@@		}

		if (!VirtualProtect(rls_stretch_scanline_asm, 4096*2, PAGE_READWRITE, &oldRights))  {
			MessageBox(NULL, "Unable to gain write access to requested page.", "Descent II Error", MB_OK);
			exit(1);
		}

//		wsprintf(buf, "Start:%x FN1:%x",(DWORD)pImageBase+pHeader->BaseOfCode, (DWORD)rls_stretch_scanline_asm);
//		MessageBox(NULL, buf, "Info", MB_OK);
	}
}


void ValidateSystem()
{
	DWORD version;

	version = GetVersion();

	if ((version < 0x80000000) || ((BYTE)version >=0x04)) {
		if ((version < 0x80000000)  && ((BYTE)version >= 0x04)) {
			Platform_system = WINNT_PLATFORM;
			DD_Emulation = 1;
		}	 
		else Platform_system = WIN95_PLATFORM;
	}
	else {
		MessageBox(NULL, "Descent II will not run under Windows 3.x", "DESCENT II", MB_OK);
		exit(1);
	}

//	Do the named mutex thingy.
	{
		HANDLE hMutex, hMutex2;

		hMutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, "BigRedStinksMutex");
		if (hMutex) {
			hMutex2 = OpenMutex(MUTEX_ALL_ACCESS, FALSE, "BigRedSucksMutex");
			if (hMutex2) {
				CloseHandle(hMutex2);
				exit(1);				// Don't even tell user he's trying to run a third copy
			}

			hDescent2Mutex2 = CreateMutex(NULL, FALSE, "BigRedSucksMutex");
			if (!hDescent2Mutex2) {
				MessageBox(NULL, "Unable to create second mutex object.", "Descent II error", MB_OK);
				exit(1);
			}
			MessageBox(NULL, "You may only run one instance of Descent II at a time.", 
					"Descent II error", MB_OK);
			CloseHandle(hMutex);
			exit(1);
		}
		hDescent2Mutex = CreateMutex(NULL, FALSE, "BigRedStinksMutex");
		if (!hDescent2Mutex) {
			MessageBox(NULL, "Unable to create mutex object.", "Descent II error", MB_OK);
			exit(1);
		}
	}
//	Do the Version ID thing for Beta testers
#if defined(RELEASE) && defined(VERSION_NAME) && !defined(NDEBUG)
	{ 
		char buf[256];

		sprintf(buf, "Descent II %s v%d.%d\n%s\n%s %s", VERSION_TYPE, Version_major, Version_minor,
															VERSION_NAME, __DATE__, __TIME__);
		MessageBox(NULL, buf, "Descent II beta information", MB_OK);
	}
#endif

//	Super memory check
	{
		MEMORYSTATUS memstat;
		int mem_free;

		memset(&memstat, 0, sizeof(memstat));
		memstat.dwLength = sizeof(memstat);
		GlobalMemoryStatus(&memstat);

		logentry("Memory: \n");
		logentry("  MemoryLoad     = %10d\n",memstat.dwMemoryLoad);
		logentry("  TotalPhys      = %10d\n",memstat.dwTotalPhys);
		logentry("  AvailPhys      = %10d\n",memstat.dwAvailPhys);
		logentry("  TotalPageFile  = %10d\n",memstat.dwTotalPageFile);
		logentry("  AvailPageFile  = %10d\n",memstat.dwAvailPageFile);
		logentry("  TotalVirtual   = %10d\n",memstat.dwTotalVirtual);
		logentry("  AvailVirtual   = %10d\n",memstat.dwAvailVirtual);

		mem_free = memstat.dwTotalPhys;
//		mem_free=100;
//		logentry("  mem_free       = %10d\n",mem_free);
		
		if ( mem_free < MIN_PHYSICAL_MEM_FREE) {
			logentry("Using low memory option.\n");
			piggy_low_memory = 1;
		}

		if ( memstat.dwAvailPageFile < MIN_VIRTUAL_MEM_FREE) {
			MessageBox(NULL, "Not enough virtual memory.\n You need at least 16MB of free drive space.\n", 
							"Descent II Error", MB_OK);
			exit(1);
		}

		if ( ((mem_free > SOUND_22K_CUTOFF) || FindArg("-sound22k")) && !FindArg("-sound11k") && !FindArg("-lowmem") ) {
			logentry("Using 22KHz sounds\n");
			digi_sample_rate = SAMPLE_RATE_22K;
		} else {
			logentry("Using 11KHz sounds\n");
			digi_sample_rate = SAMPLE_RATE_11K;
		}
	}
}	


int DoMessageStuff(MSG *msg)
{

	while(1)
	{
		if (PeekMessage(msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg->message == WM_QUIT) {
				WriteConfigFile();
				exit(0);
			}

			TranslateMessage(msg);
			DispatchMessage(msg);
		}
		else if (!_AppPaused && (_AppActive || !_DDFullScreen)) {
			break;
		} else {

			if ((Game_mode & GM_MULTI)) {
				if (Control_center_destroyed) {
					ShowWindow(_hAppWnd, SW_MAXIMIZE);
					WinDelayIdle();
					SetForegroundWindow(_hAppWnd);
					WinDelayIdle();
					_RedrawScreen = TRUE;
				}
				else if (multi_menu_poll() == -1) {
					ShowWindow(_hAppWnd, SW_MAXIMIZE);
					WinDelayIdle();

					if (GetForegroundWindow() == _hAppWnd) {
						if (_AppPaused || !_AppActive) {
							mprintf((0, "Descent2 Daemon...Pause: %d, Active %d\n",_AppPaused, _AppActive)); 
							SetActiveWindow(_hAppWnd);
							_RedrawScreen = FALSE;
							break;
						}
					}
					else {
						SetForegroundWindow(_hAppWnd);
						WinDelayIdle();
						_RedrawScreen = FALSE;
					}
				}
			}
			else {
				mprintf((0, "Waiting...Pause: %d, Active %d\n",_AppPaused, _AppActive)); 
				WaitMessage();
			}
		}
	}
		

	return MSG_NORMAL;
}


void WinDelayIdle()
{
	MSG msg;

	mprintf((0, "Entering Idle message loop...\n"));

	while (1)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				WriteConfigFile();
				exit(0);
			}

			mprintf((0, "Retreiving message(0x%x:0x%x,0x%x) in idle loop.\n",msg.message, msg.wParam, msg.lParam));

			TranslateMessage(&msg);
			DispatchMessage(&msg);

		}
		else break;
	}
}


void WinDelay(int msecs)
{
	int timeout;
	MSG msg;


	timeout = timeGetTime() + msecs;

	while (timeout > timeGetTime())
	{
		DoMessageStuff(&msg);
		if (msg.message == WM_QUIT) exit(0);
	}
}


void	WErrorCatch(char *message)
{
	mprintf((1, "Bye Bye: %s\n", message));

	strcpy(WinErrorMessage, message);

	WinErrorTrap = 1;

	logentry("%s", WinErrorMessage);
	CloseWindow(_hAppWnd);
}


//static BOOL CursorVisible = TRUE;
static HANDLE CursorHandle = NULL;


void LoadCursorWin(int cursor)
{
	switch (cursor)
	{
		case MOUSE_DEFAULT_CURSOR:
			CursorHandle = LoadCursor(NULL, IDC_ARROW);
			break;
		
		case MOUSE_WAIT_CURSOR:	
			CursorHandle = LoadCursor(NULL, IDC_WAIT);
			break;
		
		default:
			CursorHandle = NULL;	
	}

	SetCursor(CursorHandle);
}
 

void HideCursorW()
{					 
	while (ShowCursor(FALSE) >=0);
}
 

void ShowCursorW()
{
	if (DD_Emulation) return;
	
	if (CursorHandle == NULL)
		LoadCursorWin(MOUSE_DEFAULT_CURSOR);

	while (ShowCursor(TRUE) < 0);
}	



//	Descent 2 Shutdown and Restore functions
//	----------------------------------------------------------------------------

void SaveVideoState();
void RestoreVideoState();

static int Saved_Game_window_w;
static int Saved_Game_window_h;
static int SavedScreenMode = 0;

extern int Digi_initialized;
extern int Current_display_mode;
extern BOOL WMVEPlaying, RMVEPlaying;
extern int current_song_level;


void RestoreScreenContext()
{
	if ( _SCRContext.bkg_filename != NULL )	{
		mprintf((0, "Loading background file...\n"));
		nm_draw_background1( _SCRContext.bkg_filename );
	}
	gr_palette_load(gr_palette);
}


void D2Shutdown()
{
	if (GameShutdown) return;

	_AppPaused = TRUE;

	if (!Joystick_calibrating) joy_stop_poll();

	key_flush();

	SaveVideoState();

	songs_stop_all();

	if (CD_audio_desktop_dev != -1) 
		auxSetVolume(CD_audio_desktop_dev, CD_audio_desktop_vol);

	if (Function_mode == FMODE_GAME && !(Game_mode &GM_MULTI)) {
		stop_time();
	}

	mprintf((0, "WMVE: %d, RMVE: %d\n", WMVEPlaying, RMVEPlaying));

	if (WMVEPlaying) MovieShutdown();
	if (RMVEPlaying) MVE_rmHoldMovie();

	if (!WMVEPlaying) digi_reset();

	mprintf((0, "Descent 2 is asleep...\n"));

	GameShutdown = TRUE;
}


void D2Restore()
{
	if (!GameShutdown) return;
	
	if (!WMVEPlaying) digi_reset();
 	if (WMVEPlaying) {
		MovieRestore();
		key_flush();
		if (CD_audio_desktop_dev != -1) 
			auxGetVolume(CD_audio_desktop_dev, &CD_audio_desktop_vol);
		_AppPaused = FALSE;
		goto EndD2Restore;
	}
	else if (RMVEPlaying) {
		key_flush();
	//	if (_lpDD) RestoreVideoState();
		dd_gr_init_screen();
		W95DisplayMode = -1;

		if (SavedScreenMode != -1)  
			set_screen_mode(SavedScreenMode);

		if (CD_audio_desktop_dev != -1) 
			auxGetVolume(CD_audio_desktop_dev, &CD_audio_desktop_vol);
		MVE_rmHoldMovie();
		_RedrawScreen = TRUE;
		_AppPaused = FALSE;
		goto EndD2Restore;
	}
	else {
		if (_lpDD) RestoreVideoState();

		if (CD_audio_desktop_dev != -1) 
			auxGetVolume(CD_audio_desktop_dev, &CD_audio_desktop_vol);
																		   
		if (Function_mode == FMODE_MENU) songs_play_song(SONG_TITLE, 1);
		if (Function_mode == FMODE_GAME) {
			songs_play_level_song(current_song_level);
		}
		key_flush();
		_AppPaused = FALSE;

	}

	if (Function_mode == FMODE_GAME && !(Game_mode & GM_MULTI)) start_time();
	else if (Function_mode == FMODE_MENU) 
		keyd_time_when_last_pressed = timer_get_fixed_seconds();

EndD2Restore:
	if (!Joystick_calibrating) joy_start_poll();
	set_redbook_volume(Config_redbook_volume);
	mprintf((0, "...Descent 2 is awake!\n"));
	
	GameShutdown = FALSE;
}


void SaveVideoState()
{
// If in game mode, then restore window size
	if (Screen_mode == SCREEN_GAME) {
		Saved_Game_window_w = Game_window_w;
		Saved_Game_window_h = Game_window_h;

	}		

	SavedScreenMode = Screen_mode;
	Screen_mode = -1;
}


void RestoreVideoState()
{
//	Just reinitialize screen state.
	dd_gr_init_screen();
	W95DisplayMode = -1;


// If in game mode, then restore window size
	if (SavedScreenMode != -1)  
		set_screen_mode(SavedScreenMode);
																	
	if (SavedScreenMode == SCREEN_GAME) {
		Game_window_w = Saved_Game_window_w;
		Game_window_h = Saved_Game_window_h;
		init_cockpit();
	}
	if (Function_mode == FMODE_GAME) {
		load_palette(Current_level_palette,1,1);
	}				

	_RedrawScreen = TRUE;
	RestoreScreenContext();
}


