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
 * Routines for bitblt's.
 *
 */

#include <string.h>
#include "u_mem.h"
#include "gr.h"
#include "grdef.h"
#include "rle.h"
#include "dxxerror.h"
#include "byteswap.h"
#ifdef OGL
#include "ogl_init.h"
#endif

static int gr_bitblt_dest_step_shift = 0;
static int gr_bitblt_double = 0;
static ubyte *gr_bitblt_fade_table=NULL;

static void gr_bm_ubitblt00_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest);
static void gr_bm_ubitblt00m_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest);
static void gr_bm_ubitblt0x_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest);

static void gr_linear_movsd( ubyte * source, ubyte * dest, unsigned int nbytes) {
	memcpy(dest,source,nbytes);
}

static void gr_linear_rep_movsdm(ubyte *src, ubyte *dest, int num_pixels) {
	register ubyte c;
	while (num_pixels--)
		if ((c=*src++)!=255)
			*dest++=c;
		else	dest++;
}

static void gr_linear_rep_movsdm_faded(ubyte * src, ubyte * dest, int num_pixels, ubyte fade_value ) {
	register ubyte c;
	while (num_pixels--)
		if ((c=*src++)!=255)
			*dest++=gr_fade_table[((int)fade_value<<8)|(int)c];
		else	dest++;
}

static void gr_linear_rep_movsd_2x(ubyte * source, ubyte * dest, uint nbytes ) {
	register ubyte c;
	while (nbytes--) {
		if (nbytes&1)
			*dest++=*source++;
		else {
		        unsigned short *sp=(unsigned short *)dest;
			c=*source++;
			*sp=((short)c<<8)|(short)c;
			dest+=2;
		}
	}
}

static void gr_ubitmap00( int x, int y, grs_bitmap *bm )
{
	register int y1;
	int dest_rowsize;

	unsigned char * dest;
	unsigned char * src;

	dest_rowsize=grd_curcanv->cv_bitmap.bm_rowsize << gr_bitblt_dest_step_shift;
	dest = &(grd_curcanv->cv_bitmap.bm_data[ dest_rowsize*y+x ]);

	src = bm->bm_data;

	for (y1=0; y1 < bm->bm_h; y1++ )    {
		if (gr_bitblt_double)
			gr_linear_rep_movsd_2x( src, dest, bm->bm_w );
		else
			gr_linear_movsd( src, dest, bm->bm_w );
		src += bm->bm_rowsize;
		dest+= (int)(dest_rowsize);
	}
}

static void gr_ubitmap00m( int x, int y, grs_bitmap *bm )
{
	register int y1;
	int dest_rowsize;

	unsigned char * dest;
	unsigned char * src;

	dest_rowsize=grd_curcanv->cv_bitmap.bm_rowsize << gr_bitblt_dest_step_shift;
	dest = &(grd_curcanv->cv_bitmap.bm_data[ dest_rowsize*y+x ]);

	src = bm->bm_data;

	if (gr_bitblt_fade_table==NULL)	{
		for (y1=0; y1 < bm->bm_h; y1++ )    {
			gr_linear_rep_movsdm( src, dest, bm->bm_w );
			src += bm->bm_rowsize;
			dest+= (int)(dest_rowsize);
		}
	} else {
		for (y1=0; y1 < bm->bm_h; y1++ )    {
			gr_linear_rep_movsdm_faded( src, dest, bm->bm_w, gr_bitblt_fade_table[y1+y] );
			src += bm->bm_rowsize;
			dest+= (int)(dest_rowsize);
		}
	}
}

static void gr_ubitmap012( int x, int y, grs_bitmap *bm )
{
	register int x1, y1;
	unsigned char * src;

	src = bm->bm_data;

	for (y1=y; y1 < (y+bm->bm_h); y1++ )    {
		for (x1=x; x1 < (x+bm->bm_w); x1++ )    {
			gr_setcolor( *src++ );
			gr_upixel( x1, y1 );
		}
	}
}

