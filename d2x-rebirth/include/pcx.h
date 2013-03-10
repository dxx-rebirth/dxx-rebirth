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
 */


#ifndef _PCX_H
#define _PCX_H

#define PCX_ERROR_NONE          0
#define PCX_ERROR_OPENING       1
#define PCX_ERROR_NO_HEADER     2
#define PCX_ERROR_WRONG_VERSION 3
#define PCX_ERROR_READING       4
#define PCX_ERROR_NO_PALETTE    5
#define PCX_ERROR_WRITING       6
#define PCX_ERROR_MEMORY        7

// Reads filename into bitmap bmp, and fills in palette.  If bmp->bm_data==NULL,
// then bmp->bm_data is allocated and the w,h are filled.
// If palette==NULL the palette isn't read in.  Returns error code.

extern int pcx_read_bitmap( char * filename, grs_bitmap * bmp, int bitmap_type, ubyte * palette );

// Writes the bitmap bmp to filename, using palette. Returns error code.

extern int pcx_write_bitmap( char * filename, grs_bitmap * bmp, ubyte * palette );

extern const char *pcx_errormsg(int error_number);

#endif
