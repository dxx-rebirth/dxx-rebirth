/* $Id: gr.c,v 1.1.1.1 2006/03/17 19:53:38 zicodxx Exp $ */
/*
 *
 * SDL video functions.
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <SDL/SDL.h>
#ifdef SDL_IMAGE
#include <SDL_image.h>
#endif

#include "gr.h"
#include "grdef.h"
#include "palette.h"
#include "u_mem.h"
#include "error.h"
#include "menu.h"
#include "vers_id.h"
#include "args.h"
#include "gamefont.h"

#ifdef _WIN32_WCE // should really be checking for "Pocket PC" somehow
# define LANDSCAPE
#endif

int sdl_video_flags = SDL_SWSURFACE | SDL_HWPALETTE | SDL_DOUBLEBUF;
//end addition -MM

SDL_Surface *screen;
#ifdef LANDSCAPE
static SDL_Surface *real_screen, *screen2;
#endif

int gr_installed = 0;

#ifdef LANDSCAPE
/* Create a new rotated surface for drawing */
SDL_Surface *CreateRotatedSurface(SDL_Surface *s)
{
#if 0
    return(SDL_CreateRGBSurface(s->flags, s->h, s->w,
    s->format->BitsPerPixel,
    s->format->Rmask,
    s->format->Gmask,
    s->format->Bmask,
    s->format->Amask));
#else
    return(SDL_CreateRGBSurface(s->flags, s->h, s->w, 8, 0, 0, 0, 0));
#endif
}

/* Used to copy the rotated scratch surface to the screen */
void BlitRotatedSurface(SDL_Surface *from, SDL_Surface *to)
{

    int bpp = from->format->BytesPerPixel;
    int w=from->w, h=from->h, pitch=to->pitch;
    int i,j;
    Uint8 *pfrom, *pto, *to0;

    SDL_LockSurface(from);
    SDL_LockSurface(to);
    pfrom=(Uint8 *)from->pixels;
    to0=(Uint8 *) to->pixels+pitch*(w-1);
    for (i=0; i<h; i++)
    {
        to0+=bpp;
        pto=to0;
        for (j=0; j<w; j++)
        {
            if (bpp==1) *pto=*pfrom;
            else if (bpp==2) *(Uint16 *)pto=*(Uint16 *)pfrom;
            else if (bpp==4) *(Uint32 *)pto=*(Uint32 *)pfrom;
            else if (bpp==3)
                {
                    pto[0]=pfrom[0];
                    pto[1]=pfrom[1];
                    pto[2]=pfrom[2];
                }
            pfrom+=bpp;
            pto-=pitch;
        }
    }
    SDL_UnlockSurface(from);
    SDL_UnlockSurface(to);
}
#endif

void gr_flip()
{
#ifdef LANDSCAPE
	screen2 = SDL_DisplayFormat(screen);
	BlitRotatedSurface(screen2, real_screen);
	//SDL_SetColors(real_screen, screen->format->palette->colors, 0, 256);
	SDL_UpdateRect(real_screen, 0, 0, 0, 0);
	SDL_FreeSurface(screen2);
#else
	SDL_Flip(screen);
#endif
}

// Set the buffer to draw to. 0 is front, 1 is back
// With SDL, can't use it without resetting the video mode
void gr_set_draw_buffer(int buf)
{
	buf = buf;
}


int gr_check_mode(u_int32_t mode)
{
	int w, h;

	w = SM_W(mode);
	h = SM_H(mode);

	return !SDL_VideoModeOK(w, h, 8, sdl_video_flags);
}


