/*
 * $Source: /cvs/cvsroot/d2x/arch/ogl/wgl.c,v $
 * $Revision: 1.1 $
 * $Author: bradleyb $
 * $Date: 2001-10-25 08:25:34 $
 *
 * opengl platform specific functions for WGL - added by Peter Hawkins
 * fullscreen example code courtesy of Jeff Slutter
 * everything merged together and cleaned up by Matt Mueller
 *         (with some win32 help from Nirvana)
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2001/01/29 13:47:52  bradleyb
 * Fixed build, some minor cleanups.
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <windows.h>
#include <mmsystem.h>
#include "ogl_init.h"
#include "vers_id.h"
#include "error.h"
#include "key.h"
#include "joy.h"
#include "mouse.h"
#include "digi.h"
#include "args.h"
/*#include "event.h"*/


HINSTANCE hInst=NULL;
HWND g_hWnd=NULL;

extern int Inferno_verbose;

static int mouse_hidden=0;


//extern unsigned int key_wparam, key_lparam, key_msg;
void keyboard_handler();
extern int WMKey_Handler_Ready;

HDC hDC;

static int GLPREF_width,GLPREF_height;
static int GLSTATE_width,GLSTATE_height;
static bool GLPREF_windowed;

static HGLRC GL_ResourceContext=NULL;
//static WORD Saved_gamma_values[256*3];
bool OpenGL_Initialize(void);
void OpenGL_Shutdown(void);


