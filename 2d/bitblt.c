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

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <string.h>
#include "u_mem.h"
#include "gr.h"
#include "grdef.h"
#include "rle.h"
#include "error.h"

#ifdef OGL
#include "ogl_init.h"
#endif

int gr_bitblt_dest_step_shift = 0;
int gr_bitblt_double = 0;
ubyte *gr_bitblt_fade_table=NULL;

void gr_bm_ubitblt00_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest);
void gr_bm_ubitblt00m_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest);
void gr_bm_ubitblt0x_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest, int masked);

void gr_bm_ubitblt01(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest);
void gr_bm_ubitblt02(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest);

#include "linear.h"
#include "modex.h"
#include "vesa.h"

#ifdef NO_ASM
void gr_linear_movsd( ubyte * source, ubyte * dest, unsigned int nbytes) {
	memcpy(dest,source,nbytes);
}

void gr_linear_rep_movsdm(ubyte *src, ubyte *dest, int num_pixels) {
	register ubyte c;
	while (num_pixels--)
		if ((c=*src++)!=255)
			*dest++=c;
		else	dest++;
}

void gr_linear_rep_movsdm_faded(ubyte * src, ubyte * dest, int num_pixels, ubyte fade_value ) {
	register ubyte c;
	while (num_pixels--)
		if ((c=*src++)!=255)
			*dest++=gr_fade_table[((int)fade_value<<8)|(int)c];
		else	dest++;
}
void gr_linear_rep_movsd_2x(ubyte * source, ubyte * dest, uint nbytes ) {
	register ubyte c;
	while (nbytes--) {
		if (nbytes&1)
			*dest++=*source++;
		else {
			c=*source++;
			*((unsigned short *)dest)++=((short)c<<8)|(short)c;
		}
	}
}
#endif
#ifdef D1XD3D
#include "d3dhelp.h"
#endif

void gr_ubitmap00( int x, int y, grs_bitmap *bm )
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

void gr_ubitmap00m( int x, int y, grs_bitmap *bm )
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

//"             jmp     aligned4                                "       
//"             mov     eax, edi                                "       
//"             and     eax, 11b                                "       
//"             jz              aligned4                        "       
//"             mov     ebx, 4                                  "       
//"             sub     ebx, eax                                "       
//"             sub     ecx, ebx                                "       
//"alignstart:                                                  "       
//"             mov     al, [esi]                               "       
//"             add     esi, 4                                  "       
//"             mov     [edi], al                               "       
//"             inc     edi                                     "       
//"             dec     ebx                                     "       
//"             jne     alignstart                              "       
//"aligned4:                                                    "       

#ifdef __DJGPP__
// From Linear to ModeX
void gr_bm_ubitblt01(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	ubyte * dbits;
	ubyte * sbits;
	int sstep,dstep;
	int y,plane;
	int w1;

	if ( w < 4 ) return;

	sstep = src->bm_rowsize;
	dstep = dest->bm_rowsize << gr_bitblt_dest_step_shift;

	if (!gr_bitblt_double)	{
		for (plane=0; plane<4; plane++ )	{
			gr_modex_setplane( (plane+dx)&3 );
			sbits = src->bm_data + (src->bm_rowsize * sy) + sx + plane;
			dbits = &gr_video_memory[(dest->bm_rowsize * dy) + ((plane+dx)/4) ];
			w1 = w >> 2;
			if ( (w&3) > plane ) w1++;
			for (y=dy; y < dy+h; y++ )		{
				modex_copy_scanline( sbits, dbits, w1 );		
				dbits += dstep;
				sbits += sstep;
			}
		}
	} else {
		for (plane=0; plane<4; plane++ )	{
			gr_modex_setplane( (plane+dx)&3 );
			sbits = src->bm_data + (src->bm_rowsize * sy) + sx + plane/2;
			dbits = &gr_video_memory[(dest->bm_rowsize * dy) + ((plane+dx)/4) ];
			w1 = w >> 2;
			if ( (w&3) > plane ) w1++;
			for (y=dy; y < dy+h; y++ )		{
				modex_copy_scanline_2x( sbits, dbits, w1 );		
				dbits += dstep;
				sbits += sstep;
			}
		}
	}
}


