/*  DT_drawtext.c
 *  Written By: Garrett Banuk <mongoose@mongeese.org>
 *  This is free, just be sure to give me credit when using it
 *  in any of your programs.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "SDL.h"
#include "SDL_image.h"
#include "DT_drawtext.h"
#include "internal.h"


static BitFont *BitFonts = NULL;	/* Linked list of fonts */


/* sets the transparency value for the font in question.  assumes that
 * we're in an OpenGL context.  */
void DT_SetFontAlphaGL(int FontNumber, int a) {
	unsigned char val;
	BitFont *CurrentFont;
	unsigned char r_targ, g_targ, b_targ;
	int i, imax;
	unsigned char *pix;

	/* get pointer to font */
	CurrentFont = DT_FontPointer(FontNumber);
	if(CurrentFont == NULL) {
		PRINT_ERROR("Setting font alpha for non-existent font!\n");
		return;
	}
	if(CurrentFont->FontSurface->format->BytesPerPixel == 2) {
		PRINT_ERROR("16-bit SDL surfaces do not support alpha-blending under OpenGL\n");
		return;
	}
	if(a < SDL_ALPHA_TRANSPARENT)
		val = SDL_ALPHA_TRANSPARENT;
	else if(a > SDL_ALPHA_OPAQUE)
		val = SDL_ALPHA_OPAQUE;
	else
		val = a;

	/* iterate over all pixels in the font surface.  For each
	 * pixel that is (255,0,255), set its alpha channel
	 * appropriately.  */
	imax = CurrentFont->FontSurface->h * (CurrentFont->FontSurface->w << 2);
	pix = (unsigned char *)(CurrentFont->FontSurface->pixels);
	r_targ = 255;		/*pix[0]; */
	g_targ = 0;		/*pix[1]; */
	b_targ = 255;		/*pix[2]; */
	for(i = 3; i < imax; i += 4)
		if(pix[i - 3] == r_targ && pix[i - 2] == g_targ && pix[i - 1] == b_targ)
			pix[i] = val;
	/* also make sure that alpha blending is disabled for the font
	   surface. */
	SDL_SetAlpha(CurrentFont->FontSurface, 0, SDL_ALPHA_OPAQUE);
}

/* Loads the font into a new struct
 * returns -1 as an error else it returns the number
 * of the font for the user to use
 */
int DT_LoadFont(const char *BitmapName, int flags) {
	int FontNumber = 0;
	BitFont **CurrentFont = &BitFonts;
	SDL_Surface *Temp;


	while(*CurrentFont) {
		CurrentFont = &((*CurrentFont)->NextFont);
		FontNumber++;
	}

	/* load the font bitmap */
	if(NULL == (Temp = IMG_Load(BitmapName))) {
		PRINT_ERROR("Cannot load file ");
		printf("%s\n", BitmapName);
		return -1;
	}

	/* Add a font to the list */
	*CurrentFont = (BitFont *) malloc(sizeof(BitFont));

	(*CurrentFont)->FontSurface = SDL_DisplayFormat(Temp);
	SDL_FreeSurface(Temp);

	(*CurrentFont)->CharWidth = (*CurrentFont)->FontSurface->w / 256;
	(*CurrentFont)->CharHeight = (*CurrentFont)->FontSurface->h;
	(*CurrentFont)->FontNumber = FontNumber;
	(*CurrentFont)->NextFont = NULL;


	/* Set font as transparent if the flag is set.  The assumption we'll go on
	 * is that the first pixel of the font image will be the color we should treat
	 * as transparent.
	 */
	if(flags & TRANS_FONT) {
		if(SDL_GetVideoSurface()->flags & SDL_OPENGLBLIT)
			DT_SetFontAlphaGL(FontNumber, SDL_ALPHA_TRANSPARENT);
		else
			SDL_SetColorKey((*CurrentFont)->FontSurface, SDL_SRCCOLORKEY | SDL_RLEACCEL, SDL_MapRGB((*CurrentFont)->FontSurface->format, 255, 0, 255));
	} else if(SDL_GetVideoSurface()->flags & SDL_OPENGLBLIT)
		DT_SetFontAlphaGL(FontNumber, SDL_ALPHA_OPAQUE);

	return FontNumber;
}

/* Takes the font type, coords, and text to draw to the surface*/
void DT_DrawText(const char *string, SDL_Surface *surface, int FontType, int x, int y) {
	int loop;
	int characters;
	int current;
	SDL_Rect SourceRect, DestRect;
	BitFont *CurrentFont;

	CurrentFont = DT_FontPointer(FontType);

	/* see how many characters can fit on the screen */
	if(x > surface->w || y > surface->h)
		return;

	if(strlen(string) < (surface->w - x) / CurrentFont->CharWidth)
		characters = strlen(string);
	else
		characters = (surface->w - x) / CurrentFont->CharWidth;

	DestRect.x = x;
	DestRect.y = y;
	DestRect.w = CurrentFont->CharWidth;
	DestRect.h = CurrentFont->CharHeight;

	SourceRect.y = 0;
	SourceRect.w = CurrentFont->CharWidth;
	SourceRect.h = CurrentFont->CharHeight;

	/* Now draw it */
	for(loop = 0; loop < characters; loop++) {
		current = string[loop];
		if (current<0 || current > 255)
			current = 0;
		//SourceRect.x = string[loop] * CurrentFont->CharWidth;
		SourceRect.x = current * CurrentFont->CharWidth;
		SDL_BlitSurface(CurrentFont->FontSurface, &SourceRect, surface, &DestRect);
		DestRect.x += CurrentFont->CharWidth;
	}
	/* if we're in OpenGL-mode, we need to manually update after blitting. */
	if(surface->flags & SDL_OPENGLBLIT) {
		DestRect.x = x;
		DestRect.w = characters * CurrentFont->CharWidth;
		SDL_UpdateRects(surface, 1, &DestRect);
	}
}


/* Returns the height of the font numbers character
 * returns 0 if the fontnumber was invalid */
int DT_FontHeight(int FontNumber) {
	BitFont *CurrentFont;

	CurrentFont = DT_FontPointer(FontNumber);
	if(CurrentFont)
		return CurrentFont->CharHeight;
	else
		return 0;
}

/* Returns the width of the font numbers charcter */
int DT_FontWidth(int FontNumber) {
	BitFont *CurrentFont;

	CurrentFont = DT_FontPointer(FontNumber);
	if(CurrentFont)
		return CurrentFont->CharWidth;
	else
		return 0;
}

/* Returns a pointer to the font struct of the number
 * returns NULL if theres an error
 */
BitFont *DT_FontPointer(int FontNumber) {
	BitFont *CurrentFont = BitFonts;
	BitFont *temp;

	while(CurrentFont)
		if(CurrentFont->FontNumber == FontNumber)
			return CurrentFont;
		else {
			temp = CurrentFont;
			CurrentFont = CurrentFont->NextFont;
		}

	return NULL;

}

/* removes all the fonts currently loaded */
void DT_DestroyDrawText() {
	BitFont *CurrentFont = BitFonts;
	BitFont *temp;

	while(CurrentFont) {
		temp = CurrentFont;
		CurrentFont = CurrentFont->NextFont;

		SDL_FreeSurface(temp->FontSurface);
		free(temp);
	}

	BitFonts = NULL;
}


