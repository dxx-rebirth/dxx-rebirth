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
 * Routines to draw the texture mapped scanlines.
 *
 */

#include <math.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "maths.h"
#include "gr.h"
#include "grdef.h"
#include "texmap.h"
#include "texmapl.h"
#include "scanline.h"
#include "strutil.h"
#include "dxxerror.h"

void c_tmap_scanline_flat()
{
	ubyte *dest;
        int x, index = fx_xleft + (bytes_per_row * fx_y );

	dest = (ubyte *)(write_buffer + fx_xleft + (bytes_per_row * fx_y )  );

	for (x= fx_xright-fx_xleft+1 ; x > 0; --x ) {
		if (++index >= SWIDTH*SHEIGHT) return;
		*dest++ = tmap_flat_color;
	}
//         memset(dest,tmap_flat_color,fx_xright-fx_xleft+1);
}

void c_tmap_scanline_shaded()
{
	int fade;
	ubyte *dest, tmp;
	int x, index = fx_xleft + (bytes_per_row * fx_y );

	dest = (ubyte *)(write_buffer + fx_xleft + (bytes_per_row * fx_y)  );

	fade = tmap_flat_shade_value<<8;
	for (x= fx_xright-fx_xleft+1 ; x > 0; --x ) {
		if (++index >= SWIDTH*SHEIGHT) return;
		tmp = *dest;
		*dest++ = gr_fade_table[ fade |(tmp)];
	}
}

