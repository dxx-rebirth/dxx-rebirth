/* $Id: palette.h,v 1.6 2005-08-02 06:11:10 chris Exp $ */
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
 * Protoypes for palette functions
 *
 */


#ifndef _PALETTE_H
#define _PALETTE_H

#define DEFAULT_LEVEL_PALETTE "groupa.256" //don't confuse with DEFAULT_PALETTE

extern void gr_palette_set_gamma( int gamma );
extern int gr_palette_get_gamma();
extern ubyte gr_palette_faded_out;
extern int grd_fades_disabled;
extern void gr_palette_clear();
extern int gr_palette_fade_out(ubyte *pal, int nsteps, int allow_keys );
extern int gr_palette_fade_in(ubyte *pal,int nsteps, int allow_keys );
extern void gr_palette_load( ubyte * pal );
extern void gr_make_cthru_table(ubyte * table, ubyte r, ubyte g, ubyte b );
extern void gr_make_blend_table(ubyte *blend_table, ubyte r, ubyte g, ubyte b);
extern int gr_find_closest_color_current( int r, int g, int b );
extern void gr_palette_read(ubyte * palette);
extern void init_computed_colors(void);

extern ubyte gr_palette_gamma;
extern ubyte gr_current_pal[256*3];

#endif
