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
 * $Source: /cvs/cvsroot/d2x/texmap/scanline.c,v $
 * $Revision: 1.3 $
 * $Author: bradleyb $
 * $Date: 2001-10-25 02:22:46 $
 * 
 * Routines to draw the texture mapped scanlines.
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2001/01/31 15:18:04  bradleyb
 * Makefile and conf.h fixes
 *
 * Revision 1.1.1.1  2001/01/19 03:30:16  bradleyb
 * Import of d2x-0.0.8
 *
 * Revision 1.4  1999/10/18 00:31:01  donut
 * more alpha fixes from Falk Hueffner
 *
 * Revision 1.3  1999/10/14 04:48:21  donut
 * alpha fixes, and gl_font args
 *
 * Revision 1.2  1999/09/21 07:22:40  sekmu
 * remove unused var warning
 *
 * Revision 1.1.1.1  1999/06/14 22:14:08  donut
 * Import of d1x 1.37 source.
 *
 * Revision 1.2  1995/02/20  18:23:39  john
 * Added new module for C versions of inner loops.
 * 
 * Revision 1.1  1995/02/20  17:42:27  john
 * Initial revision
 * 
 * 
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#ifdef RCS
static char rcsid[] = "$Id: scanline.c,v 1.3 2001-10-25 02:22:46 bradleyb Exp $";
#endif

#include <math.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "maths.h"
#include "mono.h"
#include "gr.h"
#include "grdef.h"
#include "texmap.h"
#include "texmapl.h"
#include "scanline.h"

void c_tmap_scanline_flat()
{
	ubyte *dest;
//        int x;

	dest = (ubyte *)(write_buffer + fx_xleft + (bytes_per_row * fx_y )  );

/*	for (x= fx_xright-fx_xleft+1 ; x > 0; --x ) {
		*dest++ = tmap_flat_color;
	}*/
        memset(dest,tmap_flat_color,fx_xright-fx_xleft+1);
}

void c_tmap_scanline_shaded()
{
	int fade;
	ubyte *dest, tmp;
	int x;

	dest = (ubyte *)(write_buffer + fx_xleft + (bytes_per_row * fx_y)  );

	fade = tmap_flat_shade_value<<8;
	for (x= fx_xright-fx_xleft+1 ; x > 0; --x ) {
		tmp = *dest;
		*dest++ = gr_fade_table[ fade |(tmp)];
	}
}

void c_tmap_scanline_lin_nolight()
{
	ubyte *dest;
	uint c;
	int x;
	fix u,v,dudx, dvdx;

	u = fx_u;
	v = fx_v*64;
	dudx = fx_du_dx; 
	dvdx = fx_dv_dx*64; 

	dest = (ubyte *)(write_buffer + fx_xleft + (bytes_per_row * fx_y)  );

	if (!Transparency_on)	{
		for (x= fx_xright-fx_xleft+1 ; x > 0; --x ) {
			*dest++ = (uint)pixptr[ (f2i(v)&(64*63)) + (f2i(u)&63) ];
			u += dudx;
			v += dvdx;
		}
	} else {
		for (x= fx_xright-fx_xleft+1 ; x > 0; --x ) {
			c = (uint)pixptr[ (f2i(v)&(64*63)) + (f2i(u)&63) ];
			if ( c!=255)
				*dest = c;
			dest++;
			u += dudx;
			v += dvdx;
		}
	}
}


