/*==========================================================================
 *
 *  Copyright (C) 1998 Microsoft Corporation. All Rights Reserved.
 *
 *  File:       ddutil.cpp
 *  Content:    Routines for loading bitmap and palettes from resources
 *
 ***************************************************************************/

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */

extern IDirectDrawPalette  *DDLoadPalette(IDirectDraw4 *pdd, LPCSTR szBitmap);
extern IDirectDrawSurface4 *DDLoadBitmap(IDirectDraw4 *pdd, LPCSTR szBitmap, int dx, int dy);
extern HRESULT              DDReLoadBitmap(IDirectDrawSurface4 *pdds, LPCSTR szBitmap);
extern HRESULT              DDCopyBitmap(IDirectDrawSurface4 *pdds, HBITMAP hbm, int x, int y, int dx, int dy);
extern DWORD                DDColorMatch(IDirectDrawSurface4 *pdds, COLORREF rgb);
extern HRESULT              DDSetColorKey(IDirectDrawSurface4 *pdds, COLORREF rgb);

#ifdef __cplusplus
}
#endif	/* __cplusplus */

