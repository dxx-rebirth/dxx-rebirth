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

#include "pa_enabl.h"   //$$POLY_ACC

#include <dos.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <stdio.h>
#include <conio.h>

#include "types.h"
#include "mem.h"
#include "error.h"
#include "mono.h"

//	New for VGA Library.
#include "gr.h"
#include "vga.h"
#include "palette.h"
#include "grdef.h"
#include "fxvesa.h"

#if defined(POLY_ACC)
#include "poly_acc.h"
#endif

// Global Variables -----------------------------------------------------------

ubyte *vga_screen_addr = NULL;

unsigned char * vga_video_memory = (unsigned char *)0xA0000;

char vga_pal_default[768];

int vga_installed = 0;

//extern int gr_palette_realized;
int vga_prevent_palette_loading = 0;

ubyte * pVideoMode =  (volatile ubyte *)0x449;
uword * pNumColumns = (volatile uword *)0x44a;
ubyte * pNumRows = (volatile ubyte *)0x484;
uword * pCharHeight = (volatile uword *)0x485;
uword * pCursorPos = (volatile uword *)0x450;
uword * pCursorType = (volatile uword *)0x460;
uword * pTextMemory = (volatile uword *)0xb8000;

ubyte saved_video_mode = 0;
uword saved_num_columns = 0;
ubyte saved_num_rows = 0;
uword saved_char_height = 0;
uword saved_cursor_pos = 0;
uword saved_cursor_type = 0;
ubyte saved_is_graphics = 0;
uword * saved_video_memory = NULL;

int cga_show_screen_info = 0;


//	VGA Functions --------------------------------------------------------------

ubyte vga_get_screen_mode( void )
{
	union REGS regs;

	regs.w.ax = 0x0f00;
	int386( 0x10, &regs, &regs );
	return regs.w.ax & 0xff;
}

void vga_set_cellheight( ubyte height )
{
	ubyte temp;

   outp( 0x3d4, 9 );
	temp = inp( 0x3d5 );
   temp &= 0xE0;
	temp |= height;
	outp( 0x3d5, temp );
}

void vga_set_linear()
{
	outpw( 0x3c4, 0xE04 );		  // enable chain4 mode
	outpw( 0x3d4, 0x4014 );		  // turn on dword mode
	outpw( 0x3d4, 0xa317 );		  // turn off byte mode
}

void vga_16_to_256()
{
	outpw( 0x3ce, 0x4005 );	 	// set Shift reg to 1

	inp( 0x3da );					// dummy input

	outp( 0x3c0, 0x30 );
	outp( 0x3c0, 0x61 );		   // turns on PCS & PCC

	inp( 0x3da );					// dummy input

	outp( 0x3c0, 0x33 );
	outp( 0x3c0, 0 );
}

void vga_turn_screen_off()
{
	ubyte temp;
	temp = inp( 0x3da );
	outp( 0x3c0, 0 );
}

void vga_turn_screen_on()
{
	ubyte temp;
	temp = inp( 0x3da );
	outp( 0x3c0, 0x20 );
}

