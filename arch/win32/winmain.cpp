//-----------------------------------------------------------------------------
// File: WinMain.cpp
//
// Desc: Windows code for Direct3D samples
//
//       This code uses the Direct3D sample framework.
//
//
// Copyright (c) 1996-1998 Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

#include "pch.h"
#include "WinMain.h"
#include "D3DFrame.h"
#include "D3DEnum.h"
#include "D3DUtil.h"
#include "resource.h"

#include "scene.h"


//-----------------------------------------------------------------------------
// Global variables for using the D3D sample framework class
//-----------------------------------------------------------------------------
CD3DFramework* g_pFramework        = NULL;
BOOL           g_bActive           = FALSE;
BOOL           g_bReady            = FALSE;
BOOL           g_bFrameMoving      = TRUE;
BOOL           g_bSingleStep       = FALSE;
BOOL           g_bWindowed         = TRUE;
BOOL           g_bShowStats        = TRUE;
RECT           g_rcWindow;
HACCEL         g_hAccel;
HWND g_hWnd;

enum APPMSGTYPE { MSG_NONE, MSGERR_APPMUSTEXIT, MSGWARN_SWITCHTOSOFTWARE };




//-----------------------------------------------------------------------------
// Local function-prototypes
//-----------------------------------------------------------------------------
INT     CALLBACK AboutProc( HWND, UINT, WPARAM, LPARAM );
LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM );
HRESULT Initialize3DEnvironment( HWND );
HRESULT Change3DEnvironment( HWND );
HRESULT Render3DEnvironment();
VOID    Cleanup3DEnvironment();
VOID    DisplayFrameworkError( HRESULT, APPMSGTYPE );
VOID    AppShowStats();
VOID    AppOutputText( LPDIRECT3DDEVICE3, DWORD, DWORD, CHAR* );
VOID    AppPause( BOOL );




//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: Entry point to the program. Initializes everything, and goes into a
//       message-processing loop. Idle time is used to render the scene.
//-----------------------------------------------------------------------------
//INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
extern "C" INT InitMain ()
{
	HINSTANCE hInst = GetModuleHandle (NULL);

	// Register the window class
    WNDCLASS wndClass = { CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, hInst,
                          LoadIcon( hInst, MAKEINTRESOURCE(IDI_MAIN_ICON)),
                          LoadCursor(NULL, IDC_ARROW), 
                          (HBRUSH)GetStockObject(WHITE_BRUSH),
						  MAKEINTRESOURCE(IDR_MENU),
                          TEXT("Render Window") };
    RegisterClass( &wndClass );

    // Create our main window
    g_hWnd = CreateWindow( TEXT("Render Window"), g_strAppTitle,
                              WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                              CW_USEDEFAULT, 300, 300, 0L, 0L, hInst, 0L );

	RECT rect;
	if (GetClientRect (g_hWnd, &rect))
	{
		int cx = 320 + (300 - rect.right);
		int cy = 200 + (300 - rect.bottom);

		SetWindowPos (g_hWnd, NULL, 0, 0, cx, cy, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOZORDER);
	}

    ShowWindow( g_hWnd, SW_SHOWNORMAL );
    UpdateWindow( g_hWnd );

	// Save the window size/pos for switching modes
	GetWindowRect( g_hWnd, &g_rcWindow );

    // Load keyboard accelerators
    g_hAccel = LoadAccelerators( hInst, MAKEINTRESOURCE(IDR_MAIN_ACCEL) );

	// Enumerate available D3D devices, passing a callback that allows devices
	// to be accepted/rejected based on what capabilities the app requires.
	HRESULT hr;
	if( FAILED( hr = D3DEnum_EnumerateDevices( App_ConfirmDevice ) ) )
	{
		DisplayFrameworkError( hr, MSGERR_APPMUSTEXIT );
        return 0;
	}

	// Check if we could not get a device that renders into a window, which
	// means the display must be 16- or 256-color mode. If so, let's bail.
	D3DEnum_DriverInfo* pDriverInfo;
	D3DEnum_DeviceInfo* pDeviceInfo;
    D3DEnum_GetSelectedDriver( &pDriverInfo, &pDeviceInfo );
	if( FALSE == pDeviceInfo->bWindowed )
    {
		Cleanup3DEnvironment();
		DisplayFrameworkError( D3DFWERR_INVALIDMODE, MSGERR_APPMUSTEXIT );
		return 0;
    }

	// Initialize the 3D environment for the app
    if( FAILED( hr = Initialize3DEnvironment( g_hWnd ) ) )
    {
	    Cleanup3DEnvironment();
		DisplayFrameworkError( hr, MSGERR_APPMUSTEXIT );
        return 0;
    }

	g_bReady = TRUE;

	return 1;
}