// From Linear to ModeX masked
void gr_bm_ubitblt01m(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	//ubyte * dbits1;
	//ubyte * sbits1;
	
	ubyte * dbits;
	ubyte * sbits;

	int x;
//	int y;

	sbits =   src->bm_data  + (src->bm_rowsize * sy) + sx;
	dbits =   &gr_video_memory[(dest->bm_rowsize * dy) + dx/4];

	for (x=dx; x < dx+w; x++ )	{	
		gr_modex_setplane( x&3 );

		//sbits1 = sbits;
		//dbits1 = dbits;
		//for (y=0; y < h; y++ )    {
		//	*dbits1 = *sbits1;
		//	sbits1 += src_bm_rowsize;
		//	dbits1 += dest_bm_rowsize;
		//	}
		modex_copy_column_m(sbits, dbits, h, src->bm_rowsize, dest->bm_rowsize << gr_bitblt_dest_step_shift );

		sbits++;
		if ( (x&3)==3 )
			dbits++;
	}
}

#endif


void gr_ubitmap012( int x, int y, grs_bitmap *bm )
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

void gr_ubitmap012m( int x, int y, grs_bitmap *bm )
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


void gr_ubitmapGENERIC(int x, int y, grs_bitmap * bm)
{
	register int x1, y1;

	for (y1=0; y1 < bm->bm_h; y1++ )    {
		for (x1=0; x1 < bm->bm_w; x1++ )    {
			gr_setcolor( gr_gpixel(bm,x1,y1) );
			gr_upixel( x+x1, y+y1 );
		}
	}
}

void gr_ubitmapGENERICm(int x, int y, grs_bitmap * bm)
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
			ogl_ubitmapm(x,y,bm);
			return;
#endif
#ifdef D1XD3D
		case BM_DIRECTX:
			Assert ((int)grd_curcanv->cv_bitmap.bm_data == BM_D3D_RENDER || (int)grd_curcanv->cv_bitmap.bm_data == BM_D3D_DISPLAY);
			Win32_BlitLinearToDirectX_bm(bm, 0, 0, bm->bm_w, bm->bm_h, x, y, 0);
			return;
#endif
#ifdef __DJGPP__
		case BM_SVGA:
			if ( bm->bm_flags & BM_FLAG_RLE )
				gr_bm_ubitblt0x_rle(bm->bm_w, bm->bm_h, x, y, 0, 0, bm, &grd_curcanv->cv_bitmap, 0 );
			else
				gr_bm_ubitblt02( bm->bm_w, bm->bm_h, x, y, 0, 0, bm, &grd_curcanv->cv_bitmap);
			return;
		case BM_MODEX:
			gr_bm_ubitblt01(bm->bm_w, bm->bm_h, x+XOFFSET, y+YOFFSET, 0, 0, bm, &grd_curcanv->cv_bitmap);
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
			ogl_ubitmapm(x,y,bm);
			return;
#endif
#ifdef D1XD3D
		case BM_DIRECTX:
			if (bm->bm_w < 35 && bm->bm_h < 35) {
				// ugly hack needed for reticle
				if ( bm->bm_flags & BM_FLAG_RLE )
					gr_bm_ubitblt0x_rle(bm->bm_w, bm->bm_h, x, y, 0, 0, bm, &grd_curcanv->cv_bitmap, 1 );
				else
					gr_ubitmapGENERICm(x, y, bm);
				return;
			}
			Assert ((int)grd_curcanv->cv_bitmap.bm_data == BM_D3D_RENDER || (int)grd_curcanv->cv_bitmap.bm_data == BM_D3D_DISPLAY);
			Win32_BlitLinearToDirectX_bm(bm, 0, 0, bm->bm_w, bm->bm_h, x, y, 1);
			return;
#endif
#ifdef __DJGPP__
		case BM_SVGA:
			gr_ubitmapGENERICm(x, y, bm);
			return;
		case BM_MODEX:
			gr_bm_ubitblt01m(bm->bm_w, bm->bm_h, x+XOFFSET, y+YOFFSET, 0, 0, bm, &grd_curcanv->cv_bitmap);
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