void c_tmap_scanline_lin_nolight()
{
	ubyte *dest;
	uint c;
	int x, index = fx_xleft + (bytes_per_row * fx_y );
	fix u,v,dudx, dvdx;

	u = fx_u;
	v = fx_v*64;
	dudx = fx_du_dx; 
	dvdx = fx_dv_dx*64; 

	dest = (ubyte *)(write_buffer + fx_xleft + (bytes_per_row * fx_y)  );

	if (!Transparency_on)	{
		for (x= fx_xright-fx_xleft+1 ; x > 0; --x ) {
			if (++index >= SWIDTH*SHEIGHT) return;
			*dest++ = (uint)pixptr[ (f2i(v)&(64*63)) + (f2i(u)&63) ];
			u += dudx;
			v += dvdx;
		}
	} else {
		for (x= fx_xright-fx_xleft+1 ; x > 0; --x ) {
			if (++index >= SWIDTH*SHEIGHT) return;
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
	int x, j, index = fx_xleft + (bytes_per_row * fx_y);
	fix u,v,l,dudx, dvdx, dldx;

	u = fx_u;
	v = fx_v*64;
	dudx = fx_du_dx; 
	dvdx = fx_dv_dx*64; 

	l = fx_l>>8;
	dldx = fx_dl_dx/256;
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
				if (++index >= SWIDTH*SHEIGHT) return;
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
			index += 4;
			if (index+4 >= SWIDTH*SHEIGHT) return;
			}

		while (x-- > 0)
			{
			if (++index >= SWIDTH*SHEIGHT) return;
			//edited 05/18/99 Matt Mueller - changed from 0xff00 to 0x7f00 to fix glitches
			*dest++ = (unsigned long) fadeTableLocalCopy[ (l&(0x7f00)) + (uint) pixPtrLocalCopy[ (f2i(v)&(64*63)) + (f2i(u)&63) ] ];
			//end edit -MM
			l += dldx;
			u += dudx;
			v += dvdx;
			}

	} else {
		for (x= fx_xright-fx_xleft+1 ; x > 0; --x ) {
			if (++index >= SWIDTH*SHEIGHT) return;
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
	dldx = fx_dl_dx/256;
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
	int		x, j, index = fx_xleft + (bytes_per_row * fx_y);
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
					if (++index >= SWIDTH*SHEIGHT) return;
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
				index +=8;
				if (index+8 >= SWIDTH*SHEIGHT) return;
			}
		}
		while (x-- > 0) {
			if (++index >= SWIDTH*SHEIGHT) return;
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
					if (++index >= SWIDTH*SHEIGHT) return;
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
				index += 8;
				if (index+8 >= SWIDTH*SHEIGHT) return;
			}
		}
		while (x-- > 0) {
			if (++index >= SWIDTH*SHEIGHT) return;
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
	int x, index = fx_xleft + (bytes_per_row * fx_y);
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
			if (++index >= SWIDTH*SHEIGHT) return;
			*dest++ = (uint)pixptr[ ( (v/z)&(64*63) ) + ((u/z)&63) ];
			u += dudx;
			v += dvdx;
			z += dzdx;
		}
	} else {
		for (x= fx_xright-fx_xleft+1 ; x > 0; --x ) {
			if (++index >= SWIDTH*SHEIGHT) return;
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
	int             x, j, index = fx_xleft + (bytes_per_row * fx_y);
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
					if (++index >= SWIDTH*SHEIGHT) return;
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
				index += 8;
				if (index+8 >= SWIDTH*SHEIGHT) return;
			}
		}
		while (x-- > 0) {
			if (++index >= SWIDTH*SHEIGHT) return;
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
					if (++index >= SWIDTH*SHEIGHT) return;
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
				index += 8;
				if (index+8 >= SWIDTH*SHEIGHT) return;
			}
		}
		while (x-- > 0) {
			if (++index >= SWIDTH*SHEIGHT) return;
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
	int x, j, index = fx_xleft + (bytes_per_row * fx_y);
	fix l,u,v,z;
	fix dudx, dvdx, dzdx, dldx;

	u = fx_u;
	v = fx_v*64;
	z = fx_z;
	dudx = fx_du_dx; 
	dvdx = fx_dv_dx*64; 
	dzdx = fx_dz_dx;

	l = fx_l>>8;
	dldx = fx_dl_dx/256;
	dest = (ubyte *)(write_buffer + fx_xleft + (bytes_per_row * fx_y)  );

	if (!Transparency_on)	{
		ubyte*			pixPtrLocalCopy = pixptr;
		ubyte*			fadeTableLocalCopy = gr_fade_table;
		unsigned long	destlong;

		x = fx_xright-fx_xleft+1; // x = number of pixels in scanline

		if ((j = (unsigned long) dest & 3) != 0)
			{
			j = 4 - j;

			if (j > x)
				j = x;

			while (j > 0)
				{	
				if (++index >= SWIDTH*SHEIGHT) return;
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
			index += 4;
			if (index+4 >= SWIDTH*SHEIGHT) return;
			}

		while (x-- > 0)
			{
			if (++index >= SWIDTH*SHEIGHT) return;
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
			if (++index >= SWIDTH*SHEIGHT) return;
			c = (uint)pixptr[ ( (v/z)&(64*63) ) + ((u/z)&63) ];
			if ( c!=TRANSPARENCY_COLOR)
				*dest = gr_fade_table[ (l&(0x7f00)) + c ];
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
	dldx = fx_dl_dx/256;
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

void c_tmap_scanline_quad()
{
	ubyte *dest;
	uint c;
	int x, index = fx_xleft + (bytes_per_row * fx_y);
	fix u,v,l,dudx, dvdx, dldx;

	// Quadratic setup stuff:
	fix a1, a2, b1, b2, dudx1, dvdx1;
	fix u0 = fx_u;
	fix u2 = fx_u + fx_du_dx*(fx_xright-fx_xleft+1);	// This just needs to be uright from outer loop
	fix v0 = fx_v;
	fix v2 = fx_v + fx_dv_dx*(fx_xright-fx_xleft+1);	// This just needs to be vright from outer loop
	fix w0 = fx_z;
	fix w2 = fx_z + fx_dz_dx*(fx_xright-fx_xleft+1);	// This just needs to be zright from outer loop
	fix u1 = fixdiv((u0+u2),(w0+w2));						
	fix v1 = fixdiv((v0+v2),(w0+w2));
	int dx = fx_xright-fx_xleft+1;		
	u0 = fixdiv( u0, w0 );	// Divide Z out.  This should be in outer loop
	v0 = fixdiv( v0, w0 );	// Divide Z out.  This should be in outer loop
	u2 = fixdiv( u2, w2 );	// Divide Z out.  This should be in outer loop
	v2 = fixdiv( v2, w2 );	// Divide Z out.  This should be in outer loop
	a1 = (-3*u0+4*u1-u2)/dx;			
	b1 = (-3*v0+4*v1-v2)/dx;
	a2 = (2*(u0-2*u1+u2))/(dx*dx);	
	b2 = (2*(v0-2*v1+v2))/(dx*dx);
	dudx = a1 + a2;
	dvdx = b1 + b2;
	dudx1 = 2*a2;
	dvdx1 = 2*b2;
	u = u0;
	v = v0;

	// Normal lighting setup
	l = fx_l>>8;
	dldx = fx_dl_dx>>8;
	
	// Normal destination pointer setup
	dest = (ubyte *)(write_buffer + fx_xleft + (bytes_per_row * fx_y)  );

	if (!Transparency_on)	{
		for (x= fx_xright-fx_xleft+1 ; x > 0; --x ) {
			if (++index >= SWIDTH*SHEIGHT) return;
			*dest++ = gr_fade_table[ (l&(0xff00)) + (uint)pixptr[  (f2i(v)&63)*64 + (f2i(u)&63) ] ];
			l += dldx;
			u += dudx;
			v += dvdx;
			dudx += dudx1;		// Extra add for quadratic!
			dvdx += dvdx1;		// Extra add for quadratic!
		}
	} else {
		for (x= fx_xright-fx_xleft+1 ; x > 0; --x ) {
			if (++index >= SWIDTH*SHEIGHT) return;
			c = (uint)pixptr[  (f2i(v)&63)*64 + (f2i(u)&63) ];
			if ( c!=255)
				*dest = gr_fade_table[ (l&(0xff00)) + c ];
			dest++;
			l += dldx;
			u += dudx;
			v += dvdx;
			dudx += dudx1;		// Extra add for quadratic!
			dvdx += dvdx1;		// Extra add for quadratic!
		}
	}
}

void (*cur_tmap_scanline_per)(void);
void (*cur_tmap_scanline_per_nolight)(void);
void (*cur_tmap_scanline_lin)(void);
void (*cur_tmap_scanline_lin_nolight)(void);
void (*cur_tmap_scanline_flat)(void);
void (*cur_tmap_scanline_shaded)(void);

//runtime selection of optimized tmappers.  12/07/99  Matthew Mueller
//the reason I did it this way rather than having a *tmap_funcs that then points to a c_tmap or fp_tmap struct thats already filled in, is to avoid a second pointer dereference.
void select_tmap(const char *type)
{
	if (!type)
	{
#if !defined(NO_ASM) && !defined(OGL)
		select_tmap("i386");
#elif defined(macintosh) && !defined(OGL)
		select_tmap("ppc");
#else
		select_tmap("c");
#endif
		return;
	}
#if !defined(NO_ASM) && !defined(OGL)
	if (d_stricmp(type, "i386")==0)
	{
		cur_tmap_scanline_per=asm_tmap_scanline_per;
		cur_tmap_scanline_per_nolight=asm_tmap_scanline_per;
		cur_tmap_scanline_lin=asm_tmap_scanline_lin_lighted;
		cur_tmap_scanline_lin_nolight=asm_tmap_scanline_lin;
		cur_tmap_scanline_flat=asm_tmap_scanline_flat;
		cur_tmap_scanline_shaded=asm_tmap_scanline_shaded;
	}
	else
#elif defined(macintosh) && !defined(OGL)
	if (d_stricmp(type,"ppc")==0){
		cur_tmap_scanline_per=asm_tmap_scanline_per;
		cur_tmap_scanline_per_nolight=asm_tmap_scanline_per;
		cur_tmap_scanline_lin=c_tmap_scanline_lin;
		cur_tmap_scanline_lin_nolight=c_tmap_scanline_lin_nolight;
		cur_tmap_scanline_flat=c_tmap_scanline_flat;
		cur_tmap_scanline_shaded=c_tmap_scanline_shaded;
	}
	else
#endif
	if (d_stricmp(type,"fp")==0){
		cur_tmap_scanline_per=c_fp_tmap_scanline_per;
		cur_tmap_scanline_per_nolight=c_fp_tmap_scanline_per_nolight;
		cur_tmap_scanline_lin=c_tmap_scanline_lin;
		cur_tmap_scanline_lin_nolight=c_tmap_scanline_lin_nolight;
		cur_tmap_scanline_flat=c_tmap_scanline_flat;
		cur_tmap_scanline_shaded=c_tmap_scanline_shaded;
	}
	else if (d_stricmp(type,"quad")==0){
		cur_tmap_scanline_per=c_tmap_scanline_quad;
		cur_tmap_scanline_per_nolight=c_tmap_scanline_per_nolight;
		cur_tmap_scanline_lin=c_tmap_scanline_lin;
		cur_tmap_scanline_lin_nolight=c_tmap_scanline_lin_nolight;
		cur_tmap_scanline_flat=c_tmap_scanline_flat;
		cur_tmap_scanline_shaded=c_tmap_scanline_shaded;
	}
	else {
		cur_tmap_scanline_per=c_tmap_scanline_per;
		cur_tmap_scanline_per_nolight=c_tmap_scanline_per_nolight;
		cur_tmap_scanline_lin=c_tmap_scanline_lin;
		cur_tmap_scanline_lin_nolight=c_tmap_scanline_lin_nolight;
		cur_tmap_scanline_flat=c_tmap_scanline_flat;
		cur_tmap_scanline_shaded=c_tmap_scanline_shaded;
	}
}