static void gr_ubitmap012m( int x, int y, grs_bitmap *bm )
{
	register int x1, y1;
	unsigned char * src;

	src = bm->bm_data;

	for (y1=y; y1 < (y+bm->bm_h); y1++ )    {
		for (x1=x; x1 < (x+bm->bm_w); x1++ )    {
			if ( *src != 255 )	{
				gr_setcolor( *src );
				gr_upixel( x1, y1 );
			}
			src++;
		}
	}
}

static void gr_ubitmapGENERIC(int x, int y, grs_bitmap * bm)
{
	register int x1, y1;

	for (y1=0; y1 < bm->bm_h; y1++ )    {
		for (x1=0; x1 < bm->bm_w; x1++ )    {
			gr_setcolor( gr_gpixel(bm,x1,y1) );
			gr_upixel( x+x1, y+y1 );
		}
	}
}

static void gr_ubitmapGENERICm(int x, int y, grs_bitmap * bm)
{
	register int x1, y1;
	ubyte c;

	for (y1=0; y1 < bm->bm_h; y1++ )    {
		for (x1=0; x1 < bm->bm_w; x1++ )    {
			c = gr_gpixel(bm,x1,y1);
			if ( c != 255 )	{
				gr_setcolor( c );
				gr_upixel( x+x1, y+y1 );
			}
		}
	}
}

void gr_ubitmap( int x, int y, grs_bitmap *bm )
{   int source, dest;

	source = bm->bm_type;
	dest = TYPE;

	if (source==BM_LINEAR) {
		switch( dest )
		{
		case BM_LINEAR:
			if ( bm->bm_flags & BM_FLAG_RLE )
				gr_bm_ubitblt00_rle(bm->bm_w, bm->bm_h, x, y, 0, 0, bm, &grd_curcanv->cv_bitmap );
			else
				gr_ubitmap00( x, y, bm );
			return;
#ifdef OGL
		case BM_OGL:
			ogl_ubitmapm_cs(x,y,-1,-1,bm,-1,F1_0);
			return;
#endif
		default:
			gr_ubitmap012( x, y, bm );
			return;
		}
	} else  {
		gr_ubitmapGENERIC(x, y, bm);
	}
}

void gr_ubitmapm( int x, int y, grs_bitmap *bm )
{   int source, dest;


	source = bm->bm_type;
	dest = TYPE;

	if (source==BM_LINEAR) {
		switch( dest )
		{
		case BM_LINEAR:
			if ( bm->bm_flags & BM_FLAG_RLE )
				gr_bm_ubitblt00m_rle(bm->bm_w, bm->bm_h, x, y, 0, 0, bm, &grd_curcanv->cv_bitmap );
			else
				gr_ubitmap00m( x, y, bm );
			return;
#ifdef OGL
		case BM_OGL:
			ogl_ubitmapm_cs(x,y,-1,-1,bm,-1,F1_0);
			return;
#endif
		default:
			gr_ubitmap012m( x, y, bm );
			return;
		}
	} else  {
		gr_ubitmapGENERICm(x, y, bm);
	}
}

// From Linear to Linear
static void gr_bm_ubitblt00(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	unsigned char * dbits;
	unsigned char * sbits;
	//int	src_bm_rowsize_2, dest_bm_rowsize_2;
	int dstep;

	int i;

	sbits =   src->bm_data	+ (src->bm_rowsize * sy) + sx;
	dbits =   dest->bm_data + (dest->bm_rowsize * dy) + dx;

	dstep = dest->bm_rowsize << gr_bitblt_dest_step_shift;

	// No interlacing, copy the whole buffer.
	if (gr_bitblt_double)
	    for (i=0; i < h; i++ )    {
		gr_linear_rep_movsd_2x( sbits, dbits, w );
		sbits += src->bm_rowsize;
		dbits += dstep;
	    }
	else
	    for (i=0; i < h; i++ )    {
		gr_linear_movsd( sbits, dbits, w );
		//memcpy(dbits, sbits, w);
		sbits += src->bm_rowsize;
		dbits += dstep;
	    }
}

