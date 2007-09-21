#ifndef D3DHELP_H_
#define D3DHELP_H_

void Win32_Rect(int left, int top, int right, int bot, int iSurf, int iCol);
#ifndef __cplusplus
void Win32_BlitLinearToDirectX_bm(grs_bitmap *bm, int sx, int sy, 
	int w, int h, int dx, int dy, int mask);
#endif
void Win32_InvalidatePages(void);

#endif
