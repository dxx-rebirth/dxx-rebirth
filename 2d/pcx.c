/* $Id: pcx.c,v 1.8 2003-06-16 06:57:34 btb Exp $ */
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
 * Routines to read/write pcx images.
 *
 * Old Log:
 * Revision 1.6  1995/03/01  15:38:12  john
 * Better ModeX support.
 *
 * Revision 1.5  1995/01/21  17:54:17  john
 * Added pcx reader for modes other than modex.
 *
 * Revision 1.4  1994/12/08  19:03:56  john
 * Made functions use cfile.
 *
 * Revision 1.3  1994/11/29  02:53:24  john
 * Added error messages; made call be more similiar to iff.
 *
 * Revision 1.2  1994/11/28  20:03:50  john
 * Added PCX functions.
 *
 * Revision 1.1  1994/11/28  19:57:56  john
 * Initial revision
 *
 */

#ifdef HAVE_CONFIG_H
#include <conf.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gr.h"
#include "grdef.h"
#include "u_mem.h"
#include "pcx.h"
#include "cfile.h"

#ifdef OGL
#include "palette.h"
#endif

#if defined(POLY_ACC)
#include "poly_acc.h"
#endif

int pcx_encode_byte(ubyte byt, ubyte cnt, CFILE *fid);
int pcx_encode_line(ubyte *inBuff, int inLen, CFILE *fp);

/* PCX Header data type */
typedef struct {
	ubyte   Manufacturer;
	ubyte   Version;
	ubyte   Encoding;
	ubyte   BitsPerPixel;
	short   Xmin;
	short   Ymin;
	short   Xmax;
	short   Ymax;
	short   Hdpi;
	short   Vdpi;
	ubyte   ColorMap[16][3];
	ubyte   Reserved;
	ubyte   Nplanes;
	short   BytesPerLine;
	ubyte   filler[60];
} __pack__ PCXHeader;

#define PCXHEADER_SIZE 128

#ifdef FAST_FILE_IO
#define PCXHeader_read_n(ph, n, fp) cfread(ph, sizeof(PCXHeader), n, fp)
#else
/*
 * reads n PCXHeader structs from a CFILE
 */
int PCXHeader_read_n(PCXHeader *ph, int n, CFILE *fp)
{
	int i;

	for (i = 0; i < n; i++) {
		ph->Manufacturer = cfile_read_byte(fp);
		ph->Version = cfile_read_byte(fp);
		ph->Encoding = cfile_read_byte(fp);
		ph->BitsPerPixel = cfile_read_byte(fp);
		ph->Xmin = cfile_read_short(fp);
		ph->Ymin = cfile_read_short(fp);
		ph->Xmax = cfile_read_short(fp);
		ph->Ymax = cfile_read_short(fp);
		ph->Hdpi = cfile_read_short(fp);
		ph->Vdpi = cfile_read_short(fp);
		cfread(&ph->ColorMap, 16*3, 1, fp);
		ph->Reserved = cfile_read_byte(fp);
		ph->Nplanes = cfile_read_byte(fp);
		ph->BytesPerLine = cfile_read_short(fp);
		cfread(&ph->filler, 60, 1, fp);
	}
	return i;
}
#endif

int pcx_get_dimensions( char *filename, int *width, int *height)
{
	CFILE *PCXfile;
	PCXHeader header;

	PCXfile = cfopen(filename, "rb");
	if (!PCXfile) return PCX_ERROR_OPENING;

	if (PCXHeader_read_n(&header, 1, PCXfile) != 1) {
		cfclose(PCXfile);
		return PCX_ERROR_NO_HEADER;
	}
	cfclose(PCXfile);

	*width = header.Xmax - header.Xmin+1;
	*height = header.Ymax - header.Ymin+1;

	return PCX_ERROR_NONE;
}

