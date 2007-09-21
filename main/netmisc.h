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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/netmisc.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:42:53 $
 * 
 * .
 * 
 * $Log: netmisc.h,v $
 * Revision 1.1.1.1  2006/03/17 19:42:53  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:12:42  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:30:18  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.2  1994/08/09  19:31:54  john
 * Networking changes.
 * 
 * Revision 1.1  1994/08/08  11:18:40  john
 * Initial revision
 * 
 * 
 */



#ifndef _NETMISC_H
#define _NETMISC_H

//Returns a checksum of a block of memory.
extern ushort netmisc_calc_checksum( void * vptr, int len );

//Finds the difference between block1 and block2.  Fills in diff_buffer and returns the size of diff_buffer.
extern int netmisc_find_diff( void *block1, void *block2, int block_size, void *diff_buffer );

//Applies diff_buffer to block1 to create a new block1.  Returns the final size of block1.
extern int netmisc_apply_diff(void *block1, void *diff_buffer, int diff_size );

#endif
 