// From Linear to Linear Masked
static void gr_bm_ubitblt00m(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	unsigned char * dbits;
	unsigned char * sbits;
	//int	src_bm_rowsize_2, dest_bm_rowsize_2;

	int i;

	sbits =   src->bm_data  + (src->bm_rowsize * sy) + sx;
	dbits =   dest->bm_data + (dest->bm_rowsize * dy) + dx;

	// No interlacing, copy the whole buffer.

	if (gr_bitblt_fade_table==NULL)	{
		for (i=0; i < h; i++ )    {
			gr_linear_rep_movsdm( sbits, dbits, w );
			sbits += src->bm_rowsize;
			dbits += dest->bm_rowsize;
		}
	} else {
		for (i=0; i < h; i++ )    {
			gr_linear_rep_movsdm_faded( sbits, dbits, w, gr_bitblt_fade_table[dy+i] );
			sbits += src->bm_rowsize;
			dbits += dest->bm_rowsize;
		}
	}
}

void gr_bm_bitblt(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	int dx1=dx, dx2=dx+dest->bm_w-1;
	int dy1=dy, dy2=dy+dest->bm_h-1;

	int sx1=sx, sx2=sx+src->bm_w-1;
	int sy1=sy, sy2=sy+src->bm_h-1;

	if ((dx1 >= dest->bm_w ) || (dx2 < 0)) return;
	if ((dy1 >= dest->bm_h ) || (dy2 < 0)) return;
	if ( dx1 < 0 ) { sx1 += -dx1; dx1 = 0; }
	if ( dy1 < 0 ) { sy1 += -dy1; dy1 = 0; }
	if ( dx2 >= dest->bm_w )	{ dx2 = dest->bm_w-1; }
	if ( dy2 >= dest->bm_h )	{ dy2 = dest->bm_h-1; }

	if ((sx1 >= src->bm_w ) || (sx2 < 0)) return;
	if ((sy1 >= src->bm_h ) || (sy2 < 0)) return;
	if ( sx1 < 0 ) { dx1 += -sx1; sx1 = 0; }
	if ( sy1 < 0 ) { dy1 += -sy1; sy1 = 0; }
	if ( sx2 >= src->bm_w )	{ sx2 = src->bm_w-1; }
	if ( sy2 >= src->bm_h )	{ sy2 = src->bm_h-1; }

	// Draw bitmap bm[x,y] into (dx1,dy1)-(dx2,dy2)
	if ( dx2-dx1+1 < w )
		w = dx2-dx1+1;
	if ( dy2-dy1+1 < h )
		h = dy2-dy1+1;
	if ( sx2-sx1+1 < w )
		w = sx2-sx1+1;
	if ( sy2-sy1+1 < h )
		h = sy2-sy1+1;

	gr_bm_ubitblt(w,h, dx1, dy1, sx1, sy1, src, dest );
}

void gr_bm_ubitblt(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	register int x1, y1;

	if ( (src->bm_type == BM_LINEAR) && (dest->bm_type == BM_LINEAR ))
	{
		if ( src->bm_flags & BM_FLAG_RLE )
			gr_bm_ubitblt00_rle( w, h, dx, dy, sx, sy, src, dest );
		else
			gr_bm_ubitblt00( w, h, dx, dy, sx, sy, src, dest );
		return;
	}

#ifdef OGL
	if ( (src->bm_type == BM_LINEAR) && (dest->bm_type == BM_OGL ))
	{
		ogl_ubitblt(w, h, dx, dy, sx, sy, src, dest);
		return;
	}
	if ( (src->bm_type == BM_OGL) && (dest->bm_type == BM_LINEAR ))
	{
		return;
	}
	if ( (src->bm_type == BM_OGL) && (dest->bm_type == BM_OGL ))
	{
		return;
	}
#endif

	if ( (src->bm_flags & BM_FLAG_RLE ) && (src->bm_type == BM_LINEAR) )	{
		gr_bm_ubitblt0x_rle(w, h, dx, dy, sx, sy, src, dest);
	 	return;
	}

	for (y1=0; y1 < h; y1++ )    {
		for (x1=0; x1 < w; x1++ )    {
			gr_bm_pixel( dest, dx+x1, dy+y1, gr_gpixel(src,sx+x1,sy+y1) );
		}
	}
}