int gr_set_mode(u_int32_t mode)
{
	unsigned int w, h;

#ifdef NOGRAPH
	return 0;
#endif

	if (mode<=0)
		return 0;

	w=SM_W(mode);
	h=SM_H(mode);
	
//added on 11/06/98 by Matt Mueller to set the title bar. (moved from below)
//sekmu: might wanna copy this litte blurb to one of the text files or something
//we want to set it here so that X window manager "Style" type commands work
//for example, in fvwm2 or fvwm95:
//Style "D1X*"  NoTitle, NoHandles, BorderWidth 0
//if you can't use -fullscreen like me (crashes X), this is a big help in
//getting the window centered correctly (if you use SmartPlacement)
	SDL_WM_SetCaption(DESCENT_VERSION, "Descent II");
//end addition -MM

#ifdef SDL_IMAGE
	{
#include "descent.xpm"
		SDL_WM_SetIcon(IMG_ReadXPMFromArray(pixmap), NULL);
	}
#endif

//edited 10/05/98 by Matt Mueller - make fullscreen mode optional
	  // changed by adb on 980913: added SDL_HWPALETTE (should be option?)
        // changed by someone on 980923 to add SDL_FULLSCREEN

#ifdef LANDSCAPE
	real_screen = SDL_SetVideoMode(h, w, 0, sdl_video_flags);
	screen = CreateRotatedSurface(real_screen);
#else
	  screen = SDL_SetVideoMode(w, h, 8, sdl_video_flags);
#endif
		// end changes by someone
        // end changes by adb
//end edit -MM
        if (screen == NULL) {
           Error("Could not set %dx%dx8 video mode\n",w,h);
           exit(1);
        }
	memset( grd_curscreen, 0, sizeof(grs_screen));
	grd_curscreen->sc_mode = mode;
	grd_curscreen->sc_w = w;
	grd_curscreen->sc_h = h;
	grd_curscreen->sc_aspect = fixdiv(grd_curscreen->sc_w*GameArg.GfxAspectX,grd_curscreen->sc_h*GameArg.GfxAspectY);
	grd_curscreen->sc_canvas.cv_bitmap.bm_x = 0;
	grd_curscreen->sc_canvas.cv_bitmap.bm_y = 0;
	grd_curscreen->sc_canvas.cv_bitmap.bm_w = w;
	grd_curscreen->sc_canvas.cv_bitmap.bm_h = h;
	grd_curscreen->sc_canvas.cv_bitmap.bm_rowsize = screen->pitch;
	grd_curscreen->sc_canvas.cv_bitmap.bm_type = BM_LINEAR;
	grd_curscreen->sc_canvas.cv_bitmap.bm_data = (unsigned char *)screen->pixels;
	gr_set_current_canvas(NULL);
	//gr_enable_default_palette_loading();

//added on 9/30/98 by Matt Mueller to hide the mouse if its over the game window
	SDL_ShowCursor(0);
//end addition -MM
//--moved up--added on 9/30/98 by Matt Mueller to set the title bar.  Woohoo!
//--moved up--	SDL_WM_SetCaption(DESCENT_VERSION " " D1X_DATE, NULL);
//--moved up--end addition -MM

	gamefont_choose_game_font(w,h);
	gr_palette_load(gr_palette);

	return 0;
}

int gr_check_fullscreen(void){
	return (sdl_video_flags & SDL_FULLSCREEN)?1:0;
}

int gr_toggle_fullscreen(void){
	sdl_video_flags^=SDL_FULLSCREEN;
	SDL_WM_ToggleFullScreen(screen);
	return (sdl_video_flags & SDL_FULLSCREEN)?1:0;
}

int gr_init(int mode)
{
	int retcode;
 	// Only do this function once!
	if (gr_installed==1)
		return -1;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		Error("SDL library video initialisation failed: %s.",SDL_GetError());
	}
	MALLOC( grd_curscreen,grs_screen,1 );
	memset( grd_curscreen, 0, sizeof(grs_screen));


	if (!GameArg.SysWindow)
	     sdl_video_flags|=SDL_FULLSCREEN;

	//added 05/19/99 Matt Mueller - make HW surface optional
	if (GameArg.DbgSdlHWSurface)
	     sdl_video_flags|=SDL_HWSURFACE;
	//end addition -MM

	// Set the mode.
	if ((retcode=gr_set_mode(mode)))
	{
		return retcode;
	}

	grd_curscreen->sc_canvas.cv_color = 0;
	grd_curscreen->sc_canvas.cv_drawmode = 0;
	grd_curscreen->sc_canvas.cv_font = NULL;
	grd_curscreen->sc_canvas.cv_font_fg_color = 0;
	grd_curscreen->sc_canvas.cv_font_bg_color = 0;
	gr_set_current_canvas( &grd_curscreen->sc_canvas );

	gr_installed = 1;
	// added on 980913 by adb to add cleanup
	atexit(gr_close);
	// end changes by adb

	return 0;
}

