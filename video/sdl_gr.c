/*
 * $Source: /cvs/cvsroot/d2x/video/sdl_gr.c,v $
 * $Revision: 1.4 $
 * $Author: bradleyb $
 * $Date: 2001-01-31 13:59:23 $
 *
 * SDL video functions.
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.3  2001/01/29 13:47:52  bradleyb
 * Fixed build, some minor cleanups.
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <SDL/SDL.h>
#include "gr.h"
#include "grdef.h"
#include "palette.h"
#include "u_mem.h"
#include "error.h"
//added on 9/30/98 by Matt Mueller to set the title bar.  Woohoo!
#include "vers_id.h"
//end addition -MM

#include "gamefont.h"

//added 10/05/98 by Matt Mueller - make fullscreen mode optional
#include "args.h"

int sdl_video_flags = SDL_SWSURFACE | SDL_HWPALETTE;
char checkvidmodeok=0;
//end addition -MM

SDL_Surface *screen;

int gr_installed = 0;

//added 05/19/99 Matt Mueller - locking stuff
#ifdef GR_LOCK
#include "checker.h"
#ifdef TEST_GR_LOCK
int gr_testlocklevel=0;
#endif
inline void gr_dolock(const char *file,int line) {
	gr_dotestlock();
	if ( gr_testlocklevel==1 && SDL_MUSTLOCK(screen) ) {
#ifdef __CHECKER__
		chcksetwritable(screen.pixels,screen->w*screen->h*screen->format->BytesPerPixel);
#endif
		if ( SDL_LockSurface(screen) < 0 )Error("could not lock screen (%s:%i)\n",file,line);
	}
}
inline void gr_dounlock(void) {
	gr_dotestunlock();
	if (gr_testlocklevel==0 && SDL_MUSTLOCK(screen) ) {
		SDL_UnlockSurface(screen);
#ifdef __CHECKER__
		chcksetunwritable(screen.pixels,screen->w*screen->h*screen->format->BytesPerPixel);
#endif
	}
}
#endif
//end addition -MM

void gr_palette_clear(); // Function prototype for gr_init;


void gr_update()
{
	//added 05/19/99 Matt Mueller - locking stuff
//
//gr_testunlock();
	//end addition -MM
 SDL_UpdateRect(screen,0,0,0,0);
}

extern int VGA_current_mode; // DPH: kludge - remove at all costs

int gr_set_mode(u_int32_t mode)
{
	int w,h;
#ifdef NOGRAPH
	return 0;
#endif

	if (mode<=0)
		return 0;

	w=SM_W(mode);
	h=SM_H(mode);
	VGA_current_mode = mode;
	
	if (screen != NULL) gr_palette_clear();

//added on 11/06/98 by Matt Mueller to set the title bar. (moved from below)
//sekmu: might wanna copy this litte blurb to one of the text files or something
//we want to set it here so that X window manager "Style" type commands work
//for example, in fvwm2 or fvwm95:
//Style "D1X*"  NoTitle, NoHandles, BorderWidth 0
//if you can't use -fullscreen like me (crashes X), this is a big help in
//getting the window centered correctly (if you use SmartPlacement)
	SDL_WM_SetCaption("D2x", "Descent II");
//end addition -MM

//edited 10/05/98 by Matt Mueller - make fullscreen mode optional
	  // changed by adb on 980913: added SDL_HWPALETTE (should be option?)
        // changed by someone on 980923 to add SDL_FULLSCREEN
	if(!checkvidmodeok || SDL_VideoModeOK(w,h,8,sdl_video_flags)){
	  screen = SDL_SetVideoMode(w, h, 8, sdl_video_flags);
	} else {
	  screen=NULL;
	}
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
	grd_curscreen->sc_aspect = fixdiv(grd_curscreen->sc_w*3,grd_curscreen->sc_h*4);
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

//	gamefont_choose_game_font(w,h);
	return 0;
}

int gr_check_fullscreen(void){
	return (sdl_video_flags & SDL_FULLSCREEN)?1:0;
}

int gr_toggle_fullscreen(void){
	sdl_video_flags^=SDL_FULLSCREEN;
/*
	grd_curscreen->sc_mode=0;//hack to get it to reset screen mode
*/
        SDL_WM_ToggleFullScreen(screen);
	return (sdl_video_flags & SDL_FULLSCREEN)?1:0;
}

int gr_init(void)
{
 int retcode;
 int mode = SM(640,480);
 	// Only do this function once!
	if (gr_installed==1)
		return -1;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		Error("SDL library video initialisation failed: %s.",SDL_GetError());
	}
	MALLOC( grd_curscreen,grs_screen,1 );
	memset( grd_curscreen, 0, sizeof(grs_screen));

//added 10/05/98 by Matt Mueller - make fullscreen mode optional
	if (FindArg("-fullscreen"))
	     sdl_video_flags|=SDL_FULLSCREEN;
//end addition -MM
	//added 05/19/99 Matt Mueller - make HW surface optional
	if (FindArg("-hwsurface"))
	     sdl_video_flags|=SDL_HWSURFACE;
	//end addition -MM
	if (FindArg("-nosdlvidmodecheck"))
		checkvidmodeok=0;
	
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

void gr_palette_clear()
{
 SDL_Palette *palette;
 SDL_Color colors[256];
 int ncolors;

 palette = screen->format->palette;

 if (palette == NULL) {
    return; // Display is not palettised
 }

 ncolors = palette->ncolors;
 memset(colors, 0, ncolors * sizeof(SDL_Color));

 SDL_SetColors(screen, colors, 0, 256);

 gr_palette_faded_out = 1;
}


