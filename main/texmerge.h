/* $Id: texmerge.h,v 1.2 2003-10-10 09:36:35 btb Exp $ */
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
 * Definitions for texture merging caching stuff.
 *
 * Old Log:
 * Revision 1.1  1995/05/16  16:04:03  allender
 * Initial revision
 *
 * Revision 2.0  1995/02/27  11:28:05  john
 * New version 2.0, which has no anonymous unions, builds with
 * Watcom 10.0, and doesn't require parsing BITMAPS.TBL.
 *
 * Revision 1.4  1995/01/14  19:16:26  john
 * First version of new bitmap paging code.
 *
 * Revision 1.3  1994/01/21  15:23:26  john
 * made texmerge_close return void.
 *
 *
 * Revision 1.2  1994/01/21  15:16:01  john
 * Created new module texmerge, that merges textures together and
 * caches the results.
 *
 * Revision 1.1  1994/01/21  15:03:36  john
 * Initial revision
 *
 *
 */


#ifndef _TEXMERGE_H
#define _TEXMERGE_H

int texmerge_init(int num_cached_textures);
grs_bitmap *texmerge_get_cached_bitmap(int tmap_bottom, int tmap_top);
void texmerge_close();
void texmerge_flush();

#endif /* _TEXMERGE_H */