#ifdef MACINTOSH
int pcx_read_bitmap_palette( char *filename, ubyte *palette)
{
	PCXHeader header;
	CFILE * PCXfile;
	ubyte data;
	int i;

	PCXfile = cfopen( filename , "rb" );
	if ( !PCXfile )
		return PCX_ERROR_OPENING;

	// read 128 char PCX header
	if (PCXHeader_read_n( &header, 1, PCXfile )!=1)	{
		cfclose( PCXfile );
		return PCX_ERROR_NO_HEADER;
	}
	// Is it a 256 color PCX file?
	if ((header.Manufacturer != 10)||(header.Encoding != 1)||(header.Nplanes != 1)||(header.BitsPerPixel != 8)||(header.Version != 5))	{
		cfclose( PCXfile );
		return PCX_ERROR_WRONG_VERSION;
	}

	// Read the extended palette at the end of PCX file
	// Read in a character which should be 12 to be extended palette file

	if (palette != NULL) {
		cfseek( PCXfile, -768, SEEK_END );
		cfread( palette, 3, 256, PCXfile );
		cfseek( PCXfile, PCXHEADER_SIZE, SEEK_SET );
		for (i=0; i<768; i++ )
			palette[i] >>= 2;
#ifdef MACINTOSH
		for (i = 0; i < 3; i++) {
			data = palette[i];
			palette[i] = palette[765+i];
			palette[765+i] = data;
		}
#endif
	}
}
#endif

