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



#ifndef _WINAPP_H
#define _WINAPP_H

#include <stdarg.h>

extern void SetLibraryWinInfo(HWND hWnd, HINSTANCE hInstance);
extern HWND GetLibraryWindow(void);

extern void copen();
extern void cprintf(char *text, ...);
extern int cgetch();
extern void cclose();

extern void loginit(char *name);
extern void logclose();
extern void logentry(char *format, ...);


//	Clipboard crap
extern HBITMAP win95_screen_shot();

extern void gr_winckpit_blt_span(int xmin, int xmax, char *src, char *dest);
#pragma aux gr_winckpit_blt_span "*" parm [ebx] [ecx] [esi] [edi] modify [eax ebx ecx esi edi];

extern void gr_winckpit_blt_span_long (int xmin, int xmax, char *src, char *dest);
#pragma aux gr_winckpit_blt_span_long "*" parm [ebx] [ecx] [esi] [edi] modify [eax edx ebx ecx esi edi];

#endif
