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
 * $Source: /cvs/cvsroot/d2x/include/palette.h,v $
 * $Revision: 1.1.1.1 $
 * $Author: bradleyb $
 * $Date: 2001-01-19 03:30:16 $
 * 
 * Protoypes for palette functions
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.1.1.1  1999/06/14 22:02:19  donut
 * Import of d1x 1.37 source.
 *
 * Revision 1.11  1994/11/15  17:55:10  john
 * Made text palette fade in when game over.
 * 
 * Revision 1.10  1994/11/07  13:53:42  john
 * Added better gamma stufff.
 * 
 * Revision 1.9  1994/11/07  13:38:03  john
 * Added gamma correction stuff.
 * 
 * Revision 1.8  1994/11/05  13:05:59  john
 * Added back in code to allow keys during fade.
 * 
 * Revision 1.7  1994/11/05  12:46:37  john
 * Changed palette stuff a bit.
 * 
 * Revision 1.6  1994/09/22  16:08:37  john
 * Fixed some palette stuff.
 * 
 * Revision 1.5  1994/08/09  11:27:04  john
 * Add cthru stuff.
 * 
 * Revision 1.4  1994/06/09  10:39:33  john
 * In fade out.in functions, returned 1 if key was pressed...
 * 
 * Revision 1.3  1994/05/31  19:04:24  john
 * Added key to stop fade if desired.
 * 
 * Revision 1.2  1994/05/06  12:50:42  john
 * Added supertransparency; neatend things up; took out warnings.
 * 
 * Revision 1.1  1994/05/04  14:59:57  john
 * Initial revision
 * 
 * 
 */



#ifndef _PALETTE_H
#define _PALETTE_H

extern void gr_palette_set_gamma( int gamma );
extern int gr_palette_get_gamma();
extern ubyte gr_palette_faded_out;
extern void gr_palette_clear();
extern int gr_palette_fade_out(ubyte *pal, int nsteps, int allow_keys );
extern int gr_palette_fade_in(ubyte *pal,int nsteps, int allow_keys );
extern void gr_palette_load( ubyte * pal );
extern void gr_make_cthru_table(ubyte * table, ubyte r, ubyte g, ubyte b );
extern int gr_find_closest_color_current( int r, int g, int b );
extern void gr_palette_read(ubyte * palette);
extern void init_computed_colors(void);



extern ubyte gr_palette_gamma;
extern ubyte gr_current_pal[256*3];

#ifdef D1XD3D
typedef ubyte PALETTE [256 * 3];
extern void DoSetPalette (PALETTE *pPal);
#endif

#endif