#if 1
void c_tmap_scanline_lin()
{
	ubyte *dest;
	uint c;
	int x, j;
	fix u,v,l,dudx, dvdx, dldx;

	u = fx_u;
	v = fx_v*64;
	dudx = fx_du_dx; 
	dvdx = fx_dv_dx*64; 

	l = fx_l>>8;
	dldx = fx_dl_dx>>8;
	dest = (ubyte *)(write_buffer + fx_xleft + (bytes_per_row * fx_y)  );

	if (!Transparency_on)	{
		ubyte*			pixPtrLocalCopy = pixptr;
		ubyte*			fadeTableLocalCopy = gr_fade_table;
		unsigned long	destlong;

		x = fx_xright-fx_xleft+1;

		if ((j = (unsigned long) dest & 3) != 0)
			{
			j = 4 - j;

			if (j > x)
				j = x;

			while (j > 0)
				{	
				//edited 05/18/99 Matt Mueller - changed from 0xff00 to 0x7f00 to fix glitches
				*dest++ = (unsigned long) fadeTableLocalCopy[ (l&(0x7f00)) + (uint) pixPtrLocalCopy[ (f2i(v)&(64*63)) + (f2i(u)&63) ] ];
				//end edit -MM
				l += dldx;
				u += dudx;
				v += dvdx;
				x--;
				j--;
				}
			}

		j &= ~3;
		while (j > 0)
			{
			//edited 05/18/99 Matt Mueller - changed from 0xff00 to 0x7f00 to fix glitches
			destlong = (unsigned long) fadeTableLocalCopy[ (l&(0x7f00)) + (uint) pixPtrLocalCopy[ (f2i(v)&(64*63)) + (f2i(u)&63) ] ] << 24;
			//end edit -MM
			l += dldx;
			u += dudx;
			v += dvdx;
			//edited 05/18/99 Matt Mueller - changed from 0xff00 to 0x7f00 to fix glitches
			destlong |= (unsigned long) fadeTableLocalCopy[ (l&(0x7f00)) + (uint) pixPtrLocalCopy[ (f2i(v)&(64*63)) + (f2i(u)&63) ] ] << 16;
			//end edit -MM
			l += dldx;
			u += dudx;
			v += dvdx;
			//edited 05/18/99 Matt Mueller - changed from 0xff00 to 0x7f00 to fix glitches
			destlong |= (unsigned long) fadeTableLocalCopy[ (l&(0x7f00)) + (uint) pixPtrLocalCopy[ (f2i(v)&(64*63)) + (f2i(u)&63) ] ] << 8;
			//end edit -MM
			l += dldx;
			u += dudx;
			v += dvdx;
			//edited 05/18/99 Matt Mueller - changed from 0xff00 to 0x7f00 to fix glitches
			destlong |= (unsigned long) fadeTableLocalCopy[ (l&(0x7f00)) + (uint) pixPtrLocalCopy[ (f2i(v)&(64*63)) + (f2i(u)&63) ] ];
			//end edit -MM
			l += dldx;
			u += dudx;
			v += dvdx;
			*((unsigned long *) dest) = destlong;
			dest += 4;
			x -= 4;
			j -= 4;
			}

		while (x-- > 0)
			{
			//edited 05/18/99 Matt Mueller - changed from 0xff00 to 0x7f00 to fix glitches
			*dest++ = (unsigned long) fadeTableLocalCopy[ (l&(0x7f00)) + (uint) pixPtrLocalCopy[ (f2i(v)&(64*63)) + (f2i(u)&63) ] ];
			//end edit -MM
			l += dldx;
			u += dudx;
			v += dvdx;
			}

	} else {
		for (x= fx_xright-fx_xleft+1 ; x > 0; --x ) {
			c = (uint)pixptr[ (f2i(v)&(64*63)) + (f2i(u)&63) ];
			if ( c!=TRANSPARENCY_COLOR)
			//edited 05/18/99 Matt Mueller - changed from 0xff00 to 0x7f00 to fix glitches
				*dest = gr_fade_table[ (l&(0x7f00)) + c ];
			//end edit -MM
			dest++;
			l += dldx;
			u += dudx;
			v += dvdx;
		}
	}
}

#else
void c_tmap_scanline_lin()
{
	ubyte *dest;
	uint c;
	int x;
	fix u,v,l,dudx, dvdx, dldx;

	u = fx_u;
	v = fx_v*64;
	dudx = fx_du_dx; 
	dvdx = fx_dv_dx*64; 

	l = fx_l>>8;
	dldx = fx_dl_dx>>8;
	dest = (ubyte *)(write_buffer + fx_xleft + (bytes_per_row * fx_y)  );

	if (!Transparency_on)	{
		for (x= fx_xright-fx_xleft+1 ; x > 0; --x ) {
			//edited 05/18/99 Matt Mueller - changed from 0xff00 to 0x7f00 to fix glitches
			*dest++ = gr_fade_table[ (l&(0x7f00)) + (uint)pixptr[ (f2i(v)&(64*63)) + (f2i(u)&63) ] ];
			//end edit -MM
			l += dldx;
			u += dudx;
			v += dvdx;
		}
	} else {
		for (x= fx_xright-fx_xleft+1 ; x > 0; --x ) {
			c = (uint)pixptr[ (f2i(v)&(64*63)) + (f2i(u)&63) ];
			if ( c!=255)
			//edited 05/18/99 Matt Mueller - changed from 0xff00 to 0x7f00 to fix glitches
				*dest = gr_fade_table[ (l&(0x7f00)) + c ];
			//end edit -MM
			dest++;
			l += dldx;
			u += dudx;
			v += dvdx;
		}
	}
}
#endif

