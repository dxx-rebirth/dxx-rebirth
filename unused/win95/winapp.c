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
static char rcsid[] = "$Id: winapp.c,v 1.1.1.1 2001-01-19 03:30:15 bradleyb Exp $";
#pragma on (unreferenced)


#define _WIN32
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "mono.h"
#include "winapp.h"
#include "dd.h"

static HWND hWAppWnd = 0;
static HINSTANCE hWAppInstance = 0;


void SetLibraryWinInfo(HWND hWnd, HINSTANCE hInstance)
{
	hWAppWnd = hWnd;
	hWAppInstance = hInstance;
}


HWND GetLibraryWindow(void)
{
	return hWAppWnd;
}


HINSTANCE GetLibraryWinInstance(void)
{
	return hWAppInstance;	
}


//	Console Functions

static BOOL ConInit=FALSE;
static HANDLE ConStdOut;
static HANDLE ConStdIn;

void cinit()
{
	AllocConsole();
	ConStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	ConStdIn = GetStdHandle(STD_INPUT_HANDLE);
	ConInit = TRUE;
}

void cclose()
{
	ConInit = FALSE;
}


void cprintf(char *text, ...)
{
	char buffer[1000];
	va_list args;
	DWORD size;

	if (!ConInit) return;	

	va_start(args, text );
	vsprintf(buffer,text,args);

	WriteConsole(ConStdOut, buffer, strlen(buffer), &size, NULL);
}


int cgetch()
{
	char buffer[3];
	DWORD chread;

	ReadConsole(ConStdIn, buffer, sizeof(TCHAR), &chread, NULL);
	return (int)buffer[0];
}
	

//	Log Functions

static FILE *LogFile=NULL;

void loginit(char *name)
{
	if (LogFile) return;

	LogFile = fopen(name, "wt");
	if (!LogFile) {
		MessageBox(NULL, "Unable to create descentw.log!", "Descent II Beta Error", MB_OK);
		return;
	}

	fprintf(LogFile, "%s begins.\n--------------------------------------\n\n",name);
	fflush(LogFile);
}


void logclose()
{
	if (!LogFile) return;

	fprintf(LogFile, "\n-----------------------------------------\nlog ends.\n");

	fclose(LogFile);
	
	LogFile = NULL;
}	


void logentry(char *format, ...)
{
	char buffer[256];
	va_list args;

	if (!LogFile) return;	

	va_start(args, format );
	vsprintf(buffer, format ,args);

	mprintf((0, "%s", buffer));
	fprintf(LogFile, "%s", buffer);
	fflush(LogFile);
}


//	Disk stuff

unsigned int GetFreeDiskSpace()
{
	DWORD sec_per_cluster,
			bytes_per_sec,
			free_clusters,
			total_clusters;

	if (!GetDiskFreeSpace(
		NULL,
		&sec_per_cluster,
		&bytes_per_sec,
		&free_clusters,
		&total_clusters)) return 0x7fffffff;

	return (uint)(free_clusters * sec_per_cluster * bytes_per_sec);
}


//	Clipboarding functions

HBITMAP win95_screen_shot()
{
	HDC hdcscreen;
	HDC hdccapture;
	HBITMAP screen_bitmap;
	HBITMAP old_bitmap;

//@@	hdcscreen = CreateDC("DISPLAY", NULL, NULL, NULL);
	IDirectDrawSurface_GetDC(dd_grd_screencanv->lpdds, &hdcscreen);
	hdccapture = CreateCompatibleDC(hdcscreen);
	
	screen_bitmap = CreateCompatibleBitmap(hdcscreen, GRMODEINFO(w), GRMODEINFO(h));

	if (!screen_bitmap) {
		DeleteDC(hdccapture);
		IDirectDrawSurface_ReleaseDC(dd_grd_screencanv->lpdds, hdcscreen);
	//@@	DeleteDC(hdcscreen);
		return NULL;
	}

	old_bitmap = SelectObject(hdccapture, screen_bitmap);
	if (!old_bitmap) {
		DeleteObject(screen_bitmap);
		DeleteDC(hdccapture);
		IDirectDrawSurface_ReleaseDC(dd_grd_screencanv->lpdds, hdcscreen);
		//@@	DeleteDC(hdcscreen);
		return NULL;
	}
	
	if (!BitBlt(hdccapture, 0,0,GRMODEINFO(w), GRMODEINFO(h), hdcscreen, 0, 0,
			SRCCOPY)) {
		SelectObject(hdccapture, old_bitmap);
		DeleteObject(screen_bitmap);
		DeleteDC(hdccapture);
		IDirectDrawSurface_ReleaseDC(dd_grd_screencanv->lpdds, hdcscreen);
	//@@	DeleteDC(hdcscreen);
		return NULL;
	}

	screen_bitmap = SelectObject(hdccapture, old_bitmap);
	DeleteDC(hdccapture);

	IDirectDrawSurface_ReleaseDC(dd_grd_screencanv->lpdds, hdcscreen);
//@@	DeleteDC(hdcscreen);

	return screen_bitmap;
}		


