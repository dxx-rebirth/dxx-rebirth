#include <ggi/ggi.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "gr.h"
#include "grdef.h"
#include "palette.h"
#include "u_mem.h"
#include "error.h"

#include "gamefont.h"

int gr_installed = 0;
void *screenbuffer;

ggi_visual_t *screenvis;
const ggi_directbuffer *dbuffer;

ubyte use_directbuffer;

void gr_update()
{
	ggiFlush(screenvis);
	if (!use_directbuffer)
		ggiPutBox(screenvis, 0, 0, grd_curscreen->sc_w, grd_curscreen->sc_h, screenbuffer);
}

int gr_set_mode(u_int32_t mode)
{
	unsigned int w, h;
	ggi_mode other_mode;	

#ifdef NOGRAPH
	return 0;
#endif
	if (mode<=0)
		return 0;

	w=SM_W(mode);
	h=SM_H(mode);
	
	
	gr_palette_clear();

	if(ggiCheckGraphMode(screenvis, w, h, GGI_AUTO, GGI_AUTO, GT_8BIT, &other_mode))
		ggiSetMode(screenvis, &other_mode);
	else
		ggiSetGraphMode(screenvis, w, h, GGI_AUTO, GGI_AUTO, GT_8BIT);

	ggiSetFlags(screenvis, GGIFLAG_ASYNC);
		
	if (!ggiDBGetNumBuffers(screenvis))
		use_directbuffer = 0;
	else
	{	
		dbuffer = ggiDBGetBuffer(screenvis, 0);
		if (!(dbuffer->type & GGI_DB_SIMPLE_PLB))
			use_directbuffer = 0;
		else
			use_directbuffer = 1;
	}

        memset(grd_curscreen, 0, sizeof(grs_screen));

	grd_curscreen->sc_mode = mode;
	grd_curscreen->sc_w = w;
	grd_curscreen->sc_h = h;
	grd_curscreen->sc_aspect = fixdiv(grd_curscreen->sc_w*3,grd_curscreen->sc_h*4);
	grd_curscreen->sc_canvas.cv_bitmap.bm_x = 0;
	grd_curscreen->sc_canvas.cv_bitmap.bm_y = 0;
	grd_curscreen->sc_canvas.cv_bitmap.bm_w = w;
	grd_curscreen->sc_canvas.cv_bitmap.bm_h = h;
	grd_curscreen->sc_canvas.cv_bitmap.bm_type = BM_LINEAR;

	if (use_directbuffer)
	{
	        grd_curscreen->sc_canvas.cv_bitmap.bm_data = dbuffer->write;
        	grd_curscreen->sc_canvas.cv_bitmap.bm_rowsize = dbuffer->buffer.plb.stride;
	}
	else
	{
		free(screenbuffer);
		screenbuffer = malloc (w * h);
		grd_curscreen->sc_canvas.cv_bitmap.bm_data = screenbuffer;
		grd_curscreen->sc_canvas.cv_bitmap.bm_rowsize = w;
	}

	gr_set_current_canvas(NULL);
	
	gamefont_choose_game_font(w,h);
	
	return 0;
}

int gr_init(int mode)
{
	int retcode;
 	// Only do this function once!
	if (gr_installed==1)
		return -1;
	MALLOC(grd_curscreen,grs_screen, 1);
	memset(grd_curscreen, 0, sizeof(grs_screen));
	
	ggiInit();
	screenvis = ggiOpen(NULL);

	if ((retcode=gr_set_mode(mode)))
		return retcode;
	
	grd_curscreen->sc_canvas.cv_color = 0;
	grd_curscreen->sc_canvas.cv_drawmode = 0;
	grd_curscreen->sc_canvas.cv_font = NULL;
	grd_curscreen->sc_canvas.cv_font_fg_color = 0;
	grd_curscreen->sc_canvas.cv_font_bg_color = 0;
	gr_set_current_canvas( &grd_curscreen->sc_canvas );

	gr_installed = 1;
	atexit(gr_close);
	return 0;
}

void gr_close ()
{
	if (gr_installed==1)
	{
		ggiClose(screenvis);
		ggiExit();
		gr_installed = 0;
		free(grd_curscreen);
	}
}

// Palette functions follow.

static int last_r=0, last_g=0, last_b=0;

void gr_palette_clear()
{
	ggi_color *colors = calloc (256, sizeof(ggi_color));

	ggiSetPalette (screenvis, 0, 256, colors);

	gr_palette_faded_out = 1;
}


void gr_palette_step_up (int r, int g, int b)
{
	int i;
	ubyte *p = gr_palette;
	int temp;

	ggi_color colors[256];

	if (gr_palette_faded_out) return;

	if ((r==last_r) && (g==last_g) && (b==last_b)) return;

	last_r = r;
	last_g = g;
	last_b = b;

	for (i = 0; i < 256; i++)
	{
		temp = (int)(*p++) + r + gr_palette_gamma;
		if (temp<0) temp=0;
		else if (temp>63) temp=63;
		colors[i].r = temp * 0x3ff;
		temp = (int)(*p++) + g + gr_palette_gamma;
		if (temp<0) temp=0;
		else if (temp>63) temp=63;
		colors[i].g = temp * 0x3ff;
		temp = (int)(*p++) + b + gr_palette_gamma;
		if (temp<0) temp=0;
		else if (temp>63) temp=63;
		colors[i].b = temp * 0x3ff;
	}
	ggiSetPalette (screenvis, 0, 256, colors);
}

