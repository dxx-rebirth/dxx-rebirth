#ifndef __WINMAIN_H__
#define __WINMAIN_H__

#ifdef __cplusplus
extern "C" {
#endif

extern int InitMain (void);
extern void PumpMessages (void);

extern HWND g_hWnd;

#ifdef __cplusplus
}
#endif

#endif	// __WINMAIN_H__