HANDLE win95_dib_from_bitmap(HBITMAP hbm)
{
	HANDLE hMem,hX;
	BITMAP bm;
	BITMAPINFOHEADER bi;
	BITMAPINFOHEADER *pbi;
	RGBQUAD *rgbp;
	HDC hdc;
	char grpal[768];
	int i;
	HRESULT hRes;

	GetObject(hbm, sizeof(bm), &bm);

	memset(&bi, 0, sizeof(bi));
	
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bm.bmWidth;
	bi.biHeight = bm.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = bm.bmPlanes * bm.bmBitsPixel;
	bi.biCompression = BI_RGB;

	hMem = GlobalAlloc(GHND, bi.biSize + 256 * sizeof(RGBQUAD));
	if (!hMem) {
		mprintf((1, "WINAPP: Unable to allocate memory for DIB.\n"));
		return NULL;
	}

	pbi = (BITMAPINFOHEADER *)GlobalLock(hMem);
	*pbi = bi;

	hRes = IDirectDrawSurface_GetDC(dd_grd_screencanv->lpdds, &hdc);
	if (hRes != DD_OK) {
		GlobalFree(hMem);
		mprintf((1, "WINAPP: Unable to get GDI DC for DIB creation.\n"));
		return NULL;
	}
	GetDIBits(hdc, hbm, 0, bi.biHeight, NULL, (BITMAPINFO *)pbi, DIB_RGB_COLORS);
	bi = *pbi;
	GlobalUnlock(hMem); 
	
	if (!bi.biSizeImage) {
		bi.biSizeImage = bi.biWidth * bi.biHeight;
	}		
	
	hX = GlobalReAlloc(hMem, bi.biSize + 256*sizeof(RGBQUAD) + bi.biSizeImage,0);
	if (!hX) {
		GlobalFree(hMem);
		IDirectDrawSurface_ReleaseDC(dd_grd_screencanv->lpdds, hdc);
		mprintf((1, "WINAPP: Unable to reallocate mem for DIB.\n"));
		return NULL;
	}
	hMem = hX;

	pbi = GlobalLock(hMem);
	
	if (!GetDIBits(hdc, hbm, 0, 
			pbi->biHeight, 
			(LPSTR)pbi + pbi->biSize + 256*sizeof(RGBQUAD),
			(BITMAPINFO *)pbi, DIB_RGB_COLORS)) {
		GlobalUnlock(hMem);
		GlobalFree(hMem);
		IDirectDrawSurface_ReleaseDC(dd_grd_screencanv->lpdds, hdc);
		mprintf((1, "WINAPP: GetDIBits was unable to get needed info.\n"));
		return NULL;
	}
	bi = *pbi;

	IDirectDrawSurface_ReleaseDC(dd_grd_screencanv->lpdds, hdc);
	
	rgbp = (RGBQUAD *)(((LPSTR)pbi + pbi->biSize));

	gr_palette_read(grpal);
	for (i = 0; i < 256; i++) 
	{
		rgbp[i].rgbRed = grpal[i*3]<<2;
		rgbp[i].rgbGreen = grpal[i*3+1]<<2;
		rgbp[i].rgbBlue = grpal[i*3+2]<<2;
		rgbp[i].rgbReserved = 0;
	}			

	GlobalUnlock(hMem);

	return hMem;
}

		
			