int pcx_read_bitmap( char * filename, grs_bitmap * bmp,int bitmap_type ,ubyte * palette )
{
	PCXHeader header;
	CFILE * PCXfile;
	int i, row, col, count, xsize, ysize;
	ubyte data, *pixdata;
#if defined(POLY_ACC)
    unsigned char local_pal[768];

    pa_flush();
#endif

	PCXfile = cfopen( filename , "rb" );
	if ( !PCXfile )
		return PCX_ERROR_OPENING;

	// read 128 char PCX header
	if (PCXHeader_read_n( &header, 1, PCXfile )!=1) {
		cfclose( PCXfile );
		return PCX_ERROR_NO_HEADER;
	}

	// Is it a 256 color PCX file?
	if ((header.Manufacturer != 10)||(header.Encoding != 1)||(header.Nplanes != 1)||(header.BitsPerPixel != 8)||(header.Version != 5))	{
		cfclose( PCXfile );
		return PCX_ERROR_WRONG_VERSION;
	}

	// Find the size of the image
	xsize = header.Xmax - header.Xmin + 1;
	ysize = header.Ymax - header.Ymin + 1;

#if defined(POLY_ACC)
    // Read the extended palette at the end of PCX file
    if(bitmap_type == BM_LINEAR15)      // need palette for conversion from 8bit pcx to 15bit.
    {
        cfseek( PCXfile, -768, SEEK_END );
        cfread( local_pal, 3, 256, PCXfile );
        cfseek( PCXfile, PCXHEADER_SIZE, SEEK_SET );
        for (i=0; i<768; i++ )
            local_pal[i] >>= 2;
        pa_save_clut();
        pa_update_clut(local_pal, 0, 256, 0);
    }
#endif

	if ( bitmap_type == BM_LINEAR )	{
		if ( bmp->bm_data == NULL )	{
			gr_init_bitmap_alloc (bmp, bitmap_type, 0, 0, xsize, ysize, xsize);
		}
	}

	if ( bmp->bm_type == BM_LINEAR )	{
		for (row=0; row< ysize ; row++)      {
			pixdata = &bmp->bm_data[bmp->bm_rowsize*row];
			for (col=0; col< xsize ; )      {
				if (cfread( &data, 1, 1, PCXfile )!=1 )	{
					cfclose( PCXfile );
					return PCX_ERROR_READING;
				}
				if ((data & 0xC0) == 0xC0)     {
					count =  data & 0x3F;
					if (cfread( &data, 1, 1, PCXfile )!=1 )	{
						cfclose( PCXfile );
						return PCX_ERROR_READING;
					}
#ifdef MACINTOSH
					if (data == 0)
						data = 255;
					else if (data == 255)
						data = 0;
#endif
					memset( pixdata, data, count );
					pixdata += count;
					col += count;
				} else {
#ifdef MACINTOSH
					if (data == 0)
						data = 255;
					else if (data == 255)
						data = 0;
#endif
					*pixdata++ = data;
					col++;
				}
			}
		}
#if defined(POLY_ACC)
    } else if( bmp->bm_type == BM_LINEAR15 )    {
        ushort *pixdata2, pix15;
        PA_DFX (pa_set_backbuffer_current());
		  PA_DFX (pa_set_write_mode(0));
		for (row=0; row< ysize ; row++)      {
            pixdata2 = (ushort *)&bmp->bm_data[bmp->bm_rowsize*row];
			for (col=0; col< xsize ; )      {
				if (cfread( &data, 1, 1, PCXfile )!=1 )	{
					cfclose( PCXfile );
					return PCX_ERROR_READING;
				}
				if ((data & 0xC0) == 0xC0)     {
					count =  data & 0x3F;
					if (cfread( &data, 1, 1, PCXfile )!=1 )	{
						cfclose( PCXfile );
						return PCX_ERROR_READING;
					}
                    pix15 = pa_clut[data];
                    for(i = 0; i != count; ++i) pixdata2[i] = pix15;
                    pixdata2 += count;
					col += count;
				} else {
                    *pixdata2++ = pa_clut[data];
					col++;
				}
			}
        }
        pa_restore_clut();
		  PA_DFX (pa_swap_buffer());
        PA_DFX (pa_set_frontbuffer_current());

#endif
	} else {
		for (row=0; row< ysize ; row++)      {
			for (col=0; col< xsize ; )      {
				if (cfread( &data, 1, 1, PCXfile )!=1 )	{
					cfclose( PCXfile );
					return PCX_ERROR_READING;
				}
				if ((data & 0xC0) == 0xC0)     {
					count =  data & 0x3F;
					if (cfread( &data, 1, 1, PCXfile )!=1 )	{
						cfclose( PCXfile );
						return PCX_ERROR_READING;
					}
					for (i=0;i<count;i++)
						gr_bm_pixel( bmp, col+i, row, data );
					col += count;
				} else {
					gr_bm_pixel( bmp, col, row, data );
					col++;
				}
			}
		}
	}

	// Read the extended palette at the end of PCX file
	if ( palette != NULL )	{
		// Read in a character which should be 12 to be extended palette file
		if (cfread( &data, 1, 1, PCXfile )==1)	{
			if ( data == 12 )	{
				if (cfread(palette,768, 1, PCXfile)!=1)	{
					cfclose( PCXfile );
					return PCX_ERROR_READING;
				}
				for (i=0; i<768; i++ )
					palette[i] >>= 2;
#ifdef MACINTOSH
				for (i = 0; i < 3; i++) {
					data = palette[i];
					palette[i] = palette[765+i];
					palette[765+i] = data;
				}
#endif
			}
		} else {
			cfclose( PCXfile );
			return PCX_ERROR_NO_PALETTE;
		}
	}
	cfclose(PCXfile);
	return PCX_ERROR_NONE;
}

