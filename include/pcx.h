/* $Id: pcx.h,v 1.2 2002-08-06 04:49:43 btb Exp $ */
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
 *
 * Routines to read/write pcx images.
 *
 * Old Log:
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

#include "cfile.h"

#define PCX_ERROR_NONE          0
#define PCX_ERROR_OPENING       1
#define PCX_ERROR_NO_HEADER     2
#define PCX_ERROR_WRONG_VERSION 3
#define PCX_ERROR_READING       4
#define PCX_ERROR_NO_PALETTE    5
#define PCX_ERROR_WRITING       6
#define PCX_ERROR_MEMORY        7

/* PCX Header data type */
typedef struct	{
	ubyte		Manufacturer;
	ubyte		Version;
	ubyte		Encoding;
	ubyte		BitsPerPixel;
	short		Xmin;
	short		Ymin;
	short		Xmax;
	short		Ymax;
	short		Hdpi;
	short		Vdpi;
	ubyte		ColorMap[16][3];
	ubyte		Reserved;
	ubyte		Nplanes;
	short		BytesPerLine;
	ubyte		filler[60];
} PCXHeader;

// Reads filename into bitmap bmp, and fills in palette.  If bmp->bm_data==NULL,
// then bmp->bm_data is allocated and the w,h are filled.
// If palette==NULL the palette isn't read in.  Returns error code.

extern int pcx_read_bitmap( char * filename, grs_bitmap * bmp, int bitmap_type, ubyte * palette );

// Writes the bitmap bmp to filename, using palette. Returns error code.

extern int pcx_write_bitmap( char * filename, grs_bitmap * bmp, ubyte * palette );

extern char *pcx_errormsg(int error_number);

int pcx_read_fullscr(char * filename, ubyte * palette);

/*
 * reads a PCXHeader structure from a CFILE
 */
void PCXHeader_read(PCXHeader *ph, CFILE *fp);

#endif