void PumpMessages (void)
{
	MSG msg;
	while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
    {
        // Exit App ?!?
        if (msg.message == WM_QUIT)
        {
            return;
        }

		if (!TranslateAccelerator (g_hWnd, g_hAccel, &msg))
		{
			TranslateMessage (&msg);
			DispatchMessage (&msg);
		}
    }
}




//-----------------------------------------------------------------------------
// Name: WndProc()
// Desc: This is the basic Windows-programming function that processes
//       Windows messages. We need to handle window movement, painting,
//       and destruction.
//-----------------------------------------------------------------------------
LRESULT CALLBACK WndProc( HWND g_hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    switch( uMsg )
    {
        case WM_PAINT:
            if( g_pFramework )
			{
				// If we are paused, and in fullscreen mode, give the dialogs
				// a GDI surface to draw on.
				if( !g_bReady && !g_bWindowed)
					g_pFramework->FlipToGDISurface( TRUE );
				else // Simply repaint the frame's contents
					g_pFramework->ShowFrame();
			}
            break;

        case WM_MOVE:
            if( g_bActive && g_bReady && g_bWindowed )
			{
			    GetWindowRect( g_hWnd, &g_rcWindow );
                g_pFramework->Move( (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam) );
			}
            break;

        case WM_SIZE:
            // Check to see if we are losing our window...
            if( SIZE_MAXHIDE==wParam || SIZE_MINIMIZED==wParam )
                g_bActive = FALSE;
			else g_bActive = TRUE;

            // A new window size will require a new viewport and backbuffer
            // size, so the 3D structures must be changed accordingly.
            if( g_bActive && g_bReady && g_bWindowed )
			{
				g_bReady = FALSE;
				GetWindowRect( g_hWnd, &g_rcWindow );
				Change3DEnvironment( g_hWnd );
				g_bReady = TRUE;
			}
            break;

		case WM_GETMINMAXINFO:
			((MINMAXINFO*)lParam)->ptMinTrackSize.x = 100;
			((MINMAXINFO*)lParam)->ptMinTrackSize.y = 100;
			break;

        case WM_SETCURSOR:
            if( g_bActive && g_bReady && (!g_bWindowed) )
            {
                SetCursor(NULL);
                return TRUE;
            }
            break;

        case WM_CLOSE:
            DestroyWindow( g_hWnd );
            return 0;
        
        case WM_DESTROY:
            Cleanup3DEnvironment();
            PostQuitMessage(0);
            return 0L;

		case WM_ENTERMENULOOP:
			AppPause(TRUE);
			break;

		case WM_EXITMENULOOP:
			AppPause(FALSE);
			break;

		case WM_CONTEXTMENU:
			{
				HMENU hMenu = LoadMenu( 0, MAKEINTRESOURCE(IDR_POPUP) );
				TrackPopupMenuEx( GetSubMenu( hMenu, 0 ),
								  TPM_VERTICAL, LOWORD(lParam), 
								  HIWORD(lParam), g_hWnd, NULL );
			}
			break;

        case WM_COMMAND:
            switch( LOWORD(wParam) )
            {
				case SC_MONITORPOWER:
					// Prevent potential crashes when the monitor powers down
					return 1;

				case IDM_TOGGLESTART:
					g_bFrameMoving = !g_bFrameMoving;
					break;

				case IDM_SINGLESTEP:
					g_bSingleStep = TRUE;
					break;

                case IDM_CHANGEDEVICE:
                    // Display the driver-selection dialog box.
		            if( g_bActive && g_bReady )
					{
						AppPause(TRUE);
						if( g_bWindowed )
							GetWindowRect( g_hWnd, &g_rcWindow );

						HWND hWnd = g_hWnd;
						if( IDOK == D3DEnum_UserDlgSelectDriver( hWnd, g_bWindowed ) )
						{
							D3DEnum_DriverInfo* pDriverInfo;
							D3DEnum_DeviceInfo* pDeviceInfo;
							D3DEnum_GetSelectedDriver( &pDriverInfo, &pDeviceInfo );
							g_bWindowed = pDeviceInfo->bWindowed;

							Change3DEnvironment( g_hWnd );
						}
						AppPause(FALSE);
					}
                    return 0;

                case IDM_TOGGLEFULLSCREEN:
                    // Toggle the fullscreen/window mode
		            if( g_bActive && g_bReady )
					{
						g_bReady = FALSE;
						if( g_bWindowed )
							GetWindowRect( g_hWnd, &g_rcWindow );
			            g_bWindowed = !g_bWindowed;
						Change3DEnvironment( g_hWnd );
						g_bReady = TRUE;
					}
					return 0;

                case IDM_HELP:
					AppPause(TRUE);
                    DialogBox( (HINSTANCE)GetWindowLong( g_hWnd, GWL_HINSTANCE ),
                               MAKEINTRESOURCE(IDD_ABOUT), g_hWnd, (DLGPROC)AboutProc );
					AppPause(FALSE);
                    return 0;

                case IDM_EXIT:
                    // Recieved key/menu command to exit app
                    SendMessage( g_hWnd, WM_CLOSE, 0, 0 );
                    return 0;
            }
            break;
    }

    return DefWindowProc( g_hWnd, uMsg, wParam, lParam );
}
            



