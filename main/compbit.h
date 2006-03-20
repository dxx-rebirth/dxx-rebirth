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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/compbit.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:42:15 $
 * 
 * Contains encryption key for bitmaps.tbl
 * 
 * $Log: compbit.h,v $
 * Revision 1.1.1.1  2006/03/17 19:42:15  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:12:14  donut
 * Import of d1x 1.37 source.
 *
 * Revision 2.0  1995/02/27  11:28:46  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 * 
 * Revision 1.3  1994/12/05  15:10:28  allender
 * changed extern prototype definition
 * 
 * Revision 1.2  1994/10/19  15:43:33  allender
 * header file which contains the xor value which is used when encrypting
 * bitmaps.tbl
 * 
 * Revision 1.1  1994/10/19  13:22:19  allender
 * Initial revision
 * 
 * 
 */



#ifndef _COMPBIT_H
#define _COMPBIT_H

#define BITMAP_TBL_XOR 0xD3

extern void encode_rotate_left(char *);

#endif
 