void PumpMessages(void)
{
  MSG msg;

  while (PeekMessage(&msg,NULL,0,0,PM_REMOVE|PM_NOYIELD))
  {
	TranslateMessage(&msg);
	DispatchMessage(&msg);
  }
}
static void finiObjects()
{
//	ogl_close();
//   if(mouse_hidden){
//    ShowCursor(TRUE);
//	mouse_hidden=0;
//   }

 //  DisableOpenGL( g_hWnd, hDC, hRC );
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



void ogl_swap_buffers_internal(void){
	SwapBuffers( hDC );
}

int get_win_x_bs(void){
//	return GetSystemMetrics(SM_CXBORDER)*2
	return GetSystemMetrics(SM_CXFIXEDFRAME)*2;
}
int get_win_y_bs(void){
//	return GetSystemMetrics(SM_CYBORDER)*2+GetSystemMetrics(SM_CYCAPTION);
	return GetSystemMetrics(SM_CYFIXEDFRAME)*2+GetSystemMetrics(SM_CYCAPTION);
}
void win32_create_window(int x,int y)
{
	int flags;

	WNDCLASS wcDescentClass;

	if (!hInst)
		Error("hInst=NULL\n");


	wcDescentClass.lpszClassName = "WinD1X";
	wcDescentClass.hInstance     = hInst;
	wcDescentClass.lpfnWndProc   = DescentWndProc;
	wcDescentClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wcDescentClass.hIcon         = LoadIcon(NULL, IDI_WINLOGO);
	wcDescentClass.lpszMenuName  = NULL;
	wcDescentClass.hbrBackground = NULL;
	wcDescentClass.style         = CS_OWNDC;
	wcDescentClass.cbClsExtra    = 0;
	wcDescentClass.cbWndExtra    = 0;
		
	// Register the class
	if (!RegisterClass(&wcDescentClass)){
//		printf("RegisterClass==0?\n");
		//always seems to return 0 after the first time, yet if you remove the call, it crashes. Heh.
	}

	if (ogl_fullscreen)
		flags=WS_POPUP | WS_SYSMENU;
	else
		flags=WS_OVERLAPPED | WS_BORDER | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

	if (!ogl_fullscreen){
		x+=get_win_x_bs();y+=get_win_y_bs();
	}else{
		if (FindArg("-gl_test2")){
			x=GetSystemMetrics(SM_CXSCREEN);
			y=GetSystemMetrics(SM_CYSCREEN);
		}
	}
	g_hWnd = CreateWindowEx(0,
			"WinD1X",
			"Descent",
			flags,
			0, 0,
			x,y,
			NULL,
			NULL,
			hInst,
			NULL
						   );

	if (!g_hWnd) Error("no window?\n");
	ShowWindow(g_hWnd,SW_SHOWNORMAL);
	UpdateWindow(g_hWnd);

	if (ogl_fullscreen){
        ShowCursor(FALSE);
        mouse_hidden = 1;
	}

	key_init();
	if (!FindArg( "-nomouse" ))
		mouse_init(0);
	if (!FindArg( "-nojoystick" ))
		joy_init(JOYSTICKID1);
	if (!FindArg( "-nosound" ))
		digi_init();
//	printf("arch_init successfully completed\n");
		
	OpenGL_Initialize();
		
	gl_initialized=1;
}
void ogl_destroy_window(void){
	if (gl_initialized){
		ogl_smash_texture_list_internal();
		OpenGL_Shutdown();
		if (mouse_hidden){
			ShowCursor(TRUE);
			mouse_hidden = 0;
		}
		if (g_hWnd){
			key_close();
			if (!FindArg( "-nomouse" ))
				mouse_close();
			if (!FindArg( "-nojoystick" ))
				joy_close();
			if (!FindArg( "-nosound" ))
				digi_close();
			DestroyWindow(g_hWnd);
		}else
			Error("ogl_destroy_window: no g_hWnd?\n");
		gl_initialized=0;
	}
	return;
}

void ogl_do_fullscreen_internal(void){
	if (GLPREF_windowed==ogl_fullscreen){
		ogl_destroy_window();
		win32_create_window(GLPREF_width,GLPREF_height);
		ogl_vivify_texture_list_internal();
	}
}


int ogl_init_window(int x, int y){
	GLPREF_width=x;
	GLPREF_height=y;
	if (gl_initialized){
		if (GLSTATE_width==GLPREF_width && GLSTATE_height==GLPREF_height && GLPREF_windowed!=ogl_fullscreen)
			return 0;//we are already in the right mode, don't do anything.
		if (!ogl_fullscreen && GLPREF_windowed){
			SetWindowPos(g_hWnd,0,0,0,x+get_win_x_bs(),y+get_win_y_bs(),SWP_NOMOVE);
		}else{
			ogl_destroy_window();
			win32_create_window(x,y);
			ogl_vivify_texture_list_internal();
		}
	}else {
		win32_create_window(x,y);
	}
	return 0;
}
void ogl_init(void){
	hInst=GetModuleHandle (NULL);
}
void ogl_close(void){
	ogl_destroy_window();
}


//windows opengl fullscreen changing - courtesy of Jeff Slutter

/*

  Windows Full Screen Setup

  ===========================================================================
*/


// Entering this function, the following values must be valid
//		GLPREF_width,GLPREF_height: preferred width and height
//		GLPREF_windowed: do we want windowed or full screen mode
//		g_hWnd: handle to the window created for OpenGL
//	On exit from this function (if returned true) the following values will be set
//		GLSTATE_width,GLSTATE_height: real width and height of screen
//		hDC: device context of the window
//		GL_ResourceContext: OpenGL resource context
//		Saved_gamma_values: Initial gamma values
bool OpenGL_Initialize(void)
{
	char *errstr="";
	int width,height;
	int pf;
	PIXELFORMATDESCRIPTOR pfd;//, pfd_copy;

	GLPREF_windowed=!ogl_fullscreen;
	
	if (FindArg("-gl_test1")){
		GLSTATE_width = GLPREF_width;
		GLSTATE_height = GLPREF_height;
	}else{
		if(!GLPREF_windowed)
		{
			// First set our display mode
			// Create direct draw surface
			DEVMODE devmode;
			int retval;

			devmode.dmSize=sizeof(devmode);
			devmode.dmBitsPerPel=16;
			devmode.dmPelsWidth=GLPREF_width;
			devmode.dmPelsHeight=GLPREF_height;
			devmode.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

			retval=ChangeDisplaySettings(&devmode,0);

			if (retval!=DISP_CHANGE_SUCCESSFUL)
			{
				// we couldn't switch to the desired screen mode
				// fall back to 640x480
				retval=-1;
				devmode.dmBitsPerPel=16;
				devmode.dmPelsWidth=640;
				devmode.dmPelsHeight=480;
				devmode.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

				retval=ChangeDisplaySettings(&devmode,0);
				if (retval!=DISP_CHANGE_SUCCESSFUL)
				{
					errstr="ChangeDisplaySettings";
					// we couldn't even switch to 640x480, we're out of here
					// restore screen mode to default
					ChangeDisplaySettings(NULL,0);
					goto OpenGLError;
				}else
				{
					// successful, change our global settings to reflect what 
					// mode we are at
					GLPREF_width=640;
					GLPREF_height=480;
				}
			}else
			{
				// success at changing the video mode
			}
		}


		if(GLPREF_windowed)
		{
			// we want windowed mode, figure out how big the window is
			RECT rect;
			GetWindowRect(g_hWnd,&rect);
			width=abs(rect.right-rect.left);
			height=abs(rect.bottom-rect.top);
		}else
		{
			RECT rect;
			// full screen mode, we want the window to be on top of everything
			SetWindowPos(g_hWnd,HWND_TOPMOST,0,0,GLPREF_width,GLPREF_height,SWP_FRAMECHANGED);
			width=GLPREF_width;
			height=GLPREF_height;
			GetWindowRect(g_hWnd,&rect);
		}

		GLSTATE_width = width;
		GLSTATE_height = height;

	}
	
	hDC = GetDC(g_hWnd);
	
	// Now we finally setup OpenGL
	// If OpenGL is to be dynamically loaded, do this now (if the DLL isn't already
	// loaded)
	// remove the following error when you figure out what you want to do
	// it's put here to make sure you notice this
#ifdef OGL_RUNTIME_LOAD
	ogl_init_load_library();
#endif

	// Setup our pixel format
		
	memset(&pfd,0,sizeof(pfd));
	pfd.nSize        = sizeof(pfd);
	pfd.nVersion     = 1;
	pfd.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
//	pfd.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_GENERIC_ACCELERATED;
	pfd.iPixelType   = PFD_TYPE_RGBA;
	pfd.cColorBits = 16;
	pfd.cAlphaBits = 8;
	pfd.cDepthBits = 0;
	pfd.cAccumBits = 0;
	pfd.cStencilBits = 0;
	pfd.iLayerType = PFD_MAIN_PLANE;
	pfd.dwLayerMask = PFD_MAIN_PLANE;
					

	// Find the user's "best match" PFD 
	pf = ChoosePixelFormat(hDC,&pfd);
	if(pf == 0) 
	{
		errstr="ChoosePixelFormat";
		// no pixel format closely matches	
		goto OpenGLError;
	} 

	// Set the new PFD
	if(SetPixelFormat(hDC,pf,&pfd)==FALSE) 
	{
		errstr="SetPixelFormat";
		// unable to set the pixel format
		goto OpenGLError;
	}

	// Now retrieve the PFD, we need to check some things
/*	if(DescribePixelFormat(hDC,pf,sizeof(PIXELFORMATDESCRIPTOR),&pfd_copy)==0)
	{
		errstr="DescribePixelFormat";
		// unable to get the PFD
		goto OpenGLError;
	}

	// Make sure we are hardware accelerated
	if((pfd_copy.dwFlags&PFD_GENERIC_ACCELERATED)==0&&(pfd_copy.dwFlags&PFD_GENERIC_FORMAT)!=0)
	{
		// we are not hardware accelerated!
		goto OpenGLError;
	}*/

	// Finally, create our OpenGL context and make it current
	GL_ResourceContext = wglCreateContext(hDC);
	if(GL_ResourceContext==NULL)
	{
		errstr="wglCreateContext";
		// we couldn't create a context!
		goto OpenGLError;
	}

	// Make the context current
	wglMakeCurrent(hDC,GL_ResourceContext);

	// Save our gamma values because we'll probably be changing them,
	// this way we can restore them on exit

//	GetDeviceGammaRamp(hDC,(LPVOID)Saved_gamma_values);

	return true;

OpenGLError:
	// Shutdown OpenGL
	OpenGL_Shutdown();
	Error("opengl init error: %s\n",errstr);
	return false;
}

void OpenGL_Shutdown(void)
{
	// Do any needed OpenGL shutdown here


	// Now do Window specific shutdown
	if(wglMakeCurrent)//check to make sure the function is valid (dyanmic loaded OpenGL)
		wglMakeCurrent(NULL, NULL);

	if(wglDeleteContext)//check to make sure the function is valid (dyanmic loaded OpenGL)
		wglDeleteContext(GL_ResourceContext);
	
	// Restore back to user screen settings
	if(!GLPREF_windowed)
		ChangeDisplaySettings(NULL,0);

	// Restore gamma values

//	SetDeviceGammaRamp(hDC,(LPVOID)Saved_gamma_values);
	
	ReleaseDC(g_hWnd,hDC);
}
