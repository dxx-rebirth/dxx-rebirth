/* $Id: init.c,v 1.3 2004-05-20 23:06:24 btb Exp $ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <ddraw.h>
#include <mmsystem.h>
#include "args.h"
#include "resource.h"

#ifndef NDEBUG
#ifdef _MSC_VER
#include <crtdbg.h>
#endif
#endif

extern HINSTANCE hInst;
extern HWND g_hWnd;
extern LPDIRECTDRAW            lpDD;
extern LPDIRECTDRAWSURFACE     lpDDSPrimary;
extern LPDIRECTDRAWSURFACE     lpDDSOne;
extern LPDIRECTDRAWPALETTE     lpDDPal;

extern int Inferno_verbose;

static int mouse_hidden=0;

static void finiObjects()
{
	if(lpDD!=NULL)
	{
		if(lpDDSPrimary!=NULL)
		{           
			IDirectDrawSurface_Release(lpDDSPrimary);
			lpDDSPrimary=NULL;
		}
		if(lpDDSOne!=NULL)
		{
			IDirectDrawSurface_Unlock(lpDDSOne,NULL);
			IDirectDrawSurface_Release(lpDDSOne);
			lpDDSOne=NULL;
		}
		if(lpDDPal!=NULL)
		{
			IDirectDrawSurface_Release(lpDDPal);
			lpDDPal=NULL;
		}
		IDirectDrawSurface_Release(lpDD);
		lpDD=NULL;
	}
   if(mouse_hidden)
    ShowCursor(TRUE);
} 

//extern unsigned int key_wparam, key_lparam, key_msg;
void keyboard_handler();
extern int WMKey_Handler_Ready;

void PumpMessages(void)
{
  MSG msg;

  while (PeekMessage(&msg,NULL,0,0,PM_REMOVE|PM_NOYIELD))
  {
	TranslateMessage(&msg);
	DispatchMessage(&msg);
  }
}

long PASCAL DescentWndProc(HWND hWnd,UINT message,
						   WPARAM wParam,LPARAM lParam )
{
  switch(message)
  {

   case WM_KEYDOWN:
   case WM_KEYUP:
	if (WMKey_Handler_Ready) {
//      key_wparam=wParam; key_lparam=lParam; key_msg=message;
	  keyboard_handler();
	}
	break;
   case WM_MOUSEMOVE:
   case WM_LBUTTONDOWN:
   case WM_LBUTTONUP:
   case WM_RBUTTONDOWN:
   case WM_RBUTTONUP:
   case WM_NCMOUSEMOVE:
   case WM_NCLBUTTONDOWN:
   case WM_NCLBUTTONUP:
   case WM_NCRBUTTONDOWN:
   case WM_NCRBUTTONUP:
	 break;
   case WM_PALETTECHANGED:
   case WM_PALETTEISCHANGING:
   return 0;
   case WM_ACTIVATEAPP:
//     Win32_Key_Hook(wParam);
// DPH: This doesn't work... no idea why not...
	 break;
   case WM_DESTROY:
	 finiObjects();
	 PostQuitMessage(0);
	 break;
  }
  return DefWindowProc(hWnd,message,wParam,lParam);
}

void arch_init_start()
{
	#ifndef NDEBUG
	#ifdef _MSC_VER
	if (FindArg("-memdbg"))
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | /* _CRTDBG_CHECK_ALWAYS_DF | */
			/*_CRTDBG_CHECK_CRT_DF |*/
			_CRTDBG_DELAY_FREE_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	#endif
	#endif
}

extern void key_init(void);
extern void mouse_init(void);
//added/changed 3/7/99 Owen Evans (next line)
extern void joy_init(int joyid);

void arch_init()
{
	HRESULT             ddrval;

	WNDCLASS wcDescentClass;

        wcDescentClass.lpszClassName = "WinD1X";
	wcDescentClass.hInstance     = hInst;
	wcDescentClass.lpfnWndProc   = DescentWndProc;
	wcDescentClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	//wcDescentClass.hIcon         = LoadIcon(NULL, IDI_WINLOGO);
	wcDescentClass.hIcon         = LoadIcon(hInst, MAKEINTRESOURCE(IDI_MAIN_ICON));
	wcDescentClass.lpszMenuName  = NULL;
	wcDescentClass.hbrBackground = NULL;
	wcDescentClass.style         = CS_HREDRAW | CS_VREDRAW;
	wcDescentClass.cbClsExtra    = 0;
	wcDescentClass.cbWndExtra    = 0;

	// Register the class
	RegisterClass(&wcDescentClass);
	g_hWnd = CreateWindowEx(0,
                                  "WinD1X",
				  "Descent",
				  WS_OVERLAPPED | WS_BORDER,
				  0, 0,
				  GetSystemMetrics(SM_CXSCREEN),
				  GetSystemMetrics(SM_CYSCREEN),
				  NULL,
				  NULL,
				  hInst,
				  NULL
				  );

	if (!g_hWnd) return; // CRAP!
	ShowWindow(g_hWnd,SW_SHOWNORMAL);
	UpdateWindow(g_hWnd);


	ddrval=DirectDrawCreate(NULL,&lpDD,NULL);

	if(ddrval!=DD_OK)
	{
		fprintf(stderr,"DirectDrawCreate() failed!\n");
		abort();
	}

	if (FindArg("-semiwin"))
	ddrval=IDirectDraw_SetCooperativeLevel(lpDD,g_hWnd,DDSCL_NORMAL);
	else
	{
		#ifndef NDEBUG
		ddrval=IDirectDraw_SetCooperativeLevel(lpDD,g_hWnd,
			DDSCL_EXCLUSIVE|DDSCL_FULLSCREEN|DDSCL_ALLOWREBOOT);
#else
		ddrval=IDirectDraw_SetCooperativeLevel(lpDD,g_hWnd,
			DDSCL_EXCLUSIVE|DDSCL_FULLSCREEN|DDSCL_ALLOWREBOOT);
#endif
	}
	
	if (ddrval!=DD_OK)
	{
	  fprintf(stderr,"SetCooperativeLevel() failed\n");
	  abort();
	}

        ShowCursor(FALSE);
        mouse_hidden = 1;

	SetPriorityClass(GetCurrentProcess(),HIGH_PRIORITY_CLASS);

	key_init();
	mouse_init();
//added/changed 3/7/99 Owen Evans (next line)
        joy_init(JOYSTICKID1);
	printf("arch_init successfully completed\n");
}