// Used for energy centers. See comments for c_tmap_scanline_per().
void c_fp_tmap_scanline_per_nolight()
{
	ubyte	       *dest;
	uint		c;
	int		x, j;
	double		u, v, z, dudx, dvdx, dzdx, rec_z;
	u_int64_t	destlong;

	u = f2db(fx_u);
	v = f2db(fx_v) * 64.0;
	z = f2db(fx_z);
	dudx = f2db(fx_du_dx);
	dvdx = f2db(fx_dv_dx) * 64.0;
	dzdx = f2db(fx_dz_dx);

	rec_z = 1.0 / z;

	dest = (ubyte *) (write_buffer + fx_xleft + (bytes_per_row * fx_y));

	x = fx_xright - fx_xleft + 1;
	if (!Transparency_on) {
		if (x >= 8) {
			if ((j = (size_t) dest & 7) != 0) {
				j = 8 - j;

				while (j > 0) {
					*dest++ =
					    (uint) pixptr[(((int) (v * rec_z)) & (64 * 63)) +
							 (((int) (u * rec_z)) & 63)];
					u += dudx;
					v += dvdx;
					z += dzdx;
					rec_z = 1.0 / z;
					x--;
					j--;
				}
			}

			while (j >= 8) {
				destlong =
				    (u_int64_t) pixptr[(((int) (v * rec_z)) & (64 * 63)) +
						       (((int) (u * rec_z)) & 63)];
				u += dudx;
				v += dvdx;
				z += dzdx;
				rec_z = 1.0 / z;
				destlong |=
				    (u_int64_t) pixptr[(((int) (v * rec_z)) & (64 * 63)) +
						       (((int) (u * rec_z)) & 63)] << 8;
				u += dudx;
				v += dvdx;
				z += dzdx;
				rec_z = 1.0 / z;
				destlong |=
				    (u_int64_t) pixptr[(((int) (v * rec_z)) & (64 * 63)) +
						       (((int) (u * rec_z)) & 63)] << 16;
				u += dudx;
				v += dvdx;
				z += dzdx;
				rec_z = 1.0 / z;
				destlong |=
				    (u_int64_t) pixptr[(((int) (v * rec_z)) & (64 * 63)) +
						       (((int) (u * rec_z)) & 63)] << 24;
				u += dudx;
				v += dvdx;
				z += dzdx;
				rec_z = 1.0 / z;
				destlong |=
				    (u_int64_t) pixptr[(((int) (v * rec_z)) & (64 * 63)) +
						       (((int) (u * rec_z)) & 63)] << 32;
				u += dudx;
				v += dvdx;
				z += dzdx;
				rec_z = 1.0 / z;
				destlong |=
				    (u_int64_t) pixptr[(((int) (v * rec_z)) & (64 * 63)) +
						       (((int) (u * rec_z)) & 63)] << 40;
				u += dudx;
				v += dvdx;
				z += dzdx;
				rec_z = 1.0 / z;
				destlong |=
				    (u_int64_t) pixptr[(((int) (v * rec_z)) & (64 * 63)) +
						       (((int) (u * rec_z)) & 63)] << 48;
				u += dudx;
				v += dvdx;
				z += dzdx;
				rec_z = 1.0 / z;
				destlong |=
				    (u_int64_t) pixptr[(((int) (v * rec_z)) & (64 * 63)) +
						       (((int) (u * rec_z)) & 63)] << 56;
				u += dudx;
				v += dvdx;
				z += dzdx;
				rec_z = 1.0 / z;

				*((u_int64_t *) dest) = destlong;
				dest += 8;
				x -= 8;
				j -= 8;
			}
		}
		while (x-- > 0) {
			*dest++ =
			    (u_int64_t) pixptr[(((int) (v * rec_z)) & (64 * 63)) +
					       (((int) (u * rec_z)) & 63)];
			u += dudx;
			v += dvdx;
			z += dzdx;
			rec_z = 1.0 / z;
		}
	} else {
		x = fx_xright - fx_xleft + 1;

		if (x >= 8) {
			if ((j = (size_t) dest & 7) != 0) {
				j = 8 - j;

				while (j > 0) {
					c =
					    (uint) pixptr[(((int) (v * rec_z)) & (64 * 63)) +
							 (((int) (u * rec_z)) & 63)];
					if (c != 255)
						*dest = c;
					dest++;
					u += dudx;
					v += dvdx;
					z += dzdx;
					rec_z = 1.0 / z;
					x--;
					j--;
				}
			}

			j = x;
			while (j >= 8) {
				destlong = *((u_int64_t *) dest);
				c = (uint) pixptr[(((int) (v * rec_z)) & (64 * 63)) +
						  (((int) (u * rec_z)) & 63)];
				if (c != 255) {
					destlong &= ~(u_int64_t)0xFF;
					destlong |= (u_int64_t) c;
				}
				u += dudx;
				v += dvdx;
				z += dzdx;
				rec_z = 1.0 / z;
				c = (uint) pixptr[(((int) (v * rec_z)) & (64 * 63)) +
						  (((int) (u * rec_z)) & 63)];
				if (c != 255) {
					destlong &= ~((u_int64_t)0xFF << 8);
					destlong |= (u_int64_t) c << 8;
				}
				u += dudx;
				v += dvdx;
				z += dzdx;
				rec_z = 1.0 / z;
				c = (uint) pixptr[(((int) (v * rec_z)) & (64 * 63)) +
						  (((int) (u * rec_z)) & 63)];
				if (c != 255) {
					destlong &= ~((u_int64_t)0xFF << 16);
					destlong |= (u_int64_t) c << 16;
				}
				u += dudx;
				v += dvdx;
				z += dzdx;
				rec_z = 1.0 / z;
				c = (uint) pixptr[(((int) (v * rec_z)) & (64 * 63)) +
						  (((int) (u * rec_z)) & 63)];
				if (c != 255) {
					destlong &= ~((u_int64_t)0xFF << 24);
					destlong |= (u_int64_t) c << 24;
				}
				u += dudx;
				v += dvdx;
				z += dzdx;
				rec_z = 1.0 / z;
				c = (uint) pixptr[(((int) (v * rec_z)) & (64 * 63)) +
						  (((int) (u * rec_z)) & 63)];
				if (c != 255) {
					destlong &= ~((u_int64_t)0xFF << 32);
					destlong |= (u_int64_t) c << 32;
				}
				u += dudx;
				v += dvdx;
				z += dzdx;
				rec_z = 1.0 / z;
				c = (uint) pixptr[(((int) (v * rec_z)) & (64 * 63)) +
						  (((int) (u * rec_z)) & 63)];
				if (c != 255) {
					destlong &= ~((u_int64_t)0xFF << 40);
					destlong |= (u_int64_t) c << 40;
				}
				u += dudx;
				v += dvdx;
				z += dzdx;
				rec_z = 1.0 / z;
				c = (uint) pixptr[(((int) (v * rec_z)) & (64 * 63)) +
						  (((int) (u * rec_z)) & 63)];
				if (c != 255) {
					destlong &= ~((u_int64_t)0xFF << 48);
					destlong |= (u_int64_t) c << 48;
				}
				u += dudx;
				v += dvdx;
				z += dzdx;
				rec_z = 1.0 / z;
				c = (uint) pixptr[(((int) (v * rec_z)) & (64 * 63)) +
						  (((int) (u * rec_z)) & 63)];
				if (c != 255) {
					destlong &= ~((u_int64_t)0xFF << 56);
					destlong |= (u_int64_t) c << 56;
				}
				u += dudx;
				v += dvdx;
				z += dzdx;
				rec_z = 1.0 / z;

				*((u_int64_t *) dest) = destlong;
				dest += 8;
				x -= 8;
				j -= 8;
			}
		}
		while (x-- > 0) {
			c = (uint) pixptr[(((int) (v * rec_z)) & (64 * 63)) +
					  (((int) (u * rec_z)) & 63)];
			if (c != 255)
				*dest = c;
			dest++;
			u += dudx;
			v += dvdx;
			z += dzdx;
			rec_z = 1.0 / z;
		}
	}
}