//added on 980913 by adb to fix palette problems
// need a min without side effects...
#undef min
static inline int min(int x, int y) { return x < y ? x : y; }
//end changes by adb

void gr_palette_load (ubyte *pal)	
{
	int i, j;
	ggi_color colors[256];

	for (i = 0, j = 0; j < 256; j++)
	{
		gr_current_pal[i] = pal[i];
		if (gr_current_pal[i] > 63) gr_current_pal[i] = 63;
		colors[j].r = (min(gr_current_pal[i] + gr_palette_gamma, 63)) * 0x3ff;
		i++;
                gr_current_pal[i] = pal[i];
                if (gr_current_pal[i] > 63) gr_current_pal[i] = 63;
                colors[j].g = (min(gr_current_pal[i] + gr_palette_gamma, 63)) * 0x3ff;
		i++;
                gr_current_pal[i] = pal[i];
                if (gr_current_pal[i] > 63) gr_current_pal[i] = 63;
                colors[j].b = (min(gr_current_pal[i] + gr_palette_gamma, 63)) * 0x3ff;
		i++;
	}

	ggiSetPalette(screenvis, 0, 256, colors);

	gr_palette_faded_out = 0;
	init_computed_colors();
}



int gr_palette_fade_out (ubyte *pal, int nsteps, int allow_keys)
{
	int i, j, k;
	ubyte c;
	fix fade_palette[768];
	fix fade_palette_delta[768];

	ggi_color fade_colors[256];

	if (gr_palette_faded_out) return 0;

	if (pal==NULL) pal=gr_current_pal;

	for (i=0; i<768; i++)
	{
		gr_current_pal[i] = pal[i];
		fade_palette[i] = i2f(pal[i]);
		fade_palette_delta[i] = fade_palette[i] / nsteps;
	}

	for (j=0; j<nsteps; j++)
	{
		for (i = 0; i < 768; i++)
		{
			fade_palette[i] -= fade_palette_delta[i];
			if (fade_palette[i] > i2f(pal[i] + gr_palette_gamma))
				fade_palette[i] = i2f(pal[i] + gr_palette_gamma);
		}
		for (i = 0, k = 0; k < 256; k++)
		{
			c = f2i(fade_palette[i++]);
			if (c > 63) c = 63;
			fade_colors[k].r = c * 0x3ff;
                        c = f2i(fade_palette[i++]);
                        if (c > 63) c = 63;
                        fade_colors[k].g = c * 0x3ff;
                        c = f2i(fade_palette[i++]);
                        if (c > 63) c = 63;
                        fade_colors[k].b = c * 0x3ff;
		}
		ggiSetPalette (screenvis, 0, 256, fade_colors);
		gr_update();
	}

	gr_palette_faded_out = 1;
	return 0;
}



int gr_palette_fade_in (ubyte *pal, int nsteps, int allow_keys)
{
	int i, j, k;
	ubyte c;
	fix fade_palette[768];
	fix fade_palette_delta[768];

	ggi_color fade_colors[256];

	if (!gr_palette_faded_out) return 0;

	for (i=0; i<768; i++)
	{
		gr_current_pal[i] = pal[i];
		fade_palette[i] = 0;
		fade_palette_delta[i] = i2f(pal[i] + gr_palette_gamma) / nsteps;
	}

 	for (j=0; j<nsteps; j++ )
	{
		for (i = 0; i < 768; i++ )
		{
			fade_palette[i] += fade_palette_delta[i];
			if (fade_palette[i] > i2f(pal[i] + gr_palette_gamma))
				fade_palette[i] = i2f(pal[i] + gr_palette_gamma);
		}
         	for (i = 0, k = 0; k < 256; k++)
		{
                        c = f2i(fade_palette[i++]);
                        if (c > 63) c = 63;
                        fade_colors[k].r = c * 0x3ff;
                        c = f2i(fade_palette[i++]);
                        if (c > 63) c = 63;
                        fade_colors[k].g = c * 0x3ff;
                        c = f2i(fade_palette[i++]);
                        if (c > 63) c = 63;
                        fade_colors[k].b = c * 0x3ff;
                }
                ggiSetPalette (screenvis, 0, 256, fade_colors);
		gr_update();
	}

	gr_palette_faded_out = 0;
	return 0;
}



void gr_palette_read (ubyte *pal)
{
	ggi_color colors[256];
	int i, j;

	ggiGetPalette (screenvis, 0, 256, colors);

	for (i = 0, j = 0; i < 256; i++)
	{
                pal[j++] = colors[i].r / 0x3ff;
                pal[j++] = colors[i].g / 0x3ff;
                pal[j++] = colors[i].b / 0x3ff;
	}
}

