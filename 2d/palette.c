/* $Id: palette.c,v 1.10 2003-06-10 17:50:50 btb Exp $ */
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
 * Graphical routines for setting the palette
 *
 * Old Log:
 * Revision 1.41  1995/02/02  14:26:31  john
 * Made palette fades work better with gamma thingy..
 *
 * Revision 1.40  1994/12/08  19:03:46  john
 * Made functions use cfile.
 *
 * Revision 1.39  1994/12/01  11:23:27  john
 * Limited Gamma from 0-8.
 *
 * Revision 1.38  1994/11/28  01:31:08  mike
 * optimize color lookup function, caching recently used colors.
 *
 * Revision 1.37  1994/11/18  22:50:18  john
 * Changed shorts to ints in parameters.
 *
 * Revision 1.36  1994/11/15  17:54:59  john
 * Made text palette fade in when game over.
 *
 * Revision 1.35  1994/11/10  19:53:14  matt
 * Fixed error handling is gr_use_palette_table()
 *
 * Revision 1.34  1994/11/07  13:53:48  john
 * Added better gamma stufff.
 *
 * Revision 1.33  1994/11/07  13:37:56  john
 * Added gamma correction stuff.
 *
 * Revision 1.32  1994/11/05  13:20:14  john
 * Fixed bug with find_closest_color_current not working.
 *
 * Revision 1.31  1994/11/05  13:08:09  john
 * MAde it return 0 when palette already faded out.
 *
 * Revision 1.30  1994/11/05  13:05:34  john
 * Added back in code to allow keys during fade.
 *
 * Revision 1.29  1994/11/05  12:49:50  john
 * Fixed bug with previous comment..
 *
 * Revision 1.28  1994/11/05  12:48:46  john
 * Made palette only fade in / out when its supposed to.
 *
 * Revision 1.27  1994/11/05  12:46:43  john
 * Changed palette stuff a bit.
 *
 * Revision 1.26  1994/11/01  12:59:35  john
 * Reduced palette.256 size.
 *
 * Revision 1.25  1994/10/26  23:55:35  john
 * Took out roller; Took out inverse table.
 *
 * Revision 1.24  1994/10/04  22:03:05  matt
 * Fixed bug: palette wasn't fading all the way out or in
 *
 * Revision 1.23  1994/09/22  16:08:40  john
 * Fixed some palette stuff.
 *
 * Revision 1.22  1994/09/19  11:44:31  john
 * Changed call to allocate selector to the dpmi module.
 *
 * Revision 1.21  1994/09/12  19:28:09  john
 * Fixed bug with unclipped fonts clipping.
 *
 * Revision 1.20  1994/09/12  18:18:39  john
 * Set 254 and 255 to fade to themselves in fadetable
 *
 * Revision 1.19  1994/09/12  14:40:10  john
 * Neatend.
 *
 * Revision 1.18  1994/09/09  09:31:55  john
 * Made find_closest_color not look at superx spot of 254
 *
 * Revision 1.17  1994/08/09  11:27:08  john
 * Add cthru stuff.
 *
 * Revision 1.16  1994/08/01  11:03:51  john
 * MAde it read in old/new palette.256
 *
 * Revision 1.15  1994/07/27  18:30:27  john
 * Took away the blending table.
 *
 * Revision 1.14  1994/06/09  10:39:52  john
 * In fade out.in functions, returned 1 if key was pressed...
 *
 * Revision 1.13  1994/05/31  19:04:16  john
 * Added key to stop fade if desired.
 *
 * Revision 1.12  1994/05/06  12:50:20  john
 * Added supertransparency; neatend things up; took out warnings.
 *
 * Revision 1.11  1994/05/03  19:39:02  john
 * *** empty log message ***
 *
 * Revision 1.10  1994/04/22  11:16:07  john
 * *** empty log message ***
 *
 * Revision 1.9  1994/04/08  16:59:40  john
 * Add fading poly's; Made palette fade 32 instead of 16.
 *
 * Revision 1.8  1994/03/16  17:21:17  john
 * Added slow palette searching options.
 *
 * Revision 1.7  1994/01/07  11:47:33  john
 * made use cflib
 *
 * Revision 1.6  1993/12/21  11:41:04  john
 * *** empty log message ***
 *
 * Revision 1.5  1993/12/09  15:02:47  john
 * Changed palette stuff majorly
 *
 * Revision 1.4  1993/12/07  12:31:41  john
 * moved bmd_palette to gr_palette
 *
 * Revision 1.3  1993/10/15  16:22:23  john
 * *** empty log message ***
 *
 * Revision 1.2  1993/09/26  18:59:46  john
 * fade stuff
 *
 * Revision 1.1  1993/09/08  11:44:03  john
 * Initial revision
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "pstypes.h"
#include "u_mem.h"
#include "gr.h"
#include "grdef.h"
#include "cfile.h"
#include "error.h"
#include "mono.h"
#include "fix.h"
//added/remove by dph on 1/9/99
//#include "key.h"
//end remove

