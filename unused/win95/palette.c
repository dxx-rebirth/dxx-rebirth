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

#define WIN95
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <conio.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>

#include "ddraw.h"

#include "types.h"
#include "mem.h"
#include "gr.h"
#include "cfile.h"
#include "error.h"
#include "mono.h"
#include "fix.h"
#include "key.h"
#include "winapp.h"
#include "dd.h"

typedef struct LOGPAL256 {
	WORD ver;
	WORD num;
	PALETTEENTRY ScratchPal[256];
} LOGPAL256;


//	Special --------------------------------------------------------------------
extern int gr_installed;


//	Globals --------------------------------------------------------------------	

ubyte gr_palette[256*3];					// Main Palette in RGB
ubyte gr_current_pal[256*3];				// Current Valid Palette in RGB
ubyte gr_fade_table[256*34];			 	// Fade Palette Table in RGB

ubyte	gr_palette_gamma = 0;
int 	gr_palette_gamma_param = 0;
ubyte	gr_palette_faded_out = 1;

int grd_fades_disabled=0;					// Used to skip fading for development

static LPDIRECTDRAWPALETTE	_lpDDPalActive = 0; 	
static LPDIRECTDRAW	lpDD = NULL;

static BOOL PalGDI = FALSE;
static HPALETTE hPalGDI = 0;
static LOGPAL256 PalGDIData;


void ClearSystemPalette();


//	Functions ------------------------------------------------------------------

void grwin_set_winpalette(LPDIRECTDRAW lpdd, LPDIRECTDRAWPALETTE lpDDPal)
{
	_lpDDPalActive = lpDDPal;
	lpDD = lpdd;
	
}


LPDIRECTDRAWPALETTE grwin_get_winpalette(void)
{
	return _lpDDPalActive;

}


void grwin_cleanup_palette()
{
	if (hPalGDI) DeleteObject(hPalGDI);
}


void grwin_set_palette_exclusive(int yes)
{
	if (yes) {
		PalGDI = FALSE;
		if (hPalGDI) DeleteObject(hPalGDI);
		hPalGDI = 0;
	}
	else {
		HDC hdc;

		ClearSystemPalette();

		PalGDI = TRUE;
		PalGDIData.ver = 0x300;
		PalGDIData.num = 256;
		hPalGDI = CreatePalette((PLOGPALETTE)&PalGDIData);

		hdc = GetDC(GetLibraryWindow());
		SelectPalette(hdc, hPalGDI, FALSE);
		ReleaseDC(GetLibraryWindow(), hdc);
	}
}


void grwin_gdi_realizepal(HDC hdc) 
{
	if (PalGDI) {
		SelectPalette(hdc, hPalGDI, FALSE);
		RealizePalette(hdc);
	}
}


//	----------------------------------------------------------------------------

void gr_palette_set_gamma( int gamma )
{
	if ( gamma < 0 ) gamma = 0;
	if ( gamma > 8 ) gamma = 8;

	if (gr_palette_gamma_param != gamma )	{
		gr_palette_gamma_param = gamma;
		gr_palette_gamma = gamma;
		if (!gr_palette_faded_out)	{
			gr_palette_load( gr_palette );
		}
	}	
}

int gr_palette_get_gamma()
{
	return gr_palette_gamma_param;
}


void gr_use_palette_table( char * filename )
{
	CFILE *fp;
	int i,fsize;

	fp = cfopen( filename, "rb" );
	if ( fp==NULL)
		Error("Can't open palette file <%s>",filename);

	fsize	= cfilelength( fp );
	Assert( fsize == 9472 );
	cfread( gr_palette, 256*3, 1, fp );
	cfread( gr_fade_table, 256*34, 1, fp );
	cfclose(fp);

	// This is the TRANSPARENCY COLOR
	for (i=0; i<GR_FADE_LEVELS; i++ )	{
		gr_fade_table[i*256+255] = 255;
	}

}

#define SQUARE(x) ((x)*(x))

#define	MAX_COMPUTED_COLORS	32

int	Num_computed_colors=0;

typedef struct {
	ubyte	r,g,b,color_num;
} color_record;