void gr_close()
{
	if (gr_installed==1)
	{
		gr_installed = 0;
		d_free(grd_curscreen);
	}
}

// Palette functions follow.

static int last_r=0, last_g=0, last_b=0;




void gr_palette_step_up( int r, int g, int b )
{
 int i;
 ubyte *p = gr_palette;
 int temp;

 SDL_Palette *palette;
 SDL_Color colors[256];

 if ( (r==last_r) && (g==last_g) && (b==last_b) ) return;

 last_r = r;
 last_g = g;
 last_b = b;

 palette = screen->format->palette;

 if (palette == NULL) {
    return; // Display is not palettised
 }

 for (i=0; i<256; i++) {
   temp = (int)(*p++) + r + gr_palette_gamma;
   if (temp<0) temp=0;
   else if (temp>63) temp=63;
   colors[i].r = temp * 4;
   temp = (int)(*p++) + g + gr_palette_gamma;
   if (temp<0) temp=0;
   else if (temp>63) temp=63;
   colors[i].g = temp * 4;
   temp = (int)(*p++) + b + gr_palette_gamma;
   if (temp<0) temp=0;
   else if (temp>63) temp=63;
   colors[i].b = temp * 4;
 }

 SDL_SetColors(screen, colors, 0, 256);
}

//added on 980913 by adb to fix palette problems
// need a min without side effects...
#undef min
static inline int min(int x, int y) { return x < y ? x : y; }
//end changes by adb

void gr_palette_load( ubyte *pal )	
{
 int i, j;
 SDL_Palette *palette;
 SDL_Color colors[256];
 ubyte gamma[64];

 if (memcmp(pal,gr_current_pal,768))
     SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));

 for (i=0; i<768; i++ ) {
     gr_current_pal[i] = pal[i];
     if (gr_current_pal[i] > 63) gr_current_pal[i] = 63;
 }

 if (screen == NULL) {
    return;
 }

 palette = screen->format->palette;

 if (palette == NULL) {
    return; // Display is not palettised
 }

 for (i=0;i<64;i++)
     gamma[i] = (int)((pow(((double)(14)/(double)(32)), 1.0)*i) + 0.5);

 for (i = 0, j = 0; j < 256; j++) {
     int c;
     c = gr_find_closest_color(gamma[gr_palette[j*3]],gamma[gr_palette[j*3+1]],gamma[gr_palette[j*3+2]]);
     gr_fade_table[14*256+j] = c;
     //changed on 980913 by adb to fix palette problems
     colors[j].r = (min(gr_current_pal[i++] + gr_palette_gamma, 63)) * 4;
     colors[j].g = (min(gr_current_pal[i++] + gr_palette_gamma, 63)) * 4;
     colors[j].b = (min(gr_current_pal[i++] + gr_palette_gamma, 63)) * 4;
     //end changes by adb
 }

 SDL_SetColors(screen, colors, 0, 256);
// SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));
 init_computed_colors();
 gr_remap_color_fonts();
}

void gr_palette_read(ubyte * pal)
{
 SDL_Palette *palette;
 int i, j;

 palette = screen->format->palette;

 if (palette == NULL) {
    return; // Display is not palettised
 }

 for (i = 0, j=0; i < 256; i++) {
     pal[j++] = palette->colors[i].r / 4;
     pal[j++] = palette->colors[i].g / 4;
     pal[j++] = palette->colors[i].b / 4;
 }
}
