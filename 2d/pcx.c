/* $Id: pcx.c,v 1.1.1.1 2006/03/17 19:51:57 zicodxx Exp $ */
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
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gr.h"
#include "grdef.h"
#include "u_mem.h"
#include "pcx.h"
#include "physfsx.h"


int pcx_encode_byte(ubyte byt, ubyte cnt, PHYSFS_file *fid);
int pcx_encode_line(ubyte *inBuff, int inLen, PHYSFS_file *fp);

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

/*
 * reads n PCXHeader structs from a PHYSFS_file
 */
int PCXHeader_read_n(PCXHeader *ph, int n, PHYSFS_file *fp)
{
	int i;

	for (i = 0; i < n; i++) {
		ph->Manufacturer = PHYSFSX_readByte(fp);
		ph->Version = PHYSFSX_readByte(fp);
		ph->Encoding = PHYSFSX_readByte(fp);
		ph->BitsPerPixel = PHYSFSX_readByte(fp);
		ph->Xmin = PHYSFSX_readShort(fp);
		ph->Ymin = PHYSFSX_readShort(fp);
		ph->Xmax = PHYSFSX_readShort(fp);
		ph->Ymax = PHYSFSX_readShort(fp);
		ph->Hdpi = PHYSFSX_readShort(fp);
		ph->Vdpi = PHYSFSX_readShort(fp);
		PHYSFS_read( fp, &ph->ColorMap, 16*3, 1 );
		ph->Reserved = PHYSFSX_readByte(fp);
		ph->Nplanes = PHYSFSX_readByte(fp);
		ph->BytesPerLine = PHYSFSX_readShort(fp);
		PHYSFS_read(fp, &ph->filler, 60, 1);
	}
	return i;
}

int bald_guy_load( char * filename, grs_bitmap * bmp,int bitmap_type ,ubyte * palette )
{
	PCXHeader header;
	PHYSFS_file * PCXfile;
	int i, row, col, count, fsize;
	ubyte data, c, xor_value, *pixdata;
	ubyte *bguy_data, *bguy_data1, *p;
	unsigned int xsize;
	unsigned int ysize;
	
	PCXfile = PHYSFSX_openReadBuffered( filename );
	if ( !PCXfile )
		return PCX_ERROR_OPENING;
	
	PHYSFSX_fseek(PCXfile, -1, SEEK_END);
	fsize = PHYSFS_tell(PCXfile);
	PHYSFS_read(PCXfile, &xor_value, 1, 1);
	xor_value--;
	PHYSFSX_fseek(PCXfile, 0, SEEK_SET);
	
	bguy_data = (ubyte *)d_malloc(fsize);
	bguy_data1 = (ubyte *)d_malloc(fsize);
	
	PHYSFS_read(PCXfile, bguy_data1, 1, fsize);
	
	for (i = 0; i < fsize; i++) {
		c = bguy_data1[fsize - i - 1] ^ xor_value;
		bguy_data[i] = c;
		xor_value--;
	}
	PHYSFS_close(PCXfile);
	d_free(bguy_data1);
	
	p = bguy_data;
	memcpy( &header, p, sizeof(PCXHeader) );
	p += sizeof(PCXHeader);
	
	// Is it a 256 color PCX file?
	if ((header.Manufacturer != 10)||(header.Encoding != 1)||(header.Nplanes != 1)||(header.BitsPerPixel != 8)||(header.Version != 5))	{
		d_free(bguy_data);
		return PCX_ERROR_WRONG_VERSION;
	}
	header.Xmin= INTEL_SHORT(header.Xmin);
	header.Xmax = INTEL_SHORT(header.Xmax);
	header.Ymin = INTEL_SHORT(header.Ymin);
	header.Ymax = INTEL_SHORT(header.Ymax);
	
	// Find the size of the image
	xsize = header.Xmax - header.Xmin + 1;
	ysize = header.Ymax - header.Ymin + 1;
	
	if ( bmp->bm_data == NULL )	{
		memset( bmp, 0, sizeof( grs_bitmap ) );
		bmp->bm_data = d_malloc( xsize * ysize );
		if ( bmp->bm_data == NULL )	{
			d_free(bguy_data);
			return PCX_ERROR_MEMORY;
		}
		bmp->bm_w = bmp->bm_rowsize = xsize;
		bmp->bm_h = ysize;
		bmp->bm_type = bitmap_type;
	}
	
	for (row=0; row< ysize ; row++)      {
		for (row=0; row< ysize ; row++)      {
			pixdata = &bmp->bm_data[bmp->bm_rowsize*row];
			for (col=0; col< xsize ; )      {
				data = *p;
				p++;
				if ((data & 0xC0) == 0xC0)     {
					count =  data & 0x3F;
					data = *p;
					p++;
					memset( pixdata, data, count );
					pixdata += count;
					col += count;
				} else {
					*pixdata++ = data;
					col++;
				}
			}
		}
	}
	
	
	// Read the extended palette at the end of PCX file
	// Read in a character which should be 12 to be extended palette file
	
	p++;
	if (palette != NULL) {
		for (i = 0; i < 768; i++) {
			palette[i] = *p;
			palette[i] >>= 2;
			p++;
		}
	}
	
	
	d_free(bguy_data);
	
	return PCX_ERROR_NONE;
}