color_record Computed_colors[MAX_COMPUTED_COLORS];

//	Add a computed color (by gr_find_closest_color) to list of computed colors in Computed_colors.
//	If list wasn't full already, increment Num_computed_colors.
//	If was full, replace a random one.
void add_computed_color(int r, int g, int b, int color_num)
{
	int	add_index;

	if (Num_computed_colors < MAX_COMPUTED_COLORS) {
		add_index = Num_computed_colors;
		Num_computed_colors++;
	} else
		add_index = (rand() * MAX_COMPUTED_COLORS) >> 15;

	Computed_colors[add_index].r = r;
	Computed_colors[add_index].g = g;
	Computed_colors[add_index].b = b;
	Computed_colors[add_index].color_num = color_num;
}

void init_computed_colors(void)
{
	int	i;

	for (i=0; i<MAX_COMPUTED_COLORS; i++)
		Computed_colors[i].r = 255;		//	Make impossible to match.
}

int gr_find_closest_color( int r, int g, int b )
{
	int i, j;
	int best_value, best_index, value;

	if (Num_computed_colors == 0)
		init_computed_colors();

	//	If we've already computed this color, return it!
	for (i=0; i<Num_computed_colors; i++)
		if (r == Computed_colors[i].r)
			if (g == Computed_colors[i].g)
				if (b == Computed_colors[i].b) {
					if (i > 4) {
						color_record	trec;
						trec = Computed_colors[i-1];
						Computed_colors[i-1] = Computed_colors[i];
						Computed_colors[i] = trec;
						return Computed_colors[i-1].color_num;
					}
					return Computed_colors[i].color_num;
				}

//	r &= 63;
//	g &= 63;
//	b &= 63;

	best_value = SQUARE(r-gr_palette[0])+SQUARE(g-gr_palette[1])+SQUARE(b-gr_palette[2]);
	best_index = 0;
	if (best_value==0) {
		add_computed_color(r, g, b, best_index);
 		return best_index;
	}
	j=0;
	// only go to 255, 'cause we dont want to check the transparent color.
	for (i=1; i<254; i++ )	{
		j += 3;
		value = SQUARE(r-gr_palette[j])+SQUARE(g-gr_palette[j+1])+SQUARE(b-gr_palette[j+2]);
		if ( value < best_value )	{
			if (value==0) {
				add_computed_color(r, g, b, i);
				return i;
			}
			best_value = value;
			best_index = i;
		}
	}
	add_computed_color(r, g, b, best_index);
	return best_index;
}

int gr_find_closest_color_15bpp( int rgb )
{
	return gr_find_closest_color( ((rgb>>10)&31)*2, ((rgb>>5)&31)*2, (rgb&31)*2 );
}


int gr_find_closest_color_current( int r, int g, int b )
{
	int i, j;
	int best_value, best_index, value;

//	r &= 63;
//	g &= 63;
//	b &= 63;

	best_value = SQUARE(r-gr_current_pal[0])+SQUARE(g-gr_current_pal[1])+SQUARE(b-gr_current_pal[2]);
	best_index = 0;
	if (best_value==0)
 		return best_index;

	j=0;
	// only go to 255, 'cause we dont want to check the transparent color.
	for (i=1; i<254; i++ )	{
		j += 3;
		value = SQUARE(r-gr_current_pal[j])+SQUARE(g-gr_current_pal[j+1])+SQUARE(b-gr_current_pal[j+2]);
		if ( value < best_value )	{
			if (value==0)
				return i;
			best_value = value;
			best_index = i;
		}
	}
	return best_index;
}


static int last_r=0, last_g=0, last_b=0;

