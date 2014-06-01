/*
 * Portions of this file are copyright Rebirth contributors and licensed as
 * described in COPYING.txt.
 * Portions of this file are copyright Parallax Software and licensed
 * according to the Parallax license below.
 * See COPYING.txt for license details.

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
 * Graphical routines for setting the palette
 *
 */

#include <algorithm>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "physfsx.h"
#include "pstypes.h"
#include "u_mem.h"
#include "gr.h"
#include "grdef.h"
#include "dxxerror.h"
#include "maths.h"
#include "palette.h"

#include "dxxsconf.h"
#include "compiler-range_for.h"

#define SQUARE(x) ((x)*(x))

#define	MAX_COMPUTED_COLORS	32

int	Num_computed_colors=0;

struct color_record {
	ubyte	r,g,b,color_num;
};

color_record Computed_colors[MAX_COMPUTED_COLORS];

palette_array_t gr_palette;
palette_array_t gr_current_pal;
ubyte gr_fade_table[256*34];

ubyte gr_palette_gamma = 0;
int gr_palette_gamma_param = 0;

void copy_bound_palette(palette_array_t &d, const palette_array_t &s)
{
	auto a = [](rgb_t c) {
		const ubyte bound = 63;
		c.r = std::min(c.r, bound);
		c.g = std::min(c.g, bound);
		c.b = std::min(c.b, bound);
		return c;
	};
	std::transform(s.begin(), s.end(), d.begin(), a);
}

void copy_diminish_palette(palette_array_t &palette, const ubyte *p)
{
	for (unsigned i = 0; i < palette.size(); i++) {
		palette[i].r = *p++ >> 2;
		palette[i].g = *p++ >> 2;
		palette[i].b = *p++ >> 2;
	}
}

static void diminish_entry(rgb_t &c)
{
	c.r >>= 2;
	c.g >>= 2;
	c.b >>= 2; 
}

void diminish_palette(palette_array_t &palette)
{
	range_for (rgb_t &c, palette)
		diminish_entry(c);
}

void gr_palette_set_gamma( int gamma )
{
	if ( gamma < 0 ) gamma = 0;
        if ( gamma > 16 ) gamma = 16;      //was 8

	if (gr_palette_gamma_param != gamma )	{
		gr_palette_gamma_param = gamma;
		gr_palette_gamma = gamma;
		gr_palette_load( gr_palette );
	}
}

int gr_palette_get_gamma()
{
	return gr_palette_gamma_param;
}


#if defined(DXX_BUILD_DESCENT_II)
void gr_copy_palette(palette_array_t &gr_palette, const palette_array_t &pal)
{
	gr_palette = pal;

	        Num_computed_colors = 0;
}
#endif

void gr_use_palette_table(const char * filename )
{
	PHYSFS_file *fp;
	int i,fsize;

	fp = PHYSFSX_openReadBuffered( filename );
#if defined(DXX_BUILD_DESCENT_I)
#define FAILURE_FORMAT	"Can't open palette file <%s>"
#elif defined(DXX_BUILD_DESCENT_II)
#define FAILURE_FORMAT	"Can open neither palette file <%s> nor default palette file <" DEFAULT_LEVEL_PALETTE ">"
	// the following is a hack to enable the loading of d2 levels
	// even if only the d2 mac shareware datafiles are present.
	// However, if the pig file is present but the palette file isn't,
	// the textures in the level will look wierd...
	if ( fp==NULL)
		fp = PHYSFSX_openReadBuffered( DEFAULT_LEVEL_PALETTE );
#endif
	if ( fp==NULL)
		Error(FAILURE_FORMAT,
		      filename);

	fsize	= PHYSFS_fileLength( fp );
	Assert( fsize == 9472 );
	(void)fsize;
	PHYSFS_read( fp, &gr_palette[0], sizeof(gr_palette[0]), gr_palette.size() );
	PHYSFS_read( fp, gr_fade_table, 256*34, 1 );
	PHYSFS_close(fp);

	// This is the TRANSPARENCY COLOR
	for (i=0; i<GR_FADE_LEVELS; i++ )	{
		gr_fade_table[i*256+255] = 255;
	}
#if defined(DXX_BUILD_DESCENT_II)
	Num_computed_colors = 0;	//	Flush palette cache.
// swap colors 0 and 255 of the palette along with fade table entries

#ifdef SWAP_0_255
	for (i = 0; i < 3; i++) {
		ubyte c;
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
#endif
}

//	Add a computed color (by gr_find_closest_color) to list of computed colors in Computed_colors.
//	If list wasn't full already, increment Num_computed_colors.
//	If was full, replace a random one.
static void add_computed_color(int r, int g, int b, int color_num)
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

	best_value = SQUARE(r-gr_palette[0].r)+SQUARE(g-gr_palette[0].g)+SQUARE(b-gr_palette[0].b);
	best_index = 0;
	if (best_value==0) {
		add_computed_color(r, g, b, best_index);
 		return best_index;
	}
	j=0;
	// only go to 255, 'cause we dont want to check the transparent color.
	for (i=1; i < 254; i++ )	{
		++j;
		value = SQUARE(r-gr_palette[j].r)+SQUARE(g-gr_palette[j].g)+SQUARE(b-gr_palette[j].b);
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

	best_value = SQUARE(r-gr_current_pal[0].r)+SQUARE(g-gr_current_pal[0].g)+SQUARE(b-gr_current_pal[0].b);
	best_index = 0;
	if (best_value==0)
 		return best_index;

	j=0;
	// only go to 255, 'cause we dont want to check the transparent color.
	for (i=1; i < 254; i++ )	{
		++j;
		value = SQUARE(r-gr_current_pal[j].r)+SQUARE(g-gr_current_pal[j].g)+SQUARE(b-gr_current_pal[j].b);
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

	for (i=0; i < 256; i++ )	{
		r1 = gr_palette[i].r + r;
		if ( r1 > 63 ) r1 = 63;
		g1 = gr_palette[i].g + g;
		if ( g1 > 63 ) g1 = 63;
		b1 = gr_palette[i].b + b;
		if ( b1 > 63 ) b1 = 63;
		table[i] = gr_find_closest_color( r1, g1, b1 );
	}
}

#if defined(DXX_BUILD_DESCENT_II)
void gr_make_blend_table(ubyte *blend_table, ubyte r, ubyte g, ubyte b)
{
	int i, j;
	float alpha;
	ubyte r1, g1, b1;

	for (j = 0; j < GR_FADE_LEVELS; j++)
	{
		alpha = 1.0 - (float)j / ((float)GR_FADE_LEVELS - 1);
		for (i = 0; i < 255; i++)
		{
			r1 = (ubyte)((1.0 - alpha) * (float)gr_palette[i].r + (alpha * (float)r));
			g1 = (ubyte)((1.0 - alpha) * (float)gr_palette[i].g + (alpha * (float)g));
			b1 = (ubyte)((1.0 - alpha) * (float)gr_palette[i].b + (alpha * (float)b));
			blend_table[i + j * 256] = gr_find_closest_color(r1, g1, b1);
		}
		blend_table[i + j * 256] = 255; // leave white alone
	}
}
#endif