//-----------------------------------------------------------------------------
// Name: AboutProc()
// Desc: Minimal message proc function for the about box
//-----------------------------------------------------------------------------
BOOL CALLBACK AboutProc( HWND g_hWnd, UINT uMsg, WPARAM wParam, LPARAM )
{
    if( WM_COMMAND == uMsg )
		if( IDOK == LOWORD(wParam) || IDCANCEL == LOWORD(wParam) )
			EndDialog (g_hWnd, TRUE);

    return ( WM_INITDIALOG == uMsg ) ? TRUE : FALSE;
}




//-----------------------------------------------------------------------------
// Note: From this point on, the code is DirectX specific support for the app.
//-----------------------------------------------------------------------------




//-----------------------------------------------------------------------------
// Name: AppInitialize()
// Desc: Initializes the sample framework, then calls the app-specific function
//       to initialize device specific objects. This code is structured to
//       handled any errors that may occur duing initialization
//-----------------------------------------------------------------------------
HRESULT AppInitialize( HWND g_hWnd )
{
	D3DEnum_DriverInfo* pDriverInfo;
	D3DEnum_DeviceInfo* pDeviceInfo;
    DWORD   dwFrameworkFlags = 0L;
	HRESULT hr;

    D3DEnum_GetSelectedDriver( &pDriverInfo, &pDeviceInfo );

    dwFrameworkFlags |= (!g_bWindowed         ? D3DFW_FULLSCREEN : 0L );
    dwFrameworkFlags |= ( g_bAppUseZBuffer    ? D3DFW_ZBUFFER    : 0L );
    dwFrameworkFlags |= ( g_bAppUseBackBuffer ? D3DFW_BACKBUFFER : 0L );

	// Initialize the D3D framework
    if( SUCCEEDED( hr = g_pFramework->Initialize( g_hWnd, &pDriverInfo->guid,
		               &pDeviceInfo->guid, &pDeviceInfo->pCurrentMode->ddsd, 
		               dwFrameworkFlags ) ) )
	{
		// Let the app run its startup code which creates the 3d scene.
		if( SUCCEEDED( hr = App_InitDeviceObjects( g_pFramework->GetD3DDevice(),
			                                    g_pFramework->GetViewport() ) ) )
			return S_OK;
		else
		{
			App_DeleteDeviceObjects( g_pFramework->GetD3DDevice(),
		                             g_pFramework->GetViewport() );
			g_pFramework->DestroyObjects();
		}
	}

	// If we get here, the first initialization passed failed. If that was with a
	// hardware device, try again using a software rasterizer instead.
	if( pDeviceInfo->bIsHardware )
	{
		// Try again with a software rasterizer
		DisplayFrameworkError( hr, MSGWARN_SWITCHTOSOFTWARE );
		D3DEnum_SelectDefaultDriver( D3DENUM_SOFTWAREONLY );
		return AppInitialize( g_hWnd );
	}

	return hr;
}




