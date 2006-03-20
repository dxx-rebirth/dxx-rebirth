// tmerge.c - C Texture merge routines for use with D1X
// Ripped from ldescent by <dph-man@iname.com>

#ifdef NO_ASM // If for some reason we have elected not to use assembler...

#include "gr.h"

void gr_merge_textures( ubyte * lower, ubyte * upper, ubyte * dest )
{
 int x,y;
 ubyte c;
      for (y=0;y<64;y++) for (x=0;x<64;x++) {
		c=upper[64*y+x];
		if (c==TRANSPARENCY_COLOR)
			c=lower[64*y+x];
		*dest++=c;
      }
}

void gr_merge_textures_1( ubyte * lower, ubyte * upper, ubyte * dest )
{
 int x,y;
 ubyte c;
	for (y=0; y<64; y++ )
		for (x=0; x<64; x++ )	{
			c = upper[ 64*x+(63-y) ];
			if (c==TRANSPARENCY_COLOR)
				c = lower[ 64*y+x ];
			*dest++ = c;
                }
}

void gr_merge_textures_2( ubyte * lower, ubyte * upper, ubyte * dest )
{
 int x,y;
 ubyte c;
	for (y=0; y<64; y++ )
		for (x=0; x<64; x++ )	{
			c = upper[ 64*(63-y)+(63-x) ];
			if (c==TRANSPARENCY_COLOR)
				c = lower[ 64*y+x ];
			*dest++ = c;
		}
}

void gr_merge_textures_3( ubyte * lower, ubyte * upper, ubyte * dest )
{
 int x,y;
 ubyte c;
	for (y=0; y<64; y++ )
		for (x=0; x<64; x++ )	{
			c = upper[ 64*(63-x)+y  ];
			if (c==TRANSPARENCY_COLOR)
				c = lower[ 64*y+x ];
			*dest++ = c;
		}
}


#endif