// Clipped bitmap ...
void gr_bitmap( int x, int y, grs_bitmap *bm )
{
	int dx1=x, dx2=x+bm->bm_w-1;
	int dy1=y, dy2=y+bm->bm_h-1;
#ifndef OGL
	int sx=0, sy=0;
#endif

	if ((dx1 >= grd_curcanv->cv_bitmap.bm_w ) || (dx2 < 0)) return;
	if ((dy1 >= grd_curcanv->cv_bitmap.bm_h) || (dy2 < 0)) return;
	if ( dx1 < 0 )
	{
#ifndef OGL
		sx = -dx1;
#endif
		dx1 = 0;
	}
	if ( dy1 < 0 )
	{
#ifndef OGL
		sy = -dy1;
#endif
		dy1 = 0;
	}
	if ( dx2 >= grd_curcanv->cv_bitmap.bm_w )	{ dx2 = grd_curcanv->cv_bitmap.bm_w-1; }
	if ( dy2 >= grd_curcanv->cv_bitmap.bm_h )	{ dy2 = grd_curcanv->cv_bitmap.bm_h-1; }

	// Draw bitmap bm[x,y] into (dx1,dy1)-(dx2,dy2)
#ifdef OGL
	ogl_ubitmapm_cs(x, y, 0, 0, bm, -1, F1_0);
#else
	gr_bm_ubitblt(dx2-dx1+1,dy2-dy1+1, dx1, dy1, sx, sy, bm, &grd_curcanv->cv_bitmap );
#endif
}

void gr_bitmapm( int x, int y, grs_bitmap *bm )
{
	int dx1=x, dx2=x+bm->bm_w-1;
	int dy1=y, dy2=y+bm->bm_h-1;
	int sx=0, sy=0;

	if ((dx1 >= grd_curcanv->cv_bitmap.bm_w ) || (dx2 < 0)) return;
	if ((dy1 >= grd_curcanv->cv_bitmap.bm_h) || (dy2 < 0)) return;
	if ( dx1 < 0 ) { sx = -dx1; dx1 = 0; }
	if ( dy1 < 0 ) { sy = -dy1; dy1 = 0; }
	if ( dx2 >= grd_curcanv->cv_bitmap.bm_w )	{ dx2 = grd_curcanv->cv_bitmap.bm_w-1; }
	if ( dy2 >= grd_curcanv->cv_bitmap.bm_h )	{ dy2 = grd_curcanv->cv_bitmap.bm_h-1; }

	// Draw bitmap bm[x,y] into (dx1,dy1)-(dx2,dy2)

	if ( (bm->bm_type == BM_LINEAR) && (grd_curcanv->cv_bitmap.bm_type == BM_LINEAR ))
	{
		if ( bm->bm_flags & BM_FLAG_RLE )
			gr_bm_ubitblt00m_rle(dx2-dx1+1,dy2-dy1+1, dx1, dy1, sx, sy, bm, &grd_curcanv->cv_bitmap );
		else
			gr_bm_ubitblt00m(dx2-dx1+1,dy2-dy1+1, dx1, dy1, sx, sy, bm, &grd_curcanv->cv_bitmap );
		return;
	}

	gr_bm_ubitbltm(dx2-dx1+1,dy2-dy1+1, dx1, dy1, sx, sy, bm, &grd_curcanv->cv_bitmap );

}