//-----------------------------------------------------------------------------
// Name: Initialize3DEnvironment()
// Desc: Called when the app window is initially created, this triggers
//       creation of the remaining portion (the 3D stuff) of the app.
//-----------------------------------------------------------------------------
HRESULT Initialize3DEnvironment( HWND g_hWnd )
{
	HRESULT hr;

	// Initialize the app
	if( FAILED( hr = App_OneTimeSceneInit( g_hWnd ) ) )
		return E_FAIL;

    // Create a new CD3DFramework class. This class does all of our D3D
    // initialization and manages the common D3D objects.
    if( NULL == ( g_pFramework = new CD3DFramework() ) )
		return E_OUTOFMEMORY;

	// Finally, initialize the framework and scene.
	return AppInitialize( g_hWnd );
}

    


//-----------------------------------------------------------------------------
// Name: Change3DEnvironment()
// Desc: Handles driver, device, and/or mode changes for the app.
//-----------------------------------------------------------------------------
HRESULT Change3DEnvironment( HWND g_hWnd )
{
	HRESULT hr;
    
    // Release all objects that need to be re-created for the new device
    App_DeleteDeviceObjects( g_pFramework->GetD3DDevice(), 
                             g_pFramework->GetViewport() );

	// Release the current framework objects (they will be recreated later on)
	if( FAILED( hr = g_pFramework->DestroyObjects() ) )
	{
		DisplayFrameworkError( hr, MSGERR_APPMUSTEXIT );
        DestroyWindow( g_hWnd );
		return hr;
	}

	// In case we're coming from a fullscreen mode, restore the window size
	if( g_bWindowed )
	{
		SetWindowPos( g_hWnd, HWND_NOTOPMOST, g_rcWindow.left, g_rcWindow.top,
                      ( g_rcWindow.right - g_rcWindow.left ), 
                      ( g_rcWindow.bottom - g_rcWindow.top ), SWP_SHOWWINDOW );
	}

    // Inform the framework class of the driver change. It will internally
    // re-create valid surfaces, a d3ddevice, and a viewport.
	if( FAILED( hr = AppInitialize( g_hWnd ) ) )
	{
		DisplayFrameworkError( hr, MSGERR_APPMUSTEXIT );
		DestroyWindow( g_hWnd );
		return hr;
	}

	// Trigger the rendering of a frame and return
	g_bSingleStep = TRUE;
	return S_OK;
}

FLOAT g_fTime;

extern "C" HRESULT Win32_start_frame ()
{
	// Check the cooperative level before rendering
	if( FAILED( g_pFramework->GetDirectDraw()->TestCooperativeLevel() ) )
		return S_OK;

	// Get the current time
	g_fTime = GetTickCount() * 0.001f;

	return App_StartFrame( g_pFramework->GetD3DDevice(),
		                   g_pFramework->GetViewport(), 
                           (D3DRECT*)g_pFramework->GetViewportRect() );
}