#ifdef __DJGPP__
// From linear to SVGA
void gr_bm_ubitblt02(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	unsigned char * sbits;

	unsigned int offset, EndingOffset, VideoLocation;

	int sbpr, dbpr, y1, page, BytesToMove;

	sbpr = src->bm_rowsize;

	dbpr = dest->bm_rowsize << gr_bitblt_dest_step_shift;

	VideoLocation = (unsigned int)dest->bm_data + (dest->bm_rowsize * dy) + dx;

	sbits = src->bm_data + ( sbpr*sy ) + sx;

	for (y1=0; y1 < h; y1++ )    {

		page    = VideoLocation >> 16;
		offset  = VideoLocation & 0xFFFF;

		gr_vesa_setpage( page );

		EndingOffset = offset+w-1;

		if ( EndingOffset <= 0xFFFF )
		{
			if ( gr_bitblt_double )
				gr_linear_rep_movsd_2x( (void *)sbits, (void *)(offset+gr_video_memory), w );
			else
				gr_linear_movsd( (void *)sbits, (void *)(offset+gr_video_memory), w );

			VideoLocation += dbpr;
			sbits += sbpr;
		}
		else
		{
			BytesToMove = 0xFFFF-offset+1;

			if ( gr_bitblt_double )
				gr_linear_rep_movsd_2x( (void *)sbits, (void *)(offset+gr_video_memory), BytesToMove );
			else
				gr_linear_movsd( (void *)sbits, (void *)(offset+gr_video_memory), BytesToMove );

			page++;
			gr_vesa_setpage(page);

			if ( gr_bitblt_double )
				gr_linear_rep_movsd_2x( (void *)(sbits+BytesToMove/2), (void *)gr_video_memory, EndingOffset - 0xFFFF );
			else
				gr_linear_movsd( (void *)(sbits+BytesToMove), (void *)gr_video_memory, EndingOffset - 0xFFFF );

			VideoLocation += dbpr;
			sbits += sbpr;
		}
	}
}

// From SVGA to linear
void gr_bm_ubitblt20(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	unsigned char * dbits;

	unsigned int offset, offset1, offset2;

	int sbpr, dbpr, y1, page;

	dbpr = dest->bm_rowsize;

	sbpr = src->bm_rowsize;

	for (y1=0; y1 < h; y1++ )    {

		offset2 =   (unsigned int)src->bm_data  + (sbpr * (y1+sy)) + sx;
		dbits   =   dest->bm_data + (dbpr * (y1+dy)) + dx;

		page = offset2 >> 16;
		offset = offset2 & 0xFFFF;
		offset1 = offset+w-1;
		gr_vesa_setpage( page );

		if ( offset1 > 0xFFFF )  {
			// Overlaps two pages
			while( offset <= 0xFFFF )
				*dbits++ = gr_video_memory[offset++];
			offset1 -= (0xFFFF+1);
			offset = 0;
			page++;
			gr_vesa_setpage(page);
		}
		while( offset <= offset1 )
			*dbits++ = gr_video_memory[offset++];

	}
}
#endif // __DJGPP__
//@extern int Interlacing_on;

// From Linear to Linear
void gr_bm_ubitblt00(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
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
void gr_bm_ubitblt00m(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
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
		ogl_ubitblt_tolinear(w, h, dx, dy, sx, sy, src, dest);
		return;
	}
	if ( (src->bm_type == BM_OGL) && (dest->bm_type == BM_OGL ))
	{
		ogl_ubitblt_copy(w, h, dx, dy, sx, sy, src, dest);
		return;
	}
#endif

#ifdef D1XD3D
	if ( (src->bm_type == BM_LINEAR) && (dest->bm_type == BM_DIRECTX ))
	{
		Assert ((int)dest->bm_data == BM_D3D_RENDER || (int)dest->bm_data == BM_D3D_DISPLAY);
		Win32_BlitLinearToDirectX_bm (src, sx, sy, w, h, dx, dy, 0);
		return;
	}
	if ( (src->bm_type == BM_DIRECTX) && (dest->bm_type == BM_LINEAR ))
	{
		return;
	}
	if ( (src->bm_type == BM_DIRECTX) && (dest->bm_type == BM_DIRECTX ))
	{
		return;
	}
#endif

	if ( (src->bm_flags & BM_FLAG_RLE ) && (src->bm_type == BM_LINEAR) )	{
		gr_bm_ubitblt0x_rle(w, h, dx, dy, sx, sy, src, dest, 0 );
	 	return;
	}
#ifdef __DJGPP__
	if ( (src->bm_type == BM_LINEAR) && (dest->bm_type == BM_SVGA ))
	{
		gr_bm_ubitblt02( w, h, dx, dy, sx, sy, src, dest );
		return;
	}

	if ( (src->bm_type == BM_SVGA) && (dest->bm_type == BM_LINEAR ))
	{
		gr_bm_ubitblt20( w, h, dx, dy, sx, sy, src, dest );
		return;
	}

	if ( (src->bm_type == BM_LINEAR) && (dest->bm_type == BM_MODEX ))
	{
		gr_bm_ubitblt01( w, h, dx+XOFFSET, dy+YOFFSET, sx, sy, src, dest );
		return;
	}
#endif
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
	int sx=0, sy=0;

	if ((dx1 >= grd_curcanv->cv_bitmap.bm_w ) || (dx2 < 0)) return;
	if ((dy1 >= grd_curcanv->cv_bitmap.bm_h) || (dy2 < 0)) return;
	if ( dx1 < 0 ) { sx = -dx1; dx1 = 0; }
	if ( dy1 < 0 ) { sy = -dy1; dy1 = 0; }
	if ( dx2 >= grd_curcanv->cv_bitmap.bm_w )	{ dx2 = grd_curcanv->cv_bitmap.bm_w-1; }
	if ( dy2 >= grd_curcanv->cv_bitmap.bm_h )	{ dy2 = grd_curcanv->cv_bitmap.bm_h-1; }
		
	// Draw bitmap bm[x,y] into (dx1,dy1)-(dx2,dy2)

	gr_bm_ubitblt(dx2-dx1+1,dy2-dy1+1, dx1, dy1, sx, sy, bm, &grd_curcanv->cv_bitmap );

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
		ogl_ubitblt_tolinear(w, h, dx, dy, sx, sy, src, dest);
		return;
	}
	if ( (src->bm_type == BM_OGL) && (dest->bm_type == BM_OGL ))
	{
		ogl_ubitblt_copy(w, h, dx, dy, sx, sy, src, dest);
		return;
	}