#include "palette.h"

extern int gr_installed;

#define SQUARE(x) ((x)*(x))

#define	MAX_COMPUTED_COLORS	32

int	Num_computed_colors=0;

typedef struct {
	ubyte	r,g,b,color_num;
} color_record;

color_record Computed_colors[MAX_COMPUTED_COLORS];

ubyte gr_palette[256*3];
ubyte gr_current_pal[256*3];
ubyte gr_fade_table[256*34];

ubyte gr_palette_gamma = 0;
int gr_palette_gamma_param = 0;
ubyte gr_palette_faded_out = 1;

int grd_fades_disabled=0;   // Used to skip fading for development

void gr_palette_set_gamma( int gamma )
{
	if ( gamma < 0 ) gamma = 0;
//added/changed on 10/27/98 by Victor Rachels to increase brightness slider
        if ( gamma > 16 ) gamma = 16;      //was 8
//end this section change - Victor Rachels

	if (gr_palette_gamma_param != gamma )	{
		gr_palette_gamma_param = gamma;
		gr_palette_gamma = gamma;
		if (!gr_palette_faded_out)
			gr_palette_load( gr_palette );
	}
}

int gr_palette_get_gamma()
{
	return gr_palette_gamma_param;
}


void gr_copy_palette(ubyte *gr_palette, ubyte *pal, int size)
{
	        memcpy(gr_palette, pal, size);

	        Num_computed_colors = 0;
}


void gr_use_palette_table( char * filename )
{
	CFILE *fp;
	int i,fsize;
#ifdef SWAP_0_255
	ubyte c;
#endif

	fp = cfopen( filename, "rb" );

	// the following is a hack to enable the loading of d2 levels
	// even if only the d2 mac shareware datafiles are present.
	// However, if the pig file is present but the palette file isn't,
	// the textures in the level will look wierd...
	if ( fp==NULL)
		fp = cfopen( DEFAULT_LEVEL_PALETTE, "rb" );
	if ( fp==NULL)
		Error("Can open neither palette file <%s> "
		      "nor default palette file <"
		      DEFAULT_LEVEL_PALETTE
		      ">.\n",
		      filename);

	fsize	= cfilelength( fp );
	Assert( fsize == 9472 );
	cfread( gr_palette, 256*3, 1, fp );
	cfread( gr_fade_table, 256*34, 1, fp );
	cfclose(fp);

	// This is the TRANSPARENCY COLOR
	for (i=0; i<GR_FADE_LEVELS; i++ )	{
		gr_fade_table[i*256+255] = 255;
	}

	Num_computed_colors = 0;	//	Flush palette cache.
#if defined(POLY_ACC)
	pa_update_clut(gr_palette, 0, 256, 0);
#endif

// swap colors 0 and 255 of the palette along with fade table entries

#ifdef SWAP_0_255
	for (i = 0; i < 3; i++) {
		c = gr_palette[i];
		gr_palette[i] = gr_palette[765+i];
		gr_palette[765+i] = c;
	}

	for (i = 0; i < GR_FADE_LEVELS * 256; i++) {
		if (gr_fade_table[i] == 0)
			gr_fade_table[i] = 255;
	}
	for (i=0; i<GR_FADE_LEVELS; i++)
		gr_fade_table[i*256] = TRANSPARENCY_COLOR;
#endif
}

