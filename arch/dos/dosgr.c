// Graphics functions for DOS.

#include <stdio.h>
#include <string.h>
#include <io.h>
#include <dos.h>
#include <dpmi.h>
#include "gr.h"
#include "grdef.h"
#include "palette.h"
#include "maths.h"
#include "u_mem.h"
#include "u_dpmi.h"
#include "vesa.h"
#include "modex.h"
#include "error.h"

#include "gamefont.h"

#ifdef __DJGPP__
#include <sys/nearptr.h>
#endif

static int last_r=0, last_g=0, last_b=0;

/* old gr.c stuff */
unsigned char * gr_video_memory = (unsigned char *)0xa0000;

char gr_pal_default[768];

int gr_installed = 0;

volatile ubyte * pVideoMode =  (volatile ubyte *)0x449;
volatile ushort * pNumColumns = (volatile ushort *)0x44a;
volatile ubyte * pNumRows = (volatile ubyte *)0x484;
volatile ushort * pCharHeight = (volatile ushort *)0x485;
volatile ushort * pCursorPos = (volatile ushort *)0x450;
volatile ushort * pCursorType = (volatile ushort *)0x460;
volatile ushort * pTextMemory = (volatile ushort *)0xb8000;

typedef struct screen_save {
	ubyte 	video_mode;
	ubyte 	is_graphics;
	ushort	char_height;
	ubyte		width;
	ubyte		height;
	ubyte		cursor_x, cursor_y;
	ubyte		cursor_sline, cursor_eline;
	ushort * video_memory;
} screen_save;

screen_save gr_saved_screen;

int gr_show_screen_info = 0;

void gr_set_cellheight( ubyte height )
{
	ubyte temp;

   outp( 0x3d4, 9 );
	temp = inp( 0x3d5 );
   temp &= 0xE0;
	temp |= height;
	outp( 0x3d5, temp );
}

void gr_set_linear()
{
	outpw( 0x3c4, 0xE04 );		  // enable chain4 mode
	outpw( 0x3d4, 0x4014 );		  // turn on dword mode
	outpw( 0x3d4, 0xa317 );		  // turn off byte mode
}

void gr_16_to_256()
{
	outpw( 0x3ce, 0x4005 );	 	// set Shift reg to 1

	inp( 0x3da );					// dummy input

	outp( 0x3c0, 0x30 );
	outp( 0x3c0, 0x61 );		   // turns on PCS & PCC

	inp( 0x3da );					// dummy input

	outp( 0x3c0, 0x33 );
	outp( 0x3c0, 0 );
}

void gr_turn_screen_off()
{
	ubyte temp;
	temp = inp( 0x3da );
	outp( 0x3c0, 0 );
}

void gr_turn_screen_on()
{
	ubyte temp;
	temp = inp( 0x3da );
	outp( 0x3c0, 0x20 );
}

void gr_set_misc_mode( uint mode )
{
	union REGS regs;

	memset( &regs, 0, sizeof(regs) );
	regs.w.ax = mode;
	int386( 0x10, &regs, &regs );

}

void gr_set_3dbios_mode( uint mode )
{
	union REGS regs;
	memset( &regs, 0, sizeof(regs) );
	regs.w.ax = 0x4fd0;
	regs.w.bx = 0x3d00 | (mode & 0xff);
	int386( 0x10, &regs, &regs );
}



void gr_set_text_25()
{
	union REGS regs;

	regs.w.ax = 3;
	int386( 0x10, &regs, &regs );

}

void gr_set_text_43()
{
	union REGS regs;

	regs.w.ax = 0x1201;
	regs.w.bx = 0x30;
	int386( 0x10, &regs, &regs );

	regs.w.ax = 3;
	int386( 0x10, &regs, &regs );

	regs.w.ax = 0x1112;
	regs.w.bx = 0x0;
	int386( 0x10, &regs, &regs );
}