#endif
#ifdef D1XD3D
	if ( (src->bm_type == BM_LINEAR) && (dest->bm_type == BM_DIRECTX ))
	{
		Assert ((int)dest->bm_data == BM_D3D_RENDER || (int)dest->bm_data == BM_D3D_DISPLAY);
		Win32_BlitLinearToDirectX_bm (src, sx, sy, w, h, dx, dy, 1);
		return;
	}
	if ( (src->bm_type == BM_DIRECTX) && (dest->bm_type == BM_DIRECTX ))
	{
		Assert ((int)src->bm_data == BM_D3D_RENDER || (int)src->bm_data == BM_D3D_DISPLAY);
//		Win32_BlitDirectXToDirectX (w, h, dx, dy, sx, sy, src->bm_data, dest->bm_data, 0);
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

//-NOT-used // From linear to SVGA
//-NOT-used void gr_bm_ubitblt02_2x(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
//-NOT-used {
//-NOT-used 	unsigned char * sbits;
//-NOT-used 
//-NOT-used 	unsigned int offset, EndingOffset, VideoLocation;
//-NOT-used 
//-NOT-used 	int sbpr, dbpr, y1, page, BytesToMove;
//-NOT-used 
//-NOT-used 	sbpr = src->bm_rowsize;
//-NOT-used 
//-NOT-used 	dbpr = dest->bm_rowsize << gr_bitblt_dest_step_shift;
//-NOT-used 
//-NOT-used 	VideoLocation = (unsigned int)dest->bm_data + (dest->bm_rowsize * dy) + dx;
//-NOT-used 
//-NOT-used 	sbits = src->bm_data + ( sbpr*sy ) + sx;
//-NOT-used 
//-NOT-used 	for (y1=0; y1 < h; y1++ )    {
//-NOT-used 
//-NOT-used 		page    = VideoLocation >> 16;
//-NOT-used 		offset  = VideoLocation & 0xFFFF;
//-NOT-used 
//-NOT-used 		gr_vesa_setpage( page );
//-NOT-used 
//-NOT-used 		EndingOffset = offset+w-1;
//-NOT-used 
//-NOT-used 		if ( EndingOffset <= 0xFFFF )
//-NOT-used 		{
//-NOT-used 			gr_linear_rep_movsd_2x( (void *)sbits, (void *)(offset+gr_video_memory), w );
//-NOT-used 
//-NOT-used 			VideoLocation += dbpr;
//-NOT-used 			sbits += sbpr;
//-NOT-used 		}
//-NOT-used 		else
//-NOT-used 		{
//-NOT-used 			BytesToMove = 0xFFFF-offset+1;
//-NOT-used 
//-NOT-used 			gr_linear_rep_movsd_2x( (void *)sbits, (void *)(offset+gr_video_memory), BytesToMove );
//-NOT-used 
//-NOT-used 			page++;
//-NOT-used 			gr_vesa_setpage(page);
//-NOT-used 
//-NOT-used 			gr_linear_rep_movsd_2x( (void *)(sbits+BytesToMove/2), (void *)gr_video_memory, EndingOffset - 0xFFFF );
//-NOT-used 
//-NOT-used 			VideoLocation += dbpr;
//-NOT-used 			sbits += sbpr;
//-NOT-used 		}
//-NOT-used 
//-NOT-used 
//-NOT-used 	}
//-NOT-used }


//-NOT-used // From Linear to Linear
//-NOT-used void gr_bm_ubitblt00_2x(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
//-NOT-used {
//-NOT-used 	unsigned char * dbits;
//-NOT-used 	unsigned char * sbits;
//-NOT-used 	//int	src_bm_rowsize_2, dest_bm_rowsize_2;
//-NOT-used 
//-NOT-used 	int i;
//-NOT-used 
//-NOT-used 	sbits =   src->bm_data  + (src->bm_rowsize * sy) + sx;
//-NOT-used 	dbits =   dest->bm_data + (dest->bm_rowsize * dy) + dx;
//-NOT-used 
//-NOT-used 	// No interlacing, copy the whole buffer.
//-NOT-used 	for (i=0; i < h; i++ )    {
//-NOT-used 		gr_linear_rep_movsd_2x( sbits, dbits, w );
//-NOT-used 
//-NOT-used 		sbits += src->bm_rowsize;
//-NOT-used 		dbits += dest->bm_rowsize << gr_bitblt_dest_step_shift;
//-NOT-used 	}
//-NOT-used }

void gr_bm_ubitblt00_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	unsigned char * dbits;
	unsigned char * sbits;

	int i;

	sbits = &src->bm_data[4 + src->bm_h];
	for (i=0; i<sy; i++ )
		sbits += (int)src->bm_data[4+i];

	dbits = dest->bm_data + (dest->bm_rowsize * dy) + dx;

	// No interlacing, copy the whole buffer.
	for (i=0; i < h; i++ )    {
		gr_rle_expand_scanline( dbits, sbits, sx, sx+w-1 );
		sbits += (int)src->bm_data[4+i+sy];
		dbits += dest->bm_rowsize << gr_bitblt_dest_step_shift;
	}
}