int pcx_read_bitmap( char * filename, grs_bitmap * bmp,int bitmap_type ,ubyte * palette )
{
	PCXHeader header;
	PHYSFS_file * PCXfile;
	int i, row, col, count, xsize, ysize;
	ubyte data, *pixdata;

	PCXfile = PHYSFSX_openReadBuffered( filename );
	if ( !PCXfile )
		return PCX_ERROR_OPENING;

	// read 128 char PCX header
	if (PCXHeader_read_n( &header, 1, PCXfile )!=1) {
		PHYSFS_close( PCXfile );
		return PCX_ERROR_NO_HEADER;
	}

	// Is it a 256 color PCX file?
	if ((header.Manufacturer != 10)||(header.Encoding != 1)||(header.Nplanes != 1)||(header.BitsPerPixel != 8)||(header.Version != 5))	{
		PHYSFS_close( PCXfile );
		return PCX_ERROR_WRONG_VERSION;
	}

	// Find the size of the image
	xsize = header.Xmax - header.Xmin + 1;
	ysize = header.Ymax - header.Ymin + 1;

	if ( bitmap_type == BM_LINEAR )	{
		if ( bmp->bm_data == NULL )	{
			gr_init_bitmap_alloc (bmp, bitmap_type, 0, 0, xsize, ysize, xsize);
		}
	}

	if ( bmp->bm_type == BM_LINEAR )	{
		for (row=0; row< ysize ; row++)      {
			pixdata = &bmp->bm_data[bmp->bm_rowsize*row];
			for (col=0; col< xsize ; )      {
				if (PHYSFS_read( PCXfile, &data, 1, 1 )!=1 )	{
					PHYSFS_close( PCXfile );
					return PCX_ERROR_READING;
				}
				if ((data & 0xC0) == 0xC0)     {
					count =  data & 0x3F;
					if (PHYSFS_read( PCXfile, &data, 1, 1 )!=1 )	{
						PHYSFS_close( PCXfile );
						return PCX_ERROR_READING;
					}
					memset( pixdata, data, count );
					pixdata += count;
					col += count;
				} else {
					*pixdata++ = data;
					col++;
				}
			}
		}
	} else {
		for (row=0; row< ysize ; row++)      {
			for (col=0; col< xsize ; )      {
				if (PHYSFS_read( PCXfile, &data, 1, 1 )!=1 )	{
					PHYSFS_close( PCXfile );
					return PCX_ERROR_READING;
				}
				if ((data & 0xC0) == 0xC0)     {
					count =  data & 0x3F;
					if (PHYSFS_read( PCXfile, &data, 1, 1 )!=1 )	{
						PHYSFS_close( PCXfile );
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
		if (PHYSFS_read( PCXfile, &data, 1, 1 )==1)	{
			if ( data == 12 )	{
				if (PHYSFS_read(PCXfile, palette,768, 1)!=1)	{
					PHYSFS_close( PCXfile );
					return PCX_ERROR_READING;
				}
				for (i=0; i<768; i++ )
					palette[i] >>= 2;
			}
		} else {
			PHYSFS_close( PCXfile );
			return PCX_ERROR_NO_PALETTE;
		}
	}
	PHYSFS_close(PCXfile);
	return PCX_ERROR_NONE;
}

int pcx_write_bitmap( char * filename, grs_bitmap * bmp, ubyte * palette )
{
	int retval;
	int i;
	ubyte data;
	PCXHeader header;
	PHYSFS_file *PCXfile;

	memset( &header, 0, PCXHEADER_SIZE );

	header.Manufacturer = 10;
	header.Encoding = 1;
	header.Nplanes = 1;
	header.BitsPerPixel = 8;
	header.Version = 5;
	header.Xmax = bmp->bm_w-1;
	header.Ymax = bmp->bm_h-1;
	header.BytesPerLine = bmp->bm_w;

	PCXfile = PHYSFSX_openWriteBuffered(filename);
	if ( !PCXfile )
		return PCX_ERROR_OPENING;

	if (PHYSFS_write(PCXfile, &header, PCXHEADER_SIZE, 1) != 1)
	{
		PHYSFS_close(PCXfile);
		return PCX_ERROR_WRITING;
	}

	for (i=0; i<bmp->bm_h; i++ )	{
		if (!pcx_encode_line( &bmp->bm_data[bmp->bm_rowsize*i], bmp->bm_w, PCXfile ))	{
			PHYSFS_close(PCXfile);
			return PCX_ERROR_WRITING;
		}
	}

	// Mark an extended palette
	data = 12;
	if (PHYSFS_write(PCXfile, &data, 1, 1) != 1)
	{
		PHYSFS_close(PCXfile);
		return PCX_ERROR_WRITING;
	}

	// Write the extended palette
	for (i=0; i<768; i++ )
		palette[i] <<= 2;

	retval = PHYSFS_write(PCXfile, palette, 768, 1);

	for (i=0; i<768; i++ )
		palette[i] >>= 2;

	if (retval !=1)	{
		PHYSFS_close(PCXfile);
		return PCX_ERROR_WRITING;
	}

	PHYSFS_close(PCXfile);
	return PCX_ERROR_NONE;

}

// returns number of bytes written into outBuff, 0 if failed
int pcx_encode_line(ubyte *inBuff, int inLen, PHYSFS_file *fp)
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
int pcx_encode_byte(ubyte byt, ubyte cnt, PHYSFS_file *fid)
{
	if (cnt) {
		if ( (cnt==1) && (0xc0 != (0xc0 & byt)) )	{
			if(EOF == PHYSFSX_putc(fid, (int)byt))
				return 0; 	// disk write error (probably full)
			return 1;
		} else {
			if(EOF == PHYSFSX_putc(fid, (int)0xC0 | cnt))
				return 0; 	// disk write error
			if(EOF == PHYSFSX_putc(fid, (int)byt))
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