//	Add a computed color (by gr_find_closest_color) to list of computed colors in Computed_colors.
//	If list wasn't full already, increment Num_computed_colors.
//	If was full, replace a random one.
void add_computed_color(int r, int g, int b, int color_num)
{
	int	add_index;

	if (Num_computed_colors < MAX_COMPUTED_COLORS) {
		add_index = Num_computed_colors;
		Num_computed_colors++;
	} else
		add_index = (d_rand() * MAX_COMPUTED_COLORS) >> 15;

	Computed_colors[add_index].r = r;
	Computed_colors[add_index].g = g;
	Computed_colors[add_index].b = b;
	Computed_colors[add_index].color_num = color_num;
}

void init_computed_colors(void)
{
	int	i;

	for (i=0; i<MAX_COMPUTED_COLORS; i++)
		Computed_colors[i].r = 255;		//	Make impossible to match.
}

int gr_find_closest_color( int r, int g, int b )
{
	int i, j;
	int best_value, best_index, value;

	if (Num_computed_colors == 0)
		init_computed_colors();

	//	If we've already computed this color, return it!
	for (i=0; i<Num_computed_colors; i++)
		if (r == Computed_colors[i].r)
			if (g == Computed_colors[i].g)
				if (b == Computed_colors[i].b) {
					if (i > 4) {
						color_record	trec;
						trec = Computed_colors[i-1];
						Computed_colors[i-1] = Computed_colors[i];
						Computed_colors[i] = trec;
						return Computed_colors[i-1].color_num;
					}
					return Computed_colors[i].color_num;
				}

//	r &= 63;
//	g &= 63;
//	b &= 63;

	best_value = SQUARE(r-gr_palette[0])+SQUARE(g-gr_palette[1])+SQUARE(b-gr_palette[2]);
	best_index = 0;
	if (best_value==0) {
		add_computed_color(r, g, b, best_index);
 		return best_index;
	}
	j=0;
	// only go to 255, 'cause we dont want to check the transparent color.
	for (i=1; i<254; i++ )	{
		j += 3;
		value = SQUARE(r-gr_palette[j])+SQUARE(g-gr_palette[j+1])+SQUARE(b-gr_palette[j+2]);
		if ( value < best_value )	{
			if (value==0) {
				add_computed_color(r, g, b, i);
				return i;
			}
			best_value = value;
			best_index = i;
		}
	}
	add_computed_color(r, g, b, best_index);
	return best_index;
}

int gr_find_closest_color_15bpp( int rgb )
{
	return gr_find_closest_color( ((rgb>>10)&31)*2, ((rgb>>5)&31)*2, (rgb&31)*2 );
}


int gr_find_closest_color_current( int r, int g, int b )
{
	int i, j;
	int best_value, best_index, value;

//	r &= 63;
//	g &= 63;
//	b &= 63;

	best_value = SQUARE(r-gr_current_pal[0])+SQUARE(g-gr_current_pal[1])+SQUARE(b-gr_current_pal[2]);
	best_index = 0;
	if (best_value==0)
 		return best_index;

	j=0;
	// only go to 255, 'cause we dont want to check the transparent color.
	for (i=1; i<254; i++ )	{
		j += 3;
		value = SQUARE(r-gr_current_pal[j])+SQUARE(g-gr_current_pal[j+1])+SQUARE(b-gr_current_pal[j+2]);
		if ( value < best_value )	{
			if (value==0)
				return i;
			best_value = value;
			best_index = i;
		}
	}
	return best_index;
}

void gr_make_cthru_table(ubyte * table, ubyte r, ubyte g, ubyte b )
{
	int i;
	ubyte r1, g1, b1;

	for (i=0; i<256; i++ )	{
		r1 = gr_palette[i*3+0] + r;
		if ( r1 > 63 ) r1 = 63;
		g1 = gr_palette[i*3+1] + g;
		if ( g1 > 63 ) g1 = 63;
		b1 = gr_palette[i*3+2] + b;
		if ( b1 > 63 ) b1 = 63;
		table[i] = gr_find_closest_color( r1, g1, b1 );
	}
}