void c_tmap_scanline_per_nolight()
{
	ubyte *dest;
	uint c;
	int x;
	fix u,v,z,dudx, dvdx, dzdx;

	u = fx_u;
	v = fx_v*64;
	z = fx_z;
	dudx = fx_du_dx; 
	dvdx = fx_dv_dx*64; 
	dzdx = fx_dz_dx;

	dest = (ubyte *)(write_buffer + fx_xleft + (bytes_per_row * fx_y)  );

	if (!Transparency_on)	{
		for (x= fx_xright-fx_xleft+1 ; x > 0; --x ) {
			*dest++ = (uint)pixptr[ ( (v/z)&(64*63) ) + ((u/z)&63) ];
			u += dudx;
			v += dvdx;
			z += dzdx;
		}
	} else {
		for (x= fx_xright-fx_xleft+1 ; x > 0; --x ) {
			c = (uint)pixptr[ ( (v/z)&(64*63) ) + ((u/z)&63) ];
			if ( c!=255)
				*dest = c;
			dest++;
			u += dudx;
			v += dvdx;
			z += dzdx;
		}
	}
}

// This texture mapper uses floating point extensively and writes 8 pixels at once, so it likely works
// best on 64 bit RISC processors.
// WARNING: it is not endian clean. For big endian, reverse the shift counts in the unrolled loops. I
// have no means to test that, so I didn't try it. Please tell me if you get this to work on a big
// endian machine.
// If you're using an Alpha, use the Compaq compiler for this file for quite some fps more.
// Unfortunately, it won't compile the whole source, so simply compile everything, change the
// compiler to ccc, remove scanline.o and compile again.
// Please send comments/suggestions to falk.hueffner@student.uni-tuebingen.de.
void c_fp_tmap_scanline_per()
{
	ubyte          *dest;
	uint            c;
	int             x, j;
	double          u, v, z, l, dudx, dvdx, dzdx, dldx, rec_z;
	u_int64_t       destlong;

	u = f2db(fx_u);
	v = f2db(fx_v) * 64.0;
	z = f2db(fx_z);
	l = f2db(fx_l);
	dudx = f2db(fx_du_dx);
	dvdx = f2db(fx_dv_dx) * 64.0;
	dzdx = f2db(fx_dz_dx);
	dldx = f2db(fx_dl_dx);

	rec_z = 1.0 / z; // gcc 2.95.2 is won't do this optimization itself

	dest = (ubyte *) (write_buffer + fx_xleft + (bytes_per_row * fx_y));
	x = fx_xright - fx_xleft + 1;

	if (!Transparency_on) {
		if (x >= 8) {
			if ((j = (size_t) dest & 7) != 0) {
				j = 8 - j;

				while (j > 0) {
					*dest++ =
					    gr_fade_table[((int) fabs(l)) * 256 +
							  (uint) pixptr[(((int) (v * rec_z)) & (64 * 63)) +
									(((int) (u * rec_z)) & 63)]];
					l += dldx;
					u += dudx;
					v += dvdx;
					z += dzdx;
					rec_z = 1.0 / z;
					x--;
					j--;
				}
			}

			j = x;
			while (j >= 8) {
				destlong =
				    (u_int64_t) gr_fade_table[((int) fabs(l)) * 256 +
							      (uint) pixptr[(((int) (v * rec_z)) & (64 * 63)) +
									    (((int) (u * rec_z)) & 63)]];
				l += dldx;
				u += dudx;
				v += dvdx;
				z += dzdx;
				rec_z = 1.0 / z;
				destlong |=
				    (u_int64_t) gr_fade_table[((int) fabs(l)) * 256 +
							      (uint) pixptr[(((int) (v * rec_z)) & (64 * 63)) +
									    (((int) (u * rec_z)) & 63)]] << 8;
				l += dldx;
				u += dudx;
				v += dvdx;
				z += dzdx;
				rec_z = 1.0 / z;
				destlong |=
				    (u_int64_t) gr_fade_table[((int) fabs(l)) * 256 +
							      (uint) pixptr[(((int) (v * rec_z)) & (64 * 63)) +
									    (((int) (u * rec_z)) & 63)]] << 16;
				l += dldx;
				u += dudx;
				v += dvdx;
				z += dzdx;
				rec_z = 1.0 / z;
				destlong |=
				    (u_int64_t) gr_fade_table[((int) fabs(l)) * 256 +
							      (uint) pixptr[(((int) (v * rec_z)) & (64 * 63)) +
									    (((int) (u * rec_z)) & 63)]] << 24;
				l += dldx;
				u += dudx;
				v += dvdx;
				z += dzdx;
				rec_z = 1.0 / z;
				destlong |=
				    (u_int64_t) gr_fade_table[((int) fabs(l)) * 256 +
							      (uint) pixptr[(((int) (v * rec_z)) & (64 * 63)) +
									    (((int) (u * rec_z)) & 63)]] << 32;
				l += dldx;
				u += dudx;
				v += dvdx;
				z += dzdx;
				rec_z = 1.0 / z;
				destlong |=
				    (u_int64_t) gr_fade_table[((int) fabs(l)) * 256 +
							      (uint) pixptr[(((int) (v * rec_z)) & (64 * 63)) +
									    (((int) (u * rec_z)) & 63)]] << 40;
				l += dldx;
				u += dudx;
				v += dvdx;
				z += dzdx;
				rec_z = 1.0 / z;
				destlong |=
				    (u_int64_t) gr_fade_table[((int) fabs(l)) * 256 +
							      (uint) pixptr[(((int) (v * rec_z)) & (64 * 63)) +
									    (((int) (u * rec_z)) & 63)]] << 48;
				l += dldx;
				u += dudx;
				v += dvdx;
				z += dzdx;
				rec_z = 1.0 / z;
				destlong |=
				    (u_int64_t) gr_fade_table[((int) fabs(l)) * 256 +
							      (uint) pixptr[(((int) (v * rec_z)) & (64 * 63)) +
									    (((int) (u * rec_z)) & 63)]] << 56;
				l += dldx;
				u += dudx;
				v += dvdx;
				z += dzdx;
				rec_z = 1.0 / z;

				*((u_int64_t *) dest) = destlong;
				dest += 8;
				x -= 8;
				j -= 8;
			}
		}
		while (x-- > 0) {
			*dest++ =
			    gr_fade_table[((int) fabs(l)) * 256 +
					  (uint) pixptr[(((int) (v * rec_z)) & (64 * 63)) + (((int) (u * rec_z)) & 63)]];
			l += dldx;
			u += dudx;
			v += dvdx;
			z += dzdx;
			rec_z = 1.0 / z;
		}
	} else {
		if (x >= 8) {
			if ((j = (size_t) dest & 7) != 0) {
				j = 8 - j;

				while (j > 0) {
					c = (uint) pixptr[(((int) (v * rec_z)) & (64 * 63)) + (((int) (u * rec_z)) & 63)];
					if (c != 255)
						*dest = gr_fade_table[((int) fabs(l)) * 256 + c];
					dest++;
					l += dldx;
					u += dudx;
					v += dvdx;
					z += dzdx;
					rec_z = 1.0 / z;
					x--;
					j--;
				}
			}

			j = x;
			while (j >= 8) {
				destlong = *((u_int64_t *) dest);
				c = (uint) pixptr[(((int) (v * rec_z)) & (64 * 63)) + (((int) (u * rec_z)) & 63)];
				if (c != 255) {
					destlong &= ~(u_int64_t)0xFF;
					destlong |= (u_int64_t) gr_fade_table[((int) fabs(l)) * 256 + c];
				}
				l += dldx;
				u += dudx;
				v += dvdx;
				z += dzdx;
				rec_z = 1.0 / z;
				c = (uint) pixptr[(((int) (v * rec_z)) & (64 * 63)) + (((int) (u * rec_z)) & 63)];
				if (c != 255) {
					destlong &= ~((u_int64_t)0xFF << 8);
					destlong |= (u_int64_t) gr_fade_table[((int) fabs(l)) * 256 + c] << 8;
				}
				l += dldx;
				u += dudx;
				v += dvdx;
				z += dzdx;
				rec_z = 1.0 / z;
				c = (uint) pixptr[(((int) (v * rec_z)) & (64 * 63)) + (((int) (u * rec_z)) & 63)];
				if (c != 255) {
					destlong &= ~((u_int64_t)0xFF << 16);
					destlong |= (u_int64_t) gr_fade_table[((int) fabs(l)) * 256 + c] << 16;
				}
				l += dldx;
				u += dudx;
				v += dvdx;
				z += dzdx;
				rec_z = 1.0 / z;
				c = (uint) pixptr[(((int) (v * rec_z)) & (64 * 63)) + (((int) (u * rec_z)) & 63)];
				if (c != 255) {
					destlong &= ~((u_int64_t)0xFF << 24);
					destlong |= (u_int64_t) gr_fade_table[((int) fabs(l)) * 256 + c] << 24;
				}
				l += dldx;
				u += dudx;
				v += dvdx;
				z += dzdx;
				rec_z = 1.0 / z;
				c = (uint) pixptr[(((int) (v * rec_z)) & (64 * 63)) + (((int) (u * rec_z)) & 63)];
				if (c != 255) {
					destlong &= ~((u_int64_t)0xFF << 32);
					destlong |= (u_int64_t) gr_fade_table[((int) fabs(l)) * 256 + c] << 32;
				}
				l += dldx;
				u += dudx;
				v += dvdx;
				z += dzdx;
				rec_z = 1.0 / z;
				c = (uint) pixptr[(((int) (v * rec_z)) & (64 * 63)) + (((int) (u * rec_z)) & 63)];
				if (c != 255) {
					destlong &= ~((u_int64_t)0xFF << 40);
					destlong |= (u_int64_t) gr_fade_table[((int) fabs(l)) * 256 + c] << 40;
				}
				l += dldx;
				u += dudx;
				v += dvdx;
				z += dzdx;
				rec_z = 1.0 / z;
				c = (uint) pixptr[(((int) (v * rec_z)) & (64 * 63)) + (((int) (u * rec_z)) & 63)];
				if (c != 255) {
					destlong &= ~((u_int64_t)0xFF << 48);
					destlong |= (u_int64_t) gr_fade_table[((int) fabs(l)) * 256 + c] << 48;
				}
				l += dldx;
				u += dudx;
				v += dvdx;
				z += dzdx;
				rec_z = 1.0 / z;
				c = (uint) pixptr[(((int) (v * rec_z)) & (64 * 63)) + (((int) (u * rec_z)) & 63)];
				if (c != 255) {
					destlong &= ~((u_int64_t)0xFF << 56);
					destlong |= (u_int64_t) gr_fade_table[((int) fabs(l)) * 256 + c] << 56;
				}
				l += dldx;
				u += dudx;
				v += dvdx;
				z += dzdx;
				rec_z = 1.0 / z;

				*((u_int64_t *) dest) = destlong;
				dest += 8;
				x -= 8;
				j -= 8;
			}
		}
		while (x-- > 0) {
			c = (uint) pixptr[(((int) (v * rec_z)) & (64 * 63)) + (((int) (u * rec_z)) & 63)];
			if (c != 255)
				*dest = gr_fade_table[((int) fabs(l)) * 256 + c];
			dest++;
			l += dldx;
			u += dudx;
			v += dvdx;
			z += dzdx;
			rec_z = 1.0 / z;
		}
	}
}