void gr_set_text_50()
{
	union REGS regs;

	regs.w.ax = 0x1202;
	regs.w.bx = 0x30;
	int386( 0x10, &regs, &regs );

	regs.w.ax = 3;
	int386( 0x10, &regs, &regs );

	regs.w.ax = 0x1112;
	regs.w.bx = 0x0;
	int386( 0x10, &regs, &regs );
}

ubyte is_graphics_mode()
{
	byte tmp;
	tmp = inp( 0x3DA );		// Reset flip-flip
	outp( 0x3C0, 0x30 );		// Select attr register 10
	tmp = inp( 0x3C1 );	// Get graphics/text bit
	return tmp & 1;
}

void gr_setcursor(ubyte x, ubyte y, ubyte sline, ubyte eline)
{
	union REGS regs;

	memset( &regs, 0, sizeof(regs) );
	regs.w.ax = 0x0200;
	regs.w.bx = 0;
	regs.h.dh = y;
	regs.h.dl = x;
	int386( 0x10, &regs, &regs );

	memset( &regs, 0, sizeof(regs) );
	regs.w.ax = 0x0100;
	regs.h.ch = sline & 0xf;
	regs.h.cl = eline & 0xf;
	int386( 0x10, &regs, &regs );
}

void gr_getcursor(ubyte *x, ubyte *y, ubyte * sline, ubyte * eline)
{
	union REGS regs;

	memset( &regs, 0, sizeof(regs) );
	regs.w.ax = 0x0300;
	regs.w.bx = 0;
	int386( 0x10, &regs, &regs );
	*y = regs.h.dh;
	*x = regs.h.dl;
	*sline = regs.h.ch;
	*eline = regs.h.cl;
}


int gr_save_mode()
{
	int i;

	gr_saved_screen.is_graphics = is_graphics_mode();
	gr_saved_screen.video_mode = *pVideoMode;
	
	if (!gr_saved_screen.is_graphics)	{
		gr_saved_screen.width = *pNumColumns;
		gr_saved_screen.height = *pNumRows+1;
		gr_saved_screen.char_height = *pCharHeight;
		gr_getcursor(&gr_saved_screen.cursor_x, &gr_saved_screen.cursor_y, &gr_saved_screen.cursor_sline, &gr_saved_screen.cursor_eline );
		MALLOC(gr_saved_screen.video_memory,ushort, gr_saved_screen.width*gr_saved_screen.height );
		for (i=0; i < gr_saved_screen.width*gr_saved_screen.height; i++ )
			gr_saved_screen.video_memory[i] = pTextMemory[i];
	}

	if (gr_show_screen_info )	{
		printf( "Current video mode 0x%x:\n",  gr_saved_screen.video_mode );
		if (gr_saved_screen.is_graphics)
			printf( "Graphics mode\n" );
		else	{
			printf( "Text mode\n" );
			printf( "( %d columns by %d rows)\n", gr_saved_screen.width, gr_saved_screen.height );
			printf( "Char height is %d pixel rows\n", gr_saved_screen.char_height );
			printf( "Cursor of type 0x%x,0x%x is at (%d, %d)\n", gr_saved_screen.cursor_sline, gr_saved_screen.cursor_eline,gr_saved_screen.cursor_x, gr_saved_screen.cursor_y );
		}
	}

	return 0;
}

int isvga()
{
	union REGS regs;

	memset( &regs, 0, sizeof(regs) );
	regs.w.ax = 0x1a00;
	int386( 0x10, &regs, &regs );

	if ( regs.h.al == 0x1a )
		 return 1;

	return 0;
}

