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
static char rcsid[] = "$Id: direct3d.c,v 1.1.1.1 2001-01-19 03:30:15 bradleyb Exp $";
#pragma on (unreferenced)




#define _WIN32
#define WIN95
#define WIN32_LEAN_AND_MEAN
#define INITGUID

#include <windows.h>
#include <windowsx.h>

#include <stdio.h>

#include "ddraw.h"
#include "d3d.h"

#include "pstypes.h"
#include "mono.h"
#include "args.h"
#include "error.h"
#include "winapp.h"

#include "dd.h"
#include "direct3d.h"



//	Globals
//	----------------------------------------------------------------------------

static LPDIRECT3D 			_lpD3D = NULL;		// D3D Object
static LPDIRECT3DDEVICE		_lpD3DDev = NULL;	// D3D Device
static GUID						_3DGUID;				// Device GUID

static d3d_caps				d3dCaps;				// Direct 3d Device Caps
static BOOL						d3d_initialized=0;// D3D flag
static FILE 					*LogFile=NULL;		// Log File!



//	Function prototypes
//	----------------------------------------------------------------------------

#ifdef NDEBUG
#define WRITELOG(t)
#define LOGINIT(n)
#define LOGCLOSE
#else
#define WRITELOG(t) if (LogFile) { fprintf t; fflush(LogFile); }
#define LOGINIT(n) LogFile = fopen(n, "wt"); 
#define LOGCLOSE if (LogFile) fclose(LogFile);
#endif

HRESULT FAR PASCAL d3d_enum_callback(LPGUID pguid, LPSTR lpDeviceDesc, 
								    LPSTR lpDeviceName, LPD3DDEVICEDESC lpHWDevDesc, 
								    LPD3DDEVICEDESC lpHELDevDesc, LPVOID lpUserArg);

int d3d_close();
int d3d_device_close();
int d3d_enum_devices(GUID *pguid);
int d3d_enum_texformats();
int d3d_init_device(GUID guid);
int d3d_handle_error(HRESULT err);




/* d3d_init
		Direct 3D Initialization of Immediate Mode

		We need to do a few things, first get the D3D interface from our
		DirectDraw Object
		
		Then we need to find an active device.

		Then we need to query the caps of such a device, and tell our GFX system
		about the available video modes (15-bit color)
*/


BOOL d3d_init(void)
{
	HRESULT 	res;
	GUID		guid;

	Assert(d3d_initialized == FALSE);

	LOGINIT("d3d.log");

	atexit(d3d_close);

//	initialize Direct3d interface
	res = IDirectDraw_QueryInterface(_lpDD, &IID_IDirect3D, &_lpD3D);
	if (res != DD_OK) 
		return d3d_handle_error(res);

//	enumerate all 3d devices useable by Direct3d
	if (d3d_enum_devices(&guid)) return FALSE; 

	memcpy(&_3DGUID, &guid, sizeof(GUID));

	d3d_initialized = TRUE;

	return TRUE;
}


int d3d_close(void)
{
	if (d3d_initialized) {
		IDirect3D_Release(_lpD3D);
		_lpD3D = NULL;
		d3d_initialized = FALSE;
	}
	return 0;
}		


int d3d_enum_devices(GUID *pguid)
{
	HRESULT res; 
	GUID guid;

	res = IDirect3D_EnumDevices(_lpD3D, d3d_enum_callback, (LPVOID)&guid);
	if (res != DD_OK) {
		WRITELOG((LogFile, "Unable to find a proper 3D hardware device.\n"));
		return d3d_handle_error(res);
	}

	WRITELOG((LogFile, "Found 3D Device (%s)\n", d3dCaps.devname));

	memcpy(pguid, &guid, sizeof(GUID));
	return 0;
}



/* d3d_device functions

		these functions are used once we really need to use the 3d system.
		When done with it, we call device_close.
*/

int d3d_init_device(GUID guid)
{
	HRESULT res;
	DDSURFACEDESC ddsd;

	if (!d3d_initialized) return -1;

//	Grab back buffer.
	res = IDirectDrawSurface_QueryInterface(_lpDDSBack, &_3DGUID, &_lpD3DDev);
	if (res != DD_OK) {
		WRITELOG((LogFile, "Unable to retrieve device from back buffer. %x\n",res));
		return d3d_handle_error(res);
	}
			
//	Enumerate texture formats
	Assert(d3dCaps.tmap_acc == TRUE);
	d3dCaps.tmap_formats = 0;
	if (d3d_enum_texformats()) return -1;

	return 0;
}


int d3d_device_close()
{
	if (_lpD3DDev) {
		IDirect3DDevice_Release(_lpD3DDev);
		_lpD3DDev = NULL;
	}
	return 0;
}


int d3d_enum_texformats()
{

	return 0;
}



/* Miscellaneous Utilities */
	
int d3d_handle_error(HRESULT err)
{
	if (err != DD_OK) return err;
	else return 0;
}



/* Direct 3D callbacks 

		enum_callback:   enumerates all 3D devices attached to system.
	

*/

HRESULT FAR PASCAL d3d_enum_callback(LPGUID pguid, LPSTR lpDeviceDesc, 
								   LPSTR lpDeviceName, 
									LPD3DDEVICEDESC lpHWDevDesc, 
								   LPD3DDEVICEDESC lpHELDevDesc, 
									LPVOID lpUserArg)
{

/* Ignore emulated devices (we can use our own stuff then) */
	if (lpHWDevDesc->dcmColorModel) {
		d3dCaps.hw_acc = TRUE;
	}
	else {
		d3dCaps.hw_acc = FALSE;
		return D3DENUMRET_OK;
	}

/* Test hardware caps 
		texture = MUST HAVE
*/
	if (lpHWDevDesc->dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE) {
		d3dCaps.tmap_acc = TRUE;
	}
	else {
		d3dCaps.tmap_acc = FALSE;
		return D3DENUMRET_OK;
	}

	memcpy(lpUserArg, pguid, sizeof(GUID));
	lstrcpy(d3dCaps.devname, lpDeviceName);

	return D3DENUMRET_CANCEL;
}
	
