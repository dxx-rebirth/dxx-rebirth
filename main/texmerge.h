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
 * $Source: /cvsroot/dxx-rebirth/d1x-rebirth/main/texmerge.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: zicodxx $
 * $Date: 2006/03/17 19:41:59 $
 * 
 * Definitions for texture merging caching stuff.
 * 
 * $Log: texmerge.h,v $
 * Revision 1.1.1.1  2006/03/17 19:41:59  zicodxx
 * initial import
 *
 * Revision 1.1.1.1  1999/06/14 22:13:12  donut
 * Import of d1x 1.37 source.
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
grs_bitmap * texmerge_get_cached_bitmap( int tmap_bottom, int tmap_top );
void texmerge_close();
void texmerge_flush();


#endif
 