void gr_restore_mode()
{
	int i;

	//gr_set_text_25(); 

#ifndef NOGRAPH
	gr_palette_fade_out( gr_palette, 32, 0 );
	gr_palette_set_gamma(0);
	if ( gr_saved_screen.video_mode == 3 )	{
		switch( gr_saved_screen.height )	  {
		case 43:	gr_set_text_43(); break;
		case 50:	gr_set_text_50(); break;
		default:	gr_set_text_25(); break;
		}
	} else {
		gr_set_misc_mode(gr_saved_screen.video_mode);	
	}
	if (gr_saved_screen.is_graphics==0)	{
		gr_sync_display();
		gr_sync_display();
		gr_palette_read( gr_pal_default );
		gr_palette_clear();
		for (i=0; i < gr_saved_screen.width*gr_saved_screen.height; i++ )
			pTextMemory[i]=gr_saved_screen.video_memory[i];
		gr_setcursor( gr_saved_screen.cursor_x, gr_saved_screen.cursor_y, gr_saved_screen.cursor_sline, gr_saved_screen.cursor_eline );
		gr_palette_faded_out = 1;
		gr_palette_fade_in( gr_pal_default, 32, 0 );
	}
#endif

}

void gr_close()
{
	if (gr_installed==1)
	{
		gr_installed = 0;
		gr_restore_mode();
		free(grd_curscreen);
  		if( gr_saved_screen.video_memory ) {
			free(gr_saved_screen.video_memory);
			gr_saved_screen.video_memory = NULL;
		}
	}
}

#ifndef __DJGPP__
int gr_vesa_setmode( int mode )
{
	int retcode;

	retcode=gr_vesa_checkmode( mode );
	if ( retcode ) return retcode;

	return gr_vesa_setmodea( mode );
}
#endif


int gr_set_mode(u_int32_t mode)
{
	int retcode;
	unsigned int w,h,t,data, r;

	//JOHNgr_disable_default_palette_loading();
#ifdef NOGRAPH
return 0;
#endif
	switch(mode)
	{
	case SM_ORIGINAL:
		return 0;
	case SM(320,200)://0:
		if (!isvga()) return 1;
		gr_set_misc_mode(0x13);
		w = 320; r = 320; h = 200; t=BM_LINEAR; data = (int)gr_video_memory;
		break;
	case SM(640,400)://SM_640x400V:
		retcode = gr_vesa_setmode( 0x100 ); 
		if (retcode !=0 ) return retcode;
		w = 640; r = 640; h = 400; t=BM_SVGA; data = 0;
		break;
	case SM(640,480)://SM_640x480V:
		retcode = gr_vesa_setmode( 0x101 ); 
		if (retcode !=0 ) return retcode;
		w = 640; r = 640; h = 480; t=BM_SVGA; data = 0;
		break;
	case SM(800,600)://SM_800x600V:
		retcode = gr_vesa_setmode( 0x103 ); 
		if (retcode !=0 ) return retcode;
		w = 800; r = 800; h = 600; t=BM_SVGA; data = 0;
		break;
	case SM(1024,768)://SM_1024x768V:
		retcode = gr_vesa_setmode( 0x105 ); 
		if (retcode !=0 ) return retcode;
		w = 1024; r = 1024; h = 768; t=BM_SVGA; data = 0;
		break;
/*	case SM_640x480V15:
		retcode = gr_vesa_setmode( 0x110 ); 
		if (retcode !=0 ) return retcode;
		w = 640; r = 640*2; h=480; t=BM_SVGA15; data = 0;
		break;
	case SM_800x600V15:
		retcode = gr_vesa_setmode( 0x113 ); 
		if (retcode !=0 ) return retcode;
		w = 800; r = 800*2; h=600; t=BM_SVGA15; data = 0;
		break;*/
//	case 19:
	case SM(320,100):
		if (!isvga()) return 1;
		gr_set_misc_mode(0x13);	
//		{
//			ubyte x;
//			x = inp( 0x3c5 );
//			x |= 8;
//			outp( 0x3c5, x );
//		}
		gr_set_cellheight( 3 );

		w = 320; r = 320; h = 100; t=BM_LINEAR; data = (int)gr_video_memory;
		break;
/*	case 20:
		retcode = gr_vesa_setmode( 0x102 ); 
		//gr_enable_default_palette_loading();
		if (retcode !=0 ) return retcode;
		gr_16_to_256();
		gr_set_linear();
		//gr_set_cellheight( 1 );
		gr_vesa_setlogical( 400 );
		w = 400; r = 400; h = 600; t=BM_SVGA; data = 0;
		break;
	case 21:
		if (!isvga()) return 1;
		gr_set_misc_mode(0xd);	
		gr_16_to_256();
		gr_set_linear();
		gr_set_cellheight( 3 );
		w = 160; r = 160; h = 100; t=BM_LINEAR; data = (int)gr_video_memory;
		break;
	case //22:			// 3dmax 320x400
		if (!isvga()) return 1;
		gr_set_3dbios_mode(0x31);
		//w = 320; r = 320/4; h = 400; t=BM_MODEX; data = 0;
		w = 320; r = 320; h = 400; t=BM_SVGA; data = 0;
		break;*/
	default:
		if (!isvga()) return 1;
		if (mode==SM(320,240))
			w = gr_modex_setmode( 2 );
		else if (mode==SM(320,400))
			w = gr_modex_setmode( 6 );
		else
			Error("unhandled vid mode\n");
		//gr_enable_default_palette_loading();
		h = w & 0xffff; w = w >> 16; r = w / 4;t = BM_MODEX; data = 0;
		break;
	}
	gr_palette_clear();

	memset( grd_curscreen, 0, sizeof(grs_screen));
	grd_curscreen->sc_mode = mode;
	grd_curscreen->sc_w = w;
	grd_curscreen->sc_h = h;
	grd_curscreen->sc_aspect = fixdiv(grd_curscreen->sc_w*3,grd_curscreen->sc_h*4);
	grd_curscreen->sc_canvas.cv_bitmap.bm_x = 0;
	grd_curscreen->sc_canvas.cv_bitmap.bm_y = 0;
	grd_curscreen->sc_canvas.cv_bitmap.bm_w = w;
	grd_curscreen->sc_canvas.cv_bitmap.bm_h = h;
	grd_curscreen->sc_canvas.cv_bitmap.bm_rowsize = r;
	grd_curscreen->sc_canvas.cv_bitmap.bm_type = t;
	grd_curscreen->sc_canvas.cv_bitmap.bm_data = (unsigned char *)data;
	gr_set_current_canvas(NULL);

	//gr_enable_default_palette_loading();
	gamefont_choose_game_font(w,h);

	return 0;
}