#if 1
// note the unrolling loop is broken. It is never called, and uses big endian. -- FH
void c_tmap_scanline_per()
{
	ubyte *dest;
	uint c;
	int x, j;
	fix l,u,v,z;
	fix dudx, dvdx, dzdx, dldx;

	u = fx_u;
	v = fx_v*64;
	z = fx_z;
	dudx = fx_du_dx; 
	dvdx = fx_dv_dx*64; 
	dzdx = fx_dz_dx;

	l = fx_l>>8;
	dldx = fx_dl_dx>>8;
	dest = (ubyte *)(write_buffer + fx_xleft + (bytes_per_row * fx_y)  );

	if (!Transparency_on)	{
		ubyte*			pixPtrLocalCopy = pixptr;
		ubyte*			fadeTableLocalCopy = gr_fade_table;
		unsigned long	destlong;

		x = fx_xright-fx_xleft+1;

		if ((j = (unsigned long) dest & 3) != 0)
			{
			j = 4 - j;

			if (j > x)
				j = x;

			while (j > 0)
				{	
				//edited 05/18/99 Matt Mueller - changed from 0xff00 to 0x7f00 to fix glitches
				*dest++ = fadeTableLocalCopy[ (l&(0x7f00)) + (uint)pixPtrLocalCopy[ ( (v/z)&(64*63) ) + ((u/z)&63) ] ];
				//end edit -MM
				l += dldx;
				u += dudx;
				v += dvdx;
				z += dzdx;
				x--;
				j--;
				}
			}

		j &= ~3;
		while (j > 0)
			{
			//edited 05/18/99 Matt Mueller - changed from 0xff00 to 0x7f00 to fix glitches
			destlong = (unsigned long) fadeTableLocalCopy[ (l&(0x7f00)) + (uint)pixPtrLocalCopy[ ( (v/z)&(64*63) ) + ((u/z)&63) ] ] << 24;
			//end edit -MM
			l += dldx;
			u += dudx;
			v += dvdx;
			z += dzdx;
			//edited 05/18/99 Matt Mueller - changed from 0xff00 to 0x7f00 to fix glitches
			destlong |= (unsigned long) fadeTableLocalCopy[ (l&(0x7f00)) + (uint)pixPtrLocalCopy[ ( (v/z)&(64*63) ) + ((u/z)&63) ] ] << 16;
			//end edit -MM
			l += dldx;
			u += dudx;
			v += dvdx;
			z += dzdx;
			//edited 05/18/99 Matt Mueller - changed from 0xff00 to 0x7f00 to fix glitches
			destlong |= (unsigned long) fadeTableLocalCopy[ (l&(0x7f00)) + (uint)pixPtrLocalCopy[ ( (v/z)&(64*63) ) + ((u/z)&63) ] ] << 8;
			//end edit -MM
			l += dldx;
			u += dudx;
			v += dvdx;
			z += dzdx;
			//edited 05/18/99 Matt Mueller - changed from 0xff00 to 0x7f00 to fix glitches
			destlong |= (unsigned long) fadeTableLocalCopy[ (l&(0x7f00)) + (uint)pixPtrLocalCopy[ ( (v/z)&(64*63) ) + ((u/z)&63) ] ];
			//end edit -MM
			l += dldx;
			u += dudx;
			v += dvdx;
			z += dzdx;
			*((unsigned long *) dest) = destlong;
			dest += 4;
			x -= 4;
			j -= 4;
			}

		while (x-- > 0)
			{
			//edited 05/18/99 Matt Mueller - changed from 0xff00 to 0x7f00 to fix glitches
			*dest++ = (unsigned long) fadeTableLocalCopy[ (l&(0x7f00)) + (uint)pixPtrLocalCopy[ ( (v/z)&(64*63) ) + ((u/z)&63) ] ];
			//end edit -MM
			l += dldx;
			u += dudx;
			v += dvdx;
			z += dzdx;
			}

	} else {
		for (x= fx_xright-fx_xleft+1 ; x > 0; --x ) {
			c = (uint)pixptr[ ( (v/z)&(64*63) ) + ((u/z)&63) ];
			if ( c!=TRANSPARENCY_COLOR)
			//edited 05/18/99 Matt Mueller - changed from 0xff00 to 0x7f00 to fix glitches
				*dest = gr_fade_table[ (l&(0x7f00)) + c ];
			//end edit -MM
			dest++;
			l += dldx;
			u += dudx;
			v += dvdx;
			z += dzdx;
		}
	}
}

