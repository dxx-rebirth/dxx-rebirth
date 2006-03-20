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



#ifndef _VGA_H
#define _VGA_H

#include "pstypes.h"

typedef unsigned short uword;

#define SM_ORIGINAL		-1
#define SM_320x200C     0
#define SM_320x200U     1
#define SM_320x240U     2
#define SM_360x200U     3
#define SM_360x240U     4
#define SM_376x282U     5
#define SM_320x400U     6
#define SM_320x480U     7
#define SM_360x400U     8
#define SM_360x480U     9
#define SM_360x360U     10
#define SM_376x308U     11
#define SM_376x564U     12
#define SM_640x400V     13
#define SM_640x480V     14
#define SM_800x600V     15
#define SM_1024x768V    16
#define SM_640x480V15   17
#define SM_800x600V15   18
#define SM_1280x1024V	19
#define SM_320x400_3DMAX	22		//special mode in 3dMax bios

void vga_set_cellheight( ubyte height );
void vga_set_linear(void);
void vga_16_to_256(void);
void vga_turn_screen_off(void);
void vga_turn_screen_on(void);
void vga_set_misc_mode( uword mode );
void vga_set_text_25(void);
void vga_set_text_43(void);
void vga_set_text_50(void);
ubyte is_graphics_mode(void);
int vga_save_mode(void);
int isvga(void);
void vga_set_cursor_type( uword ctype );
void vga_enable_default_palette_loading(void);
void vga_disable_default_palette_loading(void);
void vga_set_cursor_position( uword position );
void vga_restore_mode(void);
short vga_close(void);
int vga_vesa_setmode( short mode );
short vga_set_mode(short mode);
short vga_init(void);
short vga_mode13_checkmode(void);

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

short vga_check_mode(short mode);

//the current mode the adapter is in, one of the SM_ values above
extern int VGA_current_mode;

#endif