int gr_init(int mode)
{
	int org_gamma;
	int retcode;

	// Only do this function once!
	if (gr_installed==1)
		return 1;

#ifdef __DJGPP__
	if (!__djgpp_nearptr_enable()) {
		printf("nearptr enable=%x\n", __dpmi_error);
		return 10;
	}
#ifndef SAVEGR
	gr_video_memory = (unsigned char *)(__djgpp_conventional_base + 0xa0000);
#else
	gr_video_memory=(unsigned char *)-1;
#endif
	pVideoMode =  (volatile ubyte *)(__djgpp_conventional_base+0x449);
	pNumColumns = (volatile ushort *)(__djgpp_conventional_base+0x44a);
	pNumRows = (volatile ubyte *)(__djgpp_conventional_base+0x484);
	pCharHeight = (volatile ushort *)(__djgpp_conventional_base+0x485);
	pCursorPos = (volatile ushort *)(__djgpp_conventional_base+0x450);
	pCursorType = (volatile ushort *)(__djgpp_conventional_base+0x460);
	pTextMemory = (volatile ushort *)(__djgpp_conventional_base+0xb8000);
#endif
#ifndef __DJGPP__
	if (gr_init_A0000())
		return 10;
#endif

	// Save the current text screen mode
	if (gr_save_mode()==1)
		return 1;

#ifndef NOGRAPH
	// Save the current palette, and fade it out to black.
	gr_palette_read( gr_pal_default );
	gr_palette_faded_out = 0;
	org_gamma = gr_palette_get_gamma();
	gr_palette_set_gamma( 0 );
	gr_palette_fade_out( gr_pal_default, 32, 0 );
	gr_palette_clear();
	gr_palette_set_gamma( org_gamma );
	gr_sync_display();
	gr_sync_display();
#endif

#ifdef __DJGPP__
#ifdef SAVEGR
	__djgpp_nearptr_disable();
#endif
#endif

	MALLOC( grd_curscreen,grs_screen,1 );
	memset( grd_curscreen, 0, sizeof(grs_screen));

	// Set the mode.
	if ((retcode=gr_set_mode(mode)))
	{
		gr_restore_mode();
		return retcode;
	}
	//JOHNgr_disable_default_palette_loading();

	// Set all the screen, canvas, and bitmap variables that
	// aren't set by the gr_set_mode call:
	grd_curscreen->sc_canvas.cv_color = 0;
	grd_curscreen->sc_canvas.cv_drawmode = 0;
	grd_curscreen->sc_canvas.cv_font = NULL;
	grd_curscreen->sc_canvas.cv_font_fg_color = 0;
	grd_curscreen->sc_canvas.cv_font_bg_color = 0;
	gr_set_current_canvas( &grd_curscreen->sc_canvas );

#if 0
	if (!dpmi_allocate_selector( &gr_fade_table, 256*GR_FADE_LEVELS, &gr_fade_table_selector ))
		Error( "Error allocating fade table selector!" );

	if (!dpmi_allocate_selector( &gr_palette, 256*3, &gr_palette_selector ))
		Error( "Error allocating palette selector!" );
#endif

//	if (!dpmi_allocate_selector( &gr_inverse_table, 32*32*32, &gr_inverse_table_selector ))
//		Error( "Error allocating inverse table selector!" );


	// Set flags indicating that this is installed.
	gr_installed = 1;
#ifdef __GNUC__

	atexit((void (*)) gr_close);
#else
	atexit(gr_close);
#endif
	return 0;
}