extern "C" HRESULT Win32_end_frame ()
{
	// Show the frame rate, etc.
	if( g_bShowStats )
		AppShowStats();

	return App_EndFrame ();
}

extern "C" HRESULT Win32_flip_screens ()
{
    // Show the frame on the primary surface.
    if( DDERR_SURFACELOST == g_pFramework->ShowFrame() )
    {
		g_pFramework->RestoreSurfaces();
		App_RestoreSurfaces();
	}
	return S_OK;
}



//-----------------------------------------------------------------------------
// Name: Cleanup3DEnvironment()
// Desc: Cleanup scene objects
//-----------------------------------------------------------------------------
VOID Cleanup3DEnvironment()
{
    if( g_pFramework )
    {
        App_FinalCleanup( g_pFramework->GetD3DDevice(), 
                          g_pFramework->GetViewport() );

        SAFE_DELETE( g_pFramework );
    }
	g_bActive = FALSE;
	g_bReady  = FALSE;
}


  

//-----------------------------------------------------------------------------
// Name: AppPause()
// Desc: Called in to toggle the pause state of the app. This function
//       brings the GDI surface to the front of the display, so drawing
//       output like message boxes and menus may be displayed.
//-----------------------------------------------------------------------------
VOID AppPause( BOOL bPause )
{
    static DWORD dwAppPausedCount = 0L;

    if( bPause && 0 == dwAppPausedCount )
        if( g_pFramework )
            g_pFramework->FlipToGDISurface( TRUE );

    dwAppPausedCount += ( bPause ? +1 : -1 );

    g_bReady = (0==dwAppPausedCount);
}




//-----------------------------------------------------------------------------
// Name: AppShowStats()
// Desc: Shows frame rate and dimensions of the rendering device. Note: a 
//       "real" app wouldn't query the surface dimensions each frame.
//-----------------------------------------------------------------------------
VOID AppShowStats()
{
    static FLOAT fFPS      = 0.0f;
    static FLOAT fLastTime = 0.0f;
    static DWORD dwFrames  = 0L;

	// Keep track of the time lapse and frame count
	FLOAT fTime = GetTickCount() * 0.001f; // Get current time in seconds
	++dwFrames;

	// Update the frame rate once per second
	if( fTime - fLastTime > 1.0f )
    {
        fFPS      = dwFrames / (fTime - fLastTime);
        fLastTime = fTime;
        dwFrames  = 0L;
    }

    // Get dimensions of the render surface 
    DDSURFACEDESC2 ddsd;
    ddsd.dwSize = sizeof(DDSURFACEDESC2);
    g_pFramework->GetRenderSurface()->GetSurfaceDesc(&ddsd);

    // Setup the text buffer to write out
    CHAR buffer[80];
    sprintf( buffer, "%7.02f fps (%dx%dx%d)", fFPS, ddsd.dwWidth,
             ddsd.dwHeight, ddsd.ddpfPixelFormat.dwRGBBitCount );
    AppOutputText( g_pFramework->GetD3DDevice(), 0, 0, buffer );
}




//-----------------------------------------------------------------------------
// Name: AppOutputText()
// Desc: Draws text on the window.
//-----------------------------------------------------------------------------
VOID AppOutputText( LPDIRECT3DDEVICE3 pd3dDevice, DWORD x, DWORD y, CHAR* str )
{
    LPDIRECTDRAWSURFACE4 pddsRenderSurface;
    if( FAILED( pd3dDevice->GetRenderTarget( &pddsRenderSurface ) ) )
        return;

    // Get a DC for the surface. Then, write out the buffer
    HDC hDC;
    if( SUCCEEDED( pddsRenderSurface->GetDC(&hDC) ) )
    {
        SetTextColor( hDC, RGB(255,255,0) );
        SetBkMode( hDC, TRANSPARENT );
        ExtTextOut( hDC, x, y, 0, NULL, str, strlen(str), NULL );
    
        pddsRenderSurface->ReleaseDC(hDC);
    }
    pddsRenderSurface->Release();
}