int pcx_write_bitmap( char * filename, grs_bitmap * bmp, ubyte * palette )
{
	int retval;
	int i;
	ubyte data;
	PCXHeader header;
	CFILE *PCXfile;

	memset( &header, 0, PCXHEADER_SIZE );

	header.Manufacturer = 10;
	header.Encoding = 1;
	header.Nplanes = 1;
	header.BitsPerPixel = 8;
	header.Version = 5;
	header.Xmax = bmp->bm_w-1;
	header.Ymax = bmp->bm_h-1;
	header.BytesPerLine = bmp->bm_w;

	PCXfile = cfopen(filename, "wb");
	if ( !PCXfile )
		return PCX_ERROR_OPENING;

	if (cfwrite(&header, PCXHEADER_SIZE, 1, PCXfile) != 1)
	{
		cfclose(PCXfile);
		return PCX_ERROR_WRITING;
	}

	for (i=0; i<bmp->bm_h; i++ )	{
		if (!pcx_encode_line( &bmp->bm_data[bmp->bm_rowsize*i], bmp->bm_w, PCXfile ))	{
			cfclose(PCXfile);
			return PCX_ERROR_WRITING;
		}
	}

	// Mark an extended palette
	data = 12;
	if (cfwrite(&data, 1, 1, PCXfile) != 1)
	{
		cfclose(PCXfile);
		return PCX_ERROR_WRITING;
	}

	// Write the extended palette
	for (i=0; i<768; i++ )
		palette[i] <<= 2;

	retval = cfwrite(palette, 768, 1, PCXfile);

	for (i=0; i<768; i++ )
		palette[i] >>= 2;

	if (retval !=1)	{
		cfclose(PCXfile);
		return PCX_ERROR_WRITING;
	}

	cfclose(PCXfile);
	return PCX_ERROR_NONE;

}

// returns number of bytes written into outBuff, 0 if failed
int pcx_encode_line(ubyte *inBuff, int inLen, CFILE *fp)
{
	ubyte this, last;
	int srcIndex, i;
	register int total;
	register ubyte runCount; 	// max single runlength is 63
	total = 0;
	last = *(inBuff);
	runCount = 1;

	for (srcIndex = 1; srcIndex < inLen; srcIndex++) {
		this = *(++inBuff);
		if (this == last)	{
			runCount++;			// it encodes
			if (runCount == 63)	{
				if (!(i=pcx_encode_byte(last, runCount, fp)))
					return(0);
				total += i;
				runCount = 0;
			}
		} else {   	// this != last
			if (runCount)	{
				if (!(i=pcx_encode_byte(last, runCount, fp)))
					return(0);
				total += i;
			}
			last = this;
			runCount = 1;
		}
	}

	if (runCount)	{		// finish up
		if (!(i=pcx_encode_byte(last, runCount, fp)))
			return 0;
		return total + i;
	}
	return total;
}

// subroutine for writing an encoded byte pair
// returns count of bytes written, 0 if error
int pcx_encode_byte(ubyte byt, ubyte cnt, CFILE *fid)
{
	if (cnt) {
		if ( (cnt==1) && (0xc0 != (0xc0 & byt)) )	{
			if(EOF == cfputc((int)byt, fid))
				return 0; 	// disk write error (probably full)
			return 1;
		} else {
			if(EOF == cfputc((int)0xC0 | cnt, fid))
				return 0; 	// disk write error
			if(EOF == cfputc((int)byt, fid))
				return 0; 	// disk write error
			return 2;
		}
	}
	return 0;
}

//text for error messges
char pcx_error_messages[] = {
	"No error.\0"
	"Error opening file.\0"
	"Couldn't read PCX header.\0"
	"Unsupported PCX version.\0"
	"Error reading data.\0"
	"Couldn't find palette information.\0"
	"Error writing data.\0"
};


//function to return pointer to error message
char *pcx_errormsg(int error_number)
{
	char *p = pcx_error_messages;

	while (error_number--) {

		if (!p) return NULL;

		p += strlen(p)+1;

	}

	return p;

}

// fullscreen loading, 10/14/99 Jan Bobrowski

int pcx_read_fullscr(char * filename, ubyte * palette)
{
	int pcx_error;
	grs_bitmap bm;
	gr_init_bitmap_data(&bm);
	pcx_error = pcx_read_bitmap(filename, &bm, BM_LINEAR, palette);
	if (pcx_error == PCX_ERROR_NONE) {
#ifdef OGL
		gr_palette_load(palette);
#endif
		show_fullscr(&bm);
	}
	gr_free_bitmap_data(&bm);
	return pcx_error;
}