int gr_mode13_checkmode()
{
	if (isvga()) 
		return 0;
	else
		return 1;
}

//  0=Mode set OK
//  1=No VGA adapter installed
//  2=Program doesn't support this VESA granularity
//  3=Monitor doesn't support that VESA mode.:
//  4=Video card doesn't support that VESA mode.
//  5=No VESA driver found.
//  6=Bad Status after VESA call/
//  7=Not enough DOS memory to call VESA functions.
//  8=Error using DPMI.
//  9=Error setting logical line width.
// 10=Error allocating selector for A0000h
// 11=Not a valid mode support by gr.lib

int gr_check_mode(u_int32_t mode)
{
	switch(mode)
	{
	case SM(320,100)://19:
	case SM(320,200):
	case SM(320,240):
	case SM(360,200):
	case SM(360,240):
	case SM(376,282):
	case SM(320,400):
	case SM(320,480):
	case SM(360,400):
	case SM(360,480):
	case SM(360,360):
	case SM(376,308):
	case SM(376,564):		return gr_mode13_checkmode();
	case SM(640,400):		return gr_vesa_checkmode( 0x100 ); 
	case SM(640,480): 	return gr_vesa_checkmode( 0x101 ); 
	case SM(800,600): 	return gr_vesa_checkmode( 0x103 ); 
	case SM(1024,768):	return gr_vesa_setmode( 0x105 ); 
//	case SM_640x480V15:	return gr_vesa_setmode( 0x110 ); 
//	case SM_800x600V15:	return gr_vesa_setmode( 0x113 ); 
	}
	return 11;
}

/* Palette Stuff Starts Here... */