void vga_set_misc_mode( uword mode )
{
	union REGS regs;

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


void vga_set_text_25()
{
	union REGS regs;

	regs.w.ax = 3;
	int386( 0x10, &regs, &regs );

}

void vga_set_text_43()
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

void vga_set_text_50()
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

int vga_save_mode()
{
	int i;

	saved_video_mode = vga_get_screen_mode();
	saved_num_columns = *pNumColumns;
	saved_num_rows = *pNumRows+1;
	saved_char_height = *pCharHeight;
	saved_cursor_pos = *pCursorPos;
	saved_cursor_type = *pCursorType;
	saved_is_graphics = is_graphics_mode();

	if (saved_is_graphics==0)
	{
		MALLOC(saved_video_memory,uword, saved_num_columns*saved_num_rows );

		for (i=0; i < saved_num_columns*saved_num_rows; i++ )
			saved_video_memory[i] = pTextMemory[i];
	}

	if (cga_show_screen_info )
	{
		printf( "Current video mode 0x%x:\n",  saved_video_mode );
		if (saved_is_graphics)
			printf( "Graphics mode\n" );
		else
			printf( "Text mode\n" );
	
		printf( "( %d columns by %d rows)\n", saved_num_columns, saved_num_rows );
		printf( "Char height is %d pixel rows\n", saved_char_height );
		printf( "Cursor of type 0x%x is at (%d, %d)\n", saved_cursor_type, saved_cursor_pos & 0xFF, saved_cursor_pos >> 8 );
	}

	return 0;
}

int isvga()
{
	union REGS regs;

	regs.w.ax = 0x1a00;
	int386( 0x10, &regs, &regs );

	if ( regs.h.al != 0x1a ) return 0;
	if ( (regs.h.bl==7) || (regs.h.bl==8)) return 1;
	return 0;
}

void vga_set_cursor_type( uword ctype )
{
	union REGS regs;

	regs.w.ax = 0x0100;
	regs.w.cx = ctype;
	int386( 0x10, &regs, &regs );
}

void vga_enable_default_palette_loading()
{
	union REGS regs;
	regs.w.ax = 0x1200;
	regs.w.bx = 0x31;
	int386( 0x10, &regs, &regs );
	vga_prevent_palette_loading = 0;
}

void vga_disable_default_palette_loading()
{
	union REGS regs;
	regs.w.ax = 0x1201;
	regs.w.bx = 0x31;
	int386( 0x10, &regs, &regs );
	vga_prevent_palette_loading = 1;
}

void vga_set_cursor_position( uword position )
{
	union REGS regs;

	regs.w.ax = 0x0200;
	regs.w.bx = 0;
	regs.w.dx = position;
	int386( 0x10, &regs, &regs );

}

void vga_restore_mode()
{
	int i;

	if ( saved_video_mode == 3 )
	{
		switch( saved_num_rows )
		{
			case 43:	vga_set_text_43(); break;
			case 50:	vga_set_text_50(); break;
			default:	vga_set_text_25(); break;
		}
	} else {
		vga_set_misc_mode(saved_video_mode);	
	}

	if (saved_is_graphics==0)
	{
		for (i=0; i < saved_num_columns*saved_num_rows; i++ )
			pTextMemory[i]=saved_video_memory[i];
		vga_set_cursor_type( saved_cursor_type );
		vga_set_cursor_position( saved_cursor_pos );
	}
}

short vga_close()
{
#if defined(POLY_ACC)
        pa_reset();
#endif

	if (vga_installed==1)
  	{
		vga_installed = 0;
		vga_restore_mode();
  		if( saved_video_memory )
			free(saved_video_memory);
	}

	return 0;
}

int LinearSVGABuffer=0;

int vga_vesa_setmode( short mode )
{
	int retcode;

#if defined(POLY_ACC)
	Int3();
    mprintf((0, "vga_vesa_setmode %d\n"));
    return 0;
#endif

	if (VesaInit (mode))  // if there is an error with linear support
	 {
		LinearSVGABuffer=1;
		return(0);
	 }
	
   LinearSVGABuffer=0;
	retcode=gr_vesa_checkmode( mode );		// do the old banking way
	if ( retcode ) return retcode;
	return gr_vesa_setmodea( mode );
}

int VGA_current_mode = SM_ORIGINAL;
extern int VesaGetRowSize (int);

short vga_set_mode(short mode)
{
	int retcode;
	unsigned int w,h,t,data, r;

#if defined(POLY_ACC)
if(mode != SM_640x480x15xPA)
{
    Int3();
    mprintf((0, "vga_set_mode(%d)\n", mode));
    return 0;
}
#endif

	LinearSVGABuffer=0; 
	   
	if (!vga_installed)
		return 1;
   
  
	switch(mode)
	{
	case SM_ORIGINAL:
		return 0;

	case SM_320x200C:
		if (!isvga()) return 1;
		vga_set_misc_mode(0x13);	
		w = 320; r = 320; h = 200; t=BM_LINEAR; data = 0xA0000;
		break;
	
	case SM_640x400V:
		retcode = vga_vesa_setmode( 0x100 ); 
		//gr_enable_default_palette_loading();
		if (retcode !=0 ) return retcode;
		w = 640; r = 640; h = 400; t=BM_SVGA; data = 0;
		break;

	case SM_640x480V:
		retcode = vga_vesa_setmode( 0x101 ); 
		//gr_enable_default_palette_loading();
		if (retcode !=0 ) return retcode;
		w = 640; r = 640; h = 480; t=BM_SVGA; data = 0;
		break;

	case SM_800x600V:
		retcode = vga_vesa_setmode( 0x103 ); 
		//gr_enable_default_palette_loading();
		if (retcode !=0 ) return retcode;
		w = 800; h = 600; t=BM_SVGA; data = 0;
      r=VesaGetRowSize (0x103);
		break;

	case SM_1024x768V:
		retcode = vga_vesa_setmode( 0x105 ); 
		//gr_enable_default_palette_loading();
		if (retcode !=0 ) return retcode;
		w = 1024; r = 1024; h = 768; t=BM_SVGA; data = 0;
      r=VesaGetRowSize (0x105);
		break;

	case SM_1280x1024V:
		retcode = vga_vesa_setmode( 0x107 ); 
		//gr_enable_default_palette_loading();
		if (retcode !=0 ) return retcode;
		w = 1280; r = 1280; h = 1024; t=BM_SVGA; data = 0;
      r=VesaGetRowSize (0x107);
		break;

	case SM_640x480V15:
		retcode = vga_vesa_setmode( 0x110 ); 
		//gr_enable_default_palette_loading();
		if (retcode !=0 ) return retcode;
		w = 640; r = 640*2; h=480; t=BM_SVGA15; data = 0;
		break;

	case SM_800x600V15:
		retcode = vga_vesa_setmode( 0x113 ); 
		//gr_enable_default_palette_loading();
		if (retcode !=0 ) return retcode;
		w = 800; r = 800*2; h=600; t=BM_SVGA15; data = 0;
		break;

	//super-special code for 3dmax high-res mode
	case SM_320x400_3DMAX:			// 3dmax 320x400
		if (!isvga()) return 1;
		gr_set_3dbios_mode(0x31);
		//w = 320; r = 320/4; h = 400; t=BM_MODEX; data = 0;
		w = 320; r = 320; h = 400; t=BM_SVGA; data = 0;
		break;

	//@@case 19:
	//@@	if (!isvga()) return 1;
	//@@	vga_set_misc_mode(0x13);	
	//@@	vga_set_cellheight( 3 );
	//@@	w = 320; r = 320; h = 200; t=BM_LINEAR; data = 0xA0000;
	//@@	break;
	//@@
	//@@case 20:
	//@@	retcode = vga_vesa_setmode( 0x102 ); 
	//@@	//gr_enable_default_palette_loading();
	//@@	if (retcode !=0 ) return retcode;
	//@@	vga_16_to_256();
	//@@	vga_set_linear();
	//@@	//gr_set_cellheight( 1 );
	//@@	gr_vesa_setlogical( 400 );
	//@@	w = 400; r = 400; h = 600; t=BM_SVGA; data = 0;
	//@@	break;
	//@@
	//@@case 21:
	//@@	if (!isvga()) return 1;
	//@@	vga_set_misc_mode(0xd);	
	//@@	vga_16_to_256();
	//@@	vga_set_linear();
	//@@	//gr_set_cellheight( 3 );
	//@@	w = 160; r = 160; h = 200; t=BM_LINEAR; data = 0xA0000;
  	//@@	break;

	case SM_320x200U:
	case SM_320x240U:
	case SM_360x200U:
	case SM_360x240U:
	case SM_376x282U:
	case SM_320x400U:
	case SM_320x480U:
	case SM_360x400U:
	case SM_360x480U:
	case SM_360x360U:
	case SM_376x308U:
	case SM_376x564U:		//mode X modes

		if (!isvga()) return 1;
		w = gr_modex_setmode( mode );
		//gr_enable_default_palette_loading();
		h = w & 0xffff; w = w >> 16; r = w / 4;t = BM_MODEX; data = 0;
		break;

#if defined(POLY_ACC)
	case SM_640x480x15xPA:
		pa_set_mode(mode);
		w = 640; r = 640*2; h=480; t=BM_LINEAR15; data = 0; //$$ data should be what?
	   #ifdef PA_3DFX_VOODOO
		  r=2048;
		#endif
		break;
#endif

	default:			//unknown mode!!!  Very bad!!
		Error("Unknown mode %d in vga_set_mode()",mode);
	}

//	if (vga_palette_realized  && (vga_prevent_palette_loading==0))
//		gr_pal_setblock( 0, 256, gr_palette);

//	return 0;

	VGA_current_mode = mode;
#if defined(POLY_ACC)
    vga_screen_addr = pa_get_buffer_address(0);
#else
	if (LinearSVGABuffer)
		{
			mprintf ((0,"GREEEEAT!\n"));
			t=BM_LINEAR;
			vga_screen_addr=(ubyte *)VesaGetPtr();
		}
	else
		vga_screen_addr=(ubyte *)0xa0000;
#endif

	gr_palette_clear();
	return gr_init_screen( t,w,h,0,0,r,vga_screen_addr);
}


short vga_init()
{
	// Only do this function once!
	if (vga_installed==1)
		return 1;

#if !defined(POLY_ACC)
	if (gr_init_A0000())
		return 10;
#endif

	vga_screen_addr = (ubyte *)0xa0000;		//address in memory of screen

	// Save the current text screen mode
	if (vga_save_mode()==1)
		return 1;

/*	if (get_selector( &vga_fade_table, 256*GR_FADE_LEVELS, &vga_fade_table_selector ))
		Error( "Error allocating fade table selector!" );

	if (get_selector( &vga_palette, 256*3, &vga_palette_selector ))
		Error( "Error allocating palette selector!" );

	if (get_selector( &vga_inverse_table, 32*32*32, &vga_inverse_table_selector ))
		Error( "Error allocating inverse table selector!" );
*/

	// Set flags indicating that this is installed.
//	gr_palette_clear();
	vga_installed = 1;
	atexit(vga_close);

#if defined(POLY_ACC)
    pa_init();
#endif
	return 0;
}

short vga_mode13_checkmode()
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

short vga_check_mode(short mode)
{
#if defined(POLY_ACC)
if(mode != SM_640x480x15xPA)
    Int3();
#endif

	switch(mode)
	{
	case 19:
	case SM_320x200C:
	case SM_320x200U:
	case SM_320x240U:
	case SM_360x200U:
	case SM_360x240U:
	case SM_376x282U:
	case SM_320x400U:
	case SM_320x480U:
	case SM_360x400U:
	case SM_360x480U:
	case SM_360x360U:
	case SM_376x308U:
	case SM_376x564U:		return vga_mode13_checkmode();
	case SM_640x400V:		return gr_vesa_checkmode( 0x100 ); 
	case SM_640x480V: 	return gr_vesa_checkmode( 0x101 ); 
	case SM_800x600V: 	return gr_vesa_checkmode( 0x103 ); 
	case SM_1024x768V:	return gr_vesa_checkmode( 0x105 ); 
	case SM_640x480V15:	return gr_vesa_checkmode( 0x110 ); 
	case SM_800x600V15:	return gr_vesa_checkmode( 0x113 ); 
#if defined(POLY_ACC)
	case SM_640x480x15xPA: return pa_detect(mode);
#endif
	}
	return 11;
}
