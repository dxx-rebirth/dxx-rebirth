/* $Id: compbit.h,v 1.2 2003-10-10 09:36:34 btb Exp $ */
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

/*
 *
 * Contains encryption key for bitmaps.tbl
 *
 * Old Log:
 * Revision 1.1  1995/05/16  15:55:16  allender
 * Initial revision
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

#endif /* _COMPBIT_H */