#else
void c_tmap_scanline_per()
{
	ubyte *dest;
	uint c;
	int x;
	fix u,v,z,l,dudx, dvdx, dzdx, dldx;

	u = fx_u;
	v = fx_v*64;
	z = fx_z;
	dudx = fx_du_dx; 
	dvdx = fx_dv_dx*64; 
	dzdx = fx_dz_dx;

	l = fx_l>>8;
	dldx = fx_dl_dx>>8;
	dest = (ubyte *)(write_buffer + fx_xleft + (bytes_per_row * fx_y)  );

	if (!Transparency_on)	{
		for (x= fx_xright-fx_xleft+1 ; x > 0; --x ) {
			//edited 05/18/99 Matt Mueller - changed from 0xff00 to 0x7f00 to fix glitches
			*dest++ = gr_fade_table[ (l&(0x7f00)) + (uint)pixptr[ ( (v/z)&(64*63) ) + ((u/z)&63) ] ];
			//end edit -MM
			l += dldx;
			u += dudx;
			v += dvdx;
			z += dzdx;
		}
	} else {
		for (x= fx_xright-fx_xleft+1 ; x > 0; --x ) {
			c = (uint)pixptr[ ( (v/z)&(64*63) ) + ((u/z)&63) ];
			if ( c!=255)
			//edited 05/18/99 Matt Mueller - changed from 0xff00 to 0x7f00 to fix glitches
				*dest = gr_fade_table[ (l&(0x7f00)) + c ];
			//end edit -MM
			dest++;
			l += dldx;
			u += dudx;
			v += dvdx;
			z += dzdx;
		}
	}
}