void gr_palette_step_up( int r, int g, int b )
{
 int i;
 ubyte *p = gr_palette;
 int temp;

 SDL_Palette *palette;
 SDL_Color colors[256];

 if (gr_palette_faded_out) return;

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

 for (i=0; i<768; i++ ) {
     gr_current_pal[i] = pal[i];
     if (gr_current_pal[i] > 63) gr_current_pal[i] = 63;
 }

 palette = screen->format->palette;

 if (palette == NULL) {
    return; // Display is not palettised
 }

 for (i = 0, j = 0; j < 256; j++) {
     //changed on 980913 by adb to fix palette problems
     colors[j].r = (min(gr_current_pal[i++] + gr_palette_gamma, 63)) * 4;
     colors[j].g = (min(gr_current_pal[i++] + gr_palette_gamma, 63)) * 4;
     colors[j].b = (min(gr_current_pal[i++] + gr_palette_gamma, 63)) * 4;
     //end changes by adb
 }
 SDL_SetColors(screen, colors, 0, 256);

 gr_palette_faded_out = 0;
 init_computed_colors();
}



int gr_palette_fade_out(ubyte *pal, int nsteps, int allow_keys)
{
 int i, j, k;
 ubyte c;
 fix fade_palette[768];
 fix fade_palette_delta[768];

 SDL_Palette *palette;
 SDL_Color fade_colors[256];

 if (gr_palette_faded_out) return 0;

 palette = screen->format->palette;
 if (palette == NULL) {
    return -1; // Display is not palettised
 }

 if (pal==NULL) pal=gr_current_pal;

 for (i=0; i<768; i++ )	{
     gr_current_pal[i] = pal[i];
     fade_palette[i] = i2f(pal[i]);
     fade_palette_delta[i] = fade_palette[i] / nsteps;
 }
 for (j=0; j<nsteps; j++ )	{
     for (i=0, k = 0; k<256; k++ )	{
         fade_palette[i] -= fade_palette_delta[i];
         if (fade_palette[i] > i2f(pal[i] + gr_palette_gamma) )
            fade_palette[i] = i2f(pal[i] + gr_palette_gamma);
	 c = f2i(fade_palette[i]);
         if (c > 63) c = 63;
         fade_colors[k].r = c * 4;
         i++;

         fade_palette[i] -= fade_palette_delta[i];
         if (fade_palette[i] > i2f(pal[i] + gr_palette_gamma) )
            fade_palette[i] = i2f(pal[i] + gr_palette_gamma);
	 c = f2i(fade_palette[i]);
         if (c > 63) c = 63;
         fade_colors[k].g = c * 4;
         i++;

         fade_palette[i] -= fade_palette_delta[i];
         if (fade_palette[i] > i2f(pal[i] + gr_palette_gamma) )
            fade_palette[i] = i2f(pal[i] + gr_palette_gamma);
	 c = f2i(fade_palette[i]);
         if (c > 63) c = 63;
         fade_colors[k].b = c * 4;
         i++;
     }

  SDL_SetColors(screen, fade_colors, 0, 256);
 }

 gr_palette_faded_out = 1;
 return 0;
}



int gr_palette_fade_in(ubyte *pal, int nsteps, int allow_keys)
{
 int i, j, k, ncolors;
 ubyte c;
 fix fade_palette[768];
 fix fade_palette_delta[768];

 SDL_Palette *palette;
 SDL_Color fade_colors[256];

 if (!gr_palette_faded_out) return 0;

 palette = screen->format->palette;

 if (palette == NULL) {
    return -1; // Display is not palettised
 }

 ncolors = palette->ncolors;

 for (i=0; i<768; i++ )	{
     gr_current_pal[i] = pal[i];
     fade_palette[i] = 0;
     fade_palette_delta[i] = i2f(pal[i]) / nsteps;
 }

 for (j=0; j<nsteps; j++ )	{
     for (i=0, k = 0; k<256; k++ )	{
         fade_palette[i] += fade_palette_delta[i];
         if (fade_palette[i] > i2f(pal[i] + gr_palette_gamma) )
            fade_palette[i] = i2f(pal[i] + gr_palette_gamma);
	 c = f2i(fade_palette[i]);
         if (c > 63) c = 63;
         fade_colors[k].r = c * 4;
         i++;

         fade_palette[i] += fade_palette_delta[i];
         if (fade_palette[i] > i2f(pal[i] + gr_palette_gamma) )
            fade_palette[i] = i2f(pal[i] + gr_palette_gamma);
	 c = f2i(fade_palette[i]);
         if (c > 63) c = 63;
         fade_colors[k].g = c * 4;
         i++;

         fade_palette[i] += fade_palette_delta[i];
         if (fade_palette[i] > i2f(pal[i] + gr_palette_gamma) )
            fade_palette[i] = i2f(pal[i] + gr_palette_gamma);
	 c = f2i(fade_palette[i]);
         if (c > 63) c = 63;
         fade_colors[k].b = c * 4;
         i++;
     }

  SDL_SetColors(screen, fade_colors, 0, 256);
 }
 //added on 980913 by adb to fix palette problems
 gr_palette_load(pal);
 //end changes by adb

 gr_palette_faded_out = 0;
 return 0;
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