void gr_bm_ubitbltm(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	register int x1, y1;
	ubyte c;

#ifdef OGL
	if ( (src->bm_type == BM_LINEAR) && (dest->bm_type == BM_OGL ))
	{
		ogl_ubitblt(w, h, dx, dy, sx, sy, src, dest);
		return;
	}
	if ( (src->bm_type == BM_OGL) && (dest->bm_type == BM_LINEAR ))
	{
		return;
	}
	if ( (src->bm_type == BM_OGL) && (dest->bm_type == BM_OGL ))
	{
		return;
	}
#endif
	for (y1=0; y1 < h; y1++ )    {
		for (x1=0; x1 < w; x1++ )    {
			if ((c=gr_gpixel(src,sx+x1,sy+y1))!=255)
				gr_bm_pixel( dest, dx+x1, dy+y1,c  );
		}
	}

}

static void gr_bm_ubitblt00_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	unsigned char * dbits;
	unsigned char * sbits;
	int i, data_offset;

	data_offset = 1;
	if (src->bm_flags & BM_FLAG_RLE_BIG)
		data_offset = 2;

	sbits = &src->bm_data[4 + (src->bm_h*data_offset)];

	for (i=0; i<sy; i++ )
		sbits += (int)(INTEL_SHORT(src->bm_data[4+(i*data_offset)]));

	dbits = dest->bm_data + (dest->bm_rowsize * dy) + dx;

	// No interlacing, copy the whole buffer.
	for (i=0; i < h; i++ )    {
		gr_rle_expand_scanline( dbits, sbits, sx, sx+w-1 );
		if ( src->bm_flags & BM_FLAG_RLE_BIG )
			sbits += (int)INTEL_SHORT(*((short *)&(src->bm_data[4+((i+sy)*data_offset)])));
		else
			sbits += (int)(src->bm_data[4+i+sy]);
		dbits += dest->bm_rowsize << gr_bitblt_dest_step_shift;
	}
}

static void gr_bm_ubitblt00m_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	unsigned char * dbits;
	unsigned char * sbits;
	int i, data_offset;

	data_offset = 1;
	if (src->bm_flags & BM_FLAG_RLE_BIG)
		data_offset = 2;

	sbits = &src->bm_data[4 + (src->bm_h*data_offset)];
	for (i=0; i<sy; i++ )
		sbits += (int)(INTEL_SHORT(src->bm_data[4+(i*data_offset)]));

	dbits = dest->bm_data + (dest->bm_rowsize * dy) + dx;

	// No interlacing, copy the whole buffer.
	for (i=0; i < h; i++ )    {
		gr_rle_expand_scanline_masked( dbits, sbits, sx, sx+w-1 );
		if ( src->bm_flags & BM_FLAG_RLE_BIG )
			sbits += (int)INTEL_SHORT(*((short *)&(src->bm_data[4+((i+sy)*data_offset)])));
		else
			sbits += (int)(src->bm_data[4+i+sy]);
		dbits += dest->bm_rowsize << gr_bitblt_dest_step_shift;
	}
}

// in rle.c


static void gr_bm_ubitblt0x_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src,
						 grs_bitmap * dest)
{
	int i, data_offset;
	register int y1;
	unsigned char * sbits;

	data_offset = 1;
	if (src->bm_flags & BM_FLAG_RLE_BIG)
		data_offset = 2;

	sbits = &src->bm_data[4 + (src->bm_h*data_offset)];
	for (i=0; i<sy; i++ )
		sbits += (int)(INTEL_SHORT(src->bm_data[4+(i*data_offset)]));

	for (y1=0; y1 < h; y1++ )    {
		gr_rle_expand_scanline_generic( dest, dx, dy+y1,  sbits, sx, sx+w-1);
		if ( src->bm_flags & BM_FLAG_RLE_BIG )
			sbits += (int)INTEL_SHORT(*((short *)&(src->bm_data[4+((y1+sy)*data_offset)])));
		else
			sbits += (int)src->bm_data[4+y1+sy];
	}
}

// rescalling bitmaps, 10/14/99 Jan Bobrowski jb@wizard.ae.krakow.pl