//-----------------------------------------------------------------------------
// Name: DisplayFrameworkError()
// Desc: Displays error messages in a message box
//-----------------------------------------------------------------------------
VOID DisplayFrameworkError( HRESULT hr, APPMSGTYPE errType )
{
	CHAR strMsg[512];

	switch( hr )
	{
		case D3DENUMERR_NOCOMPATIBLEDEVICES:
			strcpy( strMsg, TEXT("Could not find any compatible Direct3D\n"
				    "devices.") );
			break;
		case D3DENUMERR_SUGGESTREFRAST:
			strcpy( strMsg, TEXT("Could not find any compatible devices.\n\n"
		            "Try enabling the reference rasterizer using\n"
					"EnableRefRast.reg.") );
			break;
		case D3DENUMERR_ENUMERATIONFAILED:
			strcpy( strMsg, TEXT("Enumeration failed. Your system may be in an\n"
					"unstable state and need to be rebooted") );
			break;
		case D3DFWERR_INITIALIZATIONFAILED:
			strcpy( strMsg, TEXT("Generic initialization error.\n\nEnable "
				    "debug output for detailed information.") );
			break;
		case D3DFWERR_NODIRECTDRAW:
			strcpy( strMsg, TEXT("No DirectDraw") );
			break;
		case D3DFWERR_NODIRECT3D:
			strcpy( strMsg, TEXT("No Direct3D") );
			break;
		case D3DFWERR_INVALIDMODE:
			strcpy( strMsg, TEXT("This sample requires a 16-bit (or higher) "
			        "display mode\nto run in a window.\n\nPlease switch "
				    "your desktop settings accordingly.") );
			break;
		case D3DFWERR_COULDNTSETCOOPLEVEL:
			strcpy( strMsg, TEXT("Could not set Cooperative Level") );
			break;
		case D3DFWERR_NO3DDEVICE:
			strcpy( strMsg, TEXT("No 3D Device") );
			break;
		case D3DFWERR_NOZBUFFER:
			strcpy( strMsg, TEXT("No ZBuffer") );
			break;
		case D3DFWERR_NOVIEWPORT:
			strcpy( strMsg, TEXT("No Viewport") );
			break;
		case D3DFWERR_NOPRIMARY:
			strcpy( strMsg, TEXT("No primary") );
			break;
		case D3DFWERR_NOCLIPPER:
			strcpy( strMsg, TEXT("No Clipper") );
			break;
		case D3DFWERR_BADDISPLAYMODE:
			strcpy( strMsg, TEXT("Bad display mode") );
			break;
		case D3DFWERR_NOBACKBUFFER:
			strcpy( strMsg, TEXT("No backbuffer") );
			break;
		case D3DFWERR_NONZEROREFCOUNT:
			strcpy( strMsg, TEXT("Nonzerorefcount") );
			break;
		case D3DFWERR_NORENDERTARGET:
			strcpy( strMsg, TEXT("No render target") );
			break;
		case E_OUTOFMEMORY:
			strcpy( strMsg, TEXT("Not enough memory!") );
			break;
		case DDERR_OUTOFVIDEOMEMORY:
			strcpy( strMsg, TEXT("There was insufficient video memory "
					"to use the\nhardware device.") );
			break;
		default:
			strcpy( strMsg, TEXT("Generic application error.\n\nEnable "
			        "debug output for detailed information.") );
	}

	if( MSGERR_APPMUSTEXIT == errType )
	{
		strcat( strMsg, TEXT("\n\nThis sample will now exit.") );
		MessageBox( NULL, strMsg, g_strAppTitle, MB_ICONERROR|MB_OK );
	}
	else
	{
		if( MSGWARN_SWITCHTOSOFTWARE == errType )
			strcat( strMsg, TEXT("\n\nSwitching to software rasterizer.") );
		MessageBox( NULL, strMsg, g_strAppTitle, MB_ICONWARNING|MB_OK );
	}
}
