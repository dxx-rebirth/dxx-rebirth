/* $Id: textures.h,v 1.2 2003-10-10 09:36:35 btb Exp $ */
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
 * Array of textures, and other stuff
 *
 * Old Log:
 * Revision 1.1  1995/05/16  16:04:20  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:31:54  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.3  1994/03/15  16:32:56  yuan
 * Cleaned up bm-loading code.
 * (Fixed structures too)
 *
 * Revision 1.2  1993/12/05  22:49:53  matt
 * Reworked include files in an attempt to cut down on build times
 *
 * Revision 1.1  1993/12/05  20:16:26  matt
 * Initial revision
 *
 *
 */


#ifndef _TEXTURES_H
#define _TEXTURES_H

#include "bm.h"
#include "piggy.h"

// Texture stuff... in mglobal.c

extern int NumTextures;
extern bitmap_index Textures[MAX_TEXTURES]; // Array of all texture tmaps.

#endif /* _TEXTURES_H */