static inline void scale_line(unsigned char *in, unsigned char *out, int ilen, int olen)
{
	int a = olen/ilen, b = olen%ilen;
	int c = 0, i;
	unsigned char *end = out + olen;
	while(out<end) {
		i = a;
		c += b;
		if(c >= ilen) {
			c -= ilen;
			goto inside;
		}
		while(--i>=0) {
inside:
			*out++ = *in;
		}
		in++;
	}
}

void gr_bitmap_scale_to(grs_bitmap *src, grs_bitmap *dst)
{
	unsigned char *s = src->bm_data;
	unsigned char *d = dst->bm_data;
	int h = src->bm_h;
	int a = dst->bm_h/h, b = dst->bm_h%h;
	int c = 0, i, y;

	for(y=0; y<h; y++) {
		i = a;
		c += b;
		if(c >= h) {
			c -= h;
			goto inside;
		}
		while(--i>=0) {
inside:
			scale_line(s, d, src->bm_w, dst->bm_w);
			d += dst->bm_rowsize;
		}
		s += src->bm_rowsize;
	}
}

void show_fullscr(grs_bitmap *bm)
{
	grs_bitmap * const scr = &grd_curcanv->cv_bitmap;

#ifdef OGL
	if(bm->bm_type == BM_LINEAR && scr->bm_type == BM_OGL &&
		bm->bm_w <= grd_curscreen->sc_w && bm->bm_h <= grd_curscreen->sc_h) // only scale with OGL if bitmap is not bigger than screen size
	{
		ogl_ubitmapm_cs(0,0,-1,-1,bm,-1,F1_0);//use opengl to scale, faster and saves ram. -MPM
		return;
	}
#endif
	if(scr->bm_type != BM_LINEAR) {
		grs_bitmap *tmp = gr_create_bitmap(scr->bm_w, scr->bm_h);
		gr_bitmap_scale_to(bm, tmp);
		gr_bitmap(0, 0, tmp);
		gr_free_bitmap(tmp);
		return;
	}
	gr_bitmap_scale_to(bm, scr);
}

// Find transparent area in bitmap
void gr_bitblt_find_transparent_area(grs_bitmap *bm, int *minx, int *miny, int *maxx, int *maxy)
{
	ubyte c;
	int i = 0, x = 0, y = 0, count = 0;
	static unsigned char buf[1024*1024];

	if (!(bm->bm_flags&BM_FLAG_TRANSPARENT))
		return;

	memset(buf,0,1024*1024);

	*minx = bm->bm_w - 1;
	*maxx = 0;
	*miny = bm->bm_h - 1;
	*maxy = 0;

	// decode the bitmap
	if (bm->bm_flags & BM_FLAG_RLE){
		unsigned char * dbits;
		unsigned char * sbits;
		int i, data_offset;

		data_offset = 1;
		if (bm->bm_flags & BM_FLAG_RLE_BIG)
			data_offset = 2;

		sbits = &bm->bm_data[4 + (bm->bm_h * data_offset)];
		dbits = buf;

		for (i=0; i < bm->bm_h; i++ )    {
			gr_rle_decode(sbits,dbits);
			if ( bm->bm_flags & BM_FLAG_RLE_BIG )
				sbits += (int)INTEL_SHORT(*((short *)&(bm->bm_data[4+(i*data_offset)])));
			else
				sbits += (int)bm->bm_data[4+i];
			dbits += bm->bm_w;
		}
	}
	else
	{
		memcpy(&buf, bm->bm_data, sizeof(unsigned char)*(bm->bm_w*bm->bm_h));
	}

	for (y = 0; y < bm->bm_h; y++) {
		for (x = 0; x < bm->bm_w; x++) {
			c = buf[i++];
			if (c == TRANSPARENCY_COLOR) {				// don't look for transparancy color here.
				count++;
				if (x < *minx) *minx = x;
				if (y < *miny) *miny = y;
				if (x > *maxx) *maxx = x;
				if (y > *maxy) *maxy = y;
			}
		}
	}
	Assert (count);
}