void gr_palette_step_up( int r, int g, int b )
{
	HRESULT ddresult;
	int i;
	ubyte *p;
	int temp;

	Assert(_lpDDPalActive!=0);

	if (gr_palette_faded_out) return;

	if ( (r==last_r) && (g==last_g) && (b==last_b) ) return;

	last_r = r;
	last_g = g;
	last_b = b;

	p=gr_palette;
	for (i=0; i<256; i++ )	{
		temp = (int)(*p++) + r + gr_palette_gamma;
		if (temp<0) temp=0;
		else if (temp>63) temp=63;
		PalGDIData.ScratchPal[i].peRed = temp << 2;
		temp = (int)(*p++) + g + gr_palette_gamma;
		if (temp<0) temp=0;
		else if (temp>63) temp=63;
		PalGDIData.ScratchPal[i].peGreen = temp << 2;
		temp = (int)(*p++) + b + gr_palette_gamma;
		if (temp<0) temp=0;
		else if (temp>63) temp=63;
		PalGDIData.ScratchPal[i].peBlue = temp << 2;
		PalGDIData.ScratchPal[i].peFlags = PC_NOCOLLAPSE;
	}

	if (!PalGDI) {
		ddresult = IDirectDrawPalette_SetEntries(_lpDDPalActive, 0, 0, 256, PalGDIData.ScratchPal);	
		Assert(ddresult == DD_OK);
	}
	else {
		HDC hdc;

		hdc = GetDC(GetLibraryWindow());		
		SetPaletteEntries(hPalGDI, 0, PalGDIData.num, PalGDIData.ScratchPal);
		RealizePalette(hdc);
		ReleaseDC(GetLibraryWindow(), hdc);
	}

}

void gr_palette_clear()
{
	int i;
	HRESULT ddresult;

	Assert(_lpDDPalActive!=0);

//	Zero out Palette
	for (i = 0; i < 256; i++)
	{
		PalGDIData.ScratchPal[i].peRed = 
		PalGDIData.ScratchPal[i].peBlue = 
		PalGDIData.ScratchPal[i].peGreen = 0;
		PalGDIData.ScratchPal[i].peFlags = 0;
	}

	if (!hPalGDI) {
		ddresult = IDirectDrawPalette_SetEntries(_lpDDPalActive, 0,
									0, 256,
									PalGDIData.ScratchPal);			
		Assert(ddresult == DD_OK);
	}
	else {
		HDC hdc;

		hdc = GetDC(GetLibraryWindow());		
		SetPaletteEntries(hPalGDI, 0, PalGDIData.num, PalGDIData.ScratchPal);
		RealizePalette(hdc);
		ReleaseDC(GetLibraryWindow(), hdc);
	}

	gr_palette_faded_out = 1;
	if (GRMODEINFO(emul)) DDClearDisplay();

}

void gr_palette_load( ubyte * pal )	
{
	int i;
	ubyte c;
	HRESULT ddresult;

	Assert(_lpDDPalActive!=0);

	for (i=0; i<256; i++ )	{
		c = pal[i*3] + gr_palette_gamma;
		if ( c > 63 ) c = 63;
		PalGDIData.ScratchPal[i].peRed = c << 2;
 		gr_current_pal[i*3] = pal[i*3];
		c = pal[i*3+1] + gr_palette_gamma;
		if ( c > 63 ) c = 63;
		PalGDIData.ScratchPal[i].peGreen = c << 2;
 		gr_current_pal[i*3+1] = pal[i*3+1];
		c = pal[i*3+2] + gr_palette_gamma;
		if ( c > 63 ) c = 63;
		PalGDIData.ScratchPal[i].peBlue = c << 2;
 		gr_current_pal[i*3+2] = pal[i*3+2];
		PalGDIData.ScratchPal[i].peFlags = 0;
	}
	
	if (!hPalGDI) {
		ddresult = IDirectDrawPalette_SetEntries(_lpDDPalActive, 0,
											0, 256,
											PalGDIData.ScratchPal);
		Assert(ddresult == DD_OK);
	}
	else {
		HDC hdc;

		hdc = GetDC(GetLibraryWindow());		
		SetPaletteEntries(hPalGDI, 0, PalGDIData.num, PalGDIData.ScratchPal);
		RealizePalette(hdc);
		ReleaseDC(GetLibraryWindow(), hdc);
	}

	gr_palette_faded_out = 0;

	init_computed_colors();
}

extern void gr_sync_display(void);