void gr_palette_step_up( int r, int g, int b )
{
	int i;
	ubyte *p;
	int temp;

	if (gr_palette_faded_out) return;

	if ( (r==last_r) && (g==last_g) && (b==last_b) ) return;

	last_r = r; last_g = g;	last_b = b;

	outp( 0x3c6, 0xff );
	outp( 0x3c8, 0 );
	p=gr_palette;

	for (i=0; i<256; i++ )	{
		temp = (int)(*p++) + r + gr_palette_gamma;
		if (temp<0) temp=0;
		else if (temp>63) temp=63;
		outp( 0x3c9, temp );
		temp = (int)(*p++) + g + gr_palette_gamma;
		if (temp<0) temp=0;
		else if (temp>63) temp=63;
		outp( 0x3c9, temp );
		temp = (int)(*p++) + b + gr_palette_gamma;
		if (temp<0) temp=0;
		else if (temp>63) temp=63;
		outp( 0x3c9, temp );
	}
}


void gr_palette_clear()
{
	int i;
	outp( 0x3c6, 0xff );
	outp( 0x3c8, 0 );
	for (i=0; i<768; i++ )	{
		outp( 0x3c9, 0 );
	}
	gr_palette_faded_out = 1;
}

void gr_palette_load( ubyte * pal )
{
	int i;
	ubyte c;
	outp( 0x3c6, 0xff );
	outp( 0x3c8, 0 );
	for (i=0; i<768; i++ )	{
		c = pal[i] + gr_palette_gamma;
		if ( c > 63 ) c = 63;
		outp( 0x3c9,c);
 		gr_current_pal[i] = pal[i];
	}
	gr_palette_faded_out = 0;

	init_computed_colors();
}


int gr_palette_fade_out(ubyte *pal, int nsteps, int allow_keys )
{
	ubyte c;
	int i,j;
	fix fade_palette[768];
	fix fade_palette_delta[768];

	if (gr_palette_faded_out) return 0;

	if (!pal)
		pal = gr_current_pal;

	for (i=0; i<768; i++ )	{
		fade_palette[i] = i2f(pal[i]+gr_palette_gamma);
		fade_palette_delta[i] = fade_palette[i] / nsteps;
	}

	for (j=0; j<nsteps; j++ )	{
		gr_sync_display();
		outp( 0x3c6, 0xff );
		outp( 0x3c8, 0 );
		for (i=0; i<768; i++ )	{
			fade_palette[i] -= fade_palette_delta[i];
			if (fade_palette[i] < 0 )
				fade_palette[i] = 0;
			c = f2i(fade_palette[i]);
			if ( c > 63 ) c = 63;
			outp( 0x3c9, c );
		}
	}
	gr_palette_faded_out = 1;
	return 0;
}

int gr_palette_fade_in(ubyte *pal, int nsteps, int allow_keys)
{
	int i,j;
	ubyte c;
	fix fade_palette[768];
	fix fade_palette_delta[768];


	if (!gr_palette_faded_out) return 0;

	for (i=0; i<768; i++ )	{
		gr_current_pal[i] = pal[i];
		fade_palette[i] = 0;
		fade_palette_delta[i] = i2f(pal[i]+gr_palette_gamma) / nsteps;
	}

	for (j=0; j<nsteps; j++ )	{
		gr_sync_display();
		outp( 0x3c6, 0xff );
		outp( 0x3c8, 0 );
		for (i=0; i<768; i++ )	{
			fade_palette[i] += fade_palette_delta[i];
			if (fade_palette[i] > i2f(pal[i]+gr_palette_gamma) )
				fade_palette[i] = i2f(pal[i]+gr_palette_gamma);
			c = f2i(fade_palette[i]);
                        if ( c > 63 ) c = 63;
			outp( 0x3c9, c );
		}
	}
	gr_palette_faded_out = 0;
	return 0;
}

void gr_palette_read(ubyte * palette)
{
	int i;
	outp( 0x3c6, 0xff );
	outp( 0x3c7, 0 );
	for (i=0; i<768; i++ )	{
		*palette++ = inp( 0x3c9 );
	}
}

void gr_update(void)
{ }