void gr_bm_ubitblt00m_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, grs_bitmap * dest)
{
	unsigned char * dbits;
	unsigned char * sbits;

	int i;

	sbits = &src->bm_data[4 + src->bm_h];
	for (i=0; i<sy; i++ )
		sbits += (int)src->bm_data[4+i];

	dbits = dest->bm_data + (dest->bm_rowsize * dy) + dx;

	// No interlacing, copy the whole buffer.
	for (i=0; i < h; i++ )    {
		gr_rle_expand_scanline_masked( dbits, sbits, sx, sx+w-1 );
		sbits += (int)src->bm_data[4+i+sy];
		dbits += dest->bm_rowsize << gr_bitblt_dest_step_shift;
	}
}

// in rle.c

extern void gr_rle_expand_scanline_generic( grs_bitmap * dest, int dx, int dy, ubyte *src, 
	int x1, int x2, int masked );


void gr_bm_ubitblt0x_rle(int w, int h, int dx, int dy, int sx, int sy, grs_bitmap * src, 
	grs_bitmap * dest, int masked )
{
	int i;
	register int y1;
	unsigned char * sbits;

	sbits = &src->bm_data[4 + src->bm_h];
	for (i=0; i<sy; i++ )
		sbits += (int)src->bm_data[4+i];

	for (y1=0; y1 < h; y1++ )    {
		gr_rle_expand_scanline_generic( dest, dx, dy+y1,  sbits, sx, sx+w-1, 
			masked );
		sbits += (int)src->bm_data[4+y1+sy];
	}
}

// rescalling bitmaps, 10/14/99 Jan Bobrowski jb@wizard.ae.krakow.pl

inline void scale_line(byte *in, byte *out, int ilen, int olen)
{
	int a = olen/ilen, b = olen%ilen;
	int c = 0, i;
	byte *end = out + olen;
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
	byte *s = src->bm_data;
	byte *d = dst->bm_data;
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
	if(bm->bm_type == BM_LINEAR && scr->bm_type == BM_OGL) {
		ogl_ubitblt_i(scr->bm_w,scr->bm_h,0,0,bm->bm_w,bm->bm_h,0,0,bm,scr);//use opengl to scale, faster and saves ram. -MPM
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
