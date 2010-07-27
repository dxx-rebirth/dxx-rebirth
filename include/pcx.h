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
COPYRIGHT 1993-1998 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/
/*
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/include/pcx.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:46:22 $
 * 
 * Routines to read/write pcx images.
 * 
 * $Log: pcx.h,v $
 * Revision 1.1.1.1  2006/03/17 19:46:22  zicodxx
 * initial import
 *
 * Revision 1.2  1999/11/20 10:05:16  donut
 * variable size menu patch from Jan Bobrowski.  Variable menu font size support and a bunch of fixes for menus that didn't work quite right, by me (MPM).
 *
 * Revision 1.1.1.1  1999/06/14 22:02:19  donut
 * Import of d1x 1.37 source.
 *
 * Revision 1.4  1995/01/21  17:07:34  john
 * Added out of memory error.
 * 
 * Revision 1.3  1994/11/29  02:53:10  john
 * Added error messages; made call be more similiar to iff.
 * 
 * Revision 1.2  1994/11/28  20:03:48  john
 * Added PCX functions.
 * 
 * Revision 1.1  1994/11/28  19:57:45  john
 * Initial revision
 * 
 * 
 */



#ifndef _PCX_H
#define _PCX_H

#define PCX_ERROR_NONE 				0
#define PCX_ERROR_OPENING			1
#define PCX_ERROR_NO_HEADER		2
#define PCX_ERROR_WRONG_VERSION	3
#define PCX_ERROR_READING			4
#define PCX_ERROR_NO_PALETTE		5
#define PCX_ERROR_WRITING			6
#define PCX_ERROR_MEMORY			7

// Load bitmap for little-known 'baldguy' cheat.

extern int bald_guy_load( char * filename, grs_bitmap * bmp,int bitmap_type ,ubyte * palette );

// Reads filename into bitmap bmp, and fills in palette.  If bmp->bm_data==NULL, 
// then bmp->bm_data is allocated and the w,h are filled.  
// If palette==NULL the palette isn't read in.  Returns error code.

extern int pcx_read_bitmap( char * filename, grs_bitmap * bmp, int bitmap_type, ubyte * palette );

// Writes the bitmap bmp to filename, using palette. Returns error code.

extern int pcx_write_bitmap( char * filename, grs_bitmap * bmp, ubyte * palette );

extern char *pcx_errormsg(int error_number);

#endif
 