#endif

void (*cur_tmap_scanline_per)(void);
void (*cur_tmap_scanline_per_nolight)(void);
void (*cur_tmap_scanline_lin)(void);
void (*cur_tmap_scanline_lin_nolight)(void);
void (*cur_tmap_scanline_flat)(void);
void (*cur_tmap_scanline_shaded)(void);

//runtime selection of optimized tmappers.  12/07/99  Matthew Mueller
//the reason I did it this way rather than having a *tmap_funcs that then points to a c_tmap or fp_tmap struct thats already filled in, is to avoid a second pointer dereference.
void select_tmap(char *type){
	if (!type){
#ifndef NO_ASM
		select_tmap("i386");
#else
		select_tmap("c");
#endif
		return;
	}
#ifndef NO_ASM
	if (stricmp(type,"i386")==0){
		cur_tmap_scanline_per=asm_tmap_scanline_per;
		cur_tmap_scanline_per_nolight=asm_tmap_scanline_per;
		cur_tmap_scanline_lin=asm_tmap_scanline_lin_lighted;
		cur_tmap_scanline_lin_nolight=asm_tmap_scanline_lin;
		cur_tmap_scanline_flat=asm_tmap_scanline_flat;
		cur_tmap_scanline_shaded=asm_tmap_scanline_shaded;
	}
	else if (stricmp(type,"pent")==0){
		cur_tmap_scanline_per=asm_pent_tmap_scanline_per;
		cur_tmap_scanline_per_nolight=asm_pent_tmap_scanline_per;
		cur_tmap_scanline_lin=asm_tmap_scanline_lin_lighted;
		cur_tmap_scanline_lin_nolight=asm_tmap_scanline_lin;
		cur_tmap_scanline_flat=asm_tmap_scanline_flat;
		cur_tmap_scanline_shaded=asm_tmap_scanline_shaded;
	}
	else if (stricmp(type,"ppro")==0){
		cur_tmap_scanline_per=asm_ppro_tmap_scanline_per;
		cur_tmap_scanline_per_nolight=asm_ppro_tmap_scanline_per;
		cur_tmap_scanline_lin=asm_tmap_scanline_lin_lighted;
		cur_tmap_scanline_lin_nolight=asm_tmap_scanline_lin;
		cur_tmap_scanline_flat=asm_tmap_scanline_flat;
		cur_tmap_scanline_shaded=asm_tmap_scanline_shaded;
	}
	else
#endif
	if (stricmp(type,"fp")==0){
		cur_tmap_scanline_per=c_fp_tmap_scanline_per;
		cur_tmap_scanline_per_nolight=c_fp_tmap_scanline_per_nolight;
		cur_tmap_scanline_lin=c_tmap_scanline_lin;
		cur_tmap_scanline_lin_nolight=c_tmap_scanline_lin_nolight;
		cur_tmap_scanline_flat=c_tmap_scanline_flat;
		cur_tmap_scanline_shaded=c_tmap_scanline_shaded;
	}
	else {
		if (stricmp(type,"c")!=0)
			printf("unknown tmap requested, using c tmap\n");
		cur_tmap_scanline_per=c_tmap_scanline_per;
		cur_tmap_scanline_per_nolight=c_tmap_scanline_per_nolight;
		cur_tmap_scanline_lin=c_tmap_scanline_lin;
		cur_tmap_scanline_lin_nolight=c_tmap_scanline_lin_nolight;
		cur_tmap_scanline_flat=c_tmap_scanline_flat;
		cur_tmap_scanline_shaded=c_tmap_scanline_shaded;
	}
}