int gr_palette_fade_out(ubyte *pal, int nsteps, int allow_keys )	
{
	ubyte c;
	int i,j;
	HRESULT ddresult;
	fix fade_palette[768];
	fix fade_palette_delta[768];

	allow_keys  = allow_keys;

	Assert(_lpDDPalActive!=0);

	if (gr_palette_faded_out) return 0;

	#ifndef NDEBUG
	if (grd_fades_disabled) {
		gr_palette_clear();
		return 0;
	}
	#endif

	for (i=0; i<768; i++ )	{
		fade_palette[i] = i2f(pal[i]+gr_palette_gamma);
		fade_palette_delta[i] = fade_palette[i] / nsteps;
	}


	for (j=0; j<nsteps; j++ )	{
		for (i=0; i<256; i++ )	{
			fade_palette[i*3] -= fade_palette_delta[i*3];
			if (fade_palette[i*3] < 0) fade_palette[i*3] = 0;
			fade_palette[i*3+1] -= fade_palette_delta[i*3+1];
			if (fade_palette[i*3+1] < 0) fade_palette[i*3+1] = 0;
			fade_palette[i*3+2] -= fade_palette_delta[i*3+2];
			if (fade_palette[i*3+2] < 0) fade_palette[i*3+2] = 0;
			c = f2i(fade_palette[i*3]);
			if ( c > 63 ) c = 63;
			PalGDIData.ScratchPal[i].peRed = c << 2;
			c = f2i(fade_palette[i*3+1]);
			if ( c > 63 ) c = 63;
			PalGDIData.ScratchPal[i].peGreen = c << 2;
			c = f2i(fade_palette[i*3+2]);
			if ( c > 63 ) c = 63;
			PalGDIData.ScratchPal[i].peBlue = c << 2;
			PalGDIData.ScratchPal[i].peFlags = 0;
		}

		if (!hPalGDI) {
			IDirectDraw_WaitForVerticalBlank(lpDD, DDWAITVB_BLOCKBEGIN, NULL);
			ddresult = IDirectDrawPalette_SetEntries(_lpDDPalActive, 0,
											0, 256,
											PalGDIData.ScratchPal);
			Assert(ddresult == DD_OK);
		}
		else {
			HDC hdc;

			hdc = GetDC(GetLibraryWindow());		
			SetPaletteEntries(hPalGDI, 0, PalGDIData.num, PalGDIData.ScratchPal);
			RealizePalette(hdc);
			ReleaseDC(GetLibraryWindow(), hdc);
		}
	}
	gr_palette_faded_out = 1;

	if (GRMODEINFO(emul)) DDClearDisplay();

	return 0;
}

int gr_palette_fade_in(ubyte *pal, int nsteps, int allow_keys)	
{
	HRESULT ddresult;
	int i,j;
	ubyte c;
	fix fade_palette[768];
	fix fade_palette_delta[768];

	allow_keys  = allow_keys;

	Assert(_lpDDPalActive!=0);

	if (!gr_palette_faded_out) return 0;

	#ifndef NDEBUG
	if (grd_fades_disabled) {
		gr_palette_load(pal);
		return 0;
	}
	#endif

	for (i=0; i<768; i++ )	{
		gr_current_pal[i] = pal[i];
		fade_palette[i] = 0;
		fade_palette_delta[i] = i2f(pal[i]+gr_palette_gamma) / nsteps;
	}

	for (j=0; j<nsteps; j++ )	{
		for (i=0; i<256; i++ )	{
			fade_palette[i*3] += fade_palette_delta[i*3];
			fade_palette[i*3+1] += fade_palette_delta[i*3+1];
			fade_palette[i*3+2] += fade_palette_delta[i*3+2];
			if (fade_palette[i*3] > i2f(pal[i*3]+gr_palette_gamma) )
				fade_palette[i*3] = i2f(pal[i*3]+gr_palette_gamma);
			if (fade_palette[i*3+1] > i2f(pal[i*3+1]+gr_palette_gamma) )
				fade_palette[i*3+1] = i2f(pal[i*3+1]+gr_palette_gamma);
			if (fade_palette[i*3+2] > i2f(pal[i*3+2]+gr_palette_gamma) )
				fade_palette[i*3+2] = i2f(pal[i*3+2]+gr_palette_gamma);

			c = f2i(fade_palette[i*3]);
			if ( c > 63 ) c = 63;
			PalGDIData.ScratchPal[i].peRed = c << 2;
			c = f2i(fade_palette[i*3+1]);
			if ( c > 63 ) c = 63;
			PalGDIData.ScratchPal[i].peGreen = c << 2;
			c = f2i(fade_palette[i*3+2]);
			if ( c > 63 ) c = 63;
			PalGDIData.ScratchPal[i].peBlue = c << 2;
			PalGDIData.ScratchPal[i].peFlags = 0;
		}
			
		if (!hPalGDI) {
			IDirectDraw_WaitForVerticalBlank(lpDD, DDWAITVB_BLOCKBEGIN, NULL);
			ddresult = IDirectDrawPalette_SetEntries(_lpDDPalActive, 0,
											0, 256,
											PalGDIData.ScratchPal);
			Assert (ddresult == DD_OK);
		}
		else {
			HDC hdc;
		
			hdc = GetDC(GetLibraryWindow());		
			SetPaletteEntries(hPalGDI, 0, PalGDIData.num, PalGDIData.ScratchPal);
			RealizePalette(hdc);
			ReleaseDC(GetLibraryWindow(), hdc);
		}
	}

	gr_palette_faded_out = 0;
	return 0;
}

