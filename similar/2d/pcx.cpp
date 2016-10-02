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
#include "palette.h"

#include "dxxsconf.h"
#include "dsx-ns.h"
#include "compiler-lengthof.h"
#include "compiler-range_for.h"
#include "partial_range.h"

namespace dcx {

static int pcx_encode_byte(ubyte byt, ubyte cnt, PHYSFS_File *fid);
static int pcx_encode_line(const uint8_t *inBuff, uint_fast32_t inLen, PHYSFS_File *fp);

/* PCX Header data type */
struct PCXHeader
{
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
} __pack__;

#define PCXHEADER_SIZE 128

/*
 * reads n PCXHeader structs from a PHYSFS_File
 */
static int PCXHeader_read_n(PCXHeader *ph, int n, PHYSFS_File *fp)
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
		PHYSFS_read(fp, &ph->ColorMap, 16*3, 1);
		ph->Reserved = PHYSFSX_readByte(fp);
		ph->Nplanes = PHYSFSX_readByte(fp);
		ph->BytesPerLine = PHYSFSX_readShort(fp);
		PHYSFS_read(fp, &ph->filler, 60, 1);
	}
	return i;
}

}

#if defined(DXX_BUILD_DESCENT_I)
namespace dsx {

int bald_guy_load(const char *const filename, grs_bitmap *const bmp, palette_array_t &palette)
{
	PCXHeader header;
	int i, count, fsize;
	ubyte data, c, xor_value;
	unsigned int row, xsize;
	unsigned int col, ysize;
	
	auto PCXfile = PHYSFSX_openReadBuffered(filename);
	if ( !PCXfile )
		return PCX_ERROR_OPENING;
	
	PHYSFSX_fseek(PCXfile, -1, SEEK_END);
	fsize = PHYSFS_tell(PCXfile);
	PHYSFS_read(PCXfile, &xor_value, 1, 1);
	xor_value--;
	PHYSFSX_fseek(PCXfile, 0, SEEK_SET);
	
	RAIIdmem<uint8_t[]> bguy_data, bguy_data1;
	MALLOC(bguy_data, uint8_t[], fsize);
	MALLOC(bguy_data1, uint8_t[], fsize);
	
	PHYSFS_read(PCXfile, bguy_data1, 1, fsize);
	
	for (i = 0; i < fsize; i++) {
		c = bguy_data1[fsize - i - 1] ^ xor_value;
		bguy_data[i] = c;
		xor_value--;
	}
	PCXfile.reset();
	auto p = bguy_data.get();
	memcpy( &header, p, sizeof(PCXHeader) );
	p += sizeof(PCXHeader);
	
	// Is it a 256 color PCX file?
	if ((header.Manufacturer != 10)||(header.Encoding != 1)||(header.Nplanes != 1)||(header.BitsPerPixel != 8)||(header.Version != 5))	{
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
		*bmp = {};
		MALLOC(bmp->bm_mdata, unsigned char, xsize * ysize );
		if ( bmp->bm_data == NULL )	{
			return PCX_ERROR_MEMORY;
		}
		bmp->bm_w = bmp->bm_rowsize = xsize;
		bmp->bm_h = ysize;
		bmp->set_type(bm_mode::linear);
	}
	
	for (row=0; row< ysize ; row++)      {
		auto pixdata = &bmp->get_bitmap_data()[bmp->bm_rowsize*row];
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
	
	
	// Read the extended palette at the end of PCX file
	// Read in a character which should be 12 to be extended palette file
	
	p++;
	copy_diminish_palette(palette, p);
	return PCX_ERROR_NONE;
}

}
#endif

namespace dcx {

struct PCX_PHYSFS_file
{
	RAIIPHYSFS_File PCXfile;
};

static int pcx_read_bitmap_file(struct PCX_PHYSFS_file *const pcxphysfs, grs_bitmap &bmp, bm_mode bitmap_type, palette_array_t &palette);

int pcx_read_bitmap(const char *const filename, grs_bitmap &bmp, palette_array_t &palette)
{
	int result;
	PCX_PHYSFS_file pcxphysfs{PHYSFSX_openReadBuffered(filename)};
	if (!pcxphysfs.PCXfile)
		return PCX_ERROR_OPENING;
	result = pcx_read_bitmap_file(&pcxphysfs, bmp, bm_mode::linear, palette);
	return result;
}

static int PCX_PHYSFS_read(struct PCX_PHYSFS_file *pcxphysfs, ubyte *data, unsigned size)
{
	return PHYSFS_read(pcxphysfs->PCXfile, data, size, sizeof(*data));
}

static int pcx_read_bitmap_file(struct PCX_PHYSFS_file *const pcxphysfs, grs_bitmap &bmp, const bm_mode bitmap_type, palette_array_t &palette)
{
	PCXHeader header;
	int i, row, col, count, xsize, ysize;
	ubyte data;

	// read 128 char PCX header
	if (PCXHeader_read_n( &header, 1, pcxphysfs->PCXfile )!=1) {
		return PCX_ERROR_NO_HEADER;
	}

	// Is it a 256 color PCX file?
	if ((header.Manufacturer != 10)||(header.Encoding != 1)||(header.Nplanes != 1)||(header.BitsPerPixel != 8)||(header.Version != 5))	{
		return PCX_ERROR_WRONG_VERSION;
	}

	// Find the size of the image
	xsize = header.Xmax - header.Xmin + 1;
	ysize = header.Ymax - header.Ymin + 1;

	if ( bitmap_type == bm_mode::linear )	{
		if ( bmp.bm_data == NULL )	{
			gr_init_bitmap_alloc(bmp, bitmap_type, 0, 0, xsize, ysize, xsize);
		}
	}

	if (bmp.get_type() == bm_mode::linear)
	{
		for (row=0; row< ysize ; row++)      {
			auto pixdata = &bmp.get_bitmap_data()[bmp.bm_rowsize*row];
			for (col=0; col< xsize ; )      {
				if (PCX_PHYSFS_read(pcxphysfs, &data, 1) != 1)	{
					return PCX_ERROR_READING;
				}
				if ((data & 0xC0) == 0xC0)     {
					count =  data & 0x3F;
					if (PCX_PHYSFS_read(pcxphysfs, &data, 1) != 1)	{
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
				if (PCX_PHYSFS_read(pcxphysfs, &data, 1) != 1)	{
					return PCX_ERROR_READING;
				}
				if ((data & 0xC0) == 0xC0)     {
					count =  data & 0x3F;
					if (PCX_PHYSFS_read(pcxphysfs, &data, 1) != 1)	{
						return PCX_ERROR_READING;
					}
					for (i=0;i<count;i++)
						gr_bm_pixel(bmp, col+i, row, data );
					col += count;
				} else {
					gr_bm_pixel(bmp, col, row, data );
					col++;
				}
			}
		}
	}

	// Read the extended palette at the end of PCX file
		// Read in a character which should be 12 to be extended palette file
		if (PCX_PHYSFS_read(pcxphysfs, &data, 1) == 1)	{
			if ( data == 12 )	{
				if (PCX_PHYSFS_read(pcxphysfs, reinterpret_cast<ubyte *>(&palette[0]), palette.size() * sizeof(palette[0])) != 1)	{
					return PCX_ERROR_READING;
				}
				diminish_palette(palette);
			}
		} else {
			return PCX_ERROR_NO_PALETTE;
		}
	return PCX_ERROR_NONE;
}

int pcx_write_bitmap(const char *const filename, const grs_bitmap *const bmp, palette_array_t &palette)
{
	int retval;
	ubyte data;
	PCXHeader header{};

	header.Manufacturer = 10;
	header.Encoding = 1;
	header.Nplanes = 1;
	header.BitsPerPixel = 8;
	header.Version = 5;
	header.Xmax = bmp->bm_w-1;
	header.Ymax = bmp->bm_h-1;
	header.BytesPerLine = bmp->bm_w;

	auto PCXfile = PHYSFSX_openWriteBuffered(filename);
	if ( !PCXfile )
		return PCX_ERROR_OPENING;

	if (PHYSFS_write(PCXfile, &header, PCXHEADER_SIZE, 1) != 1)
	{
		return PCX_ERROR_WRITING;
	}

	{
		const uint_fast32_t bm_w = bmp->bm_w;
		const uint_fast32_t bm_rowsize = bmp->bm_rowsize;
		const auto bm_data = bmp->get_bitmap_data();
		const auto e = &bm_data[bm_rowsize * bmp->bm_h];
		for (auto i = &bm_data[0]; i != e; i += bm_rowsize)
		{
			if (!pcx_encode_line(i, bm_w, PCXfile))
			{
			return PCX_ERROR_WRITING;
			}
		}
	}

	// Mark an extended palette
	data = 12;
	if (PHYSFS_write(PCXfile, &data, 1, 1) != 1)
	{
		return PCX_ERROR_WRITING;
	}

	// Write the extended palette
	range_for (auto &i, palette)
	{
		i.r <<= 2;
		i.g <<= 2;
		i.b <<= 2;
	}

	retval = PHYSFS_write(PCXfile, &palette[0], sizeof(palette), 1);
	if (retval !=1)	{
		return PCX_ERROR_WRITING;
	}
	return PCX_ERROR_NONE;
}

// returns number of bytes written into outBuff, 0 if failed
int pcx_encode_line(const uint8_t *inBuff, uint_fast32_t inLen, PHYSFS_File *fp)
{
	ubyte last;
	int i;
	register int total;
	register ubyte runCount; 	// max single runlength is 63
	total = 0;
	last = *(inBuff);
	runCount = 1;

	range_for (const auto ub, unchecked_partial_range(inBuff, 1u, inLen))
	{
		if (ub == last)	{
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
			last = ub;
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

static inline int PHYSFSX_putc(PHYSFS_File *file, uint8_t ch)
{
	if (PHYSFS_write(file, &ch, 1, 1) < 1)
		return -1;
	else
		return ch;
}

// subroutine for writing an encoded byte pair
// returns count of bytes written, 0 if error
int pcx_encode_byte(ubyte byt, ubyte cnt, PHYSFS_File *fid)
{
	if (cnt) {
		if ( (cnt==1) && (0xc0 != (0xc0 & byt)) )	{
			if(EOF == PHYSFSX_putc(fid, static_cast<int>(byt)))
				return 0; 	// disk write error (probably full)
			return 1;
		} else {
			if(EOF == PHYSFSX_putc(fid, 0xC0 | cnt))
				return 0; 	// disk write error
			if(EOF == PHYSFSX_putc(fid, static_cast<int>(byt)))
				return 0; 	// disk write error
			return 2;
		}
	}
	return 0;
}

//text for error messges
constexpr char pcx_error_messages[] = {
	"No error.\0"
	"Error opening file.\0"
	"Couldn't read PCX header.\0"
	"Unsupported PCX version.\0"
	"Error reading data.\0"
	"Couldn't find palette information.\0"
	"Error writing data.\0"
};


//function to return pointer to error message
const char *pcx_errormsg(int error_number)
{
	const char *p = pcx_error_messages;

	while (error_number--) {

		if (p == pcx_error_messages + lengthof(pcx_error_messages)) return NULL;

		p += strlen(p)+1;

	}

	return p;
}

}