void gr_make_cthru_table(ubyte * table, ubyte r, ubyte g, ubyte b )
{
	int i;
	ubyte r1, g1, b1;

	for (i=0; i<256; i++ )	{
		r1 = gr_palette[i*3+0] + r;
		if ( r1 > 63 ) r1 = 63;
		g1 = gr_palette[i*3+1] + g;
		if ( g1 > 63 ) g1 = 63;
		b1 = gr_palette[i*3+2] + b;
		if ( b1 > 63 ) b1 = 63;
		table[i] = gr_find_closest_color( r1, g1, b1 );
	}
}

void gr_palette_read(ubyte * palette)
{
	int i;
	HRESULT ddresult;
	
	Assert(_lpDDPalActive!=0);

	if (!hPalGDI) {
		ddresult = IDirectDrawPalette_GetEntries(_lpDDPalActive, 0, 0, 256, PalGDIData.ScratchPal);
		Assert(ddresult == DD_OK);
	}
	else {
		SetPaletteEntries(hPalGDI, 0, 256, PalGDIData.ScratchPal);
	}

	for (i=0; i<256; i++ )	{
		*palette++ = PalGDIData.ScratchPal[i].peRed >> 2;
		*palette++ = PalGDIData.ScratchPal[i].peGreen >> 2;
		*palette++ = PalGDIData.ScratchPal[i].peBlue >> 2;
	}
}



void gr_copy_palette(ubyte *gr_palette, ubyte *pal, int size)
{
	memcpy(gr_palette, pal, size);

	Num_computed_colors = 0;
}


//	GDI Palette Functions taken from ancient WinG version

void ClearSystemPalette()
{
	LOGPAL256 palette = {
		0x300,
		256,
	};
	HPALETTE screenpal = 0;
	HDC		screenDC;
	int 		counter;

//	Reset system palette to black to quicken WinG.
	for (counter = 0; counter < 256; counter++)
	{
		palette.ScratchPal[counter].peRed = 0;
		palette.ScratchPal[counter].peGreen = 0;
		palette.ScratchPal[counter].peBlue = 0;
		palette.ScratchPal[counter].peFlags = PC_NOCOLLAPSE;
	}

//	Create, select, realize, and deselect/delete palette.
	screenDC = GetDC(NULL);
	screenpal = CreatePalette((LOGPALETTE*)&palette);
	screenpal = SelectPalette(screenDC, screenpal, FALSE);
	RealizePalette(screenDC);
	screenpal = SelectPalette(screenDC, screenpal, FALSE);
	DeleteObject(screenpal);
	ReleaseDC(NULL, screenDC);
}

















